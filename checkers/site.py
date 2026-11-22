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
        ('twee-verdrijven.html', 'De dam verdrijven', 'Driving out the king'),
        ('twee-4x4.html', 'Uitzondering 1: remise op 4×4', 'Exception 1: a draw on 4×4'),
        ('twee-5x5.html', '5×5: de verdreven dam ontsnapt naar een andere hoek', '5×5: the expelled king escapes to another corner'),
        ('twee-5x5-val.html', '5×5: een val die werkt met wit en met zwart aan zet', '5×5: a trap that works with white and with black to move'),
        ('twee-langste.html', 'De langste winst op 8×8', 'The longest win on 8×8'),
        ('twee-blokkade.html', 'Uitzondering 2: de blokkade aan de rand', 'Exception 2: the blockade at the edge')]),
    ('3 tegen 2 op 8×8', '3 against 2 on 8×8', [
        ('drie-langste.html', 'De langste winst', 'The longest win'),
        ('blokkades.html', 'Uitzondering 1: de blokkades', 'Exception 1: the blockades'),
        ('curiosa.html', 'Uitzondering 2: twee curiosa', 'Exception 2: two curiosities'),
        ('val.html', 'Twee vallen om de vesting te breken', 'Two traps to break the fortress'),
        ('val-pariteit.html', 'De winst is strategisch, de pariteit doet er niet toe', 'The win is strategic, parity does not matter')]),
    ('3 tegen 2 op 9×8', '3 against 2 on 9×8', [
        ('vesting.html', 'Twee vallen om de vesting te breken', 'Two traps to break the fortress'),
        ('vesting-pariteit.html', 'De winst is niet strategisch, de pariteit doet ertoe', 'The win is non-strategic, parity matters')]),
    ('3 tegen 2 op 8×7', '3 against 2 on 8×7', [
        ('vesting-8x7.html', 'Vier vallen om de vesting te breken', 'Four traps to break the fortress'),
        ('vesting-8x7-wit.html', 'De winst is strategisch, de dam kan verdreven worden', 'The win is strategic, the king can be driven out')]),
    ('3 tegen 2 op 7×7', '3 against 2 on 7×7', [
        ('langste-7x7.html', 'Geen ontsnapping naar een andere dubbele hoek', 'No escape to another double corner')]),
    ('3 tegen 2 op 5×5', '3 against 2 on 5×5', [
        ('langste-5x5.html', 'De langste winst', 'The longest win'),
        ('vallen-5x5.html', 'Veertig vallen om de vesting te breken', 'Forty traps to break the fortress'),
        ('zetdwang-5x5.html', 'Uitzondering: zetdwang', 'Exception: zugzwang')]),
    ('3 tegen 2 op andere borden', '3 against 2 on other boards', [
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
        k521, e521 = self.engine(5, 5, 'unplayable', 2, 1)
        pg = self.page((k21, e21), (k44, e44), (k88, e88), (k521, e521))
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
            pg = self.page((k21, e21), (k44, e44), (k88, e88), (k521, e521))
            body = []
        flush('index.html')
        fen = self.longest_21(e21)
        v, _ = e21.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'8×8, 2 tegen 1, wit aan zet: {v}', f'8×8, 2 against 1, white to move: {v}'), gid=k21)
        h, *_ = pg.best_line(fen, 60, start=i0)
        body += [fig, t('''
<p>2 tegen 1 is op elk voldoende groot bord gewonnen, met twee uitzonderingen:</p>
<ul><li><a href="twee-4x4.html">het 4×4-bord</a>: daar is 2 tegen 1, verrassend, remise;</li>
<li>en <a href="twee-blokkade.html">de blokkade aan de rand</a>: in een vast handjevol stellingen (8 of 10,
onafhankelijk van de bordgrootte; op 8×8 op spiegelingen na 3) zitten de twee witte dammen opgesloten in de enkele hoek en houdt de zwarte dam ze
daar met oppositie; op oneven × oneven borden met speelbare hoeken bestaat die blokkade niet.</li></ul>
<p>De langste winst groeit gestaag met het bord: 33 ply op 8×8, 41 op 10×10, 49 op 12×12, 61 op 15×12.</p>''', '''
<p>2 against 1 is won on every board that is large enough, with two exceptions:</p>
<ul><li><a href="twee-4x4.html">the 4×4 board</a>: there 2 against 1 is, surprisingly, a draw;</li>
<li>and <a href="twee-blokkade.html">the blockade at the edge</a>: in a fixed handful of positions (8 or 10,
independent of the board size; on 8×8 up to symmetry 3) the two white kings are locked in the single corner and the black king keeps them there
by opposition; on odd × odd boards with playable corners this blockade does not exist.</li></ul>
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
                 t('<p>Daarna begint het opnieuw in de andere hoek. Op 8×8 ligt de andere dubbele hoek te ver weg (zie '
                   '<a href="twee-verdrijven.html">de dam verdrijven</a>).</p>',
                   '<p>Then it starts again in the other corner. On 8×8 the other double corner is too far away (see '
                   '<a href="twee-verdrijven.html">driving out the king</a>).</p>'),
                 '<div style="clear:both"></div>']
        flush('twee-4x4.html')
        # 5x5 with unplayable corners: only double corners. Driving the king out as on 8x8 fails: it escapes to another
        # double corner, as on 4x4. Still 2 against 1 always wins, with a sacrifice (offer a king, black must capture,
        # white captures back); there is one trap, up to symmetry, that works with white and with black to move
        from itertools import combinations
        sq55 = sorted(f'{chr(97 + f)}{r + 1}' for f, r in e521.g['sq'].values())
        count = lambda f: tuple(len([x for x in side[1:].split(',') if x]) for side in f.split(':')[1:])
        captures = lambda f: any(count(m[2]) != count(f) for m in e521.analyse(f)[1])
        syms = [lambda f, r: (f, r), lambda f, r: (4 - f, r), lambda f, r: (f, 4 - r), lambda f, r: (4 - f, 4 - r),
                lambda f, r: (r, f), lambda f, r: (4 - r, f), lambda f, r: (r, 4 - f), lambda f, r: (4 - r, 4 - f)]
        tr = lambda s, g: (lambda f, r: f'{chr(97 + f)}{r + 1}')(*g(ord(s[0]) - 97, int(s[1:]) - 1))
        canon = lambda W, B: min((tuple(sorted(tr(x, g) for x in W)), tuple(sorted(tr(x, g) for x in B))) for g in syms)
        allpos = [(list(W), [b]) for W in combinations(sq55, 2) for b in sq55 if b not in W]
        # white wins without a sacrifice: it captures the black king (or leaves it without a move) and black never captures
        direct, changed = set(), True
        fens = [e521.fen(W, B, s) for W, B in allpos for s in 'WB']
        while changed:
            changed = False
            for f in fens:
                if f in direct:
                    continue
                ms = e521.analyse(f)[1]
                ok = (any(count(m[2])[1] == 0 or m[2] in direct for m in ms) if f[0] == 'W'
                      else all(count(m[2])[0] == 2 and m[2] in direct for m in ms))
                if ok:
                    direct.add(f)
                    changed = True
        quiet = [(W, B) for W, B in allpos if not captures(e521.fen(W, B, 'W')) and not captures(e521.fen(W, B, 'B'))]
        assert all(e521.analyse(e521.fen(W, B, 'W'))[0][0] == 'W' and e521.analyse(e521.fen(W, B, 'B'))[0][0] == 'L'
                   for W, B in quiet)
        orb = lambda xs: len({canon(W, B) for W, B in xs})
        nq = orb(quiet)
        dw = orb((W, B) for W, B in quiet if e521.fen(W, B, 'W') in direct)
        db = orb((W, B) for W, B in quiet if e521.fen(W, B, 'B') in direct)

        def checked(fen, text, white_best=True):
            """play the line: black defends longest, white keeps the win (fastest if white_best)"""
            g = fen
            for (mv,), _ in self.plan(text):
                ms = e521.analyse(g)[1]
                m = next(x for x in ms if x[0] == mv)
                w = g[0] == 'W'
                assert m[1][0] == ('W' if w else 'L'), (mv, m[1])
                if not w or white_best:
                    assert int(m[1][1:]) == (min if w else max)(int(x[1][1:]) for x in ms if x[1][0] == m[1][0]), (mv, m[1])
                g = m[2]
            return g
        # driving out as on 8x8 (the same set-up in the double corner a4/b5): the king escapes to the double corner b1/a2
        fd = e521.fen(['c2', 'd3'], ['a4'], 'W')
        vd = e521.analyse(fd)[0]
        lined = '1. d3-c4 a4-b5 2. c2-d3 b5-a4 3. c4-b5 a4-b3 4. b5-a4 b3-a2'
        end = checked(fd, lined, white_best=False)
        assert end.split(':')[0] == 'W' and count(end) == (2, 1)
        figd, idd = pg.diagram(fd, t(f'5×5, 2 tegen 1, wit aan zet: {vd}', f'5×5, 2 against 1, white to move: {vd}'), gid=k521)
        hd, *_ = pg.moves(fd, self.plan(lined), 1, False, 'm1', idd)
        body += [t(f'''
<p>Op 5×5 met onspeelbare hoekvelden is elke hoek een dubbele hoek: b1/a2, d1/e2, a4/b5 en d5/e4. Wit drijft de zwarte dam
hier uit de dubbele hoek a4/b5 zoals op 8×8 (zie <a href="twee-verdrijven.html">de dam verdrijven</a>): wit op c2 en d3,
zwart op a4. Na 3. c4-b5 moet de dam eruit (3. … a4-b3), maar na 4. b5-a4 ontsnapt hij met 4. … b3-a2 naar de dubbele hoek
b1/a2. Zoals op <a href="twee-4x4.html">4×4</a> ligt de volgende dubbele hoek te dichtbij. (De snelste winst, in {vd[1:]}
ply, gaat met een offer.)</p>''', f'''
<p>On 5×5 with unplayable corner squares every corner is a double corner: b1/a2, d1/e2, a4/b5 and d5/e4. White drives the
black king out of the double corner a4/b5 here as on 8×8 (see <a href="twee-verdrijven.html">driving out the king</a>):
white on c2 and d3, black on a4. After 3. c4-b5 the king has to leave (3. … a4-b3), but after 4. b5-a4 it escapes with
4. … b3-a2 to the double corner b1/a2. As on <a href="twee-4x4.html">4×4</a> the next double corner is too close. (The
fastest win, in {vd[1:]} plies, goes with a sacrifice.)</p>'''),
                 figd, f'<p class="var">{hd}</p>', '<div style="clear:both"></div>',
                 t(f'''
<p>Is 2 tegen 1 op 5×5 dan remise, zoals op 4×4? Nee: alle rustige stellingen zijn gewonnen. Maar verdrijven helpt bijna
nooit. Op spiegelen en draaien na zijn er {nq} rustige stellingen, en met wit aan zet wint wit er maar {dw} zonder offer,
met zwart aan zet {db}. In alle andere wint wit met een offer: hij biedt een dam aan, zwart moet slaan, en wit slaat terug.
Hoe, staat bij <a href="twee-5x5-val.html">de val</a>.</p>''', f'''
<p>Is 2 against 1 on 5×5 then a draw, as on 4×4? No: all quiet positions are won. But driving out hardly ever works. Up to
mirrors and rotations there are {nq} quiet positions, and with white to move white wins only {dw} of them without a
sacrifice, with black to move {db}. In all the others white wins with a sacrifice: he offers a king, black has to capture,
and white captures back. How is shown with <a href="twee-5x5-val.html">the trap</a>.</p>''')]
        flush('twee-5x5.html')

        # the trap: white to move strikes at once with a sacrifice, black to move every move allows one (or a capture)
        def strike(f):
            for o in e521.analyse(f)[1]:
                if count(o[2]) != (2, 1):
                    continue
                rs = e521.analyse(o[2])[1]
                if rs and all(count(r[2]) == (1, 1) and any(count(x[2]) == (1, 0) for x in e521.analyse(r[2])[1]) for r in rs):
                    return o
            return None
        edges = ['b1', 'a2', 'd1', 'e2', 'a4', 'b5', 'd5', 'e4']
        traps = {canon(W, B) for W, B in quiet if B[0] in edges and strike(e521.fen(W, B, 'W'))
                 and all(strike(m[2]) or any(count(x[2]) == (2, 0) for x in e521.analyse(m[2])[1])
                         for m in e521.analyse(e521.fen(W, B, 'B'))[1])}
        assert traps == {canon(['b3', 'e4'], ['b1'])}
        ftb, ftw = e521.fen(['b3', 'e4'], ['b1'], 'B'), e521.fen(['b3', 'e4'], ['b1'], 'W')
        vtb, legaltb = e521.analyse(ftb)
        vtw = e521.analyse(ftw)[0]
        linetw = '1. b3-c2! b1xd3 2. e4xc2'
        assert count(checked(ftw, linetw)) == (1, 0)
        lineta = '1. … b1-a2 2. e4-d5! a2xc4 3. d5xb3'
        assert count(checked(ftb, lineta)) == (1, 0)
        figtb, itb = pg.diagram(ftb, t(f'5×5, 2 tegen 1, zwart aan zet: {vtb}', f'5×5, 2 against 1, black to move: {vtb}'), gid=k521)
        hta, *_ = pg.moves(ftb, self.plan(lineta), 1, True, 'm1', itb)
        figtw, itw = pg.diagram(ftw, t(f'5×5, 2 tegen 1, wit aan zet: {vtw}', f'5×5, 2 against 1, white to move: {vtw}'), gid=k521)
        htw, *_ = pg.moves(ftw, self.plan(linetw), 1, False, 'm1', itw)
        f55 = e521.fen(['a2', 'a4'], ['e2'], 'B')
        v55 = e521.analyse(f55)[0]
        line55 = '1. … e2-d3 2. a4-b5 d3-e2 3. b5-c4 e2-d1 4. a2-b3 d1-e2 5. b3-a4 e2-d1 6. c4-d3 d1-e2 7. a4-b5! e2xc4 8. b5xd3'
        assert count(checked(f55, line55)) == (1, 0) and f55 not in direct
        assert v55 == 'L' + str(max(int(e521.analyse(e521.fen(W, B, 'B'))[0][1:]) for W, B in quiet))
        fig55, i55 = pg.diagram(f55, t(f'5×5, 2 tegen 1, zwart aan zet: {v55}', f'5×5, 2 against 1, black to move: {v55}'), gid=k521)
        h55, *_ = pg.moves(f55, self.plan(line55), 1, True, 'm1', i55)
        body += [t('''
<p>Op spiegelen en draaien na is er precies één val die werkt met wit en met zwart aan zet: wit op b3 en e4, zwart op b1.
Met zwart aan zet verliest elke zwarte zet. Pendelt zwart naar a2, dan offert wit de dam op e4; gaat hij naar c2, dan slaat
wit hem meteen (1. … b1-c2 2. b3×d1).''' + ' ' + 'Alle zetten van zwart: ' + self.moves_list(pg, itb, legaltb, '1. … ') + '.</p>', '''
<p>Up to mirrors and rotations there is exactly one trap that works with white and with black to move: white on b3 and
e4, black on b1. With black to move every black move loses. If black shuttles to a2, white sacrifices the king on e4; if
it goes to c2, white captures it at once (1. … b1-c2 2. b3×d1).''' + ' ' + 'All moves of black: ' + self.moves_list(pg, itb, legaltb, '1. … ') + '.</p>'),
                 figtb, f'<p class="var">{hta}</p>', '<div style="clear:both"></div>',
                 t('<p>Met wit aan zet offert wit meteen de dam op b3:</p>',
                   '<p>With white to move white sacrifices the king on b3 at once:</p>'),
                 figtw, f'<p class="var">{htw}</p>', '<div style="clear:both"></div>',
                 t(f'''
<p>Ook de langste winst met zwart aan zet ({v55[1:]} ply) eindigt met een offer. Zwart pendelt in de dubbele hoek d1/e2;
wit zet met 6. c4-d3 een dam klaar, en na 6. … d1-e2 7. a4-b5! moet zwart slaan: 7. … e2×c4 8. b5×d3.</p>''', f'''
<p>The longest win with black to move ({v55[1:]} plies) also ends with a sacrifice. Black shuttles in the double corner
d1/e2; with 6. c4-d3 white sets up a king, and after 6. … d1-e2 7. a4-b5! black has to capture: 7. … e2×c4 8. b5×d3.</p>'''),
                 fig55, f'<p class="var">{h55}</p>', '<div style="clear:both"></div>']
        flush('twee-5x5-val.html')
        # driving out the king: the 4x4 draw position ported to the double corner a7/b8 of 8x8. With white to move white
        # drives the black king out as on 4x4, but there is no second double corner nearby to escape to; with black to
        # move black steps to the other edge square, the position is the mirror image in the diagonal a8-h1, and white
        # drives it out again
        def optimal(fen, text):         # every move of the line: white wins fastest, black defends longest
            f = fen
            for (mv,), _ in self.plan(text):
                ms = e21.analyse(f)[1]
                m = next(x for x in ms if x[0] == mv)
                w = f[0] == 'W'
                assert m[1][0] == ('W' if w else 'L') and int(m[1][1:]) == (min if w else max)(
                    int(x[1][1:]) for x in ms if x[1][0] == m[1][0]), (mv, m[1])
                f = m[2]
            return f
        fw, fb = e21.fen(['c5', 'd6'], ['a7'], 'W'), e21.fen(['c5', 'd6'], ['a7'], 'B')
        vw, vb = e21.analyse(fw)[0], e21.analyse(fb)[0]
        linew = '1. d6-c7 a7-b8 2. c5-d6 b8-a7 3. c7-b8 a7-b6 4. b8-a7 b6-a5 5. d6-e5 a5-b4 6. e5-d4 b4-a5 7. d4-c3 a5-b6 8. a7xc5'
        lineb = ('1. … a7-b8 2. c5-b6 b8-a7 3. d6-c5 a7-b8 4. b6-a7 b8-c7 5. a7-b8 c7-d8 6. c5-d4 d8-e7 7. d4-e5 e7-f8 '
                 '8. e5-f6 f8-g7 9. f6xh8')
        for fen, text in ((fw, linew), (fb, lineb)):
            end = optimal(fen, text)
            assert end.split(':')[2] == 'B'                 # the black king is captured
        # after 1. ... a7-b8 the position is the mirror image in the diagonal a8-h1 of the one with white to move
        mirror = lambda s: f'{chr(97 + 7 - (int(s[1:]) - 1))}{8 - (ord(s[0]) - 97)}'
        assert sorted(map(mirror, ['c5', 'd6'])) == ['c5', 'd6'] and mirror('a7') == 'b8'
        assert e21.analyse(e21.fen(['c5', 'd6'], ['b8'], 'W'))[0] == vw
        figw, iw = pg.diagram(fw, t(f'8×8, 2 tegen 1, wit aan zet: {vw}', f'8×8, 2 against 1, white to move: {vw}'), gid=k21)
        hw, *_ = pg.moves(fw, self.plan(linew), 1, False, 'm1', iw)
        figb, ib = pg.diagram(fb, t(f'8×8, 2 tegen 1, zwart aan zet: {vb}', f'8×8, 2 against 1, black to move: {vb}'), gid=k21)
        hb, *_ = pg.moves(fb, self.plan(lineb), 1, True, 'm1', ib)
        body += [t('<p>Bij 2 tegen 1 vlucht de zwarte dam naar een dubbele hoek, en wit moet hem daaruit verdrijven. '
                   'Neem de remisestelling van <a href="twee-4x4.html">4×4</a> en zet die op het 8×8-bord: het 4×4-bord '
                   'past in de linkerbovenhoek, met zijn dubbele hoek op a7/b8. Wit staat op c5 en d6, zwart op a7.</p>',
                   '<p>In 2 against 1 the black king flees to a double corner, and white has to drive it out. Take the '
                   'drawn position of <a href="twee-4x4.html">4×4</a> and put it on the 8×8 board: the 4×4 board fits in '
                   'the top left corner, with its double corner on a7/b8. White is on c5 and d6, black on a7.</p>'),
                 figw,
                 t('<p>Met wit aan zet drijft wit de zwarte dam uit de dubbele hoek, precies als op 4×4: na 3. c7-b8 moet '
                   'hij eruit (3. … a7-b6). Op 4×4 ontsnapt zwart dan naar de andere dubbele hoek, maar op 8×8 ligt die '
                   'te ver weg, en wit vangt de dam aan de rand.</p>',
                   '<p>With white to move white drives the black king out of the double corner, exactly as on 4×4: after '
                   '3. c7-b8 it has to leave (3. … a7-b6). On 4×4 black then escapes to the other double corner, but on '
                   '8×8 that one is too far away, and white catches the king at the edge.</p>'),
                 f'<p class="var">{hw}</p>', '<div style="clear:both"></div>',
                 figb,
                 t('<p>Met zwart aan zet gaat zwart naar het andere randveld van de dubbele hoek (1. … a7-b8). De stelling '
                   'is dan het spiegelbeeld van die met wit aan zet, gespiegeld in de diagonaal die de dubbele hoek '
                   'middendoor deelt, en wit drijft de dam opnieuw '
                   'uit de hoek, nu langs de bovenrand naar de enkele hoek h8.</p>',
                   '<p>With black to move black goes to the other edge square of the double corner (1. … a7-b8). The '
                   'position is then the mirror image of the one with white to move, mirrored in the diagonal that cuts '
                   'the double corner in half, and white '
                   'drives the king out of the corner again, now along the top edge to the single corner h8.</p>'),
                 f'<p class="var">{hb}</p>', '<div style="clear:both"></div>']
        flush('twee-verdrijven.html')
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
                  'dammen winnen ook tegen een zwarte dam in de dubbele hoek. Twee zwarte dammen bezetten daarom beide dubbele '
                  'hoeken, en per hoek twee witte dammen zou betekenen dat pas 4 tegen 2 wint. Het 8×8-bord is echter precies '
                  'klein genoeg om 2 zwarte dammen met 3 dammen te vangen: in de val doet de middelste witte dam dubbel werk. '
                  'Hij slaat terug na de ruil in '
                  'de ene hoek én na die in de andere, in de eerste val hieronder vanaf d4, in de tweede vanaf e5.</p>',
                  '<p>In <a href="twee-langste.html">2 against 1</a> the double corner is no safe haven: two white kings win '
                  'against a black king in the double corner too. 2 black kings therefore occupy both double corners, and two '
                  'white kings per corner would mean only 4 against 2 is winning. However, the 8×8 board is just small enough '
                  'to capture 2 black kings with 3 kings: in the trap the middle white king does double duty. It captures back '
                  'after the exchange in the one '
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
        x88 = self.active_passive(e88, (['c5', 'd4', 'e3'], ['g1', 'a7']), (['c7', 'e5', 'g3'], ['g1', 'a7']))
        body.append(t(f'<p>De twee vallen zijn van verschillende soort. De eerste is een actieve val, een dubbele '
                      f'ruildreiging: die dwingt meteen een ruil af, met zwart én met wit aan zet; met wit aan zet ruilt wit '
                      f'direct: {x88}. De tweede is een passieve val, een dubbele vangstdreiging: die geeft alleen met zwart aan '
                      f'zet meteen een ruil. Met wit aan zet in de passieve val moet wit van tempo wisselen om te winnen; dat '
                      f'staat op de <a href="val-pariteit.html">volgende pagina</a>.</p>',
                      f'<p>The two traps are of a different kind. The first is an active trap, a double exchange threat: it '
                      f'forces an exchange at once, with black and with white to move; with white to move white exchanges '
                      f'immediately: {x88}. The second is a passive trap, a double capture threat: it gives an immediate '
                      f'exchange with black to move only. With white to move in the passive trap, white needs to change the '
                      f'tempo in order to win; that is on the <a href="val-pariteit.html">next page</a>.</p>'))
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
            f'<p>Wit staat in de opstelling van de <a href="val.html">tweede, passieve val</a>, bij oneven pariteit, maar is zelf aan '
            f'zet, en dan is het geen val. Met de sterkste zetten stapt wit over naar de andere diagonaal, terwijl zwart '
            f'pendelt: ',
            f'<p>White stands in the set-up of the <a href="val.html">second, passive trap</a>, at odd parity, but is to move itself, '
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
        t = self.t
        partner89 = {'b8': 'a7', 'a7': 'b8', 'h8': 'i7', 'i7': 'h8'}
        x89 = self.active_passive(e89, (['d8', 'e7', 'f8'], ['b8', 'h8']), (['c7', 'e7', 'g7'], ['a7', 'i7']))
        act89 = [t('<p>Ook op 9×8 heeft wit twee vallen, hier allebei met zwart aan zet. Val 1 is de actieve, een dubbele '
                   'ruildreiging: wit op d8, e7 en f8 tegen b8 en h8. Val 2 is de passieve, een dubbele vangstdreiging: wit '
                   'op c7, e7 en g7 tegen a7 en i7.</p>',
                   '<p>On 9×8 too white has two traps, both shown here with black to move. Trap 1 is the active one, a '
                   'double exchange threat: white on d8, e7 and f8 against b8 and h8. Trap 2 is the passive one, a double '
                   'capture threat: white on c7, e7 and g7 against a7 and i7.</p>')] \
            + self.trap_blocks(pg, k89, e89, '9×8', '', [(['d8', 'e7', 'f8'], ['b8', 'h8']),
                                                          (['c7', 'e7', 'g7'], ['a7', 'i7'])], partner89)
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
                *act89,
                t('''
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
        trap2 = [
            t(f'<p>En met wit aan zet? De actieve val 1 dwingt met beide kleuren aan zet meteen een ruil af; met wit aan '
              f'zet ruilt wit direct: {x89}. De passieve val 2 geeft alleen met zwart aan zet meteen een ruil, en anders '
              f'dan op 8×8 kan wit vanuit die val geen tempo wisselen. Dat staat op de <a href="vesting-pariteit.html">'
              f'volgende pagina</a>.</p>',
              f'<p>And with white to move? The active trap 1 forces an exchange at once with either side to move; with '
              f'white to move white exchanges immediately: {x89}. The passive trap 2 gives an immediate exchange with '
              f'black to move only, and unlike on 8×8 white cannot change the tempo from that trap. That is on the '
              f'<a href="vesting-pariteit.html">next page</a>.</p>')]
        extra = [t(f'<p>Met wit aan zet wint de actieve val 1 van de <a href="vesting.html">vorige pagina</a> ook, want '
                   f'dan ruilt wit meteen: {x89}. De passieve val 2 wint met wit aan zet niet.</p>',
                   f'<p>With white to move the active trap 1 of the <a href="vesting.html">previous page</a> wins too, since '
                   f'then white exchanges at once: {x89}. The passive trap 2 does not win with white to move.</p>'), t(
            '<p>De twee vallen van de <a href="vesting.html">vorige pagina</a> gaan in elkaar over met een even aantal zetten: '
            'van de passieve val 2 (wit op c7, e7 en g7, zwart op a7 en i7) naar de actieve val 1 doen twee witte dammen elk één stap (c7-d8 en g7-f8) en pendelen beide '
            'zwarte dammen. Ze liggen dus bij dezelfde pariteit (oneven). Wit kan het tempo niet wisselen, anders dan op 8×8 met de '
            'overstap naar de andere diagonaal. De winst is hier niet strategisch: de oppositie beslist.</p>',
            '<p>The two traps of the <a href="vesting.html">previous page</a> turn into each other with an even number of '
            'moves: from the passive trap 2 (white on c7, e7 and g7, black on a7 and i7) to the active trap 1 two white kings take one step each (c7-d8 and g7-f8) and both black '
            'kings shuttle. They therefore lie at the same parity (odd). White cannot change the tempo, unlike on 8×8 with the '
            'change-over to the other diagonal. The win here is not strategic: the opposition decides.</p>')] \
            + self.no_tempo(pgp, k89, e89, '9×8', ['c7', 'e7', 'g7'], ['a7', 'i7'], '1. c7-d8 a7-b8 2. g7-f8 b8-a7',
                            t('de witte opstelling van val 1', 'the white set-up of trap 1'), all_moves=True)
        self.write('vesting-pariteit.html', None, '\n'.join(extra), pgp, k89)
        rule = next(k for k, x in enumerate(body) if isinstance(x, str) and 'D × d' in x)
        self.write('vesting.html', None, '\n'.join(body[:rule] + trap2), pg, k89)
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
<p>Op <a href="vesting-8x7-wit.html">8×7</a> verdringt wit een zwarte dam uit zijn dubbele hoek, en daar is geen andere
dubbele hoek om naartoe te vluchten. Op 7×7 wel: kan de verdreven dam steeds naar een volgende dubbele hoek ontsnappen, zoals
de zwarte dam bij <a href="twee-4x4.html">2 tegen 1 op 4×4</a>?</p>
<p>Op 7×7 met onspeelbare hoekvelden is elke hoek een dubbele hoek: b1/a2, f1/g2, a6/b7 en g6/f7. De dubbele hoeken
liggen 6 uit elkaar, aan dezelfde kant en diagonaal. Van de 28.276 rustige stellingen met zwart aan zet en beide zwarte
dammen op de randvelden van twee dubbele hoeken zijn er maar 16 remise, en daarin heeft zwart een schot.</p>
<p>De langste winst zonder slag voor beide kanten in de beginstelling duurt {v77[1:]} ply. Zwart vlucht met beide dammen
naar de dubbele hoeken langs de g-lijn: na 13. … f3-g2 staan ze op g6 en g2. Wit breekt de vesting door een zwarte dam
uit zijn dubbele hoek te verdrijven: met 19. f3-g2! bezet wit het vrije randveld naast de zwarte dam op f1, die gaat de hoek
uit (19. … f1-e2), en na 20. g2-f1 kan hij niet meer terug. Hij vlucht naar de dubbele hoek b1/a2, maar daar dwingt wit
met een offer de beslissing af: 26. b3-c2! b1×d3 27. f7-g6 f5-e6 28. d5×f7, en 2 tegen 1 wint.</p>
<p>Je zou kunnen denken dat zwart zo eindeloos van dubbele hoek naar dubbele hoek kan blijven vluchten, zoals de zwarte dam
bij <a href="twee-4x4.html">2 tegen 1 op 4×4</a>, en dat het remise is. Maar 7×7 is daar niet klein genoeg voor: wit vangt
de verdreven dam in de tweede dubbele hoek alsnog. Ook op <a href="langste-5x5.html">5×5</a> lukt het zwart niet.</p>''', f'''
<p>On <a href="vesting-8x7-wit.html">8×7</a> white drives a black king out of its double corner, and there is no other
double corner to flee to. On 7×7 there is: can the expelled king keep escaping to a next double corner, like the black king
in <a href="twee-4x4.html">2 against 1 on 4×4</a>?</p>
<p>On 7×7 with unplayable corner squares every corner is a double corner: b1/a2, f1/g2, a6/b7 and g6/f7. The double
corners are 6 apart, on the same side and diagonally. Of the 28,276 quiet positions with black to move and both black
kings on the edge squares of two double corners only 16 are drawn, and in those black has a shot.</p>
<p>The longest win without a capture for either side in the start position takes {v77[1:]} plies. Black flees with both
kings to the double corners along the g-file: after 13. … f3-g2 they stand on g6 and g2. White breaks the fortress by
driving a black king out of its double corner: with 19. f3-g2! white occupies the free edge square next to the black king
on f1, which leaves the corner (19. … f1-e2), and after 20. g2-f1 it cannot come back. It flees to the double corner
b1/a2, but there white forces the decision with a sacrifice: 26. b3-c2! b1×d3 27. f7-g6 f5-e6 28. d5×f7, and 2 against 1
wins.</p>
<p>One might think that black could keep fleeing from double corner to double corner forever like this, as the black king
does in <a href="twee-4x4.html">2 against 1 on 4×4</a>, and that it is a draw. But 7×7 is not small enough for that: white
still catches the expelled king in the second double corner. On <a href="langste-5x5.html">5×5</a> black does not manage
either.</p>'''), fig77, f'<p class="var">{h77}</p>', '<div style="clear:both"></div>']
        self.write('langste-7x7.html', None, '\n'.join(body77), pg77, k77)
        # 8x7: one pair of double corners, along the side of 7 (6 apart, closer than on 8x8): the fortress falls
        k87, e87 = self.engine(8, 7, 'unplayable')
        pg7 = self.page((k87, e87))
        partner = {'b1': 'a2', 'a2': 'b1', 'h1': 'i2', 'i2': 'h1', 'b7': 'a6', 'a6': 'b7', 'h7': 'i6', 'i6': 'h7'}
        name = '8×7'
        OW, OB = ['b3', 'c4', 'd7'], ['b1', 'a6']           # a trap at odd parity
        EW, EB = ['b3', 'c4', 'b5'], ['b1', 'b7']           # even parity: no trap, white drives a black king out
        body7 = [t('''
<p>Op 8×8 liggen de twee dubbele hoeken 7 velden uit elkaar, en daar valt de vesting altijd. Op 9×8 liggen ze 8 uit elkaar,
en daar beslist de oppositie. Op 8×7 zijn er precies twee dubbele hoeken, b1/a2 en b7/a6, allebei langs de zijde van 7, en
ze liggen maar 6 uit elkaar. (Met speelbare in plaats van onspeelbare hoekvelden is 8×7 het spiegelbeeld, met de dubbele
hoeken langs de h-lijn.)</p>
<p>Met zwart aan zet en beide zwarte dammen op de randvelden van hun dubbele hoeken (rustige stellingen) is bij oneven
pariteit elke stelling verloren (3.172), en bij even pariteit alle op 4 na (3.153 van de 3.157). Maar het gaat op twee
verschillende manieren: bij oneven pariteit zijn er 8 vallen, bij even pariteit geen enkele.</p>''', '''
<p>On 8×8 the two double corners are 7 squares apart, and there the fortress always falls. On 9×8 they are 8 apart, and
there the opposition decides. On 8×7 there are exactly two double corners, b1/a2 and b7/a6, both along the side of 7, and they are
only 6 apart. (With playable instead of unplayable corner squares 8×7 is the mirror image, with the double corners along
the h-file.)</p>
<p>With black to move and both black kings on the edge squares of their double corners (quiet positions) every position is
lost at odd parity (3,172), and all but 4 at even parity (3,153 of 3,157). But in two different ways: at odd parity there
are 8 traps, at even parity not a single one.</p>''')]
        x87 = self.active_passive(e87, (['c4', 'd5', 'e6'], ['a2', 'a6']), (OW, OB))
        body7.append(t(f'<p>Bij oneven pariteit breekt wit de vesting met een val, zoals op 8×8. Er zijn 8 vallen: de oneven '
                       f'vallen zijn actief (met wit aan zet ruilt wit in val 1 meteen: {x87}), de even vallen passief (met wit '
                       f'aan zet is het geen val). Het bord is symmetrisch onder het verwisselen van boven en onder, en op spiegeling '
                       f'na zijn er 4 vallen; hieronder staan ze alle 4.</p>',
                       f'<p>At odd parity white breaks the fortress with a trap, as on 8×8. There are 8 traps: the odd-numbered '
                       f'traps are active (with white to move white exchanges at once in trap 1: {x87}), the even-numbered ones '
                       f'passive (with white to move it is no trap). The board is symmetric under swapping top and bottom, and up '
                       f'to the mirror image there are 4 traps; all 4 are shown below.</p>'))
        traps87 = [(['c4', 'd5', 'e6'], ['a2', 'a6']), (OW, OB), (['a4', 'd5', 'e6'], ['a2', 'a6']), (OW, ['a2', 'b7'])]
        mirror = lambda xs: [x[0] + str(8 - int(x[1:])) for x in xs]
        assert all(bool(self.exchange(e87, e87.fen(W, B, 'W'))) == (k % 2 == 0)
                   for k, (W, B) in enumerate(traps87 + [(mirror(W), mirror(B)) for W, B in traps87]))
        body7 += self.trap_blocks(pg7, k87, e87, name, '', traps87, partner)
        # with white to move: the active trap exchanges at once, the passive one does not, and a tempo change along the edge
        # does not work; how white wins all the same is on the next page
        fw = e87.fen(OW, OB, 'W')
        vw, legalw = e87.analyse(fw)
        assert vw.startswith('W') and self.exchange(e87, fw) is None
        figw, iw = pg7.diagram(fw, t(f'{name}, passieve val 2, wit aan zet: {vw}', f'{name}, passive trap 2, white to move: {vw}'),
                               gid=k87)
        body7 += [t(f'<p>En met wit aan zet? De actieve val 1 dwingt met beide kleuren aan zet meteen een ruil af; met wit aan '
                    f'zet ruilt wit direct: {x87}. De passieve val 2 geeft alleen met zwart aan zet meteen een ruil. Op '
                    f'<a href="val-pariteit.html">8×8</a> wisselt wit dan van tempo op de diagonaal; op '
                    f'<a href="vesting-pariteit.html">9×8</a> lukt dat langs de rand niet, en op 8×7 evenmin. Toch is de '
                    f'passieve val met wit aan zet hier gewonnen ({vw}).</p>',
                    f'<p>And with white to move? The active trap 1 forces an exchange at once with either side to move; with '
                    f'white to move white exchanges immediately: {x87}. The passive trap 2 gives an immediate exchange with '
                    f'black to move only. On <a href="val-pariteit.html">8×8</a> white then changes the tempo on the diagonal; '
                    f'on <a href="vesting-pariteit.html">9×8</a> that does not work along the edge, and on 8×7 it does not '
                    f'either. Still the passive trap with white to move is won here ({vw}).</p>'),
                  figw, t('<p>Alle zetten van wit: ', '<p>All moves of white: ') + self.moves_list(pg7, iw, legalw, '1. ') + '.</p>',
                  '<div style="clear:both"></div>',
                  t('<p>Bij even pariteit is er helemaal geen val. Hoe wit daar wint, en in de passieve val met wit aan zet, staat '
                    'op de <a href="vesting-8x7-wit.html">volgende pagina</a>: door een zwarte dam uit zijn dubbele hoek te '
                    'verdringen.</p>',
                    '<p>At even parity there is no trap at all. How white wins there, and in the passive trap with white to move, '
                    'is on the <a href="vesting-8x7-wit.html">next page</a>: by driving a black king out of its double '
                    'corner.</p>')]
        self.write('vesting-8x7.html', None, '\n'.join(body7), pg7, k87)
        # driving a black king out of its double corner: at even parity (black to move), where there is no trap, and from the
        # passive trap with white to move, where there is no immediate exchange and no tempo change along the edge
        pgv = self.page((k87, e87))
        fe = e87.fen(EW, EB, 'B')
        ve, legale = e87.analyse(fe)
        fige, ie = pgv.diagram(fe, t(f'{name}, zwart aan zet (even pariteit): {ve}', f'{name}, black to move (even parity): {ve}'),
                               gid=k87)
        lines = []
        for line in ('1. b7-a6 2. b3-a2! b1-c2 3. a2-b1 c2-b3 4. c4xa2 a6xc4', '1. b1-a2 2. b5-a6! b7-c6 3. a6-b7 c6-b5 4. c4xa6 a2xc4'):
            h, _, f_end, _ = pgv.moves(fe, self.plan(line), 1, True, 'm1', ie)
            assert f_end[0] == 'W' and e87.analyse(f_end)[0].startswith('W') and all(
                len([x for x in side[1:].split(',') if x]) == n for side, n in zip(f_end.split(':')[1:], (2, 1)))
            lines.append(h)
        # after the invasion, the other waiting move (with the other king) loses even faster
        after = e87.analyse(e87.fen(['a2', 'c4', 'b5'], ['b1', 'a6'], 'B'))[1]
        val = {m[0]: m[1] for m in after}
        assert set(val) == {'b1-c2', 'a6-b7'} and all(v[0] == 'L' for v in val.values()) and int(val['a6-b7'][1:]) < int(val['b1-c2'][1:])
        figw2, iw2 = pgv.diagram(fw, t(f'{name}, passieve val 2, wit aan zet: {vw}', f'{name}, passive trap 2, white to move: {vw}'),
                                 gid=k87)
        linew = '1. d7-c6 a6-b7 2. c6-b5 b7-a6 3. b3-a2! b1-c2 4. a2-b1 c2-b3 5. c4xa2 a6xc4'
        hw, _, fend, _ = pgv.moves(fw, self.plan(linew), 1, False, 'm1', iw2)
        assert fend[0] == 'W' and e87.analyse(fend)[0].startswith('W') and all(
            len([x for x in side[1:].split(',') if x]) == n for side, n in zip(fend.split(':')[1:], (2, 1)))
        f = fw                              # after 2. ... b7-a6: the invasion above after 1. ... b7-a6
        for mv in [m for m, _ in self.plan(linew)][:4]:
            f = next(x[2] for x in e87.analyse(f)[1] if x[0] == mv[0])
        same = lambda a, b: a[0] == b[0] and [set(x[1:].split(',')) for x in a.split(':')[1:]] == [set(x[1:].split(',')) for x in b.split(':')[1:]]
        assert same(f, e87.fen(EW, ['b1', 'a6'], 'W'))
        for W, B, even in ((EW, EB, True), (OW, OB, False)):
            assert (rank_parity(e87, e87.fen(W, B, 'B')) == 0) == even
        for W in (['b5', 'c4', 'd5'], ['b5', 'c4', 'd3'], ['b3', 'c4', 'd5'], ['b3', 'c4', 'd3']):
            assert e87.analyse(e87.fen(W, ['a2', 'a6'], 'B'))[0] == 'D'
        bodyv = [t('<p>Waar geen val helpt, bij even pariteit en in de passieve val met wit aan zet, wint wit op 8×7 toch: de '
                   'dubbele hoeken liggen zo dicht bij elkaar dat wit erin kan binnendringen en een zwarte dam eruit '
                   'verdringt.</p>',
                   '<p>Where no trap helps, at even parity and in the passive trap with white to move, white still wins on 8×7: '
                   'the double corners are so close together that white can invade them and drive a black king out.</p>'),
                 fige, t('<p>Bij even pariteit, zwart aan zet: pendelt de ene zwarte dam, dan bezet wit het vrije randveld naast '
                         'de andere. Die heeft geen wachtzet meer en moet de dubbele hoek uit (de wachtzet met de eerste dam '
                         'verliest nog sneller). Daarna gaat wit zelf op het randveld staan dat de zwarte dam verliet, en die '
                         'kan niet meer terug. ',
                         '<p>At even parity, black to move: when the one black king shuttles, white occupies the free edge '
                         'square next to the other one. It has no waiting move left and has to leave the double corner (the '
                         'waiting move with the first king loses even faster). Then white steps onto the edge square the black '
                         'king left, and it cannot come back. ')
                 + lines[0] + t(', en 2 tegen 1 wint. Pendelt zwart in de onderste hoek, dan gaat het net zo in de bovenste: ',
                                ', and 2 against 1 wins. If black shuttles in the lower corner, it goes the same way in the '
                                'upper one: ')
                 + lines[1] + t('. Alle zetten van zwart: ', '. All moves of black: ')
                 + self.moves_list(pgv, ie, legale, '1. … ') + '.</p>', '<div style="clear:both"></div>',
                 figw2, t('<p>In de passieve val 2 met wit aan zet (<a href="vesting-8x7.html">vorige pagina</a>) loopt wit met '
                          'zijn derde dam over terwijl zwart pendelt, en na 2. … b7-a6 staat de stelling van het binnendringen '
                          'hierboven op het bord (na 1. … b7-a6): ',
                          '<p>In the passive trap 2 with white to move (<a href="vesting-8x7.html">previous page</a>) white walks '
                          'his third king over while black shuttles, and after 2. … b7-a6 the position of the invasion above is '
                          'on the board (after 1. … b7-a6): ')
                 + hw + t(', en 2 tegen 1 wint.</p>', ', and 2 against 1 wins.</p>'), '<div style="clear:both"></div>',
                 t('<p>De vesting valt dus bij beide pariteiten, op de <a href="val.html">twee manieren</a> om de vesting te '
                   'breken, en zonder tempowissel: bij oneven pariteit met een ruil (een val), bij even pariteit doordat een '
                   'zwarte dam zijn dubbele hoek uit moet en niet meer terug kan. Alleen bij zwart op a2 en a6 en wit op c4, b3 '
                   'of b5 en d3 of d5 (wit op b5, c4 en d5 bijvoorbeeld) is het remise.</p>',
                   '<p>The fortress therefore falls at both parities, in the <a href="val.html">two ways</a> to break the '
                   'fortress, and without a tempo change: at odd parity with an exchange (a trap), at even parity because a '
                   'black king has to leave its double corner and cannot come back. Only with black on a2 and a6 and white on '
                   'c4, b3 or b5 and d3 or d5 (white on b5, c4 and d5 for example) is it a draw.</p>'),
                 # counted over every quiet position with white to move and the black kings on the edge squares of
                 # b1/a2 and b7/a6: 8,281, all won, 111 of them with an exchange at once
                 t('<p>Met wit aan zet en beide zwarte dammen op de randvelden van hun dubbele hoeken (rustige stellingen) '
                   'zijn alle 8.281 stellingen gewonnen voor wit, bij beide pariteiten, en maar 111 daarvan met een directe '
                   'ruil. Op 8×7 valt de vesting dus met beide kleuren aan zet: waar een val noch een tempowissel helpt, '
                   'liggen de dubbele hoeken dicht genoeg bij elkaar om een zwarte dam eruit te verdringen.</p>'
                   '<p>Maar kan zo\'n verdreven dam niet naar een andere dubbele hoek vluchten? Op 8×7 is er geen andere; '
                   'op <a href="langste-7x7.html">7×7</a> wel.</p>',
                   '<p>With white to move and both black kings on the edge squares of their double corners (quiet positions) '
                   'all 8,281 positions are won for white, at both parities, and only 111 of them with an exchange at once. On '
                   '8×7 the fortress therefore falls with either side to move: where neither a trap nor a tempo change helps, '
                   'the double corners are close enough together to drive a black king out.</p>'
                   '<p>But can such an expelled king not flee to another double corner? On 8×7 there is none; on '
                   '<a href="langste-7x7.html">7×7</a> there is.</p>')]
        self.write('vesting-8x7-wit.html', None, '\n'.join(bodyv), pgv, k87)
        # the distance rule, with the two pairs of 9x7 that are 8 apart
        k97, e97 = self.engine(9, 7, 'unplayable')
        name = t('9×7 onspeelbare hoeken', '9×7 unplayable corners')
        pga = self.page((k97, e97))
        rulebody = [t('<p>Op 9×7 met onspeelbare hoekvelden zijn er vier dubbele hoeken: b1/a2, h1/i2, b7/a6 en h7/i6. '
                      'Langs de zijde van 7 liggen ze 6 uit elkaar, zoals op <a href="vesting-8x7.html">8×7</a>; de twee '
                      'andere soorten paren liggen 8 uit elkaar: langs de lange kant, zoals op 9×8, en diagonaal. Bij beide zijn er 2 vallen bij even pariteit en geen bij oneven: de oppositie beslist. '
                      'Langs de lange kant zijn het dezelfde twee vallen als op 9×8, de actieve val 1 en de passieve val 2:</p>',
                      '<p>On 9×7 with unplayable corner squares there are four double corners: b1/a2, h1/i2, b7/a6 and '
                      'h7/i6. Along the side of 7 they are 6 apart, as on <a href="vesting-8x7.html">8×7</a>; the two other '
                      'kinds of pairs are 8 apart: along the long side, as on 9×8, and diagonally. '
                      'For both there are 2 traps at even parity and none at odd: the opposition decides. Along the long side '
                      'they are the same two traps as on 9×8, the active trap 1 and the passive trap 2:</p>')]
        rulebody += self.trap_blocks(pga, k97, e97, name + ',', t('lange kant', 'long side'),
                                     [(['d1', 'e2', 'f1'], ['b1', 'h1']), (['c2', 'e2', 'g2'], ['a2', 'i2'])],
                                     partner)
        self.active_passive(e97, (['d1', 'e2', 'f1'], ['b1', 'h1']), (['c2', 'e2', 'g2'], ['a2', 'i2']))
        rulebody.append(t('<p>Diagonaal: op <a href="vesting-9x9.html">9×9</a> is de diagonaal te lang, op 9×7 niet. De drie '
                          'witte dammen staan op een rij op de verbindingsdiagonaal b1–h7, net als op 8×8:</p>',
                          '<p>Diagonally: on <a href="vesting-9x9.html">9×9</a> the diagonal is too long, on 9×7 it is not. The '
                          'three white kings stand in a row on the connecting diagonal b1–h7, as on 8×8:</p>'))
        rulebody += self.trap_blocks(pga, k97, e97, name + ',', t('diagonaal', 'diagonal'), [(['d3', 'e4', 'f5'], ['b1', 'h7'])], partner)
        rulebody.append(t('<p>Toch beslist hier de oppositie, want er is geen val op een tweede diagonaal. Op 8×8 verbinden twee '
                          'diagonalen de dubbele hoeken, één voor elke helft, en door van de ene naar de andere over te stappen '
                          'wisselt wit het tempo. Op 9×7 liggen alleen de hoeken b1 en h7 op één diagonaal; de andere helften '
                          'van de dubbele hoeken, a2 en i6, liggen niet op een gemeenschappelijke diagonaal. Er is dus wel een tweede '
                          'val, wit op c2, e4 en g6 tegen a2 en i6, maar die staat op dezelfde diagonaal b1–h7, bij dezelfde '
                          'pariteit:</p>',
                          '<p>Still the opposition decides here, because there is no trap on a second diagonal. On 8×8 two '
                          'diagonals connect the double corners, one for each half, and by moving over from the one to the other '
                          'white changes the tempo. On 9×7 only the corners b1 and h7 lie on one diagonal; the other halves of '
                          'the double corners, a2 and i6, do not lie on a common diagonal. So there is a second trap, white on c2, e4 '
                          'and g6 against a2 and i6, but it stands on the same diagonal b1–h7, at the same parity:</p>'))
        rulebody += self.trap_blocks(pga, k97, e97, name + ',', t('diagonaal', 'diagonal'), [(['c2', 'e4', 'g6'], ['a2', 'i6'])], partner, 2)
        x97 = self.active_passive(e97, (['d3', 'e4', 'f5'], ['b1', 'h7']), (['c2', 'e4', 'g6'], ['a2', 'i6']))
        rulebody.append(t('<p>Omdat beide vallen op dezelfde diagonaal staan, kost de overstap van val 2 naar val 1 twee witte '
                          'zetten (c2-d3 en g6-f5), terwijl zwart twee keer pendelt (a2-b1 en i6-h7): een even aantal, dus het '
                          f'tempo draait niet om. Val 1 is de actieve val (met wit aan zet ruilt wit meteen: {x97}), val 2 de '
                          'passieve, die alleen met zwart aan zet wint; en vanuit de passieve val kan wit hier geen tempo '
                          'wisselen.</p>',
                          '<p>Because both traps stand on the same diagonal, the change-over from trap 2 to trap 1 costs two white '
                          'moves (c2-d3 and g6-f5), while black shuttles twice (a2-b1 and i6-h7): an even number, so the tempo '
                          f'does not turn over. Trap 1 is the active trap (with white to move white exchanges at once: {x97}), '
                          'trap 2 the passive one, which wins with black to move only; and from the passive trap white cannot '
                          'change the tempo here.</p>'))
        # the longest win with black in the diagonally opposite double corners (even parity, black to move)
        f97 = e97.fen(['b7', 'a6', 'a4'], ['i6', 'b1'], 'B')
        v97, _ = e97.analyse(f97)
        line97 = ('1. i6-h5 2. b7-c6 h5-g4 3. a6-b5 g4-f3 4. c6-d5 b1-c2 5. b5-c4 c2-b1 6. a4-b5 b1-c2 7. b5-c6 c2-b1 '
                  '8. c4-d3 f3-g4 9. d5-e4 b1-a2 10. c6-d7 g4-h3 11. d7-e6 h3-g4 12. d3-c2 g4-h5 13. e6-f5 a2-b1 '
                  '14. e4-d3 b1-a2 15. d3-e2 a2-b1 16. c2-d1 h5-i6 17. f5-g4 i6-h7 18. e2-d3 h7-i6 19. d3-e4 i6-h7 '
                  '20. g4-f5 b1-a2 21. d1-c2 h7-i6 22. f5-g6 i6-h7 23. c2-d3 h7xf5 24. e4xg6')
        g = f97
        for k, ((mv,), _) in enumerate(self.plan(line97)):   # black a longest defence, white a strongest move
            ms = e97.analyse(g)[1]
            m = next(x for x in ms if x[0] == mv)
            w = g[0] == 'W'
            assert m[1][0] == ('W' if w else 'L') and int(m[1][1:]) == (min if w else max)(
                int(x[1][1:]) for x in ms if x[1][0] == m[1][0]), (mv, m[1])
            g = m[2]
            if mv == 'f5-g6':                  # black to move in the second trap on the diagonal b1-h7
                key = lambda x: [sorted(p[1:].split(',')) for p in x.split(':')]
                assert key(g) == key(e97.fen(['c2', 'e4', 'g6'], ['a2', 'i6'], 'B')), (g, e97.fen(['c2', 'e4', 'g6'], ['a2', 'i6'], 'B'))
        assert [len([x for x in side[1:].split(',') if x]) for side in g.split(':')[1:]] == [2, 1] and e97.analyse(g)[0][0] == 'L'
        assert rank_parity(e97, f97) == 0
        fig97, i97 = pga.diagram(f97, t(f'{name}, zwart aan zet: {v97}', f'{name}, black to move: {v97}'), gid=k97)
        h97, *_ = pga.moves(f97, self.plan(line97), 1, True, 'm1', i97)
        rulebody += [t(f'<p>De langste winst met zwart in de diagonaal tegenover elkaar liggende dubbele hoeken, bij even '
                       f'pariteit en zwart aan zet, duurt {v97[1:]} ply. Zwart verlaat de hoeken nog even, maar wit zet zijn '
                       f'dammen rustig op de diagonaal b1–h7 en bouwt daar de tweede val: na 22. f5-g6 staat zwart aan zet '
                       f'met wit op c2, e4 en g6 tegen a2 en i6, en op 22. … i6-h7 volgt 23. c2-d3 h7×f5 24. e4×g6. Wit kan '
                       f'dat afdwingen omdat de pariteit goed is: zwart kan het tempo niet wisselen, dus wit staat met de val '
                       f'klaar terwijl zwart aan zet is.</p>',
                       f'<p>The longest win with black in the diagonally opposite double corners, at even parity and black to '
                       f'move, takes {v97[1:]} plies. Black leaves the corners for a while, but white calmly places his kings '
                       f'on the diagonal b1–h7 and builds the second trap there: after 22. f5-g6 black is to move with white '
                       f'on c2, e4 and g6 against a2 and i6, and 22. … i6-h7 is met by 23. c2-d3 h7×f5 24. e4×g6. White can '
                       f'force this because the parity is right: black cannot change the tempo, so white stands ready with the '
                       f'trap while black is to move.</p>'),
                     fig97, f'<p class="var">{h97}</p>', '<div style="clear:both"></div>']
        diag = lambda a, b: abs(ord(a[0]) - ord(b[0])) == abs(int(a[1:]) - int(b[1:]))
        assert diag('b1', 'h7') and not diag('a2', 'i6') and all(diag('b1', x) for x in ('c2', 'e4', 'g6', 'd3', 'f5'))
        for W, B in ((['c2', 'e2', 'g2'], ['a2', 'i2']), (['d3', 'e4', 'f5'], ['b1', 'h7']), (['c2', 'e4', 'g6'], ['a2', 'i6'])):
            assert rank_parity(e97, e97.fen(W, B, 'B')) == 0 and e97.analyse(e97.fen(W, B, 'B'))[0][0] == 'L'
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
            '<p>Zie de vallen op <a href="val.html">8×8</a>, <a href="vesting.html">9×8</a> en <a href="vesting-8x7.html">8×7</a>, '
            'en de langste winsten op <a href="langste-7x7.html">7×7</a> en <a href="langste-5x5.html">5×5</a>.</p>',
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
            '<p>See the traps on <a href="val.html">8×8</a>, <a href="vesting.html">9×8</a> and <a href="vesting-8x7.html">8×7</a>, '
            'and the longest wins on <a href="langste-7x7.html">7×7</a> and <a href="langste-5x5.html">5×5</a>.</p>'),
            self.page((k89, e89)), None)


    def board55(self):
        """5x5 with unplayable corners (12 squares, every corner a double corner): 2 against 1 always wins, 3 against 2
        nearly always, the longest win drives a black king out of its double corner, and the only draws are zugzwang"""
        from itertools import combinations
        from collections import Counter
        t = self.t
        n = lambda x: f'{x:,}'.replace(',', '.' if self.lang == 'nl' else ',')
        k55, e55 = self.engine(5, 5, 'unplayable')
        k21, e21 = self.engine(5, 5, 'unplayable', 2, 1)
        squares = sorted(f'{chr(97 + f)}{r + 1}' for f, r in e55.g['sq'].values())
        count = lambda f: tuple(len([x for x in side[1:].split(',') if x]) for side in f.split(':')[1:])
        captures = lambda e, f: any(count(m[2]) != count(f) for m in e.analyse(f)[1])

        def quiet(e, W, B):              # nobody can capture
            return not captures(e, e.fen(W, B, 'W')) and not captures(e, e.fen(W, B, 'B'))

        def positions(e, nw, nb):
            for W in combinations(squares, nw):
                for B in combinations([s for s in squares if s not in W], nb):
                    if quiet(e, list(W), list(B)):
                        yield list(W), list(B)
        # the 8 symmetries of the board (mirrors and rotations)
        syms = [lambda f, r: (f, r), lambda f, r: (4 - f, r), lambda f, r: (f, 4 - r), lambda f, r: (4 - f, 4 - r),
                lambda f, r: (r, f), lambda f, r: (4 - r, f), lambda f, r: (r, 4 - f), lambda f, r: (4 - r, 4 - f)]
        tr = lambda s, g: (lambda f, r: f'{chr(97 + f)}{r + 1}')(*g(ord(s[0]) - 97, int(s[1:]) - 1))
        canon = lambda W, B: min((tuple(sorted(tr(x, g) for x in W)), tuple(sorted(tr(x, g) for x in B))) for g in syms)

        # 2 against 1: every quiet position is won, with either side to move
        q21 = list(positions(e21, 2, 1))
        assert all(e21.analyse(e21.fen(W, B, 'W'))[0][0] == 'W' and e21.analyse(e21.fen(W, B, 'B'))[0][0] == 'L' for W, B in q21)
        # 3 against 2: the quiet positions, the fortress (black on the edge squares of two double corners)
        q = list(positions(e55, 3, 2))
        wtm = {(tuple(W), tuple(B)): e55.analyse(e55.fen(W, B, 'W'))[0] for W, B in q}
        btm = {(tuple(W), tuple(B)): e55.analyse(e55.fen(W, B, 'B'))[0] for W, B in q}
        corners = [{'b1', 'a2'}, {'d1', 'e2'}, {'a4', 'b5'}, {'d5', 'e4'}]
        cid = lambda s: next((k for k, c in enumerate(corners) if s in c), None)
        fort = [x for x in q if None not in map(cid, x[1]) and cid(x[1][0]) != cid(x[1][1])]
        cw, cb = Counter(v[0] for v in wtm.values()), Counter(v[0] for v in btm.values())
        fb = Counter(btm[(tuple(W), tuple(B))][0] for W, B in fort)
        assert set(cw) == {'W', 'D'} and set(cb) == {'L', 'D'} and set(fb) == {'L', 'D'}
        # all counts up to the symmetries of the board
        orb = lambda xs: len({canon(list(W), list(B)) for W, B in xs})
        n21, nq, nf = orb(q21), orb(q), orb(fort)
        dw = orb(x for x, v in wtm.items() if v == 'D')
        db = orb(x for x, v in btm.items() if v == 'D')
        df = orb((W, B) for W, B in fort if btm[(tuple(W), tuple(B))] == 'D')

        # the longest win from a quiet start, white to move: white wins fastest, black defends longest
        top = max(int(v[1:]) for v in wtm.values() if v[0] == 'W')
        starts = {canon(*x) for x, v in wtm.items() if v == f'W{top}'}
        f55 = e55.fen(['a4', 'b5', 'd5'], ['d3', 'e4'], 'W')
        v55, _ = e55.analyse(f55)
        assert v55 == f'W{top}'
        line55 = ('1. d5-c4 d3-e2 2. a4-b3 e4-d3 3. b3-a2 d3-e4 4. c4-d5 e2-d3 5. a2-b1 d3-e2 6. b1-c2 e2-d3 7. c2-d1 d3-e2 '
                  '8. b5-a4 e2-d3 9. d1-e2! d3-c2 10. d5-c4 c2-d1 11. c4-b3 e4-d5 12. e2-d3 d5-e4 13. b3-c2! d1xb3 14. a4xc2')
        g = f55
        for (mv,), _ in self.plan(line55):
            ms = e55.analyse(g)[1]
            m = next(x for x in ms if x[0] == mv)
            w = g[0] == 'W'
            assert m[1][0] == ('W' if w else 'L') and int(m[1][1:]) == (min if w else max)(
                int(x[1][1:]) for x in ms if x[1][0] == m[1][0]), (mv, m[1])
            g = m[2]
        assert count(g) == (2, 1) and e55.analyse(g)[0][0] == 'L'
        pg = self.page((k55, e55))
        fig, i0 = pg.diagram(f55, t(f'5×5, wit aan zet: {v55}', f'5×5, white to move: {v55}'), gid=k55)
        h55, *_ = pg.moves(f55, self.plan(line55), 1, False, 'm1', i0)
        body = [t(f'''
<p>Op <a href="langste-7x7.html">7×7</a> vangt wit een verdreven zwarte dam ook in de volgende dubbele hoek. Hoe gaat het
op een nog kleiner bord? Op 5×5 met onspeelbare hoekvelden zijn er maar 12 speelbare velden, en elke hoek is een dubbele
hoek: b1/a2, d1/e2, a4/b5 en d5/e4. Het bord heeft 8 symmetrieën (spiegelen en draaien), en alle aantallen in dit
hoofdstuk tellen stellingen op symmetrie na.</p>
<p>Anders dan op <a href="twee-4x4.html">4×4</a> is <a href="twee-5x5.html">2 tegen 1</a> hier altijd gewonnen: wit wint alle {n21} rustige
stellingen, met wit en met zwart aan zet. Ook 3 tegen 2 is bijna altijd gewonnen: van de {n(nq)} rustige stellingen met wit
aan zet zijn er {dw} remise, met zwart aan zet {db}. Met zwart aan zet en beide zwarte dammen op de randvelden van twee
dubbele hoeken zijn er {df} remise van de {n(nf)}. Die remises zijn geen vesting: zwart dwingt binnen 4 ply gelijk materiaal
af, vaak door zetdwang (zie de <a href="zetdwang-5x5.html">uitzondering</a>).</p>
<p>De langste winst vanuit een rustige stelling met wit aan zet duurt {top} ply (er zijn {len(starts)} zulke
beginstellingen). Hier begint wit met twee dammen in de dubbele hoek a4/b5. Zwart houdt zijn dammen bij
de dubbele hoeken d1/e2 en d5/e4. Wit loopt met een dam via a2, b1 en c2 naar d1 (7. c2-d1), het vrije randveld naast de
zwarte dam op e2, en neemt met 9. d1-e2! ook e2 in: de zwarte dam moet de hoek uit (9. … d3-c2). Hij keert terug op d1,
maar daar beslist het offer 13. b3-c2! d1×b3 14. a4×c2, en 2 tegen 1 wint. Hoe wit de vesting breekt, staat bij de
<a href="vallen-5x5.html">vallen</a>.</p>''', f'''
<p>On <a href="langste-7x7.html">7×7</a> white catches an expelled black king in the next double corner as well. How does
it go on an even smaller board? On 5×5 with unplayable corner squares there are only 12 playable squares, and every corner
is a double corner: b1/a2, d1/e2, a4/b5 and d5/e4. The board has 8 symmetries (mirrors and rotations), and all counts in
this chapter count positions up to symmetry.</p>
<p>Unlike on <a href="twee-4x4.html">4×4</a>, <a href="twee-5x5.html">2 against 1</a> always wins here: white wins all {n21} quiet positions,
with white and with black to move. 3 against 2 too nearly always wins: of the {n(nq)} quiet positions with white to move
{dw} are drawn, with black to move {db}. With black to move and both black kings on the edge squares of two double corners
{df} of the {n(nf)} are drawn. These draws are no fortress: black forces equal material within 4 plies,
often by zugzwang (see the <a href="zetdwang-5x5.html">exception</a>).</p>
<p>The longest win from a quiet position with white to move takes {top} plies (there are {len(starts)} such start
positions). Here white starts with two kings in the double corner a4/b5. Black keeps his kings
at the double corners d1/e2 and d5/e4. White walks with a king via a2, b1 and c2 to d1 (7. c2-d1), the free edge square
next to the black king on e2, and with 9. d1-e2! takes e2 as well: the black king has to leave the corner (9. … d3-c2).
It returns to d1, but there the sacrifice 13. b3-c2! d1×b3 14. a4×c2 decides, and 2 against 1 wins. How white breaks the
fortress is shown with the <a href="vallen-5x5.html">traps</a>.</p>'''),
                fig, f'<p class="var">{h55}</p>', '<div style="clear:both"></div>']
        self.write('langste-5x5.html', None, '\n'.join(body), pg, k55)

        # the traps: black to move, both black kings on the edge squares of two double corners, nobody can capture; every
        # shuttle move of black allows a forced exchange, and every other black move loses material at once too. A white
        # king may stand on the free edge square of a black double corner: that king then cannot shuttle
        partner = {'b1': 'a2', 'a2': 'b1', 'd1': 'e2', 'e2': 'd1', 'a4': 'b5', 'b5': 'a4', 'd5': 'e4', 'e4': 'd5'}
        punished = lambda f: self.exchange(e55, f) or any(count(m[2])[1] < 2 and m[1].startswith('W') for m in e55.analyse(f)[1])
        traps, parity = [], Counter()
        for W, B in fort:
            fb = e55.fen(W, B, 'B')
            legal = e55.analyse(fb)[1]
            sh = {f'{b}-{partner[b]}' for b in B if partner[b] not in W}
            if sh and all(self.exchange(e55, m[2]) if m[0] in sh else punished(m[2]) for m in legal):
                traps.append((W, B))
                parity['even' if sum(int(x[1:]) for x in W + B) % 2 == 0 else 'odd'] += 1
        blocked = lambda W, B: any(partner[b] in W for b in B)
        orbits_t = {canon(W, B): blocked(W, B) for W, B in traps}
        nt, nblock = len(orbits_t), sum(orbits_t.values())
        assert nt == 40 and set(parity) == {'even', 'odd'}
        assert canon(['a2', 'b3', 'e2'], ['d1', 'e4']) in orbits_t and canon(['b3', 'c2', 'c4'], ['a2', 'a4']) in orbits_t
        pgt = self.page((k55, e55))
        body = [t(f'''
<p>Met zwart aan zet en beide zwarte dammen op de randvelden van twee dubbele hoeken breekt wit de vesting op 5×5 met een
val, zoals op <a href="val.html">8×8</a>: elke pendelzet van zwart staat een gedwongen ruil toe, en elke andere zwarte zet
verliest ook meteen materiaal. Op 5×5 mag een witte dam daarbij op het vrije randveld van een zwarte dubbele hoek staan;
die zwarte dam kan dan niet pendelen. Op spiegelen en draaien na zijn er zo {nt} vallen, zowel bij even als bij oneven
pariteit, van twee soorten.</p>
<p>In {nblock} van de {nt} blokkeert wit een hoek: een witte dam staat op het vrije randveld naast een zwarte dam, die dus
niet kan pendelen. Zwart moet met de andere dam pendelen, en twee witte dammen dwingen dan de ruil af. Hier blokkeert e2
de zwarte dam op d1; het offer staat op c4, en wit slaat terug vanaf a2.</p>''', f'''
<p>With black to move and both black kings on the edge squares of two double corners white breaks the fortress on 5×5
with a trap, as on <a href="val.html">8×8</a>: every shuttle move of black allows a forced exchange, and every other black
move loses material at once too. On 5×5 a white king may stand on the free edge square of a black double corner; that
black king then cannot shuttle. Up to mirrors and rotations there are thus {nt} traps, at even as well as at odd
parity, of two kinds.</p>
<p>In {nblock} of the {nt} white blocks a corner: a white king stands on the free edge square next to a black king, which
therefore cannot shuttle. Black has to shuttle with the other king, and two white kings then force the exchange. Here e2
blocks the black king on d1; the offer is on c4, and white captures back from a2.</p>''')]
        body += self.trap_blocks(pgt, k55, e55, '5×5', '', [(['a2', 'b3', 'e2'], ['d1', 'e4'])], partner)
        body += [t(f'''
<p>In de andere {nt - nblock} is het de dubbele dreiging van <a href="val.html">8×8</a>: niets is geblokkeerd, en beide
pendelzetten van zwart staan meteen een ruil toe.</p>''', f'''
<p>In the other {nt - nblock} it is the double threat of <a href="val.html">8×8</a>: nothing is blocked, and both shuttle
moves of black allow an exchange at once.</p>''')]
        body += self.trap_blocks(pgt, k55, e55, '5×5', '', [(['b3', 'c2', 'c4'], ['a2', 'a4'])], partner, 2)
        body += [t('<p>Steeds volgt 2 tegen 1, en dat wint op 5×5 altijd. Waar zwart toch remise houdt, staat bij de '
                   '<a href="zetdwang-5x5.html">uitzondering</a>.</p>',
                   '<p>Always 2 against 1 follows, and that always wins on 5×5. Where black holds a draw all the same is '
                   'under the <a href="zetdwang-5x5.html">exception</a>.</p>')]
        self.write('vallen-5x5.html', None, '\n'.join(body), pgt, k55)

        # the exception: every quiet draw with white to move is zugzwang (each white move gives a king away, and 2 against
        # 2 is a draw); with black to move black has exactly one drawing move, into such a zugzwang
        zw = {x for x, v in wtm.items() if v == 'D'}
        for W, B in zw:
            ms = e55.analyse(e55.fen(list(W), list(B), 'W'))[1]
            assert ms and all(any(count(r[2])[0] < 3 for r in e55.analyse(m[2])[1]) for m in ms)
        zb = [x for x, v in btm.items() if v == 'D']
        name = {str(n): f'{chr(97 + f)}{r + 1}' for n, (f, r) in e55.g['sq'].items()}
        quietw = lambda f: tuple(tuple(sorted(name[x.lstrip('K')] for x in side[1:].split(',') if x)) for side in f.split(':')[1:])
        # or else an offer or a capture threat (the position after it is not quiet); either way black forces equal material
        # (2 against 2 or 1 against 1) within 4 plies

        def equal(f, d):
            w, b = count(f)
            if w == b:
                return True
            ms = e55.analyse(f)[1]
            if d == 0 or w < b or not ms:
                return False
            return (all if f[0] == 'W' else any)(equal(m[2], d - 1) for m in ms)
        kind = {}
        for W, B in zb:
            dm = [m for m in e55.analyse(e55.fen(list(W), list(B), 'B'))[1] if m[1] == 'D']
            assert len(dm) == 1 and equal(dm[0][2], 4)
            kind[(W, B)] = 'z' if quietw(dm[0][2]) in zw else 's'
        nz, ns = orb(x for x, v in kind.items() if v == 'z'), orb(x for x, v in kind.items() if v == 's')
        fz = orb((W, B) for W, B in fort if btm[(tuple(W), tuple(B))] == 'D' and kind[(tuple(W), tuple(B))] == 'z')
        assert nz + ns == db
        orbits_w = sorted({canon(*x) for x in zw})
        pgz = self.page((k55, e55))
        fb0 = e55.fen(['a4', 'b5', 'e4'], ['b1', 'd5'], 'B')
        vb0, legalb = e55.analyse(fb0)
        figb, ib = pgz.diagram(fb0, t(f'5×5, zwart aan zet: {vb0}', f'5×5, black to move: {vb0}'), gid=k55)
        draw = next(m for m in legalb if m[1] == 'D')
        f1 = draw[2]
        v1, legal1 = e55.analyse(f1)
        fig1, i1 = pgz.diagram(f1, t(f'5×5, na 1. … {draw[0]}, wit aan zet: {v1}', f'5×5, after 1. … {draw[0]}, white to move: {v1}'),
                               gid=k55)
        other = [o for o in orbits_w if o != canon(*quietw(f1))]
        assert len(orbits_w) == 2 and len(other) == 1
        fo = e55.fen(list(other[0][0]), list(other[0][1]), 'W')
        vo, legalo = e55.analyse(fo)
        figo, io = pgz.diagram(fo, t(f'5×5, wit aan zet: {vo}', f'5×5, white to move: {vo}'), gid=k55)
        body = [t(f'''
<p>Op spiegelen en draaien na zijn er van de {n(nq)} rustige stellingen met wit aan zet maar {dw} remise, en in beide staat
wit in zetdwang: elke witte zet geeft een dam weg, en 2 tegen 2 is remise. Met zwart aan zet zijn er {db} remises, en in
elk ervan heeft zwart precies één remisezet. In {nz} ervan zet die zet wit in zo'n zetdwang; in de andere {ns} offert zwart een dam of
dreigt hij te slaan. Steeds dwingt zwart binnen 4 ply gelijk materiaal af: 2 tegen 2 of 1 tegen 1. Van de {df} remises met
beide zwarte dammen op de randvelden van twee dubbele hoeken zijn er {fz} zetdwang.</p>''', f'''
<p>Up to mirrors and rotations only {dw} of the {n(nq)} quiet positions with white to move are drawn, and in both white is
in zugzwang: every white move gives a king away, and 2 against 2 is a draw. With black to move there are {db} draws, and in
each of them black has exactly one drawing move. In {nz} of them that move puts white in such a zugzwang; in the other {ns} black offers a
king or threatens to capture. Always black forces equal material within 4 plies: 2 against 2 or 1 against 1. Of the {df}
draws with both black kings on the edge squares of two double corners {fz} are zugzwang.</p>'''),
                figb,
                t(f'''<p>Een voorbeeld: wit staat met twee dammen in de dubbele hoek a4/b5, en de derde op e4, naast de zwarte dam op d5. Zwart
redt zich alleen met 1. … {draw[0]}; beide hoekzetten verliezen. Alle zetten van zwart: '''
                  + self.moves_list(pgz, ib, legalb, '1. … ') + '.</p>', f'''<p>An example: white has two kings in the double corner a4/b5, and the third on e4, next to the black king on d5. Black
saves himself only with 1. … {draw[0]}; both corner moves lose. All moves of black: '''
                  + self.moves_list(pgz, ib, legalb, '1. … ') + '.</p>'), '<div style="clear:both"></div>',
                fig1, t('<p>Na 1. … ' + draw[0] + ' geeft elke witte zet een dam weg. Alle zetten van wit: ',
                        '<p>After 1. … ' + draw[0] + ' every white move gives a king away. All moves of white: ')
                + self.moves_list(pgz, i1, legal1, '2. ') + '.</p>', '<div style="clear:both"></div>',
                figo, t('<p>De andere zetdwangstelling: ook hier geeft elke witte zet een dam weg. Alle zetten van wit: ',
                        '<p>The other zugzwang position: here too every white move gives a king away. All moves of white: ')
                + self.moves_list(pgz, io, legalo, '1. ') + '.</p>', '<div style="clear:both"></div>']
        self.write('zetdwang-5x5.html', None, '\n'.join(body), pgz, k55)

    def active_passive(self, e, active, passive):
        """the active trap (a double exchange threat: with white to move white exchanges at once) and the passive one
        (a double capture threat: it wins with black to move only); returns the exchange of the active trap as text"""
        ex = self.exchange(e, e.fen(*active, 'W'))
        assert ex and self.exchange(e, e.fen(*passive, 'W')) is None
        return f'1. {ex[0][0]} {ex[1][0]} 2. {ex[2][0]}'

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
            free = {f'{b}-{partner[b]}' for b in B if partner[b] not in W}      # a blocked king cannot shuttle
            sh = self.shuttles(pg, gid, e, i0, legal, free)
            assert sh and len(sh) == len(free) and all(ok for _, ok, _ in sh), (fen, sh)
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

    def no_tempo(self, pg, gid, e, name, W, B, line, target, all_moves=False):
        """from a trap set-up with white to move, white steps over to another trap while black shuttles: an even number
        of moves, so white is to move again and it stays a draw"""
        t = self.t
        fen = e.fen(W, B, 'W')
        v, legal = e.analyse(fen)
        fig, i0 = pg.diagram(fen, t(f'{name}, wit aan zet: {v}', f'{name}, white to move: {v}'), gid=gid)
        assert not all_moves or all(m[1] == 'D' for m in legal)
        every = (t(' Elke witte zet houdt remise, want zwart pendelt gewoon: ', ' Every white move draws, since black just '
                   'shuttles: ') + self.moves_list(pg, i0, legal, '1. ') + '.') if all_moves else ''
        h, i_end, f_end, _ = pg.moves(fen, self.plan(line), 1, False, 'm1', i0, nbook=len(self.plan(line)))
        v2, _ = e.analyse(f_end)
        assert v == v2 == 'D' and f_end[0] == 'W'
        fig2, _ = pg.diagram(f_end, t(f'{name}, wit aan zet: {v2}', f'{name}, white to move: {v2}'), gid=gid)
        return [fig, t(f'<p>Met wit aan zet is de opstelling van de passieve val geen val.{every} Stapt wit over naar {target}, terwijl zwart '
                       f'pendelt, ', f'<p>With white to move the set-up of the passive trap is no trap.{every} If white steps over to '
                       f'{target} while black shuttles, ') + f'{h}' + t(
                       ', dan is dat na een even aantal zetten bereikt, en is wit weer aan zet: het tempo is niet '
                       'omgedraaid, en het blijft remise.</p>',
                       ', it is reached after an even number of moves, and white is to move again: the tempo has not turned '
                       'over, and it stays a draw.</p>'), '<div style="clear:both"></div>',
                fig2, t('<p>Wit staat in de opstelling van de actieve val 1, maar zwart pendelt met dezelfde dam heen en '
                        'terug en staat niet in de val; wit is aan zet: remise. (Zou zwart met beide dammen pendelen, dan '
                        'stond hij wel in val 1, met wit aan zet, en dwingt wit meteen de ruil af.)</p>',
                        '<p>White stands in the set-up of the active trap 1, but black shuttles with the same king to and fro '
                        'and is not in the trap; white is to move: a draw. (Were black to shuttle with both kings, he would '
                        'stand in trap 1, with white to move, and white forces the exchange at once.)</p>'), '<div style="clear:both"></div>']

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
                                 [(['i4', 'h5', 'i6'], ['i2', 'i8']), (['h3', 'h5', 'h7'], ['h1', 'h9'])],
                                 {'h1': 'i2', 'i2': 'h1', 'h9': 'i8', 'i8': 'h9'})
        x99 = self.active_passive(e99, (['i4', 'h5', 'i6'], ['i2', 'i8']), (['h3', 'h5', 'h7'], ['h1', 'h9']))
        body.append(t('<p>Ook hier gaan de twee vallen in elkaar over met een even aantal zetten (h3-i4 en h7-i6; de zwarte '
                      'dammen pendelen), en liggen ze bij dezelfde pariteit. Val 1 is de actieve val (met wit aan zet ruilt '
                      f'wit meteen: {x99}), val 2 de passieve, die alleen met zwart aan zet wint. Wit kan vanuit de passieve '
                      'val het tempo niet wisselen: de winst is niet strategisch maar hangt af van de oppositie.</p>',
                      '<p>Here too the two traps turn into each other with an even number of moves (h3-i4 and h7-i6; the '
                      'black kings shuttle), and they lie at the same parity. Trap 1 is the active trap (with white to move '
                      f'white exchanges at once: {x99}), trap 2 the passive one, which wins with black to move only. White '
                      'cannot change the tempo from the passive trap: the win is not strategic but depends on the '
                      'opposition.</p>'))
        body += self.no_tempo(pg, k99, e99, t('9×9 onspeelbare hoeken', '9×9 unplayable corners'), ['h3', 'h5', 'h7'],
                              ['h1', 'h9'], '1. h3-i4 h9-i8 2. h7-i6 i8-h9',
                              t('de witte opstelling van val 1', 'the white set-up of trap 1'))
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
        s.board55()
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
