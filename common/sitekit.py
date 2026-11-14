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
    return {'w': 6, 'h': 6, 'sq': sq, 'lake': [], 'alg': False}


class FastEngine:
    """egdb_fast --serve for one board and material (the databases are built at start, in seconds)."""

    def __init__(self, exe, *args):
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
        return {'w': w, 'h': h, 'parity': parity, 'sq': sq, 'lake': lake, 'alg': True}

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

    def child(self, prev, mv):
        return self.add(mv[2], squares(mv[3] if len(mv) > 3 else mv[0]), prev)

    def diagram(self, fen, caption, gid=None):
        i = self.add(fen, gid=gid)
        g = self.pos[i]['g']
        return (f'<figure><a class="diagram" href="#" data-i="{i}"><div class="smallboard" data-g="{g}" '
                f'data-fen="{fen}"></div></a><figcaption>{caption}</figcaption></figure>'), i

    def moves(self, fen, plan, number, black_first, cls='m1', start=None, extend=0, nbook=None, gid=None):
        """plan: [(notations, marks)] from fen as links, then at most extend best moves (to the end of the
        game); moves from index nbook on are the database's. Returns (html, last index, end fen, plies)."""
        nbook = len(plan) if nbook is None else nbook
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
            flags = []
            if mv[1][0] != best and best != '?':
                flags.append('err')
            elif len(keeping) == 1 and best in 'WD':
                flags.append('only')
            if n >= nbook:
                flags.append('db')
            i = self.child(prev, mv)
            num = f'{number + k // 2}.' if k % 2 == 0 else (f'{number + k // 2}. …' if n == 0 else '')
            nota = mv[0].replace('x', '×') + marks
            out.append(f'<a class="{cls} {" ".join(flags)}" href="#" data-i="{i}" title="{mv[1]}">'
                       f'{num + " " if num else ""}{nota}</a>')
            prev, fen, k, n = i, mv[2], k + 1, n + 1
        if out and not self.analyse(g, fen)[1]:
            out[-1] = out[-1].replace('</a>', '+</a>')
        return ' '.join(out), prev, fen, n

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
                         'm': [[m[0].replace('x', '×'), m[1], self.index.get((p['g'], m[2]))] for m in legal]})
        return json.dumps(rows, ensure_ascii=False, separators=(',', ':'))


def write_page(path, title, nav, body, page, site_title):
    tpl = TEMPLATE.read_text()
    geoms = json.dumps(page.geoms, separators=(',', ':'))
    out = (tpl.replace('{{TITLE}}', esc(title)).replace('{{SITE}}', esc(site_title)).replace('{{NAV}}', nav)
           .replace('{{BODY}}', body).replace('{{GEOMS}}', geoms).replace('{{DATA}}', page.data()))
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text(out)


def md(text, base=''):
    """the small part of Markdown used in the reports -> HTML; relative links point into the repository"""
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
        return s
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
