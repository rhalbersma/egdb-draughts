#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Shared parts of the companion sites (GitHub Pages): pages with clickable moves and a large board that
follows them, for every position the database value and all moves; any board geometry.

An oracle has analyse(fen) -> (value, [(notation, value for the mover, next fen[, numeric notation])]),
best first; values are Wn / Ln / D in plies.
"""

import html
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEMPLATE = ROOT / 'common' / 'site_template.html'
REPO = 'https://github.com/rhalbersma/egdb-draughts'
SITE = 'https://rhalbersma.github.io/egdb-draughts/'


def esc(s):
    return html.escape(s, quote=False)


def badge(ok, text):
    return f'<span class="verdict {"ok" if ok else "bad" if ok is False else ""}">{text}</span>'


def squares(nota):
    """from and to square of a move in numeric notation (13-10, 13x10, 1x3x5)"""
    sq = [int(x) for x in re.split('[-x]', nota)]
    return [sq[0], sq[-1]]


def geom_6x6():
    """6x6 international board: square 1 = b6, 16 = a1"""
    sq = {}
    for n in range(1, 19):
        r = (n - 1) // 3
        c = 2 * ((n - 1) % 3) + (1 if r % 2 == 0 else 0)
        sq[n] = [c, 5 - r]
    return {'w': 6, 'h': 6, 'sq': sq, 'lake': [], 'pref': 'num'}


class FastEngine:
    """egdb_fast --serve for one board and material (the databases are built at start, in seconds)."""

    def __init__(self, exe, *args):
        self.args = [str(a) for a in args]
        self.p = subprocess.Popen([str(exe), *map(str, args), '--serve', '--no-text'], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True, bufsize=1)
        while True:
            r = self.p.stdout.readline()
            if not r:
                raise RuntimeError(f'egdb_fast {args} died')
            if r.strip() == 'ready':
                break
        self.cache = {}
        self.g = self._geom()

    def cmd(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()
        out = []
        while True:
            r = self.p.stdout.readline()
            if not r:
                raise RuntimeError('egdb_fast died')
            r = r.rstrip('\n')
            if r == 'end':
                return out
            out.append(r)

    def _geom(self):
        out = self.cmd('geom')
        w, h, parity = map(int, out[0].split()[1:])
        sq, lake = {}, []
        for r in out[1:]:
            t = r.split()
            if t[0] == 'sq':
                sq[int(t[1])] = [int(t[2]), int(t[3])]
            elif t[0] == 'lake':
                lake.append([int(t[1]), int(t[2])])
        # the standard notation of the board: numeric on the boards of a game (8x8 checkers, 10x10 international
        # draughts), algebraic on all others
        checkers = 'checkers' in self.args
        standard = (w, h) == ((8, 8) if checkers else (10, 10))
        return {'w': w, 'h': h, 'parity': parity, 'sq': sq, 'lake': lake, 'pref': 'num' if standard else 'alg'}

    def analyse(self, fen):
        if fen not in self.cache:
            out = self.cmd('pos ' + fen)
            if out and out[0].startswith('error'):
                raise ValueError(f'{out[0]}: {fen}')
            val = out[0].split()[1]
            moves = []
            for r in out[1:]:
                _, alg, num, v, nxt = r.split()
                moves.append((alg, v, nxt, num))
            self.cache[fen] = (val, moves)
        return self.cache[fen]

    def fen(self, white, black, stm='W'):
        """FEN from algebraic squares (kings only)"""
        num = {f'{chr(97 + f)}{r + 1}': n for n, (f, r) in self.g['sq'].items()}
        return f"{stm}:W{','.join('K' + str(num[s]) for s in white)}:B{','.join('K' + str(num[s]) for s in black)}"


class Page:
    def __init__(self):
        self.pos = []          # {'f', 'last', 'prev', 'next', 'o': oracle, 'g': geometry id}
        self.index = {}
        self.geoms = {}
        self.oracles = {}
        self.seen = set()

    def use(self, gid, oracle, geom):
        self.geoms[gid] = geom
        self.oracles[gid] = oracle

    def add(self, fen, last=None, prev=None, gid=None):
        gid = gid if gid is not None else (self.pos[prev]['g'] if prev is not None else next(iter(self.geoms)))
        i = len(self.pos)
        self.pos.append({'f': fen, 'last': last, 'prev': prev, 'next': None, 'g': gid})
        if prev is not None and self.pos[prev]['next'] is None:
            self.pos[prev]['next'] = i
        self.index.setdefault((gid, fen), i)
        return i

    def analyse(self, i_or_gid, fen=None):
        gid = self.pos[i_or_gid]['g'] if fen is None else i_or_gid
        return self.oracles[gid].analyse(self.pos[i_or_gid]['f'] if fen is None else fen)

    def notations(self, gid, mv):
        """(numeric, algebraic) notation of a move, with × for captures"""
        if len(mv) > 3:
            num, alg = mv[3], mv[0]
        else:
            sq = self.geoms[gid]['sq']
            num = mv[0]
            alg = re.sub(r'\d+', lambda m: f'{chr(97 + sq[int(m.group())][0])}{sq[int(m.group())][1] + 1}', num)
        return num.replace('x', '×'), alg.replace('x', '×')

    def label(self, gid, mv):
        """the notation of a move as an element that the notation switch can change"""
        num, alg = self.notations(gid, mv)
        pref = self.geoms[gid].get('pref', 'num')
        return f'<span class="mv" data-g="{gid}" data-n="{num}" data-a="{alg}">{num if pref == "num" else alg}</span>'

    # ---- moves in running text: 13-10, 11×6, 2×9 (numeric) or c3-d4, j8×h10×f8 (algebraic) ----------------

    MOVE_RE = re.compile(r'(?<![\w.,/])(\d{1,3}(?:[-x×]\d{1,3})+|[a-z]\d{1,2}(?:[-x×][a-z]\d{1,2})+)(?![\w/]|\.\d)')

    # single squares: algebraic names (h10) in algebraic texts; in numeric texts numbers after "op", "veld(en)",
    # "on", "square(s)" (dam op 9, schijven op 14 en 18)
    SQ_ALG_RE = re.compile(r'(?<![\w.,/×-])([a-z]\d{1,2})(?![\w/×-]|\.\d)')
    SQ_NUM_RE = re.compile(r'\b(op|veld|velden|on|square|squares) (\d{1,2}(?:(?:, | en | and )\d{1,2})*)(?![\d×x-]|\.\d)'
                           r'(?!\s*(?:ply|plies|stuk|piece|zet|move|dam|schijf|king|man\b|men\b|keer|time|tegen|against))')

    def _square(self, n_or_name, gid):
        sq = self.geoms[gid]['sq']
        name = {int(n): f'{chr(97 + f)}{r + 1}' for n, (f, r) in sq.items()}
        num_of = {v: k for k, v in name.items()}
        if isinstance(n_or_name, int):
            if n_or_name not in name:
                return None
            n, a = n_or_name, name[n_or_name]
        else:
            if n_or_name not in num_of or n_or_name in ('p0', 'p1'):      # p0 / p1: the colourings, not squares
                return None
            n, a = num_of[n_or_name], n_or_name
        pref = self.geoms[gid].get('pref', 'num')
        return f'<span class="mv" data-g="{gid}" data-n="{n}" data-a="{a}">{n if pref == "num" else a}</span>'

    def _squares(self, text, gid, authored):
        if authored == 'alg':
            return self.SQ_ALG_RE.sub(lambda m: self._square(m.group(1), gid) or m.group(0), text)

        def sub(m):
            parts = re.split(r'(, | en | and )', m.group(2))
            conv = [p if i % 2 else (self._square(int(p), gid) or p) for i, p in enumerate(parts)]
            return f'{m.group(1)} ' + ''.join(conv)
        return self.SQ_NUM_RE.sub(sub, text)

    def _token(self, tok, gid):
        """a move in running text -> notation element, or None if it is not a move on this board"""
        sq = self.geoms[gid]['sq']
        name = {int(n): f'{chr(97 + f)}{r + 1}' for n, (f, r) in sq.items()}
        num_of = {v: k for k, v in name.items()}
        parts = re.split(r'[-x×]', tok)
        seps = re.findall(r'[-x×]', tok)
        if tok[0].isdigit():
            nums = [int(x) for x in parts]
            if any(n not in name for n in nums) or len(set(nums)) < 2:   # 6×6, 10×10: board sizes, not moves
                return None
            algs = [name[n] for n in nums]
        else:
            if any(x not in num_of for x in parts):
                return None
            algs, nums = parts, [num_of[x] for x in parts]
        join = lambda xs: ''.join(str(x) + (seps[i].replace('x', '×') if i < len(seps) else '') for i, x in enumerate(xs))
        num, alg = join(nums), join(algs)
        pref = self.geoms[gid].get('pref', 'num')
        return f'<span class="mv" data-g="{gid}" data-n="{num}" data-a="{alg}">{num if pref == "num" else alg}</span>'

    def _convert(self, text, gid, authored='num'):
        """authored: the notation the text is written in; only moves in that notation are converted (so that
        material like 3-1 in a text with algebraic moves stays as it is)"""
        out, pos = [], 0
        for m in self.MOVE_RE.finditer(text):
            out.append(self._squares(text[pos:m.start()], gid, authored))
            tok = m.group(1)
            out.append((self._token(tok, gid) if tok[0].isdigit() == (authored == 'num') else None) or m.group(0))
            pos = m.end()
        out.append(self._squares(text[pos:], gid, authored))
        return ''.join(out)

    def prose(self, text, gid, authored='num'):
        """plain text -> HTML in which the moves follow the notation switch"""
        return self._convert(esc(text), gid, authored)

    def prose_html(self, html_, gid, authored='num'):
        """the same for an HTML fragment: only text outside tags, not inside notation elements, code or pre"""
        out, skip = [], 0
        for part in re.split(r'(<[^>]+>)', html_):
            if part.startswith('<'):
                if re.match(r'<(span class="mv"|code|pre)[\s>]', part):
                    skip += 1
                elif skip and re.match(r'</(span|code|pre)>', part):
                    skip -= 1
                out.append(part)
            else:
                out.append(part if skip else self._convert(part, gid, authored))
        return ''.join(out)

    def child(self, prev, mv):
        return self.add(mv[2], squares(mv[3] if len(mv) > 3 else mv[0]), prev)

    def diagram(self, fen, caption, gid=None):
        i = self.add(fen, gid=gid)
        g = self.pos[i]['g']
        return (f'<figure><a class="diagram" href="#" data-i="{i}"><div class="smallboard" data-g="{g}" '
                f'data-fen="{fen}"></div></a><figcaption>{caption}</figcaption></figure>'), i

    def moves(self, fen, plan, number, black_first, cls='m1', start=None, extend=0, nbook=None, gid=None,
              asides=None, auto_aside=True):
        """plan: [(notations, marks)] from fen as links, then at most extend best moves (to the end of the
        game); moves from index nbook on are the database's. A move of the plan that changes the result gets a
        red "?" and an aside in brackets, as in the analyses on 10x10.org: a "≡" link back to the position
        before the move, then the better line. asides: {plan index: f(prev, fen, number, black) -> html} for a
        full refutation; otherwise the best move with its value (once per page for the same position and move).
        Returns (html, last index, end fen, plies)."""
        nbook = len(plan) if nbook is None else nbook
        asides = asides or {}
        prev = self.add(fen, gid=gid) if start is None else start
        g = self.pos[prev]['g']
        out, k, n = [], (1 if black_first else 0), 0
        while n < len(plan) + extend:
            v, legal = self.analyse(g, fen)
            if not legal:
                break
            if n < len(plan):
                cand = [m for m in legal if m[0] in plan[n][0] or (len(m) > 3 and m[3] in plan[n][0])]
                if not cand:
                    break
                mv, marks = cand[0], plan[n][1]
            else:
                mv, marks = legal[0], ''
            best = legal[0][1][0]
            keeping = [m for m in legal if m[1][0] == best]
            err = mv[1][0] != best and best != '?'
            flags = []
            if not err and len(keeping) == 1 and best in 'WD':
                flags.append('only')
            if n >= nbook:
                flags.append('db')
            i = self.child(prev, mv)
            mnum, black = number + k // 2, k % 2 == 1
            num = f'{mnum}.' if not black else (f'{mnum}. …' if n == 0 else '')
            nota = self.label(g, mv) + marks
            if err and '?' not in marks:      # a "!" of the book that the database contradicts goes in parentheses
                nota = self.label(g, mv) + (f'({marks})' if marks else '') + '<span class="q">?</span>'
            out.append(f'<a class="{cls} {" ".join(flags)}" href="#" data-i="{i}" title="{mv[1]}">'
                       f'{num + " " if num else ""}{nota}</a>')
            if err and n < nbook:
                if n in asides:
                    self.seen.add((g, fen, mv[0]))
                    out.append(self.aside(prev, asides[n](prev, fen, mnum, black)))
                elif auto_aside and (g, fen, mv[0]) not in self.seen:
                    self.seen.add((g, fen, mv[0]))
                    h, *_ = self.moves(fen, [((legal[0][0],), '!')], mnum, black, 'm3', prev, gid=g, auto_aside=False)
                    out.append(self.aside(prev, f'{h} <span class="val">{legal[0][1]}</span>'))
            prev, fen, k, n = i, mv[2], k + 1, n + 1
        if out and not self.analyse(g, fen)[1]:
            out[-1] = out[-1].replace('</a>', '+</a>')
        return ' '.join(out), prev, fen, n

    @staticmethod
    def aside(prev, html_):
        return f'<span class="aside">[<a class="m3 eq" href="#" data-i="{prev}" title="terug naar de stelling">≡</a> {html_}]</span>'

    def best_line(self, fen, plies, cls='m1', start=None, gid=None, number=1, stop_draw=12):
        """the database's main line from fen (fastest win, longest defence), at most plies long"""
        g = gid if gid is not None else (self.pos[start]['g'] if start is not None else next(iter(self.geoms)))
        v, _ = self.analyse(g, fen)
        return self.moves(fen, [], number, fen[0] == 'B', cls, start, extend=stop_draw if v == 'D' else plies,
                          nbook=0, gid=gid)

    def data(self):
        rows = []
        for p in self.pos:
            v, legal = self.oracles[p['g']].analyse(p['f'])
            rows.append({'f': p['f'], 'v': v, 'last': p['last'], 'prev': p['prev'], 'next': p['next'], 'g': p['g'],
                         'm': [[*self.notations(p['g'], m), m[1], self.index.get((p['g'], m[2]))] for m in legal]})
        return json.dumps(rows, ensure_ascii=False, separators=(',', ':'))


