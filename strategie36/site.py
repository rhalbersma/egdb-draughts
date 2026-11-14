#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Static companion site (GitHub Pages) for the check of "Strategie der 36 velden": one page per page of the
book's website, in the same order, with the diagrams, the claims, the variations and the refutations; every
move is clickable, a large board follows, and for every position the database value and all moves are shown.

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
from check import Engine, parse_line                           # noqa: E402
from claims import DIAGRAMS, LINES                             # noqa: E402
from corrections import (absolute, agrees, build_book,         # noqa: E402
                         build_known, refutation)
from report import STATUS, sides_to_show                       # noqa: E402

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'
WSC = 30000
ORIGINAL = 'https://www.hijkendtc.nl/strategie-der-36-velden-hjb/'
REPO = 'https://github.com/rhalbersma/egdb-draughts'
H = 'https://www.hijkendtc.nl/'

# the pages of the book's website in its own order: (file, label, title, original, sections, diagrams)
PAGES = [
    ('h1', 'Hoofdstuk 1', 'Inleiding', H + 'strategie-der-36-velden/', ['1'], ['1.1', '1.2']),
    ('h2', 'Hoofdstuk 2', 'De lange lijn', H + 'strategie-der-36-velden-ii/', ['2'], ['2.1', '2.2', '2.3']),
    ('h3', 'Hoofdstuk 3', 'Een hinderlijke schijf', H + 'strategie-der-36-velden-2/', ['3.1', '3.2'],
     ['3.1', '3.3', '3.4', '3.5']),
    ('h3-3', 'Hoofdstuk 3.3', 'Moeizame remise en luchtig slot', H + 'strategie-der-36-velden-3/', ['3.3'],
     ['3.4', '3.5']),
    ('h4', 'Hoofdstuk 4', 'Een zwarte schijf op 15 en een zwarte dam', H + 'strategie-der-36-velden-hoofdstuk-4/',
     ['4.1', '4.2', '4.3'], ['4.1', '4.2', '4.3', '4.4', '4.5']),
    ('h5a', 'Hoofdstuk 5a', 'Scouppe I', H + 'scouppe/', ['5.1'], ['5.1', '5.2']),
    ('h5b', 'Hoofdstuk 5b', 'Scouppe Trucs', H + 'scouppe-trucs/', ['5.2'], ['5.4', '5.6']),
    ('h5c', 'Hoofdstuk 5c', 'Scouppe: Dam op 17', H + 'dam-op-17/', ['5.3'], ['5.3', '5.5']),
    ('h5d', 'Hoofdstuk 5d', 'Compositie op finesse 5', H + 'compositie-op-finesse-5/', ['5.4', '5.5'], ['5.7']),
    ('h6a', 'Hoofdstuk 6a', 'Intermezzo: acht opgaven (sectie 6.1)', H + 'acht-opgaven/', ['6.1'],
     ['6.1', '6.2', '6.8', '6.7', '6.3', '6.4', '6.5', '6.9']),
    ('h6b', 'Hoofdstuk 6b', 'Zwaar manoeuvreren (sectie 6.2)', H + 'zwaar-manouevreren/', ['6.2'],
     ['6.6', '6.4', '6.2', '6.7', '6.8', '6.9', '6.10']),
    ('h7', 'Hoofdstuk 7', 'De randslag-opening', H + 'randslag/', ['7.1'], ['7.1', '7.2']),
    ('h6a-opl', 'Hoofdstuk 6a', 'Oplossingen (sectie 6.1)', H + 'oplossingen/', ['6.3'], ['6.1', '6.3', '6.4']),
    ('h6-opl1', '', 'Oplossingen zwaar manoeuvreren I', H + '6-4-oplossingen-2-zwaar-manoeuvreren/', ['6.4'],
     ['6.6', '6.6a', '6.7', '6.7a']),
    ('h6-opl2', '', 'Oplossingen zwaar manoeuvreren II', H + '6-5-oplossingen-3-de-stand-waar-alles-om-draait/',
     ['6.5'], ['6.9', '6.9a']),
    ('h6-opl3', '', 'Oplossingen zwaar manoeuvreren III',
     H + '6-6-oplossingen-4-toegift-nog-steeds-zwaar-manoeuvreren/', ['6.6'],
     ['6.10', '6.11', '6.12', '6.13', '6.14']),
    ('h7-2', 'Hoofdstuk 7.2', 'De derde zet', H + '7-2-de-derde-zet/', ['7.2'], ['7.1', '7.3', '7.4']),
    ('h7-3', 'Hoofdstuk 7.3', 'De vierde zet', H + '7-3-de-vierde-zet/', ['7.3'],
     ['7.4', '7.9', '7.5', '7.7', '7.6', '7.8', '7.10', '7.11']),
    ('h7-4', 'Hoofdstuk 7.4', 'Naar voren', H + '7-4-naar-voren/', ['7.4'], ['7.12', '7.14', '7.13', '7.15']),
    ('h7-5', 'Hoofdstuk 7.5', 'Achteruit', H + '7-5-achteruit-de-zesde-zet/', ['7.5'], ['7.16', '7.17']),
    ('h7-6', 'Hoofdstuk 7.6', 'Een dijk van een stand', H + '7-6-een-dijk-van-een-stand/', ['7.6'],
     ['7.18', '7.19', '7.20', '7.21']),
    ('h7-7', 'Hoofdstuk 7.7', 'Conclusie', H + '7-7-conclusie/', ['7.7'], ['7.22', '7.23']),
    ('h8', 'Hoofdstuk 8.0', 'Een schijf op 4', H + '8-een-schijf-op-4/', ['8'],
     ['8.1', '8.2', '8.3', '8.4', '8.5', '8.6', '8.7']),
    ('h9', 'Hoofdstuk 9.0', 'Het taaie eindspel', H + '9-het-taaie-eindspel/', ['9.1', '9.2', '9.3', '9.4'],
     ['9.1', '9.2', '9.3', '9.4', '9.5', '9.6', '9.7', '9.8', '9.12']),
]


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


