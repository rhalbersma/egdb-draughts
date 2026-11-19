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

With --cores, the groups are refined further (the group limit halved, below the RAM limit) until a list schedule on
that many cores reaches the target utilization: memory sets the coarsest layout, parallelism how much finer to go
(Gilbert 2009 subdivided for parallelism, not memory). A group's time is its positions (--rate turns it into days).

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


# the builders: RAM per position of a group (both sides to move), file bytes per position and side (depth, outcome),
# CPU per position and side to move, measured at 6x6 on a 2.1 GHz Xeon (dtm; larger boards cost more per position)
# movegen: move generations per position and side, the build's cost over the bench's (the measured builds over the
# bench of the same rules, international draughts on 6x6: 0.284 us per move generation), so that a bench of other rules
# (--calibration, `<engine> bench n`) gives their cost per position
BUILDERS = {
    'thompson86': dict(cost=COSTS['thompson86'], depth=1, outcome=1 / 8, us=0.35, two_sided=False, movegen=0.35 / 0.284),
    'syzygy': dict(cost=COSTS['syzygy'], depth=1, outcome=2 / 8, us=0.68, two_sided=True, movegen=0.68 / 0.284),
    'wdl': dict(cost=COSTS['wdl'], depth=0, outcome=2 / 8, us=0.52, two_sided=True, movegen=0.52 / 0.284),
}
REF_GHZ = 2.1


def this_ghz():
    try:
        with open('/proc/cpuinfo') as f:
            return next(float(l.split(':')[1]) / 1000 for l in f if l.startswith('cpu MHz'))
    except (OSError, StopIteration, ValueError):
        return REF_GHZ


def read_bench(path):
    """the weighted line of `<engine> bench n`: us per move generation, moves, capture and promotion shares"""
    with open(path) as f:
        for line in f:
            x = line.rstrip('\n').split('\t')
            if x[0] == 'all':
                return dict(us=float(x[3]), moves=float(x[4]), captures=float(x[5]), promotions=float(x[6]))
    raise SystemExit(f'{path}: no "all" line (the output of <engine> bench n)')


class Profile:
    """an instrumented build (src/egdb_calibrate.py): per metric, the cost per position and side to move and the levels
    by number of pieces, measured up to the build's pieces and extrapolated beyond them geometrically from the last two"""

    def __init__(self, path):
        self.cost, self.levels = {}, {}
        rows = []
        with open(path) as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                x = line.rstrip('\n').split('\t')
                rows.append((x[0], int(x[2]), int(x[3]), int(x[5]), float(x[10])))
        for metric in {r[0] for r in rows}:
            sec, pos, lev = defaultdict(float), defaultdict(int), defaultdict(int)
            for m, n, p, l, t in rows:
                if m == metric and n >= 3:
                    sec[n] += t
                    pos[n] += p
                    lev[n] = max(lev[n], l)
            self.cost[metric] = {n: sec[n] / pos[n] for n in sec if pos[n]}
            self.levels[metric] = dict(lev)
        self.measured = max(max(c) for c in self.cost.values())

    def _extrapolate(self, table, n, geometric):
        if n in table:
            return table[n]
        ks = sorted(table)
        if len(ks) < 2:
            return table[ks[-1]]
        a, b = ks[-2], ks[-1]
        if geometric:
            return table[b] * (table[b] / table[a]) ** ((n - b) / (b - a))
        return table[b] + (table[b] - table[a]) * (n - b) / (b - a)

    def us(self, metric, n):                                   # microseconds per position and side to move
        return 1e6 * self._extrapolate(self.cost[metric], n, True)

    def level(self, metric, n):
        return self._extrapolate(self.levels[metric], n, False)


def read_calibration(path):
    """a bench (`<engine> bench n`) or an instrumented build (src/egdb_calibrate.py)"""
    with open(path) as f:
        head = f.readline()
    return Profile(path) if head.startswith('#metric') else read_bench(path)


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
    """split: None, a (wp, bp) pair or a list of them"""
    if split and isinstance(split[0], int):
        split = [split]
    out = []
    for wp in range(1, n):
        for bp in range(1, n - wp + 1):
            if split and (wp, bp) not in split:
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