UI = {
    'nl': {'back': 'Vorige stelling (←)', 'play': 'Variant automatisch afspelen', 'fwd': 'Volgende stelling (→)',
           'nums': 'Veldnamen op het bord aan/uit', 'nota': 'Wissel tussen numerieke (1, 2, …) en algebraïsche (a1, b2, …) notatie',
           'js': {'white': 'wit', 'black': 'zwart', 'tomove': 'aan zet', 'nomoves': 'geen zetten: verloren'},
           'legend': 'Klik op een zet om de stelling op het grote bord te zien; ◀ ▶ (of de pijltjestoetsen) lopen door '
                     'de variant. Waarden gelden voor de partij aan zet; bij een zet voor wie hem doet, met n het aantal '
                     'ply tot het einde van de partij, de zet meegeteld: W = winst (oneven), L = verlies (even), '
                     'D = remise. <span style="color:var(--link);font-weight:600">Blauw</span> = hoofdvariant, '
                     '<span style="color:var(--m2)">groen</span> = zijvariant, <em>cursief</em> = voortzetting van de '
                     'database, <span class="q">?</span> = zet die volgens de database het resultaat verandert, met '
                     'tussen [ ] de betere zet of de weerlegging (<span style="color:var(--db)">≡</span> = terug naar de '
                     'stelling vóór die zet), <span style="color:var(--only)">*</span> = enige zet die de winst of de '
                     'remise behoudt. De knop # zet de veldnamen op het grote bord aan of uit; de knop met het bereik van de veldnamen (bv. 1…18 of '
                     'a1…f6 op 6×6) wisselt tussen numerieke en algebraïsche notatie, ook in de tekst.'},
    'en': {'back': 'Previous position (←)', 'play': 'Play the line automatically', 'fwd': 'Next position (→)',
           'nums': 'Square names on the board on/off', 'nota': 'Switch between numeric (1, 2, …) and algebraic (a1, b2, …) notation',
           'js': {'white': 'white', 'black': 'black', 'tomove': 'to move', 'nomoves': 'no moves: lost'},
           'legend': 'Click a move to see the position on the large board; ◀ ▶ (or the arrow keys) step through the '
                     'line. Values are for the side to move; for a move, for the side making it, with n the number of '
                     'plies to the end of the game, the move included: W = win (odd), L = loss (even), D = draw. '
                     '<span style="color:var(--link);font-weight:600">Blue</span> = main line, '
                     '<span style="color:var(--m2)">green</span> = side line, <em>italics</em> = continuation by the '
                     'database, <span class="q">?</span> = a move that changes the result according to the database, '
                     'with the better move or the refutation in [ ] (<span style="color:var(--db)">≡</span> = back to '
                     'the position before that move), <span style="color:var(--only)">*</span> = the only move that '
                     'keeps the win or the draw. The # button turns the square names on the large board on or off; '
                     'the button with the range of the square names (e.g. 1…18 or a1…f6 on 6×6) switches between numeric and algebraic notation, in the text as well.'},
}


