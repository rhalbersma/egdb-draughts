#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Assemble RAPPORT.md (Dutch): bevindingen.md, methode.md and the tables generated from results.json."""

import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


sys.path.insert(0, str(HERE))
from board import with_columns          # noqa: E402
from claims import DIAGRAMS, LINES      # noqa: E402
from check import parse_line            # noqa: E402


def mark(ok):
    return {True: '✅', False: '❌', None: ''}[ok]


def claim_label(claim, stm):
    """A claim of the book (for white: W+, =, Z+, notW+) -> W, D, L, <=D or >=D for the side to move."""
    if claim is None:
        return None
    if stm == 'W':
        return {'W+': 'W', '=': 'D', 'Z+': 'L', 'notW+': '≤D'}[claim]
    return {'W+': 'L', '=': 'D', 'Z+': 'W', 'notW+': '≥D'}[claim]


def numeric(nota):
    return tuple(int(x) for x in nota.replace('x', '-').split('-'))


def strength(item):
    """sort key: wins (fastest first), draws, losses (slowest first), then the move"""
    nota, v = item
    if v[0] == 'W':
        rank = (0, int(v[1:]))
    elif v == 'D':
        rank = (1, 0)
    else:
        rank = (2, -int(v[1:]))
    return rank + numeric(nota)


def sides_to_show(did):
    """White to move always, black to move only when the book says something about it; a diagram that
    the book only discusses with black to move gets only that board."""
    _, cw, cb, _ = DIAGRAMS[did]
    starts = {parse_line(text)[0] for _, start, text, _, _ in LINES if start == did}
    show_b = cb is not None or True in starts
    show_w = cw is not None or False in starts or not show_b
    return (['W'] if show_w else []) + (['B'] if show_b else [])


def diagram_blocks(diagrams):
    out = ['Per diagram het bord met de veldnummers en alle zetten van de partij aan zet, sterkste eerst: winst '
           '(snelste eerst), remise, verlies (taaiste eerst). De waarde van de stelling geldt voor de partij aan '
           'zet; bij een zet voor wie hem doet, met n het aantal ply tot het einde van de partij, de zet '
           'meegeteld (winst oneven, verlies even, D = remise). Erachter de claim van het boek, vertaald naar de '
           'partij aan zet (W, D, L; ≤D = "wit wint niet"). w/b = witte/zwarte schijf, W/B = dam.', '']
    for d in diagrams:
        for stm in sides_to_show(d['id']):
            sd = d['sides'][stm]
            side = 'wit' if stm == 'W' else 'zwart'
            fen = stm + d['fen'][1:]
            if 'error' in sd:
                out += [f"#### Diagram {d['id']}, {side} aan zet", '', f"*{d['note']}*: {sd['error']}", '']
                continue
            cl = claim_label(sd['claim'], stm)
            title = f"{side} aan zet: {sd['val']}" + (f"  (boek: {cl} {mark(sd['ok'])})" if cl else '')
            num = '1.' if stm == 'W' else '1…'
            col = [title, ''] + [f'  {num} {m:6} {v}' for m, v in sorted(sd['moves'], key=strength)]
            out += [f"#### Diagram {d['id']}, {side} aan zet", '', f"*{d['note']}*", '',
                    '```text', with_columns(fen, [col]), '```', '']
    return '\n'.join(out)


STATUS = {'ok': '✅', 'info': 'ℹ️', 'opmerking': '⚠️', 'onjuist': '❌', 'notatie': '✏️'}


def lines_table(lines):
    out = ['De waarden gelden voor de partij die aan het einde van de variant aan zet is (kolom "aan zet"): '
           'W, D of L met de afstand in ply; de claim van het boek is naar diezelfde partij vertaald.']
    sec = None
    for r in lines:
        if r['section'] != sec:
            sec = r['section']
            out += ['', f'#### Sectie {sec}', '', '| | vanaf | variant | aan zet | boek | database | toelichting |',
                    '|---|---|---|---|---|---|---|']
        notes = []
        if r.get('note'):
            notes.append(f"*{r['note']}*")
        notes += r['issues']
        notes += [f'bevestigd: {c}' for c in r.get('confirmed', [])]
        notes += r.get('info', [])
        if 'illegal' in r:
            stm, cl, res = '–', claim_label(r['claim'], 'W') if r['claim'] else '–', '–'
            cl = '–' if r['claim'] is None else r['claim'].replace('W+', 'wit wint').replace('Z+', 'zwart wint').replace('notW+', 'wit wint niet').replace('=', 'remise')
        else:
            e = r['end_fen'][0]
            stm, cl, res = ('wit' if e == 'W' else 'zwart'), (claim_label(r['claim'], e) or '–'), r['end_val']
        text = r['text'].replace('|', '\\|')
        out.append(f"| {STATUS[r['status']]} | {r['start']} | {text} | {stm} | {cl} | {res} | {'<br>'.join(notes)} |")
    return '\n'.join(out)


def main():
    data = json.loads((HERE / 'results.json').read_text())
    counts = {}
    for r in data['lines']:
        counts[r['status']] = counts.get(r['status'], 0) + 1
    dcount = {'ok': 0, 'fout': 0}
    for d in data['diagrams']:
        for s in d['sides'].values():
            if s.get('ok') is True:
                dcount['ok'] += 1
            elif s.get('ok') is False:
                dcount['fout'] += 1
    parts = [
        f"Diagramclaims: {dcount['ok']} bevestigd, {dcount['fout']} onjuist. "
        f"Varianten: {len(data['lines'])}, waarvan " + ', '.join(f'{STATUS[k]} {k}: {v}' for k, v in sorted(counts.items())) + '.',
        '', '### Weerleggingen van onjuiste diagramclaims', '', (HERE / 'weerleggingen.md').read_text(),
        '### Diagrammen', '', diagram_blocks(data['diagrams']), '', '### Varianten', '', lines_table(data['lines']), '',
        f"Database-statistiek: `{' '.join(data['stats'])}`", ''
    ]
    tables = '\n'.join(parts)
    doc = '\n'.join([(HERE / 'bevindingen.md').read_text(), (HERE / 'methode.md').read_text(),
                     '## Resultaten per diagram en variant', '', tables])
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / 'RAPPORT.md'
    out.write_text(doc)


if __name__ == '__main__':
    main()
