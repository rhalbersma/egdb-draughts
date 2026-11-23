#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The fortress test for 2 kings against 1 in checkers, which gives the colours of the 2-1 table of site.py, as a
yes/no question: the black king on a square of a double corner, black to move, nobody can capture, black can avoid
losing material at once, the white kings anywhere; per double corner and per parity (of the sum of the rank numbers of
all kings, which never changes without a capture) the number of positions and of positions won for white (and lost for
white). White breaks the fortress at a parity when one of those set-ups wins. Exact.

usage: fortress21.py out.json [path/to/egdb_fast]
"""
import json
import sys
from itertools import combinations
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'common'))
from sitekit import FastEngine
from longest32 import double_corners

EXE = sys.argv[2] if len(sys.argv) > 2 else str(Path(__file__).resolve().parent.parent / 'build' / 'egdb_fast')


def board(w, h, corners):
    e = FastEngine(EXE, '--width', w, '--height', h, '--corners', corners, '--rules', 'checkers', '--white', 2, '--black', 1)
    sq = {n: tuple(fr) for n, fr in e.g['sq'].items()}
    at = {fr: n for n, fr in sq.items()}
    nb = {n: [at[(f + df, r + dr)] for df in (-1, 1) for dr in (-1, 1) if (f + df, r + dr) in at] for n, (f, r) in sq.items()}
    name = {n: f'{chr(97 + f)}{r + 1}' for n, (f, r) in sq.items()}

    def captures(us, them):
        occ = set(us) | set(them)
        return any(to is not None and to not in occ
                   for x in us for v in nb[x] if v in them
                   for to in [at.get((2 * sq[v][0] - sq[x][0], 2 * sq[v][1] - sq[x][1]))])

    def value(fen):
        e.p.stdin.write(f'val {fen}\n')
        e.p.stdin.flush()
        v = e.p.stdout.readline().split()[1]
        e.p.stdout.readline()
        return v

    out = {}
    for dc in double_corners(e):
        res = {'even': [0, 0, 0], 'odd': [0, 0, 0]}          # positions, won for white, lost for white
        for b in dc:
            for wk in combinations(sorted(set(sq) - {b}), 2):
                if captures(wk, (b,)) or captures((b,), wk) or all(captures(wk, (y,)) for y in nb[b] if y not in wk):
                    continue                                 # black to move must be able to avoid losing material at once
                v = value(e.fen([name[x] for x in wk], [name[b]], 'B'))
                par = 'even' if sum(sq[x][1] + 1 for x in (*wk, b)) % 2 == 0 else 'odd'
                res[par][0] += 1
                res[par][1] += v[0] == 'L'                   # black to move loses: white wins
                res[par][2] += v[0] == 'W'
        out['/'.join(name[x] for x in dc)] = res
    e.p.kill()
    return out


if __name__ == '__main__':
    res = {}
    for h in range(3, 11):
        for d in range(10):
            w = h + d
            for c in (['unplayable', 'playable'] if w % 2 and h % 2 else ['playable']):
                r = board(w, h, c)
                res[f'{w}x{h} {c}'] = r
                print(f'{w}x{h} {c}:', r, flush=True)
    json.dump(res, open(sys.argv[1], 'w'))