FLAG_NL = ('<svg viewBox="0 0 9 6" aria-hidden="true"><rect width="9" height="6" fill="#21468B"/>'
           '<rect width="9" height="4" fill="#fff"/><rect width="9" height="2" fill="#AE1C28"/></svg>')
FLAG_UK = ('<svg viewBox="0 0 60 30" aria-hidden="true"><clipPath id="uk-t"><path d="M30,15h30v15zv15h-30zh-30v-15zv-15h30z"/>'
           '</clipPath><path d="M0,0v30h60v-30z" fill="#012169"/><path d="M0,0L60,30M60,0L0,30" stroke="#fff" '
           'stroke-width="6"/><path d="M0,0L60,30M60,0L0,30" clip-path="url(#uk-t)" stroke="#C8102E" stroke-width="4"/>'
           '<path d="M30,0v30M0,15h60" stroke="#fff" stroke-width="10"/><path d="M30,0v30M0,15h60" stroke="#C8102E" '
           'stroke-width="6"/></svg>')


def lang_link(lang, alt):
    """the language switch: two flags, the current language marked, the other one a link to the same page"""
    flags = []
    for code, name, flag in (('nl', 'Nederlands', FLAG_NL), ('en', 'English', FLAG_UK)):
        if code == lang:
            flags.append(f'<span class="flag cur" title="{name}" aria-current="true">{flag}</span>')
        else:
            flags.append(f'<a class="flag" href="{alt}" hreflang="{code}" lang="{code}" title="{name}" '
                         f'aria-label="{name}">{flag}</a>')
    return f'<span class="langs">{"".join(flags)}</span>'