def side(stm):
    return 'wit' if stm == 'W' else 'zwart'


def base(did):
    m = re.match(r'(\d+\.\d+)', did)
    return m.group(1) if m else did


def chapter(did):
    return did.split('.')[0]


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

    def moves(self, fen, plan, number, black_first, cls='m1', start=None, extend=0, nbook=None):
        """plan: [(notations, marks)] from fen as links, then at most extend best moves (to the end of the
        game); moves from index nbook on are the database's. Returns (html, last index, end fen, plies)."""
        nbook = len(plan) if nbook is None else nbook
        prev = self.add(fen) if start is None else start
        out, k, n = [], (1 if black_first else 0), 0
        while n < len(plan) + extend:
            v, legal = self.o.analyse(fen)
            if not legal:
                break
            if n < len(plan):
                cand = [m for m in legal if m[0] in plan[n][0]]
                if not cand:
                    break
                mv, marks = cand[0], plan[n][1]
            else:
                mv, marks = legal[0], ''
            best = legal[0][1][0]
            keeping = [m for m in legal if m[1][0] == best]
            flags = []
            if mv[1][0] != best and best != '?':
                flags.append('err')
            elif len(keeping) == 1 and best in 'WD':
                flags.append('only')
            if n >= nbook:
                flags.append('db')
            i = self.child(prev, mv)
            num = f'{number + k // 2}.' if k % 2 == 0 else (f'{number + k // 2}. …' if n == 0 else '')
            nota = mv[0].replace('x', '×') + marks
            out.append(f'<a class="{cls} {" ".join(flags)}" href="#" data-i="{i}" title="{mv[1]}">'
                       f'{num + " " if num else ""}{nota}</a>')
            prev, fen, k, n = i, mv[2], k + 1, n + 1
        if out and not self.o.analyse(fen)[1]:
            out[-1] = out[-1].replace('</a>', '+</a>')
        return ' '.join(out), prev, fen, n

    def line(self, fen, text, cls='m1', start=None):
        black_first, mv = parse_line(text)
        m0 = re.match(r'\s*(\d+)\.', text)
        plan = [((f'{f}-{t}', f'{f}x{t}'), marks) for f, t, cap, marks, only in mv]
        return self.moves(fen, plan, int(m0.group(1)) if m0 else 1, black_first, cls, start)

    def data(self):
        rows = []
        for p in self.pos:
            v, legal = self.o.analyse(p['f'])
            rows.append({'f': p['f'], 'v': v, 'last': p['last'], 'prev': p['prev'], 'next': p['next'],
                         'm': [[m[0].replace('x', '×'), m[1], self.index.get(m[2])] for m in legal]})
        return json.dumps(rows, ensure_ascii=False, separators=(',', ':'))


def claim_label(claim, stm):
    return ({'W+': 'W', '=': 'D', 'Z+': 'L', 'notW+': '≤D'} if stm == 'W' else
            {'W+': 'L', '=': 'D', 'Z+': 'W', 'notW+': '≥D'})[claim]


def badge(ok, text):
    return f'<span class="verdict {"ok" if ok else "bad" if ok is False else ""}">{text}</span>'


def esc(s):
    return html.escape(s, quote=False)


