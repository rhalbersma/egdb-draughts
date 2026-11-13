#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Exact values of all openings of 6x6 international draughts up to a given depth (default 3 plies),
in the style of Table 2 and Fig. 3 of Schaeffer et al., "Checkers is solved" (Science, 2007).

usage: opening_6x6.py path/to/egdb_6x6 [--db data/6x6_le9.db] [--depth 3] [--out results/opening_6x6]

Every node gets its exact value with distance to mate (DTM, plies until the side to move has no move
left), computed by egdb_6x6: slice databases (prebuilt up to 9 pieces, on demand for positions with
kings on both sides) and a forward alpha-beta search with exact DTM for the other positions.
"""

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'


class Engine:
    def __init__(self, exe, db):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        if db:
            out = self.cmd(f'load {db}')
            if not out[0].startswith('load ok'):
                raise RuntimeError(out)

    def cmd(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()
        out = []
        while (r := self.p.stdout.readline().rstrip('\n')) != 'end':
            out.append(r)
        return out

    def nodes(self):
        s = self.cmd('stats')[0].split()
        return int(s[s.index('nodes') + 1]), int(s[s.index('extra') + 4])

    def pos(self, fen):
        out = self.cmd('pos ' + fen)
        return out[0].split()[1], [tuple(r.split()[1:4]) for r in out[1:]]


def white_view(val, stm):
    """'W13' for the side to move -> (result for white, DTM)."""
    if val == 'D':
        return '=', None
    wins = (val[0] == 'W') == (stm == 'W')
    return ('W+' if wins else 'Z+'), int(val[1:])


def build(eng, fen, path, depth, log):
    t0 = time.time()
    n0, e0 = eng.nodes()
    val, moves = eng.pos(fen)
    n1, e1 = eng.nodes()
    node = {'path': path, 'fen': fen, 'stm': fen[0], 'val': val, 'time': round(time.time() - t0, 1),
            'nodes': n1 - n0, 'slice_positions': e1 - e0, 'children': []}
    res, dtm = white_view(val, fen[0])
    print(f'[{time.time() - log["t0"]:8.1f}s] {" ".join(path) or "start":24} {res} {dtm or ""}', file=sys.stderr, flush=True)
    for nota, v, nxt in moves:
        # v is the value of the move for the side to move here
        child = {'path': path + [nota], 'fen': nxt, 'stm': nxt[0], 'val': v, 'children': []}
        if len(path) + 1 < depth:
            child = build(eng, nxt, path + [nota], depth, log)
            child['val_from_parent'] = v
        node['children'].append(child)
    return node


def result_of(node):
    """Result for white and DTM of a node. A searched node ('nodes' present) stores the value for its
    own side to move; a leaf stores the value of the move into it, for the side that made that move."""
    if 'nodes' in node:
        return white_view(node['val'], node['stm'])
    return white_view(node['val'], 'B' if node['stm'] == 'W' else 'W')


def fmt(res, dtm):
    return {'W+': 'wit wint', '=': 'remise', 'Z+': 'zwart wint'}[res] + ('' if dtm is None else f' ({dtm})')


def table(root, depth):
    rows = []

    def walk(n):
        if len(n['path']) == depth:
            rows.append(n)
        for c in n['children']:
            walk(c)
    walk(root)
    out = ['| # | opening | resultaat (DTM) | zoekknopen | slicestellingen | tijd (s) |', '|---|---|---|---|---|---|']
    for i, n in enumerate(rows, 1):
        res, dtm = result_of(n)
        out.append(f"| {i} | {' '.join(n['path'])} | {fmt(res, dtm)} | {n.get('nodes', '')} | {n.get('slice_positions', '')} | {n.get('time', '')} |")
    return '\n'.join(out)


def tree_text(root):
    lines = []

    def walk(n, indent):
        res, dtm = result_of(n)
        label = n['path'][-1] if n['path'] else 'beginstand'
        side = '' if not n['path'] else ('wit ' if len(n['path']) % 2 == 1 else 'zwart ')
        lines.append(f"{'    ' * indent}{side}{label}  {fmt(res, dtm)}")
        for c in sorted(n['children'], key=lambda c: c['path'][-1]):
            walk(c, indent + 1)
    walk(root, 0)
    return '```\n' + '\n'.join(lines) + '\n```'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default='data/6x6_le9.db')
    ap.add_argument('--depth', type=int, default=3)
    ap.add_argument('--out', default='results/opening_6x6')
    args = ap.parse_args()
    eng = Engine(args.engine, args.db)
    root = build(eng, START, [], args.depth, {'t0': time.time()})
    Path(args.out + '.json').write_text(json.dumps(root, indent=1))
    res, dtm = white_view(root['val'], 'W')
    md = [f'# Openingen van 6×6-dammen, exact opgelost tot {args.depth} ply', '',
          f'Beginstand (wit aan zet): **{fmt(res, dtm)}**.', '',
          'Resultaten vanuit wit gezien; tussen haakjes de afstand tot het einde van de partij in ply bij perfect spel.', '',
          '## Boom', '', tree_text(root), '', f'## Alle openingen van {args.depth} ply', '', table(root, args.depth), '']
    Path(args.out + '.md').write_text('\n'.join(md))
    print('done', file=sys.stderr)


if __name__ == '__main__':
    main()
