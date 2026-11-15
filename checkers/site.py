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

# the chapters, each with one page per section: (Dutch, English, [(file, Dutch, English)])
CHAPTERS = [
    ('2 tegen 1', '2 against 1', [
        ('twee-langste.html', 'Gewonnen, op twee uitzonderingen na', 'Won, with two exceptions'),
        ('twee-blokkade.html', 'Uitzondering 1: de blokkade aan de rand', 'Exception 1: the blockade at the edge'),
        ('twee-4x4.html', 'Uitzondering 2: remise op 4×4', 'Exception 2: a draw on 4×4')]),
    ('3 tegen 2', '3 against 2', [
        ('drie-langste.html', 'De langste winst op 8×8', 'The longest win on 8×8'),
        ('blokkades.html', 'De blokkades', 'The blockades')]),
    ('De val', 'The trap', [
        ('val.html', 'De val bij beide pariteiten', 'The trap at both parities'),
        ('val-pariteit.html', 'Waarom beide pariteiten', 'Why both parities'),
        ('val-race.html', 'De race naar de val', 'The race to the trap')]),
    ('De vesting op grotere borden', 'The fortress on larger boards', [
        ('vesting.html', 'Oppositie op 8×9', 'Opposition on 8×9'),
        ('afstandsregel.html', 'De afstandsregel', 'The distance rule'),
        ('vesting-9x9.html', '9×9 onspeelbare hoeken: de val en de diagonaal', '9×9 unplayable corners: the trap and the diagonal')]),
]
# every page in reading order: (file, Dutch, English, chapter number or None)
PAGES = [('index.html', 'Overzicht', 'Overview', None)] + [(f, nl, en, c) for c, (_, _, secs) in enumerate(CHAPTERS)
                                                        for f, nl, en in secs]


TABLE32 = {'nl': '''<h2>3 dammen tegen 2 per bord</h2>
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
<tr><td>7×9 onspeelbare hoeken (vier dubbele hoeken)</td><td>124.148 (13,2%)</td><td>93</td><td>zie <a href="vesting.html">vesting</a></td></tr>
<tr><td>9×9 onspeelbare hoeken (vier dubbele hoeken)</td><td>1.201.460 (29,5%)</td><td>99</td><td>zie <a href="vesting.html">vesting</a></td></tr>
</table>
<p>Op borden met twee oneven zijden zijn de vier hoekvelden óf alle vier speelbaar óf alle vier onspeelbaar; bij onspeelbare hoekvelden is elke hoek een dubbele hoek.</p>''',
           'en': '''<h2>3 kings against 2 per board</h2>
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
<tr><td>7×9 unplayable corners (four double corners)</td><td>124,148 (13.2%)</td><td>93</td><td>see <a href="vesting.html">fortress</a></td></tr>
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
zettenregels. Elke stelling is klikbaar.</p>''', '''
<p>Endgames with kings only in English checkers: short kings (one square), mandatory capture and free choice among
captures. On boards of any size, computed exactly with
<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/src/egdb_fast.cpp"><code>egdb_fast</code></a>. Values in
plies for the side to move: W = win, L = loss, D = draw; purely game-theoretic, without move-count rules. Every position
is clickable.</p>''')]
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
<p>De langste winst groeit gestaag met het bord: 33 ply op 8×8, 41 op 10×10, 49 op 12×12, 61 op 12×15.</p>''', '''
<p>2 against 1 is won on every board that is large enough, with two exceptions:</p>
<ul><li><a href="twee-blokkade.html">the blockade at the edge</a>: in a fixed handful of positions (8 or 10,
independent of the board size; on 8×8 up to symmetry 3) the two white kings are locked in the single corner and the black king keeps them there
by opposition; on odd × odd boards with playable corners this blockade does not exist;</li>
<li>and, surprisingly, <a href="twee-4x4.html">the 4×4 board</a>: there 2 against 1 is a draw.</li></ul>
<p>The longest win grows steadily with the board: 33 plies on 8×8, 41 on 10×10, 49 on 12×12, 61 on 12×15.</p>'''),
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
            f'<a href="afstandsregel.html">afstandsregel</a>: D × d = 7 × 7, de vesting valt). De oppositie, die op 8×9 en '
            f'9×10 de helft van zulke stellingen remise maakt, speelt op 8×8 dus geen rol.</p>',
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
            f'makes half of such positions a draw on 8×9 and 9×10, plays no role on 8×8.</p>'),
            f'<p class="var">{h}</p>', '<div style="clear:both"></div>']
        flush('drie-langste.html')
        body, phases = self.traps88(pg, k88, e88)
        flush('val.html')
        body = phases
        flush('val-pariteit.html')
        body = self.race88(pg, k88, e88)
        flush('val-race.html')
        # the 3-2 blockades on 8x8: 1252 quiet draws with white to move, 316 up to symmetry; 307 of them contain the
        # 2-1 blockade in a single corner
        body += [t('''
