#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Static companion pages (GitHub Pages, docs/) for the check of "Strategie der 36 velden": text with
clickable moves, a large board that follows them, and for every position the database value and all moves.

The pages are generated locally (the databases are needed) into site/ (not tracked) and published on the
gh-pages branch:

    python3 strategie36/site.py data/egdb_6x6
    git worktree add ../gh-pages gh-pages && cp site/* ../gh-pages/ && cd ../gh-pages && git add -A && git commit

usage: site.py path/to/egdb_6x6 [--out site]
"""

import argparse
import html
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
from check import Engine, parse_line   # noqa: E402
from claims import DIAGRAMS, LINES     # noqa: E402

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'
WSC = 30000


def label(s):
    """mate score for the side to move -> W n / L n / D"""
    if s == 0:
        return 'D'
    return f'W{WSC - s}' if s > 0 else f'L{WSC + s}'


def mover(v):
    """value of the position after a move (for the opponent) -> value of the move for the side making it"""
    if v == 'D':
        return 'D'
    return ('W' if v[0] == 'L' else 'L') + str(int(v[1:]) + 1)


def rank(v):
    if v == 'D':
        return (1, 0)
    return (0, int(v[1:])) if v[0] == 'W' else (2, -int(v[1:]))


def pieces(fen):
    return sum(len([x for x in part[1:].split(',') if x]) for part in fen.split(':')[1:])


class Oracle:
    """Values from the databases (at most 10 pieces) and from the solved opening tree (more pieces)."""

    def __init__(self, eng):
        self.eng = eng
        self.tree = json.loads((ROOT / 'results' / 'opening_6x6.json').read_text())['tree']

    def analyse(self, fen):
        """-> (value, [(notation, value for the mover, next fen)]) best first"""
        if pieces(fen) <= 10:
            return self.eng.analyse(fen)
        out = self.eng.cmd('moves ' + fen)
        legal = []
        for r in out:
            _, nota, nxt = r.split()
            v = self.value(nxt)
            legal.append((nota, mover(v) if v else '?', nxt))
        legal.sort(key=lambda m: rank(m[1]) if m[1] != '?' else (3, 0))
        return self.value(fen) or '?', legal

    def value(self, fen):
        if pieces(fen) <= 10:
            return self.eng.analyse(fen)[0]
        node = self.tree.get(fen)
        if node and node['score'][0] == node['score'][1]:
            return label(node['score'][0])
        return None


class Page:
    def __init__(self, oracle):
        self.o = oracle
        self.pos = []          # {'f': fen, 'last': [from, to], 'prev': i, 'next': i}
        self.index = {}

    def add(self, fen, last=None, prev=None):
        i = len(self.pos)
        self.pos.append({'f': fen, 'last': last, 'prev': prev, 'next': None})
        if prev is not None and self.pos[prev]['next'] is None:
            self.pos[prev]['next'] = i
        self.index.setdefault(fen, i)
        return i

    def child(self, prev, mv):
        sq = [int(x) for x in re.split('[-x]', mv[0])]
        return self.add(mv[2], [sq[0], sq[-1]], prev)

    def diagram(self, fen, caption):
        i = self.add(fen)
        return (f'<figure><a class="diagram" href="#" data-i="{i}"><div class="smallboard" data-fen="{fen}"></div></a>'
                f'<figcaption>{caption}</figcaption></figure>'), i

    def line(self, fen, text, cls='m1', start=None, extend=0):
        """The moves of text (book notation) from fen as links; extend: continue with the best moves for at
        most this many plies (to the end of the game). Returns (html, index of the last position, end fen)."""
        black_first, moves = parse_line(text)
        m0 = re.match(r'\s*(\d+)\.', text)
        number = int(m0.group(1)) if m0 else 1
        prev = self.add(fen) if start is None else start
        out, k = [], (1 if black_first else 0)
        plan = [(f'{f}-{t}', f'{f}x{t}', marks + ('' if not only else '')) for f, t, cap, marks, only in moves]
        n = 0
        while n < len(plan) + extend:
            v, legal = self.o.analyse(fen)
            if not legal:
                break
            if n < len(plan):
                cand = [m for m in legal if m[0] in plan[n][:2]]
                if not cand:
                    break
                mv, marks = cand[0], plan[n][2]
            else:
                mv, marks = legal[0], ''
            best = legal[0][1][0]
            keeping = [m for m in legal if m[1][0] == best]
            flags = []
            if mv[1][0] != best:
                flags.append('err')
            elif len(keeping) == 1 and best in 'WD':
                flags.append('only')
            if n >= len(plan):
                flags.append('db')
            sq = [int(x) for x in re.split('[-x]', mv[0])]
            i = self.add(mv[2], [sq[0], sq[-1]], prev)
            num = f'{number + k // 2}.' if k % 2 == 0 else (f'{number + k // 2}. …' if n == 0 else '')
            nota = mv[0].replace('x', '×') + marks
            tip = f'{mv[1]}'
            out.append(f'<a class="{cls} {" ".join(flags)}" href="#" data-i="{i}" title="{tip}">'
                       f'{num + " " if num else ""}{nota}</a>')
            prev, fen, k, n = i, mv[2], k + 1, n + 1
        v, legal = self.o.analyse(fen)
        if not legal:
            out[-1] = out[-1].replace('</a>', '+</a>')
        return ' '.join(out), prev, fen

    def data(self):
        rows = []
        for p in self.pos:
            v, legal = self.o.analyse(p['f'])
            rows.append({'f': p['f'], 'v': v, 'last': p['last'], 'prev': p['prev'], 'next': p['next'],
                         'm': [[m[0].replace('x', '×'), m[1], self.index.get(m[2])] for m in legal]})
        return json.dumps(rows, ensure_ascii=False, separators=(',', ':'))


def verdict(claim, stm, v):
    """book claim for white -> label for the side to move, and whether the database agrees"""
    want = ({'W+': 'W', '=': 'D', 'Z+': 'L', 'notW+': '≤D'} if stm == 'W' else
            {'W+': 'L', '=': 'D', 'Z+': 'W', 'notW+': '≥D'})[claim]
    got = v[0] if v != 'D' else 'D'
    ok = got == want if len(want) == 1 else (got in ('D', 'L') if want == '≤D' else got in ('D', 'W'))
    return want, ok


def side(stm):
    return 'wit' if stm == 'W' else 'zwart'


def page_7_22(o):
    pg = Page(o)
    body = []
    fen22 = 'B' + DIAGRAMS['7.22'][0][1:]
    intro, last, end = pg.line(START, '1. 13-11 6-8 2. 11×6 2×9 3. 16-13 5-8 4. 14-11 1-5 5. 11×6 9×2 6. 15-12 5-7 '
                                      '7. 17-14')
    assert end == fen22, end
    fig, i22 = pg.diagram(fen22, 'diagram 7.22, zwart aan zet')
    v22, legal22 = o.analyse(fen22)
    body.append('<h1>7.7 Conclusie</h1>')
    body.append(f'<p>De stand na {intro}</p>')
    body.append(fig)
    body.append('<blockquote>„Op drie van de vier zetten wint wit.”</blockquote>')
    moves = ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i22, m)}">7. … {m[0].replace("x", "×")}</a> '
                      f'<span class="val">{m[1]}</span>' for m in legal22)
    body.append(f'<p class="db">Database: zwart aan zet <span class="val">{v22}</span> '
                f'<span class="verdict bad">boek: D ✗</span><br>Wit wint op elke zet; het boek noemt er vier, de vijfde '
                f'(7-11) verliest het snelst: {moves}.</p>')
    for sec, start, text, claim, note in LINES:
        if start != '7.22' or note.startswith('database:'):
            continue
        h, last, end = pg.line(fen22, text, 'm2', start=i22)
        v, _ = o.analyse(end)
        want, ok = verdict(claim, end[0], v)
        badge = (f'<span class="verdict {"ok" if ok else "bad"}">boek: {side(end[0])} aan zet {want} '
                 f'{"✓" if ok else "✗"} · database: {v}</span>')
        body.append(f'<p class="var">{h} {badge}' + (f'<br><span class="note">{html.escape(note)}</span>' if note else '')
                    + '</p>')
    body.append('<h2>De weerlegging</h2>')
    h, last, end = pg.line(fen22, '7. … 7-10 8. 13-11 4-7 9. 11×4 10-13 10. 12-8', 'm1', start=i22, extend=80)
    body.append('<p>Het boek speelt in de hoofdvariant 10. 4-1, en dat geeft de winst weg. De database wint met '
                f'10. 12-8!, tot het einde van de partij:</p><p class="var">{h}</p>')
    body.append('<p class="src">Bron: Herm Jan Brascamp, <a href="https://www.hijkendtc.nl/7-7-conclusie/">7.7 '
                'Conclusie</a>, <em>Strategie der 36 velden</em>, hijkendtc.nl.</p>')
    return '\n'.join(body), pg


TEMPLATE = (HERE / 'site_template.html')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(ROOT / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(ROOT / 'data' / '6x6_10.dtm'))
    ap.add_argument('--out', default=str(ROOT / 'site'))
    args = ap.parse_args()
    o = Oracle(Engine(args.engine, 10, args.db, args.attach))
    out = Path(args.out)
    out.mkdir(exist_ok=True)
    body, pg = page_7_22(o)
    page = (TEMPLATE.read_text().replace('{{TITLE}}', '7.7 Conclusie').replace('{{BODY}}', body)
            .replace('{{DATA}}', pg.data()))
    (out / 'voorbeeld-7-22.html').write_text(page)
    (out / '.nojekyll').write_text('')
    index = ('<h1>Strategie der 36 velden</h1><p>Herm Jan Brascamp, <em>Strategie der 36 velden</em> '
             '(<a href="https://www.hijkendtc.nl/strategie-der-36-velden-hjb/">hijkendtc.nl</a>), nagerekend met exacte '
             'eindspeldatabases voor het 6×6-bord.</p><ul><li><a href="voorbeeld-7-22.html">7.7 Conclusie: diagram '
             '7.22</a></li></ul>')
    ipg = Page(o)
    fig, _ = ipg.diagram(START, 'de beginstand')
    (out / 'index.html').write_text(TEMPLATE.read_text().replace('{{TITLE}}', 'Overzicht')
                                    .replace('{{BODY}}', fig + index).replace('{{DATA}}', ipg.data()))
    print(f'{len(pg.pos)} positions', file=sys.stderr)


if __name__ == '__main__':
    main()
