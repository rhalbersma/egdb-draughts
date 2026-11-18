#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The two ways of doing a round in solve_slice_hybrid (egdb_6x6), measured, and the rule that chooses between them.

A round n of a slice can be done two ways from the same state, with exactly the same result:
  forward     every undecided position (in a loss round: whose exits all lose) examines its own king moves;
  retrograde  the frontier (the positions with value n - 1) generates its predecessors; in a loss round with counts.
Both walk work lists only (no scan of the slice), so the least work for a slice is the sum over its rounds of
min(forward, retrograde). A build with EGDB_PROFILE=file does every round both ways from the same state, times both and
checks them (the state, and the lists, then follow the retrograde round). Each line of the file is one round:
    wp bp wm bm slice T n U F edges L X forward_ns retrograde_ns
with T the positions of the slice, U the positions the forward round examines, L the length of the lists it walks (U and
the positions decided by retrograde rounds since a forward round last compacted them), F the frontier, X the wins
through an exit in n and edges the predecessors generated. (Older profiles have 0 for L and no X: L = U, X = 0.)

The engine does a round retrograde if
    k_r + r_w F + r_x X  (odd n)  or  k_r + r_l F  (even n)   <   k_f + U + r_s (L - U)
with EGDB_COSTS="k_f k_r r_w r_l r_s r_x" (in units of the cost of examining one position forward). Only k_f - k_r
matters. A least-squares fit of the times is dominated by the largest rounds and chooses badly in the many small ones, so
this script fits the rule itself: the constants that minimise the total time of the rounds it chooses (exact coordinate
descent on an importance sample of the rounds: a round of t ns is kept with probability min(1, t / 1 ms) and weighted
by its inverse). It prints
  - the least-squares cost per position of each kind of work (for kappa = c / w), and the fitted rule;
  - by class of material and in total: forward only, retrograde only, the best single switch per slice (forward rounds,
    then retrograde), the optimum per round (sum of min), the rule and the rule fitted per class of material;
  - with --materials, the same per material and the share of rounds in which forward is cheaper.

