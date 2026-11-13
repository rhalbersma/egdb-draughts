#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Check every claim of "Strategie der 36 velden" against the exact 6x6 databases of egdb_6x6.

usage: check.py path/to/egdb_6x6 [--slicemax 7] [--out report.md] [--json results.json]
"""

import argparse
import json
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from claims import DIAGRAMS, LINES  # noqa: E402


class Engine:
    def __init__(self, exe, slicemax):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        self.cache = {}
        self.cmd(f'slicemax {slicemax}')

    def cmd(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()
        out = []
        while True:
            r = self.p.stdout.readline()
            if not r:
                raise RuntimeError('engine died')
            r = r.rstrip('\n')
            if r == 'end':
                return out
            out.append(r)

    def analyse(self, fen):
        if fen not in self.cache:
            out = self.cmd('pos ' + fen)
            if out and out[0].startswith('error'):
                raise ValueError(f'{out[0]}: {fen}')
            val = out[0].split()[1]
            moves = []
            for r in out[1:]:
                _, nota, v, nxt = r.split()
                moves.append((nota, v, nxt))
            self.cache[fen] = (val, moves)
        return self.cache[fen]


def with_stm(fen, stm):
    return stm + fen[1:]


def stm_of(fen):
    return fen[0]


def result_for_white(val, stm):
    """'W13' etc. for the side to move -> 'W+', '=', 'Z+' for white."""
    if val == 'D':
        return '='
    mover_wins = val[0] == 'W'
    white_wins = mover_wins == (stm == 'W')
    return 'W+' if white_wins else 'Z+'


def outcome(val):
    return 'win' if val[0] == 'W' else 'loss' if val[0] == 'L' else 'draw'


RANK = {'win': 2, 'draw': 1, 'loss': 0}


def claim_ok(claim, res):
    if claim is None:
        return None
    if claim == 'notW+':
        return res != 'W+'
    return claim == res


MOVE_RE = re.compile(r'(\d+)\s*([-x×])\s*(\d+)\s*([!?]*)\s*(\[enige\])?')


def parse_line(text):
    """Returns (black_first, [(from, to, capture, marks, only)])."""
    t = text.replace('...', '…')
    black_first = bool(re.match(r'^\s*\d+\.\s*…', t))
    moves = []
    for m in MOVE_RE.finditer(t):
        # skip move numbers like "12." which are not followed by - or x: the regex needs a separator
        moves.append((int(m.group(1)), int(m.group(3)), m.group(2) != '-', m.group(4), bool(m.group(5))))
    return black_first, moves


def fmt_val(v):
    if v == 'D':
        return 'remise'
    n = int(v[1:])
    return ('winst' if v[0] == 'W' else 'verlies') + f' in {n}'


def check_line(eng, sec, start, text, claim, note):
    fen0 = DIAGRAMS[start][0]
    black_first, moves = parse_line(text)
    fen = with_stm(fen0, 'B' if black_first else 'W')
    rec = {'section': sec, 'start': start, 'text': text, 'claim': claim, 'note': note,
           'start_fen': fen, 'issues': [], 'plies': []}
    for i, (f, t, cap, marks, only) in enumerate(moves):
        val, legal = eng.analyse(fen)
        stm = stm_of(fen)
        match = [mv for mv in legal if mv[0] in (f'{f}-{t}', f'{f}x{t}')]
        side = 'wit' if stm == 'W' else 'zwart'
        nota = f'{f}{"×" if cap else "-"}{t}'
        if not match:
            rec['issues'].append(f'ONMOGELIJK: zet {i + 1} ({side}) {nota} is niet legaal in {fen}; '
                                 f'legale zetten: {", ".join(m[0] for m in legal)}')
            rec['illegal'] = i
            break
        if len({m[2] for m in match}) > 1:
            rec['issues'].append(f'meerduidig: {nota} kan op {len(match)} manieren; de eerste is genomen')
        nota_e, v_after, nxt = match[0]
        best = outcome(val)
        got = outcome(v_after)
        keeping = [m[0] for m in legal if outcome(m[1]) == best]
        ply = {'fen': fen, 'move': nota_e, 'val_before': val, 'val_after_mover': v_after,
               'keeping': keeping, 'marks': marks}
        rec['plies'].append(ply)
        favoured = {'W+': 'W', 'Z+': 'B', 'notW+': 'B'}.get(claim)
        mover_dtm = lambda v: int(v[1:]) if v != 'D' else None
        if RANK[got] < RANK[best]:
            word = {('win', 'draw'): 'geeft de winst weg (remise)', ('win', 'loss'): 'verliest zelfs',
                    ('draw', 'loss'): 'verliest'}[(best, got)]
            alt = ', '.join(keeping)
            if '?' in marks:
                rec.setdefault('confirmed', []).append(f'{nota_e}{marks} {word}, terecht als fout gemarkeerd (beter: {alt})')
            elif stm == favoured or (claim == '=' and best == 'draw'):
                rec['issues'].append(f'zet {i + 1} ({side}) {nota_e}: {word}; behoudend waren: {alt}')
            else:
                rec.setdefault('info', []).append(f'zet {i + 1} ({side}) {nota_e} {word} (behoudend: {alt})')
        elif '?' in marks and '!' not in marks:
            if best == 'loss' and mover_dtm(v_after) < mover_dtm(val):
                rec.setdefault('confirmed', []).append(
                    f'{nota_e}{marks}: verliest sneller ({fmt_val(v_after)} i.p.v. {fmt_val(val)})')
            elif best == 'win' and mover_dtm(v_after) > mover_dtm(val):
                rec.setdefault('info', []).append(
                    f'zet {i + 1} ({side}) {nota_e}{marks}: als fout gemarkeerd, wint nog steeds maar langzamer '
                    f'({fmt_val(v_after)} i.p.v. {fmt_val(val)})')
            else:
                rec['issues'].append(f'zet {i + 1} ({side}) {nota_e}{marks}: als fout gemarkeerd, maar verandert '
                                     f'het resultaat niet ({fmt_val(val)} blijft {fmt_val(v_after)})')
        if '!' in marks and '?' not in marks and RANK[got] == RANK[best] and best != 'loss':
            if len(keeping) > 1:
                ply['note'] = f'{nota_e}{marks}: niet de enige behoudende zet ({", ".join(keeping)})'
        if only:
            if RANK[got] < RANK[best]:
                rec['issues'].append(f'zet {i + 1} {nota_e} "de enige": de zet behoudt het resultaat niet')
            elif len(keeping) > 1:
                others = [m for m in legal if outcome(m[1]) == best and m[0] != nota_e]
                desc = ', '.join(f'{m[0]} ({fmt_val(m[1])})' for m in others)
                rec['issues'].append(f'zet {i + 1} {nota_e} ({fmt_val(v_after)}) "de enige": ook {desc} '
                                     f'behoudt/behouden het resultaat')
            else:
                rec.setdefault('confirmed', []).append(f'{nota_e} is inderdaad de enige behoudende zet')
        fen = nxt
    val, legal = eng.analyse(fen)
    rec['end_fen'] = fen
    rec['end_val'] = val
    rec['end_result'] = result_for_white(val, stm_of(fen))
    if 'illegal' in rec:
        rec['status'] = 'notatie'
    else:
        ok = claim_ok(claim, rec['end_result'])
        rec['claim_ok'] = ok
        if ok is False:
            rec['status'] = 'onjuist'
        elif any(not s.startswith('meerduidig') for s in rec['issues']):
            rec['status'] = 'opmerking'
        else:
            rec['status'] = 'ok' if ok else 'info'
    return rec


def check_diagram(eng, did):
    fen, cw, cb, note = DIAGRAMS[did]
    rec = {'id': did, 'fen': fen, 'note': note, 'sides': {}}
    for stm, claim in (('W', cw), ('B', cb)):
        f = with_stm(fen, stm)
        try:
            val, legal = eng.analyse(f)
        except ValueError as e:
            rec['sides'][stm] = {'error': str(e)}
            continue
        res = result_for_white(val, stm)
        best = outcome(val)
        keeping = [(m[0], m[1]) for m in legal if outcome(m[1]) == best]
        rec['sides'][stm] = {'claim': claim, 'val': val, 'result': res, 'ok': claim_ok(claim, res),
                             'keeping': keeping, 'nmoves': len(legal)}
    return rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--slicemax', type=int, default=7)
    ap.add_argument('--json', default=str(Path(__file__).resolve().parent / 'results.json'))
    args = ap.parse_args()
    eng = Engine(args.engine, args.slicemax)
    t0 = time.time()
    diagrams = []
    for did in DIAGRAMS:
        diagrams.append(check_diagram(eng, did))
        print(f'[{time.time() - t0:7.1f}s] diagram {did}', file=sys.stderr, flush=True)
    lines = []
    for sec, start, text, claim, note in LINES:
        lines.append(check_line(eng, sec, start, text, claim, note))
        r = lines[-1]
        print(f'[{time.time() - t0:7.1f}s] {sec} {start} {r["status"]}: {text[:60]}', file=sys.stderr, flush=True)
    stats = eng.cmd('stats')
    Path(args.json).write_text(json.dumps({'diagrams': diagrams, 'lines': lines, 'stats': stats}, indent=1, ensure_ascii=False))
    print('done', stats, file=sys.stderr)


if __name__ == '__main__':
    main()
