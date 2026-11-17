#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The two ways of doing a round in solve_slice_hybrid (egdb_6x6), measured.

A round n of a slice can be done two ways from the same state, with exactly the same result:
  forward     every undecided position (in a loss round: whose exits all lose) examines its own king moves;
  retrograde  the frontier (the positions with value n - 1) generates its predecessors; in a loss round with counts.
Both walk work lists only (no scan of the slice), so the least work for a slice is the sum over its rounds of
min(forward, retrograde). A build with EGDB_PROFILE=file does every round both ways from the same state, times both and
checks them. Each line of the file is one round:
    wp bp wm bm slice T n U F edges 0 forward_ns retrograde_ns
with T the positions of the slice, U the positions the forward round examines, F the frontier and edges the
predecessors generated.

This script (streaming, any size of profile)
  - fits the cost model (least squares, nanoseconds):
        forward    k_f + c U
        retrograde k_r + w F (odd n), k_r + l F (even n)
    and prints it in units of c as EGDB_COSTS="k_f k_r r_w r_l" (r_w = w / c, r_l = l / c); kappa = c / w;
  - compares, by class of material and in total: forward only, retrograde only, the best single switch per slice
    (forward rounds, then retrograde), the optimum per round (sum of min) and the model choosing per round;
  - with --materials, lists per material the same totals and the share of rounds in which forward is cheaper.

usage: switch.py profile.txt [--costs "k_f k_r r_w r_l"] [--materials]
"""

import argparse
from collections import defaultdict

import numpy as np
import pandas as pd

COLS = ['wp', 'bp', 'wm', 'bm', 'slice', 'S', 'n', 'U', 'F', 'E', 'C', 'f', 'r']
TYPES = dict(wp='int8', bp='int8', wm='int8', bm='int8', slice='int64', S='int64', n='int64', U='int64', F='int64',
             E='int64', C='int64', f='int64', r='int64')
CHUNK = 4_000_000
CLASSES = ('kings only', 'one man', 'two or more men')
KEYS = ('forward', 'retrograde', 'single', 'opt', 'model')


def chunks(path):
    for d in pd.read_csv(path, sep=' ', header=None, names=COLS, dtype=TYPES, chunksize=CHUNK, engine='c',
                         on_bad_lines='skip'):
        d['mat'] = ((d.wp.astype('int64') * 16 + d.bp) * 16 + d.wm) * 16 + d.bm
        d['key'] = d.mat * (1 << 32) + d.slice
        men = (d.wm + d.bm).to_numpy()
        d['cls'] = np.where(men == 0, 0, np.where(men == 1, 1, 2))
        d['odd'] = (d.n % 2).astype('int64')
        yield d


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


def account(d, tot, costs):
    """add the strategies of the complete slices in d to the totals per material"""
    if len(d) == 0:
        return
    kf, kr, rw, rl = costs
    pred_f = kf + d.U
    pred_r = kr + np.where(d.odd == 1, rw, rl) * d.F
    d = d.assign(opt=np.minimum(d.f, d.r), model=np.where(pred_r < pred_f, d.r, d.f), fc=(d.f < d.r).astype('int64'))
    g = d.groupby('key', sort=False)
    # single switch before round s: forward rounds before it, retrograde from it on
    d['single'] = g.f.cumsum() - d.f + g.r.transform('sum') - g.r.cumsum() + d.r
    per = d.groupby('key', sort=False).agg(mat=('mat', 'first'), cls=('cls', 'first'), forward=('f', 'sum'),
                                           retrograde=('r', 'sum'), single=('single', 'min'), opt=('opt', 'sum'),
                                           model=('model', 'sum'), rounds=('n', 'size'), fc=('fc', 'sum'))
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
    ap.add_argument('--costs', help='"k_f k_r r_w r_l" instead of the fitted constants')
    ap.add_argument('--materials', action='store_true', help='one line per material')
    a = ap.parse_args()

    # pass 1: the fits (all, and per class) and a few sums
    fits = {c: (Normal(2), Normal(3)) for c in ('all',) + CLASSES}
    sums = defaultdict(int)
    for d in chunks(a.profile):
        for c, g in [('all', d)] + [(CLASSES[k], d[d.cls == k]) for k in range(3)]:
            if len(g) == 0:
                continue
            o = np.ones(len(g))
            fits[c][0].add(np.column_stack([o, g.U]), g.f)
            fits[c][1].add(np.column_stack([o, g.F * g.odd, g.F * (1 - g.odd)]), g.r)
        sums['rounds'] += len(d)
        sums['E'] += int(d.E.sum())
        sums['F'] += int(d.F.sum())
    k_f, c = fits['all'][0].solve()
    k_r, w, l = fits['all'][1].solve()
    print(f'{sums["rounds"]:,} rounds')
    print(f'forward:    {k_f:8.0f} ns per round + {c:6.1f} ns per examined position')
    print(f'retrograde: {k_r:8.0f} ns per round + {w:6.1f} ns per frontier position (odd n), {l:.1f} (even n, with counts)')
    print(f'            predecessors per frontier position b_b = {sums["E"] / max(1, sums["F"]):.2f}')
    fitted = [k_f / c, k_r / c, w / c, l / c]
    print(f'kappa = c / w = {c / w:.2f};  EGDB_COSTS="{" ".join(f"{v:.4g}" for v in fitted)}"')
    for cl in CLASSES:
        if fits[cl][0].A[0, 0] == 0:
            continue
        kk, cc = fits[cl][0].solve()
        kr2, ww, ll = fits[cl][1].solve()
        print(f'  {cl:16} k_f {kk:7.0f}  c {cc:6.1f}  k_r {kr2:7.0f}  w {ww:6.1f}  l {ll:6.1f}  kappa {cc / ww:.2f}')
    costs = [float(v) for v in a.costs.split()] if a.costs else fitted
    if a.costs:
        print(f'model with EGDB_COSTS="{a.costs}"')

    # pass 2: the strategies per material (the rounds of a slice are consecutive within the lines of its material)
    tot = defaultdict(lambda: defaultdict(float))
    carry = None
    for d in chunks(a.profile):
        if carry is not None:
            d = pd.concat([carry, d], ignore_index=True)
        hold = d.key == d.groupby('mat').key.transform('last')     # the last slice of a material may go on
        carry = d[hold]
        account(d[~hold], tot, costs)
    if carry is not None:
        account(carry, tot, costs)

    def line(label, x):
        s = {k: x[k] / 1e9 for k in KEYS}
        print(f'  {label:16} {int(x["slices"]):,} slices, {int(x["rounds"]):,} rounds, forward cheaper in '
              f'{100 * x["fc"] / max(1, x["rounds"]):.1f}%')
        print(f'  {"":16} forward {s["forward"]:.1f}  retrograde {s["retrograde"]:.1f}  best single switch {s["single"]:.1f}  '
              f'optimum per round {s["opt"]:.1f}  model per round {s["model"]:.1f} ({100 * (s["model"] / s["opt"] - 1):+.1f}%)')

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
        print('\nmaterial     slices   rounds  fwd cheaper  seconds: forward retrograde   single  optimum    model')
        for m in sorted(tot, key=lambda m: ((m >> 12) + (m >> 8 & 15), (m >> 4 & 15) + (m & 15), m)):
            t = tot[m]
            print(f'{name(m)}  {int(t["slices"]):7}  {int(t["rounds"]):7}  {100 * t["fc"] / max(1, t["rounds"]):10.1f}%  '
                  + ' '.join(f'{t[k] / 1e9:8.2f}' for k in KEYS))


if __name__ == '__main__':
    main()
