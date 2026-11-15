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

TITLE = {'nl': 'Checkers: 3 dammen tegen 2', 'en': 'Checkers: 3 kings against 2'}

# the chapters, each with one page per section: (Dutch, English, [(file, Dutch, English)])
CHAPTERS = [
    ('2 tegen 1', '2 against 1', [
        ('twee-langste.html', 'De langste winst op 8×8', 'The longest win on 8×8'),
        ('twee-blokkade.html', 'Uitzondering 1: de blokkade aan de rand', 'Exception 1: the blockade at the edge'),
        ('twee-4x4.html', 'Uitzondering 2: remise op 4×4', 'Exception 2: a draw on 4×4')]),
    ('3 tegen 2 op 8×8', '3 against 2 on 8×8', [
        ('drie-langste.html', 'De langste winst', 'The longest win'),
        ('blokkades.html', 'Uitzondering 1: de blokkades', 'Exception 1: the blockades'),
        ('curiosa.html', 'Uitzondering 2: twee curiosa', 'Exception 2: two curiosities'),
        ('val.html', 'Twee vallen om de vesting te breken', 'Two traps to break the fortress'),
        ('val-pariteit.html', 'De winst is strategisch, de pariteit doet er niet toe', 'The win is strategic, parity does not matter')]),
    ('3 tegen 2 op andere borden', '3 against 2 on other boards', [
        ('langste-7x7.html', 'De langste winst op 7×7', 'The longest win on 7×7'),
        ('vesting-8x7.html', 'Langs de zijde van 7 valt de vesting op 8×7', 'Along the side of 7 the fortress falls on 8×7'),
        ('vesting.html', 'Eén val om de vesting te breken op 9×8', 'One trap to break the fortress on 9×8'),
        ('vesting-pariteit.html', 'De winst is niet strategisch, de pariteit doet ertoe op 9×8',
         'The win is non-strategic, parity matters on 9×8'),
        ('vesting-9x9.html', '9×9 onspeelbare hoeken: de val en de diagonaal', '9×9 unplayable corners: the trap and the diagonal'),
        ('afstandsregel.html', 'De afstandsregel', 'The distance rule')]),
    ('Conclusie', 'Conclusion', [
        ('conclusie.html', 'Conclusie', 'Conclusion')]),
]
# every page in reading order: (file, Dutch, English, chapter number or None)
PAGES = [('index.html', 'Overzicht', 'Overview', None)] + [(f, nl, en, c) for c, (_, _, secs) in enumerate(CHAPTERS)
                                                        for f, nl, en in secs]


TABLE32 = {'nl': '''<h2>3 dammen tegen 2 per bord</h2>
<table class="tbl"><tr><th>Bord</th><th>Rustige remises (3 dammen aan zet)</th><th>Langste winst</th>
<th>Zwart in beide dubbele hoeken: remises</th></tr>
<tr><td>7×6</td><td>273</td><td>63</td><td>0%</td></tr>
<tr><td>8×7</td><td>792 (0,16%)</td><td>75</td><td>0%</td></tr>
<tr><td>8×8</td><td>1.252 (0,11%)</td><td>67</td><td>0%</td></tr>
<tr><td>9×8</td><td>87.670 (4,0%)</td><td>79</td><td>50% (oppositie)</td></tr>
<tr><td>10×9</td><td>344.907 (4,4%)</td><td>93</td><td>50% (oppositie)</td></tr>
<tr><td>10×8</td><td>288.748</td><td>97</td><td>~100%</td></tr>
<tr><td>10×10</td><td>1.012.242 (7,1%)</td><td>115</td><td>~100%</td></tr>
<tr><td>11×10</td><td>1.810.258 (7,5%)</td><td>109</td><td>~100%</td></tr>
<tr><td>12×10</td><td>2.744.990</td><td>127</td><td>~100%</td></tr>
<tr><td>9×7 onspeelbare hoeken (vier dubbele hoeken)</td><td>124.148 (13,2%)</td><td>93</td><td>zie <a href="vesting.html">vesting</a></td></tr>
<tr><td>9×9 onspeelbare hoeken (vier dubbele hoeken)</td><td>1.201.460 (29,5%)</td><td>99</td><td>zie <a href="vesting.html">vesting</a></td></tr>
</table>
<p>Op borden met twee oneven zijden zijn de vier hoekvelden óf alle vier speelbaar óf alle vier onspeelbaar; bij onspeelbare hoekvelden is elke hoek een dubbele hoek.</p>''',
           'en': '''<h2>3 kings against 2 per board</h2>
<table class="tbl"><tr><th>Board</th><th>Quiet draws (3 kings to move)</th><th>Longest win</th>
<th>Black in both double corners: draws</th></tr>
<tr><td>7×6</td><td>273</td><td>63</td><td>0%</td></tr>
<tr><td>8×7</td><td>792 (0.16%)</td><td>75</td><td>0%</td></tr>
<tr><td>8×8</td><td>1,252 (0.11%)</td><td>67</td><td>0%</td></tr>
<tr><td>9×8</td><td>87,670 (4.0%)</td><td>79</td><td>50% (opposition)</td></tr>
<tr><td>10×9</td><td>344,907 (4.4%)</td><td>93</td><td>50% (opposition)</td></tr>
<tr><td>10×8</td><td>288,748</td><td>97</td><td>~100%</td></tr>
<tr><td>10×10</td><td>1,012,242 (7.1%)</td><td>115</td><td>~100%</td></tr>
<tr><td>11×10</td><td>1,810,258 (7.5%)</td><td>109</td><td>~100%</td></tr>
<tr><td>12×10</td><td>2,744,990</td><td>127</td><td>~100%</td></tr>
<tr><td>9×7 unplayable corners (four double corners)</td><td>124,148 (13.2%)</td><td>93</td><td>see <a href="vesting.html">fortress</a></td></tr>
<tr><td>9×9 unplayable corners (four double corners)</td><td>1,201,460 (29.5%)</td><td>99</td><td>see <a href="vesting.html">fortress</a></td></tr>
</table>
<p>On boards with two odd sides the four corner squares are either all playable or all unplayable; with unplayable corner squares every corner is a double corner.</p>'''}