def write_page(path, title, nav, body, page, site_title, lang='nl'):
    tpl = TEMPLATE.read_text()
    u = UI[lang]
    tpl = (tpl.replace('{{LANG}}', lang).replace('{{T_BACK}}', u['back']).replace('{{T_PLAY}}', u['play'])
           .replace('{{T_FWD}}', u['fwd']).replace('{{T_NUMS}}', u['nums']).replace('{{T_NOTA}}', u['nota'])
           .replace('{{LEGEND}}', u['legend']).replace('{{UI}}', json.dumps(u['js'], ensure_ascii=False)))
    geoms = json.dumps(page.geoms, separators=(',', ':'))
    out = (tpl.replace('{{TITLE}}', esc(title)).replace('{{SITE}}', esc(site_title)).replace('{{NAV}}', nav)
           .replace('{{BODY}}', body).replace('{{GEOMS}}', geoms).replace('{{DATA}}', page.data()))
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text(out)


def md(text, base='', conv=None):
    """the small part of Markdown used in the reports -> HTML; relative links point into the repository; conv
    (optional) post-processes the HTML of every paragraph, e.g. Page.prose_html for the moves"""
    def link(m):
        url = m.group(2)
        if not re.match(r'https?:|#', url):
            url = f'{REPO}/blob/main/{base}{url}'
        return f'<a href="{url}">{m.group(1)}</a>'

    def inline(s):
        s = esc(s)
        s = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link, s)
        s = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', s)
        s = re.sub(r'\*(.+?)\*', r'<em>\1</em>', s)
        s = re.sub(r'`(.+?)`', r'<code>\1</code>', s)
        return conv(s) if conv else s
    out, para, items = [], [], []

    def flush():
        if para:
            out.append('<p>' + inline(' '.join(para)) + '</p>')
            para.clear()
        if items:
            out.append('<ul>' + ''.join(f'<li>{inline(x)}</li>' for x in items) + '</ul>')
            items.clear()
    for ln in text.split('\n'):
        if not ln.strip():
            flush()
        elif ln.startswith('#'):
            flush()
            lvl = len(ln) - len(ln.lstrip('#'))
            out.append(f'<h{min(lvl, 3)}>{inline(ln.lstrip("#").strip())}</h{min(lvl, 3)}>')
        elif ln.startswith('* '):
            if para:
                flush()
            items.append(ln[2:].strip())
        elif ln.startswith('  ') and items:
            items[-1] += ' ' + ln.strip()
        else:
            para.append(ln.strip())
    flush()
    return '\n'.join(out)


def menu(groups, current=None, label='Inhoud'):
    """an expandable menu for the header: groups = [(heading or None, [(href, text)])]"""
    out = [f'<details class="menu"><summary>{esc(label)}</summary><nav class="menu-panel">']
    for heading, items in groups:
        if heading:
            out.append(f'<div class="mh">{esc(heading)}</div>')
        for href, text in items:
            cur = ' aria-current="page"' if href == current else ''
            out.append(f'<a href="{href}"{cur}>{esc(text)}</a>')
    out.append('</nav></details>')
    return ''.join(out)
