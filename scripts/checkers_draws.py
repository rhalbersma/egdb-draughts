# Classify the quiet checkers 3-2 draws with white to move: what happens after each white move?
# usage: checkers_draws.py W H dbdir
import sys
from collections import Counter

W, H = int(sys.argv[1]), int(sys.argv[2])
dbdir = sys.argv[3]
P = 1
sq, inv = {}, {}
for r in range(H):
    for f in range(W):
        if (f + r) % 2 != P:
            n = (f + (H - 1 - r) * W) // 2 + 1
            sq[n] = (f, r)
            inv[(f, r)] = n
alg = lambda n: f"{chr(97 + sq[n][0])}{sq[n][1] + 1}"
dirs = [(1, 1), (-1, 1), (1, -1), (-1, -1)]

def nb(n, d, k=1):
    f, r = sq[n]
    return inv.get((f + k * d[0], r + k * d[1]))

def captures(own, opp):
    """complete jump sequences for short kings: list of (from, to, captured set)"""
    out = []
    def dfs(start, cur, occ, taken):
        ext = False
        for d in dirs:
            t, u = nb(cur, d), nb(cur, d, 2)
            if t in opp and t not in taken and u is not None and u not in occ:
                ext = True
                dfs(start, u, occ, taken | {t})
        if not ext and taken:
            out.append((start, cur, frozenset(taken)))
    for s in own:
        dfs(s, s, (own | opp) - {s}, frozenset())
    return out

def moves(own, opp):
    caps = captures(own, opp)
    if caps:
        return caps, True
    occ = own | opp
    return [(s, t, frozenset()) for s in own for d in dirs if (t := nb(s, d)) is not None and t not in occ], False

db = {}
for line in open(f"{dbdir}/db_3_2.txt"):
    if line[0] == '#': continue
    w, b, dw, dbt, cw, cb = line.rstrip('\n').split('\t')
    db[(w, b)] = (dw, dbt, cw, cb)

def key(white, black):
    return (','.join(map(str, sorted(white))), ','.join(map(str, sorted(black))))

cat = Counter()
examples = {}
for (w, b), (dw, dbt, cw, cb) in db.items():
    if dw != 'D' or cw != '0' or cb != '0':
        continue
    white = set(map(int, w.split(','))); black = set(map(int, b.split(',')))
    ms, _ = moves(white, black)
    after = []
    for s, t, _ in ms:
        nw = (white - {s}) | {t}
        bcaps = captures(black, nw)
        if bcaps:
            # how many white kings can black take at most, and does it hit two kings (a fork)?
            best = max(len(c) for _, _, c in bcaps)
            targets = {x for _, _, c in bcaps for x in c}
            after.append('fork' if len(targets) >= 2 and best == 1 else f'loses {best}')
        else:
            after.append('safe')
    if all(a != 'safe' for a in after):
        k = 'every white move allows a black capture'
    else:
        k = 'white has a safe move, but no win'
    cat[k] += 1
    examples.setdefault(k, f"W:W{','.join('K%d' % x for x in sorted(white))}:B{','.join('K%d' % x for x in sorted(black))}")
print(f"{H}x{W}: {sum(cat.values())} quiet draws with white to move")
for k, n in cat.most_common():
    print(f"  {k:45} {n:6}   e.g. {examples[k]}")