<p>Op 7×8 en 8×8 is 3 tegen 2 in het algemeen gewonnen. De weinige remises ontstaan bijna alleen als wit tegen de rand
gedrukt wordt (0,0% remise zonder witte dam aan de rand, 0,5% met drie), en zwart in beide dubbele hoeken maakt nooit
remise. Op 8×8 zijn er 1.252 rustige remisestellingen met wit aan zet, 316 op spiegelingen van het bord na, en bijna
allemaal draaien ze om de enkele hoek. In 307 zit de blokkade van <a href="twee-blokkade.html">2 tegen 1</a>: twee witte
dammen in een enkele hoek, vastgehouden door één zwarte dam. In 5 houdt zwart dezelfde twee witte dammen met twee dammen
vast (op f6 en e5 tegen h8 en g7), in 2 zit één witte dam vast in de hoek. Alleen 2 klassen hebben niets met de hoek te
maken (het derde diagram).</p>''', '''
<p>On 7×8 and 8×8 3-2 is won generically. The few draws occur almost only when white is pushed against the edge (draw
rate 0.0% with no white king on the edge, 0.5% with three), and black holding both double corners never draws. On 8×8
there are 1,252 quiet drawn positions with white to move, 316 up to the symmetries of the board, and nearly all of them
turn on the single corner. 307 contain the blockade of <a href="twee-blokkade.html">2 against 1</a>: two white kings in a
single corner, held by one black king. In 5 black holds the same two white kings with two kings (on f6 and e5 against h8
and g7), in 2 one white king is locked in the corner. Only 2 classes have nothing to do with the corner (the third
diagram).</p>''')]
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
             'Een van de 9 klassen zonder blokkade in de hoek: de zwarte dam op e5 staat midden tussen de drie witte dammen. Een '
             'witte dam die ernaast komt, wordt geslagen (1. e7-d6 e5×c7). Remise met wit én met zwart aan zet; met zwart aan '
             'zet houden alleen e5-d6 en e5-d4 remise. ',
             'One of the 9 classes without a corner blockade: the black king on e5 stands in the middle of the three white kings. '
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
        """the trap at both parities on 8x8: every shuttle move of black allows a forced exchange"""
        t = self.t
        body = [t('<p>Wit breekt de dubbele hoeken met een val: een opstelling waarin elke pendelzet van zwart een '
                  'gedwongen ruil toelaat: met een rustige zet dwingt wit zwart een witte dam te slaan, wit slaat terug, en 2 tegen 1 wint. Zwart '
                  'heeft twee dammen en dus twee pendelzetten; de val moet ze allebei straffen, ook als de ruil de andere zwarte dam treft. Of zo\'n val werkt, hangt '
                  'af van de pariteit: even of oneven, de pariteit van de som van de rijnummers (1, 2, …) van alle dammen, met zwart aan zet. Die verandert '
                  'zonder slag nooit, want dammen kunnen geen tempo verliezen; dat is de oppositie. De database telt de '
                  'vallen met zwart aan zet en beide zwarte dammen op de randvelden van twee dubbele hoeken:</p>',
                  '<p>White breaks the double corners with a trap: a set-up in which every shuttle move of black allows a '
                  'forced exchange: with a quiet move white forces black to capture a white king, white captures back, and 2 against 1 wins. '
                  'Black has two kings and therefore two shuttle moves; the trap has to punish both, even when the exchange hits the other black king. Whether such a trap '
                  'works depends on the parity: even or odd, the parity of the sum of the rank numbers (1, 2, …) of all kings, with black to move. It '
                  'never changes without a capture, since kings cannot lose a tempo; that is the opposition. The database '
                  'counts the traps with black to move and both black kings on the edge squares of two double corners:</p>'),
                t('<table class="tbl"><tr><th>Bord</th><th>Vallen bij even pariteit</th><th>Vallen bij oneven pariteit</th><th>Gevolg</th></tr>'
                  '<tr><td>8×8</td><td>3</td><td>3</td><td>de vesting valt altijd</td></tr>'
                  '<tr><td>8×9, 9×10</td><td>0</td><td>3</td><td>de oppositie beslist: de helft remise</td></tr>'
                  '<tr><td>10×10</td><td>0</td><td>0</td><td>de vesting houdt</td></tr></table>',
                  '<table class="tbl"><tr><th>Board</th><th>Traps at even parity</th><th>Traps at odd parity</th><th>Consequence</th></tr>'
                  '<tr><td>8×8</td><td>3</td><td>3</td><td>the fortress always falls</td></tr>'
                  '<tr><td>8×9, 9×10</td><td>0</td><td>3</td><td>the opposition decides: half are draws</td></tr>'
                  '<tr><td>10×10</td><td>0</td><td>0</td><td>the fortress holds</td></tr></table>'),
                t('<p>Op 8×8 heeft wit dus bij elke pariteit een val en kiest hij de val die bij de oppositie past. In de vallen '
                  'staan de drie witte dammen op een van de twee lange diagonalen die de dubbele hoeken verbinden, tussen '
                  'de zwarte dammen in; welke diagonaal, bepaalt de pariteit. Alleen op 8×8 lopen beide diagonalen van de ene '
                  'dubbele hoek naar de andere. De tellingen komen uit '
                  '<a href="https://github.com/rhalbersma/egdb-draughts/blob/main/checkers/traps.py"><code>traps.py</code></a>.</p>',
                  '<p>On 8×8 white therefore has a trap at each parity and picks the one that fits the opposition. In the '
                  'traps the three white kings stand on one of the two long diagonals that connect the double corners, '
                  'between the black kings; which diagonal decides the parity. Only on 8×8 do both diagonals run from one '
                  'double corner to the other. The counts come from '
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
        # why both parities: the diagonal of white decides the parity, and moving over to the other diagonal costs three
        # moves (one per king), an odd number
        ms = {m[3]: m for m in e88.analyse('W:WK14,K18,K23:BK5,K32')[1]}
        shift = ', '.join(pg.label(k88, ms[x]) for x in ('14-10', '18-15', '23-19'))
        phases = [t(
            '<p>Op welke diagonaal zwart staat, maakt niet uit: met de witte dammen 14, 18 en 23 is er een val tegen de '
            'zwarte dammen 5 en 32, dus op dezelfde diagonaal, en ook een tegen 1 en 28, op de andere. Wat de pariteit '
            'bepaalt, is de diagonaal van wit:</p>',
            '<p>Which diagonal black stands on does not matter: with the white kings 14, 18 and 23 there is a trap against '
            'the black kings 5 and 32, on the same diagonal, and also one against 1 and 28, on the other. What decides the '
            'parity is the diagonal of white:</p>'),
            t('<table class="tbl"><tr><th>Zwart</th><th>Wit op diagonaal 5…32: even pariteit</th><th>Wit op diagonaal 1…28: oneven pariteit</th></tr>'
              '<tr><td>5 en 32</td><td>14, 18, 23</td><td>10, 15, 19 · 6, 15, 24</td></tr>'
              '<tr><td>1 en 28</td><td>14, 18, 23 · 9, 18, 27</td><td>10, 15, 19</td></tr></table>',
              '<table class="tbl"><tr><th>Black</th><th>White on diagonal 5…32: even parity</th><th>White on diagonal 1…28: odd parity</th></tr>'
              '<tr><td>5 and 32</td><td>14, 18, 23</td><td>10, 15, 19 · 6, 15, 24</td></tr>'
              '<tr><td>1 and 28</td><td>14, 18, 23 · 9, 18, 27</td><td>10, 15, 19</td></tr></table>'),
            t(f'<p>Van de val met 14, 18, 23 naar die met 10, 15, 19 gaat elke witte dam één stap naar de andere diagonaal: '
              f'{shift}. Dat zijn drie zetten, een oneven aantal: het tempo draait om, en de pariteit wisselt. Met '
              f'twee dammen zou zo\'n overstap twee zetten kosten, en dan wisselt het tempo niet.</p>',
              f'<p>From the trap with 14, 18, 23 to the one with 10, 15, 19 every white king takes one step to the other '
              f'diagonal: {shift}. That is three moves, an odd number: the tempo turns over, and the parity changes. '
              f'With two kings such a change-over would take two moves, and then the tempo does not change.</p>')]
        return body, phases

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
        body = [t(
            f'<p>Ja, zolang zwart in zijn dubbele hoeken blijft. Vanuit elke rustige stelling met zwart in beide dubbele '
            f'hoeken en wit aan zet dwingt wit bij beide pariteiten een val af, tegen elke pendelverdediging, in hoogstens {n} '
            f'zetten. Gerekend is het spel waarin zwart alleen pendelt en wit rustige zetten doet die zwart geen slag geven; '
            f'een pendelzet die wit een slag geeft, kost zwart meteen een dam. Het aantal witte zetten tot de val:</p>',
            f'<p>Yes, as long as black stays in its double corners. From every quiet position with black in both double '
            f'corners and white to move, white forces a trap at both parities, against every shuttling defence, in at most '
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
                   'meestal kan zwart later ook weer terug in de hoek. Het redt niets: alle stellingen blijven verloren.</p>',
                   '<p>Black can also leave a corner itself. According to the database that is often just as good as '
                   'shuttling, and usually black can later get back into the corner. It saves nothing: every position stays '
                   'lost.</p>'), '<div style="clear:both"></div>']
        return body

    def vesting(self):
        k89, e89 = self.engine(9, 8, 'playable')
        k99, e99 = self.engine(9, 9, 'unplayable')
        pg = self.page((k89, e89))
        pg9 = self.page((k99, e99))
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
        body = [t('''
