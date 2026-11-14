#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Solve the openings of 6x6 international draughts, in the style of Schaeffer et al., "Checkers is
solved" (Science, 2007): a manager builds the opening tree to a fixed depth, merging transpositions
(so it is a DAG), the leaves are solved by depth-first alpha-beta down to the endgame databases, and
the values are backed up by minimax through the DAG.

usage: opening_6x6.py path/to/egdb_6x6 [--db data/6x6_le10.db] [--depth 10] [--table 3] [--out opening_6x6]

Leaves are positions at the given depth, positions inside the databases (at most `built_max` pieces)
and positions without moves. All values are win / draw / loss (no distances), as in the checkers proof.
"""

import argparse
import json
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'


class Engine:
    def __init__(self, exe, db):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        out = self.cmd(f'load {db}')
        if not out[0].startswith('load ok'):
            raise RuntimeError(out)
        self.built = int(out[0].split()[-1])

    def cmd(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()
        out = []
        while (r := self.p.stdout.readline().rstrip('\n')) != 'end':
            out.append(r)
        return out

    def moves(self, fen):
        return [tuple(r.split()[1:3]) for r in self.cmd('moves ' + fen)]

    def wdl(self, fen):
        return self.cmd('wdl ' + fen)[0].split()[1]

    def stats(self):
        s = self.cmd('stats')[0].split()
        return {'tt': int(s[s.index('tt') + 1]), 'nodes': int(s[s.index('nodes') + 1]),
                'on_demand_positions': int(s[s.index('extra') + 4])}


def pieces(fen):
    return sum(len([x for x in part[1:].split(',') if x]) for part in fen.split(':')[1:])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default='data/6x6_le10.db')
    ap.add_argument('--depth', type=int, default=10, help='depth of the manager tree in plies')
    ap.add_argument('--table', type=int, default=3, help='depth of the openings in the table')
    ap.add_argument('--out', default=str(Path(__file__).resolve().parent / 'opening_6x6'))
    args = ap.parse_args()
    t0 = time.time()
    log = lambda msg: print(f'[{time.time() - t0:8.1f}s] {msg}', file=sys.stderr, flush=True)
    eng = Engine(args.engine, args.db)

    # manager: the opening DAG to the given depth
    nodes = {START: {'depth': 0, 'children': None}}
    layers = [[START]]
    per_depth = []
    for d in range(args.depth):
        nxt, edges = [], 0
        for fen in layers[d]:
            node = nodes[fen]
            if pieces(fen) <= eng.built:
                continue                          # inside the databases: a leaf
            node['children'] = eng.moves(fen)
            for nota, child in node['children']:
                edges += 1
                if child not in nodes:
                    nodes[child] = {'depth': d + 1, 'children': None}
                    nxt.append(child)
        per_depth.append({'depth': d + 1, 'moves': edges, 'positions': len(nxt), 'transpositions': edges - len(nxt)})
        log(f'depth {d + 1}: {edges} moves, {len(nxt)} positions ({edges - len(nxt)} transpositions)')
        layers.append(nxt)

    # solvers: every leaf by depth-first alpha-beta down to the databases
    leaves = [f for f, n in nodes.items() if not n['children']]
    log(f'{len(leaves)} leaves')
    for i, fen in enumerate(leaves, 1):
        n = nodes[fen]
        t = time.time()
        n['val'] = eng.wdl(fen)               # for the side to move
        n['time'] = round(time.time() - t, 2)
        if i % 50 == 0 or n['time'] > 10 or i == len(leaves):
            log(f'leaf {i}/{len(leaves)} ({n["time"]} s) {fen} {n["val"]} {eng.stats()}')

    # back up through the DAG (every move is one ply, so a node's children are one layer deeper)
    for d in range(len(layers) - 1, -1, -1):
        for fen in layers[d]:
            n = nodes[fen]
            if 'val' in n:
                continue
            vals = [nodes[c]['val'] for _, c in n['children']]
            n['val'] = 'W' if 'L' in vals else 'D' if 'D' in vals else 'L'

    def white(fen):
        v = nodes[fen]['val']
        return '=' if v == 'D' else ('W+' if (v == 'W') == (fen[0] == 'W') else 'Z+')

    # openings of `table` plies (paths, so transposed openings appear under each move order)
    rows = []

    def walk(fen, path):
        if len(path) == args.table or not nodes[fen]['children']:
            rows.append((path, fen))
            return
        for nota, child in nodes[fen]['children']:
            walk(child, path + [nota])
    walk(START, [])

    tree = []

    def walk_tree(fen, path):
        name = 'beginstand' if not path else ('wit ' if len(path) % 2 else 'zwart ') + path[-1]
        tree.append('    ' * len(path) + f'{name}  {fmt(white(fen))}')
        if len(path) < args.table and nodes[fen]['children']:
            for nota, child in nodes[fen]['children']:
                walk_tree(child, path + [nota])

    def fmt(r):
        return {'W+': 'wit wint', '=': 'remise', 'Z+': 'zwart wint'}[r]

    walk_tree(START, [])
    res = Counter(white(f) for _, f in rows)
    md = [f'# Openingen van 6×6-dammen', '',
          f'Beginstand, wit aan zet: **{fmt(white(START))}**.', '',
          f'Methode: een openingsboom van {args.depth} ply met samengevoegde transposities; elk blad (diepte '
          f'{args.depth}, of hoogstens {eng.built} stukken) is met alpha-beta tot in de databases opgelost en de '
          'waarden zijn met minimax teruggerekend. Waarden zijn winst/remise/verlies, vanuit wit gezien.', '',
          '## Boom', '', '| diepte | zetten | stellingen | transposities |', '|---|---|---|---|']
    md += [f"| {r['depth']} | {r['moves']} | {r['positions']} | {r['transpositions']} |" for r in per_depth]
    md += ['', f'Bladeren: {len(leaves)}. Rekentijd: {time.time() - t0:.0f} s. Engine: {eng.stats()}.', '',
           '```', *tree, '```', '',
           f'## Alle openingen van {args.table} ply', '',
           f"{len(rows)} openingen: wit wint {res['W+']}, remise {res['=']}, zwart wint {res['Z+']}.", '',
           '| # | opening | resultaat |', '|---|---|---|']
    md += [f"| {i} | {' '.join(p)} | {fmt(white(f))} |" for i, (p, f) in enumerate(rows, 1)]
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out + '.md').write_text('\n'.join(md) + '\n')
    Path(args.out + '.json').write_text(json.dumps({'start': START, 'depth': args.depth, 'per_depth': per_depth,
                                                    'nodes': nodes}, indent=0))
    log(f'done: start position {fmt(white(START))}')


if __name__ == '__main__':
    main()
