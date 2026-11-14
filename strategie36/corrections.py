#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Database variations for the diagram claims that are wrong: the fastest win (the defender resisting as
long as possible), the longest defence (the winner winning as fast as possible), or for a draw the drawing
moves. A variation stops when it reaches another diagram of the book, at the end of the game, or after a
maximum number of plies (draws).

usage: corrections.py path/to/egdb_6x6 [--ids 7.13,8.7] [--json corrections.json]
"""

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from check import Engine            # noqa: E402
from claims import DIAGRAMS         # noqa: E402


def score(v):
    """value of a move for the side that makes it -> sortable score (higher is better)"""
    if v == 'D':
        return 0
    n = int(v[1:])
    return 100000 - n if v[0] == 'W' else -100000 + n


def variation(eng, fen, known, max_draw_plies=12, max_plies=120):
    """best moves from fen; returns (moves as text, value at the start, diagram reached or None)"""
    val, _ = eng.analyse(fen)
    moves, ply, reached = [], 0, None
    black_first = fen[0] == 'B'
    while ply < max_plies:
        v, legal = eng.analyse(fen)
        if not legal:
            break
        if ply > 0 and fen in known:
            reached = known[fen]
            break
        if v == 'D' and ply >= max_draw_plies:
            break
        best = max(legal, key=lambda m: (score(m[1]), [-int(x) for x in m[0].replace('x', '-').split('-')]))
        moves.append(best[0].replace('x', '×'))
        fen = best[2]
        ply += 1
    text = []
    for i, m in enumerate(moves):
        k = i + (1 if black_first else 0)
        if k % 2 == 0:
            text.append(f'{k // 2 + 1}. {m}')
        elif i == 0:
            text.append(f'1. … {m}')
        else:
            text.append(m)
    return ' '.join(text), val, reached, fen


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(HERE.parent / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(HERE.parent / 'data' / '6x6_10.dtm'))
    ap.add_argument('--results', default=str(HERE / 'results.json'))
    ap.add_argument('--ids', default=None, help='only these diagrams (comma separated)')
    ap.add_argument('--json', default=str(HERE / 'corrections.json'))
    args = ap.parse_args()
    eng = Engine(args.engine, 10, args.db, args.attach)
    results = json.loads(Path(args.results).read_text())
    known = {}
    for did, (fen, cw, cb, _) in DIAGRAMS.items():
        for stm, c in (('W', cw), ('B', cb)):
            if c is not None:
                known.setdefault(stm + fen[1:], did)
    ids = set(args.ids.split(',')) if args.ids else None
    out = []
    for d in results['diagrams']:
        if ids and d['id'] not in ids:
            continue
        for stm, sd in d['sides'].items():
            if sd.get('ok') is not False:
                continue
            fen = stm + d['fen'][1:]
            text, val, reached, end = variation(eng, fen, {k: v for k, v in known.items() if v != d['id']})
            out.append({'id': d['id'], 'stm': stm, 'claim': sd['claim'], 'val': val, 'line': text,
                        'reached': reached, 'end_fen': end})
            print(f"{d['id']} {stm}: {val}  {text}" + (f'  → diagram {reached}' if reached else ''), file=sys.stderr)
    Path(args.json).write_text(json.dumps(out, indent=1, ensure_ascii=False))


if __name__ == '__main__':
    main()
