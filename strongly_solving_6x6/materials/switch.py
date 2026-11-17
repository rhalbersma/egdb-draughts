#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The choice between a forward and a retrograde round in solve_slice_hybrid (egdb_6x6), measured.

A forward round and a retrograde round decide the same positions from the same state, so every round can be done
either way, and the least work for a slice is the sum over its rounds of min(forward, retrograde). A build with
EGDB_PROFILE=file does every round of every slice both ways from the same state, times both and checks them. Each line
of the file is one round:
    wp bp wm bm slice T n U F edges candidates forward_ns retrograde_ns
with T the positions of the slice, U the positions the forward round examines, F the frontier of the retrograde round
(the positions with value n - 1), edges the predecessors it generates and candidates the loss candidates it verifies.

This script
  - fits the cost model (least squares, nanoseconds):
        forward    k_f + a_f T + c U
        retrograde k_r + a_r T + w F (odd n), k_r + a_r T + l F (even n)
    and prints it in units of c as EGDB_COSTS="k_f k_r a_f a_r r_w r_l"; kappa = c / w (forward cost per examined
    position over retrograde cost per frontier position);
  - compares, by class of material and in total: forward only, retrograde only, the best single switch per slice
    (forward rounds, then retrograde), the optimum per round (sum of min), and the model choosing per round;
  - with --materials, lists per material the same totals, the share of rounds in which forward is cheaper and the
    median best single switch round.

