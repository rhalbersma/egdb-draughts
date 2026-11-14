#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""How far is 6x6 international draughts solved?

  * the complete opening DAG above the databases (positions with more than `built_max` pieces and kings on
    at most one side): how many of its positions have a proven value, with the same one-ply bounds and
    interval backup as opening_solve_6x6.py;
  * the number of legal positions with 11 and 12 pieces (at most 6 per side), with and without kings on
    both sides;
  * optionally (--probe N) a depth-first probe of the positions with more than `built_max` pieces and kings
    on both sides that are reachable beyond the unknown leaves of the DAG, up to N positions.

usage: opening_strength_6x6.py path/to/egdb_6x6 [--db data/6x6_le9.db] [--attach data/6x6_10.dtm] [--probe N]
"""

import argparse
import sys
import time
from collections import Counter
from math import comb
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from opening_solve_6x6 import START, Engine, info   # noqa: E402


def legal(n, both):
    """legal positions with n pieces, at most 6 per side, kings on both sides (both=True) or not, either side
    to move; white men stand on 4..18 and black men on 1..15 (16..18 and 1..3 are their own back ranks)"""
    tot = 0
    for wm in range(7):
        for bm in range(7):
            for wk in range(7 - wm):
                bk = n - wm - bm - wk
                if bk < 0 or bm + bk > 6 or both != (wk > 0 and bk > 0):
                    continue
                men = sum(comb(3, a) * comb(3, c) * comb(12, wm - a) * comb(12 - (wm - a), bm - c)
                          for a in range(min(wm, 3) + 1) for c in range(min(bm, 3) + 1) if wm - a + bm - c <= 12)
                free = 18 - wm - bm
                tot += men * comb(free, wk) * comb(free - wk, bk)
    return 2 * tot


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default='data/6x6_le9.db')
    ap.add_argument('--attach', default='data/6x6_10.dtm')
    ap.add_argument('--probe', type=int, default=0)
    args = ap.parse_args()
    t0 = time.time()
    log = lambda m: print(f'[{time.time() - t0:7.1f}s] {m}', flush=True)
    for n in (11, 12):
        log(f'legal positions with {n} pieces: kings on both sides {legal(n, True):,}, '
            f'otherwise {legal(n, False):,}')

    eng = Engine(args.engine, args.db, args.attach)
    built = eng.built
    children, iv, kind = {}, {}, {}
    layer = [START]
    while layer:
        nxt = set()
        for fen in layer:
            ms = eng.moves(fen)
            if not ms:
                iv[fen], kind[fen] = (-1, -1), 'no moves'
                continue
            children[fen] = [c for _, c in ms]
            for c in children[fen]:
                if c in iv or c in children:
                    continue
                n, wk, bk = info(c)
                if n <= built:
                    v = eng.wdl(c)
                    iv[c], kind[c] = (v, v), 'database'
                elif wk and bk:
                    iv[c], kind[c] = (-1, 1), 'unknown'
                else:
                    nxt.add(c)
        layer = sorted(nxt)
    unknown = [f for f, k in kind.items() if k == 'unknown']
    for fen in unknown:                     # one-ply bounds, as in opening_solve_6x6.py
        cs = []
        for _, c in eng.moves(fen):
            n, _, _ = info(c)
            cs.append((eng.wdl(c),) * 2 if n <= built else (-1, 1))
        if not cs:
            iv[fen], kind[fen] = (-1, -1), 'no moves'
            continue
        iv[fen] = (max(-h for _, h in cs), max(-l for l, _ in cs))
        if iv[fen][0] == iv[fen][1]:
            kind[fen] = 'unknown, decided in one ply'
    log(f'DAG: {len(children)} interior positions, leaves {dict(Counter(kind.values()))}')
    stack = [(START, False)]
    while stack:
        fen, expanded = stack.pop()
        if fen in iv:
            continue
        if not expanded:
            stack.append((fen, True))
            stack.extend((c, False) for c in children[fen] if c not in iv)
            continue
        cs = [iv[c] for c in children[fen]]
        iv[fen] = (max(-hi for _, hi in cs), max(-lo for lo, _ in cs))
    open_ = Counter({(0, 1): '>=D', (-1, 0): '<=D', (-1, 1): '?'}[iv[f]] for f in children if iv[f][0] < iv[f][1])
    log(f'interior positions decided: {len(children) - sum(open_.values())} of {len(children)}; open {dict(open_)}; '
        f'start position {iv[START]}')

    if args.probe:
        unknown = [f for f in unknown if kind[f] == 'unknown']
        seen, stack = set(unknown), list(unknown)
        expanded = 0
        while stack and len(seen) < args.probe:
            fen = stack.pop()
            expanded += 1
            for _, c in eng.moves(fen):
                n, wk, bk = info(c)
                if n > built and wk and bk and c not in seen:
                    seen.add(c)
                    stack.append(c)
        mat = Counter()
        for f in seen:
            _, w, b = f.split(':')
            W, B = [x for x in w[1:].split(',') if x], [x for x in b[1:].split(',') if x]
            mat[f'{sum(x[0] != "K" for x in W)}m{sum(x[0] == "K" for x in W)}K-'
                f'{sum(x[0] != "K" for x in B)}m{sum(x[0] == "K" for x in B)}K'] += 1
        log(f'probe beyond the {len(unknown)} unknown leaves: {len(seen)} positions with kings on both sides '
            f'({expanded} expanded), complete: {not stack}; most common material {mat.most_common(5)}')


if __name__ == '__main__':
    main()
