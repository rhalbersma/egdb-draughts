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
from sitekit import FastEngine, Page, esc, write_page   # noqa: E402

TITLE = 'Checkers: 3 dammen tegen 2'

PAGES = [
    ('index.html', 'Overzicht'),
    ('vesting.html', 'De vesting in de dubbele hoek'),
]


class Site:
    def __init__(self, exe, out):
        self.exe = exe
        self.out = out
        self.engines = {}

    def engine(self, w, h, p, nw=3, nb=2):
        key = f'{h}x{w}' + ('' if p == 1 else ' p0') + ('' if (nw, nb) == (3, 2) else f' {nw}-{nb}')
        if key not in self.engines:
            self.engines[key] = FastEngine(self.exe, '--width', w, '--height', h, '--parity', p, '--rules', 'checkers',
                                           '--white', nw, '--black', nb)
        return key, self.engines[key]

    def nav(self):
        links = ''.join(f'<a href="{f}">{esc(t)}</a>' for f, t in PAGES[1:])
        return f'<a href="../index.html">egdb-draughts</a><a href="index.html">{esc(TITLE)}</a>' + links

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
        out = []
        for tok in text.split():
            m = re.fullmatch(r'([a-z]\d+)([-x])([a-z]\d+)([!?]*)', tok)
            if m:
                out.append(((f'{m.group(1)}{m.group(2)}{m.group(3)}',), m.group(4)))
        return out

    @staticmethod
    def moves_list(pg, i0, legal, prefix):
        return ', '.join(f'<a class="m1" href="#" data-i="{pg.child(i0, m)}">{prefix}{m[0].replace("x", "×")}</a> '
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
        pg = self.page((k21, e21), (k78, e78))
        body = [f'<h1>{esc(TITLE)}</h1>', '''
<p>Eindspelen met alleen dammen in het Engelse damspel (<em>checkers</em>): korte dammen (één veld), slagplicht en
vrije keuze tussen slagen. Op borden van elke afmeting, exact uitgerekend met
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>.
Waarden in ply voor de partij aan zet: W = winst, L = verlies, D = remise; zuiver speltheoretisch, zonder
zettenregels. Elke stelling is klikbaar.</p>''']
        fen = self.longest_21(e21)
        v, _ = e21.analyse(fen)
        fig, i0 = pg.diagram(fen, f'8×8, 2 tegen 1, wit aan zet: {v}', gid=k21)
        h, *_ = pg.best_line(fen, 60, start=i0)
        body += ['<h2>2 dammen tegen 1</h2>', fig, '''
<p>Remise op 4×4. Op elk groter bord gewonnen, op een vast handjevol stellingen na (8 of 10, onafhankelijk van de
bordgrootte) waarin de twee witte dammen opgesloten zitten in de enkele hoek en de zwarte dam ze daar houdt met
oppositie. Op oneven × oneven borden met speelbare hoeken zijn er geen uitzonderingen. De langste winst groeit
gestaag: 33 ply op 8×8, 41 op 10×10, 49 op 12×12, 61 op 12×15.</p>''',
                 f'<p>De langste winst op 8×8 ({v[1:]} ply):</p>', f'<p class="var">{h}</p>',
                 '<div style="clear:both"></div>']
        body.append('''<h2>3 dammen tegen 2</h2>
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
</table>''')
        fen = 'W:WK1,K2,K3:BK10,K11'
        fig, i0 = pg.diagram(fen, '7×8, wit aan zet', gid=k78)
        v, legal = e78.analyse(fen)
        i1 = pg.child(i0, [m for m in legal if m[0] == 'c7-b6'][0])
        f1 = pg.pos[i1]['f']
        _, legal1 = e78.analyse(f1)
        body += ['<h2>Kleine borden: blokkades</h2>', fig, '''
<p>Op 7×8 en 8×8 is 3 tegen 2 in het algemeen gewonnen. De weinige remises ontstaan bijna alleen als wit tegen de rand
gedrukt wordt (0,0% remise zonder witte dam aan de rand, 0,5% met drie), en zwart in beide dubbele hoeken maakt nooit
remise.</p>''', f'<p>In deze stelling op 7×8 ({v}) verliezen vier van de vijf witte zetten meteen een dam '
                 f'(a7-b6 c5×a7, c7-d6 e5×c7, e7-d6 c5×e7, e7-f6 e5×g7): {self.moves_list(pg, i0, legal, "1. ")}.</p>',
                 f'<p>Na <a class="m1" href="#" data-i="{i1}">1. c7-b6</a> moet zwart de enige remisezet vinden; alles '
                 f'andere verliest: {self.moves_list(pg, i1, legal1, "1. … ")}.</p>',
                 '<p>Na 1. … e5-d4 kan wit zijn dammen eeuwig houden maar breekt nooit uit: zwart kan in geen van de '
                 '901.577 stellingen die zonder slag bereikbaar zijn een slag afdwingen.</p>']
        self.write('index.html', 'Overzicht', '\n'.join(body), pg)

    def vesting(self):
        k89, e89 = self.engine(9, 8, 1)
        k99, e99 = self.engine(9, 9, 0)
        pg = self.page((k89, e89), (k99, e99))
        fen = e89.fen(['c7', 'e7', 'g7'], ['a7', 'i7'])
        fig, i0 = pg.diagram(fen, '8×9, wit aan zet', gid=k89)
        v, legal = e89.analyse(fen)
        fb = 'B' + fen[1:]
        ib = pg.add(fb, gid=k89)
        vb, _ = e89.analyse(fb)
        hb, *_ = pg.moves(fb, self.plan('1. a7-b8 2. g7-f8! b8xd6 3. e7xc5'), 1, True, 'm1', ib)
        hb2, *_ = pg.best_line(pg.pos[int(re.findall(r'data-i="(\d+)"', hb)[-1])]['f'], 40,
                               start=int(re.findall(r'data-i="(\d+)"', hb)[-1]), number=3)
        f2 = e89.fen(['d6', 'e7', 'g7'], ['a7', 'i7'])
        fig2, i2 = pg.diagram(f2, '8×9, wit aan zet', gid=k89)
        v2, legal2 = e89.analyse(f2)
        h2, *_ = pg.moves(f2, self.plan('1. d6-c7!'), 1, False, 'm1', i2)
        f9 = e99.fen(['a2', 'c2', 'd3'], ['b1', 'i8'])
        fig9, i9 = pg.diagram(f9, '9×9 p0, wit aan zet', gid=k99)
        v9, legal9 = e99.analyse(f9)
        body = ['<h1>De vesting in de dubbele hoek</h1>', '''
<p>Een dam op de twee randvelden van een dubbele hoek kan nooit geslagen worden en heeft altijd een wachtzet. Op 8×9 en
9×10 maakt een zwarte dam in elke dubbele hoek remise in precies de helft van de stellingen, en welke helft dat is,
beslist de <strong>oppositie</strong>: de pariteit van de som van de rijen van alle stukken wisselt bij elke zet, en
dammen kunnen geen tempo verliezen, dus wie de oppositie heeft, ligt vanaf het begin vast.</p>
<table class="tbl"><tr><th>Zwart in beide dubbele hoeken</th><th>Pariteit 0</th><th>Pariteit 1</th></tr>
<tr><td>8×9</td><td>99,5% remise</td><td>0,0% remise</td></tr>
<tr><td>9×10</td><td>99,8%</td><td>0,0%</td></tr>
<tr><td>10×11</td><td>99,9%</td><td>99,9%</td></tr></table>''',
                '<h2>8×9: oppositie</h2>', fig,
                f'<p>Zwart op a7 en i7 (pendelend met b8 en h8), wit op c7, e7 en g7. Met wit aan zet ({v}) houdt elke '
                f'zet remise: zwart pendelt gewoon a7-b8-a7. {self.moves_list(pg, i0, legal, "1. ")}.</p>',
                f'<p>Met zwart aan zet staat zwart in zetdwang en verliest in {vb[1:]} ply: {hb} en 2 tegen 1 wint: '
                f'{hb2}</p>', '<div style="clear:both"></div>', fig2,
                f'<p>Met de witte dam op d6 in plaats van c7 ({v2}) wint wit met {h2}, en bereikt zo dezelfde stelling met '
                f'zwart aan zet.</p>', '<div style="clear:both"></div>', '''
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
<em>D</em> ≥ 9: de vesting houdt.</p>''', '<h2>9×9 p0: één hoek bevriezen kost drie dammen</h2>', fig9,
                f'<p>Op 9×9 p0 kost het bevriezen van één hoek al alle drie de witte dammen (één op het pendelveld, één die '
                f'de uitgang blokkeert, één die de blokkeerder dekt), dus de andere zwarte dam pendelt gewoon ({v9}): '
                f'{self.moves_list(pg, i9, legal9, "1. ")}.</p>']
        self.write('vesting.html', 'De vesting in de dubbele hoek', '\n'.join(body), pg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--engine', default=str(ROOT / 'build' / 'egdb_fast'))
    ap.add_argument('--out', default=str(ROOT / '_site' / 'checkers'))
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    s = Site(args.engine, out)
    s.index()
    s.vesting()


if __name__ == '__main__':
    main()