usage: switch.py profile.txt [--costs "k_f k_r a_f a_r r_w r_l"] [--materials]
"""

import argparse

import numpy as np
import pandas as pd

COLS = ['wp', 'bp', 'wm', 'bm', 'slice', 'S', 'n', 'U', 'F', 'E', 'C', 'f', 'r']


def fit(d):
    odd = (d.n % 2).to_numpy()
    one = np.ones(len(d))
    (k_f, a_f, c), *_ = np.linalg.lstsq(np.column_stack([one, d.S, d.U]).astype(float), d.f.astype(float), rcond=None)
    (k_r, a_r, w, l), *_ = np.linalg.lstsq(np.column_stack([one, d.S, d.F * odd, d.F * (1 - odd)]).astype(float),
                                          d.r.astype(float), rcond=None)
    return dict(k_f=k_f, a_f=a_f, c=c, k_r=k_r, a_r=a_r, w=w, l=l)


def totals(d, costs):
    """per slice: forward only, retrograde only, best single switch, optimum per round, model per round (ns)"""
    k_f, k_r, a_f, a_r, r_w, r_l = costs
    pred_f = k_f + a_f * d.S + d.U
    pred_r = k_r + a_r * d.S + np.where(d.n % 2 == 1, r_w, r_l) * d.F
    d['opt'] = np.minimum(d.f, d.r)
    d['model'] = np.where(pred_r < pred_f, d.r, d.f)
    d['fwd_cheaper'] = (d.f < d.r).astype(int)
    g = d.groupby('key', sort=False)
    # single switch before round s: forward rounds before it, retrograde from it on
    d['single'] = g.f.cumsum() - d.f + g.r.transform('sum') - g.r.cumsum() + d.r
    per = g.agg(forward=('f', 'sum'), retrograde=('r', 'sum'), opt=('opt', 'sum'), model=('model', 'sum'),
                single=('single', 'min'), rounds=('n', 'size'), fwd_cheaper=('fwd_cheaper', 'sum'))
    per['single'] = np.minimum(per.single, per.forward)
    best_s = d.loc[g.single.idxmin(), ['key', 'n']].set_index('key').n
    per['best_s'] = np.where(per.single < per.forward, best_s.reindex(per.index), per.rounds + 1)
    return per


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('profile')
    ap.add_argument('--costs', help='"k_f k_r a_f a_r r_w r_l" instead of the fitted constants')
    ap.add_argument('--materials', action='store_true', help='one line per material')
    a = ap.parse_args()

    types = dict(wp='int8', bp='int8', wm='int8', bm='int8', slice='int32', S='int32', n='int16', U='int32', F='int32',
                 E='int64', C='int64', f='int64', r='int64')
    d = pd.read_csv(a.profile, sep=' ', header=None, names=COLS, dtype=types, on_bad_lines='skip', engine='c')
    d['mat'] = ((d.wp.astype('int32') * 16 + d.bp) * 16 + d.wm) * 16 + d.bm      # a material as one integer
    d['key'] = d.mat.astype('int64') * (1 << 32) + d.slice.astype('int64')
    men = (d.wm + d.bm).to_numpy()
    d['cls'] = pd.Categorical(np.where(men == 0, 'kings only', np.where(men == 1, 'one man', 'two or more men')))
    d.drop(columns=['wp', 'bp', 'wm', 'bm', 'slice'], inplace=True)
    print(f'{d.key.nunique():,} slices, {len(d):,} rounds, {d.mat.nunique()} materials')

    m = fit(d)
    b_b = d.E.sum() / max(1, d.F.sum())
    even = (d.n % 2 == 0).to_numpy()
    gamma = d.C[even].sum() / max(1, d.E[even].sum())
    d.drop(columns=['E', 'C'], inplace=True)
    print(f'forward:    {m["k_f"]:.0f} ns per round + {m["a_f"]:.2f} ns per position of the slice + {m["c"]:.1f} ns per examined position')
    print(f'retrograde: {m["k_r"]:.0f} ns per round + {m["a_r"]:.2f} ns per position of the slice + {m["w"]:.1f} ns per frontier position '
          f'(odd n), {m["l"]:.1f} (even n)')
    print(f'            predecessors per frontier position b_b = {b_b:.2f}, candidates per predecessor (even n) gamma = {gamma:.3f}')
    fitted = [m['k_f'] / m['c'], m['k_r'] / m['c'], m['a_f'] / m['c'], m['a_r'] / m['c'], m['w'] / m['c'], m['l'] / m['c']]
    print(f'kappa = c / w = {m["c"] / m["w"]:.2f};  EGDB_COSTS="{" ".join(f"{v:.4g}" for v in fitted)}"')
    for k, g in d.groupby('cls'):
        mk = fit(g)
        print(f'  {k:16} c {mk["c"]:6.1f}  w {mk["w"]:6.1f}  l {mk["l"]:6.1f}  kappa {mk["c"] / mk["w"]:.2f}')
    costs = [float(v) for v in a.costs.split()] if a.costs else fitted
    if a.costs:
        print(f'model with EGDB_COSTS="{a.costs}"')

    per = totals(d, costs)
    first = d.groupby('key', sort=False)[['mat', 'cls']].first()
    per = per.join(first)
    per['mat'] = per.mat.map(lambda x: f'{int(x) >> 12:02}-{int(x) >> 8 & 15:02}-{int(x) >> 4 & 15:02}-{int(x) & 15:02}')

    def line(label, p):
        s = p[['forward', 'retrograde', 'single', 'opt', 'model']].sum() / 1e9
        print(f'  {label:16} {len(p):9,} slices  forward {s.forward:9.1f}  retrograde {s.retrograde:9.1f}  '
              f'best single switch {s.single:9.1f}  optimum per round {s.opt:9.1f}  model per round {s.model:9.1f}  '
              f'(model {100 * (s.model / s.opt - 1):+.1f}% over the optimum)')

    print('\nseconds:')
    for k, p in per.groupby('cls'):
        line(k, p)
    line('all', per)
    rounds = per.rounds.sum()
    print(f'rounds in which forward is cheaper: {per.fwd_cheaper.sum():,} of {rounds:,} ({100 * per.fwd_cheaper.sum() / rounds:.1f}%)')

    if a.materials:
        print('\nmaterial     slices   rounds  fwd cheaper  best s (median)   seconds: forward retrograde single optimum model')
        mats = per.groupby('mat').agg(slices=('rounds', 'size'), rounds=('rounds', 'sum'), fc=('fwd_cheaper', 'sum'),
                                      s=('best_s', 'median'), forward=('forward', 'sum'), retrograde=('retrograde', 'sum'),
                                      single=('single', 'sum'), opt=('opt', 'sum'), model=('model', 'sum'))
        order = sorted(mats.index, key=lambda x: (sum(map(int, x.split('-')[:2])), sum(map(int, x.split('-')[2:])), x))
        for mat in order:
            x = mats.loc[mat]
            print(f'{mat}  {int(x.slices):7}  {int(x.rounds):7}  {100 * x.fc / max(1, x.rounds):10.1f}%  {x.s:15.0f}   '
                  f'{x.forward / 1e9:8.2f} {x.retrograde / 1e9:8.2f} {x.single / 1e9:8.2f} {x.opt / 1e9:8.2f} {x.model / 1e9:8.2f}')


if __name__ == '__main__':
    main()
