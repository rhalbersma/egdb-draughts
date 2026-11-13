#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Solve the openings of 6x6 international draughts through the complete opening DAG.

The DAG is expanded from the start position for as long as positions have more than `built_max`
pieces and kings on at most one side (such positions cannot repeat, so every position belongs to one
ply and the DAG is finite). Its leaves are:
  * positions with at most `built_max` pieces: exact win / draw / loss from the databases;
  * positions without moves: lost;
  * positions with more pieces and kings on both sides: unknown, the interval [loss, win].
Values are backed up as intervals [lo, hi] (-1 loss, 0 draw, 1 win, for the side to move), like the
"<= Draw" results of the checkers proof (Schaeffer et al., Science 2007). Where the root or an opening
is not decided, the unknown leaves that matter are listed.

usage: opening_solve_6x6.py path/to/egdb_6x6 [--db data/6x6_le10.db] [--table 3] [--out results/opening_6x6]
"""

import argparse
import json
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'
VAL = {'L': -1, 'D': 0, 'W': 1}


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
        return VAL[self.cmd('wdl ' + fen)[0].split()[1]]


def info(fen):
    _, w, b = fen.split(':')
    W = [x for x in w[1:].split(',') if x]
    B = [x for x in b[1:].split(',') if x]
    return len(W) + len(B), any(x[0] == 'K' for x in W), any(x[0] == 'K' for x in B)


def label(lo, hi, stm):
    """Interval for the side to move -> text for white."""
    if stm == 'B':
        lo, hi = -hi, -lo
    if lo == hi:
        return {1: 'wit wint', 0: 'remise', -1: 'zwart wint'}[lo]
    return {(0, 1): 'wit wint of remise (≥ =)', (-1, 0): 'remise of zwart wint (≤ =)', (-1, 1): 'onbeslist'}[(lo, hi)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default='data/6x6_le10.db')
    ap.add_argument('--table', type=int, default=3)
    ap.add_argument('--out', default='results/opening_6x6')
    args = ap.parse_args()
    t0 = time.time()
    log = lambda m: print(f'[{time.time() - t0:7.1f}s] {m}', file=sys.stderr, flush=True)
    eng = Engine(args.engine, args.db)
    log(f'database loaded, built_max {eng.built}')

    # the complete opening DAG
    children = {}              # interior node -> [(move, child)]
    leaf = {}                  # leaf -> (lo, hi, kind)
    layers = [[START]]
    while layers[-1]:
        nxt = set()
        for fen in layers[-1]:
            ms = eng.moves(fen)
            if not ms:
                children.pop(fen, None)
                leaf[fen] = (-1, -1, 'no moves')
                continue
            children[fen] = ms
            for _, c in ms:
                if c in leaf or c in children:
                    continue
                n, wk, bk = info(c)
                if n <= eng.built:
                    v = eng.wdl(c)
                    leaf[c] = (v, v, 'database')
                elif wk and bk:
                    leaf[c] = (-1, 1, 'unknown')
                else:
                    nxt.add(c)
        layers.append(sorted(nxt))
        log(f'ply {len(layers) - 1}: {len(nxt)} open, {len(leaf)} leaves')
    kinds = Counter(k for _, _, k in leaf.values())
    log(f'DAG: {len(children)} interior positions, leaves {dict(kinds)}')

    # one-ply bounds for the unknown leaves: captures (and every move to fewer pieces) lead into the
    # databases, so a known winning or drawing move raises the lower bound; the upper bound stays a win
    # as long as some move leads to another unknown position
    sharpened = 0
    for fen, (lo, hi, kind) in list(leaf.items()):
        if kind != 'unknown':
            continue
        cs = []
        for _, c in eng.moves(fen):
            n, wk, bk = info(c)
            if n <= eng.built:
                v = eng.wdl(c)
                cs.append((v, v))
            else:
                cs.append((-1, 1))
        if not cs:
            leaf[fen] = (-1, -1, 'no moves')
            continue
        nlo, nhi = max(-h for _, h in cs), max(-l for l, _ in cs)
        if (nlo, nhi) != (-1, 1):
            sharpened += 1
        leaf[fen] = (nlo, nhi, 'unknown' if nlo < nhi else 'unknown, decided in one ply')
    kinds = Counter(k for _, _, k in leaf.values())
    log(f'one-ply bounds sharpened {sharpened} unknown leaves; leaves {dict(kinds)}')

    # back up intervals, deepest layer first
    iv = {f: (lo, hi) for f, (lo, hi, _) in leaf.items()}
    for layer in reversed(layers):
        for fen in layer:
            if fen in iv:
                continue
            cs = [iv[c] for _, c in children[fen]]
            iv[fen] = (max(-hi for _, hi in cs), max(-lo for lo, _ in cs))
    root = iv[START]
    log(f'start position: {label(*root, "W")}')

    # unknown leaves that can still change the value of the start position: a leaf matters if it lies
    # on a path along which every move is still possibly best (child interval overlaps the best bound)
    relevant = set()
    stack, seen = [START], {START}
    while stack:
        fen = stack.pop()
        if fen not in children:
            if leaf[fen][0] < leaf[fen][1]:
                relevant.add(fen)
            continue
        lo, hi = iv[fen]
        if lo == hi:
            continue
        for _, c in children[fen]:
            clo, chi = iv[c]
            if -clo >= lo and c not in seen:      # this move can still reach the parent's best value
                seen.add(c)
                stack.append(c)

    rows = []

    def walk(fen, path):
        if len(path) == args.table or fen not in children:
            rows.append((path, fen))
            return
        for nota, c in children[fen]:
            walk(c, path + [nota])
    walk(START, [])
    tree = []

    def walk_tree(fen, path):
        name = 'beginstand' if not path else ('wit ' if len(path) % 2 else 'zwart ') + path[-1]
        tree.append('    ' * len(path) + f'{name}  {label(*iv[fen], fen[0])}')
        if len(path) < args.table and fen in children:
            for nota, c in children[fen]:
                walk_tree(c, path + [nota])
    walk_tree(START, [])
    res = Counter(label(*iv[f], f[0]) for _, f in rows)
    md = ['# Openingen van 6×6-dammen', '',
          f'Beginstand, wit aan zet: **{label(*root, "W")}**.', '',
          'Methode: de volledige openings-DAG (transposities samengevoegd), uitgebreid zolang een stelling meer dan '
          f'{eng.built} stukken heeft en hoogstens één kant dammen. Bladeren: databasestellingen (exact), stellingen '
          'zonder zetten (verloren) en stellingen met meer stukken en dammen aan beide kanten (onbekend). Waarden zijn '
          'als intervallen teruggerekend, vanuit wit gezien.', '',
          f'DAG: {len(children)} inwendige stellingen, bladeren: ' + ', '.join(f'{k} {v}' for k, v in kinds.items()) + '.',
          f'Onbekende bladeren die de beginstand nog kunnen beïnvloeden: {len(relevant)}.', '',
          '## Boom', '', '```', *tree, '```', '', f'## Alle openingen van {args.table} ply', '',
          f'{len(rows)} openingen: ' + ', '.join(f'{k}: {v}' for k, v in res.items()) + '.', '',
          '| # | opening | resultaat |', '|---|---|---|']
    md += [f"| {i} | {' '.join(p)} | {label(*iv[f], f[0])} |" for i, (p, f) in enumerate(rows, 1)]
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out + '.md').write_text('\n'.join(md) + '\n')
    Path(args.out + '.json').write_text(json.dumps({
        'start': START, 'root': root, 'leaf_kinds': kinds, 'relevant_unknown': sorted(relevant),
        'openings': [{'moves': p, 'fen': f, 'interval': iv[f]} for p, f in rows]}, indent=0))
    log('done')


if __name__ == '__main__':
    main()
