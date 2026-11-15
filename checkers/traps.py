#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Checkers 3-2: can white catch the double-corner fortress at both parities?

Black has a king on one of the two edge squares of each of two double corners, black to move, and nobody can
capture. A TRAP is a white set-up in which both shuttle moves of black (one per corner) allow an immediate forced
exchange: a quiet white offer, after which every black move is a capture and white captures back (3-2 -> 2-1).
The parity (the opposition) is the parity of the sum of the rank numbers (1, 2, ...) of all five kings with black
to move; it never changes without a capture. Result: traps at both parities on 8x8, at one parity on 8x9 and 9x10,
none on 10x10.

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

def run(w, h, colouring):
    e = FastEngine(EXE, '--width', w, '--height', h, '--corners', colouring, '--rules', 'checkers', '--white', 3, '--black', 2)
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
                    phase = 'even' if sum(sq[x][1] + 1 for x in (*W, a, b)) % 2 == 0 else 'odd'   # the parity
                    res[('positions', phase)] += 1
                    if exchange(fen(W, (a2, b), 'W')) and exchange(fen(W, (a, b2), 'W')):
                        res[('traps', phase)] += 1
                        if res[('traps', phase)] == 1:
                            res[('example', phase, f0)] = 1
    print(f'{h}x{w} ({colouring} corner squares): double corners {corners}')
    for k, v in sorted(res.items(), key=str):
        print('  ', k, v)
    sys.stdout.flush()

class Race:
    """8x8: how fast can white force a trap against black's longest defence inside the double corners?

    The game: black only shuttles inside its two double corners (if it cannot, it is frozen and must leave them);
    white makes quiet moves that leave black no capture. White reaches its goal at a trap (black to move, both
    shuttle moves allow a forced exchange); a shuttle move that gives white a capture loses a king at once."""

    CA, CB = (1, 5), (28, 32)

    def __init__(self, e):
        self.e = e
        self.sq = {n: tuple(fr) for n, fr in e.g['sq'].items()}
        self.at = {fr: n for n, fr in self.sq.items()}
        self.nb = {n: [self.at[(f + df, r + dr)] for df in (-1, 1) for dr in (-1, 1) if (f + df, r + dr) in self.at]
                   for n, (f, r) in self.sq.items()}
        states = [(W, a, b) for a in self.CA for b in self.CB
                  for W in combinations([n for n in self.sq if n not in (a, b)], 3)]
        self.capt = {x for x in states if self.captures(x[0], x[1:])}
        self.quiet = [x for x in states if x not in self.capt and not self.captures(x[1:], x[0])]
        traps = {x for x in self.quiet if len(self.shuttles(*x)) == 2
                 and all(self.exchange(self.fen(x[0], y, 'W')) for y in self.shuttles(*x))}
        self.btm = {x: 0 for x in traps}         # black to move: white moves needed to force a trap
        self.wtm = {}                            # white to move: likewise
        wq = [x for x in states if x not in self.capt]
        bq = [x for x in states if not self.captures(x[1:], x[0])]
        n = 0
        while True:
            n += 1
            neww = {x: n for x in wq if x not in self.wtm and any((W2, *x[1:]) in self.btm for W2 in self.wmoves(*x))}
            self.wtm.update(neww)
            newb = {x: n for x in bq if x not in self.btm and self.shuttles(*x)
                    and all((x[0], *y) in self.wtm or (x[0], *y) in self.capt for y in self.shuttles(*x))}
            self.btm.update(newb)
            if not neww and not newb:
                break

    @staticmethod
    def fen(W, B, stm):
        return f"{stm}:W{','.join('K%d' % x for x in sorted(W))}:B{','.join('K%d' % x for x in sorted(B))}"

    def captures(self, side, other):
        occ = set(side) | set(other)
        for s in side:
            f, r = self.sq[s]
            for df in (-1, 1):
                for dr in (-1, 1):
                    m, l = self.at.get((f + df, r + dr)), self.at.get((f + 2 * df, r + 2 * dr))
                    if m in other and l is not None and l not in occ:
                        return True
        return False

    def shuttles(self, W, a, b):
        o = lambda c, x: c[1] if x == c[0] else c[0]
        return [y for y, free in (((o(self.CA, a), b), o(self.CA, a) not in W), ((a, o(self.CB, b)), o(self.CB, b) not in W))
                if free]

    def wmoves(self, W, a, b):
        occ = set(W) | {a, b}
        for i, s in enumerate(W):
            for t in self.nb[s]:
                if t not in occ:
                    W2 = tuple(sorted(W[:i] + (t,) + W[i + 1:]))
                    if not self.captures((a, b), W2):
                        yield W2

    def exchange(self, f):
        """white to move: a quiet move after which black must capture and white captures back, as three moves"""
        e = self.e
        for off in e.analyse(f)[1]:
            if npieces(off[2]) != (3, 2):
                continue
            replies = e.analyse(off[2])[1]
            if not replies or any(npieces(r[2]) != (2, 2) for r in replies):
                continue
            backs = [next((m for m in e.analyse(r[2])[1] if npieces(m[2])[1] < 2), None) for r in replies]
            if all(backs):
                return off, replies[0], backs[0]
        return None

    def distribution(self):
        return Counter(self.wtm[x] for x in self.quiet if x in self.wtm)

    def farthest(self):
        """the start (white to move, white kings off the double corners) that takes white longest"""
        corner = set(self.CA) | set(self.CB)
        return min((x for x in self.quiet if x in self.wtm and not corner & set(x[0])), key=lambda x: (-self.wtm[x], x))

    def line(self, x):
        """the moves (engine move tuples) from white-to-move state x: white fastest, black longest, to the trap,
        then a shuttle and the exchange"""
        def step(fen, target):
            key = lambda f: (f.split(':')[1], f.split(':')[2])
            norm = lambda f: tuple(sorted(int(v.lstrip('K')) for v in f[1:].split(',') if v))
            want = tuple(norm(z) for z in key(target))
            return next(m for m in self.e.analyse(fen)[1] if tuple(norm(z) for z in key(m[2])) == want)
        moves, W, a, b = [], *x
        while True:
            fen = self.fen(W, (a, b), 'W')
            W2 = min((y for y in self.wmoves(W, a, b) if (y, a, b) in self.btm), key=lambda y: self.btm[(y, a, b)])
            moves.append(step(fen, self.fen(W2, (a, b), 'B')))
            W = W2
            sh = [y for y in self.shuttles(W, a, b) if (W, *y) not in self.capt]
            if self.btm[(W, a, b)] == 0:          # the trap: a shuttle and the forced exchange
                a2, b2 = sh[0]
                moves.append(step(self.fen(W, (a, b), 'B'), self.fen(W, (a2, b2), 'W')))
                return moves + list(self.exchange(self.fen(W, (a2, b2), 'W')))
            a2, b2 = max(sh, key=lambda y: self.wtm[(W, *y)])
            moves.append(step(self.fen(W, (a, b), 'B'), self.fen(W, (a2, b2), 'W')))
            a, b = a2, b2


if __name__ == '__main__':
    for w, h in ((8, 8), (9, 8), (10, 9), (10, 10)):    # width, height (corner squares playable)
        run(w, h, 'playable')
    r = Race(FastEngine(EXE, '--width', 8, '--height', 8, '--corners', 'playable', '--rules', 'checkers', '--white', 3, '--black', 2))
    print('8x8: white moves to force a trap, white to move:', dict(sorted(r.distribution().items())))
    print('   farthest:', r.fen(r.farthest()[0], r.farthest()[1:], 'W'), [m[0] for m in r.line(r.farthest())])
