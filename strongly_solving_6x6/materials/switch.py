#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The switch from forward to retrograde rounds in solve_slice_hybrid (egdb_6x6), measured.

A build with EGDB_PROFILE=file does every round of every slice both ways from the same state, times both and checks
that they decide the same positions. Each line of the file is one round:
    wp bp wm bm slice T n U F edges candidates forward_ns retrograde_ns
with T the positions of the slice, U the positions the forward round examines, F the frontier of the retrograde round
(the positions with value n - 1), edges the predecessors it generates and candidates the loss candidates it verifies.

This script
  - fits the cost model of the switch (least squares, in nanoseconds):
        forward    a_f T + c U
        retrograde a_r T + w F (odd n), a_r T + l F (even n)
    and prints the constants in units of c, as EGDB_COSTS="a_f a_r r_w r_l" (r_w = w / c, r_l = l / c);
    kappa = c / w is the forward cost per examined position over the retrograde cost per frontier position;
  - finds, per slice, the best single switch round s (forward rounds before s, retrograde from s on; s = 1 is
    retrograde only, s = R + 1 forward only) from the measured times, and the round the model picks;
  - lists the switching points per material (pieces, then men), with the frontier ratio F / U at the best switch, and
    the total time of forward only, retrograde only, the best switch and the model's switch.