def md(text):
    """the small part of Markdown used in bevindingen.md -> HTML"""
    def inline(s):
        s = esc(s)
        s = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', lambda m: f'<a href="{m.group(2).replace("../", REPO + "/blob/main/")}">{m.group(1)}</a>', s)
        s = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', s)
        s = re.sub(r'\*(.+?)\*', r'<em>\1</em>', s)
        s = re.sub(r'`(.+?)`', r'<code>\1</code>', s)
        return s
    out, para, items = [], [], []

    def flush():
        if para:
            out.append('<p>' + inline(' '.join(para)) + '</p>')
            para.clear()
        if items:
            out.append('<ul>' + ''.join(f'<li>{inline(x)}</li>' for x in items) + '</ul>')
            items.clear()
    for ln in text.split('\n'):
        if not ln.strip():
            flush()
        elif ln.startswith('#'):
            flush()
            lvl = len(ln) - len(ln.lstrip('#'))
            out.append(f'<h{min(lvl, 3)}>{inline(ln.lstrip("#").strip())}</h{min(lvl, 3)}>')
        elif ln.startswith('* '):
            if para:
                flush()
            items.append(ln[2:].strip())
        elif ln.startswith('  ') and items:
            items[-1] += ' ' + ln.strip()
        else:
            para.append(ln.strip())
    flush()
    return '\n'.join(out)


