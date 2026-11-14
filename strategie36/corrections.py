#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Refutations of the diagram claims and of the variations of the book that are wrong, as diagrams with
the main line next to them.

Diagrams: the line of the book is followed as long as its moves keep the result; the first move of the book
that changes the result is replaced by the best move: the refutation, marked "!". Variations: the book's
claim is about the position at the end of the variation; the refutation replaces the last move of the
variation after which the claim no longer holds. When no position on the variation agrees with the claim,
there is no refutation and the main line continues from the end of the variation. After the refutation the
main line is the fastest win against the longest defence (for a draw a short continuation), until the end of
the game ("+") or another diagram of the book ("-> diagram X (value)"). "*" marks the only move that keeps
a win or a draw, "?" a move of the book that changes the result.

usage: corrections.py path/to/egdb_6x6 [--out weerleggingen.md] [--out-lines weerleggingen_varianten.md]
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


def absolute(fen, v):
    """value for the side to move -> 'W' (white wins), 'Z' (black wins) or 'D'"""
    if v == 'D':
        return 'D'
    return 'W' if (v[0] == 'W') == (fen[0] == 'W') else 'Z'


def agrees(claim, a):
    return {'W+': a == 'W', '=': a == 'D', 'Z+': a == 'Z', 'notW+': a != 'W'}[claim]


def refutation(eng, fen, cont, number, known, own, refute_at=None, stop_on_book=True,
               max_draw_plies=12, max_plies=120):
    """Main line from fen. With refute_at=None (diagrams) the book's moves cont are followed while they keep
    the result; otherwise (variations) they are followed up to ply refute_at, which is replaced by the best
    move (refute_at = len(cont): no refutation, continue after the book)."""
    seq, star, err, cur, refute, reached, ending = [], [], [], fen, None, None, None
    while len(seq) < max_plies:
        v, legal = eng.analyse(cur)
        if not legal:
            ending = 'mate'
            break
        k = len(seq)
        on_book = refute is None and k < len(cont) and (refute_at is None or k < refute_at)
        may_stop = stop_on_book or (not on_book and refute_at is not None and k > min(refute_at, len(cont) - 1))
        if seq and may_stop and cur in known and known[cur] != own:
            reached = (known[cur], v)
            break
        base = refute if refute is not None else len(cont)
        if v == 'D' and not on_book and k >= base + max_draw_plies:
            ending = 'draw'
            break
        bm = best(legal)
        if on_book:
            mb = [m for m in legal if m[0] == cont[k]][0]
            if refute_at is not None or mb[1][0] == bm[1][0]:
                mv = mb
            else:
                mv, refute = bm, k
        else:
            mv = bm
            if refute is None and ((refute_at is not None and k == refute_at < len(cont))
                                   or (refute_at is None and not cont)):
                refute = k
        keeping = [m for m in legal if m[1][0] == bm[1][0]]
        star.append(len(keeping) == 1 and bm[1][0] in 'WD' and mv[1][0] == bm[1][0])
        err.append(mv[1][0] != bm[1][0])
        seq.append(mv)
        cur = mv[2]
    return {'seq': seq, 'star': star, 'err': err, 'refute': refute, 'reached': reached, 'ending': ending,
            'number': number, 'nbook': len(cont)}


def render(r, stm, fen):
    """-> (head, lines of the main line, position shown, book moves before it). The diagram shows the position
    where the refutation is played (or, without refutation, the end of the book's variation), and the main line
    starts there."""
    seq, refute, ending = r['seq'], r['refute'], r['ending']
    if refute is not None:
        s = refute
    elif r['reached']:
        s = 0
    else:
        s = min(r['nbook'], len(seq))
    pos = seq[s - 1][2] if s else fen

    def nota(i):
        return (seq[i][0].replace('x', '×') + ('?' if r['err'][i] else '') + ('!' if i == refute else '')
                + ('*' if r['star'][i] else '') + ('+' if ending == 'mate' and i == len(seq) - 1 else ''))

    def number(k):
        return r['number'] + (k + (1 if stm == 'B' else 0)) // 2

    def black(k):
        return (k + (1 if stm == 'B' else 0)) % 2 == 1
    prefix = ' '.join((f'{number(i)}. ' if not black(i) else (f'{number(i)}. … ' if i == 0 else '')) + nota(i)
                      for i in range(s))
    col = []
    i = s
    while i < len(seq):
        num = f'{number(i)}.'.ljust(4)
        if black(i):
            col.append(f'  {num}…          {nota(i)}')
            i += 1
            continue
        col.append(f"  {num}{nota(i):10} {nota(i + 1) if i + 1 < len(seq) else ''}".rstrip())
        i += 2
    if r['reached']:
        col.append(f"  → diagram {r['reached'][0]} ({r['reached'][1]})")
    elif ending == 'draw':
        col.append('  … D')
    if refute is not None:
        head = f"weerlegging {number(refute)}. {'… ' if black(refute) else ''}{nota(refute)}"
    elif r['reached']:
        head = f"variant naar diagram {r['reached'][0]} ({r['reached'][1]})"
    else:
        head = 'voortzetting na het boek'
    return head, col, pos, prefix


