#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

# Cross table for checkers 3-2: white kings on the edge x black kings in double corners.
# usage: edge_corner.py W H dbdir   (double corners are detected automatically)
import sys
from collections import Counter

W, H, d = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
P = 1
sq, inv = {}, {}
for r in range(H):
    for f in range(W):
        if (f + r) % 2 != P:
            n = (f + (H - 1 - r) * W) // 2 + 1
            sq[n] = (f, r)
            inv[(f, r)] = n
alg = lambda f, r: f"{chr(97 + f)}{r + 1}"

# a corner whose own square is not playable has two playable neighbours on the edges: a double corner
groups, labels = [], []
for cf, cr in ((0, 0), (W - 1, 0), (0, H - 1), (W - 1, H - 1)):
    if (cf, cr) in inv:
        continue
    df = 1 if cf == 0 else -1
    dr = 1 if cr == 0 else -1
    g = [(cf + df, cr), (cf, cr + dr)]
    groups.append({inv[x] for x in g})
    labels.append('/'.join(alg(*x) for x in g))

tot, dr_ = Counter(), Counter()
for line in open(f"{d}/db_3_2.txt"):
    if line[0] == '#': continue
    w, b, dw, dbt, cw, cb = line.rstrip('\n').split('\t')
    if cw != '0' or cb != '0': continue
    e = sum(1 for x in w.split(',') if (lambda f, r: r in (0, H - 1) or f in (0, W - 1))(*sq[int(x)]))
    bl = set(map(int, b.split(',')))
    k = sum(1 for g in groups if g & bl)
    tot[(e, k)] += 1
    dr_[(e, k)] += dw == 'D'

print(f"\n{H}x{W}  (double corners {', '.join(labels)})")
print("| White on edge | Black in 0 double corners | in 1 | in 2 |")
print("|---|---|---|---|")
for e in range(4):
    cells = []
    for k in range(3):
        n, x = tot[(e, k)], dr_[(e, k)]
        cells.append(f"{x:,} / {n:,} ({100 * x / n:.1f}%)".replace(',', '.') if n else '-')
    print(f"| {e} | " + " | ".join(cells) + " |")
alln, alld = sum(tot.values()), sum(dr_.values())
print(f"total: {alld:,} / {alln:,} draws ({100 * alld / alln:.2f}%)".replace(',', '.'))