class Site:
    def __init__(self, eng, out):
        self.eng = eng
        self.o = Oracle(eng)
        self.out = out
        self.results = json.loads((HERE / 'results.json').read_text())
        self.diag = {d['id']: d for d in self.results['diagrams']}
        self.known = build_known()
        self.book = build_book(eng)
        sec_page = {s: p[0] for p in PAGES for s in p[4]}
        self.lines = {p[0]: [] for p in PAGES}
        for r in self.results['lines']:
            self.lines[sec_page[r['section']]].append(r)
        # the home page of every diagram: the first page with one of its variations, else the first page of its
        # chapter that shows it, else the first page of its chapter
        order = [p[0] for p in PAGES]
        self.home = {}
        for did in DIAGRAMS:
            cand = [p for p in order if any(r['start'] == did for r in self.lines[p])]
            if not cand:
                cand = [p[0] for p in PAGES if base(did) in p[5] and chapter(p[4][0]) == chapter(did)]
            if not cand:
                cand = [p[0] for p in PAGES if chapter(p[4][0]) == chapter(did)]
            self.home[did] = cand[0]
        self.page_of = {p[0]: p for p in PAGES}

    def nav(self, orig=ORIGINAL):
        return (f'<a href="index.html">Strategie der 36 velden · gecontroleerd</a><a href="opening.html">opening</a>'
                f'<a href="{orig}">het origineel</a>')

    def write(self, name, title, body, pg, orig=ORIGINAL):
        tpl = (HERE / 'site_template.html').read_text()
        page = (tpl.replace('{{TITLE}}', esc(title)).replace('{{NAV}}', self.nav(orig)).replace('{{BODY}}', body)
                .replace('{{DATA}}', pg.data()))
        (self.out / name).write_text(page)

    def refutation_html(self, pg, fen, start, cont, number, own, refute_at=None, stop_on_book=True):
        r = refutation(self.eng, fen, cont, number, self.known, own, refute_at=refute_at, stop_on_book=stop_on_book)
        plan = [((m[0],), '!' if i == r['refute'] else '') for i, m in enumerate(r['seq'])]
        nbook = r['refute'] if r['refute'] is not None else r['nbook']
        h, _, _, _ = pg.moves(fen, plan, number, fen[0] == 'B', 'm1', start, nbook=nbook)
        if r['reached']:
            d = r['reached'][0]
            h += f' → <a href="{self.home[d]}.html#d{d}">diagram {d}</a> ({r["reached"][1]})'
        elif r['ending'] == 'draw':
            h += ' … D'
        return h

    def diagram_block(self, pg, did, pid):
        d = self.diag[did]
        out = [f'<section class="diag" id="d{did}"><h3>Diagram {did}</h3>']
        home = self.home[did] == pid
        mylines = [r for r in self.lines[pid] if r['start'] == did]
        if home:
            for stm in sides_to_show(did):
                fen = stm + d['fen'][1:]
                sd = d['sides'][stm]
                fig, i0 = pg.diagram(fen, f'diagram {did}, {side(stm)} aan zet')
                out.append(fig)
                if 'error' in sd:
                    out.append(f'<p class="claim">{esc(d["note"])}: {esc(sd["error"])}</p>')
                    continue
                v, legal = self.o.analyse(fen)
                cl = f' · boek: <em>{esc(d["note"])}</em>' if sd['claim'] else ''
                ok = sd['ok']
                b = badge(ok, f'boek: {claim_label(sd["claim"], stm)} {"✓" if ok else "✗"}') if sd['claim'] else ''
                out.append(f'<p class="claim"><strong>{side(stm)} aan zet: <span class="val">{v}</span></strong> {b}'
                           f'{cl if not sd["claim"] else ""}</p>')
                if sd['claim'] is None and d['note']:
                    pass
                mv = ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i0, m)}">{m[0].replace("x", "×")}</a> '
                               f'<span class="val">{m[1]}</span>' for m in legal)
                out.append(f'<p class="moves">{mv or "geen zetten"}</p>')
                if ok is False:
                    cont, number = self.book.get(fen, ([], 1))
                    h = self.refutation_html(pg, fen, i0, cont, number, did)
                    out.append(f'<div class="refute"><span class="t">weerlegging</span><br>{h}</div>')
            if d['note']:
                out.insert(1, f'<p class="note">{esc(d["note"])}</p>')
        elif mylines:
            r0 = mylines[0]
            fig, _ = pg.diagram(r0['start_fen'], f'diagram {did}, {side(r0["start_fen"][0])} aan zet')
            out.append(fig + f'<p class="note">zie ook <a href="{self.home[did]}.html#d{did}">'
                             f'{esc(self.page_of[self.home[did]][2])}</a></p>')
        starts = {}
        for r in mylines:
            out.append(self.line_html(pg, r, starts))
        out.append('</section>')
        return '\n'.join(out)

    def line_html(self, pg, r, starts):
        fen = r['start_fen']
        if fen not in starts:
            starts[fen] = pg.add(fen)
        i0 = starts[fen]
        icon = STATUS[r['status']]
        notes = r['issues'] + r.get('info', []) + [f'bevestigd: {c}' for c in r.get('confirmed', [])]
        if 'illegal' in r:
            body = f'<em>{esc(r["text"])}</em>'
            verdict = badge(None, '✏️ notatiefout')
        else:
            body, _, end, _ = pg.line(fen, r['text'], 'm2', i0)
            e = r['end_fen'][0]
            if r['claim']:
                ok = r.get('claim_ok')
                verdict = badge(ok, f'{icon} boek: {side(e)} aan zet {claim_label(r["claim"], e)} · database: '
                                    f'{r["end_val"]}')
            else:
                verdict = badge(None, f'{icon} database: {side(e)} aan zet {r["end_val"]}')
        out = [f'<p class="var">{body} {verdict}']
        if r['note']:
            out.append(f'<br><span class="note">{esc(r["note"])}</span>')
        out.append('</p>')
        if notes:
            out.append('<ul class="issues">' + ''.join(f'<li>{esc(x)}</li>' for x in notes) + '</ul>')
        if r['status'] == 'onjuist':
            cont = [p['move'] for p in r['plies']]
            absv = [absolute(p['fen'], p['val_before']) for p in r['plies']]
            okk = [i for i, a in enumerate(absv) if agrees(r['claim'], a)]
            m0 = re.match(r'\s*(\d+)\.', r['text'])
            h = self.refutation_html(pg, fen, i0, cont, int(m0.group(1)) if m0 else 1, r['start'],
                                     refute_at=okk[-1] if okk else len(cont), stop_on_book=False)
            out.append(f'<div class="refute"><span class="t">weerlegging</span><br>{h}</div>')
        return '\n'.join(out)

    def chapter_page(self, k):
        pid, lab, title, orig, sections, listed = PAGES[k]
        pg = Page(self.o)
        dids = [did for did in DIAGRAMS if self.home[did] == pid or any(r['start'] == did for r in self.lines[pid])]

        def key(did):
            b = base(did)
            return (listed.index(b) if b in listed else len(listed), list(DIAGRAMS).index(did))
        dids.sort(key=key)
        body = [f'<p class="src">{esc(lab)}</p>' if lab else '', f'<h1>{esc(title)}</h1>',
                f'<p class="src">Herm Jan Brascamp, <a href="{orig}">{esc(title)}</a>, <em>Strategie der 36 '
                f'velden</em>, hijkendtc.nl. Hieronder per diagram de claims van het boek, de varianten en de '
                f'weerleggingen, nagerekend met de databases.</p>']
        for did in dids:
            body.append(self.diagram_block(pg, did, pid))
        prev = f'<a href="{PAGES[k - 1][0]}.html">← {esc(PAGES[k - 1][2])}</a>' if k > 0 else '<span></span>'
        nxt = f'<a href="{PAGES[k + 1][0]}.html">{esc(PAGES[k + 1][2])} →</a>' if k + 1 < len(PAGES) else '<span></span>'
        body.append(f'<nav class="pager">{prev}{nxt}</nav>')
        self.write(f'{pid}.html', title, '\n'.join(body), pg, orig)
        return len(pg.pos)

    def counts(self, pid):
        ok = bad = 0
        for did, d in self.diag.items():
            if self.home.get(did) != pid:
                continue
            for s in d['sides'].values():
                ok += s.get('ok') is True
                bad += s.get('ok') is False
        st = {}
        for r in self.lines[pid]:
            st[r['status']] = st.get(r['status'], 0) + 1
        parts = []
        if ok or bad:
            parts.append(f'diagrammen ✅ {ok}' + (f' ❌ {bad}' if bad else ''))
        if st:
            parts.append('varianten ' + ' '.join(f'{STATUS[s]} {n}' for s, n in sorted(st.items())))
        return ' · '.join(parts)

    def index_page(self):
        pg = Page(self.o)
        text = (HERE / 'bevindingen.md').read_text()
        intro = text[:text.index('## Belangrijkste bevindingen')]
        intro = intro[intro.index('\n') + 1:]          # without the title
        fig, _ = pg.diagram(START, 'de beginstand')
        toc = ['<ul class="toc">']
        for pid, lab, title, orig, sections, listed in PAGES:
            toc.append(f'<li><span class="h">{esc(lab)}</span><a href="{pid}.html">{esc(title)}</a>'
                       f'<span class="cnt">{self.counts(pid)}</span> <a class="orig" href="{orig}">origineel</a></li>')
        toc.append('</ul>')
        body = ['<h1>Strategie der 36 velden</h1>', fig, md(intro), '<h2>Inhoud</h2>',
                '<p>De hoofdstukken in de volgorde van de website van het boek; per hoofdstuk de diagrammen, de '
                'varianten en de weerleggingen.</p>', '\n'.join(toc),
                '<h2>De opening</h2><p>Met de databases tot en met 10 stukken is ook de beginstand van 6×6 opgelost: '
                '<a href="opening.html">remise, met alle openingen tot 3 ply</a>.</p>',
                f'<p class="src">Het volledige rapport, de databases en de programma\'s: <a href="{REPO}">{REPO}</a>.</p>']
        self.write('index.html', 'Overzicht', '\n'.join(body), pg)

    def opening_page(self):
        pg = Page(self.o)
        fig, i0 = pg.diagram(START, 'de beginstand, wit aan zet')
        v0, legal0 = self.o.analyse(START)
        body = ['<h1>De opening</h1>', fig,
                f'<p>De beginstand is <strong>remise</strong>. Alle vijf eerste zetten van wit houden remise. '
                f'Hieronder elke zet tot 3 ply met zijn waarde, sterkste eerst; klik op een zet om de stelling te '
                f'zien. De waarden komen uit de opgeloste openingsboom (stellingen met meer dan 10 stukken) en de '
                f'databases.</p>', '<div class="optree">']
        for m1 in legal0:
            i1 = pg.child(i0, m1)
            body.append(f'<h3><a class="m1" href="#" data-i="{i1}">1. {m1[0].replace("x", "×")}</a> '
                        f'<span class="val">{m1[1]}</span></h3>')
            for m2 in self.o.analyse(m1[2])[1]:
                i2 = pg.child(i1, m2)
                rep = [f'<a class="m2" href="#" data-i="{pg.child(i2, m3)}">2. {m3[0].replace("x", "×")}</a> '
                       f'<span class="val">{m3[1]}</span>' for m3 in self.o.analyse(m2[2])[1]]
                body.append(f'<p><a class="m1" href="#" data-i="{i2}">1. … {m2[0].replace("x", "×")}</a> '
                            f'<span class="val">{m2[1]}</span> — ' + ', '.join(rep) + '</p>')
        body.append('</div>')
        self.write('opening.html', 'De opening', '\n'.join(body), pg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(ROOT / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(ROOT / 'data' / '6x6_10.dtm'))
    ap.add_argument('--out', default=str(ROOT / 'site'))
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(exist_ok=True)
    for f in out.glob('*.html'):
        f.unlink()
    site = Site(Engine(args.engine, 10, args.db, args.attach), out)
    for k in range(len(PAGES)):
        n = site.chapter_page(k)
        print(f'{PAGES[k][0]}: {n} positions', file=sys.stderr)
    site.index_page()
    site.opening_page()
    (out / '.nojekyll').write_text('')


if __name__ == '__main__':
    main()
