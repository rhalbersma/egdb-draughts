#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The winning line of every position in longest_wins_6x6.txt: perfect play to the end of the game, from the store.

Each position is played out with the pv command of egdb_6x6 (the winner takes the fastest win, the loser the slowest
loss), and the line is checked: exactly as many plies as the position's DTM, and every position on it has a value one
ply closer to the end than the one before. The result, one line per position, is what the site 6x6 Is Strongly Solved
replays:
    <pieces> W<dtm> <fen> <move> <move> ...

usage: longest_lines.py path/to/egdb_6x6 --store data/units [--input longest_wins_6x6.txt] [--output longest_wins_6x6_lines.txt]
"""

import argparse
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent


class Engine:
    def __init__(self, exe, store):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        out = self.cmd(f'store {store}')
        if not out or not out[0].startswith('store ok'):
            raise RuntimeError(f'cannot open the store {store}: {out}')

    def cmd(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()
        out = []
        while (r := self.p.stdout.readline().rstrip('\n')) != 'end':
            if r.startswith('error'):
                raise RuntimeError(f'{line}: {r}')
            out.append(r)
        return out

    def value(self, fen):
        return self.cmd('dtm ' + fen)[0].split()[1]

    def after(self, fen, nota):
        """the position after the move with this notation"""
        for r in self.cmd('moves ' + fen):
            _, n, nxt = r.split()
            if n == nota:
                return nxt
        raise RuntimeError(f'{nota} is not a legal move in {fen}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--store', required=True, help='the complete unit store (e.g. data/units)')
    ap.add_argument('--input', default=str(HERE / 'longest_wins_6x6.txt'))
    ap.add_argument('--output', default=str(HERE / 'longest_wins_6x6_lines.txt'))
    a = ap.parse_args()
    eng = Engine(a.engine, a.store)
    rows = []
    for line in Path(a.input).read_text().splitlines():
        n, v, fen = line.split()
        if eng.value(fen) != v:
            raise RuntimeError(f'{fen}: the store says {eng.value(fen)}, the list {v}')
        out = eng.cmd('pv ' + fen)
        pv = out[1].split()[1:]
        if len(pv) != int(v[1:]):
            raise RuntimeError(f'{fen}: the line has {len(pv)} plies, the value is {v}')
        f, d = fen, int(v[1:])                 # every ply one closer to the end, the sides alternating
        for k, nota in enumerate(pv):
            f = eng.after(f, nota)
            want = f'{"W" if (d - k - 1) % 2 else "L"}{d - k - 1}'
            if eng.value(f) != want:
                raise RuntimeError(f'{fen}: after {" ".join(pv[:k + 1])} the value is {eng.value(f)}, not {want}')
        rows.append(f'{n} {v} {fen} {" ".join(pv)}')
        print(f'{n:>2} pieces: {v}, {len(pv)} plies checked')
    Path(a.output).write_text('\n'.join(rows) + '\n')


if __name__ == '__main__':
    main()
