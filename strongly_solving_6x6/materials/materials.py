#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Plan of the per-material 6x6 build (see Snakefile): the stored materials, their dependencies, sizes, threads and
critical-path priorities.

A material is named wp-bp-wm-bm: white pieces, black pieces, white men, black men (kings: wp - wm and bp - bm). As in
egdb_6x6, only one of a material and its colour mirror is stored (the one whose (wm, bm, wk, bk) is not larger than
(bm, wm, bk, wk)), with both sides to move; a symmetric material with white to move only.

Dependencies: every move out of a material's slices other than a man move (which stays in the material) goes to
  - a promotion: one man fewer and one king more for the moving side, or
  - a capture: fewer pieces of the captured side (several in one move), possibly with the capturing man promoted.
Each target is stored as itself or as its colour mirror. Every target has fewer pieces, or as many pieces and fewer men,
so the graph is acyclic and (pieces, men) ascending is a build order.
"""

from functools import lru_cache
from math import comb

N = 18                                  # dark squares of the 6x6 board


def men_count(w, b):
    """placements of w white and b black men (as men_count in egdb_6x6)"""
    total = 0
    for j in range(4):                  # white men on 16-18, where no black man can stand
        if j <= w and w - j <= 12 and b <= 15 - (w - j):
            total += comb(3, j) * comb(12, w - j) * comb(15 - (w - j), b)
    return total


def stored(wm, bm, wk, bk):
    if min(wm, bm, wk, bk) < 0 or not (1 <= wm + wk <= 6 and 1 <= bm + bk <= 6):
        return False
    return not (bm, wm, bk, wk) < (wm, bm, wk, bk)


def canonical(wm, bm, wk, bk):
    return (wm, bm, wk, bk) if stored(wm, bm, wk, bk) else (bm, wm, bk, wk)


def name(mat):
    wm, bm, wk, bk = mat
    return f'{wm + wk:02}-{bm + bk:02}-{wm:02}-{bm:02}'


def parse(s):
    wp, bp, wm, bm = map(int, s.split('-'))
    return wm, bm, wp - wm, bp - bm


def positions(mat):
    wm, bm, wk, bk = mat
    F = N - wm - bm
    sides = 1 if (wm, bm, wk, bk) == (bm, wm, bk, wk) else 2
    return men_count(wm, bm) * sides * comb(F, wk) * comb(F - wk, bk)


def materials(nmax):
    """all stored materials with at most nmax pieces, in build order (pieces, men, name)"""
    out = []
    for wm in range(7):
        for bm in range(7):
            for wk in range(7):
                for bk in range(7):
                    if wm + bm + wk + bk <= nmax and stored(wm, bm, wk, bk) and men_count(wm, bm) > 0:
                        out.append((wm, bm, wk, bk))
    return sorted(out, key=lambda x: (sum(x), x[0] + x[1], name(x)))


def dependencies(mat):
    """the stored materials that a move out of this material's slices can reach (directly)"""
    wm, bm, wk, bk = mat
    targets = set()
    for side in (0, 1):                 # the side to move: both are stored
        m, k, om, ok = (wm, wk, bm, bk) if side == 0 else (bm, bk, wm, wk)
        moves = []
        if m:
            moves.append((m - 1, k + 1, om, ok))                         # promotion
        for i in range(om + 1):                                          # captures: i men, j kings
            for j in range(ok + 1):
                if i + j == 0 or i + j == om + ok:                       # nothing captured, or nothing left
                    continue
                moves.append((m, k, om - i, ok - j))
                if m:
                    moves.append((m - 1, k + 1, om - i, ok - j))         # capturing man promoted
        for m2, k2, om2, ok2 in moves:
            t = (m2, om2, k2, ok2) if side == 0 else (om2, m2, ok2, k2)
            c = canonical(*t)
            if stored(*c) and men_count(c[0], c[1]) > 0 and c != mat:
                targets.add(c)
    return sorted(targets, key=lambda x: (sum(x), x[0] + x[1], name(x)))


@lru_cache(maxsize=None)
def plan(nmax, cores, big_positions=4_000_000):
    """name -> deps, positions, big (all kings or one man, and large: one job with all cores), threads, priority"""
    mats = materials(nmax)
    names = {m: name(m) for m in mats}
    deps = {m: [d for d in dependencies(m) if d in names] for m in mats}
    # the duration of a job, roughly: positions times a cost that grows with the share of kings (the 12-piece units of
    # the international build ran about 2.4 times slower per position with only kings than with 4 men)
    big = {m: m[0] + m[1] <= 1 and positions(m) >= big_positions for m in mats}
    dur = {m: positions(m) * (1 + 2 * (m[2] + m[3]) / sum(m)) / (cores if big[m] else 1) for m in mats}
    # critical path: a job's own duration plus the longest chain of jobs that depend on it
    users = {m: [] for m in mats}
    for m in mats:
        for d in deps[m]:
            users[d].append(m)
    rank = {}
    for m in reversed(mats):            # dependents come later in build order
        rank[m] = dur[m] + max((rank[u] for u in users[m]), default=0)
    top = max(rank.values()) or 1
    return {names[m]: {'deps': [names[d] for d in deps[m]], 'positions': positions(m), 'big': big[m],
                       'threads': 2 if big[m] else 1, 'priority': int(1000 * rank[m] / top)} for m in mats}


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser(description='print the plan of the per-material build')
    ap.add_argument('--nmax', type=int, default=12)
    ap.add_argument('--cores', type=int, default=10)
    a = ap.parse_args()
    p = plan(a.nmax, a.cores)
    for n, x in p.items():
        print(f'{n}  {x["positions"]:>13,}  {"big" if x["big"] else "   "}  priority {x["priority"]:4}  deps {len(x["deps"]):2}  {" ".join(x["deps"])}')
    print(f'{len(p)} materials, {sum(x["positions"] for x in p.values()):,} positions')
