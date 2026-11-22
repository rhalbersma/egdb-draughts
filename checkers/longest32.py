#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The longest win on every board of the 3-2 table (heights 3 to 10, widths up to height + 9), with the analyses along its
main line, stored in longest32.json.gz for the site (building the 3-2 databases of all boards takes about an hour).
Also stored per board: the quiet positions counted by their value with white and with black to move, as the colours of
the table use them. A position is quiet when nobody can capture, white to move has a move that loses no material, and
black to move cannot create a fork (a move after which white has no such move). Exact on small boards, a random sample
of 3000 quiet positions on the others. Resumable: boards already in the file are skipped.

usage: longest32.py [path/to/egdb_fast]
"""

import gzip
import json
import math
import random
import sys
from itertools import combinations
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'common'))
from sitekit import CachedEngine, FastEngine   # noqa: E402

EXE = sys.argv[1] if len(sys.argv) > 1 else str(HERE.parent / 'build' / 'egdb_fast')
OUT = HERE / 'longest32.json.gz'
HEIGHTS, EXTRA = range(3, 11), range(10)       # a row per height H, a column per W - H
SAMPLE, EXACT = 3000, 150000


def quiet_counts(e, rng):
    """the quiet positions by (value with white to move, value with black to move), e.g. {'WL': 2950, 'WD': 50}"""
    sq = {n: tuple(fr) for n, fr in e.g['sq'].items()}
    at = {fr: n for n, fr in sq.items()}
    nb = {n: [at[(f + df, r + dr)] for df in (-1, 1) for dr in (-1, 1) if (f + df, r + dr) in at] for n, (f, r) in sq.items()}
    name = {n: f'{chr(97 + f)}{r + 1}' for n, (f, r) in sq.items()}

    def jumps(att, victims, occ):                  # can the king on att capture one of victims?
        f, r = sq[att]
        for v in nb[att]:
            if v in victims:
                to = at.get((2 * sq[v][0] - f, 2 * sq[v][1] - r))
                if to is not None and to not in occ:
                    return True
        return False

    def captures(us, them):
        occ = set(us) | set(them)
        return any(jumps(x, set(them), occ) for x in us)

    def safe(wk, bk):                              # white to move can capture or make a move after which black cannot
        if captures(wk, bk):
            return True
        occ = set(wk) | set(bk)
        return any(not captures(bk, (set(wk) - {x}) | {y}) for x in wk for y in nb[x] if y not in occ)

    def quiet(wk, bk):
        if captures(wk, bk) or captures(bk, wk) or not safe(wk, bk):
            return False
        occ = set(wk) | set(bk)
        return all(safe(wk, (set(bk) - {x}) | {y}) for x in bk for y in nb[x] if y not in occ)

    n = len(sq)
    if math.comb(n, 3) * math.comb(n - 3, 2) <= EXACT:
        cands = ((wk, bk) for wk in combinations(sorted(sq), 3)
                 for bk in combinations(sorted(set(sq) - set(wk)), 2))
        limit = None
    else:
        keys = sorted(sq)
        cands = ((s[:3], s[3:]) for s in iter(lambda: rng.sample(keys, 5), None))
        limit = SAMPLE
    counts, k = {}, 0
    for wk, bk in cands:
        if not quiet(wk, bk):
            continue
        fen = e.fen([name[x] for x in wk], [name[x] for x in bk])
        key = e.analyse(fen)[0][0] + e.analyse('B' + fen[1:])[0][0]
        counts[key] = counts.get(key, 0) + 1
        k += 1
        if limit and k >= limit:
            break
    return {'counts': counts, 'exact': limit is None}


def longest_any(e):
    """a position with white to move and the longest win (small boards only)"""
    name = {n: f'{chr(97 + f)}{r + 1}' for n, (f, r) in e.g['sq'].items()}
    best = None
    for wk in combinations(sorted(name), 3):
        for bk in combinations(sorted(set(name) - set(wk)), 2):
            fen = e.fen([name[x] for x in wk], [name[x] for x in bk])
            v = e.analyse(fen)[0]
            if v[0] == 'W' and (best is None or int(v[1:]) > best[0]):
                best = (int(v[1:]), fen)
    return best[1] if best else None


def main():
    data = json.loads(gzip.decompress(OUT.read_bytes())) if OUT.exists() else {}
    rng = random.Random(1)
    for h in HEIGHTS:
        for d in EXTRA:
            w = h + d
            for corners in (['unplayable', 'playable'] if w % 2 and h % 2 else ['playable']):
                key = f'{w}x{h} {corners}'
                if key in data:
                    continue
                if w * h // 2 + (w * h % 2 and corners == 'playable') < 6:
                    continue                       # no room for 5 kings and a move
                e = FastEngine(EXE, '--width', w, '--height', h, '--corners', corners, '--rules', 'checkers',
                               '--white', 3, '--black', 2)
                if not e.longest:                  # egdb_fast reports none on a few small boards: search them all
                    e.longest = longest_any(e)
                v = e.analyse(e.longest)[0] if e.longest else 'D'
                entry = CachedEngine.entry(e, int(v[1:])) if v[0] == 'W' else {'g': e.g, 'longest': None, 'analyses': {}}
                entry['quiet'] = quiet_counts(e, rng)
                data[key] = entry
                e.p.terminate()
                OUT.write_bytes(gzip.compress(json.dumps(data, separators=(',', ':'), sort_keys=True).encode(), mtime=0))
                print(key, v, entry['quiet'], flush=True)


if __name__ == '__main__':
    main()
