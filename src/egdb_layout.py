#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""What a RAM budget does to the build: the groups and their DAG (synthesis sections 1.1, 7.2, 7.3).

    python3 src/egdb_layout.py --board 8x8 --pieces 10 --ram 900MB --cost gilbert2005
    python3 src/egdb_layout.py --board 8x8 --pieces 10 --split 5-5 --ram 2GB --cost gilbert2005 --materials

Per material class, the coarsest granularity whose groups fit the budget, refined only where needed (as Gilbert's
threshold did): the whole material if it fits; else its (White tempo, Black tempo) cells, packed into groups up to the
budget with the other cells of the same total tempo (they are independent of each other), and a cell that still does
not fit is cut into packs of its configurations of men, as many per pack as fit (the configurations of one cell are
independent of each other too). A configuration's positions are its king
placements, the same for every configuration of a material: a configuration that does not fit (a kings-only material
is a single one) needs a lower rung, symmetry (--kings-symmetry) or a bigger machine, and is reported as such.

Comfortable: with RAM for 1 depth byte and 4 bit planes per position and side, both sides, of the largest
configuration (kings-only materials by their symmetry), every group can stay at rungs 0-2: granularity alone makes the
rest fit, and the I/O is the files once and the inputs.

The floor: no build needs less than 1 bit per position (and side to move) of one configuration of men with all its
king placements, the largest configuration over the materials; reported with and without the kings-only materials. A
kings-only position has the symmetry of the playable squares (the two diagonals and the half turn), so a kings-only
material needs only a quarter of its positions (--kings-symmetry 4, the default; Gilbert 2005 used one diagonal, 2:
"the single-corner symmetry would halve it again but was not needed").

Then the DAG of the groups, by levels: a group can start when the groups it leads into are finished. Inside a
material, cell (tw, tb) needs (tw - 1, tb) and (tw, tb - 1), so a group's level is its material's start plus its total
tempo tw + tb less the material's smallest; a material needs every
material its captures and promotions lead into (materials outside --split count as built). The width of a level is the
number of groups that could run side by side.

Positions are legal placements per side to move (men of different colours on different squares, kings on the empty
squares), the disjoint index; --cost gives the RAM per position of a group (both sides to move in RAM):

    gilbert2005   0.25 B   2 bits per position: Gilbert's all-kings 5v5 slice, 16.3 G positions, "almost 4 GB of buffer"
    wdl           0.5 B    2 bits per position and side to move, both in RAM (Chinook: slice and mirror together)
    bit           0.25 B   1 bit per position and side (rung 3: Wu-Beal, Lincke)
    byte          3 B      1 depth byte and 4 bit planes per position and side, both in RAM (rungs 0-2: "comfortable")
    thompson86    27.4 B   11 bit planes, seed lists at their worst-case bound (24 B), depth bytes (rung 1)
    syzygy        30.3 B   9 bit planes per side, level lists at their bound, 16-bit values (rung 1)
    or a number of bytes per position.
