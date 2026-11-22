#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The fortress test for 2 kings against 1 in checkers, which gives the colours of the 2-1 table of site.py: the black king in a double corner, the white kings anywhere,
quiet (nobody can capture, white to move has a move that loses no material, black to move cannot create a fork); per
double corner the positions by (value with white to move, value with black to move). Exact.

usage: fortress21.py out.json [path/to/egdb_fast]
"""
import json
import sys
from itertools import combinations
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'common'))
from sitekit import FastEngine

EXE = sys.argv[2] if len(sys.argv) > 2 else str(Path(__file__).resolve().parent.parent / 'build' / 'egdb_fast')


def board(w, h, corners):
    e = FastEngine(EXE, '--width', w, '--height', h, '--corners', corners, '--rules', 'checkers', '--white', 2, '--black', 1)
    sq = {n: tuple(fr) for n, fr in e.g['sq'].items()}
    at = {fr: n for n, fr in sq.items()}
    nb = {n: [at[(f + df, r + dr)] for df in (-1, 1) for dr in (-1, 1) if (f + df, r + dr) in at] for n, (f, r) in sq.items()}
    name = {n: f'{chr(97 + f)}{r + 1}' for n, (f, r) in sq.items()}

    def jumps(att, victims, occ):
        f, r = sq[att]
        for v in nb[att]:
            if v in victims:
                to = at.get((2 * sq[v][0] - f, 2 * sq[v][1] - r))
                if to is not None and to not in occ:
                    return True
        return False

    def captures(us, them):
        occ = set(us) | set(them)
        return any(jumps(x, set(them), occ) for x in us)

    def safe(wk, bk):
        if captures(wk, bk):
            return True
        occ = set(wk) | set(bk)
        return any(not captures(bk, (set(wk) - {x}) | {y}) for x in wk for y in nb[x] if y not in occ)

    def quiet(wk, bk):
        if captures(wk, bk) or captures(bk, wk) or not safe(wk, bk):
            return False
        occ = set(wk) | set(bk)
        return all(safe(wk, (set(bk) - {x}) | {y}) for x in bk for y in nb[x] if y not in occ)

    dcs = []
    for cf, cr in ((0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)):
        if (cf, cr) not in at:
            dcs.append((at[(cf + (1 if cf == 0 else -1), cr)], at[(cf, cr + (1 if cr == 0 else -1))]))
    out = {}
    for dc in dcs:
        counts = {}
        for b in dc:
            for wk in combinations(sorted(set(sq) - {b}), 2):
                if not quiet(wk, (b,)):
                    continue
                fen = e.fen([name[x] for x in wk], [name[b]])
                key = e.analyse(fen)[0][0] + e.analyse('B' + fen[1:])[0][0]
                counts[key] = counts.get(key, 0) + 1
        out['/'.join(name[x] for x in dc)] = counts
    e.p.kill()
    return out


res = {}
for h in range(3, 11):
    for d in range(10):
        w = h + d
        for c in (['unplayable', 'playable'] if w % 2 and h % 2 else ['playable']):
            r = board(w, h, c)
            res[f'{w}x{h} {c}'] = r
            print(f'{w}x{h} {c}:', r, flush=True)
json.dump(res, open(sys.argv[1], 'w'))