def layout(board, m, max_pos, kings_symmetry, ram_pos=None):
    """the groups of a material: (granularity, {total tempo: [positions of each group]}, largest group, unfit
    configurations: those larger than ram_pos, by default max_pos)

    Too large for one group: per total tempo (whose cells are independent of each other), the cells that fit are packed
    into groups up to the budget, and a cell that does not fit becomes packs of its configurations."""
    wm, wk, bm, bk = m
    k = board.kings(m)
    ram_pos = ram_pos or max_pos
    if wm + bm == 0:                                            # kings only: a single configuration
        k //= kings_symmetry
        return 'material', {0: [k]}, k, (1 if k > ram_pos else 0)
    cells = {c: n * k for c, n in board.cells[(wm, bm)].items()}
    if sum(cells.values()) <= max_pos:
        return 'material', {0: [sum(cells.values())]}, sum(cells.values()), 0
    layers = defaultdict(list)
    for (tw, tb), p in cells.items():
        layers[tw + tb].append(((tw, tb), p))
    groups, largest, unfit, gran = {}, 0, 0, 'tempi'
    per_pack = max(1, max_pos // k)                             # configurations per group, in a cell that is cut
    for t, cs in layers.items():
        sizes = []                                              # first fit, in order of size
        for c, p in sorted(cs, key=lambda x: -x[1]):
            if p <= max_pos:
                if not sizes or sizes[-1] + p > max_pos:
                    sizes.append(0)
                sizes[-1] += p
            else:
                gran = 'configuration'
                n = board.cells[(wm, bm)][c]
                sizes += [per_pack * k] * (n // per_pack) + ([n % per_pack * k] if n % per_pack else [])
                if k > ram_pos:
                    unfit += n
        groups[t] = sizes
        largest = max(largest, max(sizes))
    return gran, groups, largest, unfit


def simulate(plan, ms, inside, cores, canon=lambda m: m, weight=lambda m: 1.0):
    """list scheduling of the groups on `cores` workers, a group's time its positions: (makespan, total work, critical
    path). A material's first layer waits for every material it leads into, each layer for the one before; the groups
    of one layer are independent. Ready groups go by the longest remaining path (critical path first)."""
    import heapq
    users = defaultdict(list)
    deps = {m: {canon(s) for s in successors(m)} & inside - {m} for m in ms}
    for m in ms:
        for s in deps[m]:
            users[s].append(m)
    stages = {m: [sorted((g * weight(m) for g in plan[m][1][t]), reverse=True) for t in sorted(plan[m][1])] for m in ms}

    @functools.lru_cache(maxsize=None)
    def tail(m):                                                # the longest path from the end of m
        return max((head(u, 0) for u in users[m]), default=0)

    @functools.lru_cache(maxsize=None)
    def head(m, k):                                             # the longest path from the start of stage k of m
        return sum(st[0] for st in stages[m][k:]) + tail(m)

    for m in sorted(ms, key=lambda m: -sum(m)):                 # fill the caches from the top, no deep recursion
        head(m, 0)
    pending = {m: len(deps[m]) for m in ms}
    left = {}
    ready, running, now, n = [], [], 0, 0

    def release(m, k):
        nonlocal n
        left[(m, k)] = len(stages[m][k])
        for size in stages[m][k]:
            n += 1
            heapq.heappush(ready, (-head(m, k), n, size, m, k))

    for m in ms:
        if pending[m] == 0:
            release(m, 0)
    while ready or running:
        while ready and len(running) < cores:
            _, _, size, m, k = heapq.heappop(ready)
            heapq.heappush(running, (now + size, n, m, k))
            n += 1
        now, _, m, k = heapq.heappop(running)
        left[(m, k)] -= 1
        if left[(m, k)] == 0:
            if k + 1 < len(stages[m]):
                release(m, k + 1)
            else:
                for u in users[m]:
                    pending[u] -= 1
                    if pending[u] == 0:
                        release(u, 0)
    work = sum(sum(sum(v) for v in stages[m]) for m in ms)
    return now, work, max(head(m, 0) for m in ms)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--board', choices=BOARDS, default='8x8')
    ap.add_argument('--pieces', type=int, default=10)
    ap.add_argument('--split', help='only these splits of pieces per side, e.g. 5-5 or 4-4,5-3,3-5 (the others count as built)')
    ap.add_argument('--exact', action='store_true', help='only materials of exactly --pieces pieces (the others count as built)')
    ap.add_argument('--ram', type=parse_bytes, required=True, help='the RAM a group may use, e.g. 900MB, 2GiB')
    ap.add_argument('--cost', help=f'bytes per position: {", ".join(COSTS)}, or a number (default: the builder\'s, else '
                                   f'gilbert2005)')
    ap.add_argument('--builder', choices=BUILDERS, help='estimate space, I/O and time for this builder: thompson86 '
                    '(one-sided, half results), syzygy (two-sided, full results), wdl (two-sided, outcomes only)')
    ap.add_argument('--metrics', type=int, default=1, help='with --builder: depth metrics built (dtm, dtc, dtz), default 1')
    ap.add_argument('--ghz', type=float, default=REF_GHZ, help=f'with --builder: the clock (calibrated at {REF_GHZ} GHz)')
    ap.add_argument('--us', type=float, help='with --builder: microseconds per position and side to move at --ghz, '
                    'instead of the 6x6 calibration (Gilbert 2009, 10x10: about 5.1 at 2.4 GHz)')
    ap.add_argument('--calibration', help='with --builder: an instrumented build (src/egdb_calibrate.py on a small build\'s '
                    'logs: the cost per position and the levels by pieces, extrapolated) or a bench (`<engine> bench n`: '
                    'the cost per position from the move generation)')
    ap.add_argument('--bench-ghz', type=float, help='the clock of the machine the bench ran on (default: this one)')
    ap.add_argument('--disk', type=parse_bytes, help='with --builder: the SSD (or disk) size, e.g. 4TB')
    ap.add_argument('--disk-mbps', type=float, default=500, help='with --builder: sequential MB/s (default 500, a SATA SSD)')
    ap.add_argument('--tbw', type=float, help='with --builder: the SSD\'s endurance in TB written')
    ap.add_argument('--index', choices=('disjoint', 'overlapping'), default='disjoint',
                    help='with --builder: file slots per legal placement (disjoint, the default) or per independent '
                         'colex ranks per colour and type, as thompson86 and syzygy index today (overlapping)')
    ap.add_argument('--levels', type=int, default=100,
                    help='with --builder: levels of a group that needs rung 3 (its per-level streams), default 100')
    ap.add_argument('--kings-symmetry', type=int, default=4,
                    help='divide kings-only materials by this: 4 (both diagonals and the half turn, the default), '
                         '2 (one diagonal, as Gilbert 2005), 1 (none)')
    ap.add_argument('--materials', action='store_true', help='one line per material')
    ap.add_argument('--cores', type=int, help='refine the groups below the RAM limit until this many cores are kept busy')
    ap.add_argument('--target', type=float, default=0.9, help='with --cores: the utilization to reach (default 0.9)')
    ap.add_argument('--rate', type=float, help='positions per second per core, to turn the schedule into time '
                                             '(Gilbert 2009: 197529)')
    a = ap.parse_args()
    b_ = BUILDERS.get(a.builder)
    if a.cost is None:
        cost = b_['cost'] if b_ else COSTS['gilbert2005']
    else:
        cost = COSTS[a.cost] if a.cost in COSTS else float(a.cost)
    if a.builder and a.cores is None:
        a.cores = 1
    split = [tuple(map(int, x.split('-'))) for x in a.split.split(',')] if a.split else None
    ms = [m for m in materials(a.pieces, split) if not a.exact or sum(m) == a.pieces]
    if b_ and b_['two_sided']:                                  # one of each mirror pair
        ms = [m for m in ms if not (m[2], m[0], m[3], m[1]) < (m[0], m[2], m[1], m[3])]
    canon = ((lambda m: m if not (m[2], m[0], m[3], m[1]) < (m[0], m[2], m[1], m[3]) else (m[2], m[3], m[0], m[1]))
             if b_ and b_['two_sided'] else (lambda m: m))
    board = Board(a.board, a.pieces)
    max_pos = int(a.ram / cost)
    inside = set(ms)

    plan = {m: layout(board, m, max_pos, a.kings_symmetry) for m in ms}
    refine = []
    if a.cores:                                                 # finer groups until the cores are busy
        cap = max_pos
        while True:
            span_, work, path = simulate(plan, ms, inside, a.cores, canon)
            groups_n = sum(len(v) for p in plan.values() for v in p[1].values())
            refine.append((cap, groups_n, span_, work / (a.cores * span_), path))
            if work / (a.cores * span_) >= a.target or cap < 2 or (len(refine) > 1 and groups_n == refine[-2][1]):
                break
            cap //= 2
            plan = {m: layout(board, m, cap, a.kings_symmetry, max_pos) for m in ms}
    positions = {m: sum(n * board.kings(m) for n in board.cells[(m[0], m[2])].values()) for m in ms}

    # levels: a material starts after every material it leads into (inside the selection) has finished
    @functools.lru_cache(maxsize=None)
    def span(m):
        gran, groups, _, _ = plan[m]
        if gran == 'material':
            return 1
        return max(groups) - min(groups) + 1   # layers

    @functools.lru_cache(maxsize=None)
    def start(m):
        return max((start(c) + span(c) for c in {canon(s) for s in successors(m)} & inside - {m}), default=0)

    width = defaultdict(int)
    for m in ms:
        gran, groups, _, _ = plan[m]
        t0 = min(groups)
        for t, g in groups.items():
            width[start(m) + t - t0] += len(g)

    by = defaultdict(lambda: [0, 0])
    for m in ms:
        by[plan[m][0]][0] += 1
        by[plan[m][0]][1] += sum(len(v) for v in plan[m][1].values())
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
    if refine:
        def when(units):
            if not a.rate:
                return f'{count(units)} positions'
            d = units / a.rate / 86400
            return f'{d:,.0f} days' if d >= 2 else f'{d * 24:,.1f} h'
        print(f'{a.cores} cores (target utilization {a.target:.0%}), the time of a group its positions'
              + (f' at {a.rate:,.0f} per second' if a.rate else '') + ':')
        print(f'  {"group limit":>12} {"groups":>10} {"makespan":>12} {"utilization":>11} {"critical path":>14}')
        for cap, g, span_, util, path in refine:
            print(f'  {count(cap):>12} {g:>10,} {when(span_):>12} {util:>11.0%} {when(path):>14}')
        print(f'  total work {when(work)} on one core; the critical path is the floor of the makespan')
    if b_:
        estimate(a, b_, board, ms, plan, positions, canon, refine, max_pos, cost)
    if a.materials:
        print(f'{"material":12} {"positions":>10} {"granularity":13} {"groups":>10} {"largest":>10} {"MB":>8} {"start":>5} {"levels":>6}')
        for m in sorted(ms, key=lambda m: (start(m), name(m))):
            gran, groups, largest, uf = plan[m]
            print(f'{name(m):12} {count(positions[m]):>10} {gran:13} {sum(len(v) for v in groups.values()):>10,} {count(largest):>10} '
                  f'{largest * cost / 1e6:>8.0f} {start(m):>5} {span(m):>6}' + ('  does not fit' if uf else ''))


def estimate(a, b, board, ms, plan, positions, canon, refine, max_pos, cost):
    """space, I/O and time of the build on the machine (synthesis section 7.4)"""
    n = a.metrics if b['depth'] else 1
    legal = lambda m: positions[m] if m in positions else sum(c * board.kings(m) for c in board.cells[(m[0], m[2])].values())
    men_sq = board.squares - board.per_row                      # a man never stands on its promotion row

    def pos(m):                                                 # file slots per side to move
        if a.index == 'disjoint':
            return legal(m)
        wm, wk, bm, bk = m
        return math.comb(men_sq, wm) * math.comb(men_sq, bm) * math.comb(board.squares, wk) * math.comb(board.squares, bk)
    P = sum(pos(m) for m in ms)                                 # slots per side to move, stored materials
    depth = 2 * P * b['depth'] * n                              # both sides to move
    outcome = 2 * P * b['outcome'] * n                          # each metric's build writes its own (identical) outcomes
    files = depth + outcome
    # scratch: the depth sink's group-order files of the jobs writing side by side (a file written under a temporary
    # name becomes the final one by a rename, so it is counted in the files already); values of 1 byte (thompson86) or
    # 2 (syzygy) per position and side
    v = 0 if not b['depth'] else (2 if b['two_sided'] else 1)
    scratch = sum(sorted((2 * pos(m) * v for m in ms), reverse=True)[:a.cores])
    # reads: each build reads the files of the materials it leads into (depths for dtm, outcomes otherwise)
    best = worst = 0
    for metric in range(max(n, 1)):
        per_pos = 2 * ((b['depth'] + b['outcome'] * (b['two_sided'])) if metric == 0 and b['depth'] else b['outcome'])
        used = set()
        for m in ms:
            for s_ in {canon(x) for x in successors(m)} - {m}:
                if s_[0] + s_[1] == 0 or s_[2] + s_[3] == 0:
                    continue
                worst += pos(s_) * per_pos
                used.add(s_)
        best += sum(pos(x) * per_pos for x in used)
    # groups that do not fit RAM at the builder's cost: rung 3, about a dozen plane streams per level
    big = [g for p_ in plan.values() for v in p_[1].values() for g in v if g * cost > a.ram]
    streams = sum(a.levels * 12 * 2 * g / 8 for g in big) * n
    writes, reads_lo, reads_hi = files, best, worst
    mbps = a.disk_mbps * 1e6
    io_lo = (writes + reads_lo + streams) / mbps
    io_hi = (writes + reads_hi + streams) / mbps
    cal = read_calibration(a.calibration) if a.calibration else None
    bench = cal if isinstance(cal, dict) else None
    profile = cal if isinstance(cal, Profile) else None
    if profile:                                                 # the instrumented build: per material, its own cost
        clock = (a.bench_ghz or this_ghz()) / a.ghz
        metrics = sorted(profile.cost)
        cpu = path_s = 0
        for metric in metrics:
            w = lambda m, metric=metric: 2 * profile.us(metric, sum(m)) * 1e-6 * clock
            span_m, _, path_m = simulate(plan, ms, set(ms), a.cores, canon, w)
            cpu += span_m
            path_s = max(path_s, path_m)
        cpu *= n / len(metrics) if b['depth'] else 1
        shown = ', '.join(f'{x} {profile.us(x, a.pieces):.2f}' for x in metrics)
        lev = ', '.join(f'{x} {profile.level(x, a.pieces):.0f}' for x in metrics)
        profile_line = (f'  profile instrumented build to {profile.measured} pieces: at {a.pieces} pieces {shown} µs per position '
                        f'and side' + (' (extrapolated)' if a.pieces > profile.measured else '') + f', levels {lev}')
    if a.us:
        us, source = a.us, 'given'
    elif profile:
        us, source = b['us'] * REF_GHZ / a.ghz, 'profile'
    elif bench:
        us = b['movegen'] * bench['us'] * (a.bench_ghz or this_ghz()) / a.ghz
        source = (f'bench: {bench["us"]:.3f} us per move generation x {b["movegen"]:.2f}, {bench["moves"]:.2f} moves, '
                  f'{bench["captures"]:.0%} captures, {bench["promotions"]:.0%} promotions')
    else:
        us, source = b['us'] * REF_GHZ / a.ghz, '6x6 calibration scaled by clock'
    us *= 1e-6
    span_, work, path = refine[-1][2], refine[-1][2] * 0 + sum(sum(sum(v) for v in p_[1].values()) for p_ in plan.values()), refine[-1][4]
    cpu = span_ * 2 * us * n                                    # both sides to move, one metric after another
    def t(sec):
        for unit, d in (('days', 86400), ('h', 3600), ('min', 60)):
            if sec >= 2 * d:
                return f'{sec / d:,.1f} {unit}'
        return f'{sec:,.0f} s'
    print(f'estimate for {a.builder} ({n} metric{"s" if n > 1 else ""}), {a.cores} cores at {a.ghz:g} GHz, '
          f'{size(a.ram)} RAM per group, disk {size(a.disk, False) if a.disk else "?"} at {a.disk_mbps:g} MB/s:')
    if profile:
        print(profile_line)
    print(f'  space   ({a.index} index) files {size(files, False)} (depths {size(depth, False)}, outcomes {size(outcome, False)}), '
          f'scratch {size(scratch, False)}: {size(files + scratch, False)}'
          + (f' of {size(a.disk, False)}: {"fits" if files + scratch <= a.disk else "DOES NOT FIT"}' if a.disk else ''))
    print(f'  I/O     writes {size(writes, False)} (sequential), reads {size(reads_lo, False)} to {size(reads_hi, False)}'
          + (f', rung-3 streams {size(streams, False)} ({len(big)} groups over the RAM, {a.levels} levels each)' if big else '')
          + f': {t(io_lo)} to {t(io_hi)} of disk time'
          + (f'; {writes / (a.tbw * 1e12):.2%} of {a.tbw:g} TBW' if a.tbw else ''))
    if bench:                                                   # pass 0's lookups into other materials
        look = 2 * P * bench['moves'] * (bench['captures'] + bench['promotions']) * n
        print(f'  lookups {count(look)} into other materials\' files ({bench["moves"] * (bench["captures"] + bench["promotions"]):.2f} '
              f'per position): random reads, cheap while the inputs stay in the page cache')
    if profile:
        print(f'  time    CPU {t(cpu)} on {a.cores} cores (each material at its own measured or extrapolated cost; '
              f'critical path {t(path_s)}); with the disk time: {t(max(cpu, io_lo))} (overlapped) to {t(cpu + io_hi)}')
        return
    print(f'  time    CPU {t(cpu)} on {a.cores} cores ({us * 1e6:.2f} µs per position and side, {source}; '
          f'critical path {t(path * 2 * us * n)}); '
          f'with the disk time: {t(max(cpu, io_lo))} (overlapped) to {t(cpu + io_hi)}')
    if a.board != '6x6' and not a.us and not bench:
        print('          the 6x6 calibration is a floor for larger boards (longer rays, flying kings, inputs out of cache): pass --us')


if __name__ == '__main__':
    sys.exit(main())
