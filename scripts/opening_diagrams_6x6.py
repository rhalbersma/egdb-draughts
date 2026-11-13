#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Diagrams of the opening tree with the legal moves and their values next to the board.

Reads results/opening_6x6.json (from opening_solve_6x6.py) and uses egdb_6x6 only for move generation.

usage: opening_diagrams_6x6.py path/to/egdb_6x6 [--json results/opening_6x6.json] [--plies 0,1] [--out FILE]
"""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'strategie36'))
from board import diagram                      # noqa: E402
from opening_solve_6x6 import label             # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--json', default=str(HERE.parent / 'results' / 'opening_6x6.json'))
    ap.add_argument('--plies', default='0,1')
    ap.add_argument('--out', default=None)
    args = ap.parse_args()
    data = json.loads(Path(args.json).read_text())
    tree = data['tree']
    p = subprocess.Popen([args.engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)

    def moves(fen):
        p.stdin.write('moves ' + fen + '\n')
        p.stdin.flush()
        out = []
        while (r := p.stdout.readline().rstrip('\n')) != 'end':
            out.append(tuple(r.split()[1:3]))
        return out

    def numeric(nota):
        return tuple(int(x) for x in re.split('[-x]', nota))

    def key(m):
        nota, c = m
        lo, hi = tree[c]['interval']
        n = tree[c]['dtm']
        if lo != hi:
            rank = (2, 0)
        elif lo == -1:
            rank = (0, n if n is not None else 10**6)
        elif lo == 0:
            rank = (1, 0)
        else:
            rank = (3, -n if n is not None else 10**6)
        return rank + numeric(nota)

    plies = {int(x) for x in args.plies.split(',')}
    out = []

    def walk(fen, path):
        ply = len(path)
        ms = sorted(moves(fen), key=key) if ply < 3 else []
        if ply in plies:
            lo, hi = tree[fen]['interval']
            side = 'wit' if fen[0] == 'W' else 'zwart'
            title = 'Beginstand' if not path else ' '.join(
                (f'{i // 2 + 1}. ' if i % 2 == 0 else '') + m for i, m in enumerate(path))
            value = label(lo, hi, fen[0], tree[fen]['dtm'])
            notes = [f'{side} aan zet: {value}', ''] + [
                f'  {nota:6} {label(*tree[c]["interval"], c[0], tree[c]["dtm"], True)}' for nota, c in ms]
            board = diagram(fen).split('\n')
            board += [''] * (len(notes) - len(board))
            width = max(len(b) for b in board)
            board = [b.ljust(width) for b in board]
            notes += [''] * (len(board) - len(notes))
            out.extend([f'#### {title} (ply {ply})', '', '```'])
            out.extend((b + '     ' + n).rstrip() for b, n in zip(board, notes))
            out.extend(['```', ''])
        if ply < max(plies):
            for nota, c in ms:
                walk(c, path + [nota])
    walk(data['start'], [])
    text = '\n'.join(out)
    if args.out:
        Path(args.out).write_text(text + '\n')
    else:
        print(text)


if __name__ == '__main__':
    main()
