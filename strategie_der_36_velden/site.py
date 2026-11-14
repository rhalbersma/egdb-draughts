#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Static companion site (GitHub Pages) for the check of "Strategie der 36 velden": one page per page of the
book's website, in the same order, with the diagrams, the claims, the variations and the refutations; every
move is clickable, a large board follows, and for every position the database value and all moves are shown.

The pages are generated locally (the databases are needed) into _site/strategie_der_36_velden/ (not tracked) and published on the
gh-pages branch:

    python3 strategie_der_36_velden/site.py data/egdb_6x6
    git worktree add ../gh-pages gh-pages && cp -r _site/* ../gh-pages/ && cd ../gh-pages && git add -A && git commit

usage: site.py path/to/egdb_6x6 [--out _site/strategie_der_36_velden]
"""

import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / 'common'))
from sitekit import Page, badge, esc, geom_6x6, lang_link, md, menu, write_page   # noqa: E402
from check import Engine, parse_line                           # noqa: E402
from claims import DIAGRAMS, LINES                             # noqa: E402
from corrections import (absolute, agrees, build_book,         # noqa: E402
                         build_known, refutation)
from report import STATUS, sides_to_show                       # noqa: E402

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'
WSC = 30000
ORIGINAL = 'https://www.hijkendtc.nl/strategie-der-36-velden-hjb/'
REPO = 'https://github.com/rhalbersma/egdb-draughts'
TITLE = 'Strategie der 36 velden'
H = 'https://www.hijkendtc.nl/'

# the menu and the landing page: the pages in the order of the book's website, consecutive pages of a chapter
# grouped
LANDING = [
    ('Hoofdstuk 1', ['h1']), ('Hoofdstuk 2', ['h2']), ('Hoofdstuk 3', ['h3', 'h3-3']), ('Hoofdstuk 4', ['h4']),
    ('Hoofdstuk 5 · Scouppe', ['h5a', 'h5b', 'h5c', 'h5d']), ('Hoofdstuk 6 · Opgaven', ['h6a', 'h6b']),
    ('Hoofdstuk 7', ['h7']), ('Hoofdstuk 6 · Oplossingen', ['h6a-opl', 'h6-opl1', 'h6-opl2', 'h6-opl3']),
    ('Hoofdstuk 7 · Vervolg', ['h7-2', 'h7-3', 'h7-4', 'h7-5', 'h7-6', 'h7-7']), ('Hoofdstuk 8', ['h8']),
    ('Hoofdstuk 9', ['h9']),
]

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


# remarks of check.py (Dutch) -> English
NOTE_EN = [('ONMOGELIJK', 'IMPOSSIBLE'), ('is niet legaal in', 'is not legal in'), ('legale zetten', 'legal moves'),
           ('"de enige": ook', '"the only move": also'), ('behoudt/behouden het resultaat', 'keep(s) the result'),
           ('als fout gemarkeerd, maar verandert het resultaat niet', 'marked as an error, but does not change the result'),
           ('blijft', 'stays'), ('verliest sneller', 'loses faster'), ('i.p.v.', 'instead of'), ('bevestigd', 'confirmed'),
           ('(wit)', '(white)'), ('(zwart)', '(black)'), ('zet ', 'move '), ('tekst', 'text'), ('gecorrigeerd', 'corrected')]


def note_en(x):
    for a, b in NOTE_EN:
        x = x.replace(a, b)
    return x


def heading_en(h):
    for a, b in (('Hoofdstuk', 'Chapter'), ('Opgaven', 'Problems'), ('Oplossingen', 'Solutions'),
                 ('Vervolg', 'continued'), ('De opening', 'The opening')):
        h = h.replace(a, b)
    return h


def base(did):
    m = re.match(r'(\d+\.\d+)', did)
    return m.group(1) if m else did


def chapter(did):
    return did.split('.')[0]


class Oracle:
    """Values from the databases (at most 10 pieces) and from the solved opening tree (more pieces)."""

    def __init__(self, eng):
        self.eng = eng
        self.tree = json.loads((HERE / 'opening' / 'opening_6x6.json').read_text())['tree']

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


def book_line(pg, fen, text, cls='m1', start=None, asides=None):
    """the moves of a line of the book (book notation) from fen as links"""
    black_first, mv = parse_line(text)
    m0 = re.match(r'\s*(\d+)\.', text)
    plan = [((f'{f}-{t}', f'{f}x{t}'), marks) for f, t, cap, marks, only in mv]
    return pg.moves(fen, plan, int(m0.group(1)) if m0 else 1, black_first, cls, start, asides=asides)


def claim_label(claim, stm):
    return ({'W+': 'W', '=': 'D', 'Z+': 'L', 'notW+': '≤D'} if stm == 'W' else
            {'W+': 'L', '=': 'D', 'Z+': 'W', 'notW+': '≥D'})[claim]


class Site:
    def __init__(self, eng, out, lang='nl'):
        self.lang = lang
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

    def t(self, nl, en):
        return nl if self.lang == 'nl' else en

    def side(self, stm):
        return self.t('wit', 'white') if stm == 'W' else self.t('zwart', 'black')

    def lab(self, lab):
        return lab if self.lang == 'nl' else lab.replace('Hoofdstuk', 'Chapter')

    def note(self, x):
        return x if self.lang == 'nl' else note_en(x)

    def nav(self, orig=ORIGINAL, current=None):
        t = self.t
        groups = [('', [('index.html', t('Overzicht', 'Overview'))])]
        for heading, pids in LANDING:
            items = []
            for pid in pids:
                _, lab, title, *_ = self.page_of[pid]
                short = lab.replace('Hoofdstuk ', '') if lab else ''
                items.append((f'{pid}.html', f'{short} {title}'.strip()))
            groups.append((heading if self.lang == 'nl' else heading_en(heading), items))
        groups.append((t('De opening', 'The opening'),
                       [('opening.html', t('De beginstand en alle openingen tot 3 ply',
                                           'The start position and all openings to 3 plies'))]))
        home = '../nl/index.html' if self.lang == 'nl' else '../../index.html'      # Dutch: default, English in en/
        alt = f'en/{current}' if self.lang == 'nl' else f'../{current}'
        return (f'<a class="brand" href="index.html">Strategie der 36 velden</a>'
                f'{menu(groups, current, t("Hoofdstukken", "Chapters"))}<a href="opening.html">Opening</a>'
                f'<a href="{orig}">{t("Origineel", "Original")} ↗</a><a href="{home}">{t("Alle sites", "All sites")}</a>'
                f'{lang_link(self.lang, alt)}')

    def page(self):
        pg = Page()
        pg.use('6x6', self.o, geom_6x6())
        return pg

    def write(self, name, title, body, pg, orig=ORIGINAL):
        write_page(self.out / name, title, self.nav(orig, name), body, pg, TITLE, self.lang)

    def claim_refutation(self, pg, fen, i0, did):
        """the refutation of a wrong diagram claim, as in an annotated game: the book's line as the main line with
        "?" at its first move that changes the result and the refutation as an aside; without such a move of the
        book the refutation itself is the main line"""
        cont, number = self.book.get(fen, ([], 1))
        r = refutation(self.eng, fen, cont, number, self.known, did)
        k = r['refute']
        if k is None or k == 0:
            return self.continuation(pg, r, 0, i0, fen, number, fen[0] == 'B', '!' if k == 0 else '', 'm1')
        plan = [((m,), '') for m in cont[:k + 1]]
        h, *_ = pg.moves(fen, plan, number, fen[0] == 'B', 'm1', i0, asides={
            k: lambda prev, f, mnum, black: self.continuation(pg, r, k, prev, f, mnum, black, '!')})
        return h

    def diagram_block(self, pg, did, pid):
        d = self.diag[did]
        out = [f'<section class="diag" id="d{did}"><h3>Diagram {did}</h3>']
        home = self.home[did] == pid
        # lines added from the database (not from the book) are shown as refutations instead
        mylines = [r for r in self.lines[pid] if r['start'] == did and not (r['note'] or '').startswith('database:')]
        if home:
            for stm in sides_to_show(did):
                fen = stm + d['fen'][1:]
                sd = d['sides'][stm]
                fig, i0 = pg.diagram(fen, f'diagram {did}, {self.side(stm)} {self.t("aan zet", "to move")}')
                out.append(fig)
                if 'error' in sd:
                    out.append(f'<p class="claim">{pg.prose(d['note'], '6x6')}: {pg.prose(sd['error'], '6x6')}</p>')
                    continue
                v, legal = self.o.analyse(fen)
                cl = f' · boek: <em>{pg.prose(d['note'], '6x6')}</em>' if sd['claim'] else ''
                ok = sd['ok']
                b = (badge(ok, f'{self.t("boek", "book")}: {claim_label(sd["claim"], stm)} {"✓" if ok else "✗"}')
                     if sd['claim'] else '')
                out.append(f'<p class="claim"><strong>{self.side(stm)} {self.t("aan zet", "to move")}: '
                           f'<span class="val">{v}</span></strong> {b}{cl if not sd["claim"] else ""}</p>')
                if sd['claim'] is None and d['note']:
                    pass
                mv = ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i0, m)}">{pg.label('6x6', m)}</a> '
                               f'<span class="val">{m[1]}</span>' for m in legal)
                out.append(f'<p class="moves">{mv or self.t("geen zetten", "no moves")}</p>')
                if ok is False:
                    out.append(f'<p class="var">{self.claim_refutation(pg, fen, i0, did)}</p>')
            if d['note']:
                out.insert(1, f'<p class="note">{pg.prose(d['note'], '6x6')}</p>')
        elif mylines:
            r0 = mylines[0]
            fig, _ = pg.diagram(r0['start_fen'], f'diagram {did}, {self.side(r0["start_fen"][0])} '
                                                 f'{self.t("aan zet", "to move")}')
            out.append(fig + f'<p class="note">{self.t("zie ook", "see also")} <a href="{self.home[did]}.html#d{did}">'
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
        # remarks about moves that change the result are shown in the line itself ("?" and an aside)
        notes = [x for x in r['issues'] + r.get('info', []) if 'behoudend' not in x and 'verliest zelfs' not in x]
        notes += [f'bevestigd: {c}' for c in r.get('confirmed', [])
                  if 'terecht als fout gemarkeerd' not in c and 'inderdaad de enige' not in c]
        m0 = re.match(r'\s*(\d+)\.', r['text'])
        number = int(m0.group(1)) if m0 else 1
        tail = ''
        if 'illegal' in r:
            body = f'<em>{pg.prose(r['text'], '6x6')}</em>'
            verdict = badge(None, self.t('✏️ notatiefout', '✏️ notation error'))
        else:
            asides = {}
            if r['status'] == 'onjuist':
                cont = [p['move'] for p in r['plies']]
                absv = [absolute(p['fen'], p['val_before']) for p in r['plies']]
                okk = [i for i, a in enumerate(absv) if agrees(r['claim'], a)]
                ref = refutation(self.eng, fen, cont, number, self.known, r['start'],
                                 refute_at=okk[-1] if okk else len(cont), stop_on_book=False)
                k = ref['refute']
                if k is not None:
                    asides[k] = lambda prev, f, mnum, black, ref=ref, k=k: self.continuation(pg, ref, k, prev, f, mnum,
                                                                                         black, '!')
            body, last, end, n = book_line(pg, fen, r['text'], 'm2', i0, asides)
            if r['status'] == 'onjuist' and ref['refute'] is None:
                b = (number + (n + (1 if fen[0] == 'B' else 0)) // 2)
                h = self.continuation(pg, ref, ref['nbook'], last, end, b, (n + (1 if fen[0] == 'B' else 0)) % 2 == 1, '')
                tail = ' ' + (h if h.startswith(' →') or h.startswith(' …') else pg.aside(last, h))
            e = r['end_fen'][0]
            if r['claim']:
                ok = r.get('claim_ok')
                verdict = badge(ok, f'{icon} {self.t("boek", "book")}: {self.side(e)} {self.t("aan zet", "to move")} '
                                    f'{claim_label(r["claim"], e)} · database: {r["end_val"]}')
            else:
                verdict = badge(None, f'{icon} database: {self.side(e)} {self.t("aan zet", "to move")} {r["end_val"]}')
        out = [f'<p class="var">{body}{tail} {verdict}']
        if r['note']:
            out.append(f'<br><span class="note">{pg.prose(r['note'], '6x6')}</span>')
        out.append('</p>')
        if notes:
            out.append('<ul class="issues">' + ''.join(f'<li>{pg.prose(self.note(x), '6x6')}</li>' for x in notes) + '</ul>')
        return '\n'.join(out)

    def continuation(self, pg, ref, k, prev, fen, mnum, black, mark, cls='m3'):
        """the database's line from ply k of a refutation as moves of an aside (or of the main line)"""
        plan = [((m[0],), mark if i == 0 else '') for i, m in enumerate(ref['seq'][k:])]
        h, *_ = pg.moves(fen, plan, mnum, black, cls, prev, nbook=0, auto_aside=False)
        if ref['reached']:
            d = ref['reached'][0]
            h += f' → <a href="{self.home[d]}.html#d{d}">diagram {d}</a> ({ref["reached"][1]})'
        elif ref['ending'] == 'draw':
            h += ' … D'
        return h

    def chapter_page(self, k):
        pid, lab, title, orig, sections, listed = PAGES[k]
        pg = self.page()
        dids = [did for did in DIAGRAMS if self.home[did] == pid or any(r['start'] == did for r in self.lines[pid])]

        def key(did):
            b = base(did)
            return (listed.index(b) if b in listed else len(listed), list(DIAGRAMS).index(did))
        dids.sort(key=key)
        body = [f'<p class="src">{esc(self.lab(lab))}</p>' if lab else '', f'<h1>{esc(title)}</h1>',
                f'<p class="src">Herm Jan Brascamp, <a href="{orig}">{esc(title)}</a>, <em>Strategie der 36 '
                f'velden</em>, hijkendtc.nl. ' + self.t('Hieronder per diagram de claims van het boek, de varianten en '
                                                       'de weerleggingen, nagerekend met de databases.',
                                                       'Per diagram the claims of the book, the variations and the '
                                                       'refutations, checked with the databases. The book\'s own notes '
                                                       'are in Dutch.') + '</p>']
        marks = []
        for did in dids:
            sides = self.diag[did]['sides'].values() if self.home[did] == pid else []
            oks = [x.get('ok') for x in sides if x.get('ok') is not None]
            marks.append(f'<a href="#d{did}">{did}{" ❌" if False in oks else " ✅" if oks else ""}</a>')
        nd = len(dids)
        body.append(f'<details class="onpage"><summary>' + self.t(f'Op deze pagina: {nd} diagram{"men" if nd != 1 else ""}',
                                                               f'On this page: {nd} diagram{"s" if nd != 1 else ""}')
                    + f'</summary>{" ".join(marks)}</details>')
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
            parts.append(f'{self.t("diagrammen", "diagrams")} ✅ {ok}' + (f' ❌ {bad}' if bad else ''))
        if st:
            parts.append(f'{self.t("varianten", "variations")} ' + ' '.join(f'{STATUS[s]} {n}' for s, n in sorted(st.items())))
        return ' · '.join(parts)

    def index_page(self):
        pg = self.page()
        text = (HERE / self.t('bevindingen.md', 'findings_en.md')).read_text()
        intro = text[:text.index('## Belangrijkste bevindingen')] if self.lang == 'nl' else text
        intro = intro[intro.index('\n') + 1:]          # without the title
        parts = re.split(r'(?m)^### ', intro)
        lead, sections = parts[0], [x.split('\n', 1) for x in parts[1:]]
        fig, _ = pg.diagram(START, self.t('de beginstand', 'the start position'))
        toc = ['<ul class="toc">']
        for heading, pids in LANDING:
            links = []
            for pid in pids:
                _, lab, title, *_ = self.page_of[pid]
                short = lab.replace('Hoofdstuk ', '') if lab and len(pids) > 1 else ''
                links.append(f'<a href="{pid}.html">{esc((short + " " + title).strip())}</a>')
            toc.append(f'<li><span class="h">{esc(heading if self.lang == "nl" else heading_en(heading))}</span>'
                       f'{" · ".join(links)}</li>')
        toc.append('</ul>')
        rows = ''.join(f'<tr><td>{esc(self.lab(lab))}</td><td><a href="{pid}.html">{esc(title)}</a></td><td>{self.counts(pid)}</td></tr>'
                       for pid, lab, title, *_ in PAGES)
        t = self.t
        body = ['<h1>Strategie der 36 velden</h1>', fig, md(lead, 'strategie_der_36_velden/', lambda h: pg.prose_html(h, '6x6')),
                t('<h2>Hoofdstukken</h2>', '<h2>Chapters</h2>'),
                t('<p>In de volgorde van de website van het boek. Elk hoofdstuk toont de diagrammen met de claims van '
                  'het boek, de varianten en de weerleggingen; het menu <em>Hoofdstukken</em> bovenaan brengt je overal '
                  'heen.</p>',
                  '<p>In the order of the book\'s website (the chapter titles are Brascamp\'s, in Dutch). Each chapter '
                  'shows the diagrams with the claims of the book, the variations and the refutations; the '
                  '<em>Chapters</em> menu at the top takes you anywhere.</p>'), '\n'.join(toc),
                t('<h2>De opening</h2><p>Met de databases tot en met 10 stukken is ook de beginstand van 6×6 opgelost: '
                  '<a href="opening.html">remise, met alle openingen tot 3 ply</a>.</p>',
                  '<h2>The opening</h2><p>With the databases up to 10 pieces the start position of 6×6 is solved too: '
                  '<a href="opening.html">a draw, with all openings to 3 plies</a>.</p>'),
                t('<h2>Achtergrond</h2>', '<h2>Background</h2>')]
        for head, rest in sections:
            body.append(f'<details class="more"><summary>{esc(head.strip())}</summary><div>'
                        f'{md(rest, "strategie_der_36_velden/", lambda h: pg.prose_html(h, "6x6"))}</div></details>')
        body.append('<details class="more"><summary>' + t('Resultaten per hoofdstuk', 'Results per chapter')
                    + '</summary><div><p>' + t('✅ bevestigd, ❌ onjuist; bij de varianten ook ⚠️ opmerking, ✏️ '
                                               'notatiefout, ℹ️ informatief.',
                                               '✅ confirmed, ❌ wrong; for the variations also ⚠️ remark, ✏️ notation '
                                               'error, ℹ️ information.') + f'</p><table class="tbl">{rows}</table></div></details>')
        body.append(f'<p class="src">' + t('Het volledige rapport (Nederlands), de databases en de programma\'s',
                                           'The full report (Dutch), the databases and the programs')
                    + f': <a href="{REPO}">{REPO}</a>.</p>')
        self.write('index.html', t('Overzicht', 'Overview'), '\n'.join(body), pg)

    def opening_page(self):
        pg = self.page()
        fig, i0 = pg.diagram(START, self.t('de beginstand, wit aan zet', 'the start position, white to move'))
        v0, legal0 = self.o.analyse(START)
        body = [self.t('<h1>De opening</h1>', '<h1>The opening</h1>'), fig, self.t(
                '<p>De beginstand is <strong>remise</strong>. Alle vijf eerste zetten van wit houden remise. Hieronder '
                'elke zet tot 3 ply met zijn waarde, sterkste eerst; klik op een zet om de stelling te zien. De waarden '
                'komen uit de opgeloste openingsboom (stellingen met meer dan 10 stukken) en de databases.</p>',
                '<p>The start position is a <strong>draw</strong>. All five first moves of white keep the draw. Below, '
                'every move to 3 plies with its value, strongest first; click a move to see the position. The values '
                'come from the solved opening tree (positions with more than 10 pieces) and the databases.</p>'),
                '<div class="optree">']
        for m1 in legal0:
            i1 = pg.child(i0, m1)
            body.append(f'<h3><a class="m1" href="#" data-i="{i1}">1. {pg.label('6x6', m1)}</a> '
                        f'<span class="val">{m1[1]}</span></h3>')
            for m2 in self.o.analyse(m1[2])[1]:
                i2 = pg.child(i1, m2)
                rep = [f'<a class="m2" href="#" data-i="{pg.child(i2, m3)}">2. {pg.label('6x6', m3)}</a> '
                       f'<span class="val">{m3[1]}</span>' for m3 in self.o.analyse(m2[2])[1]]
                body.append(f'<p><a class="m1" href="#" data-i="{i2}">1. … {pg.label('6x6', m2)}</a> '
                            f'<span class="val">{m2[1]}</span> — ' + ', '.join(rep) + '</p>')
        body.append('</div>')
        rows = [(self.t('Beginstand', 'Start position'), self.t('remise', 'draw'), self.t('remise', 'draw')),
                (self.t('Eerste zetten van wit', 'First moves of white'), self.t('alle remise', 'all draw'),
                 self.t('alle vijf remise', 'all five draw')),
                (self.t('Antwoorden van zwart', 'Replies of black'),
                 self.t('direct verliezende antwoorden', 'immediately losing replies'),
                 self.t('idem, bv. 1. 13-10 4-7 (L60)', 'likewise, e.g. 1. 13-10 4-7 (L60)')),
                (self.t('Diepste verlies van zwart vanaf de beginstand', 'Deepest loss of black from the start position'),
                 '61 ply', self.t('61 ply (2 + 59 na 1. 13-10 4-7)', '61 plies (2 + 59 after 1. 13-10 4-7)')),
                (self.t('Stellingen t/m 3, 4, 5, 6, 7 stukken', 'Positions up to 3, 4, 5, 6, 7 pieces'),
                 '31,155 · 512,910 · 5,789,784 · 48,881,249 · 322,389,233' if self.lang == 'en' else
                 '31.155 · 512.910 · 5.789.784 · 48.881.249 · 322.389.233',
                 self.t('precies dezelfde aantallen', 'exactly the same numbers')),
                (self.t('Stellingen t/m 8 stukken', 'Positions up to 8 pieces'),
                 self.t('1.617.366.257', '1,617,366,257'), self.t('1.702.191.569 (incl. 7 tegen 1)', '1,702,191,569 (incl. 7 against 1)'))]
        tbl = ''.join(f'<tr><td>{a}</td><td>{pg.prose(b, "6x6")}</td><td>{pg.prose(c, "6x6")}</td></tr>' for a, b, c in rows)
        body.append(self.t('<h2>Eerdere oplossingen</h2>', '<h2>Earlier solutions</h2>'))
        body.append(pg.prose_html(self.t(
            '<p>In oktober 2011 loste Rein Halbersma alle 6×6-varianten zwak op, met alpha-beta en een transpositietabel '
            'maar zonder eindspeldatabases: internationaal 6×6 is remise. Met herhaling als winst voor wit gaf de '
            'beginstand W35, met herhaling als winst voor zwart L36, dus beide kanten kunnen een herhaling afdwingen; '
            'omdat graph-history-interactie genegeerd werd, was dat geen sluitend bewijs. Privé gedeeld met '
            'Ed Gilbert (19 oktober 2011) en voor het eerst gepubliceerd op 8 april 2017, op het World Draughts Forum '
            '(damforum.nl, voorheen laatste.info) in de draad <a href="https://damforum.nl/viewtopic.php?p=118055#p118055">'
            '<em>Search Algorithm</em></a>.</p>',
            '<p>In October 2011 Rein Halbersma weakly solved all 6×6 variants, with alpha-beta and a transposition table '
            'but without endgame databases: international 6×6 is a draw. Scoring a repetition as a white win gave W35 '
            'for the start position, scoring it as a black win gave L36, so both sides can force a repetition; since '
            'graph-history interaction was ignored, this was no conclusive proof. Shared privately with Ed Gilbert '
            '(October 19, 2011) and first published on April 8, 2017, on the World Draughts Forum (damforum.nl, formerly '
            'laatste.info) in the thread <a href="https://damforum.nl/viewtopic.php?p=118055#p118055"><em>Search '
            'Algorithm</em></a>.</p>'),
            '6x6'))
        body.append(self.t(
            '<p>Jan-Jaap van Horssen loste 6×6 in 2016 opnieuw zwak op, nu wel sluitend, met proof-number search en de '
            'databases tot en met 8 stukken. Hij meldde dat op 7 augustus 2017 op het forum (draad '
            '<a href="https://damforum.nl/viewtopic.php?p=120014#p120014"><em>Breakthrough Draughts</em></a>), beschreef '
            'het op 13 augustus 2017 in de draad <a href="https://damforum.nl/viewtopic.php?p=120080#p120080"><em>Complexity '
            'of Checkers and Draughts on board sizes 6x6/…/14x14</em></a> en publiceerde het in 2018 in het ICGA Journal.</p>',
            '<p>Jan-Jaap van Horssen weakly solved 6×6 again in 2016, this time conclusively, with proof-number search and the '
            'databases up to 8 pieces. He reported it on the forum on August 7, 2017 (thread '
            '<a href="https://damforum.nl/viewtopic.php?p=120014#p120014"><em>Breakthrough Draughts</em></a>), described it '
            'on August 13, 2017 in the thread <a href="https://damforum.nl/viewtopic.php?p=120080#p120080"><em>Complexity of '
            'Checkers and Draughts on board sizes 6x6/…/14x14</em></a> and published it in the ICGA Journal in 2018.</p>'))
        body.append(self.t('<h3>Vergelijking met Van Horssen (2018)</h3>', '<h3>Comparison with Van Horssen (2018)</h3>'))
        body.append(self.t(
            '<p>Jan-Jaap van Horssen loste 6×6 al eerder zwak op, in <em>Complexity of checkers and draughts on different '
            'board sizes</em>, ICGA Journal 40 (2018) 341–352: een database met winst/remise/verlies tot en met 8 stukken '
            'en proof-number search vanuit de beginstand. Onze databases en openingsboom zijn daar onafhankelijk van, en '
            'komen op alle punten met hem overeen:</p>',
            '<p>Jan-Jaap van Horssen weakly solved 6×6 earlier, in <em>Complexity of checkers and draughts on different '
            'board sizes</em>, ICGA Journal 40 (2018) 341–352: a win/draw/loss database up to 8 pieces and proof-number '
            'search from the start position. Our databases and opening tree are independent of his, and agree with him on '
            'every point:</p>'))
        body.append(f'<table class="tbl"><tr><th></th><th>Van Horssen (2018)</th><th>{self.t("Hier", "Here")}</th></tr>{tbl}</table>')
        body.append(self.t(
            '<p>Het verschil bij 8 stukken is precies 84.825.312: de stellingen met 7 stukken tegen 1, die Van Horssen '
            'terecht weglaat, want elke kant begint met 6 stukken. Hij bewijst de waarde van de beginstand (zonder '
            'transpositietabel, om graph-history-problemen te vermijden); hier zijn de afstanden tot het einde exact voor '
            'de hele boom tot en met ply 4.</p>',
            '<p>The difference at 8 pieces is exactly 84,825,312: the positions with 7 pieces against 1, which Van Horssen '
            'rightly leaves out, since each side starts with 6 pieces. He proves the value of the start position (without '
            'a transposition table, to avoid graph-history problems); here the distances to the end are exact for the '
            'whole tree up to ply 4.</p>'))
        self.write('opening.html', self.t('De opening', 'The opening'), '\n'.join(body), pg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(ROOT / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(ROOT / 'data' / '6x6_10.dtm'))
    ap.add_argument('--out', default=str(ROOT / '_site' / 'strategie_der_36_velden'))
    args = ap.parse_args()
    eng = Engine(args.engine, 10, args.db, args.attach)
    for lang in ('nl', 'en'):
        out = Path(args.out) / ('' if lang == 'nl' else 'en')
        out.mkdir(parents=True, exist_ok=True)
        for f in out.glob('*.html'):
            f.unlink()
        site = Site(eng, out, lang)
        for k in range(len(PAGES)):
            n = site.chapter_page(k)
            print(f'{lang} {PAGES[k][0]}: {n} positions', file=sys.stderr)
        site.index_page()
        site.opening_page()
    (out / '.nojekyll').write_text('')


if __name__ == '__main__':
    main()
