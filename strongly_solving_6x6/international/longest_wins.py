#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The longest wins in a 6x6 unit store: per number of pieces (or per material with --materials) the position with the
largest DTM, as FEN and depth in plies, from the unitmax command of egdb_6x6 (capture positions resolved).

Only finished units are read (those with a line in <dir>/crc32c.txt). A per-material store (strongly_solving_6x6/materials,
lines "wp bp wm bm crc") is recognised by its crc32c.txt and read with matmax instead. A unit or material built by an engine
that writes statistics has a .stats file (the longest win and loss per material, counted at the end of its build): it is
read instead, at once. Otherwise unitmax reads every stored position of the unit, so it takes about as long as a
verification of that unit; use few threads while a build is running. --rescan ignores the .stats files.

usage: longest_wins.py path/to/egdb_6x6 [--dir data/units] [--pieces 2-8] [--threads 2] [--materials] [--losses] [--rescan]
"""

import argparse
import subprocess
from pathlib import Path


def finished_units(d):
    """(pieces, command, stats file) per finished unit or material"""
    units = []
    for line in (Path(d) / 'crc32c.txt').read_text().splitlines():
        t = line.split()
        if len(t) > 2 and t[2] == 'positions':                       # unit store: n m positions <count> <crc>
            units.append((int(t[0]), f'unitmax {d} {t[0]} {t[1]}', Path(d) / f'6x6-{int(t[0]):02}-{int(t[1]):02}.stats'))
        else:                                                        # material store: wp bp wm bm <crc>
            name = '-'.join(f'{int(x):02}' for x in t[:4])
            units.append((int(t[0]) + int(t[1]), f'matmax {d} {" ".join(t[:4])}', Path(d) / f'6x6-{name}.stats'))
    return units


def from_stats(path, n):
    """the rows of unitmax from a .stats file: stats wm bm wk bk S s positions p wins w draws d losses l maxwin v fen maxloss v fen"""
    rows = []
    for line in path.read_text().splitlines():
        t = line.split()
        if t and t[0] == 'stats':
            rows.append({'n': n, 'material': tuple(map(int, t[1:5])),
                         'win': None if t[16] == '-' else (int(t[16][1:]), t[17]),
                         'loss': None if t[19] == '-' else (int(t[19][1:]), t[20])})
    return rows


def unitmax(engine, command, n, threads):
    out = subprocess.run([engine], input=f'{command} {threads}\n', capture_output=True, text=True).stdout
    rows = []
    for line in out.splitlines():
        t = line.split()
        if t and t[0] == 'max':
            # max n m material wm bm wk bk win <W..|-> <fen|-> loss <L..|-> <fen|->
            rows.append({'n': n, 'material': tuple(map(int, t[4:8])),
                         'win': None if t[9] == '-' else (int(t[9][1:]), t[10]),
                         'loss': None if t[12] == '-' else (int(t[12][1:]), t[13])})
        elif t and t[0] == 'error':
            raise RuntimeError(f'{command}: {line}')
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--dir', default='data/units')
    ap.add_argument('--pieces', default='2-12', help='a number or a range, e.g. 8 or 2-10')
    ap.add_argument('--threads', type=int, default=2)
    ap.add_argument('--materials', action='store_true', help='one line per material instead of per number of pieces')
    ap.add_argument('--losses', action='store_true', help='the longest losses (side to move loses) instead of wins')
    ap.add_argument('--rescan', action='store_true', help='read every position with unitmax even where a .stats file exists')
    a = ap.parse_args()
    lo, _, hi = a.pieces.partition('-')
    lo, hi = int(lo), int(hi or lo)
    key = 'loss' if a.losses else 'win'
    best = {}
    for n, command, stats in finished_units(a.dir):
        if not lo <= n <= hi:
            continue
        rows = from_stats(stats, n) if stats.exists() and not a.rescan else unitmax(a.engine, command, n, a.threads)
        for r in rows:
            if r[key] is None:
                continue
            k = (n, r['material']) if a.materials else n
            if k not in best or r[key][0] > best[k][0]:
                best[k] = r[key]
    for k in sorted(best):
        depth, fen = best[k]
        label = f'{k[0]:2} {"-".join(map(str, k[1]))}' if a.materials else f'{k:2}'
        print(f'{label}  {"L" if a.losses else "W"}{depth:<4} {fen}')


if __name__ == '__main__':
    main()
