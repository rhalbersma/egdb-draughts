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


def fmt_val(v):
    if v is None:
        return '–'
    if v == 'D':
        return 'remise'
    return ('winst' if v[0] == 'W' else 'verlies') + f' ({v[1:]})'


def res_nl(r):
    return {'W+': 'wit wint', '=': 'remise', 'Z+': 'zwart wint', 'notW+': 'wit wint niet', None: '–'}[r]


def mark(ok):
    return {True: '✅', False: '❌', None: ''}[ok]


def diagram_table(diagrams):
    out = ['Tussen haakjes: de afstand tot het einde van de partij in ply (DTM) bij perfect spel.', '',
           '| diagram | stelling | boek, wit aan zet | database | boek, zwart aan zet | database | behoudende zetten (wit aan zet) |',
           '|---|---|---|---|---|---|---|']
    for d in diagrams:
        w, b = d['sides'].get('W', {}), d['sides'].get('B', {})
        if d['id'] == 'start':
            continue

        def cell(s):
            if 'error' in s:
                return 'n.v.t.'
            n = '' if s['val'] == 'D' else f" ({s['val'][1:]})"
            return f"{res_nl(s['result'])}{n} {mark(s['ok'])}"
        kw = w.get('keeping', [])
        keep = ', '.join(m + ('' if v == 'D' else f' ({v[1:]})') for m, v in kw[:6])
        if len(kw) > 6:
            keep += ', …'
        out.append(f"| {d['id']} | `{d['fen'][2:]}` | {res_nl(w.get('claim'))} | {cell(w)} | {res_nl(b.get('claim'))} | {cell(b)} | {keep} |")
    return '\n'.join(out)


STATUS = {'ok': '✅', 'info': 'ℹ️', 'opmerking': '⚠️', 'onjuist': '❌', 'notatie': '✏️'}


def lines_table(lines):
    out = []
    sec = None
    for r in lines:
        if r['section'] != sec:
            sec = r['section']
            out += ['', f'#### Sectie {sec}', '', '| | vanaf | variant | boek | database | toelichting |', '|---|---|---|---|---|---|']
        notes = []
        if r.get('note'):
            notes.append(f"*{r['note']}*")
        notes += r['issues']
        notes += [f'bevestigd: {c}' for c in r.get('confirmed', [])]
        notes += r.get('info', [])
        res = (res_nl(r['end_result']) + ('' if r['end_val'] == 'D' else f" ({r['end_val'][1:]})")) if 'illegal' not in r else '–'
        text = r['text'].replace('|', '\\|')
        out.append(f"| {STATUS[r['status']]} | {r['start']} | {text} | {res_nl(r['claim'])} | {res} | {'<br>'.join(notes)} |")
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
        '', '### Diagrammen', '', diagram_table(data['diagrams']), '', '### Varianten', lines_table(data['lines']), '',
        f"Database-statistiek: `{' '.join(data['stats'])}`", ''
    ]
    tables = '\n'.join(parts)
    doc = '\n'.join([(HERE / 'bevindingen.md').read_text(), (HERE / 'methode.md').read_text(),
                     '## Resultaten per diagram en variant', '', tables])
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / 'RAPPORT.md'
    out.write_text(doc)


if __name__ == '__main__':
    main()
