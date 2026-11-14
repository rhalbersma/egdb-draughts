#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Solve the openings of 6x6 international draughts through the complete opening DAG.

The DAG is expanded from the start position for as long as positions have more than `built_max`
pieces and kings on at most one side (such positions cannot repeat, so every position belongs to one
ply and the DAG is finite). Its leaves are:
  * positions with at most `built_max` pieces: exact win / draw / loss from the databases;
  * positions without moves: lost;
  * positions with more pieces and kings on both sides: unknown, the interval [loss, win].
Values are backed up as intervals [lo, hi] (-1 loss, 0 draw, 1 win, for the side to move), like the
"<= Draw" results of the checkers proof (Schaeffer et al., Science 2007). Where the root or an opening
is not decided, the unknown leaves that matter are listed.

usage: opening_solve_6x6.py path/to/egdb_6x6 [--db data/6x6_le10.db] [--table 3] [--out opening_6x6]
                            [--dtm-db data/6x6_le9.db]
       opening_solve_6x6.py path/to/egdb_6x6 --db data/6x6_le9.db --attach data/6x6_10.dtm --dtm [--table 4]
"""

import argparse
import json
import re
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path

START = 'W:W13,14,15,16,17,18:B1,2,3,4,5,6'
VAL = {'L': -1, 'D': 0, 'W': 1}


class Engine:
    def __init__(self, exe, db, attach=None):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        out = self.cmd(f'load {db}')
        if not out[0].startswith('load ok'):
            raise RuntimeError(out)
        if attach:
            out = self.cmd(f'attach {attach}')
            if not out[0].startswith('attach ok'):
                raise RuntimeError(out)
        self.built = int(out[0].split()[-1])

    def cmd(self, line):
        self.p.stdin.write(line + '\n')
        self.p.stdin.flush()
        out = []
        while (r := self.p.stdout.readline().rstrip('\n')) != 'end':
            out.append(r)
        return out

    def moves(self, fen):
        return [tuple(r.split()[1:3]) for r in self.cmd('moves ' + fen)]

    def wdl(self, fen):
        return VAL[self.cmd('wdl ' + fen)[0].split()[1]]


def info(fen):
    _, w, b = fen.split(':')
    W = [x for x in w[1:].split(',') if x]
    B = [x for x in b[1:].split(',') if x]
    return len(W) + len(B), any(x[0] == 'K' for x in W), any(x[0] == 'K' for x in B)


def label(lo, hi, stm, dtm=None, move=False):
    """D, Wn or Ln (or >=D, <=D, ? when not decided). For a position (the start position) the value is for
    the side to move and n is its distance to mate. For a move (move=True; the node is the position after
    the move) the value is for the side that made the move and n counts from the position before the
    move, the move included. Either way a win has an odd n and a loss an even n."""
    if move:
        lo, hi = -hi, -lo
    if lo != hi:
        return {(0, 1): '≥D', (-1, 0): '≤D', (-1, 1): '?'}[(lo, hi)]
    return {1: 'W', 0: 'D', -1: 'L'}[lo] + (str(dtm + (1 if move else 0)) if dtm is not None and lo != 0 else '')


# mate-distance scores for the side to move: win in n = WSC - n, loss in n = -(WSC - n), draw = 0
WSC = 30000


def to_score(v):
    return 0 if v == 'D' else (WSC - int(v[1:]) if v[0] == 'W' else -(WSC - int(v[1:])))


def up(s):
    """score of a position -> score of the move into it, for the side that made the move (one ply more)"""
    return 1 - s if s > 0 else -s - 1 if s < 0 else 0


def score_label(lo, hi, move=False):
    """Score interval for the side to move (of a position, or of the position after a move) -> D, Wn, Ln,
    or a bound: >=Wn (a win in at most n plies), <=Ln (a loss in at most n plies), >=D, <=D, ?. For a move
    the value is for the side that made it and n counts from the position before the move, the move included."""
    if move:
        lo, hi = up(hi), up(lo)
    if lo == hi:
        return 'D' if lo == 0 else f'W{WSC - lo}' if lo > 0 else f'L{WSC + lo}'
    if lo > 0:
        return f'≥W{WSC - lo}'
    if hi < 0:
        return f'≤L{WSC + hi}'
    if lo == 0:
        return '≥D'
    if hi == 0:
        return '≤D'
    return '?'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--db', default='data/6x6_le10.db')
    ap.add_argument('--table', type=int, default=3)
    ap.add_argument('--tree', type=int, default=None, help='depth of the stored tree with distances (default: --table)')
    ap.add_argument('--out', default=str(Path(__file__).resolve().parent / 'opening_6x6'))
    ap.add_argument('--attach', default=None, help='layer to attach after loading --db (e.g. data/6x6_10.dtm)')
    ap.add_argument('--dtm', action='store_true', help='the databases have exact DTM: compute the distances with the same engine')
    ap.add_argument('--dtm-db', default=None, help='database with exact DTM (e.g. data/6x6_le9.db) for the distances')
    args = ap.parse_args()
    t0 = time.time()
    log = lambda m: print(f'[{time.time() - t0:7.1f}s] {m}', file=sys.stderr, flush=True)
    eng = Engine(args.engine, args.db, args.attach)
    log(f'database loaded, built_max {eng.built}')

    # the complete opening DAG
    children = {}              # interior node -> [(move, child)]
    leaf = {}                  # leaf -> (lo, hi, kind)
    layers = [[START]]
    while layers[-1]:
        nxt = set()
        for fen in layers[-1]:
            ms = eng.moves(fen)
            if not ms:
                children.pop(fen, None)
                leaf[fen] = (-1, -1, 'no moves')
                continue
            children[fen] = ms
            for _, c in ms:
                if c in leaf or c in children:
                    continue
                n, wk, bk = info(c)
                if n <= eng.built:
                    v = eng.wdl(c)
                    leaf[c] = (v, v, 'database')
                elif wk and bk:
                    leaf[c] = (-1, 1, 'unknown')
                else:
                    nxt.add(c)
        layers.append(sorted(nxt))
        log(f'ply {len(layers) - 1}: {len(nxt)} open, {len(leaf)} leaves')
    kinds = Counter(k for _, _, k in leaf.values())
    log(f'DAG: {len(children)} interior positions, leaves {dict(kinds)}')

    # one-ply bounds for the unknown leaves: captures (and every move to fewer pieces) lead into the
    # databases, so a known winning or drawing move raises the lower bound; the upper bound stays a win
    # as long as some move leads to another unknown position
    sharpened = 0
    for fen, (lo, hi, kind) in list(leaf.items()):
        if kind != 'unknown':
            continue
        cs = []
        for _, c in eng.moves(fen):
            n, wk, bk = info(c)
            if n <= eng.built:
                v = eng.wdl(c)
                cs.append((v, v))
            else:
                cs.append((-1, 1))
        if not cs:
            leaf[fen] = (-1, -1, 'no moves')
            continue
        nlo, nhi = max(-h for _, h in cs), max(-l for l, _ in cs)
        if (nlo, nhi) != (-1, 1):
            sharpened += 1
        leaf[fen] = (nlo, nhi, 'unknown' if nlo < nhi else 'unknown, decided in one ply')
    kinds = Counter(k for _, _, k in leaf.values())
    log(f'one-ply bounds sharpened {sharpened} unknown leaves; leaves {dict(kinds)}')
    n_unknown = sum(v for k, v in kinds.items() if k.startswith('unknown'))
    n_decided1 = kinds.get('unknown, decided in one ply', 0)

    # back up intervals in post-order (the DAG is acyclic, but a position can be reached after different
    # numbers of plies, because men also capture backwards, so the layers are not a topological order)
    iv = {f: (lo, hi) for f, (lo, hi, _) in leaf.items()}
    stack = [(START, False)]
    while stack:
        fen, expanded = stack.pop()
        if fen in iv:
            continue
        if not expanded:
            stack.append((fen, True))
            stack.extend((c, False) for _, c in children[fen] if c not in iv)
            continue
        cs = [iv[c] for _, c in children[fen]]
        iv[fen] = (max(-hi for _, hi in cs), max(-lo for lo, _ in cs))
    root = iv[START]
    log(f'start position: {label(*root, "W")}')

    # unknown leaves that can still change the value of the start position: a leaf matters if it lies
    # on a path along which every move is still possibly best (child interval overlaps the best bound)
    relevant = set()
    stack, seen = [START], {START}
    while stack:
        fen = stack.pop()
        if fen not in children:
            if leaf[fen][0] < leaf[fen][1]:
                relevant.add(fen)
            continue
        lo, hi = iv[fen]
        if lo == hi:
            continue
        for _, c in children[fen]:
            clo, chi = iv[c]
            if -clo >= lo and c not in seen:      # this move can still reach the parent's best value
                seen.add(c)
                stack.append(c)

    # distances: a second engine with exact DTM below its database limit and alpha-beta above it, without
    # solving slices on demand; nodes whose search would need such a slice keep their result without n
    dtm = {}
    tree_nodes = {}

    def collect(fen, depth):
        if fen in tree_nodes and tree_nodes[fen] <= depth:
            return
        tree_nodes[fen] = depth
        if depth < (args.tree or args.table) and fen in children:
            for _, c in children[fen]:
                collect(c, depth + 1)
    collect(START, 0)
    sc = None
    if args.dtm:
        # exact distances (or bounds) through the DAG: database leaves exact, unknown leaves bounded by
        # their own moves (captures lead into the databases), backed up in post-order like the WDL values
        full = (-WSC, WSC - 1)
        sc = {}
        for fen, (lo, hi, kind) in leaf.items():
            if kind == 'database':
                v = to_score(eng.cmd('dtm ' + fen)[0].split()[1])
                sc[fen] = (v, v)
            elif kind == 'no moves':
                sc[fen] = (-WSC, -WSC)
            else:
                cs = []
                for _, c in eng.moves(fen):
                    n, wk, bk = info(c)
                    if n <= eng.built:
                        v = to_score(eng.cmd('dtm ' + c)[0].split()[1])
                        cs.append((v, v))
                    else:
                        cs.append(full)
                sc[fen] = (max(up(h) for _, h in cs), max(up(l) for l, _ in cs))
        log(f'distances of {len(sc)} leaves')
        stack = [(START, False)]
        while stack:
            fen, expanded = stack.pop()
            if fen in sc:
                continue
            if not expanded:
                stack.append((fen, True))
                stack.extend((c, False) for _, c in children[fen] if c not in sc)
                continue
            cs = [sc[c] for _, c in children[fen]]
            sc[fen] = (max(up(h) for _, h in cs), max(up(l) for l, _ in cs))
        for f, (lo, hi) in iv.items():          # consistency with the win/draw/loss values
            a, b = sc[f]
            if (lo == hi) and not ((lo == 1 and a > 0) or (lo == 0 and a == b == 0) or (lo == -1 and b < 0)):
                raise RuntimeError(f'distance and WDL disagree for {f}: {sc[f]} vs {iv[f]}')
        log(f'start position: {score_label(*sc[START])}')
    if args.dtm_db:
        # older route: distances by exact-DTM search in a second engine (no on-demand slices)
        eng.p.stdin.close()
        eng.p.wait()
        deng = Engine(args.engine, args.dtm_db)
        deng.cmd('ondemand 0')
        log(f'DTM database loaded, built_max {deng.built}')
        for fen in sorted(tree_nodes, key=lambda f: -tree_nodes[f]):
            lo, hi = iv[fen]
            if lo != hi or lo == 0:
                continue
            t = time.time()
            out = deng.cmd('dtm ' + fen)[0]
            if out.startswith('val '):
                v = out.split()[1]
                if (v[0] == 'W') != (lo == 1):
                    raise RuntimeError(f'DTM and WDL disagree for {fen}: {v} vs {lo}')
                dtm[fen] = int(v[1:])
            log(f'dtm ply {tree_nodes[fen]} {fen}: {out} ({time.time() - t:.1f} s)')

    def lab(fen, move):
        if sc is not None:
            return score_label(*sc[fen], move)
        return label(*iv[fen], fen[0], dtm.get(fen), move)

    rows = []

    def walk(fen, path):
        if len(path) == args.table or fen not in children:
            rows.append((path, fen))
            return
        for nota, c in children[fen]:
            walk(c, path + [nota])
    walk(START, [])
    tree = []

    def numeric(nota):
        return tuple(int(x) for x in re.split('[-x]', nota))

    def best_first(moves):
        """Siblings ordered by their value for the side that makes the move: wins (fastest first), draws,
        undecided, losses (slowest first); unknown distances last within their class; then by move."""
        def key(m):
            nota, c = m
            if sc is not None:                  # the mover's score, best first
                a, b = up(sc[c][1]), up(sc[c][0])
                return (-a, -b) + numeric(nota)
            lo, hi = iv[c]
            n = dtm.get(c)
            if lo != hi:
                rank = (2, 0)
            elif lo == -1:                      # the side to move after the move loses: a win for the mover
                rank = (0, n if n is not None else 10**6)
            elif lo == 0:
                rank = (1, 0)
            else:
                rank = (3, -n if n is not None else 10**6)
            return rank + numeric(nota)
        return sorted(moves, key=key)

    def walk_tree(fen, path):
        name = 'beginstand' if not path else ('wit ' if len(path) % 2 else 'zwart ') + path[-1]
        tree.append('    ' * len(path) + f'{name}  {lab(fen, bool(path))}')
        if len(path) < args.table and fen in children:
            for nota, c in best_first(children[fen]):
                walk_tree(c, path + [nota])
    walk_tree(START, [])
    rows.sort(key=lambda r: tuple(numeric(m) for m in r[0]))
    res = Counter(label(*iv[f], f[0], None, True)[:1] for _, f in rows)
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from board import diagram
    diagrams = ['## Diagrammen', '', 'w/b = witte/zwarte schijf, W/B = dam.', '',
                f'Beginstand, wit aan zet: **{lab(START, False)}**', '', '```text', diagram(START), '```', '']
    for nota, c in best_first(children[START]):
        diagrams += [f'1. {nota} (zwart aan zet): **{lab(c, True)}**', '', '```text', diagram(c), '```', '']
    md = ['# Openingen van 6×6-dammen', '',
          f'Beginstand, wit aan zet: **{lab(START, False)}**.', '',
          'Notatie: D = remise, Wn = winst, Ln = verlies, met n het aantal ply tot het einde van de partij bij perfect '
          'spel. Bij de beginstand geldt de waarde voor wit (aan zet). Bij een zet geldt de waarde voor wie de zet doet, '
          'en telt n vanaf de stelling vóór de zet, de zet meegeteld. Een winst heeft dus altijd een oneven n, een '
          'verlies een even n. In de tabel geldt het label voor de laatste (witte) zet van de opening. Waar de exacte '
          'afstand niet vastligt (doordat een variant via een stelling met 11 of 12 stukken en dammen aan beide kanten '
          'loopt, waarvoor geen database bestaat), staat een grens: ≥Wn = wint uiterlijk na n ply, ≤Ln = verliest '
          'uiterlijk na n ply.', '',
          'Methode: de volledige openings-DAG (transposities samengevoegd), uitgebreid zolang een stelling meer dan '
          f'{eng.built} stukken heeft en hoogstens één kant dammen. Bladeren: databasestellingen (exact), stellingen '
          'zonder zetten (verloren) en stellingen met meer stukken en dammen aan beide kanten (onbekend). Waarden zijn '
          'als intervallen teruggerekend, vanuit wit gezien.', '',
          f'DAG: {len(children)} inwendige stellingen, bladeren: ' + ', '.join(f'{k} {v}' for k, v in kinds.items()) + '.',
          f'Onbekende bladeren die de beginstand nog kunnen beïnvloeden: {len(relevant)}.', '',
          '### Stellingen met 11 of 12 stukken en dammen aan beide kanten', '',
          f'Daarvoor bestaat geen database. In de DAG komen er {n_unknown} verschillende voor (vanaf ply 11, de '
          'meeste tussen ply 15 en 30, bijna altijd met één dam per kant); in de getoonde boom tot ply '
          f'{args.tree or args.table} zitten ze niet. Ze worden niet weggelaten en niet door een zoekalgoritme '
          'weggesnoeid: elk zo\'n blad krijgt het ruimst mogelijke interval (van verlies tot winst, met elke '
          f'afstand), verfijnd met zijn eigen zetten (een slag gaat de database in; zo liggen er {n_decided1} toch '
          'vast). Minimax wordt daarna over de hele DAG zonder snoeien met onder- en bovengrenzen teruggerekend. '
          'Waar die samenvallen is de waarde bewezen, ongeacht de werkelijke waarde van de onbekende bladeren: '
          'er staat dan een zet naast die al minstens zo goed is. Dat geldt voor de beginstand en voor alle '
          'getoonde stellingen, inclusief hun afstanden. (Een alpha-beta-zoektocht naar de exacte afstand liep '
          'juist wél op deze stellingen vast; de intervalrekening maakt dat overbodig.)', '',
          *diagrams, '## Boom', '', 'Per stelling staan de zetten op volgorde van waarde voor wie ze doet: winst (snelste '
          'eerst), remise, verlies (taaiste eerst); daarbinnen numeriek.', '', '```text', *tree, '```', '',
          f'## Alle openingen van {args.table} ply', '', 'Numeriek gesorteerd.', '',
          f"{len(rows)} openingen: W {res['W']}, D {res['D']}, L {res['L']}.", '',
          '| # | opening | resultaat |', '|---|---|---|']
    md += [f"| {i} | {' '.join(p)} | {lab(f, bool(p))} |" for i, (p, f) in enumerate(rows, 1)]
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out + '.md').write_text('\n'.join(md) + '\n')
    Path(args.out + '.json').write_text(json.dumps({
        'start': START, 'root': root, 'leaf_kinds': kinds, 'relevant_unknown': sorted(relevant),
        'openings': [{'moves': p, 'fen': f, 'interval': iv[f], 'dtm': dtm.get(f)} for p, f in rows],
        'tree': {f: {'ply': d, 'interval': iv[f], 'dtm': dtm.get(f), 'score': sc[f] if sc else None}
                 for f, d in tree_nodes.items()}}, indent=0))
    log('done')


if __name__ == '__main__':
    main()
