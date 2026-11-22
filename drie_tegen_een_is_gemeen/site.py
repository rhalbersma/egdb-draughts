#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Companion site (GitHub Pages) for the kings-only endgames of "Drie tegen een is gemeen" (1988): 2 kings
against 1 and 3 kings against 1 in international draughts on boards of every size, the trapping positions and
10x10 with one square removed. Every position is clickable; the values come from egdb_fast --serve.

usage: site.py [--engine build/egdb_fast] [--out _site/drie_tegen_een_is_gemeen]
"""

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / 'common'))
from sitekit import FastEngine, Page, esc, lang_link, menu, write_page   # noqa: E402

TITLE = 'Drie tegen een is gemeen'

PAGES = [
    ('index.html', 'Overzicht', 'Overview'),
    ('twee-tegen-een.html', '2 dammen tegen 1', '2 kings against 1'),
    ('drie-tegen-een.html', '3 dammen tegen 1', '3 kings against 1'),
    ('vangstellingen.html', 'Vangstellingen', 'Trapping positions'),
    ('tien-zonder-veld.html', '10×10 met één veld weg', '10×10 with one square removed'),
]

# the results per board: a row per L (height), a column per B − L (width − height); '21' = won, longest win 21 plies,
# '(21)' = drawn (the longest win among the won positions), '*' = won if the strong side holds the long diagonal(s),
# '13/15' = unplayable/playable corner squares, '' = not in the table
RESULTS_21 = {
    3: ['3/3*', '7', '11/9', '11'],
    4: ['15*', '13', '13', '21'],
    5: ['(7)/(3)', '27', '(9)/15', '(9)'],
    6: ['(7)', '(9)', '(11)', '(9)'],
    7: ['(7)/(5)', '(9)', '(9)/(11)', '(9)'],
    8: ['(7)', '(9)', '(11)', '(9)'],
    9: ['(7)/(5)', '(9)', '(9)/(11)', '(9)'],
    10: ['(7)', '(9)', '(11)', '(9)'],
    11: ['(7)/(5)', '(9)', '(9)/(11)', '(9)'],
    12: ['(7)', '(9)', '(11)', '(9)'],
}
RESULTS_31 = {
    3: ['0/3', '5', '7/9', '9', '9/11', '13', '15/15', '', '', '', '', ''],
    4: ['9*', '11', '11', '13', '17', '23', '29', '37', '', '', '', ''],
    5: ['11/15*', '23', '13/13', '15', '17/15', '17', '19/19', '21', '23/23', '', '', ''],
    6: ['17*', '15', '17', '19', '19', '21', '25', '27', '29', '33', '', ''],
    7: ['19/17*', '19', '21/19', '23', '25/25', '35', '49/21', '53', '(73)/67', '(73)', '(73)/(43)', ''],
    8: ['25*†', '19', '23', '31', '(23)', '(33)', '(21)', '25', '(19)', '(19)', '(21)', '(19)'],
    9: ['(15)/(15)', '21', '(19)/21', '(27)', '(17)/(19)', '(31)', '(19)/(21)', '(21)', '(19)/23', '(19)', '(19)/(19)', '(21)'],
    10: ['(15)', '21', '25', '(27)', '(19)', '(31)', '(21)', '(21)', '(19)', '27', '(19)', '(19)'],
    11: ['(15)/(15)', '21', '(19)/21', '(27)', '(17)/(19)', '(31)', '(19)/(21)', '(21)', '(19)/(21)', '(19)', '(19)/25', '(19)'],
    12: ['(15)', '21', '25', '(27)', '(19)', '(31)', '(21)', '(21)', '(19)', '(21)', '', '27'],
}

LEGEND_EN = ('In the tables: <span class="res-win">21</span> = won, longest win 21 plies · <span class="res-diag">21*</span> '
             '= won provided the stronger side occupies the long diagonal(s) · <span class="res-draw">(21)</span> = drawn (the number is the longest win among the '
             'positions that are won) · <code>13/15</code> = unplayable/playable corner squares. A board <em>B × L</em> is '
             'width × height, with <em>B</em> ≥ <em>L</em>; the tables have a row for every <em>L</em> and a column for '
             'every <em>B</em> − <em>L</em>. Playable = the corner squares are playable, unplayable = they are not (only '
             'relevant for odd × odd).')

LEGEND = ('In de tabellen: <span class="res-win">21</span> = gewonnen, langste winst 21 ply · <span class="res-diag">21*</span> '
          '= gewonnen als de sterke partij de lange lijn(en) bezet · <span class="res-draw">(21)</span> = remise (het getal is de langste winst onder de '
          'stellingen die wel gewonnen zijn) · <code>13/15</code> = onspeelbare/speelbare hoekvelden. Een bord <em>B × L</em> '
          'is breedte × lengte, met <em>B</em> ≥ <em>L</em>; de tabellen hebben een rij per <em>L</em> en een kolom per '
          '<em>B</em> − <em>L</em>. Speelbaar = de hoekvelden zijn speelbaar, onspeelbaar = niet (alleen van belang bij '
          'oneven × oneven).')


class Site:
    def __init__(self, exe, out, lang='nl', engines=None):
        self.exe = exe
        self.out = out
        self.lang = lang
        self.engines = {} if engines is None else engines

    def t(self, nl, en):
        return nl if self.lang == 'nl' else en

    def title(self, k):
        return PAGES[k][1] if self.lang == 'nl' else PAGES[k][2]

    def engine(self, key, *args):
        if key not in self.engines:
            self.engines[key] = FastEngine(self.exe, *args)
        return self.engines[key]

    def nav(self, current=None):
        items = [(PAGES[k][0], self.title(k)) for k in range(len(PAGES))]
        home = '../nl/index.html' if self.lang == 'nl' else '../../index.html'      # Dutch: default, English in en/
        alt = f'en/{current}' if self.lang == 'nl' else f'../{current}'
        return (f'<a class="brand" href="index.html">{esc(TITLE)}</a>'
                f'{menu([(None, items)], current, self.t("Pagina\'s", "Pages"))}'
                f'<a href="{home}">{self.t("Alle sites", "All sites")}</a>{lang_link(self.lang, alt)}')

    def page(self, *boards):
        pg = Page()
        for gid, eng in boards:
            pg.use(gid, eng, eng.g)
        return pg

    @staticmethod
    def width_height(html_):
        """English: the board B × L (breedte, lengte) is W × H (width, height); only in text, not in tags"""
        out = []
        for part in re.split(r'(<[^>]+>)', html_):
            if not part.startswith('<'):
                part = re.sub(r'(?<![A-Za-z_])B(?![A-Za-z0-9_:])', '\0W', part)
                part = re.sub(r'(?<![A-Za-z_])L(?![A-Za-z0-9_:]| = loss)', '\0H', part).replace('\0', '')   # L = loss stays
            out.append(part)
        return ''.join(out)

    def write(self, name, title, body, pg, gid=None):
        if self.lang == 'en':
            body = self.width_height(body)
        if gid is not None:              # algebraic moves in the running text follow the notation switch
            body = pg.prose_html(body, gid, 'alg')
        k = [p[0] for p in PAGES].index(name)
        prev = f'<a href="{PAGES[k - 1][0]}">← {esc(self.title(k - 1))}</a>' if k > 0 else '<span></span>'
        nxt = f'<a href="{PAGES[k + 1][0]}">{esc(self.title(k + 1))} →</a>' if k + 1 < len(PAGES) else '<span></span>'
        body += f'\n<nav class="pager">{prev}{nxt}</nav>'
        write_page(self.out / name, self.title(k), self.nav(name), body, pg, TITLE, self.lang)

    def results(self, pg, table, nw, nb, clickable=True):
        """the results per board as a table, a row per L and a column per B − L; every number links to a position with
        the longest win on that board (clickable), shown on the large board"""
        t = self.t
        ncol = len(next(iter(table.values())))
        head = '<tr><th>L \\ B − L</th>' + ''.join(f'<th>{d}</th>' for d in range(ncol)) + '</tr>'
        rows = []
        for h, cells in table.items():
            tds = []
            for d, cell in enumerate(cells):
                parts = cell.split('/') if cell else []
                links = []
                for k, part in enumerate(parts):
                    n = int(re.sub(r'\D', '', part))
                    # the colour of the result: won, won with the long diagonal(s), drawn (the hues of ggplot2)
                    kind = 'draw' if part.startswith('(') else 'diag' if '*' in part else 'win'
                    if not clickable:
                        links.append(f'<span class="res-{kind}">{esc(part)}</span>')
                        continue
                    w = h + d
                    corners = 'unplayable' if len(parts) == 2 and k == 0 else 'playable'
                    gid = f'{w}x{h}' + (' unplayable' if corners == 'unplayable' else '')
                    e = self.engine(f'{nw}-{nb} {gid}', '--width', w, '--height', h, '--corners', corners,
                                    '--white', nw, '--black', nb)
                    if gid not in pg.geoms:
                        pg.use(gid, e, e.g)
                    assert e.longest and e.analyse(e.longest)[0] == f'W{n}', (gid, part, e.longest)
                    i = pg.add(e.longest, gid=gid)
                    label = f'{w}×{h}' + (t(' onspeelbare hoeken', ' unplayable corners') if corners == 'unplayable' else
                                          t(' speelbare hoeken', ' playable corners') if len(parts) == 2 else '')
                    links.append(f'<span class="res-{kind}">'
                                 + part.replace(str(n), f'<a class="m1" href="#" data-i="{i}" title="{label}: W{n}">{n}</a>', 1)
                                 + '</span>')
                tds.append('<td>' + '/'.join(links) + '</td>')
            rows.append(f'<tr><th>{h}</th>' + ''.join(tds) + '</tr>')
        return '<table class="tbl results">' + head + ''.join(rows) + '</table>'

    @staticmethod
    def plan(text):
        """moves in algebraic notation (b5-a4, d4xa1; '!' kept) -> plan for Page.moves"""
        out = []
        for tok in text.split():
            m = re.fullmatch(r'([a-z]\d+)([-x])([a-z]\d+)([!?]*)', tok)
            if m:
                out.append(((f'{m.group(1)}{m.group(2)}{m.group(3)}',), m.group(4)))
        return out

    # ---- pages -------------------------------------------------------------------

    def index(self):
        e = self.engine('5x6', '--width', 6, '--height', 5, '--corners', 'playable', '--white', 2, '--black', 1)
        pg = self.page(('5x6', e))
        t = self.t
        fig, _ = pg.diagram('W:WK1,K8:BK7', t('6×5: twee dammen winnen (27 ply)', '6×5: two kings win (27 plies)'))
        body = [f'<h1>{TITLE}</h1>', fig, t('''
<p>Eindspelen met alleen dammen, op borden van elke afmeting <em>B × L</em> (breedte × lengte, <em>B</em> ≥ <em>L</em>, met de families <em>B</em> = <em>L</em> + 1, <em>L</em> + 2 en 2<em>L</em> − 1, zoals in het boek), met internationale regels: vliegende
dammen, slagplicht, meerslag en Turkse slag. Een Nederlands boek uit 1988 over drie dammen tegen één vond zijn
resultaten met de hand, op borden van elke grootte. De databases van
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>
rekenen elke stelling exact uit, met de afstand tot het einde van de partij in ply, en bevestigen, verfijnen en
breiden de resultaten van het boek uit.</p>
<p>Elke stelling op deze site is klikbaar: het grote bord volgt, en eronder staan de waarde en alle zetten. W = winst,
L = verlies, met het aantal ply tot het einde van de partij; D = remise. Zuiver speltheoretisch, zonder
zettenregels.</p>
<p>Omdat het eindspelen met alleen dammen zijn, zonder schijven en dus zonder voor- of achterkant, maakt het niet uit
hoe het bord ligt: kantelen in de diagonaal door a1 verandert geen enkele waarde, en ook de kleuring van de hoekvelden blijft
gelijk. Wij leggen elk bord daarom liggend neer, minstens zo breed als hoog, en noemen het <em>B</em> × <em>L</em>,
breedte × lengte (kolommen × rijen): 11×10 is het bord met <em>B</em> = 11 en <em>L</em> = 10 uit de familie
<em>B</em> = <em>L</em> + 1. De tabellen hebben een rij per <em>L</em> en een kolom per <em>B</em> − <em>L</em>. Het boek
is daarin niet consequent: het schrijft de kortste zijde meestal eerst (10×11) en tekent de borden niet altijd op dezelfde
manier.</p>''', '''
<p>Endgames with kings only, on boards of any size <em>B × L</em> (width × height, <em>B</em> ≥ <em>L</em>, with the families <em>B</em> = <em>L</em> + 1, <em>L</em> + 2 and 2<em>L</em> − 1, as in the book), under international rules: flying kings, mandatory
capture, majority capture and the Turkish strike. A Dutch book of 1988 on three kings against one, <em>Drie tegen een is
gemeen</em> ("three against one is mean"), found its results by hand, on boards of every size. The databases of
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a> compute
every position exactly, with the distance to the end of the game in plies, and confirm, refine and extend the results
of the book.</p>
<p>Every position on this site is clickable: the large board follows, and below it are the value and all moves. W = win,
L = loss, with the number of plies to the end of the game; D = draw. Purely game-theoretic, without move-count
rules.</p>
<p>Since these are endgames with kings only, without men and so without a front or back, it does not matter which way
the board lies: turning it over the diagonal through a1 changes no value, and the colouring of the corner squares stays the same too.
We therefore lay every board down, at least as wide as it is high, and call it <em>B</em> × <em>L</em>, width × height
(files × ranks): 11×10 is the board with <em>B</em> = 11 and <em>L</em> = 10 from the family <em>B</em> = <em>L</em> + 1.
The tables have a row for every <em>L</em> and a column for every <em>B</em> − <em>L</em>. The book is not consistent in
this: it mostly writes the shorter side first (10×11) and does not always draw the boards the same way.</p>'''), t('<h2>Inhoud</h2>', '<h2>Contents</h2>') + '<ul class="toc">']
        for k in range(1, len(PAGES)):
            body.append(f'<li><a href="{PAGES[k][0]}">{esc(self.title(k))}</a></li>')
        body.append('</ul>')
        body.append(t('''
<h2>Vergelijking met het boek</h2>
<p><strong>Bevestigd</strong>: alles wat het boek stellig beweert: 2 tegen 1 voor <em>L</em> ≤ 4 (met de
voorwaarde van de lange lijn op 3×3 en 4×4) en op 7×5 speelbare hoeken; 3 tegen 1 op <em>L</em> = 5 en 6 voor elke
<em>B</em> &gt; <em>L</em> (vierkante borden met de voorwaarde van de lange lijn, expliciet op 5×5 speelbare hoeken);
<em>L</em> = 7 voor <em>B</em> = 7–11; <em>L</em> = 8 voor <em>B</em> = 9, 10, 11 en 15; de families
<em>L</em> + 1, <em>L</em> + 2 en 2<em>L</em> − 1 met de kleuringsregel; de borden met een weggehaald veld 1, 2, 4, 5,
7, 8, 9, 18 en 23.</p>
<p><strong>Nieuw</strong>:</p>
<table class="tbl"><tr><th>Materiaal</th><th>Bord</th><th>Resultaat</th><th>Langste winst</th></tr>
<tr><td>2-1</td><td><a href="twee-tegen-een.html">6×5</a></td><td>gewonnen</td><td>27 ply</td></tr>
<tr><td>3-1</td><td>12×7, 13×7 onspeelbare hoeken, 14×7, 15×7 speelbare hoeken</td><td>gewonnen (algemene methode)</td><td>35, 49, 53, 67</td></tr>
<tr><td>3-1</td><td>15×7 onspeelbare hoeken, 16×7, 17×7</td><td>remise: de grens van <em>L</em> = 7</td><td></td></tr>
<tr><td>3-1</td><td><a href="tien-zonder-veld.html">10×10 zonder g7 (19)</a></td><td>gewonnen</td><td>33</td></tr>
<tr><td>3-1</td><td><a href="tien-zonder-veld.html">10×10 zonder f10, d8, f8 (3, 12, 13)</a></td><td>remise</td><td></td></tr>
</table>
<p><strong>Verfijnd</strong>: 10×10 zonder i9 (geïsoleerd veld j10), zonder h8 (één kunstmatige zetdwang), 8×8 met wit
op de lange lijn (40 vorken met zwart aan zet), en de precieze grenzen van elke rij tot en met <em>L</em> = 12.</p>
<p class="src">De programma's, de tabellen en de Engelse toelichting:
<a href="https://github.com/rhalbersma/egdb-draughts">github.com/rhalbersma/egdb-draughts</a>.</p>''', '''
<h2>Comparison with the book</h2>
<p><strong>Confirmed</strong>: everything the book states firmly — 2-1 for <em>L</em> ≤ 4 (with the long-diagonal
condition on 3×3 and 4×4) and on 7×5 playable corners; 3-1 on <em>L</em> = 5, 6 for every <em>B</em> &gt; <em>L</em> (square boards
with the long-diagonal condition, explicit on 5×5 playable corners); <em>L</em> = 7 for <em>B</em> = 7–11; <em>L</em> = 8 for
<em>B</em> = 9, 10, 11, 15; the families <em>L</em> + 1, <em>L</em> + 2 and 2<em>L</em> − 1 with the colouring rule; the
boards with a square removed 1, 2, 4, 5, 7, 8, 9, 18, 23.</p>
<p><strong>New</strong>:</p>
<table class="tbl"><tr><th>Material</th><th>Board</th><th>Result</th><th>Longest win</th></tr>
<tr><td>2-1</td><td><a href="twee-tegen-een.html">6×5</a></td><td>won</td><td>27 plies</td></tr>
<tr><td>3-1</td><td>12×7, 13×7 unplayable corners, 14×7, 15×7 playable corners</td><td>won (general method)</td><td>35, 49, 53, 67</td></tr>
<tr><td>3-1</td><td>15×7 unplayable corners, 16×7, 17×7</td><td>drawn: the limit of <em>L</em> = 7</td><td></td></tr>
<tr><td>3-1</td><td><a href="tien-zonder-veld.html">10×10 without g7 (19)</a></td><td>won</td><td>33</td></tr>
<tr><td>3-1</td><td><a href="tien-zonder-veld.html">10×10 without f10, d8, f8 (3, 12, 13)</a></td><td>drawn</td><td></td></tr>
</table>
<p><strong>Refined</strong>: 10×10 without i9 (isolated square j10), without h8 (one artificial zugzwang), 8×8 with white
on the long diagonal (40 forks with black to move), and the exact limits of every row up to <em>L</em> = 12.</p>
<p class="src">The programs, the tables and the explanation:
<a href="https://github.com/rhalbersma/egdb-draughts">github.com/rhalbersma/egdb-draughts</a>.</p>'''))
        self.write('index.html', 'Overzicht', '\n'.join(body), pg)

    def twee(self):
        e56 = self.engine('5x6', '--width', 6, '--height', 5, '--corners', 'playable', '--white', 2, '--black', 1)
        e57 = self.engine('5x7', '--width', 7, '--height', 5, '--corners', 'playable', '--white', 2, '--black', 1)
        pg = self.page(('5x6', e56), ('5x7', e57))
        t = self.t
        body = [t('<h1>2 dammen tegen 1</h1>', '<h1>2 kings against 1</h1>'), self.results(pg, RESULTS_21, 2, 1),
                t('<p class="note">Klik op een getal voor de langste winst op dat bord, op het grote bord.</p>',
                  '<p class="note">Click a number for the longest win on that board, on the large board.</p>'),
                f'<p class="note">{t(LEGEND, LEGEND_EN)}</p>', t('''<ul>
<li>Voor <em>L</em> ≤ 4 (gedegenereerde borden) winnen twee dammen altijd, op de vierkante borden als ze de lange
lijn(en) bezetten.</li>
<li>Voor <em>L</em> ≥ 5 zijn maar twee borden gewonnen: <strong>6×5</strong> (nieuw) en <strong>7×5 speelbare hoeken</strong>
(bekend). Vanaf <em>L</em> = 6 is alles remise; de overige winsten zijn korte combinaties waarvan de lengte niet van
de bordgrootte afhangt.</li>
<li>Met de enkele dam aan zet zijn er op 6×5 (8) en 7×5 (16) een paar uitzonderingen: dubbele aanvallen (vorken).</li>
</ul>''', '''<ul>
<li>For <em>L</em> ≤ 4 (degenerate boards) two kings always win, on the square boards provided they hold the long
diagonal(s).</li>
<li>For <em>L</em> ≥ 5 only two boards are won: <strong>6×5</strong> (new) and <strong>7×5 playable corners</strong> (known). From
<em>L</em> = 6 on everything is drawn, and the remaining wins are short combinations whose length does not depend on the
board size.</li>
<li>With the single king to move there are a few exceptions on 6×5 (8) and 7×5 (16): double attacks (forks).</li>
</ul>''')]
        fen = 'W:WK1,K8:BK7'
        fig, i0 = pg.diagram(fen, t('6×5, wit aan zet', '6×5, white to move'), gid='5x6')
        line = ('1. c3-d4 a3-c5 2. d4-b2 c5-e3 3. a5-b4 e3-c5 4. b4-a3 c5-e3 5. b2-e5 e3-f2 6. a3-c5 f2-e1 '
                '7. e5-f4 e1-c3 8. f4-c1 c3-e5 9. c5-e3 e5-c3 10. c1-a3 c3-e5 11. e3-f4 e5-d4 12. a3-b2! d4xa1 '
                '13. f4-e5 a1-b2 14. e5xa1')
        h, *_ = pg.moves(fen, self.plan(line), 1, False, 'm1', i0)
        v, _ = e56.analyse(fen)
        body += [t('<h2>6×5: de langste winst</h2>', '<h2>6×5: the longest win</h2>'), fig,
                 t(f'<p>Wit aan zet wint in {v[1:]} ply, de langste winst op dit bord:</p>',
                   f'<p>White to move wins in {v[1:]} plies, the longest win on this board:</p>'), f'<p class="var">{h}</p>',
                 t('<p>Het lange tempospel dient één doel: offeren zodat zwart <em>de hoek in</em> slaat, en dan de tweede '
                   'dam achter hem op de hoekdiagonaal zetten. Ruim 80% van alle rustige winsten op 6×5 en 7×5 eindigt '
                   'met een zwarte slag de hoek in.</p>',
                   '<p>The long tempo play serves one purpose: sacrifice so that black captures <em>into the corner</em>, '
                   'then place the second king behind it on the corner diagonal. More than 80% of all quiet wins on 6×5 '
                   'and 7×5 end with black capturing into a corner.</p>')]
        fen = 'W:WK1,K9:BK2'
        fig, i0 = pg.diagram(fen, t('7×5 speelbare hoeken, wit aan zet', '7×5 playable corners, white to move'), gid='5x7')
        line = ('1. c3-e5 c5-a3 2. a5-d2 a3-c5 3. d2-c1 c5-f2 4. e5-g3 f2-c5 5. c1-a3 c5-d4 6. a3-b2! d4xa1 '
                '7. g3-e5 a1-b2 8. e5xa1')
        h, *_ = pg.moves(fen, self.plan(line), 1, False, 'm1', i0)
        body += [t('<h2>7×5 speelbare hoeken: hetzelfde slotmotief</h2>', '<h2>7×5 playable corners: the same final motif</h2>'), fig,
                 t('<p>Op 7×5 speelbare hoeken verschijnt hetzelfde slotmotief na 15 ply:</p>',
                   '<p>On 7×5 playable corners the same final motif appears after 15 plies:</p>'), f'<p class="var">{h}</p>']
        self.write('twee-tegen-een.html', '2 dammen tegen 1', '\n'.join(body), pg, '5x6')

    def drie(self):
        e = self.engine('8x8', '--width', 8, '--height', 8, '--corners', 'playable')
        pg = self.page(('8x8', e))
        t = self.t
        body = [t('<h1>3 dammen tegen 1</h1>', '<h1>3 kings against 1</h1>'), self.results(pg, RESULTS_31, 3, 1, clickable=False),
                f'<p class="note">{t(LEGEND, LEGEND_EN)} ' + t(
                    '† : op 8×8 zijn er 40 stellingen (2 op symmetrie na) met de enkele dam aan zet die remise maken met '
                    'een vork, ook als wit de lange lijn bezet. Leeg: niet berekend.</p>',
                    '† : on 8×8 there are 40 positions (2 up to symmetry) with the single king to move that draw by a '
                    'fork, even with white on the long diagonal. Blank: not computed.</p>'),
                t('<p>De 10×10-getallen komen overeen met mdgsoftware.nl: met drie dammen aan zet 486.444 winsten en '
                  '434.756 remises; met de enkele dam aan zet 16.812 winsten, 863.802 remises en 40.586 verliezen.</p>',
                  '<p>The 10×10 numbers agree with mdgsoftware.nl: with three kings to move 486,444 wins and 434,756 '
                  'draws; with the single king to move 16,812 wins, 863,802 draws and 40,586 losses.</p>'),
                t('''<h2>De drie families</h2>
<table class="tbl"><tr><th>Familie</th><th>Resultaat (L = 3 … 12)</th></tr>
<tr><td><strong>B = L + 1</strong></td><td>gewonnen op elk bord</td></tr>
<tr><td><strong>B = L + 2</strong></td><td>gewonnen, behalve de "verkeerde" kleuring, onspeelbare hoekvelden, voor L = 9 en 11</td></tr>
<tr><td><strong>B = 2L − 1</strong></td><td>gewonnen, behalve onspeelbaar voor L = 9 en 11 (19×10: 27 ply, 21×11 speelbare hoeken: 25 ply,
23×12: 27 ply)</td></tr></table>
<p>De kleuringsregel van het boek klopt: vanaf <em>L</em> = 9 wint bij oneven <em>L</em> alleen de kleuring met
speelbare hoekvelden. Tot en met <em>L</em> = 7 wint de verkeerde kleuring nog (9×5 en 13×7 onspeelbare hoeken), omdat de algemene methode
daar nog reikt. Hoe de families winnen, staat bij de <a href="vangstellingen.html">vangstellingen</a>.</p>
<h2>De algemene methode op smalle borden</h2>
<ul>
<li><strong>L = 5, 6</strong>: elk niet-vierkant bord is gewonnen (gecontroleerd tot 13×5 en 15×6). Drie dammen in
een kolom sluiten de volle breedte af en worden over het bord geschoven; de langste winst groeit met ongeveer 2 ply per
extra kolom.</li>
<li><strong>L = 7</strong>: gewonnen tot en met <strong>14×7</strong> en <strong>15×7 speelbare hoeken</strong> (boek: tot 11×7).
Vanaf <em>B</em> = 16 is het remise. De langste winsten springen van 25 naar 35, 49, 53 en 67–73 ply: de ruimte wordt
krap.</li>
<li><strong>L = 8</strong>: gewonnen bij <em>B</em> = 9, 10, 11 en 15 (= 2<em>L</em> − 1), remise bij
<em>B</em> = 12–14 en 16–19.</li>
<li><strong>L ≥ 9</strong>: alleen de families blijven over.</li>
</ul>
<h2>Vierkante borden: de lange lijn</h2>
<p>Op vierkante borden met <em>L</em> ≥ 5 houdt de enkele dam de lange lijn(en). De rustige stellingen met drie dammen
aan zet, gesplitst naar wie de lange lijn(en) bezet (remises / stellingen):</p>
<table class="tbl"><tr><th>Bord</th><th>Wit op de lange lijn(en)</th><th>Niemand</th><th>Alleen zwart</th></tr>
<tr><td>6×6</td><td>0 / 3.154</td><td>0 / 816</td><td>784 / 812</td></tr>
<tr><td>8×8</td><td>0 / 33.364 (40 vorken met zwart aan zet)</td><td>0 / 17.184</td><td>10.696 / 11.084</td></tr>
<tr><td>5×5 speelbare hoeken (twee diagonalen, wit op beide)</td><td>0 / 540</td><td></td><td></td></tr>
<tr><td>7×7 speelbare hoeken (wit op beide)</td><td>0 / 6.032</td><td></td><td></td></tr>
<tr><td>9×9 speelbare hoeken (wit op beide)</td><td>32.492 / 39.328</td><td></td><td></td></tr>
<tr><td>10×10</td><td>177.548 / 206.466</td><td></td><td></td></tr></table>
<p>Tot en met <em>L</em> = 8 beslist de lange lijn; vanaf <em>L</em> = 9 kunnen drie dammen niet winnen, ook niet als ze
hem bezetten. Met één lange diagonaal komt de voorwaarde neer op "zwart bezet hem niet"; met twee diagonalen heeft wit
er echt beide nodig.</p>''', '''<h2>The three families</h2>
<table class="tbl"><tr><th>Family</th><th>Result (L = 3 … 12)</th></tr>
<tr><td><strong>B = L + 1</strong></td><td>won on every board</td></tr>
<tr><td><strong>B = L + 2</strong></td><td>won, except the "wrong" colouring, unplayable corner squares, for L = 9 and 11</td></tr>
<tr><td><strong>B = 2L − 1</strong></td><td>won, except unplayable for L = 9 and 11 (19×10: 27 plies, 21×11 playable corners: 25 plies,
23×12: 27 plies)</td></tr></table>
<p>The colouring rule of the book is confirmed: from <em>L</em> = 9 on, for odd <em>L</em> only the colouring with
playable corner squares wins. Up to <em>L</em> = 7 the wrong colouring still wins (9×5 and 13×7 unplayable corners) because the general
method still reaches that far. How the families win is shown with the <a href="vangstellingen.html">trapping
positions</a>.</p>
<h2>The general method on narrow boards</h2>
<ul>
<li><strong>L = 5, 6</strong>: every non-square board is won (checked up to 13×5 and 15×6). Three kings in a column
close off the full width and can be pushed along the board; the longest win grows by about 2 plies per extra
column.</li>
<li><strong>L = 7</strong>: won up to <strong>14×7</strong> and <strong>15×7 playable corners</strong> (book: up to 11×7). From
<em>B</em> = 16 on it is drawn. The longest wins jump from 25 to 35, 49, 53 and 67–73 plies: the space becomes
tight.</li>
<li><strong>L = 8</strong>: won at <em>B</em> = 9, 10, 11 and 15 (= 2<em>L</em> − 1), drawn at <em>B</em> = 12–14 and
16–19.</li>
<li><strong>L ≥ 9</strong>: only the families remain.</li>
</ul>
<h2>Square boards: the long diagonal</h2>
<p>On square boards with <em>L</em> ≥ 5 the single king holds the long diagonal(s). Splitting the quiet positions with
three kings to move by who occupies the long diagonal(s) (draws / positions):</p>
<table class="tbl"><tr><th>Board</th><th>White on the long diagonal(s)</th><th>Nobody</th><th>Only black</th></tr>
<tr><td>6×6</td><td>0 / 3,154</td><td>0 / 816</td><td>784 / 812</td></tr>
<tr><td>8×8</td><td>0 / 33,364 (40 forks with black to move)</td><td>0 / 17,184</td><td>10,696 / 11,084</td></tr>
<tr><td>5×5 playable corners (two diagonals, white on both)</td><td>0 / 540</td><td></td><td></td></tr>
<tr><td>7×7 playable corners (white on both)</td><td>0 / 6,032</td><td></td><td></td></tr>
<tr><td>9×9 playable corners (white on both)</td><td>32,492 / 39,328</td><td></td><td></td></tr>
<tr><td>10×10</td><td>177,548 / 206,466</td><td></td><td></td></tr></table>
<p>Up to <em>L</em> = 8 the long diagonal decides; from <em>L</em> = 9 on three kings cannot win even when they hold it.
With one long diagonal the condition amounts to "black does not hold it"; with two diagonals white really needs
both.</p>''')]
        fen = self.longest(e, 3, 1)
        if fen:
            fig, i0 = pg.diagram(fen, t('8×8, wit aan zet', '8×8, white to move'))
            v, _ = e.analyse(fen)
            h, *_ = pg.best_line(fen, 80, start=i0)
            body += [t('<h2>8×8: een langste winst</h2>', '<h2>8×8: a longest win</h2>'), fig,
                     t(f'<p>Met wit op de lange lijn wint wit altijd. Een van de langste winsten ({v[1:]} ply):</p>',
                       f'<p>With white on the long diagonal white always wins. One of the longest wins ({v[1:]} plies):</p>'),
                     f'<p class="var">{h}</p>']
            # after 9. h8-c3 (ply 17) the white kings form Petrov's triangle
            k = i0
            for _ in range(17):
                k = pg.pos[k]['next']
            tri = pg.pos[k]['f']
            fig2, _ = pg.diagram(tri, t('na 9. h8-c3: de driehoek van Petrov', 'after 9. h8-c3: Petrov\'s triangle'))
            body += ['<div style="clear:both"></div>', fig2, t(
                '<p>Met 9. h8-c3 staat de <strong>driehoek van Petrov</strong>: de drie witte dammen vormen een driehoek '
                'waarvan de punt naar de zwarte dam aan de rand wijst; het gebruikelijke diagram (wit d6, c3, e3 tegen '
                'zwart d8) is hier gespiegeld in de lange lijn. Het is de klassieke methode waarmee drie dammen die de '
                'lange lijn bezetten de enkele dam vangen. In het Russische damspel publiceerde A.D. Petrov haar in '
                '1827, maar Pedro Ruiz Montero beschreef haar al in 1591; buiten Rusland heet ze ook de methode van '
                'Montero (<a href="https://ru.wikipedia.org/wiki/%D0%A2%D1%80%D0%B5%D1%83%D0%B3%D0%BE%D0%BB%D1%8C%D0%BD%D0%B8%D0%BA_%D0%9F%D0%B5%D1%82%D1%80%D0%BE%D0%B2%D0%B0">Wikipedia</a>). Daarna is zwart snel uitgeteld: wit offert twee keer '
                '(11. c5-f2 en 12. c3-a5) en slaat de zwarte dam met 13. a5×c3.</p>',
                '<p>After 9. h8-c3 the white kings form <strong>Petrov\'s triangle</strong>: a triangle whose point is '
                'aimed at the black king on the edge; the usual diagram (white d6, c3, e3 against black d8) is mirrored '
                'in the long diagonal here. It is the classic method by which three kings holding the long diagonal '
                'catch the single king. In Russian draughts A.D. Petrov published it in 1827, but Pedro Ruiz Montero '
                'described it as early as 1591; outside Russia it is also known as Montero\'s method '
                '(<a href="https://ru.wikipedia.org/wiki/%D0%A2%D1%80%D0%B5%D1%83%D0%B3%D0%BE%D0%BB%D1%8C%D0%BD%D0%B8%D0%BA_%D0%9F%D0%B5%D1%82%D1%80%D0%BE%D0%B2%D0%B0">Wikipedia</a>). After that black is soon counted out: white sacrifices twice '
                '(11. c5-f2 and 12. c3-a5) and captures the black king with 13. a5×c3.</p>')]
        self.write('drie-tegen-een.html', '3 dammen tegen 1', '\n'.join(body), pg, '8x8')

    @staticmethod
    def longest(e, nw, nb):
        """a quiet position with white to move and the longest win (searched over the long diagonal setups
        of 8x8: white kings on the long diagonal, black king anywhere), or None"""
        sq = sorted(e.g['sq'])
        name = {n: f'{chr(97 + f)}{r + 1}' for n, (f, r) in e.g['sq'].items()}
        diag = [n for n in sq if e.g['sq'][n][0] == e.g['sq'][n][1]]
        best = None
        from itertools import combinations
        for wk in combinations(diag, nw):
            for b in sq:
                if b in wk:
                    continue
                fen = e.fen([name[x] for x in wk], [name[b]])
                v, legal = e.analyse(fen)
                if v[0] != 'W' or any('x' in m[0] for m in legal):
                    continue
                _, bl = e.analyse('B' + fen[1:])
                if any('x' in m[0] for m in bl):
                    continue
                if best is None or int(v[1:]) > best[0]:
                    best = (int(v[1:]), fen)
        return best[1] if best else None

    def traps(self):
        fams = [('10x11', 11, 10, ['b2', 'k9', 'i7'], 'L + 1', 'h10'),
                ('10x12', 12, 10, ['b2', 'l8', 'j6'], 'L + 2', None),
                ('10x19', 19, 10, ['b2', 'j8', 'r2'], '2L − 1', None)]
        engines = {gid: self.engine(gid, '--width', w, '--height', h, '--corners', 'playable') for gid, w, h, *_ in fams}
        pg = self.page(*engines.items())
        t = self.t
        body = [t('<h1>Vangstellingen</h1>', '<h1>Trapping positions</h1>'), t('''
<p>Elke familie heeft een vangstelling voor de drie witte dammen die vastligt <strong>ten opzichte van een hoek of van
de V</strong>. Op dezelfde plaats gehouden kost de omzetting op elk bord precies even veel ply. De sterke partij bouwt
de vangstelling op vanuit een willekeurige stelling en zet daarna snel om: op 17×9 is de langste winst 23 ply, waarvan
hooguit 8 voor de omzetting.</p>
{TABLE}
<p>(Langste verlies met zwart aan zet / langste winst met wit aan zet, over elk veld van de zwarte dam waar de partij
aan zet niet kan slaan. Telt men alleen de velden waar geen van beide kan slaan, dan is de langste winst met wit aan zet
9, 9 en 7 ply.) Hieronder per familie het veld van de zwarte dam met de langste winst waar geen van beide kan
slaan.</p>''', '''
<p>Each family has a trapping position for the three white kings that is fixed <strong>relative to a corner or to the
V</strong>. Kept at the same place on every board size, the conversion always takes exactly the same number of plies.
The strong side builds the trapping position from an arbitrary position and then converts quickly: on 17×9 the longest
win is 23 plies, of which at most 8 are the conversion.</p>
{TABLE}
<p>(Longest loss with black to move / longest win with white to move, over every square of the black king where the side
to move cannot capture. Counting only the squares where neither side can capture, the longest win with white to move is
9, 9 and 7 plies.) Below, per family, the square of the black king with the longest win where neither side can
capture.</p>''')]
        # the trapping positions in general coordinates (file, rank), counted from (1, 1) = a1 at the bottom left;
        # checked on every board of the column "boards" with the databases
        gen = [('L + 1', '(2, 2), (B, L − 1), (B − 2, L − 3)', '10x11', '7×6 … 12×11', 10, 9),
               ('L + 2', '(2, 2), (B, L − 2), (B − 2, L − 4)', '10x12', '8×6 … 12×10', 12, 11),
               ('2L − 1', '(2, 2), (L, L − 2), (B − 1, 2)', '10x19', '9×5 … 17×9', 8, 7)]
        ex = {gid: wk for gid, w, h, wk, *_ in fams}
        shown = {gid: f'{w}×{h}' for gid, w, h, *_ in fams}       # as drawn: files × ranks (the book: 10×11 etc.)
        rows = ''.join(f'<tr><td><strong>{f}</strong></td><td>{c} — {t("bv.", "e.g.")} {shown[g]}: '
                       f'{pg.prose(", ".join(ex[g]), g, "alg")}</td><td>{boards}</td><td>{lb}</td><td>{lw}</td></tr>'
                       for f, c, g, boards, lb, lw in gen)
        table = (t('<table class="tbl"><tr><th>Familie</th><th>Witte dammen (kolom, rij)</th><th>Gecontroleerd</th>'
                   '<th>Zwart aan zet</th><th>Wit aan zet</th></tr>',
                   '<table class="tbl"><tr><th>Family</th><th>White kings (file, rank)</th><th>Boards checked</th>'
                   '<th>Black to move</th><th>White to move</th></tr>') + rows + '</table>'
                 + t('<p class="note">Coördinaten (kolom, rij) gerekend vanaf (1, 1) = a1 linksonder, op het bord '
                     '<em>B</em> × <em>L</em> met <em>B</em> kolommen en <em>L</em> rijen. Bij 2<em>L</em> − 1 staan (2, 2) en '
                     '(<em>B</em> − 1, 2) op de benen van de V en (<em>L</em>, <em>L</em> − 2) onder de top.</p>',
                     '<p class="note">Coordinates (file, rank) counted from (1, 1) = a1 at the bottom left, on the board '
                     '<em>B</em> × <em>L</em> with <em>B</em> files and <em>L</em> ranks. For 2<em>L</em> − 1, (2, 2) and '
                     '(<em>B</em> − 1, 2) stand on the legs of the V and (<em>L</em>, <em>L</em> − 2) below its apex.</p>'))
        body[1] = body[1].replace('{TABLE}', table)
        for gid, w, h, wk, fam, black in fams:
            e = engines[gid]
            if black is None:
                black = self.hardest(e, wk)
            fen = e.fen(wk, [black])
            fig, i0 = pg.diagram(fen, f'{shown[gid]}: ' + t(f'familie {fam}, wit aan zet',
                                                                       f'family {fam}, white to move'), gid=gid)
            v, _ = e.analyse(fen)
            hl, *_ = pg.best_line(fen, 40, start=i0)
            fb = 'B' + fen[1:]
            ib = pg.add(fb, gid=gid)
            vb, _ = e.analyse(fb)
            hb, *_ = pg.best_line(fb, 40, start=ib)
            body += [f'<h2>{t("Familie", "Family")} {fam}: {shown[gid]}</h2>', fig,
                     pg.prose_html(t(f'<p>Witte dammen op {", ".join(wk)}, de zwarte dam op {black}. Wit aan zet: {v}.</p>',
                       f'<p>White kings on {", ".join(wk)}, the black king on {black}. White to move: {v}.</p>'), gid, 'alg'),
                     f'<p class="var">{hl}</p>', t(f'<p>Dezelfde stelling met zwart aan zet: {vb}.</p>',
                                                   f'<p>The same position with black to move: {vb}.</p>'),
                     f'<p class="var">{hb}</p>']
            if gid == '10x11':
                body.append(pg.prose_html(t('<p>Op 11×10 (in het boek 10×11, diagram 8) wint wit met wit aan zet op elk veld van de zwarte dam '
                            'binnen 5 ply, <strong>behalve op h10, het gearceerde veld in het boek, waar het 9 ply '
                            'kost</strong>. De dam op b2 staat op de lange lijn, waarvan het andere eind een ingang '
                            'rechtsboven is; de twee dammen rechtsboven vormen er een halve val mee en de bovenste kan '
                            'de lange lijn afsluiten.</p>',
                              '<p>On 11×10 (10×11 in the book, diagram 8) white to move wins within 5 plies on every square of '
                              'the black king, <strong>except h10, the hatched square in the book, which takes 9 '
                              'plies</strong>. The king at b2 stands on the long diagonal, whose other end is an entrance '
                              'at the top right; the two kings at the top right form a half trap with it and the upper one '
                              'can close the long diagonal.</p>'), gid, 'alg'))
            if gid == '10x19':
                body.append(t('<p><strong>De V</strong> (alleen bij <em>B</em> = 2<em>L</em> − 1): de diagonalen vanuit '
                            'de twee hoeken van één lange zijde ontmoeten elkaar precies in het midden van de overkant. '
                            'Korter, en de benen kruisen onder de rand; langer, en ze raken de rand op twee verschillende '
                            'velden. Even <em>L</em> heeft altijd de twee hoeken; oneven <em>L</em> alleen met speelbare '
                            'hoekvelden.</p>',
                              '<p><strong>The V</strong> (only for <em>B</em> = 2<em>L</em> − 1): the diagonals from the '
                              'two corners of one long side meet exactly in the middle of the opposite side. Shorter, and '
                              'the legs cross below the edge; longer, and they hit the edge at two different squares. Even '
                              '<em>L</em> always has the two corners; odd <em>L</em> has them only with playable corner squares.</p>'))
            body.append('<div style="clear:both"></div>')
        self.write('vangstellingen.html', 'Vangstellingen', '\n'.join(body), pg)

    @staticmethod
    def hardest(e, wk):
        """the square of the black king with the longest win for white to move (quiet positions)"""
        best = None
        for n, (f, r) in e.g['sq'].items():
            b = f'{chr(97 + f)}{r + 1}'
            if b in wk:
                continue
            fen = e.fen(wk, [b])
            v, legal = e.analyse(fen)
            if v[0] != 'W' or any('x' in m[0] for m in legal) or any('x' in m[0] for m in e.analyse('B' + fen[1:])[1]):
                continue
            if best is None or int(v[1:]) > best[0]:
                best = (int(v[1:]), b)
        return best[1]

    def lakes(self):
        e = self.engine('h8', '--board', 'international', '--lake', 'h8')
        pg = self.page(('h8', e))
        rows = [('1 (b10)', 'a9, j2, i1', 'W', '<strong>W</strong>', '23', '0'),
                ('2 (d10)', 'a7, j4, g1', 'D', '<strong>D</strong>', '(33)', '64.675'),
                ('3 (f10)', 'j6, a5, e1', '?', '<strong>D</strong>', '(23)', '337.341'),
                ('4 (h10)', 'j8, a3, c1', 'D', '<strong>D</strong>', '(29)', '66.013'),
                ('5 (j10, Mertens)', 'a1', 'W', '<strong>W</strong>', '21', '0'),
                ('7 (c9)', 'b8, i3, h2', 'W', '<strong>W</strong>', '25', '0'),
                ('8 (e9)', 'b6, i5, f2', 'D', '<strong>D</strong>', '(31)', '64.192'),
                ('9 (g9)', 'i7, b4, d2', 'D', '<strong>D</strong>', '(27)', '66.653'),
                ('10 (i9)', 'b2', 'W', self.t('<strong>W als j10 leeg is</strong>', '<strong>W if j10 is empty</strong>'), '(19)',
                 self.t('29.875, alle met een stuk op het geïsoleerde veld j10', '29,875, all with a piece on the isolated square j10')),
                ('12 (d8)', 'c7, h4, g3', '?', '<strong>D</strong>', '(35)', '66.365'),
                ('13 (f8)', 'h6, c5, e3', '?', '<strong>D</strong>', '(37)', '67.259'),
                ('14 (h8)', 'c3', 'W', self.t('<strong>W op één zetdwang na</strong>', '<strong>W except one zugzwang</strong>'), '33',
                 self.t('2 (één stelling en haar spiegelbeeld)', '2 (one position and its mirror image)')),
                ('18 (e7)', 'd6, g5, f4', 'D', '<strong>D</strong>', '(35)', '67.660'),
                ('19 (g7)', 'd4', '?', '<strong>W</strong>', '33', '0'),
                ('23 (f6)', 'e5', 'W', '<strong>W</strong>', '35', '0')]
        t = self.t
        if self.lang == 'en':
            rows = [tuple(c.replace('.', ',') if i == 5 and c[:1].isdigit() else c for i, c in enumerate(r)) for r in rows]
        tbl = [t('<table class="tbl"><tr><th>Weg</th><th>Symmetrische velden</th><th>Boek</th><th>Database</th>'
                 '<th>Langste winst</th><th>Rustige remises (3 dammen aan zet)</th></tr>',
                 '<table class="tbl"><tr><th>Removed</th><th>Symmetric squares</th><th>Book</th><th>Database</th>'
                 '<th>Longest win</th><th>Quiet draws (3 kings to move)</th></tr>')]
        tbl += [f'<tr>{"".join(f"<td>{c}</td>" for c in r)}</tr>' for r in rows]
        tbl.append('</table>')
        fen = 'W:WK4,K5,K10:BK15'
        fig, i0 = pg.diagram(fen, t('10×10 zonder h8, wit aan zet', '10×10 without h8, white to move'))
        v, legal = e.analyse(fen)
        mv = ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i0, m)}">1. {pg.label(pg.pos[i0]['g'], m)}</a> '
                       f'<span class="val">{m[1]}</span>' for m in legal)
        fb = 'B' + fen[1:]
        figb, ib = pg.diagram(fb, t('10×10 zonder h8, zwart aan zet', '10×10 without h8, black to move'))
        vb, _ = e.analyse(fb)
        hb, *_ = pg.best_line(fb, 40, start=ib)
        body = [t('<h1>10×10 met één veld weg</h1>', '<h1>10×10 with one square removed</h1>'),
                t('<p>Eén veld per symmetrieklasse (de 4 symmetrieën van het bord geven 15 klassen), 3 dammen tegen 1:</p>',
                  '<p>One square per symmetry class (the 4 symmetries of the board give 15 classes), 3 kings against 1:</p>'),
                '\n'.join(tbl), t('''<ul>
<li>Zonder i9 is j10 geïsoleerd: een dam daar kan nooit zetten en nooit geslagen worden.</li>
<li>De enige uitzondering zonder h8 staat hieronder.</li></ul>''', '''<ul>
<li>Removing i9 isolates j10: a king there can never move and never be captured.</li>
<li>The single exception with h8 removed is shown below.</li></ul>'''),
                t('<h2>Zonder h8: de zetdwang</h2>', '<h2>Without h8: the zugzwang</h2>'), fig,
                t(f'<p>Wit op h10, j10 en i9, zwart op j8, wit aan zet: {v}. i9 en j10 zitten vast en elke zet van h10 laat '
                  f'een dubbele slag toe (1. h10-g9 j8×h10×f8), en 1 tegen 1 is remise. Alle zetten: {mv}.</p>',
                  f'<p>White on h10, j10 and i9, black on j8, white to move: {v}. i9 and j10 are locked in, every move of '
                  f'h10 allows a double capture (1. h10-g9 j8×h10×f8), and 1-1 is drawn. All moves: {mv}.</p>'),
                figb, t(f'<p>Met zwart aan zet is dezelfde stelling verloren in {vb[1:]} ply:</p>',
                        f'<p>With black to move the same position is lost in {vb[1:]} plies:</p>'), f'<p class="var">{hb}</p>']
        self.write('tien-zonder-veld.html', '10×10 met één veld weg', '\n'.join(body), pg, 'h8')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--engine', default=str(ROOT / 'build' / 'egdb_fast'))
    ap.add_argument('--out', default=str(ROOT / '_site' / 'drie_tegen_een_is_gemeen'))
    args = ap.parse_args()
    engines = {}
    for lang in ('nl', 'en'):
        out = Path(args.out) / ('' if lang == 'nl' else 'en')
        out.mkdir(parents=True, exist_ok=True)
        s = Site(args.engine, out, lang, engines)
        s.index()
        s.twee()
        s.drie()
        s.traps()
        s.lakes()


if __name__ == '__main__':
    main()
