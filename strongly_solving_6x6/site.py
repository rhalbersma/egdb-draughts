#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The companion site Strongly Solving 6x6 Draughts (_site/strongly_solving_6x6): for now international draughts on
the 6x6 board, with the solved start position and the complete opening tree to 4 plies (international/). English by
default, Dutch in nl/.

usage: site.py path/to/egdb_6x6 [--db data/6x6_le9.db] [--attach data/6x6_10.dtm] [--out _site/strongly_solving_6x6]
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

TITLE = 'Strongly Solving 6×6'

# international draughts, in chapters with one page per section: (Dutch, English, [(file, Dutch, English)])
CHAPTERS = [
    ('De opgeloste beginstand', 'The solved start position', [
        ('international.html', 'De openingsboom tot 4 ply', 'The opening tree to 4 plies')]),
    ('Eerdere oplossingen', 'Earlier solutions', [
        ('history.html', 'Perft en een zoektocht (2009, 2011)', 'Perft and a search (2009, 2011)'),
        ('van-horssen.html', 'Van Horssen (2016–2018)', 'Van Horssen (2016–2018)')]),
    ('Hoe ver is 6×6 opgelost?', 'How far is 6×6 solved?', [
        ('status.html', 'Zwak opgelost, sterk tot en met 10 stukken', 'Weakly solved, strongly up to 10 pieces'),
        ('missing.html', 'Wat er nog ontbreekt', 'What is still missing')]),
    ('Op weg naar 12 stukken', 'Towards 12 pieces', [
        ('towards-12.html', 'De omvang tot 12 stukken', 'The size up to 12 pieces'),
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
            '<p>Het doel: dammen op het 6×6-bord <strong>sterk oplossen</strong>, dus de waarde van elke legale stelling '
            'kennen. De weg ernaartoe: exacte eindspeldatabases, met de afstand tot het einde van de partij, en daarboven '
            'de volledige openingsboom. Voorlopig met één variant.</p>',
            '<p>The goal: to <strong>strongly solve</strong> draughts on the 6×6 board, that is, to know the value of every '
            'legal position. The way there: exact endgame databases, with the distance to the end of the game, and above '
            'them the complete opening tree. For now with one variant.</p>'),
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
        """how far 6x6 is solved (numbers from international/opening_strength_6x6.py)"""
        t = self.t
        rows = [(t('Tot en met 10 stukken', 'Up to 10 pieces'), t('18.379.036.017', '18,379,036,017'),
                 t('sterk opgelost, met exacte afstand tot het einde (DTM)', 'strongly solved, with the exact distance to '
                   'the end (DTM)')),
                (t('11–12 stukken, hooguit één kant dammen: de volledige openings-DAG',
                   '11–12 pieces, kings on at most one side: the complete opening DAG'), t('193.708', '193,708'),
                 t('188.103 (97%) bewezen; open: 2.008 ≥D, 578 ≤D, 3.019 ?', '188,103 (97%) proven; open: 2,008 ≥D, '
                   '578 ≤D, 3,019 ?')),
                (t('Alle overige stellingen met 11 of 12 stukken', 'All other positions with 11 or 12 pieces'),
                 t('ca. 45,5 miljard, waarvan 44,5 miljard met dammen aan beide kanten',
                   'about 45.5 billion, of which 44.5 billion with kings on both sides'), t('onbekend', 'unknown'))]
        tbl = ''.join(f'<tr><td>{a}</td><td>{b}</td><td>{c}</td></tr>' for a, b, c in rows)
        return (t(
            '<p>In de termen van Allis (1994) is 6×6 hiermee, net als bij Van Horssen, <strong>zwak opgelost</strong>: de '
            'waarde van de beginstand is bewezen, met een strategie om die te halen. <strong>Sterk opgelost</strong> zou '
            'betekenen dat de waarde van elke legale stelling bekend is. Zover is het niet, maar inhoudelijk ligt er veel '
            'meer vast dan bij een zwakke oplossing gebruikelijk is:</p>',
            '<p>In the terms of Allis (1994) 6×6 is thereby, as with Van Horssen, <strong>weakly solved</strong>: the '
            'value of the start position is proven, with a strategy to achieve it. <strong>Strongly solved</strong> would '
            'mean that the value of every legal position is known. It is not that far, but in substance much more is '
            'settled than is usual for a weak solution:</p>')
            + f'<table class="tbl"><tr><th></th><th>{t("Stellingen", "Positions")}</th><th>Status</th></tr>{tbl}</table>'
            + t('<p class="note">Legale stellingen met hooguit 6 stukken per kant en wit aan zet, zoals Van Horssen telt.</p>',
                '<p class="note">Legal positions with at most 6 pieces per side and white to move, as Van Horssen counts.</p>')
            + t('<h2>Wat er voor sterk oplossen nog ontbreekt</h2>', '<h2>What is still missing for a strong solution</h2>')
            + t('<p>De open stellingen in de DAG hangen allemaal af van de 5.101 bladeren met 11 of 12 stukken en dammen '
                'aan beide kanten waarvan de waarde ook na één ply niet vastligt. Achter die bladeren ligt geen handvol '
                'stellingen: een verkenning vanaf die bladeren vond meer dan 5 miljoen verschillende bereikbare stellingen '
                'met dammen aan beide kanten voordat hij werd gestopt, vooral eindspelen met alleen dammen, zoals 6 dammen '
                'tegen 5 (alleen al 14.702.688 legale stellingen). Sterk oplossen vraagt dus de databases voor 11 en 12 '
                'stukken met dammen aan beide kanten: bijna tweeënhalf keer zoveel stellingen als alle stellingen tot en '
                'met 10 stukken samen.</p><p>Kort gezegd: formeel zwak opgelost, inhoudelijk sterk tot en met 10 stukken, met een '
                'openingsboom die voor 97% is bewezen en een exacte afstand tot het einde voor de beginstand en alle '
                'stellingen tot en met ply 4.</p>',
                '<p>The open positions in the DAG all depend on the 5,101 leaves with 11 or 12 pieces and kings on both '
                'sides whose value is not settled after one ply either. Behind those leaves lies not a handful of '
                'positions: a probe from those leaves found more than 5 million different reachable positions with kings '
                'on both sides before it was stopped, mostly endgames with kings only, such as 6 kings against 5 (14,702,688 '
                'legal positions for that material alone). Solving strongly therefore needs the databases for 11 and 12 '
                'pieces with kings on both sides: almost two and a half times as many positions as all positions up to '
                '10 pieces together.</p><p>In short: formally weakly solved, in substance strongly up to 10 pieces, with an '
                'opening tree that is 97% proven and an exact distance to the end for the start position and all positions '
                'up to ply 4.</p>'))

    def international(self):
        pg = self.page()
        fig, i0 = pg.diagram(START, self.t('de beginstand, wit aan zet', 'the start position, white to move'))
        v0, legal0 = self.o.analyse(START)
        body = [fig, self.t(
                '<p>De beginstand is <strong>remise</strong>. Alle vijf eerste zetten van wit houden remise. Hieronder de '
                'volledige openingsboom tot 4 ply: elke zet met zijn waarde, sterkste eerst; klik op een zet om de '
                'stelling te zien. De waarden komen uit de opgeloste openingsboom (stellingen met meer dan 10 stukken) en '
                'de databases, met de exacte afstand tot het einde.</p>',
                '<p>The start position is a <strong>draw</strong>. All five first moves of white keep the draw. Below, the '
                'complete opening tree to 4 plies: every move with its value, strongest first; click a move to see the '
                'position. The values come from the solved opening tree (positions with more than 10 pieces) and the '
                'databases, with the exact distance to the end.</p>'),
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
            'transpositietabel, om graph-history-problemen te vermijden); hier zijn de afstanden tot het einde exact voor '
            'de hele boom tot en met ply 4.</p>',
            '<p>So we reproduce his databases up to 8 pieces. He proves the value of the start position (without '
            'a transposition table, to avoid graph-history problems); here the distances to the end are exact for the '
            'whole tree up to ply 4.</p>'))
        self.write('van-horssen.html', '\n'.join(body), pg)

    def status(self):
        text = self.strength()
        cut = text.index('<h2>' + self.t('Wat er voor sterk oplossen nog ontbreekt', 'What is still missing for a strong solution') + '</h2>')
        self.write('status.html', text[:cut], self.page())
        self.write('missing.html', text[cut:].split('</h2>', 1)[1], self.page())

    def towards12(self):
        t = self.t
        num = lambda x: f'{x:,}'.replace(',', '.') if self.lang == 'nl' else f'{x:,}'
        rows = [('≤ 10', 18_379_036_017, 18_379_036_017), ('11', 22_071_836_160, 40_450_872_177),
                ('12', 23_385_567_744, 63_836_439_921)]
        tbl = ''.join(f'<tr><td>{a}</td><td>{num(b)}</td><td>{num(c)}</td></tr>' for a, b, c in rows)
        body = [t('<p>Sterk oplossen vraagt de databases tot en met 12 stukken. Het aantal legale stellingen, met hooguit '
                  '6 stukken per kant en wit aan zet (tot en met 8 stukken precies de aantallen van Van Horssen):</p>',
                  '<p>A strong solution needs the databases up to 12 pieces. The number of legal positions, with at most 6 '
                  'pieces per side and white to move (up to 8 pieces exactly Van Horssen\'s numbers):</p>'),
                f'<table class="tbl"><tr><th>{t("Stukken", "Pieces")}</th><th>{t("Stellingen (hooguit 6 per kant)", "Positions (at most 6 per side)")}</th>'
                f'<th>{t("Cumulatief", "Cumulative")}</th></tr>{tbl}</table>',
                t('<h2>Geheugen</h2><p>Gebouwd wordt per slice: de schijven liggen vast, alleen de dammen variëren. De grootste '
                  'slice is 6 dammen tegen 6, met 17.153.136 stellingen: bij 2 bits per stelling ruim 4 MB. Het geheugen '
                  'voor het bouwen zelf is dus geen probleem. Het probleem is het snel opzoeken in de andere databases, bij '
                  'slagen, schijfzetten en promoties: tot en met 11 stukken is dat ongecomprimeerd zo\'n 10 GB, tot en met '
                  '12 stukken 16 GB. Daarom moeten de databases gecomprimeerd worden, met willekeurige toegang; zie de '
                  '<a href="compression.html">volgende sectie</a>.</p>'
                  '<h2>Rekentijd</h2><p>De databases tot en met 10 stukken werden gebouwd met zo\'n 3,8 miljoen stellingen '
                  'per seconde. Voor de 45,5 miljard stellingen met 11 of 12 stukken is dat ruwweg 3 tot 4 uur; met '
                  'gecomprimeerd opzoeken en de slices met dammen aan beide kanten, die binnen de slice iteratief worden '
                  'opgelost, eerder 10 tot 30 uur. Dat kan op één machine.</p>',
                  '<h2>Memory</h2><p>The databases are built slice by slice: the men are fixed, only the kings vary. The '
                  'largest slice is 6 kings against 6, with 17,153,136 positions: at 2 bits per position just over 4 MB. '
                  'The memory for building itself is therefore no problem. The problem is fast lookup in the other '
                  'databases, for captures, man moves and promotions: up to 11 pieces that is some 10 GB uncompressed, up '
                  'to 12 pieces 16 GB. So the databases have to be compressed, with random access; see the '
                  '<a href="compression.html">next section</a>.</p>'
                  '<h2>Computing time</h2><p>The databases up to 10 pieces were built at some 3.8 million positions per '
                  'second. For the 45.5 billion positions with 11 or 12 pieces that is roughly 3 to 4 hours; with '
                  'compressed lookup and the slices with kings on both sides, which are solved iteratively within the '
                  'slice, more like 10 to 30 hours. That fits on one machine.</p>')]
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default=str(ROOT / 'data' / '6x6_le9.db'))
    ap.add_argument('--attach', default=str(ROOT / 'data' / '6x6_10.dtm'))
    ap.add_argument('--out', default=str(ROOT / '_site' / 'strongly_solving_6x6'))
    args = ap.parse_args()
    eng = Engine(args.engine, 10, args.db, args.attach)
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