def side(stm):
    return 'wit' if stm == 'W' else 'zwart'


INTRO = ('Het diagram toont de stelling waarin de weerlegging ("!") gespeeld wordt, met de partij aan zet en de '
         'waarde; ernaast de hoofdvariant vanaf de weerlegging: de snelste winst tegen de taaiste verdediging (bij '
         'remise een korte voortzetting), tot het einde van de partij ("+") of tot een ander diagram uit het boek. '
         '"*" = de enige zet die de winst of de remise behoudt, "?" = een zet uit het boek die het resultaat '
         'verandert.')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(HERE.parent / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(HERE.parent / 'data' / '6x6_10.dtm'))
    ap.add_argument('--results', default=str(HERE / 'results.json'))
    ap.add_argument('--out', default=str(HERE / 'weerleggingen.md'))
    ap.add_argument('--out-lines', default=str(HERE / 'weerleggingen_varianten.md'))
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
           'behoudt; de eerste zet van het boek die het resultaat verandert, wordt vervangen door de beste zet. '
           'Zonder variant in het boek is de eerste beste zet de weerlegging. ' + INTRO, '']
    for d in results['diagrams']:
        for stm in ('W', 'B'):
            sd = d['sides'].get(stm, {})
            if sd.get('ok') is not False:
                continue
            fen = stm + d['fen'][1:]
            cont, number = book.get(fen, ([], 1))
            r = refutation(eng, fen, cont, number, known, d['id'])
            head, col, pos, prefix = render(r, stm, fen)
            ps = pos[0]
            col = [f"{side(ps)} aan zet: {eng.analyse(pos)[0]}", f"boek:  {claim_label(sd['claim'], ps)} ❌", ''] + col
            note = f"*{d['note']}*" + (f"; boek: {prefix}" if prefix else '')
            out += [f"#### Diagram {d['id']}, {side(stm)} aan zet: {head}", '', note, '',
                    '```text', with_columns(pos, [col]), '```', '']
            print(f"diagram {d['id']} {stm}: {head}", file=sys.stderr)
    Path(args.out).write_text('\n'.join(out) + '\n')

    out = ['Voor elke variant waarvan de eindclaim niet klopt: de claim van het boek gaat over de stelling aan het '
           'einde van de variant. De weerlegging vervangt de laatste zet van de variant waarna de claim niet meer '
           'opgaat. Boven de hoofdvariant staan ook de eindstelling van de variant uit het boek en de claim van het boek. Klopt de claim in geen enkele stelling van de variant, dan is er geen weerlegging en loopt de '
           'hoofdvariant door vanaf het einde van de variant; het diagram toont dan die eindstelling. ' + INTRO, '']
    for ln in results['lines']:
        if ln['status'] != 'onjuist':
            continue
        fen = ln['start_fen']
        stm = fen[0]
        cont = [p['move'] for p in ln['plies']]
        absv = [absolute(p['fen'], p['val_before']) for p in ln['plies']]
        ok = [i for i, a in enumerate(absv) if agrees(ln['claim'], a)]
        refute_at = ok[-1] if ok else len(cont)
        m0 = re.match(r'\s*(\d+)\.', ln['text'])
        number = int(m0.group(1)) if m0 else 1
        r = refutation(eng, fen, cont, number, known, ln['start'], refute_at=refute_at, stop_on_book=False)
        head, col, pos, prefix = render(r, stm, fen)
        e = ln['end_fen'][0]
        ps = pos[0]
        if r['refute'] is None and not r['reached']:
            head += f": {side(e)} aan zet {ln['end_val']}"
            col = [f"{side(e)} aan zet: {ln['end_val']}", f"boek:  {claim_label(ln['claim'], e)} ❌", ''] + col
        else:
            col = [f"{side(ps)} aan zet: {eng.analyse(pos)[0]}", f"einde boek: {side(e)} aan zet {ln['end_val']}",
                   f"boek:  {claim_label(ln['claim'], e)} ❌", ''] + col
        text = ln['text'] + (f" — {ln['note']}" if ln['note'] else '')
        out += [f"#### Sectie {ln['section']}, vanaf diagram {ln['start']}: {head}", '', f'*{text}*', '',
                '```text', with_columns(pos, [col]), '```', '']
        print(f"line {ln['section']} {ln['start']}: {head}", file=sys.stderr)
    Path(args.out_lines).write_text('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
