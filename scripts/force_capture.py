# Checkers (short kings): from a start position, can black force winning a white king, and how fast?
# Explores all positions reachable without captures; black "scores" when it is black's turn and
# black has a capture; white "escapes" when it is white's turn and white has a capture.
# usage: force_capture.py W H FEN
import sys
from collections import deque

W, H = int(sys.argv[1]), int(sys.argv[2])
fen = sys.argv[3]
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

def has_capture(own, opp):
    occ = own | opp
    return any(nb(s, d) in opp and nb(s, d, 2) is not None and nb(s, d, 2) not in occ for s in own for d in dirs)

def quiet_moves(own, opp):
    occ = own | opp
    return [(s, t) for s in own for d in dirs if (t := nb(s, d)) is not None and t not in occ]

def parse(fen):
    stm, *fields = fen.split(':')
    pos = {}
    for fl in fields:
        pos[fl[0]] = frozenset(int(x[1:]) for x in fl[1:].split(','))
    return (pos['W'], pos['B'], 0 if stm == 'W' else 1)

start = parse(fen)
succ, kind = {}, {}
seen = {start}
queue = deque([start])
while queue:
    s = queue.popleft()
    w, b, stm = s
    own, opp = (w, b) if stm == 0 else (b, w)
    if has_capture(own, opp):
        kind[s] = 'score' if stm == 1 else 'escape'
        succ[s] = []
        continue
    kind[s] = 'play'
    nxt = []
    for f, t in quiet_moves(own, opp):
        no = (own - {f}) | {t}
        c = (no, b, 1) if stm == 0 else (w, no, 0)
        nxt.append(((f, t), c))
        if c not in seen:
            seen.add(c)
            queue.append(c)
    succ[s] = nxt

# attractor for black, by distance (plies until black has a capture on the move)
dist = {s: 0 for s, k in kind.items() if k == 'score'}
pred = {}
for s, nx in succ.items():
    for _, c in nx:
        pred.setdefault(c, []).append(s)
remaining = {s: len(nx) for s, nx in succ.items()}
frontier = list(dist)
d = 0
while frontier:
    nxt_frontier = []
    for c in frontier:
        for p in pred.get(c, []):
            if p in dist:
                continue
            if p[2] == 1:                       # black to move: one move into the attractor suffices
                dist[p] = d + 1
                nxt_frontier.append(p)
            else:                               # white to move: every move must lead into it
                remaining[p] -= 1
                if remaining[p] == 0:
                    dist[p] = d + 1
                    nxt_frontier.append(p)
    frontier = nxt_frontier
    d += 1

print(f"{len(seen)} positions reachable without captures")
if start in dist:
    print(f"black forces a capture: black has a capture on the move after {dist[start]} plies (with best play)")
    # principal line: white delays as long as possible, black is fastest
    s, line = start, []
    while kind[s] == 'play':
        moves = succ[s]
        if s[2] == 0:
            mv, c = max(((m, c) for m, c in moves if c in dist), key=lambda mc: dist[mc[1]])
        else:
            mv, c = min(((m, c) for m, c in moves if c in dist), key=lambda mc: dist[mc[1]])
        line.append(f"{alg(mv[0])}-{alg(mv[1])}")
        s = c
    print("line:", ' '.join(line), "-> black captures")
else:
    print("black cannot force a capture: white can avoid losing a king forever (repetition)")
