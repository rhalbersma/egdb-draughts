#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Companion site (GitHub Pages) for the kings-only endgames of English checkers: 2 kings against 1 and
3 kings against 2 on boards of every size, the blockades and the double-corner fortress. Every position is
clickable; the values come from egdb_fast --serve --rules checkers.

usage: site.py [--engine build/egdb_fast] [--out _site/checkers]
"""

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / 'common'))
from sitekit import FastEngine, Page, esc, lang_link, menu, write_page   # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
from traps import Race   # noqa: E402

TITLE = {'nl': 'Checkers: 3 dammen tegen 2', 'en': 'Checkers: 3 kings against 2'}

PAGES = [
    ('index.html', 'Overzicht', 'Overview'),
    ('vesting.html', 'De vesting in de dubbele hoek', 'The double-corner fortress'),
]


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

    def engine(self, w, h, p, nw=3, nb=2):
        key = f'{h}x{w}' + ('' if p == 1 else ' p0') + ('' if (nw, nb) == (3, 2) else f' {nw}-{nb}')
        if key not in self.engines:
            self.engines[key] = FastEngine(self.exe, '--width', w, '--height', h, '--parity', p, '--rules', 'checkers',
                                           '--white', nw, '--black', nb)
        return key, self.engines[key]

    def nav(self, current=None):
        items = [(PAGES[k][0], self.title(k)) for k in range(len(PAGES))]
        home = '../index.html' if self.lang == 'en' else '../../nl/index.html'      # English: default, Dutch in nl/
        alt = f'nl/{current}' if self.lang == 'en' else f'../{current}'
        return (f'<a class="brand" href="index.html">{esc(TITLE[self.lang])}</a>'
                f'{menu([(None, items)], current, self.t("Pagina\'s", "Pages"))}'
                f'<a href="{home}">{self.t("Alle sites", "All sites")}</a>{lang_link(self.lang, alt)}')

    def page(self, *boards):
        pg = Page()
        for gid, eng in boards:
            pg.use(gid, eng, eng.g)
        return pg

    def write(self, name, title, body, pg, gid=None):
        if gid is not None:              # algebraic moves in the running text follow the notation switch
            body = pg.prose_html(body, gid, 'alg')
        k = [p[0] for p in PAGES].index(name)
        prev = f'<a href="{PAGES[k - 1][0]}">← {esc(self.title(k - 1))}</a>' if k > 0 else '<span></span>'
        nxt = f'<a href="{PAGES[k + 1][0]}">{esc(self.title(k + 1))} →</a>' if k + 1 < len(PAGES) else '<span></span>'
        body += f'\n<nav class="pager">{prev}{nxt}</nav>'
        write_page(self.out / name, self.title(k), self.nav(name), body, pg, TITLE[self.lang], self.lang)

    @staticmethod
    def plan(text):
        out = []
        for tok in text.split():
            m = re.fullmatch(r'([a-z]\d+)([-x])([a-z]\d+)([!?]*)', tok)
            if m:
                out.append(((f'{m.group(1)}{m.group(2)}{m.group(3)}',), m.group(4)))
        return out

    @staticmethod
    def moves_list(pg, i0, legal, prefix):
        return ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i0, m)}">{prefix}{pg.label(pg.pos[i0]['g'], m)}</a> '
                         f'<span class="val">{m[1]}</span>' for m in legal)

    @staticmethod
    def longest_21(e):
        """the quiet 2-1 position with white to move and the longest win"""
        from itertools import combinations
        name = {n: f'{chr(97 + f)}{r + 1}' for n, (f, r) in e.g['sq'].items()}
        best = None
        for wk in combinations(sorted(name), 2):
            for b in name:
                if b in wk:
                    continue
                fen = e.fen([name[x] for x in wk], [name[b]])
                v, legal = e.analyse(fen)
                if v[0] != 'W' or any('x' in m[0] for m in legal):
                    continue
                if any('x' in m[0] for m in e.analyse('B' + fen[1:])[1]):
                    continue
                if best is None or int(v[1:]) > best[0]:
                    best = (int(v[1:]), fen)
        return best[1]

    # ---- pages -------------------------------------------------------------------

    def index(self):
        k21, e21 = self.engine(8, 8, 1, 2, 1)
        k78, e78 = self.engine(8, 7, 1)
        k88, e88 = self.engine(8, 8, 1)
        pg = self.page((k21, e21), (k88, e88), (k78, e78))
        t = self.t
        body = [f'<h1>{esc(TITLE[self.lang])}</h1>', t('''
<p>Eindspelen met alleen dammen in het Engelse damspel (<em>checkers</em>): korte dammen (één veld), slagplicht en
vrije keuze tussen slagen. Op borden van elke afmeting, exact uitgerekend met
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>.
Waarden in ply voor de partij aan zet: W = winst, L = verlies, D = remise; zuiver speltheoretisch, zonder
zettenregels. Elke stelling is klikbaar.</p>''', '''
<p>Endgames with kings only in English checkers: short kings (one square), mandatory capture and free choice among
captures. On boards of any size, computed exactly with
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>. Values in
plies for the side to move: W = win, L = loss, D = draw; purely game-theoretic, without move-count rules. Every position
is clickable.</p>''')]
        fen = self.longest_21(e21)
        v, _ = e21.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'8×8, 2 tegen 1, wit aan zet: {v}', f'8×8, 2 against 1, white to move: {v}'), gid=k21)
        h, *_ = pg.best_line(fen, 60, start=i0)
        body += [t('<h2>2 dammen tegen 1</h2>', '<h2>2 kings against 1</h2>'), fig, t('''
<p>Remise op 4×4. Op elk groter bord gewonnen, op een vast handjevol stellingen na (8 of 10, onafhankelijk van de
bordgrootte) waarin de twee witte dammen opgesloten zitten in de enkele hoek en de zwarte dam ze daar houdt met
oppositie. Op oneven × oneven borden met speelbare hoeken zijn er geen uitzonderingen. De langste winst groeit
gestaag: 33 ply op 8×8, 41 op 10×10, 49 op 12×12, 61 op 12×15.</p>''', '''
<p>Drawn on 4×4. On every larger board it is won, except for a fixed handful of positions (8 or 10, independent of the
board size) in which the two white kings are locked in the single corner and the black king keeps them there by
opposition. On odd × odd boards with playable corners there are no exceptions at all. The longest win grows steadily:
33 plies on 8×8, 41 on 10×10, 49 on 12×12, 61 on 12×15.</p>'''),
                 t(f'<p>De langste winst op 8×8 ({v[1:]} ply):</p>', f'<p>The longest win on 8×8 ({v[1:]} plies):</p>'),
                 f'<p class="var">{h}</p>',
                 '<div style="clear:both"></div>']
        body.append(t('''<h2>3 dammen tegen 2</h2>
<table class="tbl"><tr><th>Bord</th><th>Rustige remises (3 dammen aan zet)</th><th>Langste winst</th>
<th>Zwart in beide dubbele hoeken: remises</th></tr>
<tr><td>6×7</td><td>273</td><td>63</td><td>0%</td></tr>
<tr><td>7×8</td><td>792 (0,16%)</td><td>75</td><td>0%</td></tr>
<tr><td>8×8</td><td>1.252 (0,11%)</td><td>67</td><td>0%</td></tr>
<tr><td>8×9</td><td>87.670 (4,0%)</td><td>79</td><td>50% (oppositie)</td></tr>
<tr><td>9×10</td><td>344.907 (4,4%)</td><td>93</td><td>50% (oppositie)</td></tr>
<tr><td>8×10</td><td>288.748</td><td>97</td><td>~100%</td></tr>
<tr><td>10×10</td><td>1.012.242 (7,1%)</td><td>115</td><td>~100%</td></tr>
<tr><td>10×11</td><td>1.810.258 (7,5%)</td><td>109</td><td>~100%</td></tr>
<tr><td>10×12</td><td>2.744.990</td><td>127</td><td>~100%</td></tr>
<tr><td>7×9 p0 (vier dubbele hoeken)</td><td>124.148 (13,2%)</td><td>93</td><td>zie <a href="vesting.html">vesting</a></td></tr>
<tr><td>9×9 p0 (vier dubbele hoeken)</td><td>1.201.460 (29,5%)</td><td>99</td><td>zie <a href="vesting.html">vesting</a></td></tr>
</table>''', '''<h2>3 kings against 2</h2>
<table class="tbl"><tr><th>Board</th><th>Quiet draws (3 kings to move)</th><th>Longest win</th>
<th>Black in both double corners: draws</th></tr>
<tr><td>6×7</td><td>273</td><td>63</td><td>0%</td></tr>
<tr><td>7×8</td><td>792 (0.16%)</td><td>75</td><td>0%</td></tr>
<tr><td>8×8</td><td>1,252 (0.11%)</td><td>67</td><td>0%</td></tr>
<tr><td>8×9</td><td>87,670 (4.0%)</td><td>79</td><td>50% (opposition)</td></tr>
<tr><td>9×10</td><td>344,907 (4.4%)</td><td>93</td><td>50% (opposition)</td></tr>
<tr><td>8×10</td><td>288,748</td><td>97</td><td>~100%</td></tr>
<tr><td>10×10</td><td>1,012,242 (7.1%)</td><td>115</td><td>~100%</td></tr>
<tr><td>10×11</td><td>1,810,258 (7.5%)</td><td>109</td><td>~100%</td></tr>
<tr><td>10×12</td><td>2,744,990</td><td>127</td><td>~100%</td></tr>
<tr><td>7×9 p0 (four double corners)</td><td>124,148 (13.2%)</td><td>93</td><td>see <a href="vesting.html">fortress</a></td></tr>
<tr><td>9×9 p0 (four double corners)</td><td>1,201,460 (29.5%)</td><td>99</td><td>see <a href="vesting.html">fortress</a></td></tr>
</table>'''))
        for fen, head, cap in (
                ('W:WK4,K8,K29:BK1,K28', t('<h2>3 tegen 2 op 8×8: zwart in de dubbele hoeken</h2>',
                                           '<h2>3 against 2 on 8×8: black in the double corners</h2>'),
                 t('<p>De beste verdediging lijkt de twee zwarte dammen elk in een dubbele hoek te zetten: een dam op de twee '
                   'randvelden van een dubbele hoek kan niet geslagen worden en heeft altijd een wachtzet (zie de '
                   '<a href="vesting.html">vesting</a>). Op 8×8 helpt het niet: alle 13.320 rustige stellingen met zwart in '
                   'beide dubbele hoeken zijn gewonnen, met wit aan zet én met zwart aan zet. De oppositie, die op 8×9 en '
                   '9×10 precies de helft van zulke stellingen remise maakt, speelt op 8×8 dus geen rol. Hier liggen de twee '
                   'dubbele hoeken diagonaal tegenover elkaar en dicht bij elkaar (in de afstandsregel van de vesting: '
                   'D × d = 7 × 7, de vesting valt).</p><p>De langste van deze stellingen, met de witte dammen in de enkele '
                   'hoeken, duurt {v} ply. Op zet 10 zet wit een dam op het tweede veld van de hoek linksboven: de zwarte '
                   'dam daar kan niet meer pendelen, en zwart moet met de andere dam zetten. Op zet 18 dwingt wit met een '
                   'offer een ruil af: zwart moet slaan, wit slaat terug, en 2 tegen 1 wint.</p>',
                   '<p>The best defence seems to be to put each black king in a double corner: a king on the two edge '
                   'squares of a double corner cannot be captured and always has a waiting move (see the '
                   '<a href="vesting.html">fortress</a>). On 8×8 it does not help: all 13,320 quiet positions with black in '
                   'both double corners are won, with white to move and with black to move. The opposition, which makes '
                   'exactly half of such positions a draw on 8×9 and 9×10, plays no role on 8×8. Here the two double '
                   'corners lie diagonally opposite and close together (in the distance rule of the fortress: '
                   'D × d = 7 × 7, the fortress falls).</p><p>The longest of these positions, with the white kings in the '
                   'single corners, takes {v} plies. On move 10 white puts a king on the second square of the top-left '
                   'corner: the black king there can no longer shuttle, and black has to move the other king. On move 18 '
                   'white forces an exchange with a sacrifice: black has to capture, white captures back, and 2 against 1 '
                   'wins.</p>')),
                ('W:WK3,K4,K30:BK11,K13', t('<h2>3 tegen 2 op 8×8: de langste winst</h2>',
                                            '<h2>3 against 2 on 8×8: the longest win</h2>'),
                 t('<p>De langste winst op 8×8 zonder slag voor beide kanten in de beginstelling: {v} ply.</p>',
                   '<p>The longest win on 8×8 without a capture for either side in the start position: {v} plies.</p>'))):
            v, _ = e88.analyse(fen)
            fig, i0 = pg.diagram(fen, t(f'8×8, wit aan zet: {v}', f'8×8, white to move: {v}'), gid=k88)
            h, *_ = pg.best_line(fen, 80, start=i0)
            body += [head, fig, cap.format(v=v[1:]), f'<p class="var">{h}</p>', '<div style="clear:both"></div>']
            if fen == 'W:WK4,K8,K29:BK1,K28':
                body += self.traps88(pg, k88, e88)
        fen = 'W:WK1,K2,K3:BK10,K11'
        fig, i0 = pg.diagram(fen, t('7×8, wit aan zet', '7×8, white to move'), gid=k78)
        v, legal = e78.analyse(fen)
        i1 = pg.child(i0, [m for m in legal if m[0] == 'c7-b6'][0])
        f1 = pg.pos[i1]['f']
        _, legal1 = e78.analyse(f1)
        body += [t('<h2>Kleine borden: blokkades</h2>', '<h2>Small boards: blockades</h2>'), fig, t('''
<p>Op 7×8 en 8×8 is 3 tegen 2 in het algemeen gewonnen. De weinige remises ontstaan bijna alleen als wit tegen de rand
gedrukt wordt (0,0% remise zonder witte dam aan de rand, 0,5% met drie), en zwart in beide dubbele hoeken maakt nooit
remise.</p>''', '''
<p>On 7×8 and 8×8 3-2 is won generically. The few draws occur almost only when white is pushed against the edge (draw
rate 0.0% with no white king on the edge, 0.5% with three), and black holding both double corners never draws.</p>'''),
                 t(f'<p>In deze stelling op 7×8 ({v}) verliezen vier van de vijf witte zetten meteen een dam '
                   f'(a7-b6 c5×a7, c7-d6 e5×c7, e7-d6 c5×e7, e7-f6 e5×g7): ',
                   f'<p>In this position on 7×8 ({v}) four of the five white moves lose a king at once '
                   f'(a7-b6 c5×a7, c7-d6 e5×c7, e7-d6 c5×e7, e7-f6 e5×g7): ') + f'{self.moves_list(pg, i0, legal, "1. ")}.</p>',
                 t(f'<p>Na <a class="m1" href="#" data-i="{i1}">1. c7-b6</a> moet zwart de enige remisezet vinden; alles '
                   f'andere verliest: ', f'<p>After <a class="m1" href="#" data-i="{i1}">1. c7-b6</a> black has to find '
                   f'the only drawing move; everything else loses: ') + f'{self.moves_list(pg, i1, legal1, "1. … ")}.</p>',
                 t('<p>Na 1. … e5-d4 kan wit zijn dammen eeuwig houden maar breekt nooit uit: zwart kan in geen van de '
                   '901.577 stellingen die zonder slag bereikbaar zijn een slag afdwingen.</p>',
                   '<p>After 1. … e5-d4 white can keep its kings forever but never breaks out: black cannot force a '
                   'capture in any of the 901,577 positions reachable without captures.</p>')]
        self.write('index.html', 'Overzicht', '\n'.join(body), pg, k78)

    @staticmethod
    def exchange(e, fen):
        """white to move: a quiet offer after which black must capture and white captures back (3-2 -> 2-1), as the
        moves (offer, capture, recapture), or None"""
        count = lambda f: tuple(len([x for x in side[1:].split(',') if x]) for side in f.split(':')[1:])
        for off in e.analyse(fen)[1]:
            if count(off[2]) != (3, 2):
                continue
            replies = e.analyse(off[2])[1]
            if not replies or any(count(r[2]) != (2, 2) for r in replies):
                continue
            backs = [next((m for m in e.analyse(r[2])[1] if count(m[2])[1] < 2), None) for r in replies]
            if all(backs):
                return off, replies[0], backs[0]
        return None

    def traps88(self, pg, k88, e88):
        """the trap in both phases on 8x8: every shuttle move of black allows a forced exchange"""
        t = self.t
        body = [t('<h2>3 tegen 2 op 8×8: de val in beide fasen</h2>', '<h2>3 against 2 on 8×8: the trap in both phases</h2>'),
                t('<p>Wit breekt de dubbele hoeken met een val: een opstelling waarin elke pendelzet van zwart een '
                  'gedwongen ruil toelaat: met een rustige zet dwingt wit zwart een witte dam te slaan, wit slaat terug, en 2 tegen 1 wint. Zwart '
                  'heeft twee dammen en dus twee pendelzetten; de val moet ze allebei straffen, ook als de ruil de andere zwarte dam treft. Of zo\'n val werkt, hangt '
                  'af van de fase: de pariteit van de som van de rijen van alle dammen, met zwart aan zet. Die verandert '
                  'zonder slag nooit, want dammen kunnen geen tempo verliezen; dat is de oppositie. De database telt de '
                  'vallen met zwart aan zet en beide zwarte dammen op de randvelden van twee dubbele hoeken:</p>',
                  '<p>White breaks the double corners with a trap: a set-up in which every shuttle move of black allows a '
                  'forced exchange: with a quiet move white forces black to capture a white king, white captures back, and 2 against 1 wins. '
                  'Black has two kings and therefore two shuttle moves; the trap has to punish both, even when the exchange hits the other black king. Whether such a trap '
                  'works depends on the phase: the parity of the sum of the ranks of all kings, with black to move. It '
                  'never changes without a capture, since kings cannot lose a tempo; that is the opposition. The database '
                  'counts the traps with black to move and both black kings on the edge squares of two double corners:</p>'),
                t('<table class="tbl"><tr><th>Bord</th><th>Vallen in fase 0</th><th>Vallen in fase 1</th><th>Gevolg</th></tr>'
                  '<tr><td>8×8</td><td>3</td><td>3</td><td>de vesting valt altijd</td></tr>'
                  '<tr><td>8×9, 9×10</td><td>3</td><td>0</td><td>de oppositie beslist: de helft remise</td></tr>'
                  '<tr><td>10×10</td><td>0</td><td>0</td><td>de vesting houdt</td></tr></table>',
                  '<table class="tbl"><tr><th>Board</th><th>Traps in phase 0</th><th>Traps in phase 1</th><th>Consequence</th></tr>'
                  '<tr><td>8×8</td><td>3</td><td>3</td><td>the fortress always falls</td></tr>'
                  '<tr><td>8×9, 9×10</td><td>3</td><td>0</td><td>the opposition decides: half are draws</td></tr>'
                  '<tr><td>10×10</td><td>0</td><td>0</td><td>the fortress holds</td></tr></table>'),
                t('<p>Op 8×8 heeft wit dus in elke fase een val en kiest hij de val die bij de oppositie past. In de vallen '
                  'staan de drie witte dammen op een van de twee lange diagonalen die de dubbele hoeken verbinden, tussen '
                  'de zwarte dammen in; welke diagonaal, bepaalt de fase. Alleen op 8×8 lopen beide diagonalen van de ene '
                  'dubbele hoek naar de andere. De tellingen komen uit '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>.</p>',
                  '<p>On 8×8 white therefore has a trap in each phase and picks the one that fits the opposition. In the '
                  'traps the three white kings stand on one of the two long diagonals that connect the double corners, '
                  'between the black kings; which diagonal decides the phase. Only on 8×8 do both diagonals run from one '
                  'double corner to the other. The counts come from '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>.</p>')]
        for fen, ph in (('B:WK14,K18,K23:BK5,K32', 1), ('B:WK6,K15,K24:BK5,K32', 0)):
            v, legal = e88.analyse(fen)
            fig, i0 = pg.diagram(fen, t(f'8×8, zwart aan zet (fase {ph}): {v}', f'8×8, black to move (phase {ph}): {v}'),
                                 gid=k88)
            lines = []
            for m in legal:
                nb = [int(x.lstrip('K')) for x in m[2].split(':')[2][1:].split(',') if x]
                if not set(nb) & {1, 28}:          # only the shuttle moves 5-1 and 32-28
                    continue
                ex = self.exchange(e88, m[2])
                i1 = pg.child(i0, m)
                parts = [f'<a class="m1" href="#" data-i="{i1}">1. … {pg.label(k88, m)}</a>']
                i = i1
                for k, mv in enumerate(ex):
                    i = pg.child(i, mv)
                    pre = '2. ' if k == 0 else ('3. ' if k == 2 else '')
                    parts.append(f'<a class="m1" href="#" data-i="{i}">{pre}{pg.label(k88, mv)}</a>')
                lines.append(' '.join(parts))
            body += [fig, t('<p>Elke pendelzet van zwart wordt gestraft: ', '<p>Every shuttle move of black is punished: ')
                     + '; '.join(lines) + t(', en 2 tegen 1 wint. Alle zetten van zwart: ',
                                            ', and 2 against 1 wins. All moves of black: ')
                     + self.moves_list(pg, i0, legal, '1. … ') + '.</p>', '<div style="clear:both"></div>']
        # why both phases: the diagonal of white decides the phase, and moving over to the other diagonal costs three
        # moves (one per king), an odd number (no "op <number>" or algebraic squares in this prose: the page converts
        # those on the 7x8 board)
        ms = {m[3]: m for m in e88.analyse('W:WK14,K18,K23:BK5,K32')[1]}
        shift = ', '.join(pg.label(k88, ms[x]) for x in ('14-10', '18-15', '23-19'))
        body += [t('<h3>Waarom wit in beide fasen een val heeft</h3>', '<h3>Why white has a trap in both phases</h3>'), t(
            '<p>Op welke diagonaal zwart staat, maakt niet uit: met de witte dammen 14, 18 en 23 is er een val tegen de '
            'zwarte dammen 5 en 32, dus op dezelfde diagonaal, en ook een tegen 1 en 28, op de andere. Wat de fase '
            'bepaalt, is de diagonaal van wit:</p>',
            '<p>Which diagonal black stands on does not matter: with the white kings 14, 18 and 23 there is a trap against '
            'the black kings 5 and 32, on the same diagonal, and also one against 1 and 28, on the other. What decides the '
            'phase is the diagonal of white:</p>'),
            t('<table class="tbl"><tr><th>Zwart</th><th>Wit op diagonaal 5…32: fase 1</th><th>Wit op diagonaal 1…28: fase 0</th></tr>'
              '<tr><td>5 en 32</td><td>14, 18, 23</td><td>10, 15, 19 · 6, 15, 24</td></tr>'
              '<tr><td>1 en 28</td><td>14, 18, 23 · 9, 18, 27</td><td>10, 15, 19</td></tr></table>',
              '<table class="tbl"><tr><th>Black</th><th>White on diagonal 5…32: phase 1</th><th>White on diagonal 1…28: phase 0</th></tr>'
              '<tr><td>5 and 32</td><td>14, 18, 23</td><td>10, 15, 19 · 6, 15, 24</td></tr>'
              '<tr><td>1 and 28</td><td>14, 18, 23 · 9, 18, 27</td><td>10, 15, 19</td></tr></table>'),
            t(f'<p>Van de val met 14, 18, 23 naar die met 10, 15, 19 gaat elke witte dam één stap naar de andere diagonaal: '
              f'{shift}. Dat zijn drie zetten, een oneven aantal: het tempo draait om, en wit staat in de andere fase. Met '
              f'twee dammen zou zo\'n overstap twee zetten kosten, en dan wisselt het tempo niet.</p>',
              f'<p>From the trap with 14, 18, 23 to the one with 10, 15, 19 every white king takes one step to the other '
              f'diagonal: {shift}. That is three moves, an odd number: the tempo turns over, and white is in the other phase. '
              f'With two kings such a change-over would take two moves, and then the tempo does not change.</p>')]
        body += self.race88(pg, k88, e88)
        return body

    def race88(self, pg, k88, e88):
        """how fast white forces a trap on 8x8 against the longest defence inside the double corners"""
        t = self.t
        if not hasattr(e88, 'race'):            # computed once for both languages
            e88.race = Race(e88)
        r = e88.race
        dist = sorted(r.distribution().items())
        x = r.farthest()
        fen = r.fen(x[0], x[1:], 'W')
        n = r.wtm[x]
        rows = ''.join(f'<td>{k}</td>' for k, _ in dist)
        cnt = ''.join(f'<td>{c:,}</td>'.replace(',', '.' if self.lang == 'nl' else ',') for _, c in dist)
        body = [t('<h3>Kan wit de val altijd opbouwen?</h3>', '<h3>Can white always build the trap?</h3>'), t(
            f'<p>Ja, zolang zwart in zijn dubbele hoeken blijft. Vanuit elke rustige stelling met zwart in beide dubbele '
            f'hoeken en wit aan zet dwingt wit in beide fasen een val af, tegen elke pendelverdediging, in hoogstens {n} '
            f'zetten. Gerekend is het spel waarin zwart alleen pendelt en wit rustige zetten doet die zwart geen slag geven; '
            f'een pendelzet die wit een slag geeft, kost zwart meteen een dam. Het aantal witte zetten tot de val:</p>',
            f'<p>Yes, as long as black stays in its double corners. From every quiet position with black in both double '
            f'corners and white to move, white forces a trap in both phases, against every shuttling defence, in at most '
            f'{n} moves. The game computed: black only shuttles, and white makes quiet moves that leave black no capture; a '
            f'shuttle move that gives white a capture costs black a king at once. The number of white moves to the trap:</p>'),
            f'<table class="tbl"><tr><th>{t("Zetten", "Moves")}</th>{rows}</tr><tr><th>{t("Stellingen", "Positions")}</th>'
            f'{cnt}</tr></table>']
        fig, i0 = pg.diagram(fen, t(f'8×8, wit aan zet: de val na {n} zetten', f'8×8, white to move: the trap after {n} moves'),
                             gid=k88)
        parts, i, num = [], i0, 1
        for k, m in enumerate(r.line(x)):
            i = pg.child(i, m)
            pre = f'{num}. ' if k % 2 == 0 else ''
            num += k % 2
            parts.append(f'<a class="m1" href="#" data-i="{i}">{pre}{pg.label(k88, m)}</a>')
        body += [fig, t('<p>Het langste verweer: zwart pendelt zo lang mogelijk, wit bouwt de val zo snel mogelijk op en '
                        'dwingt na de laatste pendelzet de ruil af.</p>',
                        '<p>The longest defence: black shuttles as long as possible, white builds the trap as fast as '
                        'possible and forces the exchange after the last shuttle move.</p>'),
                 f'<p class="var">{" ".join(parts)}</p>',
                 t('<p>Zwart kan ook zelf een hoek verlaten. Volgens de database is dat vaak even goed als pendelen, en '
                   'meestal kan zwart later ook weer terug in de hoek, zoals de zwarte dam rechtsonder in de hoofdvariant '
                   'van het eerste diagram (zet 10 tot en met 13). Het redt niets: alle stellingen blijven verloren.</p>',
                   '<p>Black can also leave a corner itself. According to the database that is often just as good as '
                   'shuttling, and usually black can later get back into the corner, as the black king at the bottom right '
                   'does in the main line of the first diagram (moves 10 to 13). It saves nothing: every position stays '
                   'lost.</p>'), '<div style="clear:both"></div>']
        return body

    def vesting(self):
        k89, e89 = self.engine(9, 8, 1)
        k99, e99 = self.engine(9, 9, 0)
        pg = self.page((k89, e89), (k99, e99))
        fen = e89.fen(['c7', 'e7', 'g7'], ['a7', 'i7'])
        t = self.t
        fig, i0 = pg.diagram(fen, t('8×9, wit aan zet', '8×9, white to move'), gid=k89)
        v, legal = e89.analyse(fen)
        fb = 'B' + fen[1:]
        ib = pg.add(fb, gid=k89)
        vb, _ = e89.analyse(fb)
        hb, *_ = pg.moves(fb, self.plan('1. a7-b8 2. g7-f8! b8xd6 3. e7xc5'), 1, True, 'm1', ib)
        hb2, *_ = pg.best_line(pg.pos[int(re.findall(r'data-i="(\d+)"', hb)[-1])]['f'], 40,
                               start=int(re.findall(r'data-i="(\d+)"', hb)[-1]), number=3)
        f2 = e89.fen(['d6', 'e7', 'g7'], ['a7', 'i7'])
        fig2, i2 = pg.diagram(f2, t('8×9, wit aan zet', '8×9, white to move'), gid=k89)
        v2, legal2 = e89.analyse(f2)
        h2, *_ = pg.moves(f2, self.plan('1. d6-c7!'), 1, False, 'm1', i2)
        f9 = e99.fen(['a2', 'c2', 'd3'], ['b1', 'i8'])
        fig9, i9 = pg.diagram(f9, t('9×9 p0, wit aan zet', '9×9 p0, white to move'), gid=k99)
        v9, legal9 = e99.analyse(f9)
        body = [t('<h1>De vesting in de dubbele hoek</h1>', '<h1>The double-corner fortress</h1>'), t('''
<p>Een dam op de twee randvelden van een dubbele hoek kan nooit geslagen worden en heeft altijd een wachtzet. Op 8×9 en
9×10 maakt een zwarte dam in elke dubbele hoek remise in precies de helft van de stellingen, en welke helft dat is,
beslist de <strong>oppositie</strong>: de pariteit van de som van de rijen van alle stukken wisselt bij elke zet, en
dammen kunnen geen tempo verliezen, dus wie de oppositie heeft, ligt vanaf het begin vast.</p>
<table class="tbl"><tr><th>Zwart in beide dubbele hoeken</th><th>Pariteit 0</th><th>Pariteit 1</th></tr>
<tr><td>8×9</td><td>99,5% remise</td><td>0,0% remise</td></tr>
<tr><td>9×10</td><td>99,8%</td><td>0,0%</td></tr>
<tr><td>10×11</td><td>99,9%</td><td>99,9%</td></tr></table>''', '''
<p>A king on the two edge squares of a double corner can never be captured and always has a waiting move. On 8×9 and
9×10 a black king in each double corner draws in exactly half of the positions, and that half is decided by
<strong>opposition</strong>: the parity of the sum of the ranks of all pieces flips with every move, and kings cannot
lose a tempo, so who has the opposition is fixed from the start.</p>
<table class="tbl"><tr><th>Black in both double corners</th><th>Parity 0</th><th>Parity 1</th></tr>
<tr><td>8×9</td><td>99.5% draws</td><td>0.0% draws</td></tr>
<tr><td>9×10</td><td>99.8%</td><td>0.0%</td></tr>
<tr><td>10×11</td><td>99.9%</td><td>99.9%</td></tr></table>'''),
                t('<h2>8×9: oppositie</h2>', '<h2>8×9: opposition</h2>'), fig,
                t(f'<p>Zwart op a7 en i7 (pendelend met b8 en h8), wit op c7, e7 en g7. Met wit aan zet ({v}) houdt elke '
                  f'zet remise: zwart pendelt gewoon a7-b8-a7. ',
                  f'<p>Black on a7 and i7 (shuttling with b8 and h8), white on c7, e7 and g7. With white to move ({v}) every '
                  f'move draws: black just shuttles a7-b8-a7. ') + f'{self.moves_list(pg, i0, legal, "1. ")}.</p>',
                t(f'<p>Met zwart aan zet staat zwart in zetdwang en verliest in {vb[1:]} ply: {hb} en 2 tegen 1 wint: ',
                  f'<p>With black to move black is in zugzwang and loses in {vb[1:]} plies: {hb} and 2-1 wins: ')
                + f'{hb2}</p>', '<div style="clear:both"></div>', fig2,
                t(f'<p>Met de witte dam op d6 in plaats van c7 ({v2}) wint wit met {h2}, en bereikt zo dezelfde stelling met '
                  f'zwart aan zet.</p>', f'<p>With the white king on d6 instead of c7 ({v2}) white wins with {h2}, reaching '
                  f'the same position with black to move.</p>'), '<div style="clear:both"></div>', t('''
<h2>De afstandsregel</h2>
<p>Wat beslist, is de afstand tussen de hoeken van de dubbele hoeken die zwart bezet (<em>D</em> = grootste,
<em>d</em> = kleinste coördinaatverschil; <em>d</em> = 0 als ze aan dezelfde kant liggen), niet of ze aan dezelfde
kant liggen:</p>
<table class="tbl"><tr><th>Borden</th><th>D × d</th><th>Vesting</th></tr>
<tr><td>6×7, 7×8, 7×9 p0 (verticaal paar)</td><td>6 × 0</td><td>valt</td></tr>
<tr><td>8×8</td><td>7 × 7</td><td>valt</td></tr>
<tr><td>8×9, 9×10, 9×9 p0 (zelfde kant), 7×9 p0 (horizontaal)</td><td>8 × 0</td><td>oppositie beslist</td></tr>
<tr><td>7×9 p0 (diagonaal paar)</td><td>8 × 6</td><td>oppositie beslist</td></tr>
<tr><td>9×9 p0 (diagonaal paar)</td><td>8 × 8</td><td>houdt</td></tr>
<tr><td>8×10, 10×10, 10×11, 10×12</td><td>≥ 9</td><td>houdt</td></tr></table>
<p><em>D</em> ≤ 7: de vesting valt · <em>D</em> = 8: de oppositie beslist, tenzij ook <em>d</em> = 8 ·
<em>D</em> ≥ 9: de vesting houdt.</p>''', '''
<h2>The distance rule</h2>
<p>What decides is the distance between the corners of the double corners black occupies (<em>D</em> = largest,
<em>d</em> = smallest coordinate difference; <em>d</em> = 0 when they are on the same side), not whether they lie on the
same side:</p>
<table class="tbl"><tr><th>Boards</th><th>D × d</th><th>Fortress</th></tr>
<tr><td>6×7, 7×8, 7×9 p0 (vertical pair)</td><td>6 × 0</td><td>falls</td></tr>
<tr><td>8×8</td><td>7 × 7</td><td>falls</td></tr>
<tr><td>8×9, 9×10, 9×9 p0 (same side), 7×9 p0 (horizontal)</td><td>8 × 0</td><td>opposition decides</td></tr>
<tr><td>7×9 p0 (diagonal pair)</td><td>8 × 6</td><td>opposition decides</td></tr>
<tr><td>9×9 p0 (diagonal pair)</td><td>8 × 8</td><td>holds</td></tr>
<tr><td>8×10, 10×10, 10×11, 10×12</td><td>≥ 9</td><td>holds</td></tr></table>
<p><em>D</em> ≤ 7: the fortress falls · <em>D</em> = 8: opposition decides, unless also <em>d</em> = 8 ·
<em>D</em> ≥ 9: the fortress holds.</p>'''),
                t('<h2>9×9 p0: één hoek bevriezen kost drie dammen</h2>', '<h2>9×9 p0: freezing one corner takes three kings</h2>'),
                fig9,
                t(f'<p>Op 9×9 p0 kost het bevriezen van één hoek al alle drie de witte dammen (één op het pendelveld, één die '
                  f'de uitgang blokkeert, één die de blokkeerder dekt), dus de andere zwarte dam pendelt gewoon ({v9}): ',
                  f'<p>On 9×9 p0, freezing one corner already takes all three white kings (one on the shuttle square, one '
                  f'blocking the exit, one guarding the blocker), so the other black king simply shuttles ({v9}): ')
                + f'{self.moves_list(pg, i9, legal9, "1. ")}.</p>']
        self.write('vesting.html', 'De vesting in de dubbele hoek', '\n'.join(body), pg, k89)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--engine', default=str(ROOT / 'build' / 'egdb_fast'))
    ap.add_argument('--out', default=str(ROOT / '_site' / 'checkers'))
    args = ap.parse_args()
    engines = {}
    for lang in ('en', 'nl'):
        out = Path(args.out) / ('' if lang == 'en' else 'nl')
        out.mkdir(parents=True, exist_ok=True)
        s = Site(args.engine, out, lang, engines)
        s.index()
        s.vesting()


if __name__ == '__main__':
    main()