usage: switch.py profile.txt [--costs "a_f a_r r_w r_l"] [--materials]
"""

import argparse
from collections import defaultdict
from statistics import median


def lstsq(rows, ys):
    """least squares for a few unknowns (normal equations, Gaussian elimination)"""
    k = len(rows[0])
    A = [[sum(r[i] * r[j] for r in rows) for j in range(k)] + [sum(r[i] * y for r, y in zip(rows, ys))] for i in range(k)]
    for i in range(k):
        p = max(range(i, k), key=lambda q: abs(A[q][i]))
        A[i], A[p] = A[p], A[i]
        for q in range(k):
            if q != i and A[i][i]:
                f = A[q][i] / A[i][i]
                A[q] = [a - f * b for a, b in zip(A[q], A[i])]
    return [A[i][k] / A[i][i] if A[i][i] else 0.0 for i in range(k)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('profile')
    ap.add_argument('--costs', help='"a_f a_r r_w r_l" instead of the fitted constants')
    ap.add_argument('--materials', action='store_true', help='one line per material')
    a = ap.parse_args()

    slices = defaultdict(list)          # (material, slice) -> rounds
    for line in open(a.profile):
        x = line.split()
        if len(x) != 13:
            continue
        mat = '-'.join(f'{int(t):02}' for t in x[:4])
        _, T, n, U, F, E, C, fns, rns = map(int, x[4:])
        slices[(mat, x[4])].append(dict(T=T, n=n, U=U, F=F, E=E, C=C, f=fns, r=rns))
    rounds = [r for rs in slices.values() for r in rs]
    print(f'{len(slices):,} slices, {len(rounds):,} rounds')

    # the cost model
    c_f = lstsq([[r['T'], r['U']] for r in rounds], [r['f'] for r in rounds])
    c_r = lstsq([[r['T'], r['F'] if r['n'] % 2 else 0, 0 if r['n'] % 2 else r['F']] for r in rounds], [r['r'] for r in rounds])
    a_f, c = c_f
    a_r, w, l = c_r
    fit = [a_f / c, a_r / c, w / c, l / c]
    detail = lstsq([[r['T'], r['F'], r['E'], r['C']] for r in rounds], [r['r'] for r in rounds])
    odd = [r for r in rounds if r['n'] % 2]
    even = [r for r in rounds if not r['n'] % 2]
    b_b = sum(r['E'] for r in rounds) / max(1, sum(r['F'] for r in rounds))
    gamma = sum(r['C'] for r in even) / max(1, sum(r['E'] for r in even))
    print(f'forward:    {a_f:.2f} ns per position of the slice + {c:.1f} ns per examined position')
    print(f'retrograde: {a_r:.2f} ns per position of the slice + {w:.1f} ns per frontier position (odd n), {l:.1f} (even n)')
    print(f'            = {detail[0]:.2f} T + {detail[1]:.1f} F + {detail[2]:.1f} per predecessor + {detail[3]:.1f} per candidate (ns)')
    print(f'            predecessors per frontier position b_b = {b_b:.2f}, candidates per predecessor (even n) gamma = {gamma:.3f}')
    print(f'kappa = c / w = {c / w:.2f};  measured EGDB_COSTS="{" ".join(f"{v:.4g}" for v in fit)}"')
    costs = [float(v) for v in a.costs.split()] if a.costs else fit
    if a.costs:
        print(f'model with EGDB_COSTS="{a.costs}"')

    def model_switch(rs):
        for i, r in enumerate(rs):
            ret = costs[1] * r['T'] + (costs[2] if r['n'] % 2 else costs[3]) * r['F']
            if ret < costs[0] * r['T'] + r['U']:
                return i
        return len(rs)

    # per slice: the best switch and the model's
    groups = defaultdict(lambda: defaultdict(float))
    best_at = defaultdict(list)
    ratio_at = defaultdict(list)
    tot = defaultdict(float)
    for (mat, _), rs in slices.items():
        rs.sort(key=lambda r: r['n'])
        f = [r['f'] for r in rs]
        r_ = [r['r'] for r in rs]
        cost = [sum(f[:s]) + sum(r_[s:]) for s in range(len(rs) + 1)]
        s = min(range(len(cost)), key=lambda i: cost[i])
        m = model_switch(rs)
        for key, v in (('forward', cost[-1]), ('retrograde', cost[0]), ('best', cost[s]), ('model', cost[m])):
            groups[mat][key] += v
            tot[key] += v
        groups[mat]['slices'] += 1
        best_at[mat].append(s + 1)
        if s < len(rs) and rs[s]['U']:
            ratio_at[mat].append(rs[s]['F'] / rs[s]['U'])

    def cls(mat):
        wp, bp, wm, bm = map(int, mat.split('-'))
        return 'kings only' if wm + bm == 0 else 'one man' if wm + bm == 1 else 'two or more men'

    print()
    print('switch round s (1 = retrograde from the start), best by the measured times, per class of material:')
    for k in ('kings only', 'one man', 'two or more men'):
        mats = [m for m in groups if cls(m) == k]
        if not mats:
            continue
        ss = [s for m in mats for s in best_at[m]]
        hist = defaultdict(int)
        for s in ss:
            hist[min(s, 6)] += 1
        h = ', '.join(f'{"s>=6" if s == 6 else f"s={s}"} {100 * c / len(ss):.0f}%' for s, c in sorted(hist.items()))
        t = {key: sum(groups[m][key] for m in mats) / 1e9 for key in ('forward', 'retrograde', 'best', 'model')}
        print(f'  {k:16} {len(ss):6} slices: {h}')
        print(f'  {"":16} seconds: forward only {t["forward"]:.1f}, retrograde only {t["retrograde"]:.1f}, '
              f'best switch {t["best"]:.1f}, model {t["model"]:.1f}')
    print(f'all seconds: forward only {tot["forward"] / 1e9:.1f}, retrograde only {tot["retrograde"] / 1e9:.1f}, '
          f'best switch {tot["best"] / 1e9:.1f}, model {tot["model"] / 1e9:.1f}')

    if a.materials:
        print()
        print('material     slices  best s: median  max   F/U at the best switch (median)   seconds: forward retrograde best model')
        for mat in sorted(groups, key=lambda m: (sum(map(int, m.split('-')[:2])), sum(map(int, m.split('-')[2:])), m)):
            g = groups[mat]
            ra = f'{median(ratio_at[mat]):.3f}' if ratio_at[mat] else '-'
            print(f'{mat}  {int(g["slices"]):6}  {median(best_at[mat]):13}  {max(best_at[mat]):4}   {ra:>33}   '
                  f'{g["forward"] / 1e9:8.2f} {g["retrograde"] / 1e9:8.2f} {g["best"] / 1e9:8.2f} {g["model"] / 1e9:8.2f}')


if __name__ == '__main__':
    main()
