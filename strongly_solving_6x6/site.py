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
from sitekit import Page, esc, geom_6x6, lang_link, write_page   # noqa: E402

# the oracle (databases plus the solved opening tree) and the engine of the Strategie der 36 velden site
_spec = importlib.util.spec_from_file_location('strategie_site', ROOT / 'strategie_der_36_velden' / 'site.py')
S = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(S)
START, REPO, Oracle, Engine = S.START, S.REPO, S.Oracle, S.Engine

TITLE = 'Strongly Solving 6×6 Draughts'


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

    def nav(self, current):
        t = self.t
        home = '../index.html' if self.lang == 'en' else '../../nl/index.html'      # English: default, Dutch in nl/
        alt = f'nl/{current}' if self.lang == 'en' else f'../{current}'
        book = '../strategie_der_36_velden/en/' if self.lang == 'en' else '../../strategie_der_36_velden/'
        return (f'<a class="brand" href="index.html">{esc(TITLE)}</a>'
                f'<a href="international.html">{t("Internationaal", "International")}</a>'
                f'<a href="{book}">Strategie der 36 velden</a><a href="{home}">{t("Alle sites", "All sites")}</a>'
                f'{lang_link(self.lang, alt)}')

    def write(self, name, title, body, pg):
        write_page(self.out / name, title, self.nav(name), body, pg, TITLE, self.lang)

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
            '<div class="hub">' + t(
                '<a class="card" href="international.html"><strong>Internationaal dammen</strong><span>De beginstand is '
                'remise. De volledige openingsboom tot 4 ply met exacte waarden, de geschiedenis van de oplossing en hoe '
                'ver 6×6 is opgelost: sterk tot en met 10 stukken.</span></a>',
                '<a class="card" href="international.html"><strong>International draughts</strong><span>The start '
                'position is a draw. The complete opening tree to 4 plies with exact values, the history of the solution '
                'and how far 6×6 is solved: strongly up to 10 pieces.</span></a>') + '</div>',
            t(f'<p>Het eerste gebruik van deze databases was de controle van Herm Jan Brascamps <em>Strategie der 36 '
              f'velden</em>, met <a href="../../strategie_der_36_velden/">een eigen site</a>.</p>',
              f'<p>The first use of these databases was checking Herm Jan Brascamp\'s <em>Strategie der 36 velden</em>, '
              f'with <a href="../strategie_der_36_velden/en/">a site of its own</a>.</p>'),
            f'<p class="src">' + t('De programma\'s en databases', 'The programs and databases') + f': <a href="{REPO}">{REPO}</a>.</p>']
        self.write('index.html', t('Overzicht', 'Overview'), '\n'.join(body), pg)

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
        return (t('<h3>Hoe ver is 6×6 opgelost?</h3>', '<h3>How far is 6×6 solved?</h3>') + t(
            '<p>In de termen van Allis (1994) is 6×6 hiermee, net als bij Van Horssen, <strong>zwak opgelost</strong>: de '
            'waarde van de beginstand is bewezen, met een strategie om die te halen. <strong>Sterk opgelost</strong> zou '
            'betekenen dat de waarde van elke legale stelling bekend is. Zover is het niet, maar inhoudelijk ligt er veel '
            'meer vast dan bij een zwakke oplossing gebruikelijk is:</p>',
            '<p>In the terms of Allis (1994) 6×6 is thereby, as with Van Horssen, <strong>weakly solved</strong>: the '
            'value of the start position is proven, with a strategy to achieve it. <strong>Strongly solved</strong> would '
            'mean that the value of every legal position is known. It is not that far, but in substance much more is '
            'settled than is usual for a weak solution:</p>')
            + f'<table class="tbl"><tr><th></th><th>{t("Stellingen", "Positions")}</th><th>Status</th></tr>{tbl}</table>'
            + t('<p class="note">Legale stellingen met hooguit 6 stukken per kant en wit aan zet, zoals Van Horssen telt. '
                'Onze databases tot en met 10 stukken bevatten er 25.741.493.062, inclusief de overbodige stellingen met '
                'meer dan 6 stukken aan één kant.</p>',
                '<p class="note">Legal positions with at most 6 pieces per side and white to move, as Van Horssen counts. '
                'Our databases up to 10 pieces hold 25,741,493,062, including the superfluous positions with more than 6 '
                'pieces on one side.</p>')
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
        body = [self.t('<h1>6×6 internationaal dammen: de opgeloste beginstand</h1>',
                       '<h1>6×6 international draughts: the solved start position</h1>'), fig, self.t(
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
                 self.t('1.617.366.257', '1,617,366,257'), self.t('1.617.366.257: precies hetzelfde', '1,617,366,257: exactly the same'))]
        tbl = ''.join(f'<tr><td>{a}</td><td>{pg.prose(b, "6x6")}</td><td>{pg.prose(c, "6x6")}</td></tr>' for a, b, c in rows)
        body.append(self.t('<h2>Eerdere oplossingen</h2>', '<h2>Earlier solutions</h2>'))
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
            '<p>We reproduceren zijn databases dus tot en met 8 stukken. Van Horssen laat de stellingen met 7 stukken '
            'tegen 1 terecht weg, want elke kant begint met 6 stukken; onze databases rekenden ze (84.825.312 bij 8 '
            'stukken) overbodig ook uit, en zonder die stellingen zijn de aantallen precies gelijk. Hij bewijst de waarde van de beginstand (zonder '
            'transpositietabel, om graph-history-problemen te vermijden); hier zijn de afstanden tot het einde exact voor '
            'de hele boom tot en met ply 4.</p>',
            '<p>So we reproduce his databases up to 8 pieces. Van Horssen rightly leaves out the positions with 7 pieces '
            'against 1, since each side starts with 6 pieces; our databases needlessly computed them as well (84,825,312 '
            'at 8 pieces), and without them the numbers are exactly the same. He proves the value of the start position (without '
            'a transposition table, to avoid graph-history problems); here the distances to the end are exact for the '
            'whole tree up to ply 4.</p>'))
        body.append(self.strength())
        self.write('international.html', self.t('Internationaal dammen', 'International draughts'), '\n'.join(body), pg)


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
        print(f'{lang}: done', file=sys.stderr)


if __name__ == '__main__':
    main()