def rank_parity(e, fen):
    """the parity of the sum of the rank numbers (1, 2, ...) of all pieces: 0 = even, 1 = odd"""
    sq = {int(n): fr for n, fr in e.g['sq'].items()}
    nums = [int(x.lstrip('K')) for part in fen.split(':')[1:] for x in part[1:].split(',') if x]
    return sum(sq[n][1] + 1 for n in nums) % 2


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

    def engine(self, w, h, corners, nw=3, nb=2):
        key = f'{h}x{w}' + ('' if corners == 'playable' else f' {corners}') + ('' if (nw, nb) == (3, 2) else f' {nw}-{nb}')
        if key not in self.engines:
            self.engines[key] = FastEngine(self.exe, '--width', w, '--height', h, '--corners', corners, '--rules', 'checkers',
                                           '--white', nw, '--black', nb)
        return key, self.engines[key]

    def chapter(self, c):
        return CHAPTERS[c][0] if self.lang == 'nl' else CHAPTERS[c][1]

    def nav(self, current=None):
        groups = [('', [(PAGES[0][0], self.title(0))])] + [
            (self.chapter(c), [(f, nl if self.lang == 'nl' else en) for f, nl, en in secs]) for c, (_, _, secs) in enumerate(CHAPTERS)]
        home = '../index.html' if self.lang == 'en' else '../../nl/index.html'      # English: default, Dutch in nl/
        alt = f'nl/{current}' if self.lang == 'en' else f'../{current}'
        return (f'<a class="brand" href="index.html">{esc(TITLE[self.lang])}</a>'
                f'{menu(groups, current, self.t("Hoofdstukken", "Chapters"))}'
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
        if PAGES[k][3] is not None:      # a section page: its chapter above its title
            body = f'<p class="src">{esc(self.chapter(PAGES[k][3]))}</p><h1>{esc(self.title(k))}</h1>\n' + body
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
        k21, e21 = self.engine(8, 8, 'playable', 2, 1)
        k44, e44 = self.engine(4, 4, 'playable', 2, 1)
        k88, e88 = self.engine(8, 8, 'playable')
        pg = self.page((k21, e21), (k44, e44), (k88, e88))
        t = self.t
        body = [f'<h1>{esc(TITLE[self.lang])}</h1>', t('''
<p>Eindspelen met alleen dammen in het Engelse damspel (<em>checkers</em>): korte dammen (één veld), slagplicht en
vrije keuze tussen slagen. Op borden van elke afmeting, exact uitgerekend met
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>.
Waarden in ply voor de partij aan zet: W = winst, L = verlies, D = remise; zuiver speltheoretisch, zonder
zettenregels. Borden heten breedte × hoogte, zoals ze getekend zijn. Elke stelling is klikbaar.</p>''', '''
<p>Endgames with kings only in English checkers: short kings (one square), mandatory capture and free choice among
captures. On boards of any size, computed exactly with
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>. Values in
plies for the side to move: W = win, L = loss, D = draw; purely game-theoretic, without move-count rules. Every position
is clickable. Boards are named width × height, as drawn.</p>''')]
        toc = ['<h2>' + t('Hoofdstukken', 'Chapters') + '</h2><ul class="toc">']
        for c, (_, _, secs) in enumerate(CHAPTERS):
            links = [f'<a href="{f}">{esc(nl if self.lang == "nl" else en)}</a>' for f, nl, en in secs]
            items = links[0] if len(links) == 1 else '<ul class="sub">' + ''.join(f'<li>{x}</li>' for x in links) + '</ul>'
            toc.append(f'<li><span class="h">{esc(self.chapter(c))}</span>{items}</li>')
        fig, _ = pg.diagram('W:WK4,K8,K29:BK1,K28', t('8×8, zwart in beide dubbele hoeken: wit wint (W61)',
                                                       '8×8, black in both double corners: white wins (W61)'), gid=k88)
        body.insert(1, fig)
        body += toc + ['</ul>', TABLE32[self.lang]]

        def flush(name):                # finish the page so far, start the next one
            nonlocal body, pg
            self.write(name, None, '\n'.join(body), pg, k88)
            pg = self.page((k21, e21), (k44, e44), (k88, e88))
            body = []
        flush('index.html')
        fen = self.longest_21(e21)
        v, _ = e21.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'8×8, 2 tegen 1, wit aan zet: {v}', f'8×8, 2 against 1, white to move: {v}'), gid=k21)
        h, *_ = pg.best_line(fen, 60, start=i0)
        body += [fig, t('''
<p>2 tegen 1 is op elk voldoende groot bord gewonnen, met twee uitzonderingen:</p>
<ul><li><a href="twee-blokkade.html">de blokkade aan de rand</a>: in een vast handjevol stellingen (8 of 10,
onafhankelijk van de bordgrootte; op 8×8 op spiegelingen na 3) zitten de twee witte dammen opgesloten in de enkele hoek en houdt de zwarte dam ze
daar met oppositie; op oneven × oneven borden met speelbare hoeken bestaat die blokkade niet;</li>
<li>en, verrassend, <a href="twee-4x4.html">het 4×4-bord</a>: daar is 2 tegen 1 remise.</li></ul>
<p>De langste winst groeit gestaag met het bord: 33 ply op 8×8, 41 op 10×10, 49 op 12×12, 61 op 15×12.</p>''', '''
<p>2 against 1 is won on every board that is large enough, with two exceptions:</p>
<ul><li><a href="twee-blokkade.html">the blockade at the edge</a>: in a fixed handful of positions (8 or 10,
independent of the board size; on 8×8 up to symmetry 3) the two white kings are locked in the single corner and the black king keeps them there
by opposition; on odd × odd boards with playable corners this blockade does not exist;</li>
<li>and, surprisingly, <a href="twee-4x4.html">the 4×4 board</a>: there 2 against 1 is a draw.</li></ul>
<p>The longest win grows steadily with the board: 33 plies on 8×8, 41 on 10×10, 49 on 12×12, 61 on 15×12.</p>'''),
                 t(f'<p>De langste winst op 8×8 ({v[1:]} ply):</p>', f'<p>The longest win on 8×8 ({v[1:]} plies):</p>'),
                 f'<p class="var">{h}</p>',
                 '<div style="clear:both"></div>']
        flush('twee-langste.html')
        # the 2-1 blockade on 8x8: two white kings locked in the single corner, the black king keeps the opposition.
        # 10 quiet draws with white to move, 5 in each single corner, 3 up to the symmetries of the board: two that
        # turn into each other (the repetition) and one in which every white move gives a king away
        body += [t('<p>De blokkade: de twee witte dammen zitten in de enkele hoek en de zwarte dam houdt ze daar met '
                   'oppositie. Op 8×8 zijn er precies 10 rustige remisestellingen met wit aan zet, 5 in elke enkele hoek; '
                   'op spiegelingen van het bord na zijn het er 3. Met zwart aan zet is elk van de drie verloren.</p>',
                   '<p>The blockade: the two white kings are in the single corner and the black king keeps them there by '
                   'opposition. On 8×8 there are exactly 10 quiet drawn positions with white to move, 5 in each single '
                   'corner; up to the symmetries of the board there are 3. With black to move each of the three is '
                   'lost.</p>')]
        notes = {
            'W:WK3,K4:BK11': ('Wit op h8 en f8, zwart op f6. Alleen f8-g7 houdt beide dammen (h8-g7 geeft er een weg, f8-e7 verliest), '
                              'en na 1. f8-g7 f6-e7 staat de volgende stelling op het bord: ',
                              'White on h8 and f8, black on f6. Only f8-g7 keeps both kings (h8-g7 gives one away, f8-e7 loses), '
                              'and after 1. f8-g7 f6-e7 the next position is on the board: '),
            'W:WK4,K8:BK7': ('Wit op h8 en g7, zwart op e7 (of gespiegeld op g5). De dam op h8 kan niet weg; na 1. g7-f8 e7-f6 is '
                             'de vorige stelling terug, en zo gaan de twee in elkaar over: herhaling. ',
                             'White on h8 and g7, black on e7 (or mirrored on g5). The king on h8 cannot move; after 1. g7-f8 '
                             'e7-f6 the previous position is back, and so the two turn into each other: repetition. '),
            'W:WK3,K12:BK11': ('Wit op f8 en h6, zwart op f6: wit staat in zetdwang. Elke witte zet geeft een dam weg, en 1 tegen 1 '
                               'is remise. ',
                               'White on f8 and h6, black on f6: white is in zugzwang. Every white move gives a king away, '
                               'and 1 against 1 is a draw. ')}
        for k, fen in enumerate(('W:WK3,K4:BK11', 'W:WK4,K8:BK7', 'W:WK3,K12:BK11')):
            v, legal = e21.analyse(fen)
            vb, _ = e21.analyse('B' + fen[1:])
            fig, i0 = pg.diagram(fen, t(f'8×8, 2 tegen 1, wit aan zet: {v} (zwart aan zet: {vb})',
                                        f'8×8, 2 against 1, white to move: {v} (black to move: {vb})'), gid=k21)
            nl, en = notes[fen]
            body += [fig, f'<p>{t(nl, en)}{t("Alle zetten van wit: ", "All moves of white: ")}'
                     f'{self.moves_list(pg, i0, legal, "1. ")}.</p>', '<div style="clear:both"></div>']
        flush('twee-blokkade.html')
        # 4x4: the draw; driven out of one double corner, black escapes through the centre to the other one
        fen = 'W:WK6,K8:BK3'
        fig, i0 = pg.diagram(fen, t(f'4×4, 2 tegen 1, wit aan zet: {e44.analyse(fen)[0]}',
                                    f'4×4, 2 against 1, white to move: {e44.analyse(fen)[0]}'), gid=k44)
        parts, i, f = [], i0, fen
        for k, num in enumerate('6-4 3-1 8-6 1-3 4-1 3-5 1-3 5-8'.split()):
            m = next(x for x in e44.analyse(f)[1] if x[3] == num)
            i, f = pg.child(i, m), m[2]
            parts.append(f'<a class="m1" href="#" data-i="{i}">{f"{k // 2 + 1}. " if k % 2 == 0 else ""}{pg.label(k44, m)}</a>')
        body += [fig, t('<p>Op 4×4 is 2 tegen 1 remise. Het bord heeft twee dubbele hoeken met alleen de twee middenvelden '
                        'ertussen. Drijft wit de zwarte dam met beide dammen uit de ene dubbele hoek, dan ontsnapt zwart door '
                        'het midden naar de andere:</p>',
                        '<p>On 4×4, 2 against 1 is a draw. The board has two double corners with only the two centre squares '
                        'between them. If white drives the black king out of one double corner with both kings, black escapes '
                        'through the centre to the other one:</p>'),
                 f'<p class="var">{" ".join(parts)}</p>',
                 t('<p>Daarna begint het opnieuw in de andere hoek.</p>', '<p>Then it starts again in the other corner.</p>'),
                 '<div style="clear:both"></div>']
        flush('twee-4x4.html')
        # the longest win: black flees to both double corners and white builds the trap there
        fen = 'W:WK3,K4,K30:BK11,K13'
        v, _ = e88.analyse(fen)
        v61, _ = e88.analyse('W:WK4,K8,K29:BK1,K28')
        fig, i0 = pg.diagram(fen, t(f'8×8, wit aan zet: {v}', f'8×8, white to move: {v}'), gid=k88)
        h, *_ = pg.best_line(fen, 80, start=i0)
        body += [fig, t(
            f'<p>De langste winst op 8×8 zonder slag voor beide kanten in de beginstelling duurt {v[1:]} ply. Zwart vlucht met '
            f'beide dammen naar de dubbele hoeken: na 15. … 27-32 staan ze op 5 en 32. Een dam op de twee randvelden van een '
            f'dubbele hoek kan niet geslagen worden en heeft altijd een wachtzet (zie de <a href="vesting.html">vesting</a>), '
            f'maar op 8×8 helpt dat niet: alle 13.320 rustige stellingen met zwart in beide dubbele hoeken zijn gewonnen, met '
            f'wit aan zet én met zwart aan zet; de langste duurt {v61[1:]} ply. Wit bouwt in de dubbele hoeken een '
            f'vangstelling en dwingt op zet 21 met een offer een ruil af: zwart moet slaan, wit slaat terug, en 2 tegen 1 '
            f'wint. Hoe die val werkt, staat bij <a href="val.html">de val</a>.</p>'
            f'<p>De twee dubbele hoeken liggen op 8×8 diagonaal tegenover elkaar en dicht bij elkaar (in de '
            f'<a href="afstandsregel.html">afstandsregel</a>: D × d = 7 × 7, de vesting valt). De oppositie, die op 9×8 en '
            f'10×9 de helft van zulke stellingen remise maakt, speelt op 8×8 dus geen rol.</p>',
            f'<p>The longest win on 8×8 without a capture for either side in the start position takes {v[1:]} plies. Black '
            f'flees with both kings to the double corners: after 15. … 27-32 they stand on 5 and 32. A king on the two edge '
            f'squares of a double corner cannot be captured and always has a waiting move (see the '
            f'<a href="vesting.html">fortress</a>), but on 8×8 that does not help: all 13,320 quiet positions with black in '
            f'both double corners are won, with white to move and with black to move; the longest takes {v61[1:]} plies. '
            f'White builds a trap in the double corners and on move 21 forces an exchange with a sacrifice: black has to '
            f'capture, white captures back, and 2 against 1 wins. How that trap works is shown at '
            f'<a href="val.html">the trap</a>.</p>'
            f'<p>On 8×8 the two double corners lie diagonally opposite and close together (in the '
            f'<a href="afstandsregel.html">distance rule</a>: D × d = 7 × 7, the fortress falls). The opposition, which '
            f'makes half of such positions a draw on 9×8 and 10×9, plays no role on 8×8.</p>'),
            f'<p class="var">{h}</p>',
            t('<p>Zwart probeert zijn toevlucht te zoeken in een vesting door beide dubbele hoeken te bezetten, maar wit '
              'breekt de vesting. Hoe wit dat doet, analyseren we na de twee uitzonderingen, bij '
              '<a href="val.html">twee vallen om de vesting te breken</a>.</p>',
              '<p>Black tries to take refuge in a fortress by occupying both double corners, but white breaks the fortress. '
              'How white achieves this is analysed after the two exceptions, at '
              '<a href="val.html">two traps to break the fortress</a>.</p>'),
            '<div style="clear:both"></div>']
        flush('drie-langste.html')
        body, phases, pgp = self.traps88(pg, k88, e88)
        flush('val.html')
        body, pg = phases, pgp
        flush('val-pariteit.html')
        # the 3-2 blockades on 8x8: 1252 quiet draws with white to move, 316 up to symmetry; 307 of them contain the
        # 2-1 blockade in a single corner
        body += [t('''
<p>Op 8×7 en 8×8 is 3 tegen 2 in het algemeen gewonnen. De weinige remises ontstaan bijna alleen als wit tegen de rand
gedrukt wordt (0,0% remise zonder witte dam aan de rand, 0,5% met drie), en zwart in beide dubbele hoeken maakt nooit
remise. Op 8×8 zijn er 1.252 rustige remisestellingen met wit aan zet, 316 op spiegelingen van het bord na, en bijna
allemaal draaien ze om de enkele hoek. In 307 zit de blokkade van <a href="twee-blokkade.html">2 tegen 1</a>: twee witte
dammen in een enkele hoek, vastgehouden door één zwarte dam. In 5 houdt zwart dezelfde twee witte dammen met twee dammen
vast (op f6 en e5 tegen h8 en g7), in 2 zit één witte dam vast in de hoek (de <a href="curiosa.html">twee curiosa</a>). Alleen 2 klassen hebben niets met de hoek te
maken (het derde diagram).</p>''', '''
<p>On 8×7 and 8×8 3-2 is won generically. The few draws occur almost only when white is pushed against the edge (draw
rate 0.0% with no white king on the edge, 0.5% with three), and black holding both double corners never draws. On 8×8
there are 1,252 quiet drawn positions with white to move, 316 up to the symmetries of the board, and nearly all of them
turn on the single corner. 307 contain the blockade of <a href="twee-blokkade.html">2 against 1</a>: two white kings in a
single corner, held by one black king. In 5 black holds the same two white kings with two kings (on f6 and e5 against h8
and g7), in 2 one white king is locked in the corner (the <a href="curiosa.html">two curiosities</a>). Only 2 classes have nothing to do with the corner (the third
diagram).</p>''')]
        body += [t('''
<p>Het algemene patroon: een van de drie motieven van <a href="twee-blokkade.html">1 tegen 2 in de enkele hoek</a>, en
de overgebleven zwarte dam heeft de oppositie tegen de overgebleven witte dam. Met wit aan zet staan die twee dan op
hetzelfde systeem: het verschil in lijn en het verschil in rij zijn allebei even. Ze mogen verder overal over het bord
staan, zolang de vrije witte dam de blokkade in de hoek niet kan opheffen. De oppositie is noodzakelijk: van de rustige
stellingen met een van de drie motieven en de vrije dammen op verschillende systemen is er geen enkele remise; met de
vrije dammen op hetzelfde systeem is ruwweg 4 op de 10 remise, en in de rest breekt de vrije witte dam de blokkade toch
open.</p>
<p>Net als bij 1 tegen 2 is de blokkade dus nooit strategisch: van de 1.252 rustige remisestellingen met wit aan zet
zijn er 1.244 verloren met zwart aan zet. De enige 8 die met wie ook aan zet remise zijn, zijn de twee klassen met de
zwarte dam midden tussen de witte (het derde diagram).</p>''', '''
<p>The general pattern: one of the three motifs of <a href="twee-blokkade.html">1 against 2 in the single corner</a>, and
the remaining black king has the opposition against the remaining white king. With white to move those two then stand on
the same system: the difference in files and the difference in ranks are both even. Otherwise they may stand anywhere
on the board, as long as the free white king cannot lift the blockade in the corner. The opposition is necessary: of
the quiet positions with one of the three motifs and the free kings on different systems not a single one is drawn;
with the free kings on the same system roughly 4 in 10 are drawn, and in the rest the free white king breaks the
blockade after all.</p>
<p>As with 1 against 2, the blockade is therefore never strategic: of the 1,252 quiet drawn positions with white to
move, 1,244 are lost with black to move. The only 8 that are drawn whoever is to move are the two classes with the black
king in the middle of the white kings (the third diagram).</p>''')]
        cases = [
            ('W:WK3,K4,K29:BK11,K22',
             'Een dubbele blokkade: in de enkele hoek rechtsboven dezelfde blokkade als bij 2 tegen 1, en de derde witte dam zit '
             'opgesloten in de andere enkele hoek. Zet hij de hoek uit, dan slaat zwart hem en is het 2 tegen 2. ',
             'A double blockade: in the single corner at the top right the same blockade as in 2 against 1, and the third white '
             'king is locked in the other single corner. If it leaves the corner, black captures it and it is 2 against 2. '),
            ('W:WK1,K3,K4:BK9,K11',
             'Drie witte dammen aan de rand, de twee zwarte dammen er recht tegenover: zwart op b6 tegenover b8, zwart op f6 '
             'tegenover f8 en h8. Wit aan zet komt niet verder: elke zet houdt remise, maar meer niet. Het werk doet de hoek, '
             'niet de rand: van de 24 opstellingen met drie witte dammen op de bovenrand en twee zwarte op de zesde rij zijn er '
             'maar 3 remise, en in alle drie houdt zwart op f6 de witte dammen op f8 en h8 vast (de blokkade van 2 tegen 1). '
             'Wit op b8, d8 en f8 tegen zwart op d6 en f6, drie naast elkaar met twee recht tegenover, wint (W13). ',
             'Three white kings on the edge, the two black kings right opposite them: black on b6 opposite b8, black on f6 '
             'opposite f8 and h8. White to move makes no progress: every move keeps the draw, but no more. The corner does the '
             'work, not the edge: of the 24 set-ups with three white kings on the top edge and two black kings on the sixth '
             'rank only 3 are drawn, and in all three black on f6 holds the white kings on f8 and h8 (the blockade of 2 against '
             '1). White on b8, d8 and f8 against black on d6 and f6, three side by side with two right opposite, wins (W13). '),
            ('W:WK7,K14,K23:BK13,K15',
             'Een van de 2 klassen die niets met de hoek te maken hebben (de andere: zwart op h4 in plaats van a5): de zwarte '
             'dam op e5 staat midden tussen de drie witte dammen. Een '
             'witte dam die ernaast komt, wordt geslagen (1. e7-d6 e5×c7). Remise met wit én met zwart aan zet; met zwart aan '
             'zet houden alleen e5-d6 en e5-d4 remise. ',
             'One of the 2 classes that have nothing to do with the corner (the other: black on h4 instead of a5): the black '
             'king on e5 stands in the middle of the three white kings. '
             'A white king that steps next to it is captured (1. e7-d6 e5×c7). A draw with white and with black to move; with '
             'black to move only e5-d6 and e5-d4 keep the draw. ')]
        for fen, nl, en in cases:
            v, legal = e88.analyse(fen)
            vb, _ = e88.analyse('B' + fen[1:])
            fig, i0 = pg.diagram(fen, t(f'8×8, wit aan zet: {v} (zwart aan zet: {vb})',
                                        f'8×8, white to move: {v} (black to move: {vb})'), gid=k88)
            body += [fig, f'<p>{t(nl, en)}{t("Alle zetten van wit: ", "All moves of white: ")}'
                     f'{self.moves_list(pg, i0, legal, "1. ")}.</p>', '<div style="clear:both"></div>']
        flush('blokkades.html')
        # two curiosities: one white king locked in the single corner in another way
        body += [t('<p>Twee van de 316 remiseklassen van 3 tegen 2 op 8×8 passen in geen van de families van '
                   '<a href="blokkades.html">de blokkades</a>: er zit maar één witte dam vast in de enkele hoek, en toch komt '
                   'wit niet verder.</p>',
                   '<p>Two of the 316 drawn classes of 3 against 2 on 8×8 fit none of the families of '
                   '<a href="blokkades.html">the blockades</a>: only one white king is locked in the single corner, and still '
                   'white makes no progress.</p>')]
        for fen, nl, en in (
            ('W:WK2,K4,K11:BK10,K12',
             'Eén witte dam zit vast in de hoek: na h8-g7 slaat zwart h6×f8. Tegen de zwarte dam op d6 komen de twee andere '
             'witte dammen niet verder, en f6-g7 en f6-g5 verliezen zelfs. ',
             'One white king is locked in the corner: after h8-g7 black captures h6×f8. Against the black king on d6 the two '
             'other white kings make no progress, and f6-g7 and f6-g5 even lose. '),
            ('W:WK4,K11,K15:BK3,K8',
             'De zwarte dammen staan in de enkele hoek van wit, op f8 en g7, voor de witte dam op h8. Die kan niet bewegen: g7 '
             'is bezet, en slaan kan niet omdat f6 bezet is. Alleen f6-e7 houdt remise; de andere zetten verliezen. ',
             'The black kings stand in white\'s single corner, on f8 and g7, in front of the white king on h8. It cannot move: '
             'g7 is occupied, and it cannot capture because f6 is occupied. Only f6-e7 keeps the draw; the other moves lose. '),
                ):
            v, legal = e88.analyse(fen)
            vb, _ = e88.analyse('B' + fen[1:])
            fig, i0 = pg.diagram(fen, t(f'8×8, wit aan zet: {v} (zwart aan zet: {vb})',
                                        f'8×8, white to move: {v} (black to move: {vb})'), gid=k88)
            body += [fig, f'<p>{t(nl, en)}{t("Alle zetten van wit: ", "All moves of white: ")}'
                     f'{self.moves_list(pg, i0, legal, "1. ")}.</p>', '<div style="clear:both"></div>']
        flush('curiosa.html')

    @staticmethod
    def exchange(e, fen):
        """white to move: a quiet offer after which black must capture and white captures back to a won 2-1, as the
        moves (offer, capture, recapture), or None"""
        count = lambda f: tuple(len([x for x in side[1:].split(',') if x]) for side in f.split(':')[1:])
        for off in e.analyse(fen)[1]:
            if count(off[2]) != (3, 2):
                continue
            replies = e.analyse(off[2])[1]
            if not replies or any(count(r[2]) != (2, 2) for r in replies):
                continue
            # the recapture has to leave a won 2-1 (black must not be able to capture back at once)
            backs = [next((m for m in e.analyse(r[2])[1] if count(m[2])[1] < 2 and m[1].startswith('W')), None) for r in replies]
            if all(backs):
                return off, replies[0], backs[0]
        return None

    def traps88(self, pg, k88, e88):
        """two traps on 8x8, one at each parity: every shuttle move of black allows a forced exchange"""
        t = self.t
        body = [t('<p>Bij <a href="twee-langste.html">2 tegen 1</a> is de dubbele hoek geen veilige haven: twee witte '
                  'dammen winnen ook tegen een zwarte dam in de dubbele hoek. Bij 3 tegen 2 bezet zwart daarom beide dubbele '
                  'hoeken, en per hoek twee witte dammen zou neerkomen op 4 tegen 2. Op 8×8 is het bord precies klein genoeg om '
                  'het met 3 dammen te doen: in de val doet de middelste witte dam dubbel werk. Hij slaat terug na de ruil in '
                  'de ene hoek én na die in de andere, in de eerste val hieronder vanaf d4, in de tweede vanaf e5.</p>',
                  '<p>In <a href="twee-langste.html">2 against 1</a> the double corner is no safe haven: two white kings win '
                  'against a black king in the double corner too. In 3 against 2 black therefore occupies both double corners, '
                  'and two white kings per corner would mean 4 against 2. On 8×8 the board is just small enough to do it with '
                  '3 kings: in the trap the middle white king does double duty. It captures back after the exchange in the one '
                  'corner and after the one in the other, in the first trap below from d4, in the second from e5.</p>')
                + t('<p>Een dam op de randvelden van een dubbele hoek kan niet geslagen worden. Om de vesting te breken, moet '
                    'wit dus een zwarte dam uit zijn dubbele hoek krijgen, en dat kan op twee manieren: ofwel wit ruilt een '
                    'zwarte dam af zonder er zelf een dam bij in te schieten (zwart moet slaan, wit slaat terug, en 2 tegen 1 '
                    'wint), ofwel een zwarte dam moet de dubbele hoek verlaten en kan niet meer terug. Op 8×8 gaat het met de '
                    'eerste manier.</p>',
                    '<p>A king on the edge squares of a double corner cannot be captured. To break the fortress white '
                    'therefore has to get a black king out of its double corner, and there are two ways: either white '
                    'exchanges a black king without losing a king for nothing (black has to capture, white captures back, '
                    'and 2 against 1 wins), or a black king has to leave the double corner and cannot come back. On 8×8 it '
                    'goes the first way.</p>')
                + t('<p>Wit breekt de dubbele hoeken met een val: een opstelling waarin elke pendelzet van zwart een '
                  'gedwongen ruil toelaat: met een rustige zet dwingt wit zwart een witte dam te slaan, wit slaat terug, en 2 '
                  'tegen 1 wint. Zwart heeft twee dammen en dus twee pendelzetten; de val moet ze allebei straffen, ook als de '
                  'ruil de andere zwarte dam treft. En verlaat een zwarte dam zijn dubbele hoek, dan moet dat ook meteen '
                  'materiaal kosten. Of zo\'n val werkt, hangt af van de pariteit: even of oneven, de pariteit '
                  'van de som van de rijnummers (1, 2, …) van alle dammen, met zwart aan zet. Die verandert zonder slag nooit, '
                  'want dammen kunnen geen tempo verliezen; dat is de oppositie.</p>'
                  '<p>De database telt de vallen met zwart aan zet en beide zwarte dammen op de randvelden van de twee '
                  'dubbele hoeken: op 8×8 zijn er 2 bij even en 2 bij oneven pariteit: de twee vallen hieronder en hun '
                  'spiegelbeelden (het bord 180° gedraaid), want draaien keert de pariteit om. Wit heeft dus bij elke pariteit een '
                  'val, en de vesting valt altijd: wit kiest de val die bij de oppositie past. In de vallen staan de drie '
                  'witte dammen op een van de twee lange diagonalen die de dubbele hoeken verbinden, tussen de zwarte dammen '
                  'in; welke diagonaal, bepaalt de pariteit. De tellingen komen uit '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>.</p>',
                  '<p>White breaks the double corners with a trap: a set-up in which every shuttle move of black allows a '
                  'forced exchange: with a quiet move white forces black to capture a white king, white captures back, and 2 '
                  'against 1 wins. Black has two kings and therefore two shuttle moves; the trap has to punish both, even when '
                  'the exchange hits the other black king. And when a black king leaves its double corner, that too has to '
                  'cost material at once. Whether such a trap works depends on the parity: even or odd, the '
                  'parity of the sum of the rank numbers (1, 2, …) of all kings, with black to move. It never changes without a '
                  'capture, since kings cannot lose a tempo; that is the opposition.</p>'
                  '<p>The database counts the traps with black to move and both black kings on the edge squares of the two '
                  'double corners: on 8×8 there are 2 at even and 2 at odd parity: the two traps below and their mirror '
                  'images (the board turned 180°), since turning reverses the parity. White therefore has a trap at each parity, '
                  'and the fortress always falls: white picks the trap that fits the opposition. In the traps the three white '
                  'kings stand on one of the two long diagonals that connect the double corners, between the black kings; '
                  'which diagonal decides the parity. The counts come from '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>.</p>')]
        for fen in ('B:WK14,K18,K23:BK5,K32', 'B:WK6,K15,K24:BK5,K32'):
            v, legal = e88.analyse(fen)
            even = rank_parity(e88, fen) == 0
            fig, i0 = pg.diagram(fen, t(f'8×8, zwart aan zet ({"even" if even else "oneven"} pariteit): {v}',
                                        f'8×8, black to move ({"even" if even else "odd"} parity): {v}'),
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
        pgp = self.page((k88, e88))      # the parity page has a page (positions, boards) of its own
        # why both parities: the diagonal of white decides the parity, and moving over to the other diagonal costs three
        # moves (one per king), an odd number. Squares in the text are written algebraically, so that the notation switch
        # can show them either way.
        sq88 = {int(n): fr for n, fr in e88.g['sq'].items()}
        al = lambda n: f'{chr(97 + sq88[n][0])}{sq88[n][1] + 1}'
        alg = lambda ns: ', '.join(al(n) for n in ns[:-1]) + t(' en ', ' and ') + al(ns[-1])
        rows = [((5, 32), (14, 18, 23), '5…32', True), ((1, 28), (9, 18, 27), '5…32', True),
                ((5, 32), (6, 15, 24), '1…28', False), ((1, 28), (10, 15, 19), '1…28', False)]
        for B, W, _, even in rows:          # every row is a trap at the stated parity (checked with the database)
            fen = f"B:W{','.join('K%d' % x for x in W)}:B{','.join('K%d' % x for x in B)}"
            assert (rank_parity(e88, fen) == 0) == even and e88.analyse(fen)[0].startswith('L')
        diag = lambda d: f'{al(int(d.split("…")[0]))}…{al(int(d.split("…")[1]))}'
        table = (f'<table class="tbl"><tr><th>{t("Zwart", "Black")}</th><th>{t("Wit", "White")}</th>'
                 f'<th>{t("Diagonaal van wit", "Diagonal of white")}</th><th>{t("Pariteit", "Parity")}</th></tr>'
                 + ''.join(f'<tr><td>{alg(B)}</td><td>{alg(W)}</td><td>{diag(d)}</td>'
                           f'<td>{t("even" if even else "oneven", "even" if even else "odd")}</td></tr>' for B, W, d, even in rows)
                 + '</table>')
        phases = [t(
            f'<p>Op welke diagonaal zwart staat, maakt niet uit: op elk van de twee diagonalen heeft wit een val tegen de zwarte '
            f'dammen op {alg((5, 32))} en een tegen {alg((1, 28))}. Wat de pariteit bepaalt, is de diagonaal van wit. De vier '
            f'vallen, de twee van de <a href="val.html">vorige pagina</a> en hun spiegelbeelden (180° gedraaid):</p>',
            f'<p>Which diagonal black stands on does not matter: on each of the two diagonals white has a trap against the black '
            f'kings on {alg((5, 32))} and one against {alg((1, 28))}. What decides the parity is the diagonal of white. The four '
            f'traps, the two of the <a href="val.html">previous page</a> and their mirror images (turned 180°):</p>'), table]
        # the change-over, with the strongest moves: white in the set-up of the second trap (odd parity) but white to move;
        # after 4. 24-27 both shuttle moves of black lead into a trap at even parity (the first trap, or the second turned)
        fen = 'W:WK6,K15,K24:BK5,K32'
        assert self.exchange(e88, fen) is None
        n = lambda x: int(x[1][1:])

        def check(f, line):                    # white plays a strongest move, black a longest defence
            for k, mv in enumerate(line):
                ms = e88.analyse(f)[1]
                m = next(x for x in ms if x[3] == mv)
                w = (f[0] == 'W')
                assert m[1][0] == ('W' if w else 'L') and n(m) == (min if w else max)(n(x) for x in ms if x[1][0] == m[1][0])
                f = m[2]
            return f
        line = ('15-18', '5-1', '6-10', '1-5', '10-14', '32-28', '24-27')
        f4 = check(fen, line)
        assert f4 == 'B:WK14,K18,K27:BK5,K28'
        v, _ = e88.analyse(fen)
        fig, i0 = pgp.diagram(fen, t(f'8×8, wit aan zet: {v}', f'8×8, white to move: {v}'), gid=k88)
        h, *_ = pgp.moves(fen, [((x,), '') for x in line], 1, False, 'm1', i0, nbook=len(line))
        phases += [fig, t(
            f'<p>Wit staat in de opstelling van de <a href="val.html">tweede val</a>, bij oneven pariteit, maar is zelf aan '
            f'zet, en dan is het geen val. Met de sterkste zetten stapt wit over naar de andere diagonaal, terwijl zwart '
            f'pendelt: ',
            f'<p>White stands in the set-up of the <a href="val.html">second trap</a>, at odd parity, but is to move itself, '
            f'and then it is no trap. With the strongest moves white moves over to the other diagonal, while black shuttles: ')
            + f'{h}.</p>', '<div style="clear:both"></div>']
        v4, legal4 = e88.analyse(f4)
        fig4, i4 = pgp.diagram(f4, t(f'8×8, zwart aan zet: {v4}', f'8×8, black to move: {v4}'), gid=k88)
        variants = []
        for var, goal in ((('28-32', '27-23'), 'B:WK14,K18,K23:BK5,K32'), (('5-1', '14-9'), 'B:WK9,K18,K27:BK1,K28')):
            assert check(f4, var) == goal and rank_parity(e88, goal) == 0
            hv, *_ = pgp.moves(f4, [((x,), '') for x in var], 4, True, 'm1', i4, nbook=len(var))
            variants.append(hv)
        phases += [fig4, t(
            f'<p>Na 4. {al(24)}-{al(27)} heeft zwart twee pendelzetten, en allebei lopen ze in een val. ',
            f'<p>After 4. {al(24)}-{al(27)} black has two shuttle moves, and both run into a trap. ')
            + variants[0] + t(f': de <a href="val.html">eerste val</a>. ', f': the <a href="val.html">first trap</a>. ')
            + variants[1] + t(f': de tweede val, 180° gedraaid. Wit heeft vijf zetten gedaan, een oneven aantal: het tempo is '
                              f'omgedraaid, en zwart staat aan zet in een val bij even pariteit.</p>',
                              f': the second trap, turned 180°. White has made five moves, an odd number: the tempo has turned '
                              f'over, and black is to move in a trap at even parity.</p>'),
            '<div style="clear:both"></div>']
        return body, phases, pgp

    def vesting(self):
        k89, e89 = self.engine(9, 8, 'playable')
        k99, e99 = self.engine(9, 9, 'unplayable')
        pg = self.page((k89, e89))
        pg9 = self.page((k99, e99))
        fen = e89.fen(['c7', 'e7', 'g7'], ['a7', 'i7'])
        t = self.t
        v, legal = e89.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'9×8, wit aan zet: {v}', f'9×8, white to move: {v}'), gid=k89)
        fb = 'B' + fen[1:]                 # the same position with black to move
        vb, _ = e89.analyse(fb)
        figb, ib = pg.diagram(fb, t(f'9×8, zwart aan zet: {vb}', f'9×8, black to move: {vb}'), gid=k89)
        hb, *_ = pg.moves(fb, self.plan('1. a7-b8 2. g7-f8! b8xd6 3. e7xc5'), 1, True, 'm1', ib)
        hb2, *_ = pg.best_line(pg.pos[int(re.findall(r'data-i="(\d+)"', hb)[-1])]['f'], 40,
                               start=int(re.findall(r'data-i="(\d+)"', hb)[-1]), number=3)
        body = [t('''
<p>Een dam op de twee randvelden van een dubbele hoek kan nooit geslagen worden en heeft altijd een wachtzet. Op 9×8
maakt een zwarte dam in elke dubbele hoek remise in de helft van de stellingen, en welke helft dat is, beslist de
<strong>oppositie</strong>: de pariteit (even of oneven) van de som van de rijnummers van alle stukken wisselt bij elke
zet, en dammen kunnen geen tempo verliezen, dus met zwart aan zet ligt de pariteit vanaf het begin vast.</p>
<p>De vraag kwam op in de draad <a href="https://damforum.nl/viewtopic.php?t=8915"><em>Draughts on a 10x11 or 8x9 board</em></a> op het World Draughts Forum (november 2026): houden twee dammen in beide dubbele hoeken op 9×8 stand, terwijl ze op 8×8 verliezen? Op 9×8 hangt het van de oppositie af; waarom 8×8 altijd verloren is, staat bij de <a href="val.html">vallen op 8×8</a>.</p>
<p>Met zwart aan zet en de zwarte dammen op de randvelden van de twee dubbele hoeken (rustige stellingen, niemand kan
slaan) is op 9×8 bij even pariteit elke stelling remise (8.228 stellingen) en bij oneven pariteit geen enkele (8.244).</p>''', '''
<p>A king on the two edge squares of a double corner can never be captured and always has a waiting move. On 9×8 a
black king in each double corner draws in half of the positions, and that half is decided by
<strong>opposition</strong>: the parity (even or odd) of the sum of the rank numbers of all pieces flips with every
move, and kings cannot lose a tempo, so with black to move the parity is fixed from the start.</p>
<p>The question came up in the thread <a href="https://damforum.nl/viewtopic.php?t=8915"><em>Draughts on a 10x11 or 8x9 board</em></a> on the World Draughts Forum (November 2026): do two kings in both double corners hold on 9×8, while they lose on 8×8? On 9×8 it depends on the opposition; why 8×8 is always lost is explained at the <a href="val.html">traps on 8×8</a>.</p>
<p>With black to move and the black kings on the edge squares of the two double corners (quiet positions, nobody can
capture), on 9×8 every position is drawn at even parity (8,228 positions) and none at odd parity (8,244).</p>'''),
                fig,
                t(f'<p>Zwart op a7 en i7 (pendelend met b8 en h8), wit op c7, e7 en g7. Met wit aan zet houdt elke zet '
                  f'remise: zwart pendelt gewoon a7-b8-a7 en/of i7-h8-i7. ',
                  f'<p>Black on a7 and i7 (shuttling with b8 and h8), white on c7, e7 and g7. With white to move every move '
                  f'draws: black just shuttles a7-b8-a7 and/or i7-h8-i7. ') + f'{self.moves_list(pg, i0, legal, "1. ")}.</p>',
                '<div style="clear:both"></div>', figb,
                t(f'<p>Dezelfde stelling met zwart aan zet: zwart staat in zetdwang en verliest in {vb[1:]} ply: {hb} en 2 '
                  f'tegen 1 wint: ',
                  f'<p>The same position with black to move: black is in zugzwang and loses in {vb[1:]} plies: {hb} and 2-1 '
                  f'wins: ') + f'{hb2}</p>', '<div style="clear:both"></div>', t('''
<p>Wat beslist, is de afstand tussen de hoeken van de dubbele hoeken die zwart bezet (<em>D</em> = grootste,
<em>d</em> = kleinste coördinaatverschil; <em>d</em> = 0 als ze aan dezelfde kant liggen), niet of ze aan dezelfde
kant liggen:</p>
<table class="tbl"><tr><th>Borden</th><th>D × d</th><th>Vesting</th></tr>
<tr><td>7×6, 8×7, 9×7 onspeelbare hoeken (verticaal paar)</td><td>6 × 0</td><td>valt</td></tr>
<tr><td>8×8</td><td>7 × 7</td><td>valt</td></tr>
<tr><td>9×8, 10×9, 9×9 onspeelbare hoeken (zelfde kant), 9×7 onspeelbare hoeken (horizontaal)</td><td>8 × 0</td><td>oppositie beslist</td></tr>
<tr><td>9×7 onspeelbare hoeken (diagonaal paar)</td><td>8 × 6</td><td>oppositie beslist</td></tr>
<tr><td>9×9 onspeelbare hoeken (diagonaal paar)</td><td>8 × 8</td><td>houdt</td></tr>
<tr><td>10×8, 10×10, 11×10, 12×10</td><td>≥ 9</td><td>houdt</td></tr></table>
<p><em>D</em> ≤ 7: de vesting valt · <em>D</em> = 8: de oppositie beslist, tenzij ook <em>d</em> = 8 ·
<em>D</em> ≥ 9: de vesting houdt.</p>''', '''
<p>What decides is the distance between the corners of the double corners black occupies (<em>D</em> = largest,
<em>d</em> = smallest coordinate difference; <em>d</em> = 0 when they are on the same side), not whether they lie on the
same side:</p>
<table class="tbl"><tr><th>Boards</th><th>D × d</th><th>Fortress</th></tr>
<tr><td>7×6, 8×7, 9×7 unplayable corners (vertical pair)</td><td>6 × 0</td><td>falls</td></tr>
<tr><td>8×8</td><td>7 × 7</td><td>falls</td></tr>
<tr><td>9×8, 10×9, 9×9 unplayable corners (same side), 9×7 unplayable corners (horizontal)</td><td>8 × 0</td><td>opposition decides</td></tr>
<tr><td>9×7 unplayable corners (diagonal pair)</td><td>8 × 6</td><td>opposition decides</td></tr>
<tr><td>9×9 unplayable corners (diagonal pair)</td><td>8 × 8</td><td>holds</td></tr>
<tr><td>10×8, 10×10, 11×10, 12×10</td><td>≥ 9</td><td>holds</td></tr></table>
<p><em>D</em> ≤ 7: the fortress falls · <em>D</em> = 8: opposition decides, unless also <em>d</em> = 8 ·
<em>D</em> ≥ 9: the fortress holds.</p>'''),
                ]
        # the other of the two traps on 9x8 (the trap of the previous page is the first), on a page of its own
        pgp = self.page((k89, e89))
        extra = [t('<p>De val op de <a href="vesting.html">vorige pagina</a> is een van de twee vallen op 9×8. De andere, '
                   'val 2:</p>', '<p>The trap on the <a href="vesting.html">previous page</a> is one of the two traps on '
                   '9×8. The other one, trap 2:</p>')] + self.trap_blocks(
            pgp, k89, e89, '9×8', '', [(['d8', 'e7', 'f8'], ['b8', 'h8'])],
            {'b8': 'a7', 'a7': 'b8', 'h8': 'i7', 'i7': 'h8'}, 2) + [t(
            '<p>De twee vallen gaan in elkaar over met een even aantal zetten: van de val van de vorige pagina (wit op c7, e7 '
            'en g7, zwart op a7 en i7) naar val 2 doen twee witte dammen elk één stap (c7-d8 en g7-f8) en pendelen beide '
            'zwarte dammen. Ze liggen dus bij dezelfde pariteit (oneven). Wit kan het tempo niet wisselen, zoals op 8×8 met de '
            'overstap naar de andere diagonaal. De winst is hier niet strategisch: de oppositie beslist.</p>',
            '<p>The two traps turn into each other with an even number of moves: from the trap of the previous page (white on '
            'c7, e7 and g7, black on a7 and i7) to trap 2 two white kings take one step each (c7-d8 and g7-f8) and both black '
            'kings shuttle. They therefore lie at the same parity (odd). White cannot change the tempo, as on 8×8 with the '
            'change-over to the other diagonal. The win here is not strategic: the opposition decides.</p>')] \
            + self.no_tempo(pgp, k89, e89, '9×8', ['c7', 'e7', 'g7'], ['a7', 'i7'], '1. c7-d8 a7-b8 2. g7-f8 b8-a7',
                            t('de witte opstelling van val 2', 'the white set-up of trap 2'))
        self.write('vesting-pariteit.html', None, '\n'.join(extra), pgp, k89)
        rule = next(k for k, x in enumerate(body) if isinstance(x, str) and 'D × d' in x)
        self.write('vesting.html', None, '\n'.join(body[:rule]), pg, k89)
        self.write('vesting-9x9.html', None, '\n'.join(self.fortress99(pg9, k99, e99)), pg9, k99)
        # 7x7 with unplayable corners: the longest win (quiet start, white to move); black flees to two double corners on
        # the g-file and white drives one king out of its double corner
        k77, e77 = self.engine(7, 7, 'unplayable')
        pg77 = self.page((k77, e77))
        f77 = e77.fen(['b7', 'd7', 'a6'], ['d5', 'c4'], 'W')
        v77, _ = e77.analyse(f77)
        line77 = ('1. d7-c6 d5-e4 2. c6-b5 c4-d5 3. b5-a4 d5-e6 4. b7-c6 e6-f5 5. a6-b7 f5-e6 6. c6-d7 e6-f5 7. b7-c6 e4-f3 '
                  '8. c6-d5 f3-g4 9. d5-e6 f5-g6 10. d7-c6 g6-f5 11. e6-f7 f5-g6 12. c6-d5 g4-f3 13. a4-b5 f3-g2 14. b5-c4 g2-f3 '
                  '15. c4-d3 f3-g2 16. d5-e4 g2-f1 17. e4-f3 f1-g2 18. d3-e4 g2-f1 19. f3-g2! f1-e2 20. g2-f1 e2-d1 21. e4-d5 d1-c2 '
                  '22. f1-e2 c2-b3 23. e2-d3 b3-a2 24. d3-c4 a2-b1 25. c4-b3 g6-f5 26. b3-c2! b1xd3 27. f7-g6 f5-e6 28. d5xf7')
        g = f77
        for (mv,), _ in self.plan(line77):     # white plays a strongest move, black a longest defence
            ms = e77.analyse(g)[1]
            m = next(x for x in ms if x[0] == mv)
            w = g[0] == 'W'
            assert m[1][0] == ('W' if w else 'L') and int(m[1][1:]) == (min if w else max)(
                int(x[1][1:]) for x in ms if x[1][0] == m[1][0]), (mv, m[1])
            g = m[2]
        assert [len([x for x in side[1:].split(',') if x]) for side in g.split(':')[1:]] == [2, 1] and e77.analyse(g)[0][0] == 'L'
        # the quiet start with the longest win, and black in two double corners: all lost but 16 (a shot)
        fig77, i77 = pg77.diagram(f77, t(f'7×7, wit aan zet: {v77}', f'7×7, white to move: {v77}'), gid=k77)
        h77, *_ = pg77.moves(f77, self.plan(line77), 1, False, 'm1', i77)
        body77 = [t(f'''
<p>Op 7×7 met onspeelbare hoekvelden is elke hoek een dubbele hoek: b1/a2, f1/g2, a6/b7 en g6/f7. De dubbele hoeken
liggen 6 uit elkaar, aan dezelfde kant en diagonaal. Van de 28.276 rustige stellingen met zwart aan zet en beide zwarte
dammen op de randvelden van twee dubbele hoeken zijn er maar 16 remise, en daarin heeft zwart een schot.</p>
<p>De langste winst zonder slag voor beide kanten in de beginstelling duurt {v77[1:]} ply. Zwart vlucht met beide dammen
naar de dubbele hoeken langs de g-lijn: na 13. … f3-g2 staan ze op g6 en g2. Wit breekt de vesting door een zwarte dam
uit zijn dubbele hoek te verdrijven: met 19. f3-g2! bezet wit het vrije randveld naast de zwarte dam op f1, die gaat de hoek
uit (19. … f1-e2), en na 20. g2-f1 kan hij niet meer terug. Hij vlucht naar de dubbele hoek b1/a2, maar daar dwingt wit
met een offer de beslissing af: 26. b3-c2! b1×d3 27. f7-g6 f5-e6 28. d5×f7, en 2 tegen 1 wint.</p>''', f'''
<p>On 7×7 with unplayable corner squares every corner is a double corner: b1/a2, f1/g2, a6/b7 and g6/f7. The double
corners are 6 apart, on the same side and diagonally. Of the 28,276 quiet positions with black to move and both black
kings on the edge squares of two double corners only 16 are drawn, and in those black has a shot.</p>
<p>The longest win without a capture for either side in the start position takes {v77[1:]} plies. Black flees with both
kings to the double corners along the g-file: after 13. … f3-g2 they stand on g6 and g2. White breaks the fortress by
driving a black king out of its double corner: with 19. f3-g2! white occupies the free edge square next to the black king
on f1, which leaves the corner (19. … f1-e2), and after 20. g2-f1 it cannot come back. It flees to the double corner
b1/a2, but there white forces the decision with a sacrifice: 26. b3-c2! b1×d3 27. f7-g6 f5-e6 28. d5×f7, and 2 against 1
wins.</p>'''), fig77, f'<p class="var">{h77}</p>', '<div style="clear:both"></div>',
                  t('<p>Op 8×7 bekijken we de twee dubbele hoeken langs de zijde van 7 apart.</p>',
                    '<p>On 8×7 we look at the two double corners along the side of 7 on their own.</p>')]
        self.write('langste-7x7.html', None, '\n'.join(body77), pg77, k77)
        # 8x7: one pair of double corners, along the side of 7 (6 apart, closer than on 8x8): the fortress falls
        k87, e87 = self.engine(8, 7, 'unplayable')
        pg7 = self.page((k87, e87))
        partner = {'b1': 'a2', 'a2': 'b1', 'h1': 'i2', 'i2': 'h1', 'b7': 'a6', 'a6': 'b7', 'h7': 'i6', 'i6': 'h7'}
        name = '8×7'
        OW, OB = ['b3', 'c4', 'd7'], ['b1', 'a6']           # a trap at odd parity
        EW, EB = ['b3', 'c4', 'b5'], ['b1', 'b7']           # even parity: no trap, white drives a black king out
        body7 = [t('''
<p>Op 8×8 liggen de twee dubbele hoeken 7 velden uit elkaar, en daar valt de vesting altijd. Op
<a href="langste-7x7.html">7×7</a> vluchtte zwart naar twee dubbele hoeken langs een zijde van 7, maar 6 uit elkaar. Op
8×7 zijn er precies twee dubbele hoeken, b1/a2 en b7/a6, allebei langs de zijde van 7, en
ze liggen maar 6 uit elkaar. (Met speelbare in plaats van onspeelbare hoekvelden is 8×7 het spiegelbeeld, met de dubbele
hoeken langs de h-lijn.)</p>
<p>Met zwart aan zet en beide zwarte dammen op de randvelden van hun dubbele hoeken (rustige stellingen) is bij oneven
pariteit elke stelling verloren (3.172), en bij even pariteit alle op 4 na (3.153 van de 3.157). Maar het gaat op twee
verschillende manieren: bij oneven pariteit zijn er 8 vallen, bij even pariteit geen enkele.</p>''', '''
<p>On 8×8 the two double corners are 7 squares apart, and there the fortress always falls. On
<a href="langste-7x7.html">7×7</a> black fled to two double corners along a side of 7, only 6 apart. On 8×7 there are
exactly two double corners, b1/a2 and b7/a6, both along the side of 7, and they are
only 6 apart. (With playable instead of unplayable corner squares 8×7 is the mirror image, with the double corners along
the h-file.)</p>
<p>With black to move and both black kings on the edge squares of their double corners (quiet positions) every position is
lost at odd parity (3,172), and all but 4 at even parity (3,153 of 3,157). But in two different ways: at odd parity there
are 8 traps, at even parity not a single one.</p>''')]
        body7.append(t('<p>Bij oneven pariteit breekt wit de vesting met een val, zoals op 8×8, bijvoorbeeld deze:</p>',
                       '<p>At odd parity white breaks the fortress with a trap, as on 8×8, for example this one:</p>'))
        body7 += self.trap_blocks(pg7, k87, e87, name, '', [(OW, OB)], partner)
        # even parity: white occupies the free edge square next to a black king, which then has to leave its double corner
        fe = e87.fen(EW, EB, 'B')
        ve, legale = e87.analyse(fe)
        fige, ie = pg7.diagram(fe, t(f'{name}, zwart aan zet (even pariteit): {ve}', f'{name}, black to move (even parity): {ve}'),
                               gid=k87)
        lines = []
        for line in ('1. b7-a6 2. b3-a2! b1-c2 3. a2-b1 c2-b3 4. c4xa2 a6xc4', '1. b1-a2 2. b5-a6! b7-c6 3. a6-b7 c6-b5 4. c4xa6 a2xc4'):
            h, _, f_end, _ = pg7.moves(fe, self.plan(line), 1, True, 'm1', ie)
            assert f_end[0] == 'W' and e87.analyse(f_end)[0].startswith('W') and all(
                len([x for x in side[1:].split(',') if x]) == n for side, n in zip(f_end.split(':')[1:], (2, 1)))
            lines.append(h)
        # after the invasion, the other waiting move (with the other king) loses even faster
        after = e87.analyse(e87.fen(['a2', 'c4', 'b5'], ['b1', 'a6'], 'B'))[1]
        val = {m[0]: m[1] for m in after}
        assert set(val) == {'b1-c2', 'a6-b7'} and all(v[0] == 'L' for v in val.values()) and int(val['a6-b7'][1:]) < int(val['b1-c2'][1:])
        body7 += [fige, t('<p>Bij even pariteit is er geen val. Toch is vrijwel alles verloren, want de dubbele hoeken liggen '
                          'zo dicht bij elkaar dat wit erin kan binnendringen. Pendelt de ene zwarte dam, dan bezet wit het '
                          'vrije randveld naast de andere: die heeft geen wachtzet meer en moet de dubbele hoek uit (de '
                          'wachtzet met de eerste dam verliest nog sneller). Daarna gaat wit zelf op het randveld staan dat '
                          'de zwarte dam verliet, en die kan niet meer terug. ',
                          '<p>At even parity there is no trap. Still almost everything is lost, because the double corners are '
                          'so close together that white can invade them. When the one black king shuttles, white occupies the '
                          'free edge square next to the other one: it has no waiting move left and has to leave the double '
                          'corner (the waiting move with the first king loses even faster). Then white steps onto the edge square the '
                          'black king left, and it cannot come back. ')
                  + lines[0] + t(', en 2 tegen 1 wint. Pendelt zwart in de onderste hoek, dan gaat het net zo in de bovenste: ',
                                 ', and 2 against 1 wins. If black shuttles in the lower corner, it goes the same way in the '
                                 'upper one: ')
                  + lines[1] + t('. Alle zetten van zwart: ', '. All moves of black: ')
                  + self.moves_list(pg7, ie, legale, '1. … ') + '.</p>', '<div style="clear:both"></div>']
        body7.append(t('<p>De vesting valt dus bij beide pariteiten, op de <a href="val.html">twee manieren</a> om de vesting '
                       'te breken, en zonder tempowissel: bij oneven pariteit met een ruil (een val), bij even pariteit doordat '
                       'een zwarte dam zijn dubbele hoek uit moet en niet meer terug kan. Alleen bij zwart op a2 en a6 en wit '
                       'op c4, b3 of b5 en d3 of d5 (wit op b5, c4 en d5 bijvoorbeeld) is het remise.</p>',
                       '<p>The fortress therefore falls at both parities, in the <a href="val.html">two ways</a> to break the '
                       'fortress, and without a tempo change: at odd parity with an exchange (a trap), at even parity because a '
                       'black king has to leave its double corner and cannot come back. Only with black on a2 and a6 and white '
                       'on c4, b3 or b5 and d3 or d5 (white on b5, c4 and d5 for example) is it a draw.</p>'))
        for W, B, even in ((EW, EB, True), (OW, OB, False)):
            assert (rank_parity(e87, e87.fen(W, B, 'B')) == 0) == even
        for W in (['b5', 'c4', 'd5'], ['b5', 'c4', 'd3'], ['b3', 'c4', 'd5'], ['b3', 'c4', 'd3']):
            assert e87.analyse(e87.fen(W, ['a2', 'a6'], 'B'))[0] == 'D'
        body7.append(t('<p>Wat er gebeurt als de dubbele hoeken 8 uit elkaar liggen, zien we op <a href="vesting.html">9×8</a>.</p>',
                       '<p>What happens when the double corners are 8 apart, we see on <a href="vesting.html">9×8</a>.</p>'))
        self.write('vesting-8x7.html', None, '\n'.join(body7), pg7, k87)
        # the distance rule, with the two pairs of 9x7 that are 8 apart
        k97, e97 = self.engine(9, 7, 'unplayable')
        name = t('9×7 onspeelbare hoeken', '9×7 unplayable corners')
        pga = self.page((k97, e97))
        rulebody = [t('<p>Op 9×7 met onspeelbare hoekvelden zijn er vier dubbele hoeken: b1/a2, h1/i2, b7/a6 en h7/i6. '
                      'Langs de zijde van 7 liggen ze 6 uit elkaar, zoals op <a href="vesting-8x7.html">8×7</a>; de twee '
                      'andere soorten paren liggen 8 uit elkaar: langs de lange kant, zoals op 9×8, en diagonaal. Bij beide zijn er 2 vallen bij even pariteit en geen bij oneven: de oppositie beslist. '
                      'Langs de lange kant staan de drie witte dammen op een rij tussen de zwarte, als op 9×8:</p>',
                      '<p>On 9×7 with unplayable corner squares there are four double corners: b1/a2, h1/i2, b7/a6 and '
                      'h7/i6. Along the side of 7 they are 6 apart, as on <a href="vesting-8x7.html">8×7</a>; the two other '
                      'kinds of pairs are 8 apart: along the long side, as on 9×8, and diagonally. '
                      'For both there are 2 traps at even parity and none at odd: the opposition decides. Along the long side '
                      'the three white kings stand in a row between the black ones, as on 9×8:</p>')]
        rulebody += self.trap_blocks(pga, k97, e97, name, '', [(['c2', 'e2', 'g2'], ['a2', 'i2'])], partner)
        rulebody.append(t('<p>Diagonaal: op <a href="vesting-9x9.html">9×9</a> is de diagonaal te lang, op 9×7 niet. De drie '
                          'witte dammen staan op een rij op de verbindingsdiagonaal, net als op 8×8, maar de val werkt maar bij '
                          'één pariteit:</p>',
                          '<p>Diagonally: on <a href="vesting-9x9.html">9×9</a> the diagonal is too long, on 9×7 it is not. The '
                          'three white kings stand in a row on the connecting diagonal, as on 8×8, but the trap works at one '
                          'parity only:</p>'))
        rulebody += self.trap_blocks(pga, k97, e97, name, '', [(['d3', 'e4', 'f5'], ['b1', 'h7'])], partner, 2)
        for W, B in ((['c2', 'e2', 'g2'], ['a2', 'i2']), (['d3', 'e4', 'f5'], ['b1', 'h7'])):
            assert rank_parity(e97, e97.fen(W, B, 'B')) == 0
        self.write('afstandsregel.html', None, body[rule] + '\n' + '\n'.join(rulebody), pga, k97)
        # the conclusion (for now: the traps per board)
        t = self.t
        self.write('conclusie.html', None, t(
            '<p>De vallen per bord, met zwart aan zet en beide zwarte dammen op de randvelden van twee dubbele hoeken '
            '(geteld met <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>):</p>'
            '<table class="tbl"><tr><th>Bord</th><th>Vallen bij even pariteit</th><th>Vallen bij oneven pariteit</th><th>Gevolg</th></tr>'
            '<tr><td>8×8</td><td>2</td><td>2</td><td>de vesting valt altijd</td></tr>'
            '<tr><td>9×8, 10×9</td><td>0</td><td>2</td><td>de oppositie beslist: de helft remise</td></tr>'
            '<tr><td>10×10</td><td>0</td><td>0</td><td>de vesting houdt</td></tr></table>'
            '<p>Zwart in beide dubbele hoeken, zwart aan zet (rustige stellingen):</p>'
            '<table class="tbl"><tr><th>Bord</th><th>Even pariteit</th><th>Oneven pariteit</th></tr>'
            '<tr><td>9×8</td><td>100% remise (8.228)</td><td>0% remise (8.244)</td></tr>'
            '<tr><td>10×9</td><td>100% (18.420)</td><td>0% (18.440)</td></tr>'
            '<tr><td>11×10</td><td>100% (37.030)</td><td>100% (37.050)</td></tr></table>'
            '<p>Zie <a href="val.html">de val</a> en <a href="langste-7x7.html">3 tegen 2 op andere borden</a>.</p>',
            '<p>The traps per board, with black to move and both black kings on the edge squares of two double corners '
            '(counted with <a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>):</p>'
            '<table class="tbl"><tr><th>Board</th><th>Traps at even parity</th><th>Traps at odd parity</th><th>Consequence</th></tr>'
            '<tr><td>8×8</td><td>2</td><td>2</td><td>the fortress always falls</td></tr>'
            '<tr><td>9×8, 10×9</td><td>0</td><td>2</td><td>the opposition decides: half are draws</td></tr>'
            '<tr><td>10×10</td><td>0</td><td>0</td><td>the fortress holds</td></tr></table>'
            '<p>Black in both double corners, black to move (quiet positions):</p>'
            '<table class="tbl"><tr><th>Board</th><th>Even parity</th><th>Odd parity</th></tr>'
            '<tr><td>9×8</td><td>100% draws (8,228)</td><td>0% draws (8,244)</td></tr>'
            '<tr><td>10×9</td><td>100% (18,420)</td><td>0% (18,440)</td></tr>'
            '<tr><td>11×10</td><td>100% (37,030)</td><td>100% (37,050)</td></tr></table>'
            '<p>See <a href="val.html">the trap</a> and <a href="langste-7x7.html">3 against 2 on other boards</a>.</p>'),
            self.page((k89, e89)), None)


    def shuttles(self, pg, gid, e, i0, legal, shuttle):
        """the shuttle moves of black (algebraic, e.g. 'h1-i2') with the forced exchange that punishes each, if any"""
        out = []
        for m in legal:
            if m[0] not in shuttle:
                continue
            i1 = pg.child(i0, m)
            parts = [f'<a class="m1" href="#" data-i="{i1}">1. … {pg.label(gid, m)}</a>']
            ex = self.exchange(e, m[2])
            if ex:
                i = i1
                for k, mv in enumerate(ex):
                    i = pg.child(i, mv)
                    pre = '2. ' if k == 0 else ('3. ' if k == 2 else '')
                    parts.append(f'<a class="m1" href="#" data-i="{i}">{pre}{pg.label(gid, mv)}</a>')
            out.append((m[0], ex is not None, ' '.join(parts)))
        return out

    def trap_blocks(self, pg, gid, e, board, extra, traps_, partner, first=1):
        """one diagram per trap (black to move): every shuttle move of black and the exchange that punishes it"""
        t = self.t
        out = []
        for k, (W, B) in enumerate(traps_, first - 1):
            fen = e.fen(W, B, 'B')
            v, legal = e.analyse(fen)
            name = f'{board} {extra}' if extra else board
            fig, i0 = pg.diagram(fen, t(f'{name}, zwart aan zet: {v}', f'{name}, black to move: {v}'), gid=gid)
            sh = self.shuttles(pg, gid, e, i0, legal, {f'{b}-{partner[b]}' for b in B})
            assert len(sh) == 2 and all(ok for _, ok, _ in sh), (fen, sh)
            # a trap: every other black move (leaving a double corner) loses material at once too
            for m in legal:
                if m[0] in {x for x, _, _ in sh}:
                    continue
                assert self.exchange(e, m[2]) or any(
                    len([x for x in w[2].split(':')[2][1:].split(',') if x]) < 2 and w[1].startswith('W')
                    for w in e.analyse(m[2])[1]), (fen, m[0])
            out += [fig, t(f'<p>Val {k + 1}: wit op {", ".join(W)}, zwart op {B[0]} en {B[1]}: ',
                           f'<p>Trap {k + 1}: white on {", ".join(W)}, black on {B[0]} and {B[1]}: ')
                    + '; '.join(x for _, _, x in sh)
                    + t(', en 2 tegen 1 wint. Alle zetten van zwart: ', ', and 2 against 1 wins. All moves of black: ')
                    + self.moves_list(pg, i0, legal, '1. … ') + '.</p>', '<div style="clear:both"></div>']
        return out

    def no_tempo(self, pg, gid, e, name, W, B, line, target):
        """from a trap set-up with white to move, white steps over to another trap while black shuttles: an even number
        of moves, so white is to move again and it stays a draw"""
        t = self.t
        fen = e.fen(W, B, 'W')
        v, _ = e.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'{name}, wit aan zet: {v}', f'{name}, white to move: {v}'), gid=gid)
        h, i_end, f_end, _ = pg.moves(fen, self.plan(line), 1, False, 'm1', i0, nbook=len(self.plan(line)))
        v2, _ = e.analyse(f_end)
        assert v == v2 == 'D' and f_end[0] == 'W'
        fig2, _ = pg.diagram(f_end, t(f'{name}, wit aan zet: {v2}', f'{name}, white to move: {v2}'), gid=gid)
        return [fig, t(f'<p>Met wit aan zet is de opstelling van de val geen val. Stapt wit over naar {target}, terwijl zwart '
                       f'pendelt, ', f'<p>With white to move the set-up of the trap is no trap. If white steps over to '
                       f'{target} while black shuttles, ') + f'{h}' + t(
                       ', dan is dat na een even aantal zetten bereikt, en is wit weer aan zet: het tempo is niet '
                       'omgedraaid, en het blijft remise.</p>',
                       ', it is reached after an even number of moves, and white is to move again: the tempo has not turned '
                       'over, and it stays a draw.</p>'), '<div style="clear:both"></div>',
                fig2, t('<p>Wit staat in de opstelling van val 2, maar zwart pendelt met dezelfde dam heen en terug en '
                        'staat niet in de val; wit is aan zet: remise. (Zou zwart met beide dammen pendelen, dan stond hij '
                        'wel in val 2, met wit aan zet, en dwingt wit meteen de ruil af.)</p>',
                        '<p>White stands in the set-up of trap 2, but black shuttles with the same king to and fro and is '
                        'not in the trap; white is to move: a draw. (Were black to shuttle with both kings, he would stand in '
                        'trap 2, with white to move, and white forces the exchange at once.)</p>'), '<div style="clear:both"></div>']

    def fortress99(self, pg, k99, e99):
        """9x9 with unplayable corners: the trap works for two double corners on the same side, not for the diagonal pair"""
        t = self.t
        body = [t('''
<p>Op 9×9 met onspeelbare hoekvelden is elke hoek een dubbele hoek: b1/a2, h1/i2, a8/b9 en h9/i8. Twee dubbele hoeken
aan dezelfde kant liggen even ver uit elkaar als op <a href="vesting.html">9×8</a>, en de <a href="val.html">val</a>
werkt er net zo: bij één van de twee pariteiten, dus de oppositie beslist. Dat geldt op elk bord van 9 breed: op 9×10
heeft het paar langs de rand van 9 dezelfde vallen, en op 9×11 met onspeelbare hoeken elk van de twee paren langs een
rand van 9. Twee dubbele hoeken die diagonaal tegenover elkaar liggen, werken niet: daar is bij geen van beide
pariteiten een val. De <a href="afstandsregel.html">afstandsregel</a> op de volgende pagina vat dat samen. De database
telt de vallen met zwart aan zet en beide zwarte dammen op de randvelden van hun dubbele hoeken:</p>
<table class="tbl"><tr><th>Zwart in</th><th>Vallen bij even pariteit</th><th>Vallen bij oneven pariteit</th><th>Gevolg</th></tr>
<tr><td>twee dubbele hoeken aan dezelfde kant</td><td colspan="2">2 bij één van de twee pariteiten</td><td>de oppositie beslist</td></tr>
<tr><td>twee diagonaal tegenover elkaar</td><td>0</td><td>0</td><td>de vesting houdt</td></tr></table>
<p>Per paar dubbele hoeken zijn er 12.100 beginstellingen bij elke pariteit.</p>''', '''
<p>On 9×9 with unplayable corner squares every corner is a double corner: b1/a2, h1/i2, a8/b9 and h9/i8. Two double
corners on the same side lie as far apart as on <a href="vesting.html">9×8</a>, and the <a href="val.html">trap</a>
works there just the same: at one of the two parities, so the opposition decides. That holds on every board 9 wide: on
9×10 the pair along the edge of 9 has the same traps, and on 9×11 with unplayable corners each of the two pairs along an
edge of 9. Two diagonally opposite double corners do not work: there is no trap at either parity. The
<a href="afstandsregel.html">distance rule</a> on the next page sums this up. The database counts the traps with black
to move and both black kings on the edge squares of their double corners:</p>
<table class="tbl"><tr><th>Black in</th><th>Traps at even parity</th><th>Traps at odd parity</th><th>Consequence</th></tr>
<tr><td>two double corners on the same side</td><td colspan="2">2 at one of the two parities</td><td>the opposition decides</td></tr>
<tr><td>two diagonally opposite ones</td><td>0</td><td>0</td><td>the fortress holds</td></tr></table>
<p>Per pair of double corners there are 12,100 starting positions at each parity.</p>''')]
        # the two traps for the pair of double corners along the i-file (h1/i2 and h9/i8)
        body.append(t('<p>Voor twee dubbele hoeken aan dezelfde kant, h1/i2 en h9/i8, zijn er twee vallen. In elk ervan '
                      'wordt elke pendelzet van zwart gestraft. Net als op 9×8 werken ze maar bij één van de twee pariteiten: '
                      'de oppositie beslist.</p>',
                      '<p>For two double corners on the same side, h1/i2 and h9/i8, there are two traps. In each of them '
                      'every shuttle move of black is punished. As on 9×8 they work at only one of the two parities: the '
                      'opposition decides.</p>'))
        body += self.trap_blocks(pg, k99, e99, '9×9', t('onspeelbare hoeken', 'unplayable corners'),
                                 [(['h3', 'h5', 'h7'], ['h1', 'h9']), (['i4', 'h5', 'i6'], ['i2', 'i8'])],
                                 {'h1': 'i2', 'i2': 'h1', 'h9': 'i8', 'i8': 'h9'})
        body.append(t('<p>Ook hier gaan de twee vallen in elkaar over met een even aantal zetten (h3-i4 en h7-i6; de zwarte '
                      'dammen pendelen), en liggen ze bij dezelfde pariteit. Wit kan het tempo niet wisselen: de '
                      'winst is niet strategisch maar hangt af van de oppositie.</p>',
                      '<p>Here too the two traps turn into each other with an even number of moves (h3-i4 and h7-i6; the '
                      'black kings shuttle), and they lie at the same parity. White cannot change the tempo: the win is '
                      'not strategic but depends on the opposition.</p>'))
        body += self.no_tempo(pg, k99, e99, t('9×9 onspeelbare hoeken', '9×9 unplayable corners'), ['h3', 'h5', 'h7'],
                              ['h1', 'h9'], '1. h3-i4 h9-i8 2. h7-i6 i8-h9',
                              t('de witte opstelling van val 2', 'the white set-up of trap 2'))
        # the diagonal pair: the 8x8 trap (three kings in a row on the connecting diagonal) catches only one shuttle
        figs = []
        for W in (['d3', 'e4', 'f5'], ['e4', 'f5', 'g6']):
            fen = e99.fen(W, ['b1', 'i8'], 'B')
            v, legal = e99.analyse(fen)
            fig, i0 = pg.diagram(fen, t(f'9×9 onspeelbare hoeken, zwart aan zet: {v}', f'9×9 unplayable corners, black to move: {v}'), gid=k99)
            sh = self.shuttles(pg, k99, e99, i0, legal, {'b1-a2', 'i8-h9'})
            assert v == 'D' and len(sh) == 2 and sorted(ok for _, ok, _ in sh) == [False, True]
            hit = next(x for _, ok, x in sh if ok)
            free = next(x for _, ok, x in sh if not ok)
            figs.append((v, fig, i0, legal, hit, free))
        (v1, fig1, i1, legal1, hit1, free1), (v2, fig2, i2, legal2, hit2, free2) = figs
        body += [t('''
<p>Bij twee diagonaal tegenover elkaar liggende dubbele hoeken staan de zwarte dammen op b1 en i8 aan de twee
uiteinden van de diagonaal b1–i8. Op 8×8 staat de val precies zo: drie witte dammen op een rij midden op de
verbindingsdiagonaal, met 5 velden tussen de zwarte dammen. Op 9×9 zijn dat er 6, en die afstand is te lang: de drie
dammen kunnen maar één van de twee pendelzetten straffen.</p>''', '''
<p>With two diagonally opposite double corners the black kings on b1 and i8 stand at the two ends of the diagonal
b1–i8. On 8×8 the trap stands exactly like that: three white kings in a row in the middle of the connecting diagonal,
with 5 squares between the black kings. On 9×9 there are 6, and that distance is too long: the three kings can punish
only one of the two shuttle moves.</p>'''), fig1,
                 t(f'<p>Met wit op d3, e4 en f5 ({v1}) wordt {hit1} gestraft, maar {free1} niet: de dammen staan te ver '
                   f'van b1. Alle zetten van zwart: ',
                   f'<p>With white on d3, e4 and f5 ({v1}), {hit1} is punished, but {free1} is not: the kings are too far '
                   f'from b1. All moves of black: ') + self.moves_list(pg, i1, legal1, '1. … ') + '.</p>',
                 '<div style="clear:both"></div>', fig2,
                 t(f'<p>Eén veld verder, op e4, f5 en g6 ({v2}), is het omgekeerd: {hit2} wordt gestraft, {free2} niet. '
                   f'Alle zetten van zwart: ',
                   f'<p>One square further, on e4, f5 and g6 ({v2}), it is the other way round: {hit2} is punished, '
                   f'{free2} is not. All moves of black: ') + self.moves_list(pg, i2, legal2, '1. … ')
                 + t('. Bij geen van beide pariteiten straft één opstelling beide pendelzetten, dus de vesting houdt.</p>',
                     '. At neither parity does one set-up punish both shuttle moves, so the fortress holds.</p>'),
                 '<div style="clear:both"></div>']
        return body


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
        # pages that were renamed or merged
        (out / 'vesting-9x7.html').write_text('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" '
                                              'content="0; url=vesting-8x7.html"><link rel="canonical" href="vesting-8x7.html">'
                                              '<title>Checkers</title><a href="vesting-8x7.html">vesting-8x7.html</a>\n')
        (out / 'val-race.html').write_text('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" '
                                           'content="0; url=val.html"><link rel="canonical" href="val.html">'
                                           '<title>Checkers</title><a href="val.html">val.html</a>\n')
        (out / 'dubbele-hoeken.html').write_text('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" '
                                                 'content="0; url=drie-langste.html"><link rel="canonical" href="drie-langste.html">'
                                                 '<title>Checkers</title><a href="drie-langste.html">drie-langste.html</a>\n')
        (out / 'val-fasen.html').write_text('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" '
                                            'content="0; url=val-pariteit.html"><link rel="canonical" href="val-pariteit.html">'
                                            '<title>Checkers</title><a href="val-pariteit.html">val-pariteit.html</a>\n')


if __name__ == '__main__':
    main()
