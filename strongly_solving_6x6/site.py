#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The companion site 6x6 Is Strongly Solved (_site/6x6_is_strongly_solved): for now international draughts on
the 6x6 board, strongly solved: every position up to 12 pieces (at most 6 per side) in the databases, the start position
a draw, the complete opening tree to 4 plies and the longest win per number of pieces (international/). English by
default, Dutch in nl/. Build it with --store (the complete unit store) for values everywhere; with the older databases up
to 10 pieces the solved opening tree fills in above them.

usage: site.py path/to/egdb_6x6 [--store data/units | --db data/6x6_le9.db --attach data/6x6_10.dtm] [--out _site/6x6_is_strongly_solved]
"""

import argparse
import importlib.util
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / 'common'))
from sitekit import Page, esc, geom_6x6, lang_link, menu, write_page   # noqa: E402

# the oracle (databases plus the solved opening tree) and the engine of the Strategie der 36 velden site
_spec = importlib.util.spec_from_file_location('strategie_site', ROOT / 'strategie_der_36_velden' / 'site.py')
S = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(S)
START, REPO, Oracle, Engine = S.START, S.REPO, S.Oracle, S.Engine

TITLE = '6×6 Is Strongly Solved'

# international draughts, in chapters with one page per section: (Dutch, English, [(file, Dutch, English)])
CHAPTERS = [
    ('De opgeloste beginstand', 'The solved start position', [
        ('international.html', 'De openingsboom tot 4 ply', 'The opening tree to 4 plies')]),
    ('Eerdere oplossingen', 'Earlier solutions', [
        ('history.html', 'Perft en een zoektocht (2009, 2011)', 'Perft and a search (2009, 2011)'),
        ('van-horssen.html', 'Van Horssen (2016–2018)', 'Van Horssen (2016–2018)')]),
    ('Sterk opgelost', 'Strongly solved', [
        ('status.html', 'Elke stelling tot en met 12 stukken', 'Every position up to 12 pieces'),
        ('longest.html', 'De langste winst per aantal stukken', 'The longest win per number of pieces')]),
    ('De databases tot en met 12 stukken', 'The databases up to 12 pieces', [
        ('towards-12.html', 'Omvang en bouw', 'Size and build'),
        ('compression.html', 'Hoe goed comprimeren de databases?', 'How well do the databases compress?')]),
]
# every page in reading order: (file, Dutch, English, chapter number or None)
PAGES = [('index.html', 'Overzicht', 'Overview', None)] + [(f, nl, en, c) for c, (_, _, secs) in enumerate(CHAPTERS)
                                                        for f, nl, en in secs]


class Site:
    def __init__(self, eng, out, lang='en'):
        self.lang = lang
        self.o = Oracle(eng)
        self.out = out

    def t(self, nl, en):
        return nl if self.lang == 'nl' else en

    def page(self):
        pg = Page()
        pg.use('6x6', self.o, geom_6x6())
        return pg

    def title(self, k):
        return PAGES[k][1] if self.lang == 'nl' else PAGES[k][2]

    def chapter(self, c):
        return CHAPTERS[c][0] if self.lang == 'nl' else CHAPTERS[c][1]

    def nav(self, current):
        t = self.t
        home = '../index.html' if self.lang == 'en' else '../../nl/index.html'      # English: default, Dutch in nl/
        alt = f'nl/{current}' if self.lang == 'en' else f'../{current}'
        groups = [('', [(PAGES[0][0], self.title(0))])] + [
            (self.chapter(c), [(f, nl if self.lang == 'nl' else en) for f, nl, en in secs]) for c, (_, _, secs) in enumerate(CHAPTERS)]
        return (f'<a class="brand" href="index.html">{esc(TITLE)}</a>'
                f'{menu(groups, current, t("Hoofdstukken", "Chapters"))}'
                f'<a href="{home}">{t("Alle sites", "All sites")}</a>{lang_link(self.lang, alt)}')

    def write(self, name, body, pg):
        k = [p[0] for p in PAGES].index(name)
        if PAGES[k][3] is not None:      # a section page: its chapter above its title
            body = f'<p class="src">{esc(self.chapter(PAGES[k][3]))}</p><h1>{esc(self.title(k))}</h1>\n' + body
        prev = f'<a href="{PAGES[k - 1][0]}">← {esc(self.title(k - 1))}</a>' if k > 0 else '<span></span>'
        nxt = f'<a href="{PAGES[k + 1][0]}">{esc(self.title(k + 1))} →</a>' if k + 1 < len(PAGES) else '<span></span>'
        body += f'\n<nav class="pager">{prev}{nxt}</nav>'
        write_page(self.out / name, self.title(k), self.nav(name), body, pg, TITLE, self.lang)

    def index(self):
        pg = self.page()
        t = self.t
        fig, _ = pg.diagram(START, t('de beginstand van 6×6, wit aan zet', 'the start position of 6×6, white to move'))
        body = [f'<h1>{TITLE}</h1>', fig, t(
            '<p>Dammen op het 6×6-bord is <strong>sterk opgelost</strong>: van elke legale stelling, met hooguit 6 stukken '
            'per kant, is de waarde bekend, met de exacte afstand tot het einde van de partij. Dat zijn 63,8 miljard '
            'stellingen, in exacte databases tot en met 12 stukken. De beginstand is <strong>remise</strong>. Voorlopig met '
            'één variant: internationaal dammen.</p>',
            '<p>Draughts on the 6×6 board is <strong>strongly solved</strong>: the value of every legal position, with at '
            'most 6 pieces per side, is known, with the exact distance to the end of the game. That is 63.8 billion '
            'positions, in exact databases up to 12 pieces. The start position is a <strong>draw</strong>. For now with one '
            'variant: international draughts.</p>'),
            t('<h2>Internationaal dammen</h2>', '<h2>International draughts</h2>'),
            '<ul class="toc">' + ''.join(
                f'<li><span class="h">{esc(self.chapter(c))}</span>'
                + (f'<a href="{secs[0][0]}">{esc(secs[0][1] if self.lang == "nl" else secs[0][2])}</a>' if len(secs) == 1 else
                   '<ul class="sub">' + ''.join(f'<li><a href="{f}">{esc(nl if self.lang == "nl" else en)}</a></li>' for f, nl, en in secs)
                   + '</ul>') + '</li>' for c, (_, _, secs) in enumerate(CHAPTERS)) + '</ul>',
            t(f'<p>Het eerste gebruik van deze databases was de controle van Herm Jan Brascamps <em>Strategie der 36 '
              f'velden</em>, met <a href="../../strategie_der_36_velden/">een eigen site</a>.</p>',
              f'<p>The first use of these databases was checking Herm Jan Brascamp\'s <em>Strategie der 36 velden</em>, '
              f'with <a href="../strategie_der_36_velden/en/">a site of its own</a>.</p>'),
            f'<p class="src">' + t('De programma\'s en databases', 'The programs and databases') + f': <a href="{REPO}">{REPO}</a>.</p>']
        self.write('index.html', '\n'.join(body), pg)

    def strength(self):
        """the strong solution: every position up to 12 pieces"""
        t = self.t
        num = lambda x: f'{x:,}'.replace(',', '.') if self.lang == 'nl' else f'{x:,}'
        rows = [(t('Tot en met 10 stukken', 'Up to 10 pieces'), 18_379_036_017), (t('11 stukken', '11 pieces'), 22_071_836_160),
                (t('12 stukken', '12 pieces'), 23_385_567_744), (t('Samen', 'Together'), 63_836_439_921)]
        tbl = ''.join(f'<tr><td>{a}</td><td>{num(b)}</td><td>{t("sterk opgelost, met exacte afstand tot het einde (DTM)", "strongly solved, with the exact distance to the end (DTM)")}</td></tr>'
                      for a, b in rows)
        return (t(
            '<p>In de termen van Allis (1994) is 6×6 <strong>sterk opgelost</strong>: de waarde van elke legale stelling '
            'is bekend. Van Horssen loste het spel eerder <strong>zwak</strong> op: de waarde van de beginstand, met een '
            'strategie om die te halen. Hier staat elke stelling met hooguit 6 stukken per kant in de databases, met de '
            'exacte afstand tot het einde van de partij bij perfect spel, de beginstand met 12 stukken inbegrepen:</p>',
            '<p>In the terms of Allis (1994) 6×6 is <strong>strongly solved</strong>: the value of every legal position is '
            'known. Van Horssen solved the game <strong>weakly</strong> before: the value of the start position, with a '
            'strategy to achieve it. Here every position with at most 6 pieces per side is in the databases, with the exact '
            'distance to the end of the game under perfect play, the start position with 12 pieces included:</p>')
            + f'<table class="tbl"><tr><th></th><th>{t("Stellingen", "Positions")}</th><th>Status</th></tr>{tbl}</table>'
            + t('<p class="note">Legale stellingen met hooguit 6 stukken per kant en wit aan zet, zoals Van Horssen telt.</p>',
                '<p class="note">Legal positions with at most 6 pieces per side and white to move, as Van Horssen counts.</p>')
            + t('<h2>Gecontroleerd</h2><p>Elke eenheid (n stukken, waarvan m schijven) is na het bouwen stelling voor stelling '
                'gecontroleerd: elke waarde opnieuw berekend uit die van zijn opvolgers. Daarna is ze gecomprimeerd en is elk '
                'blok vergeleken met de ongecomprimeerde waarden; het controlegetal (CRC32C) van alle 88 eenheden staat in '
                '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/international/crc32c_6x6.txt">'
                '<code>crc32c_6x6.txt</code></a>. De openingsboom tot 4 ply komt rechtstreeks uit de databases, en elke '
                'inwendige stelling van die boom heeft dezelfde waarde als de minimax van zijn zetten. Eerder was de '
                'openingsboom bewezen met intervallen, boven de databases tot en met 10 stukken; die waarden komen op alle '
                '349 stellingen met de volledige databases overeen.</p>',
                '<h2>Checked</h2><p>After it was built, every unit (n pieces, of which m men) was checked position by '
                'position: every value recomputed from those of its successors. Then it was compressed and every block was '
                'compared with the uncompressed values; the checksum (CRC32C) of all 88 units is in '
                '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/international/crc32c_6x6.txt">'
                '<code>crc32c_6x6.txt</code></a>. The opening tree to 4 plies comes straight from the databases, and every '
                'interior position of that tree has the same value as the minimax of its moves. Before, the opening tree was '
                'proven with intervals, above the databases up to 10 pieces; those values agree with the complete databases '
                'on all 349 positions.</p>'))

    def longest(self):
        """the longest win per number of pieces (international/longest_wins_6x6.txt, from longest_wins.py): a table, the
        diagrams in a grid in reading order (2 to 12 pieces), then the winning line of each to the end of the game where
        international/longest_wins_6x6_lines.txt (longest_lines.py) has it"""
        t = self.t
        pg = self.page()
        lines_file = HERE / 'international' / 'longest_wins_6x6_lines.txt'
        lines = {r.split()[2]: r.split()[3:] for r in lines_file.read_text().splitlines()} if lines_file.exists() else {}
        rows, figs, played = [], [], []
        for line in (HERE / 'international' / 'longest_wins_6x6.txt').read_text().splitlines():
            n, v, fen = line.split()
            side = t('wit', 'white') if fen[0] == 'W' else t('zwart', 'black')
            fig, i = pg.diagram(fen, t(f'{n} stukken: {side} aan zet wint in {v[1:]} ply',
                                       f'{n} pieces: {side} to move wins in {v[1:]} plies'))
            rows.append(f'<tr><td>{n}</td><td>{v[1:]}</td><td><code>{esc(fen)}</code></td></tr>')
            figs.append(fig)
            if fen in lines:                    # the winning line, every move clickable
                html = pg.moves(fen, [((m,), '') for m in lines[fen]], 1, fen[0] == 'B', 'm1', start=i)[0]
                played.append(f'<h3>{t(f"{n} stukken", f"{n} pieces")} ({v})</h3><p class="line">{html}</p>')
        body = [t('<p>Per aantal stukken de stelling met de langste winst bij perfect spel, in ply (halve zetten), voor de '
                  'partij aan zet. Tot en met 8 stukken zijn de stellingen ook met onafhankelijk gebouwde databases '
                  'nagerekend. Met 12 stukken is de langste winst niet langer dan met 11: beide 111 ply.</p>',
                  '<p>Per number of pieces the position with the longest win under perfect play, in plies (half moves), for '
                  'the side to move. Up to 8 pieces the positions were also recomputed with independently built '
                  'databases. With 12 pieces the longest win is no longer than with 11: both 111 plies.</p>'),
                f'<table class="tbl"><tr><th>{t("Stukken", "Pieces")}</th><th>{t("Langste winst (ply)", "Longest win (plies)")}</th>'
                f'<th>{t("Stelling", "Position")}</th></tr>{"".join(rows)}</table>',
                '<div class="grid">' + ''.join(figs) + '</div>']
        if played:
            body += [t('<h2>De winnende lijnen</h2><p>Perfect spel tot het einde van de partij: de winnaar kiest steeds de '
                       'snelste winst, de verliezer het taaiste verlies. Klik op een zet om de lijn op het grote bord na te '
                       'spelen.</p>',
                       '<h2>The winning lines</h2><p>Perfect play to the end of the game: the winner always takes the fastest '
                       'win, the loser the slowest loss. Click a move to replay the line on the large board.</p>'), *played]
        self.write('longest.html', '\n'.join(body), pg)

    def international(self):
        pg = self.page()
        fig, i0 = pg.diagram(START, self.t('de beginstand, wit aan zet', 'the start position, white to move'))
        v0, legal0 = self.o.analyse(START)
        body = [fig, self.t(
                '<p>De beginstand is <strong>remise</strong>. Alle vijf eerste zetten van wit houden remise. Hieronder de '
                'volledige openingsboom tot 4 ply: elke zet met zijn waarde, sterkste eerst; klik op een zet om de '
                'stelling te zien. De waarden komen rechtstreeks uit de databases, die elke stelling met hooguit 12 stukken '
                'bevatten, met de exacte afstand tot het einde.</p>',
                '<p>The start position is a <strong>draw</strong>. All five first moves of white keep the draw. Below, the '
                'complete opening tree to 4 plies: every move with its value, strongest first; click a move to see the '
                'position. The values come straight from the databases, which hold every position with at most 12 '
                'pieces, with the exact distance to the end.</p>'),
                '<div class="optree">']
        key = lambda m: re.sub(r'[^0-9]+', '-', m[0]).strip('-')
        lab = lambda m: pg.label('6x6', m)
        for m1 in legal0:
            i1 = pg.child(i0, m1)
            k1 = key(m1)
            body.append(f'<h3 id="t-{k1}"><a class="m1" href="#" data-i="{i1}">1. {lab(m1)}</a> '
                        f'<span class="val">{m1[1]}</span></h3>')
            for m2 in self.o.analyse(m1[2])[1]:
                i2 = pg.child(i1, m2)
                body.append(f'<p class="t2" id="t-{k1}-{key(m2)}"><a class="m1" href="#" data-i="{i2}">1. … {lab(m2)}</a> '
                            f'<span class="val">{m2[1]}</span></p><ul class="t3">')
                for m3 in self.o.analyse(m2[2])[1]:
                    i3 = pg.child(i2, m3)
                    rep4 = [f'<a class="m2" href="#" data-i="{pg.child(i3, m4)}">2. … {lab(m4)}</a> '
                            f'<span class="val">{m4[1]}</span>' for m4 in self.o.analyse(m3[2])[1]]
                    body.append(f'<li><a class="m1" href="#" data-i="{i3}">2. {lab(m3)}</a> <span class="val">{m3[1]}</span>'
                                + (' — ' + ', '.join(rep4) if rep4 else '') + '</li>')
                body.append('</ul>')
        body.append('</div>')
        body.append(self.t('<p>De randslag-opening 1. 13-11 6-8 2. 11×6 2×9, het onderwerp van hoofdstuk 7 van '
                           '<em>Strategie der 36 velden</em>, staat in deze boom onder <a href="#t-13-11-6-8">1. 13-11 6-8</a>; '
                           'de controle van dat boek heeft <a href="../../strategie_der_36_velden/h7.html">een eigen site</a>.</p>',
                           '<p>The edge-capture opening 1. 13-11 6-8 2. 11×6 2×9, the subject of chapter 7 of <em>Strategie '
                           'der 36 velden</em>, is in this tree under <a href="#t-13-11-6-8">1. 13-11 6-8</a>; the check of '
                           'that book has <a href="../strategie_der_36_velden/en/h7.html">a site of its own</a>.</p>'))
        self.write('international.html', '\n'.join(body), pg)

    def history(self):
        pg = self.page()
        body = []
        rows = [(self.t('Beginstand', 'Start position'), self.t('remise', 'draw'), self.t('remise', 'draw')),
                (self.t('Eerste zetten van wit', 'First moves of white'), self.t('alle remise', 'all draw'),
                 self.t('alle vijf remise', 'all five draw')),
                (self.t('Antwoorden van zwart', 'Replies of black'),
                 self.t('direct verliezende antwoorden', 'immediately losing replies'),
                 self.t('idem, bv. 1. 13-10 4-7 (L60)', 'likewise, e.g. 1. 13-10 4-7 (L60)')),
                (self.t('Diepste verlies van zwart vanaf de beginstand', 'Deepest loss of black from the start position'),
                 '61 ply', self.t('61 ply (2 + 59 na 1. 13-10 4-7)', '61 plies (2 + 59 after 1. 13-10 4-7)')),
                (self.t('Stellingen t/m 3–8 stukken', 'Positions up to 3–8 pieces'),
                 '31,155 · 512,910 · 5,789,784 · 48,881,249 · 322,389,233 · 1,617,366,257' if self.lang == 'en' else
                 '31.155 · 512.910 · 5.789.784 · 48.881.249 · 322.389.233 · 1.617.366.257',
                 self.t('precies dezelfde aantallen', 'exactly the same numbers'))]
        tbl = ''.join(f'<tr><td>{a}</td><td>{pg.prose(b, "6x6")}</td><td>{pg.prose(c, "6x6")}</td></tr>' for a, b, c in rows)
        body.append(pg.prose_html(self.t(
            '<p>Op 5 november 2009 publiceerde Rein Halbersma de eerste perft-getallen voor het 6×6-bord (draad '
            '<a href="https://damforum.nl/viewtopic.php?p=76677#p76677"><em>Perft for arbitrary board sizes</em></a>: 5, 25, '
            '106, 369, 1.301, 4.214, 12.912, 39.526, 113.614, 329.982, 933.564 tot diepte 11); onze zettengenerator geeft '
            'precies dezelfde getallen.</p>'
            '<p>In oktober 2011 deed een zoektocht van Rein Halbersma met alpha-beta vanuit de beginstand, zonder databases '
            'en zonder filter voor graph-history-interactie, vermoeden dat de beginstand remise is (met herhaling als winst '
            'voor wit W35, met herhaling als winst voor zwart L36). Een bewijs van de juistheid was er nog niet. Privé gedeeld met '
            'Ed Gilbert (19 oktober 2011) en voor het eerst gepubliceerd op 8 april 2017, op het World Draughts Forum '
            '(damforum.nl, voorheen laatste.info) in de draad <a href="https://damforum.nl/viewtopic.php?p=118055#p118055">'
            '<em>Search Algorithm</em></a>.</p>',
            '<p>On November 5, 2009 Rein Halbersma published the first perft numbers for the 6×6 board (thread '
            '<a href="https://damforum.nl/viewtopic.php?p=76677#p76677"><em>Perft for arbitrary board sizes</em></a>: 5, 25, '
            '106, 369, 1,301, 4,214, 12,912, 39,526, 113,614, 329,982, 933,564 up to depth 11); our move generator gives '
            'exactly the same numbers.</p>'
            '<p>In October 2011 an alpha-beta search by Rein Halbersma from the start position, without databases and '
            'without a filter for graph-history interaction, suggested that the start position is a draw (scoring a '
            'repetition as a white win W35, as a black win L36). There was no proof of correctness yet. Shared privately '
            'with Ed Gilbert '
            '(October 19, 2011) and first published on April 8, 2017, on the World Draughts Forum (damforum.nl, formerly '
            'laatste.info) in the thread <a href="https://damforum.nl/viewtopic.php?p=118055#p118055"><em>Search '
            'Algorithm</em></a>.</p>'),
            '6x6'))
        self.write('history.html', '\n'.join(body), pg)
        pg = self.page()
        body = []
        body.append(self.t(
            '<p>Jan-Jaap van Horssen loste 6×6 in 2016 als eerste sluitend zwak op, met proof-number search en de '
            'databases tot en met 8 stukken. Hij meldde dat op 7 augustus 2017 op het forum (draad '
            '<a href="https://damforum.nl/viewtopic.php?p=120014#p120014"><em>Breakthrough Draughts</em></a>), beschreef '
            'het op 13 augustus 2017 in de draad <a href="https://damforum.nl/viewtopic.php?p=120080#p120080"><em>Complexity '
            'of Checkers and Draughts on board sizes 6x6/…/14x14</em></a> en publiceerde het in 2018 in het ICGA Journal.</p>',
            '<p>Jan-Jaap van Horssen was the first to weakly solve 6×6 conclusively, in 2016, with proof-number search and the '
            'databases up to 8 pieces. He reported it on the forum on August 7, 2017 (thread '
            '<a href="https://damforum.nl/viewtopic.php?p=120014#p120014"><em>Breakthrough Draughts</em></a>), described it '
            'on August 13, 2017 in the thread <a href="https://damforum.nl/viewtopic.php?p=120080#p120080"><em>Complexity of '
            'Checkers and Draughts on board sizes 6x6/…/14x14</em></a> and published it in the ICGA Journal in 2018.</p>'))
        body.append(self.t('<h2>Vergelijking met Van Horssen (2018)</h2>', '<h2>Comparison with Van Horssen (2018)</h2>'))
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
            '<p>We reproduceren zijn databases dus tot en met 8 stukken. Hij bewijst de waarde van de beginstand (zonder '
            'transpositietabel, om graph-history-problemen te vermijden); hier is de waarde van elke stelling tot en met '
            '12 stukken bekend, met de exacte afstand tot het einde.</p>',
            '<p>So we reproduce his databases up to 8 pieces. He proves the value of the start position (without '
            'a transposition table, to avoid graph-history problems); here the value of every position up to 12 pieces is '
            'known, with the exact distance to the end.</p>'))
        self.write('van-horssen.html', '\n'.join(body), pg)

    def status(self):
        self.write('status.html', self.strength(), self.page())
        self.longest()

    def towards12(self):
        t = self.t
        num = lambda x: f'{x:,}'.replace(',', '.') if self.lang == 'nl' else f'{x:,}'
        rows = [('≤ 10', 18_379_036_017, 18_379_036_017), ('11', 22_071_836_160, 40_450_872_177),
                ('12', 23_385_567_744, 63_836_439_921)]
        tbl = ''.join(f'<tr><td>{a}</td><td>{num(b)}</td><td>{num(c)}</td></tr>' for a, b, c in rows)
        body = [t('<p>De databases tot en met 12 stukken bevatten elke legale stelling met hooguit 6 stukken per kant. Het '
                  'aantal, met wit aan zet (tot en met 8 stukken precies de aantallen van Van Horssen):</p>',
                  '<p>The databases up to 12 pieces hold every legal position with at most 6 pieces per side. Their number, '
                  'with white to move (up to 8 pieces exactly Van Horssen\'s numbers):</p>'),
                f'<table class="tbl"><tr><th>{t("Stukken", "Pieces")}</th><th>{t("Stellingen (hooguit 6 per kant)", "Positions (at most 6 per side)")}</th>'
                f'<th>{t("Cumulatief", "Cumulative")}</th></tr>{tbl}</table>',
                t('<h2>Geheugen</h2><p>Gebouwd wordt per slice: de schijven liggen vast, alleen de dammen variëren. De grootste '
                  'slice is 6 dammen tegen 6, met 17.153.136 stellingen: bij 2 bits per stelling ruim 4 MB. Het geheugen '
                  'voor het bouwen zelf is dus geen probleem. Het probleem is het snel opzoeken in de andere databases, bij '
                  'slagen, schijfzetten en promoties: tot en met 11 stukken is dat ongecomprimeerd zo\'n 10 GB, tot en met '
                  '12 stukken 16 GB. Daarom moeten de databases gecomprimeerd worden, met willekeurige toegang; zie de '
                  '<a href="compression.html">volgende sectie</a>.</p>'
                  '<h2>De bouw</h2><p>Gebouwd eenheid voor eenheid (n stukken, waarvan m schijven) met '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/international/units_6x6.py">'
                  '<code>units_6x6.py</code></a>: bouwen, elke stelling controleren tegen zijn opvolgers, comprimeren (zstd -19, '
                  'blokken van 4 KiB), elk blok vergelijken met de ongecomprimeerde waarden en het controlegetal berekenen. '
                  'Alle 88 eenheden tot en met 12 stukken samen kostten 26 uur en 37 minuten op 10 cores (Xeon E5-1650 v4, '
                  'in een virtuele machine). Dat kan op één machine.</p>',
                  '<h2>Memory</h2><p>The databases are built slice by slice: the men are fixed, only the kings vary. The '
                  'largest slice is 6 kings against 6, with 17,153,136 positions: at 2 bits per position just over 4 MB. '
                  'The memory for building itself is therefore no problem. The problem is fast lookup in the other '
                  'databases, for captures, man moves and promotions: up to 11 pieces that is some 10 GB uncompressed, up '
                  'to 12 pieces 16 GB. So the databases have to be compressed, with random access; see the '
                  '<a href="compression.html">next section</a>.</p>'
                  '<h2>The build</h2><p>Built unit by unit (n pieces, of which m men) with '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/international/units_6x6.py">'
                  '<code>units_6x6.py</code></a>: build, check every position against its successors, compress (zstd -19, '
                  'blocks of 4 KiB), compare every block with the uncompressed values and compute the checksum. All 88 units '
                  'up to 12 pieces together took 26 hours and 37 minutes on 10 cores (Xeon E5-1650 v4, in a virtual '
                  'machine). That fits on one machine.</p>')]
        self.write('towards-12.html', '\n'.join(body), self.page())

    def compression(self):
        """the measurement of compression_stats on the 10-piece layer (international/compression_10.txt)"""
        t = self.t
        f = HERE / 'international' / 'compression_10.txt'
        body = [t('<p>Gemeten met <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/'
                  'international/compression_stats.cpp"><code>compression_stats</code></a> op de laag met 10 stukken.</p>',
                  '<p>Measured with <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/'
                  'international/compression_stats.cpp"><code>compression_stats</code></a> on the 10-piece layer.</p>')]
        if f.exists():
            body.append(COMPRESSION_TEXT(self, f.read_text()))
        fz = HERE / 'international' / 'compression_zstd_10.txt'
        if fz.exists():
            body.append(ZSTD_TEXT(self, fz.read_text()))
        fl = HERE / 'international' / 'compression_lookup_10.txt'
        if fl.exists():
            body.append(LOOKUP_TEXT(self, fl.read_text()))
        fm = HERE / 'international' / 'compression_mmap_10.txt'
        if fm.exists():
            body.append(MMAP_TEXT(self, fm.read_text()))
        fb = HERE / 'international' / 'build_lookups_10.txt'
        if fb.exists():
            body.append(BUILD_TEXT(self, fb.read_text()))
        fd = HERE / 'international' / 'compression_dtm_10.txt'
        if fd.exists():
            body.append(DTM_TEXT(self, fd.read_text()))
        else:
            body.append(t('<p>De meting loopt nog.</p>', '<p>The measurement is still running.</p>'))
        self.write('compression.html', '\n'.join(body), self.page())


def COMPRESSION_TEXT(site, raw):
    """the results of compression_stats (on the 10-piece layer) as tables and conclusions"""
    import re as _re
    t = site.t
    g = lambda pat: [float(x) for x in _re.search(pat, raw).groups()]
    lo, wi, dr = g(r'loss / win / draw\s+([\d.]+)% / ([\d.]+)% / ([\d.]+)%')
    dc, = g(r"capture \(don't care\) ([\d.]+)%")
    ns, sv, svs, svp = g(r'slices\s+(\d+), of which single-valued (\d+) \(([\d.]+)% of the slices, ([\d.]+)% of the positions\)')
    h0, h1 = g(r'order-0 ([\d.]+) bits, order-1 \(don.t cares filled\) ([\d.]+) bits')
    xr, xd = g(r'xz -9\s+raw ([\d.]+) bits, don.t cares filled ([\d.]+) bits')
    zr, zd = g(r'zlib -9 raw ([\d.]+) bits, don.t cares filled ([\d.]+) bits')
    f = (lambda x, d=1: f'{x:.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d=1: f'{x:.{d}f}')
    rows = [(t('Ongecomprimeerd (2 bits)', 'Uncompressed (2 bits)'), '2,000' if site.lang == 'nl' else '2.000', '1'),
            (t('Entropie, orde 0', 'Entropy, order 0'), f(h0, 3), f(2 / h0)),
            (t('zlib -9, blokken van 1 MiB', 'zlib -9, blocks of 1 MiB'), f(zr, 3), f(2 / zr)),
            (t('xz -9, blokken van 1 MiB', 'xz -9, blocks of 1 MiB'), f(xr, 3), f(2 / xr)),
            (t('Entropie, orde 1, slagplicht ingevuld', 'Entropy, order 1, captures filled in'), f(h1, 3), f(2 / h1)),
            (t('zlib -9, slagplicht ingevuld', 'zlib -9, captures filled in'), f(zd, 3), f(2 / zd)),
            (t('xz -9, slagplicht ingevuld', 'xz -9, captures filled in'), f(xd, 3), f(2 / xd))]
    tbl = ''.join(f'<tr><td>{a}</td><td>{b}</td><td>{c}×</td></tr>' for a, b, c in rows)
    gb = lambda n: f(n * xd / 8 / 1e9, 1)
    return (t(f'<h2>De verdeling</h2><p>Van alle stellingen met 10 stukken is {f(wi)}% gewonnen, {f(lo)}% verloren en '
              f'{f(dr)}% remise voor de partij aan zet. In <strong>{f(dc)}%</strong> van de stellingen kan de partij aan '
              f'zet slaan. Die waarde volgt uit één ply slagen (de slagen gaan naar databases met minder stukken), dus '
              f'een compressor mag daar alles opslaan: hier is de waarde van de vorige stelling ingevuld, zodat de reeksen '
              f'langer worden. {f(svs)}% van de slices heeft maar één waarde, maar dat zijn kleine slices: samen '
              f'{f(svp, 2)}% van de stellingen.</p>',
              f'<h2>The distribution</h2><p>Of all positions with 10 pieces {f(wi)}% are won, {f(lo)}% lost and {f(dr)}% '
              f'drawn for the side to move. In <strong>{f(dc)}%</strong> of the positions the side to move can capture. '
              f'Their value follows from one ply of captures (into databases with fewer pieces), so a compressor may store '
              f'anything there: here the value of the previous position is filled in, so that the runs get longer. '
              f'{f(svs)}% of the slices have a single value, but they are small: together {f(svp, 2)}% of the positions.</p>')
            + t('<h2>Bits per stelling</h2>', '<h2>Bits per position</h2>')
            + f'<table class="tbl"><tr><th></th><th>{t("Bits per stelling", "Bits per position")}</th>'
              f'<th>{t("Factor", "Factor")}</th></tr>{tbl}</table>'
            + t('<p class="note">De entropie geldt voor alle stellingen, zlib en xz voor een steekproef van 1 op de 16 '
                'slices.</p>', '<p class="note">The entropy is over all positions, zlib and xz over a sample of 1 in 16 '
                'slices.</p>')
            + t(f'<h2>Wat dat betekent voor 12 stukken</h2><p>Met de slagstellingen ingevuld haalt xz {f(xd, 3)} bits per '
                f'stelling, een factor {f(2 / xd, 0)} kleiner dan 2 bits. Bij die verhouding zijn de databases met 11 '
                f'stukken zo\'n {gb(22_071_836_160)} GB en die met 12 stukken {gb(23_385_567_744)} GB; tot en met 12 stukken '
                f'samen {gb(63_836_439_921)} GB. Dat past ruim in het geheugen.</p><p>Twee kanttekeningen. Voor snel '
                f'opzoeken moeten de blokken kleiner zijn dan 1 MiB (een paar KB), en dan comprimeert het iets minder; '
                f'en elke stelling met slagplicht kost bij het opzoeken één ply slagen. Daar staat tegenover dat de '
                f'slagstellingen slimmer ingevuld kunnen worden dan met de vorige waarde, bijvoorbeeld met de waarde die '
                f'een reeks aan beide kanten verlengt.</p>',
                f'<h2>What this means for 12 pieces</h2><p>With the capture positions filled in, xz reaches {f(xd, 3)} bits per '
                f'position, a factor {f(2 / xd, 0)} below 2 bits. At that rate the databases with 11 pieces take some '
                f'{gb(22_071_836_160)} GB and those with 12 pieces {gb(23_385_567_744)} GB; up to 12 pieces together '
                f'{gb(63_836_439_921)} GB. That fits in memory with room to spare.</p><p>Two caveats. For fast lookup the '
                f'blocks have to be smaller than 1 MiB (a few KB), which compresses somewhat less; and every position with a '
                f'capture costs one ply of captures when it is looked up. On the other hand the capture positions can be '
                f'filled in more cleverly than with the previous value, for example with the value that extends a run on '
                f'both sides.</p>'))


def ZSTD_TEXT(site, raw):
    """the results of compression_zstd (by block size) as a table and conclusions"""
    t = site.t
    names = {'zstd -3, as is': ('zstd -3, zoals het is', 'zstd -3, as is'),
             'zstd -3, captures filled': ('zstd -3, slagplicht ingevuld', 'zstd -3, captures filled in'),
             'zstd -9, captures filled': ('zstd -9, slagplicht ingevuld', 'zstd -9, captures filled in'),
             'zstd -19, captures filled': ('zstd -19, slagplicht ingevuld', 'zstd -19, captures filled in'),
             'zstd -9 + 100 KB dictionary': ('zstd -9 + woordenboek van 100 KB', 'zstd -9 + 100 KB dictionary'),
             'zstd -19 + 100 KB dictionary': ('zstd -19 + woordenboek van 100 KB', 'zstd -19 + 100 KB dictionary'),
             'xz -9, captures filled': ('xz -9, slagplicht ingevuld', 'xz -9, captures filled in')}
    rows, vals = [], {}
    for line in raw.splitlines():
        for k, (nl, en) in names.items():
            if line.startswith(k + ' ') and len(line.split()) >= 4:
                xs = [float(x) for x in line[len(k):].split()]
                vals[k] = xs
                f = (lambda x: f'{x:.3f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x: f'{x:.3f}')
                rows.append(f'<tr><td>{nl if site.lang == "nl" else en}</td>' + ''.join(f'<td>{f(x)}</td>' for x in xs) + '</tr>')
    z19, z3, xz = vals['zstd -19, captures filled'], vals['zstd -3, captures filled'], vals['xz -9, captures filled']
    f = (lambda x, d=1: f'{x:.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d=1: f'{x:.{d}f}')
    gb = lambda bits: f(63_836_439_921 * bits / 8 / 1e9)
    return (t('<h2>zstd en de blokgrootte</h2><p>Voor snel opzoeken moet een blok klein zijn: bij elke opvraging wordt '
              'één blok uitgepakt. Hieronder de bits per stelling bij onafhankelijke blokken van 4 KiB tot 1 MiB (een '
              'blok van 4 KiB bevat 16.384 stellingen), op dezelfde steekproef van 1 op de 16 slices.</p>',
              '<h2>zstd and the block size</h2><p>For fast lookup a block has to be small: every lookup unpacks one block. '
              'Below the bits per position for independent blocks of 4 KiB to 1 MiB (a block of 4 KiB holds 16,384 '
              'positions), on the same sample of 1 in 16 slices.</p>')
            + f'<table class="tbl"><tr><th></th><th>4 KiB</th><th>16 KiB</th><th>64 KiB</th><th>1 MiB</th></tr>{"".join(rows)}</table>'
            + t(f'<p>Kleine blokken kosten weinig: zstd -19 haalt bij 4 KiB {f(z19[0], 3)} bits per stelling, nauwelijks '
                f'meer dan de {f(z19[3], 3)} bij 1 MiB. Bij kleine blokken is zstd beter dan xz ({f(xz[0], 3)}), dat pas bij '
                f'grote blokken wint. Een woordenboek helpt alleen bij de kleinste blokken een beetje. Hoe snel opzoeken '
                f'dan is, staat in de volgende sectie. Met zstd -19 en '
                f'blokken van 4 KiB worden alle databases tot en met 12 stukken ongeveer {gb(z19[0])} GB; met het snellere '
                f'zstd -3 {gb(z3[0])} GB. Beide passen ruim in het geheugen.</p>',
                f'<p>Small blocks cost little: zstd -19 reaches {f(z19[0], 3)} bits per position at 4 KiB, hardly more than '
                f'the {f(z19[3], 3)} at 1 MiB. With small blocks zstd beats xz ({f(xz[0], 3)}), which only wins with large '
                f'blocks. A dictionary helps a little, and only for the smallest blocks. How fast lookup then is, '
                f'is shown in the next section. With zstd -19 and blocks of 4 KiB all databases '
                f'up to 12 pieces take about {gb(z19[0])} GB; with the faster zstd -3 {gb(z3[0])} GB. Both fit in memory '
                f'with room to spare.</p>'))


def LOOKUP_TEXT(site, raw):
    """the results of compression_lookup (random-access lookup speed) as a table and conclusions"""
    import re as _re
    t = site.t
    rows = []
    for m in _re.finditer(r'^(\d+) KiB\s+(\d+)\s+([\d.]+)\s+([\d.]+) us\s+([\d.]+) us\s+([\d.]+) us', raw, _re.M):
        kb, lvl, bits, r1, rn, rc = m.groups()
        f = (lambda x, d: f'{float(x):.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d: f'{float(x):.{d}f}')
        rows.append(f'<tr><td>{kb} KiB</td><td>-{lvl}</td><td>{f(bits, 3)}</td><td>{f(r1, 1)} µs</td><td>{f(rn, 1)} µs</td>'
                    f'<td>{f(rc, 2)} µs</td></tr>')
    head = t('<tr><th>Blok</th><th>zstd</th><th>Bits per stelling</th><th>Willekeurig, 1 kern</th><th>Willekeurig, 4 kernen</th>'
             '<th>In groepjes, met cache</th></tr>',
             '<tr><th>Block</th><th>zstd</th><th>Bits per position</th><th>Random, 1 core</th><th>Random, 4 cores</th>'
             '<th>In runs, with cache</th></tr>')
    return (t('<h2>Hoe snel is opzoeken?</h2><p>Gemeten met <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/'
              'strongly_solving_6x6/international/compression_lookup.cpp"><code>compression_lookup</code></a>: de '
              'gecomprimeerde blokken en een blokindex staan in het geheugen; een opvraging zoekt het blok op, pakt het uit en '
              'haalt de waarde eruit (elke uitkomst gecontroleerd tegen het origineel), 2 miljoen opvragingen per meting. '
              'Willekeurig: elke opvraging een willekeurige stelling, dus elke keer een blok uitpakken. In groepjes: 64 '
              'stellingen dicht bij elkaar (binnen ±2.000 posities), met het laatst uitgepakte blok als cache, zoals bij het '
              'bouwen van een slice, waar de opvolgers van naburige stellingen dicht bij elkaar liggen. Bij 4 kernen staat de '
              'tijd per opvraging per kern.</p>',
              '<h2>How fast is lookup?</h2><p>Measured with <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/'
              'strongly_solving_6x6/international/compression_lookup.cpp"><code>compression_lookup</code></a>: the compressed '
              'blocks and a block index are in memory; a lookup finds the block, decompresses it and extracts the value (every '
              'result checked against the original), 2 million lookups per measurement. Random: every lookup a random '
              'position, so every time a block is decompressed. In runs: 64 positions close together (within ±2,000 '
              'positions), with the last decompressed block as a cache, as when a slice is built, where the successors of '
              'nearby positions lie close together. With 4 cores the time is per lookup and core.</p>')
            + f'<table class="tbl">{head}{"".join(rows)}</table>'
            + t('<p>Zonder cache bepaalt het uitpakken alles: de tijd groeit met de blokgrootte, van zo\'n 3 µs bij 1 KiB tot '
                '45–50 µs bij 64 KiB (ongeveer 0,7 GB per seconde), en vier kernen schalen bijna perfect. zstd -19 pakt even '
                'snel uit als -3 en is kleiner; alleen het inpakken is trager, en dat gebeurt één keer. Met cache kost een '
                'opvraging zo\'n 0,5 µs, ongeveer 2 miljoen per seconde per kern. Een goede keuze lijkt blokken van 4 KiB met '
                'zstd -19: 0,210 bits per stelling (ongeveer 1,7 GB tot en met 12 stukken), zo\'n 6 µs voor een willekeurige '
                'stelling en 0,5 µs met cache. Nog niet meegeteld: een stelling met slagplicht kost bij het opzoeken één ply '
                'slagen, dus een opvraging per slag in een kleinere database.</p>',
                '<p>Without a cache decompression decides everything: the time grows with the block size, from some 3 µs at '
                '1 KiB to 45–50 µs at 64 KiB (about 0.7 GB per second), and four cores scale almost perfectly. zstd -19 '
                'decompresses as fast as -3 and is smaller; only compression is slower, and that happens once. With a cache '
                'a lookup takes some 0.5 µs, about 2 million per second per core. A good choice looks like blocks of 4 KiB with '
                'zstd -19: 0.210 bits per position (about 1.7 GB up to 12 pieces), some 6 µs for a random position and 0.5 µs '
                'with a cache. Not yet counted: a position with a capture costs one ply of captures when it is looked up, so '
                'one lookup per capture in a smaller database.</p>'))


def MMAP_TEXT(site, raw):
    """the results of compression_mmap (lookups through mmap, warm and cold) as a table and conclusions"""
    import re as _re
    t = site.t
    names = {'uncompressed, 2 bits': ('ongecomprimeerd, 2 bits', 'uncompressed, 2 bits'),
             'zstd -19, 4 KiB blocks': ('zstd -19, blokken van 4 KiB', 'zstd -19, blocks of 4 KiB'),
             'zstd -19, 16 KiB blocks': ('zstd -19, blokken van 16 KiB', 'zstd -19, blocks of 16 KiB')}
    f = (lambda x, d: f'{float(x):.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d: f'{float(x):.{d}f}')
    rows = []
    for k, (nl, en) in names.items():
        m = _re.search(_re.escape(k) + r'\s+([\d.]+)\s+([\d.]+) us\s+([\d.]+) us\s+([\d.]+) us\s+([\d.]+) us', raw)
        mb, w1, wn, c1, cn = m.groups()
        rows.append(f'<tr><td>{nl if site.lang == "nl" else en}</td><td>{f(mb, 0)} MB</td><td>{f(w1, 3)} µs</td>'
                    f'<td>{f(wn, 3)} µs</td><td>{f(c1, 2)} µs</td><td>{f(cn, 2)} µs</td></tr>')
    head = t('<tr><th></th><th>Bestand</th><th>Warm, 1 kern</th><th>Warm, 4 kernen</th><th>Koud, 1 kern</th><th>Koud, 4 kernen</th></tr>',
             '<tr><th></th><th>File</th><th>Warm, 1 core</th><th>Warm, 4 cores</th><th>Cold, 1 core</th><th>Cold, 4 cores</th></tr>')
    return (t('<h2>Opzoeken via mmap</h2><p>Gemeten met <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/'
              'strongly_solving_6x6/international/compression_mmap.cpp"><code>compression_mmap</code></a>: de bestanden worden '
              'met mmap geopend. Warm: alle pagina\'s zitten al in het geheugen van het besturingssysteem. Koud: het bestand '
              'is eerst uit dat geheugen gezet, zodat de eerste aanraking van een pagina van schijf komt (gemeten over de '
              'eerste 100.000 opvragingen; dit is een virtuele schijf in de cloud, op een eigen SSD kan het anders '
              'uitvallen). Willekeurige stellingen, elke waarde gecontroleerd.</p>',
              '<h2>Lookup through mmap</h2><p>Measured with <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/'
              'strongly_solving_6x6/international/compression_mmap.cpp"><code>compression_mmap</code></a>: the files are opened '
              'with mmap. Warm: every page is already in the operating system\'s page cache. Cold: the file is first evicted '
              'from that cache, so the first touch of a page reads it from disk (measured over the first 100,000 lookups; '
              'this is a virtual disk in the cloud, on a local SSD it may turn out differently). Random positions, every '
              'value checked.</p>')
            + f'<table class="tbl">{head}{"".join(rows)}</table>'
            + t('<p>Via mmap is opzoeken in de gecomprimeerde blokken even snel als in gewoon geheugen: het uitpakken '
                'bepaalt de tijd, en het bestand is klein genoeg om snel helemaal in het geheugen te staan. Een koude pagina '
                'kost hier een paar microseconden extra. Het ongecomprimeerde bestand is warm zo\'n 200 keer sneller (een '
                'paar tientallen nanoseconden, gewoon een geheugentoegang), maar ook 10 keer groter: tot en met 12 stukken '
                '16 GB tegen 1,7 GB. Een mogelijke tussenweg: de databases waarin het meest wordt opgezocht '
                'ongecomprimeerd, de rest gecomprimeerd.</p>',
                '<p>Through mmap, lookup in the compressed blocks is as fast as in ordinary memory: decompression decides the '
                'time, and the file is small enough to be fully cached soon. A cold page costs a few microseconds extra here. '
                'The uncompressed file is some 200 times faster when warm (a few tens of nanoseconds, just a memory access), '
                'but also 10 times larger: up to 12 pieces 16 GB against 1.7 GB. A possible middle way: the databases that '
                'are looked up most often uncompressed, the rest compressed.</p>'))


def BUILD_TEXT(site, raw):
    """the results of build_lookups (lookups while building a slice, with a block cache) as a table and conclusions"""
    import re as _re
    t = site.t
    f = (lambda x, d: f'{float(x):.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d: f'{float(x):.{d}f}')
    sl, npos = _re.search(r'slices (\d+), positions (\d+)', raw).groups()
    p1, p2, tot = _re.search(r'phase 1 ([\d.]+), phase 2 ([\d.]+), total ([\d.]+)', raw).groups()
    dist, = _re.search(r'distinct 4 KiB blocks per slice: ([\d.]+)', raw).groups()
    rows = ''.join(f'<tr><td>{c}</td><td>{f(mib, 1)} MiB</td><td>{f(100 * float(h), 2)}%</td><td>{f(us, 2)} µs</td></tr>'
                   for c, mib, h, us in _re.findall(r'(\d+) blocks \(\s*([\d.]+) MiB decoded\): hit ([\d.]+), ([\d.]+) us', raw))
    num = (lambda x: f'{int(x):,}'.replace(',', '.')) if site.lang == 'nl' else (lambda x: f'{int(x):,}')
    head = t('<tr><th>Cache (blokken van 4 KiB)</th><th>Uitgepakt</th><th>Treffers</th><th>Tijd per stelling</th></tr>',
             '<tr><th>Cache (blocks of 4 KiB)</th><th>Decompressed</th><th>Hits</th><th>Time per position</th></tr>')
    return (t(f'<h2>Opzoeken tijdens het bouwen</h2><p>Gemeten met <a href="https://github.com/rhalbersma/egdb-draughts/'
              f'blob/main/strongly_solving_6x6/international/build_lookups.cpp"><code>build_lookups</code></a>: '
              f'{num(sl)} slices met 10 stukken (1 op de 200, {num(npos)} stellingen) opnieuw opgelost met de oplosroutine '
              f'van de engine, waarbij elke opvraging buiten de slice (slagen, schijfzetten, promoties) is geteld en aan zijn '
              f'blok van 4 KiB is gekoppeld; de uitkomsten zijn gecontroleerd tegen de opgeslagen waarden. Per stelling zijn '
              f'er {f(tot, 2)} opvragingen: {f(p1, 2)} in de eerste ronde en {f(p2, 2)} bij het controleren van de '
              f'verlieskandidaten. Ze zijn zeer geconcentreerd: een slice raakt gemiddeld maar {f(dist, 1)} verschillende '
              f'blokken. Met een cache die bij elke slice leeg begint (een pessimistisch geval) en 6 µs per misser:</p>',
              f'<h2>Lookups while building</h2><p>Measured with <a href="https://github.com/rhalbersma/egdb-draughts/'
              f'blob/main/strongly_solving_6x6/international/build_lookups.cpp"><code>build_lookups</code></a>: '
              f'{num(sl)} slices with 10 pieces (1 in 200, {num(npos)} positions) solved again with the engine\'s solver, '
              f'counting every lookup outside the slice (captures, man moves, promotions) and mapping it to its 4 KiB block; '
              f'the results are checked against the stored values. Per position there are {f(tot, 2)} lookups: {f(p1, 2)} in '
              f'the first pass and {f(p2, 2)} when checking the loss candidates. They are highly concentrated: a slice touches '
              f'only {f(dist, 1)} different blocks on average. With a cache that starts empty at every slice (a pessimistic '
              f'case) and 6 µs per miss:</p>')
            + f'<table class="tbl">{head}{rows}</table>'
            + CAPTURE_TEXT(site, raw)
            + t('<p>Met een cache van 64 blokken (256 KB per kern) wordt 99,4% van de opvragingen uit de cache bediend, en '
                'kost het opzoeken zo\'n 0,17 µs per stelling. Voor de 45,5 miljard stellingen met 11 of 12 stukken is dat '
                'ruwweg 2 uur op één kern: gecomprimeerd opzoeken is dus geen bottleneck. Twee kanttekeningen: de slices met '
                '10 stukken zijn klein (gemiddeld 1.350 stellingen), en een opgezochte stelling met slagplicht kost bij '
                'gecomprimeerde opslag nog één ply slagen, wat het aantal opvragingen verhoogt.</p>',
                '<p>With a cache of 64 blocks (256 KB per core) 99.4% of the lookups are served from the cache, and lookup '
                'costs some 0.17 µs per position. For the 45.5 billion positions with 11 or 12 pieces that is roughly 2 hours '
                'on one core: compressed lookup is no bottleneck. Two caveats: the 10-piece slices are small (1,350 positions '
                'on average), and with compressed storage a looked-up position with a capture still costs one ply of '
                'captures, which raises the number of lookups.</p>'))


def CAPTURE_TEXT(site, raw):
    """capture resolution: lookups that hit a position with a capture, stored as don't care"""
    import re as _re
    t = site.t
    m = _re.search(r"([\d.]+)% of the lookups hit a capture position;\s*\nresolving captures until quiet positions takes "
                   r"([\d.]+) stored lookups per lookup, ([\d.]+) per position", raw)
    if not m:
        return ''
    pct, per_l, per_p = m.groups()
    rows = _re.findall(r'^\s+(\d+) blocks: hit ([\d.]+), ([\d.]+) us per position', raw, _re.M)
    f = (lambda x, d: f'{float(x):.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d: f'{float(x):.{d}f}')
    tbl = ''.join(f'<tr><td>{c}</td><td>{f(100 * float(h), 1)}%</td><td>{f(us, 2)} µs</td></tr>' for c, h, us in rows)
    head = t('<tr><th>Cache (blokken)</th><th>Treffers</th><th>Tijd per stelling</th></tr>',
             '<tr><th>Cache (blocks)</th><th>Hits</th><th>Time per position</th></tr>')
    return (t(f'<p><strong>Met slagplicht als &quot;maakt niet uit&quot;.</strong> {f(pct, 1)}% van de opvragingen valt op een '
              f'stelling waarin de partij aan zet kan slaan. In gecomprimeerde opslag staat daar geen waarde, dus worden de '
              f'slagen uitgespeeld tot stellingen zonder slag (alle slagen, zonder afbreken: een bovengrens). Dat kost '
              f'{f(per_l, 2)} opgeslagen opvragingen per opvraging, {f(per_p, 2)} per stelling:</p>',
              f'<p><strong>With captures as &quot;don\'t care&quot;.</strong> {f(pct, 1)}% of the lookups hit a position where '
              f'the side to move can capture. Compressed storage holds no value there, so the captures are played out to '
              f'positions without a capture (all captures, no cut-off: an upper bound). That takes {f(per_l, 2)} stored lookups '
              f'per lookup, {f(per_p, 2)} per position:</p>')
            + f'<table class="tbl">{head}{tbl}</table>'
            + t('<p>Met een cache van 256 blokken (1 MB per kern) is dat ruwweg 1 µs per stelling: voor de 45,5 miljard '
                'stellingen met 11 of 12 stukken een uur of 3 op 4 kernen, de prijs voor een opslag die 2,7 keer kleiner is.</p>',
                '<p>With a cache of 256 blocks (1 MB per core) that is roughly 1 µs per position: for the 45.5 billion '
                'positions with 11 or 12 pieces some 3 hours on 4 cores, the price for storage that is 2.7 times smaller.</p>'))


def DTM_TEXT(site, raw):
    """the results of compression_dtm (depth to mate, one byte per position) as a table and conclusions"""
    import re as _re
    t = site.t
    f = (lambda x, d: f'{float(x):.{d}f}'.replace('.', ',')) if site.lang == 'nl' else (lambda x, d: f'{float(x):.{d}f}')
    names = {'zstd -3, as is': ('zstd -3, zoals het is', 'zstd -3, as is'),
             'zstd -3, captures filled': ('zstd -3, slagplicht ingevuld', 'zstd -3, captures filled in'),
             'zstd -19, captures filled': ('zstd -19, slagplicht ingevuld', 'zstd -19, captures filled in')}
    rows, z19 = [], None
    for k, (nl, en) in names.items():
        m = _re.search(_re.escape(k) + r'\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)', raw)
        if m:
            rows.append(f'<tr><td>{nl if site.lang == "nl" else en}</td>' + ''.join(f'<td>{f(x, 3)}</td>' for x in m.groups()) + '</tr>')
            if k.startswith('zstd -19'):
                z19 = float(m.group(1))
    gb = f(63_836_439_921 * z19 / 8 / 1e9, 1)
    return (t('<h2>De afstand tot het einde (DTM)</h2><p>Met de afstand tot het einde per stelling (1 byte, 8 bits '
              'ongecomprimeerd) in plaats van alleen winst, remise of verlies, gemeten met '
              '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/international/'
              'compression_dtm.cpp"><code>compression_dtm</code></a> op dezelfde steekproef, in bits per stelling:</p>',
              '<h2>The distance to the end (DTM)</h2><p>With the distance to the end per position (1 byte, 8 bits '
              'uncompressed) instead of only win, draw or loss, measured with '
              '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/strongly_solving_6x6/international/'
              'compression_dtm.cpp"><code>compression_dtm</code></a> on the same sample, in bits per position:</p>')
            + f'<table class="tbl"><tr><th></th><th>4 KiB</th><th>64 KiB</th><th>1 MiB</th></tr>{"".join(rows)}</table>'
            + t(f'<p>Het invullen van de slagstellingen maakt DTM bijna drie keer kleiner. De keuze: <strong>zstd -19 met '
                f'blokken van 4 KiB</strong>, {f(z19, 2)} bits per stelling. Daarmee wordt DTM voor alle databases tot en met '
                f'12 stukken ongeveer {gb} GB.</p>',
                f'<p>Filling in the capture positions makes DTM almost three times smaller. The choice: <strong>zstd -19 with '
                f'blocks of 4 KiB</strong>, {f(z19, 2)} bits per position. With it DTM for all databases up to 12 pieces takes '
                f'about {gb} GB.</p>'))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(ROOT / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(ROOT / 'data' / '6x6_10.dtm'))
    ap.add_argument('--store', help='the unit store of units_6x6.py (e.g. data/units), instead of --db/--attach')
    ap.add_argument('--out', default=str(ROOT / '_site' / '6x6_is_strongly_solved'))
    args = ap.parse_args()
    eng = Engine(args.engine, 10, args.db, args.attach, store=args.store)
    for lang in ('en', 'nl'):
        out = Path(args.out) / ('' if lang == 'en' else 'nl')
        out.mkdir(parents=True, exist_ok=True)
        for f in out.glob('*.html'):
            f.unlink()
        site = Site(eng, out, lang)
        site.index()
        site.international()
        site.history()
        site.status()
        site.towards12()
        site.compression()
        print(f'{lang}: done', file=sys.stderr)


if __name__ == '__main__':
    main()
