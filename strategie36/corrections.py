#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Refutations of the diagram claims that are wrong, as diagrams with the main line next to them.

If the side to move does better than the book says, the refutation is its best move; if it does worse,
the refutation is the opponent's best reply to the book's move. The diagram shows the position after the
refutation; the main line next to it starts at the diagram position: the fastest win (the defender
resisting as long as possible), the longest defence, or for a draw a short continuation. It stops at the
end of the game or when another diagram of the book with a claim is reached.

usage: corrections.py path/to/egdb_6x6 [--out weerleggingen.md]
"""

import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from board import with_columns       # noqa: E402
from check import Engine, parse_line  # noqa: E402
from claims import DIAGRAMS, LINES    # noqa: E402

RANK = {'W': 2, 'D': 1, 'L': 0}


def score(v):
    """value of a move for the side that makes it -> sortable score (higher is better)"""
    if v == 'D':
        return 0
    n = int(v[1:])
    return 100000 - n if v[0] == 'W' else -100000 + n


def claim_label(claim, stm):
    if stm == 'W':
        return {'W+': 'W', '=': 'D', 'Z+': 'L', 'notW+': '≤D'}[claim]
    return {'W+': 'L', '=': 'D', 'Z+': 'W', 'notW+': '≥D'}[claim]


def best(legal):
    return max(legal, key=lambda m: (score(m[1]), [-int(x) for x in m[0].replace('x', '-').split('-')]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(HERE.parent / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(HERE.parent / 'data' / '6x6_10.dtm'))
    ap.add_argument('--results', default=str(HERE / 'results.json'))
    ap.add_argument('--out', default=str(HERE / 'weerleggingen.md'))
    ap.add_argument('--max-draw-plies', type=int, default=12)
    args = ap.parse_args()
    eng = Engine(args.engine, 10, args.db, args.attach)
    results = json.loads(Path(args.results).read_text())
    known = {}
    for did, (fen, cw, cb, _) in DIAGRAMS.items():
        for stm, c in (('W', cw), ('B', cb)):
            if c is not None:
                known.setdefault(stm + fen[1:], did)
    # the book's continuation from every position on a line of the book (the longest one), with the move
    # number in the book of its first move
    book = {}
    for _, start, text, _, _ in LINES:
        black_first, mv = parse_line(text)
        fen = ('B' if black_first else 'W') + DIAGRAMS[start][0][1:]
        m0 = re.match(r'\s*(\d+)\.', text)
        n0 = int(m0.group(1)) if m0 else 1
        played = []
        for f, t, cap, marks, only in mv:
            v, legal = eng.analyse(fen)
            cand = [m for m in legal if m[0] in (f'{f}-{t}', f'{f}x{t}')]
            if not cand:
                break
            played.append((fen, cand[0][0]))
            fen = cand[0][2]
        for k in range(len(played)):
            pos = played[k][0]
            kk = k + (1 if black_first else 0)
            cont = [m for _, m in played[k:]]
            if pos not in book or len(cont) > len(book[pos][0]):
                book[pos] = (cont, n0 + kk // 2)
    out = ['Voor elke diagramclaim die niet klopt: de variant van het boek wordt gevolgd zolang die het resultaat '
           'behoudt; de eerste zet van het boek die het resultaat verandert, wordt vervangen door de beste zet: de '
           'weerlegging, gemarkeerd met "!". Het diagram toont de stelling na de weerlegging, ernaast de hoofdvariant '
           'vanaf het diagram uit het boek: daarna de snelste winst tegen de taaiste verdediging (bij remise een korte '
           'voortzetting), tot het einde of tot een ander diagram uit het boek. Zonder variant in het boek is de '
           'eerste beste zet de weerlegging.', '']
    for d in results['diagrams']:
        for stm in ('W', 'B'):
            sd = d['sides'].get(stm, {})
            if sd.get('ok') is not False:
                continue
            fen = stm + d['fen'][1:]
            val = sd['val']
            bk = claim_label(sd['claim'], stm)
            cont, number = book.get(fen, ([], 1))
            seq, cur, refute, reached, ending = [], fen, None, None, None
            while len(seq) < 120:
                v, legal = eng.analyse(cur)
                if not legal:
                    ending = 'mate'
                    break
                if seq and cur in known and known[cur] != d['id']:
                    reached = (known[cur], v)
                    break
                if v == 'D' and refute is not None and len(seq) >= refute + args.max_draw_plies:
                    ending = 'draw'
                    break
                bm = best(legal)
                k = len(seq)
                if refute is None and k < len(cont):
                    mb = [m for m in legal if m[0] == cont[k]][0]
                    if mb[1][0] == bm[1][0]:            # the book's move keeps the result: follow the book
                        mv = mb
                    else:
                        mv, refute = bm, k
                else:
                    mv = bm
                    if refute is None:
                        refute = k
                seq.append(mv)
                cur = mv[2]
            after = seq[refute][2] if refute is not None else fen

            def nota(i):
                return (seq[i][0].replace('x', '×') + ('!' if i == refute else '')
                        + ('+' if ending == 'mate' and i == len(seq) - 1 else ''))
            col = [f"{'wit' if stm == 'W' else 'zwart'} aan zet: {val}  (boek: {bk} ❌)", '']
            k, i = (1 if stm == 'B' else 0), 0
            while i < len(seq):
                num = f'{number + k // 2}.'.ljust(4)
                if k % 2 == 1:
                    col.append(f'  {num}…          {nota(i)}')
                    i += 1
                    k += 1
                    continue
                col.append(f"  {num}{nota(i):10} {nota(i + 1) if i + 1 < len(seq) else ''}".rstrip())
                i += 2
                k += 2
            if reached:
                col.append(f'  → diagram {reached[0]} ({reached[1]})')
            elif ending == 'draw':
                col.append('  … D')
            if refute is not None:
                kk = refute + (1 if stm == 'B' else 0)
                n = number + kk // 2
                head = f"weerlegging {n}. {'… ' if kk % 2 else ''}{nota(refute)}"
            else:
                head = f'variant naar diagram {reached[0]} ({reached[1]})' if reached else 'variant'

            out += [f"#### Diagram {d['id']}, {'wit' if stm == 'W' else 'zwart'} aan zet: {head}", '', f"*{d['note']}*", '',
                    '```text', with_columns(after, [col]), '```', '']
            print(f"{d['id']} {stm}: {val} (boek {bk})  {head}", file=sys.stderr)
    Path(args.out).write_text('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
