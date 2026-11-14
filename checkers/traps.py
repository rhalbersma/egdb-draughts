#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Checkers 3-2: can white catch both phases of the double-corner fortress?

Black has a king on one of the two edge squares of each of two double corners, black to move, and nobody can
capture. A TRAP is a white set-up in which both shuttle moves of black (one per corner) allow an immediate forced
exchange: a quiet white offer, after which every black move is a capture and white captures back (3-2 -> 2-1).
The phase (the opposition) is the parity of the sum of the ranks of all five kings with black to move; it never
changes without a capture. Result: traps in both phases on 8x8, in one phase on 8x9 and 9x10, none on 10x10.

usage: traps.py [path/to/egdb_fast]
"""

import sys
from itertools import combinations
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'common'))
from sitekit import FastEngine   # noqa: E402

EXE = sys.argv[1] if len(sys.argv) > 1 else str(ROOT / 'build' / 'egdb_fast')

def npieces(fen):
    _, w, b = fen.split(':')
    return len([x for x in w[1:].split(',') if x]), len([x for x in b[1:].split(',') if x])

def run(w, h, p):
    e = FastEngine(EXE, '--width', w, '--height', h, '--parity', p, '--rules', 'checkers', '--white', 3, '--black', 2)
    sq = {n: tuple(fr) for n, fr in e.g['sq'].items()}
    at = {fr: n for n, fr in sq.items()}
    corners = []
    for cf, cr in ((0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)):
        if (cf, cr) in at:
            continue                         # a playable corner is a single corner
        df = 1 if cf == 0 else -1
        dr = 1 if cr == 0 else -1
        pair = (at.get((cf + df, cr)), at.get((cf, cr + dr)))
        if None not in pair:
            corners.append(pair)
    fen = lambda W, B, stm: f"{stm}:W{','.join('K%d' % x for x in sorted(W))}:B{','.join('K%d' % x for x in sorted(B))}"

    def exchange(f):                         # white to move: an offer that forces capture and recapture
        _, ms = e.analyse(f)
        for _, _, f1, _ in ms:
            if npieces(f1) != (3, 2):
                continue                     # only quiet offers
            _, bm = e.analyse(f1)
            if not bm or any(npieces(f2) != (2, 2) for _, _, f2, _ in bm):
                continue                     # black is not forced to capture
            if all(any(npieces(f3)[1] < 2 for _, _, f3, _ in e.analyse(f2)[1]) for _, _, f2, _ in bm):
                return True
        return False

    res = Counter()
    for ca, cb in combinations(corners, 2):
        for a in ca:
            for b in cb:
                a2 = ca[1] if a == ca[0] else ca[0]
                b2 = cb[1] if b == cb[0] else cb[0]
                free = [n for n in sq if n not in (a, b, a2, b2)]
                for W in combinations(free, 3):
                    f0 = fen(W, (a, b), 'B')
                    # quiet start: nobody can capture
                    _, bm = e.analyse(f0)
                    if any(npieces(x[2]) != (3, 2) for x in bm):
                        continue
                    if any(npieces(x[2]) != (3, 2) for x in e.analyse(fen(W, (a, b), 'W'))[1]):
                        continue
                    phase = sum(sq[x][1] for x in (*W, a, b)) % 2
                    res[('positions', phase)] += 1
                    if exchange(fen(W, (a2, b), 'W')) and exchange(fen(W, (a, b2), 'W')):
                        res[('traps', phase)] += 1
                        if res[('traps', phase)] == 1:
                            res[('example', phase, f0)] = 1
    print(f'{h}x{w} p{p}: double corners {corners}')
    for k, v in sorted(res.items(), key=str):
        print('  ', k, v)
    sys.stdout.flush()

if __name__ == '__main__':
    for w, h, p in ((8, 8, 1), (9, 8, 1), (10, 9, 1), (10, 10, 1)):    # width, height, parity
        run(w, h, p)
