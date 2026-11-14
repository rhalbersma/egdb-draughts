#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Size of the opening DAG of 6x6 international draughts per ply, with transpositions merged.

A position is a database leaf when it has at most `--dbmax` pieces or when both sides have a king
(both can be looked up directly in the slice databases), and a terminal leaf when the side to move has
no move. Only the remaining positions are expanded: more pieces and kings on at most one side. These
cannot repeat (the side without kings must move a man or capture), so every position belongs to exactly
one ply and the DAG is acyclic.

usage: opening_dag_6x6.py path/to/egdb_6x6 [--dbmax 10] [--maxply 80] [--maxfrontier 5000000]
"""

import argparse
import subprocess
import sys
import time

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--dbmax', type=int, default=10)
    ap.add_argument('--maxply', type=int, default=80)
    ap.add_argument('--maxfrontier', type=int, default=5_000_000)
    args = ap.parse_args()
    p = subprocess.Popen([args.engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)

    def moves(fen):
        p.stdin.write('moves ' + fen + '\n')
        p.stdin.flush()
        out = []
        while (r := p.stdout.readline().rstrip('\n')) != 'end':
            out.append(r.split()[2])
        return out

    def classify(fen):
        _, w, b = fen.split(':')
        n = len([x for x in w[1:].split(',') if x]) + len([x for x in b[1:].split(',') if x])
        if n <= args.dbmax:
            return 'small'
        if 'K' in w and 'K' in b:
            return 'kings'
        return 'open'

    print('| ply | zetten | knopen | transposities | db: ≤%d stukken | db: dammen beide kanten | geen zetten | open | knopen cumulatief | db-knopen cumulatief |' % args.dbmax)
    print('|---|---|---|---|---|---|---|---|---|---|', flush=True)
    t0 = time.time()
    layer, cum, cumdb = [START], 1, 0
    for d in range(1, args.maxply + 1):
        nxt, edges = set(), 0
        for fen in layer:
            for c in moves(fen):
                edges += 1
                nxt.add(c)
        if not nxt:
            print(f'(frontier gesloten na ply {d - 1}, {time.time() - t0:.0f} s)')
            break
        cls = {'small': 0, 'kings': 0}
        opn, term = [], 0
        for f in nxt:
            c = classify(f)
            if c == 'open':
                opn.append(f)
            else:
                cls[c] += 1
        layer = []
        for f in opn:
            if moves(f):
                layer.append(f)
            else:
                term += 1
        cum += len(nxt)
        cumdb += cls['small'] + cls['kings']
        print(f"| {d} | {edges} | {len(nxt)} | {edges - len(nxt)} | {cls['small']} | {cls['kings']} | {term} | {len(layer)} | {cum} | {cumdb} |", flush=True)
        if len(layer) > args.maxfrontier:
            print(f'(gestopt na ply {d}: frontier {len(layer)}, {time.time() - t0:.0f} s)')
            break
    print(f'tijd {time.time() - t0:.0f} s', file=sys.stderr)


if __name__ == '__main__':
    main()