"""

import argparse
import functools
import math
import re
import sys
from collections import defaultdict

C = math.comb
BOARDS = {'6x6': (6, 3), '8x8': (8, 4), '10x10': (10, 5)}       # rows, playable squares per row
COSTS = {'gilbert2005': 0.25, 'wdl': 0.5, 'bit': 0.25, 'byte': 2 * (1 + 4 / 8), 'thompson86': 11 / 8 + 24 + 2, 'syzygy': 9 * 2 / 8 + 24 + 4}


def parse_bytes(s):
    m = re.fullmatch(r'([\d.]+)\s*([KMGT]i?B|B)?', s.strip(), re.I)
    if not m:
        raise argparse.ArgumentTypeError(f'not a size: {s}')
    unit = (m[2] or 'B').upper()
    scale = {'B': 1, 'KB': 1e3, 'MB': 1e6, 'GB': 1e9, 'TB': 1e12, 'KIB': 2**10, 'MIB': 2**20, 'GIB': 2**30, 'TIB': 2**40}
    return float(m[1]) * scale[unit]


def size(b, binary=True):
    for unit, step in ((('B', 1), ('KiB', 2**10), ('MiB', 2**20), ('GiB', 2**30), ('TiB', 2**40)) if binary else
                       (('B', 1), ('KB', 1e3), ('MB', 1e6), ('GB', 1e9), ('TB', 1e12)))[::-1]:
        if b >= step:
            return f'{b / step:.1f} {unit}' if step > 1 else f'{b:.0f} B'
    return '0 B'


def count(n):
    for unit, step in (('T', 1e12), ('G', 1e9), ('M', 1e6), ('k', 1e3)):
        if n >= step:
            return f'{n / step:.1f}{unit}'
    return f'{n:.0f}'


class Board:
    def __init__(self, name, max_men):
        self.rows, self.per_row = BOARDS[name]
        self.squares = self.rows * self.per_row
        self.cells = self._men(max_men)

    def _men(self, max_men):
        """(wm, tw, bm, tb) -> disjoint placements of the men; White promotes on the last row, Black on the first"""
        poly = {(0, 0, 0, 0): 1}
        last = self.rows - 1
        for r in range(self.rows):
            for _ in range(self.per_row):
                nxt = defaultdict(int)
                for (wm, tw, bm, tb), c in poly.items():
                    nxt[(wm, tw, bm, tb)] += c
                    if wm + bm < max_men:
                        if r != last:
                            nxt[(wm + 1, tw + last - r, bm, tb)] += c
                        if r != 0:
                            nxt[(wm, tw, bm + 1, tb + r)] += c
                poly = nxt
        out = defaultdict(dict)
        for (wm, tw, bm, tb), c in poly.items():
            out[(wm, bm)][(tw, tb)] = c
        return out

    def kings(self, m):
        """the positions of one configuration of the men: the king placements on the empty squares"""
        wm, wk, bm, bk = m
        e = self.squares - wm - bm
        return C(e, wk) * C(e - wk, bk)


def materials(n, split):
    out = []
    for wp in range(1, n):
        for bp in range(1, n - wp + 1):
            if split and (wp, bp) != split:
                continue
            if not split and wp + bp > n:
                continue
            for wm in range(wp + 1):
                for bm in range(bp + 1):
                    out.append((wm, wp - wm, bm, bp - bm))
    return out


def successors(m):
    """the materials one move leads into: a promotion, or a capture of at least one piece and not the last (the
    capturing man possibly promoted), by either side"""
    out = set()
    for side in (0, 1):
        s = m if side == 0 else (m[2], m[3], m[0], m[1])
        back = (lambda x: x) if side == 0 else (lambda x: (x[2], x[3], x[0], x[1]))
        wm, wk, bm, bk = s
        if wm > 0:
            out.add(back((wm - 1, wk + 1, bm, bk)))
        for i in range(bm + 1):
            for j in range(bk + 1):
                if 0 < i + j < bm + bk:
                    out.add(back((wm, wk, bm - i, bk - j)))
                    if wm > 0:
                        out.add(back((wm - 1, wk + 1, bm - i, bk - j)))
    out.discard(m)
    return out


def name(m):
    wm, wk, bm, bk = m
    return f'{wm + wk:02}-{bm + bk:02}-{wm:02}-{bm:02}'


def layout(board, m, max_pos, kings_symmetry):
    """the groups of a material: (granularity, {total tempo: groups in that layer}, largest group, unfit configurations)

    Too large for one group: per total tempo (whose cells are independent of each other), the cells that fit are packed
    into groups up to the budget, and a cell that does not fit becomes packs of its configurations."""
    wm, wk, bm, bk = m
    k = board.kings(m)
    if wm + bm == 0:                                            # kings only: a single configuration
        k //= kings_symmetry
        return 'material', {0: 1}, k, (1 if k > max_pos else 0)
    cells = {c: n * k for c, n in board.cells[(wm, bm)].items()}
    if sum(cells.values()) <= max_pos:
        return 'material', {0: 1}, sum(cells.values()), 0
    layers = defaultdict(list)
    for (tw, tb), p in cells.items():
        layers[tw + tb].append(((tw, tb), p))
    groups, largest, unfit, gran = {}, 0, 0, 'tempi'
    per_pack = max(1, max_pos // k)                             # configurations per group, in a cell that is cut
    for t, cs in layers.items():
        g, room = 0, 0                                          # first fit, in order of size
        for c, p in sorted(cs, key=lambda x: -x[1]):
            if p <= max_pos:
                if p > room:
                    g, room = g + 1, max_pos
                room -= p
                largest = max(largest, max_pos - room)
            else:
                gran = 'configuration'
                n = board.cells[(wm, bm)][c]
                g += -(-n // per_pack)
                largest = max(largest, min(n, per_pack) * k)
                if k > max_pos:
                    unfit += n
        groups[t] = g
    return gran, groups, largest, unfit


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--board', choices=BOARDS, default='8x8')
    ap.add_argument('--pieces', type=int, default=10)
    ap.add_argument('--split', help='only this split of pieces per side, e.g. 5-5 (the others count as built)')
    ap.add_argument('--exact', action='store_true', help='only materials of exactly --pieces pieces (the others count as built)')
    ap.add_argument('--ram', type=parse_bytes, required=True, help='the RAM a group may use, e.g. 900MB, 2GiB')
    ap.add_argument('--cost', default='gilbert2005', help=f'bytes per position: {", ".join(COSTS)}, or a number')
    ap.add_argument('--kings-symmetry', type=int, default=4,
                    help='divide kings-only materials by this: 4 (both diagonals and the half turn, the default), '
                         '2 (one diagonal, as Gilbert 2005), 1 (none)')
    ap.add_argument('--materials', action='store_true', help='one line per material')
    a = ap.parse_args()
    cost = COSTS[a.cost] if a.cost in COSTS else float(a.cost)
    split = tuple(map(int, a.split.split('-'))) if a.split else None
    ms = [m for m in materials(a.pieces, split) if not a.exact or sum(m) == a.pieces]
    board = Board(a.board, a.pieces)
    max_pos = int(a.ram / cost)
    inside = set(ms)

    plan = {m: layout(board, m, max_pos, a.kings_symmetry) for m in ms}
    positions = {m: sum(n * board.kings(m) for n in board.cells[(m[0], m[2])].values()) for m in ms}

    # levels: a material starts after every material it leads into (inside the selection) has finished
    @functools.lru_cache(maxsize=None)
    def span(m):
        gran, groups, _, _ = plan[m]
        if gran == 'material':
            return 1
        return max(groups) - min(groups) + 1

    @functools.lru_cache(maxsize=None)
    def start(m):
        return max((start(s) + span(s) for s in successors(m) if s in inside), default=0)

    width = defaultdict(int)
    for m in ms:
        gran, groups, _, _ = plan[m]
        t0 = min(groups)
        for t, g in groups.items():
            width[start(m) + t - t0] += g

    by = defaultdict(lambda: [0, 0])
    for m in ms:
        by[plan[m][0]][0] += 1
        by[plan[m][0]][1] += sum(plan[m][1].values())
    total_groups = sum(v[1] for v in by.values())
    unfit = [(m, plan[m]) for m in ms if plan[m][3]]
    levels = max(width) + 1

    print(f'{a.board}, {a.pieces} pieces{", split " + a.split if a.split else ""}: {len(ms)} materials, '
          f'{count(sum(positions.values()))} positions per side to move')
    print(f'RAM per group {size(a.ram)}, {cost:g} B per position: at most {count(max_pos)} positions per group')
    for g in ('material', 'tempi', 'configuration'):
        if g in by:
            what = {'material': 'whole', 'tempi': 'packs of tempi cells', 'configuration': 'and packs of configurations'}[g]
            print(f'  {g:13} {by[g][0]:4} materials, {by[g][1]:,} groups ({what})')
    print(f'  total         {total_groups:,} groups; the largest {count(max(p[2] for p in plan.values()))} positions '
          f'({size(max(p[2] for p in plan.values()) * cost)})')
    if unfit:
        n = sum(p[3] for _, p in unfit)
        print(f'  do not fit    {n:,} groups in {len(unfit)} materials, e.g. ' +
              ', '.join(f'{name(m)} ({count(p[2])} positions, {size(p[2] * cost)})' for m, p in unfit[:4]))
    floor_all = max(board.kings(m) // (a.kings_symmetry if m[0] + m[2] == 0 else 1) for m in ms)
    men = [m for m in ms if m[0] + m[2] > 0]
    floor_men = max((board.kings(m) for m in men), default=0)
    print(f'  floor         1 bit per position of the largest configuration: {size(floor_all / 8)} '
          f'({count(floor_all)} positions)' + (f'; {size(floor_men / 8)} without the kings-only materials'
                                                  if men and floor_men != floor_all else ''))
    print(f'  comfortable   1 byte + 4 bit planes per position and side, both sides, for the largest configuration: '
          f'{size(floor_all * COSTS["byte"])}; with this much RAM no group ever leaves rungs 0-2')
    w = [width[i] for i in range(levels)]
    print(f'DAG: {levels} levels (the critical path in groups), width max {max(w):,}, mean {total_groups / levels:,.0f}')
    marks = sorted({0, levels // 4, levels // 2, 3 * levels // 4, levels - 1})
    print('  width by level: ' + ', '.join(f'{i}: {w[i]:,}' for i in marks))
    if a.materials:
        print(f'{"material":12} {"positions":>10} {"granularity":13} {"groups":>10} {"largest":>10} {"MB":>8} {"start":>5} {"levels":>6}')
        for m in sorted(ms, key=lambda m: (start(m), name(m))):
            gran, groups, largest, uf = plan[m]
            print(f'{name(m):12} {count(positions[m]):>10} {gran:13} {sum(groups.values()):>10,} {count(largest):>10} '
                  f'{largest * cost / 1e6:>8.0f} {start(m):>5} {span(m):>6}' + ('  does not fit' if uf else ''))


if __name__ == '__main__':
    sys.exit(main())