<p>Een dam op de twee randvelden van een dubbele hoek kan nooit geslagen worden en heeft altijd een wachtzet. Op 8×9 en
9×10 maakt een zwarte dam in elke dubbele hoek remise in de helft van de stellingen, en welke helft dat is, beslist de
<strong>oppositie</strong>: de pariteit (even of oneven) van de som van de rijnummers van alle stukken wisselt bij elke
zet, en dammen kunnen geen tempo verliezen, dus met zwart aan zet ligt de pariteit vanaf het begin vast.</p>
<p>De vraag kwam op in de draad <a href="https://damforum.nl/viewtopic.php?t=8915"><em>Draughts on a 10x11 or 8x9 board</em></a> op het World Draughts Forum (november 2026): houden twee dammen in beide dubbele hoeken op 8×9 stand, terwijl ze op 8×8 verliezen? Op 8×9 hangt het van de oppositie af; waarom 8×8 altijd verloren is, staat bij de <a href="val.html">vallen op 8×8</a>.</p>
<table class="tbl"><tr><th>Zwart in beide dubbele hoeken, zwart aan zet</th><th>Even pariteit</th><th>Oneven pariteit</th></tr>
<tr><td>8×9</td><td>100% remise</td><td>0% remise</td></tr>
<tr><td>9×10</td><td>100%</td><td>0%</td></tr>
<tr><td>10×11</td><td>100%</td><td>100%</td></tr></table>
<p>Geteld zijn de rustige stellingen (niemand kan slaan) met de zwarte dammen op de randvelden van twee dubbele hoeken:
8.228 en 8.244 op 8×9, 18.420 en 18.440 op 9×10, 37.030 en 37.050 op 10×11.</p>''', '''
<p>A king on the two edge squares of a double corner can never be captured and always has a waiting move. On 8×9 and
9×10 a black king in each double corner draws in half of the positions, and that half is decided by
<strong>opposition</strong>: the parity (even or odd) of the sum of the rank numbers of all pieces flips with every
move, and kings cannot lose a tempo, so with black to move the parity is fixed from the start.</p>
<p>The question came up in the thread <a href="https://damforum.nl/viewtopic.php?t=8915"><em>Draughts on a 10x11 or 8x9 board</em></a> on the World Draughts Forum (November 2026): do two kings in both double corners hold on 8×9, while they lose on 8×8? On 8×9 it depends on the opposition; why 8×8 is always lost is explained at the <a href="val.html">traps on 8×8</a>.</p>
<table class="tbl"><tr><th>Black in both double corners, black to move</th><th>Even parity</th><th>Odd parity</th></tr>
<tr><td>8×9</td><td>100% draws</td><td>0% draws</td></tr>
<tr><td>9×10</td><td>100%</td><td>0%</td></tr>
<tr><td>10×11</td><td>100%</td><td>100%</td></tr></table>
<p>Counted are the quiet positions (nobody can capture) with the black kings on the edge squares of two double corners:
8,228 and 8,244 on 8×9, 18,420 and 18,440 on 9×10, 37,030 and 37,050 on 10×11.</p>'''),
                fig,
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
<p>Wat beslist, is de afstand tussen de hoeken van de dubbele hoeken die zwart bezet (<em>D</em> = grootste,
<em>d</em> = kleinste coördinaatverschil; <em>d</em> = 0 als ze aan dezelfde kant liggen), niet of ze aan dezelfde
kant liggen:</p>
<table class="tbl"><tr><th>Borden</th><th>D × d</th><th>Vesting</th></tr>
<tr><td>6×7, 7×8, 7×9 onspeelbare hoeken (verticaal paar)</td><td>6 × 0</td><td>valt</td></tr>
<tr><td>8×8</td><td>7 × 7</td><td>valt</td></tr>
<tr><td>8×9, 9×10, 9×9 onspeelbare hoeken (zelfde kant), 7×9 onspeelbare hoeken (horizontaal)</td><td>8 × 0</td><td>oppositie beslist</td></tr>
<tr><td>7×9 onspeelbare hoeken (diagonaal paar)</td><td>8 × 6</td><td>oppositie beslist</td></tr>
<tr><td>9×9 onspeelbare hoeken (diagonaal paar)</td><td>8 × 8</td><td>houdt</td></tr>
<tr><td>8×10, 10×10, 10×11, 10×12</td><td>≥ 9</td><td>houdt</td></tr></table>
<p><em>D</em> ≤ 7: de vesting valt · <em>D</em> = 8: de oppositie beslist, tenzij ook <em>d</em> = 8 ·
<em>D</em> ≥ 9: de vesting houdt.</p>''', '''
<p>What decides is the distance between the corners of the double corners black occupies (<em>D</em> = largest,
<em>d</em> = smallest coordinate difference; <em>d</em> = 0 when they are on the same side), not whether they lie on the
same side:</p>
<table class="tbl"><tr><th>Boards</th><th>D × d</th><th>Fortress</th></tr>
<tr><td>6×7, 7×8, 7×9 unplayable corners (vertical pair)</td><td>6 × 0</td><td>falls</td></tr>
<tr><td>8×8</td><td>7 × 7</td><td>falls</td></tr>
<tr><td>8×9, 9×10, 9×9 unplayable corners (same side), 7×9 unplayable corners (horizontal)</td><td>8 × 0</td><td>opposition decides</td></tr>
<tr><td>7×9 unplayable corners (diagonal pair)</td><td>8 × 6</td><td>opposition decides</td></tr>
<tr><td>9×9 unplayable corners (diagonal pair)</td><td>8 × 8</td><td>holds</td></tr>
<tr><td>8×10, 10×10, 10×11, 10×12</td><td>≥ 9</td><td>holds</td></tr></table>
<p><em>D</em> ≤ 7: the fortress falls · <em>D</em> = 8: opposition decides, unless also <em>d</em> = 8 ·
<em>D</em> ≥ 9: the fortress holds.</p>'''),
                ]
        rule = next(k for k, x in enumerate(body) if isinstance(x, str) and 'D × d' in x)
        self.write('vesting.html', None, '\n'.join(body[:rule]), pg, k89)
        self.write('afstandsregel.html', None, body[rule], self.page((k89, e89)), k89)
        self.write('vesting-9x9.html', None, '\n'.join(self.fortress99(pg9, k99, e99)), pg9, k99)


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

    def fortress99(self, pg, k99, e99):
        """9x9 with unplayable corners: the trap works for two double corners on the same side, not for the diagonal pair"""
        t = self.t
        body = [t('''
<p>Op 9×9 met onspeelbare hoekvelden is elke hoek een dubbele hoek: b1/a2, h1/i2, a8/b9 en h9/i8.
Zwart kan twee dubbele hoeken aan dezelfde kant bezetten (afstand 8 × 0) of twee diagonaal tegenover elkaar
(8 × 8). Beslissend is weer de <a href="val.html">val</a>: een opstelling van de drie witte dammen waarin elke
pendelzet van zwart een gedwongen ruil toelaat. De database telt de vallen met zwart aan zet en beide zwarte dammen
op de randvelden van hun dubbele hoeken:</p>
<table class="tbl"><tr><th>Zwart in</th><th>Vallen bij even pariteit</th><th>Vallen bij oneven pariteit</th><th>Gevolg</th></tr>
<tr><td>twee dubbele hoeken aan dezelfde kant</td><td colspan="2">3 bij één van de twee pariteiten</td><td>de oppositie beslist</td></tr>
<tr><td>twee diagonaal tegenover elkaar</td><td>0</td><td>0</td><td>de vesting houdt</td></tr></table>
<p>Per paar dubbele hoeken zijn er 12.100 beginstellingen bij elke pariteit.</p>''', '''
<p>On 9×9 with unplayable corner squares every corner is a double corner: b1/a2, h1/i2, a8/b9 and h9/i8.
Black can occupy two double corners on the same side (distance 8 × 0) or two diagonally opposite ones (8 × 8). What
decides is again the <a href="val.html">trap</a>: a set-up of the three white kings in which every shuttle move of
black allows a forced exchange. The database counts the traps with black to move and both black kings on the edge
squares of their double corners:</p>
<table class="tbl"><tr><th>Black in</th><th>Traps at even parity</th><th>Traps at odd parity</th><th>Consequence</th></tr>
<tr><td>two double corners on the same side</td><td colspan="2">3 at one of the two parities</td><td>the opposition decides</td></tr>
<tr><td>two diagonally opposite ones</td><td>0</td><td>0</td><td>the fortress holds</td></tr></table>
<p>Per pair of double corners there are 12,100 starting positions at each parity.</p>''')]
        # the trap along the side of the i-file: black h1 and h9, white h3, h5, h7
        fen = e99.fen(['h3', 'h5', 'h7'], ['h1', 'h9'], 'B')
        v, legal = e99.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'9×9 onspeelbare hoeken, zwart aan zet: {v}', f'9×9 unplayable corners, black to move: {v}'), gid=k99)
        sh = self.shuttles(pg, k99, e99, i0, legal, {'h1-i2', 'h9-i8'})
        assert len(sh) == 2 and all(ok for _, ok, _ in sh)
        body += [fig, t('<p>Twee dubbele hoeken aan dezelfde kant, h1/i2 en h9/i8, met de witte dammen op h3, h5 en h7 '
                        'ertussen: elke pendelzet van zwart wordt gestraft: ',
                        '<p>Two double corners on the same side, h1/i2 and h9/i8, with the white kings on h3, h5 and h7 '
                        'between them: every shuttle move of black is punished: ')
                 + '; '.join(x for _, _, x in sh)
                 + t(', en 2 tegen 1 wint. Alle zetten van zwart: ', ', and 2 against 1 wins. All moves of black: ')
                 + self.moves_list(pg, i0, legal, '1. … ')
                 + t('. Net als op 8×9 en 9×10 werkt deze val maar bij één van de twee pariteiten: de oppositie beslist.</p>',
                     '. As on 8×9 and 9×10 this trap works at only one of the two parities: the opposition decides.</p>'),
                 '<div style="clear:both"></div>']
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
        (out / 'dubbele-hoeken.html').write_text('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" '
                                                 'content="0; url=drie-langste.html"><link rel="canonical" href="drie-langste.html">'
                                                 '<title>Checkers</title><a href="drie-langste.html">drie-langste.html</a>\n')
        (out / 'val-fasen.html').write_text('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" '
                                            'content="0; url=val-pariteit.html"><link rel="canonical" href="val-pariteit.html">'
                                            '<title>Checkers</title><a href="val-pariteit.html">val-pariteit.html</a>\n')


if __name__ == '__main__':
    main()