usage: switch.py profile.txt [--costs "k_f k_r r_w r_l r_s r_x"] [--materials]
"""

import argparse
from collections import defaultdict

import numpy as np
import pandas as pd

COLS = ['wp', 'bp', 'wm', 'bm', 'slice', 'S', 'n', 'U', 'F', 'E', 'L', 'X', 'f', 'r']
OLD = ['wp', 'bp', 'wm', 'bm', 'slice', 'S', 'n', 'U', 'F', 'E', 'L', 'f', 'r']
TYPES = dict(wp='int8', bp='int8', wm='int8', bm='int8', slice='int64', S='int64', n='int64', U='int64', F='int64',
             E='int64', L='int64', X='int64', f='int64', r='int64')
CHUNK = 4_000_000
TAU = 1e6                                       # importance sampling: rounds of at least 1 ms are all kept
CLASSES = ('kings only', 'one man', 'two or more men')
KEYS = ('forward', 'retrograde', 'single', 'opt', 'rule', 'crule')


def chunks(path):
    with open(path) as f:
        old = len(f.readline().split()) == len(OLD)
    names = OLD if old else COLS
    for d in pd.read_csv(path, sep=' ', header=None, names=names, dtype={k: TYPES[k] for k in names}, chunksize=CHUNK,
                         engine='c', on_bad_lines='skip'):
        if old:
            d['L'] = d.U
            d['X'] = 0
        d['mat'] = ((d.wp.astype('int64') * 16 + d.bp) * 16 + d.wm) * 16 + d.bm
        d['key'] = d.mat * (1 << 32) + d.slice
        men = (d.wm + d.bm).to_numpy()
        d['cls'] = np.where(men == 0, 0, np.where(men == 1, 1, 2))
        d['odd'] = (d.n % 2).astype('int64')
        yield d


def features(d):
    """retrograde iff th . features > 0, th = [k_f - k_r, 1, r_s, -r_w, -r_l, -r_x]"""
    odd = d.odd.to_numpy().astype(bool)
    F = d.F.to_numpy(float)
    return np.column_stack([np.ones(len(d)), d.U.to_numpy(float), (d.L - d.U).to_numpy(float), F * odd, F * ~odd,
                            d.X.to_numpy(float) * odd])


def theta(costs):
    kf, kr, rw, rl, rs, rx = costs
    return np.array([kf - kr, 1.0, rs, -rw, -rl, -rx])


def costs_of(th):
    return [th[0], 0.0, -th[3], -th[4], th[2], -th[5]]


def choose(d, costs):
    """the time of the round the rule chooses"""
    return np.where(features(d) @ theta(costs) > 0, d.r, d.f)


class Sample:
    """the importance sample of the rounds, and the rule fitted on it"""
    def __init__(self):
        self.parts = []

    def add(self, d, rng):
        t = (d.f + d.r).to_numpy(float)
        p = np.minimum(1.0, t / TAU)
        keep = rng.random(len(d)) < p
        g = d[keep]
        self.parts.append((features(g), g.f.to_numpy(float), g.r.to_numpy(float), g.cls.to_numpy(), 1.0 / p[keep]))

    def done(self):
        A, f, r, cls, w = (np.concatenate(x) for x in zip(*self.parts))
        self.A, self.f, self.r, self.cls, self.w = A, f, r, cls, w
        self.parts = []

    def cost(self, th, m):
        return (self.w[m] * np.where(self.A[m] @ th > 0, self.r[m], self.f[m])).sum()

    def coord(self, th, j, m):
        """the exact minimum over th[j] of the total time, the others fixed: the decision of a round flips at one value"""
        a = self.A[m, j]
        nz = a != 0
        if not nz.any():
            return th[j]
        b = (self.A[m] @ th - a * th[j])[nz]
        a = a[nz]
        gain = (self.w[m] * (self.r[m] - self.f[m]))[nz]       # change in time when a round goes retrograde
        t0 = -b / a
        o = np.argsort(t0)
        t0, gain, pos = t0[o], gain[o], a[o] > 0
        # far left: rounds with a > 0 forward, a < 0 retrograde; passing t0 flips one round
        c = np.concatenate([[np.where(pos, 0.0, gain).sum()], np.where(pos, gain, -gain)]).cumsum()
        i = int(np.argmin(c))
        if i == 0:
            return t0[0] - abs(t0[0]) * 1e-6 - 1e-9
        if i == len(t0):
            return t0[-1] + abs(t0[-1]) * 1e-6 + 1e-9
        return (t0[i - 1] + t0[i]) / 2

    def fit(self, starts, m):
        best = None
        for s in starts:
            th = theta(s)
            for _ in range(8):
                before = self.cost(th, m)
                for j in (0, 2, 3, 4, 5):
                    th[j] = self.coord(th, j, m)
                if self.cost(th, m) >= before * (1 - 1e-6):
                    break
            if best is None or self.cost(th, m) < self.cost(best, m):
                best = th.copy()
        return costs_of(best)


class Normal:
    """least squares accumulated over chunks: X^T X b = X^T y"""
    def __init__(self, k):
        self.A = np.zeros((k, k))
        self.b = np.zeros(k)

    def add(self, X, y):
        X = np.asarray(X, dtype=float)
        self.A += X.T @ X
        self.b += X.T @ np.asarray(y, dtype=float)

    def solve(self):
        return np.linalg.lstsq(self.A, self.b, rcond=None)[0]


def name(m):
    m = int(m)
    return f'{m >> 12:02}-{m >> 8 & 15:02}-{m >> 4 & 15:02}-{m & 15:02}'


def show(costs):
    return ' '.join(f'{v:.4g}' for v in costs)


def account(d, tot, costs, ccosts):
    """add the strategies of the complete slices in d to the totals per material"""
    if len(d) == 0:
        return
    cr = np.zeros(len(d))
    for k in range(3):
        m = (d.cls == k).to_numpy()
        if m.any():
            cr[m] = choose(d[m], ccosts[k])
    d = d.assign(opt=np.minimum(d.f, d.r), rule=choose(d, costs), crule=cr, fc=(d.f < d.r).astype('int64'))
    g = d.groupby('key', sort=False)
    # single switch before round s: forward rounds before it, retrograde from it on
    d['single'] = g.f.cumsum() - d.f + g.r.transform('sum') - g.r.cumsum() + d.r
    per = d.groupby('key', sort=False).agg(mat=('mat', 'first'), cls=('cls', 'first'), forward=('f', 'sum'),
                                           retrograde=('r', 'sum'), single=('single', 'min'), opt=('opt', 'sum'),
                                           rule=('rule', 'sum'), crule=('crule', 'sum'), rounds=('n', 'size'),
                                           fc=('fc', 'sum'))
    per['single'] = np.minimum(per.single, per.forward)
    per['slices'] = 1
    for m, x in per.groupby('mat'):
        t = tot[int(m)]
        t['cls'] = int(x.cls.iloc[0])
        for k in KEYS + ('rounds', 'fc', 'slices'):
            t[k] += float(x[k].sum())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('profile')
    ap.add_argument('--costs', help='"k_f k_r r_w r_l r_s r_x": the rule to evaluate instead of the fitted one')
    ap.add_argument('--materials', action='store_true', help='one line per material')
    a = ap.parse_args()

    # pass 1: least squares (all, and per class), the sample and a few sums
    ls = {c: (Normal(3), Normal(4)) for c in ('all',) + CLASSES}
    sample = Sample()
    rng = np.random.default_rng(1)
    sums = defaultdict(int)
    for d in chunks(a.profile):
        for c, g in [('all', d)] + [(CLASSES[k], d[d.cls == k]) for k in range(3)]:
            if len(g) == 0:
                continue
            o = np.ones(len(g))
            ls[c][0].add(np.column_stack([o, g.U, g.L - g.U]), g.f)
            ls[c][1].add(np.column_stack([o, g.F * g.odd, g.F * (1 - g.odd), g.X * g.odd]), g.r)
        sample.add(d, rng)
        sums['rounds'] += len(d)
        sums['E'] += int(d.E.sum())
        sums['F'] += int(d.F.sum())
    sample.done()

    print(f'{sums["rounds"]:,} rounds, {len(sample.f):,} in the sample')
    print('least squares (ns):')
    starts = [[0, 0, 1, 1, 0, 0], [0, 0, 2.5, 1, 0, 0]]
    for c in ('all',) + CLASSES:
        if ls[c][0].A[0, 0] == 0:
            continue
        (kf, cf, s), (kr, w, l, x) = ls[c][0].solve(), ls[c][1].solve()
        print(f'  {c:16} forward {kf:9.0f} + {cf:6.1f} U + {s:5.1f} (L - U)   retrograde {kr:9.0f} + {w:6.1f} F + {x:5.1f} X'
              f' (odd n), {l:6.1f} F (even n)   kappa = c / w = {cf / w:.2f}')
        if c == 'all':
            print(f'  {"":16} predecessors per frontier position b_b = {sums["E"] / max(1, sums["F"]):.2f}')
            starts.append([kf / cf, kr / cf, w / cf, l / cf, s / cf, x / cf])
    everything = np.ones(len(sample.f), dtype=bool)
    fitted = sample.fit(starts, everything)
    print(f'rule fitted to the least time: EGDB_COSTS="{show(fitted)}"')
    ccosts = []
    for k, c in enumerate(CLASSES):
        m = sample.cls == k
        ccosts.append(sample.fit(starts + [fitted], m) if m.any() else fitted)
        print(f'  {c:16} EGDB_COSTS="{show(ccosts[-1])}"')
    costs = [float(v) for v in a.costs.split()] if a.costs else fitted
    costs += [0.0] * (6 - len(costs))
    if a.costs:
        print(f'rule evaluated: EGDB_COSTS="{show(costs)}"')

    # pass 2: the strategies per material (the rounds of a slice are consecutive within the lines of its material)
    tot = defaultdict(lambda: defaultdict(float))
    carry = None
    for d in chunks(a.profile):
        if carry is not None:
            d = pd.concat([carry, d], ignore_index=True)
        hold = d.key == d.groupby('mat').key.transform('last')     # the last slice of a material may go on
        carry = d[hold]
        account(d[~hold], tot, costs, ccosts)
    if carry is not None:
        account(carry, tot, costs, ccosts)

    def line(label, x):
        s = {k: x[k] / 1e9 for k in KEYS}
        print(f'  {label:16} {int(x["slices"]):,} slices, {int(x["rounds"]):,} rounds, forward cheaper in '
              f'{100 * x["fc"] / max(1, x["rounds"]):.1f}%')
        print(f'  {"":16} forward {s["forward"]:.1f}  retrograde {s["retrograde"]:.1f}  best single switch {s["single"]:.1f}  '
              f'optimum per round {s["opt"]:.1f}  rule {s["rule"]:.1f} ({100 * (s["rule"] / s["opt"] - 1):+.1f}%)  '
              f'rule per class {s["crule"]:.1f} ({100 * (s["crule"] / s["opt"] - 1):+.1f}%)')

    print('\nseconds:')
    for cl in range(3):
        x = defaultdict(float)
        for t in tot.values():
            if t['cls'] == cl:
                for k, v in t.items():
                    x[k] += v
        if x['rounds']:
            line(CLASSES[cl], x)
    x = defaultdict(float)
    for t in tot.values():
        for k, v in t.items():
            x[k] += v
    line('all', x)

    if a.materials:
        print('\nmaterial     slices   rounds  fwd cheaper  seconds: forward retrograde   single  optimum     rule per class')
        for m in sorted(tot, key=lambda m: ((m >> 12) + (m >> 8 & 15), (m >> 4 & 15) + (m & 15), m)):
            t = tot[m]
            print(f'{name(m)}  {int(t["slices"]):7}  {int(t["rounds"]):7}  {100 * t["fc"] / max(1, t["rounds"]):10.1f}%  '
                  + ' '.join(f'{t[k] / 1e9:8.2f}' for k in KEYS))


if __name__ == '__main__':
    main()
