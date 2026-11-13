# usage: trap.py W H P dbfile sq sq sq
# For a fixed placement of the three white kings, show the DTM for every square of the
# black king: a map with black to move, plus summaries for black and for white to move.
import sys
from collections import Counter

W, H, P = map(int, sys.argv[1:4])
dbf = sys.argv[4]
setup = sys.argv[5:]

sq, inv = {}, {}
for r in range(H):
    for f in range(W):
        if (f + r) % 2 != P:
            n = (f + (H - 1 - r) * W) // 2 + 1
            sq[n] = (f, r)
            inv[(f, r)] = n

alg = lambda n: f"{chr(97 + sq[n][0])}{sq[n][1] + 1}"
num = lambda a: inv[(ord(a[0]) - 97, int(a[1:]) - 1)]
wk = sorted(num(a) for a in setup)
key = ','.join(map(str, wk))

rows = {}
for line in open(dbf):
    if line[0] == '#' or not line.startswith(key + '\t'):
        continue
    w, b, dw, db, cw, cb = line.rstrip('\n').split('\t')
    rows[int(b)] = (dw, db, cw, cb)

for title, idx in (("black to move", 1), ("white to move", 0)):
    print(f"\n{H}x{W} p{P}, white kings {' '.join(setup)}: DTM per square of the black king, {title}")
    print("    +" + "-" * (4 * W + 1) + "+")
    for r in reversed(range(H)):
        line = f"{r + 1:3} | "
        for f in range(W):
            n = inv.get((f, r))
            if n is None:
                line += "    "
            elif n in wk:
                line += " W  "
            else:
                can = rows[n][2 if idx == 0 else 3] == '1'   # side to move can capture
                line += f"{rows[n][idx] + ('x' if can else ''):<4}"
        print(line + "|")
    print("    +" + "-" * (4 * W + 1) + "+")
    print("      " + "".join(f"{chr(97 + f):<4}" for f in range(W)))
    c = Counter(v[idx] for v in rows.values())
    print("  counts:", ", ".join(f"{k}: {c[k]}" for k in sorted(c, key=lambda k: (k == 'D', abs(int(k)) if k != 'D' else 0))))
