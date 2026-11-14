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
from sitekit import FastEngine, Page, esc, write_page   # noqa: E402

TITLE = 'Drie tegen een is gemeen'

PAGES = [
    ('index.html', 'Overzicht'),
    ('twee-tegen-een.html', '2 dammen tegen 1'),
    ('drie-tegen-een.html', '3 dammen tegen 1'),
    ('vangstellingen.html', 'Vangstellingen'),
    ('tien-zonder-veld.html', '10×10 met één veld weg'),
]

TABLE_21 = """ B \\ L−B     0        1       2        3
 3         3/3*       7      11/9      11
 4          15*      13      13       21
 5        (7)/(3)    27     (9)/15    (9)
 6          (7)      (9)    (11)      (9)
 7        (7)/(5)    (9)    (9)/(11)  (9)
 8          (7)      (9)    (11)      (9)
 9        (7)/(5)    (9)    (9)/(11)  (9)
10          (7)      (9)    (11)      (9)
11        (7)/(5)    (9)    (9)/(11)  (9)
12          (7)      (9)    (11)      (9)"""

TABLE_31 = """ B \\ L−B    0       1      2       3      4       5      6       7      8       9     10      11
 3        0/3      5     7/9      9    9/11     13   15/15
 4         9*     11     11      13     17      23     29      37
 5      11/15*    23   13/13     15   17/15     17   19/19     21   23/23
 6        17*     15     17      19     19      21     25      27     29      33
 7      19/17*    19   21/19     23   25/25     35   49/21     53  (73)/67  (73) (73)/(43)
 8        25*†    19     23      31    (23)    (33)   (21)     25    (19)    (19)   (21)    (19)
 9     (15)/(15)  21  (19)/21   (27) (17)/(19) (31) (19)/(21) (21)  (19)/23  (19) (19)/(19) (21)
10       (15)     21     25     (27)   (19)    (31)   (21)    (21)   (19)     27   (19)    (19)
11     (15)/(15)  21  (19)/21   (27) (17)/(19) (31) (19)/(21) (21) (19)/(21) (19)  (19)/25  (19)
12       (15)     21     25     (27)   (19)    (31)   (21)    (21)   (19)    (21)          27"""

LEGEND = ('In de tabellen: <code>21</code> = gewonnen, langste winst 21 ply · <code>21*</code> = gewonnen als de '
          'sterke partij de lange lijn(en) bezet · <code>(21)</code> = remise (het getal is de langste winst onder de '
          'stellingen die wel gewonnen zijn) · <code>13/15</code> = p0/p1. Een bord <em>B × L</em> heeft <em>B</em> '
          'als kortste zijde; p1 = de hoekvelden zijn speelbaar, p0 = niet (alleen van belang bij oneven × oneven).')


class Site:
    def __init__(self, exe, out):
        self.exe = exe
        self.out = out
        self.engines = {}

    def engine(self, key, *args):
        if key not in self.engines:
            self.engines[key] = FastEngine(self.exe, *args)
        return self.engines[key]

    def nav(self):
        links = ''.join(f'<a href="{f}">{esc(t)}</a>' for f, t in PAGES[1:])
        return f'<a href="../index.html">egdb-draughts</a><a href="index.html">{TITLE}</a>' + links

    def page(self, *boards):
        pg = Page()
        for gid, eng in boards:
            pg.use(gid, eng, eng.g)
        return pg

    def write(self, name, title, body, pg):
        k = [f for f, _ in PAGES].index(name)
        prev = f'<a href="{PAGES[k - 1][0]}">← {esc(PAGES[k - 1][1])}</a>' if k > 0 else '<span></span>'
        nxt = f'<a href="{PAGES[k + 1][0]}">{esc(PAGES[k + 1][1])} →</a>' if k + 1 < len(PAGES) else '<span></span>'
        body += f'\n<nav class="pager">{prev}{nxt}</nav>'
        write_page(self.out / name, title, self.nav(), body, pg, TITLE)

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
        e = self.engine('5x6', '--width', 6, '--height', 5, '--parity', 1, '--white', 2, '--black', 1)
        pg = self.page(('5x6', e))
        fig, _ = pg.diagram('W:WK1,K8:BK7', '5×6: twee dammen winnen (27 ply)')
        body = [f'<h1>{TITLE}</h1>', fig, '''
<p>Eindspelen met alleen dammen, op borden van elke afmeting <em>B × L</em>, met internationale regels: vliegende
dammen, slagplicht, meerslag en Turkse slag. Een Nederlands boek uit 1988 over drie dammen tegen één vond zijn
resultaten met de hand, op borden van elke grootte. De databases van
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>
rekenen elke stelling exact uit, met de afstand tot het einde van de partij in ply, en bevestigen, verfijnen en
breiden de resultaten van het boek uit.</p>
<p>Elke stelling op deze site is klikbaar: het grote bord volgt, en eronder staan de waarde en alle zetten. W = winst,
L = verlies, met het aantal ply tot het einde van de partij; D = remise. Zuiver speltheoretisch, zonder
zettenregels.</p>''', '<h2>Inhoud</h2><ul class="toc">']
        for f, t in PAGES[1:]:
            body.append(f'<li><a href="{f}">{esc(t)}</a></li>')
        body.append('</ul>')
        body.append('''
<h2>Vergelijking met het boek</h2>
<p><strong>Bevestigd</strong>: alles wat het boek stellig beweert: 2 tegen 1 voor <em>B</em> ≤ 4 (met de
voorwaarde van de lange lijn op 3×3 en 4×4) en op 5×7 p1; 3 tegen 1 op <em>B</em> = 5 en 6 voor elke
<em>L</em> &gt; <em>B</em> (vierkante borden met de voorwaarde van de lange lijn, expliciet op 5×5 p1);
<em>B</em> = 7 voor <em>L</em> = 7–11; <em>B</em> = 8 voor <em>L</em> = 9, 10, 11 en 15; de families
<em>B</em> + 1, <em>B</em> + 2 en 2<em>B</em> − 1 met de kleuringsregel; de borden met een weggehaald veld 1, 2, 4, 5,
7, 8, 9, 18 en 23.</p>
<p><strong>Nieuw</strong>:</p>
<table class="tbl"><tr><th>Materiaal</th><th>Bord</th><th>Resultaat</th><th>Langste winst</th></tr>
<tr><td>2-1</td><td><a href="twee-tegen-een.html">5×6</a></td><td>gewonnen</td><td>27 ply</td></tr>
<tr><td>3-1</td><td>7×12, 7×13 p0, 7×14, 7×15 p1</td><td>gewonnen (algemene methode)</td><td>35, 49, 53, 67</td></tr>
<tr><td>3-1</td><td>7×15 p0, 7×16, 7×17</td><td>remise: de grens van <em>B</em> = 7</td><td></td></tr>
<tr><td>3-1</td><td><a href="tien-zonder-veld.html">10×10 zonder g7 (19)</a></td><td>gewonnen</td><td>33</td></tr>
<tr><td>3-1</td><td><a href="tien-zonder-veld.html">10×10 zonder f10, d8, f8 (3, 12, 13)</a></td><td>remise</td><td></td></tr>
</table>
<p><strong>Verfijnd</strong>: 10×10 zonder i9 (geïsoleerd veld j10), zonder h8 (één kunstmatige zetdwang), 8×8 met wit
op de lange lijn (40 vorken met zwart aan zet), en de precieze grenzen van elke rij tot en met <em>B</em> = 12.</p>
<p class="src">De programma's, de tabellen en de Engelse toelichting:
<a href="https://github.com/rhalbersma/egdb-draughts">github.com/rhalbersma/egdb-draughts</a>.</p>''')
        self.write('index.html', 'Overzicht', '\n'.join(body), pg)

    def twee(self):
        e56 = self.engine('5x6', '--width', 6, '--height', 5, '--parity', 1, '--white', 2, '--black', 1)
        e57 = self.engine('5x7', '--width', 7, '--height', 5, '--parity', 1, '--white', 2, '--black', 1)
        pg = self.page(('5x6', e56), ('5x7', e57))
        body = ['<h1>2 dammen tegen 1</h1>', f'<pre class="tbl">{esc(TABLE_21)}</pre>', f'<p class="note">{LEGEND}</p>',
                '''<ul>
<li>Voor <em>B</em> ≤ 4 (gedegenereerde borden) winnen twee dammen altijd, op de vierkante borden als ze de lange
lijn(en) bezetten.</li>
<li>Voor <em>B</em> ≥ 5 zijn maar twee borden gewonnen: <strong>5×6</strong> (nieuw) en <strong>5×7 p1</strong>
(bekend). Vanaf <em>B</em> = 6 is alles remise; de overige winsten zijn korte combinaties waarvan de lengte niet van
de bordgrootte afhangt.</li>
<li>Met de enkele dam aan zet zijn er op 5×6 (8) en 5×7 (16) een paar uitzonderingen: dubbele aanvallen (vorken).</li>
</ul>''']
        fen = 'W:WK1,K8:BK7'
        fig, i0 = pg.diagram(fen, '5×6, wit aan zet', gid='5x6')
        line = ('1. c3-d4 a3-c5 2. d4-b2 c5-e3 3. a5-b4 e3-c5 4. b4-a3 c5-e3 5. b2-e5 e3-f2 6. a3-c5 f2-e1 '
                '7. e5-f4 e1-c3 8. f4-c1 c3-e5 9. c5-e3 e5-c3 10. c1-a3 c3-e5 11. e3-f4 e5-d4 12. a3-b2! d4xa1 '
                '13. f4-e5 a1-b2 14. e5xa1')
        h, *_ = pg.moves(fen, self.plan(line), 1, False, 'm1', i0)
        v, _ = e56.analyse(fen)
        body += ['<h2>5×6: de langste winst</h2>', fig,
                 f'<p>Wit aan zet wint in {v[1:]} ply, de langste winst op dit bord:</p>', f'<p class="var">{h}</p>',
                 '<p>Het lange tempospel dient één doel: offeren zodat zwart <em>de hoek in</em> slaat, en dan de tweede '
                 'dam achter hem op de hoekdiagonaal zetten. Ruim 80% van alle rustige winsten op 5×6 en 5×7 eindigt '
                 'met een zwarte slag de hoek in.</p>']
        fen = 'W:WK1,K9:BK2'
        fig, i0 = pg.diagram(fen, '5×7 p1, wit aan zet', gid='5x7')
        line = ('1. c3-e5 c5-a3 2. a5-d2 a3-c5 3. d2-c1 c5-f2 4. e5-g3 f2-c5 5. c1-a3 c5-d4 6. a3-b2! d4xa1 '
                '7. g3-e5 a1-b2 8. e5xa1')
        h, *_ = pg.moves(fen, self.plan(line), 1, False, 'm1', i0)
        body += ['<h2>5×7 p1: hetzelfde slotmotief</h2>', fig,
                 '<p>Op 5×7 p1 verschijnt hetzelfde slotmotief na 15 ply:</p>', f'<p class="var">{h}</p>']
        self.write('twee-tegen-een.html', '2 dammen tegen 1', '\n'.join(body), pg)

    def drie(self):
        e = self.engine('8x8', '--width', 8, '--height', 8, '--parity', 1)
        pg = self.page(('8x8', e))
        body = ['<h1>3 dammen tegen 1</h1>', f'<pre class="tbl">{esc(TABLE_31)}</pre>', f'<p class="note">{LEGEND} '
                '† : op 8×8 zijn er 40 stellingen (2 op symmetrie na) met de enkele dam aan zet die remise maken met '
                'een vork, ook als wit de lange lijn bezet. Leeg: niet berekend.</p>',
                '<p>De 10×10-getallen komen overeen met mdgsoftware.nl: met drie dammen aan zet 486.444 winsten en '
                '434.756 remises; met de enkele dam aan zet 16.812 winsten, 863.802 remises en 40.586 verliezen.</p>',
                '''<h2>De drie families</h2>
<table class="tbl"><tr><th>Familie</th><th>Resultaat (B = 3 … 12)</th></tr>
<tr><td><strong>L = B + 1</strong></td><td>gewonnen op elk bord</td></tr>
<tr><td><strong>L = B + 2</strong></td><td>gewonnen, behalve de "verkeerde" kleuring p0 voor B = 9 en 11</td></tr>
<tr><td><strong>L = 2B − 1</strong></td><td>gewonnen, behalve p0 voor B = 9 en 11 (10×19: 27 ply, 11×21 p1: 25 ply,
12×23: 27 ply)</td></tr></table>
<p>De kleuringsregel van het boek klopt: vanaf <em>B</em> = 9 wint bij oneven <em>B</em> alleen de kleuring met
speelbare hoeken (p1). Op <em>B</em> = 7 wint de verkeerde kleuring nog (5×9 p0, 7×13 p0), omdat de algemene methode
daar nog reikt. Hoe de families winnen, staat bij de <a href="vangstellingen.html">vangstellingen</a>.</p>
<h2>De algemene methode op smalle borden</h2>
<ul>
<li><strong>B = 5, 6</strong>: elk niet-vierkant bord is gewonnen (gecontroleerd tot 5×13 en 6×15). Drie dammen in
een kolom sluiten de volle breedte af en worden over het bord geschoven; de langste winst groeit met ongeveer 2 ply per
extra kolom.</li>
<li><strong>B = 7</strong>: gewonnen tot en met <strong>7×14</strong> en <strong>7×15 p1</strong> (boek: tot 7×11).
Vanaf <em>L</em> = 16 is het remise. De langste winsten springen van 25 naar 35, 49, 53 en 67–73 ply: de ruimte wordt
krap.</li>
<li><strong>B = 8</strong>: gewonnen bij <em>L</em> = 9, 10, 11 en 15 (= 2<em>B</em> − 1), remise bij
<em>L</em> = 12–14 en 16–19.</li>
<li><strong>B ≥ 9</strong>: alleen de families blijven over.</li>
</ul>
<h2>Vierkante borden: de lange lijn</h2>
<p>Op vierkante borden met <em>B</em> ≥ 5 houdt de enkele dam de lange lijn(en). De rustige stellingen met drie dammen
aan zet, gesplitst naar wie de lange lijn(en) bezet (remises / stellingen):</p>
<table class="tbl"><tr><th>Bord</th><th>Wit op de lange lijn(en)</th><th>Niemand</th><th>Alleen zwart</th></tr>
<tr><td>6×6</td><td>0 / 3.154</td><td>0 / 816</td><td>784 / 812</td></tr>
<tr><td>8×8</td><td>0 / 33.364 (40 vorken met zwart aan zet)</td><td>0 / 17.184</td><td>10.696 / 11.084</td></tr>
<tr><td>5×5 p1 (twee diagonalen, wit op beide)</td><td>0 / 540</td><td></td><td></td></tr>
<tr><td>7×7 p1 (wit op beide)</td><td>0 / 6.032</td><td></td><td></td></tr>
<tr><td>9×9 p1 (wit op beide)</td><td>32.492 / 39.328</td><td></td><td></td></tr>
<tr><td>10×10</td><td>177.548 / 206.466</td><td></td><td></td></tr></table>
<p>Tot en met <em>B</em> = 8 beslist de lange lijn; vanaf <em>B</em> = 9 kunnen drie dammen niet winnen, ook niet als ze
hem bezetten. Met één lange diagonaal komt de voorwaarde neer op "zwart bezet hem niet"; met twee diagonalen heeft wit
er echt beide nodig.</p>''']
        fen = self.longest(e, 3, 1)
        if fen:
            fig, i0 = pg.diagram(fen, '8×8, wit aan zet')
            v, _ = e.analyse(fen)
            h, *_ = pg.best_line(fen, 80, start=i0)
            body += ['<h2>8×8: een langste winst</h2>', fig,
                     f'<p>Met wit op de lange lijn wint wit altijd. Een van de langste winsten ({v[1:]} ply):</p>',
                     f'<p class="var">{h}</p>']
        self.write('drie-tegen-een.html', '3 dammen tegen 1', '\n'.join(body), pg)

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
        fams = [('10x11', 11, 10, ['b2', 'k9', 'i7'], 'B + 1', 'h10'),
                ('10x12', 12, 10, ['b2', 'l8', 'j6'], 'B + 2', None),
                ('10x19', 19, 10, ['b2', 'j8', 'r2'], '2B − 1', None)]
        engines = {gid: self.engine(gid, '--width', w, '--height', h, '--parity', 1) for gid, w, h, *_ in fams}
        pg = self.page(*engines.items())
        body = ['<h1>Vangstellingen</h1>', '''
<p>Elke familie heeft een vangstelling voor de drie witte dammen die vastligt <strong>ten opzichte van een hoek of van
de V</strong>. Op dezelfde plaats gehouden kost de omzetting op elk bord precies even veel ply. De sterke partij bouwt
de vangstelling op vanuit een willekeurige stelling en zet daarna snel om: op 9×17 is de langste winst 23 ply, waarvan
hooguit 8 voor de omzetting.</p>
<table class="tbl"><tr><th>Familie</th><th>Witte dammen</th><th>Gecontroleerd</th><th>Zwart aan zet</th><th>Wit aan zet</th></tr>
<tr><td><strong>B + 1</strong></td><td>b2, (L−1, B−2), (L−3, B−4) — bv. 10×11: b2, k9, i7</td><td>6×7 … 11×12</td><td>10</td><td>9</td></tr>
<tr><td><strong>B + 2</strong></td><td>b2, (L−1, B−3), (L−3, B−5) — bv. 10×12: b2, l8, j6</td><td>6×8 … 10×12</td><td>12</td><td>11</td></tr>
<tr><td><strong>2B − 1</strong></td><td>één boven elk been van de V, één onder de top — bv. 10×19: b2, j8, r2</td><td>5×9 … 9×17</td><td>8</td><td>7</td></tr>
</table>
<p>(Langste verlies met zwart aan zet / langste winst met wit aan zet, over elk veld van de zwarte dam waar de partij
aan zet niet kan slaan. Telt men alleen de velden waar geen van beide kan slaan, dan is de langste winst met wit aan zet
9, 9 en 7 ply.) Hieronder per familie het veld van de zwarte dam met de langste winst waar geen van beide kan
slaan.</p>''']
        for gid, w, h, wk, fam, black in fams:
            e = engines[gid]
            if black is None:
                black = self.hardest(e, wk)
            fen = e.fen(wk, [black])
            fig, i0 = pg.diagram(fen, f'{gid.replace("x", "×")}: familie {fam}, wit aan zet', gid=gid)
            v, _ = e.analyse(fen)
            hl, *_ = pg.best_line(fen, 40, start=i0)
            fb = 'B' + fen[1:]
            ib = pg.add(fb, gid=gid)
            vb, _ = e.analyse(fb)
            hb, *_ = pg.best_line(fb, 40, start=ib)
            body += [f'<h2>Familie {fam}: {gid.replace("x", "×")}</h2>', fig,
                     f'<p>Witte dammen op {", ".join(wk)}, de zwarte dam op {black}. Wit aan zet: {v}.</p>',
                     f'<p class="var">{hl}</p>', f'<p>Dezelfde stelling met zwart aan zet: {vb}.</p>',
                     f'<p class="var">{hb}</p>']
            if gid == '10x11':
                body.append('<p>Op 10×11 (diagram 8 van het boek) wint wit met wit aan zet op elk veld van de zwarte dam '
                            'binnen 5 ply, <strong>behalve op h10, het gearceerde veld in het boek, waar het 9 ply '
                            'kost</strong>. De dam op b2 staat op de lange lijn, waarvan het andere eind een ingang '
                            'rechtsboven is; de twee dammen rechtsboven vormen er een halve val mee en de bovenste kan '
                            'de lange lijn afsluiten.</p>')
            if gid == '10x19':
                body.append('<p><strong>De V</strong> (alleen bij <em>L</em> = 2<em>B</em> − 1): de diagonalen vanuit '
                            'de twee hoeken van één lange zijde ontmoeten elkaar precies in het midden van de overkant. '
                            'Korter, en de benen kruisen onder de rand; langer, en ze raken de rand op twee verschillende '
                            'velden. Even <em>B</em> heeft altijd de twee hoeken; oneven <em>B</em> alleen met kleuring '
                            'p1.</p>')
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
                ('10 (i9)', 'b2', 'W', '<strong>W als j10 leeg is</strong>', '(19)', '29.875, alle met een stuk op het geïsoleerde veld j10'),
                ('12 (d8)', 'c7, h4, g3', '?', '<strong>D</strong>', '(35)', '66.365'),
                ('13 (f8)', 'h6, c5, e3', '?', '<strong>D</strong>', '(37)', '67.259'),
                ('14 (h8)', 'c3', 'W', '<strong>W op één zetdwang na</strong>', '33', '2 (één stelling en haar spiegelbeeld)'),
                ('18 (e7)', 'd6, g5, f4', 'D', '<strong>D</strong>', '(35)', '67.660'),
                ('19 (g7)', 'd4', '?', '<strong>W</strong>', '33', '0'),
                ('23 (f6)', 'e5', 'W', '<strong>W</strong>', '35', '0')]
        tbl = ['<table class="tbl"><tr><th>Weg</th><th>Symmetrische velden</th><th>Boek</th><th>Database</th>'
               '<th>Langste winst</th><th>Rustige remises (3 dammen aan zet)</th></tr>']
        tbl += [f'<tr>{"".join(f"<td>{c}</td>" for c in r)}</tr>' for r in rows]
        tbl.append('</table>')
        fen = 'W:WK4,K5,K10:BK15'
        fig, i0 = pg.diagram(fen, '10×10 zonder h8, wit aan zet')
        v, legal = e.analyse(fen)
        mv = ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i0, m)}">1. {m[0].replace("x", "×")}</a> '
                       f'<span class="val">{m[1]}</span>' for m in legal)
        fb = 'B' + fen[1:]
        figb, ib = pg.diagram(fb, '10×10 zonder h8, zwart aan zet')
        vb, _ = e.analyse(fb)
        hb, *_ = pg.best_line(fb, 40, start=ib)
        body = ['<h1>10×10 met één veld weg</h1>',
                '<p>Eén veld per symmetrieklasse (de 4 symmetrieën van het bord geven 15 klassen), 3 dammen tegen 1:</p>',
                '\n'.join(tbl), '''<ul>
<li>Zonder i9 is j10 geïsoleerd: een dam daar kan nooit zetten en nooit geslagen worden.</li>
<li>De enige uitzondering zonder h8 staat hieronder.</li></ul>''',
                '<h2>Zonder h8: de zetdwang</h2>', fig,
                f'<p>Wit op h10, j10 en i9, zwart op j8, wit aan zet: {v}. i9 en j10 zitten vast en elke zet van h10 laat '
                f'een dubbele slag toe (1. h10-g9 j8×h10×f8), en 1 tegen 1 is remise. Alle zetten: {mv}.</p>',
                figb, f'<p>Met zwart aan zet is dezelfde stelling verloren in {vb[1:]} ply:</p>', f'<p class="var">{hb}</p>']
        self.write('tien-zonder-veld.html', '10×10 met één veld weg', '\n'.join(body), pg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--engine', default=str(ROOT / 'build' / 'egdb_fast'))
    ap.add_argument('--out', default=str(ROOT / '_site' / 'drie_tegen_een_is_gemeen'))
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    s = Site(args.engine, out)
    s.index()
    s.twee()
    s.drie()
    s.traps()
    s.lakes()


if __name__ == '__main__':
    main()
