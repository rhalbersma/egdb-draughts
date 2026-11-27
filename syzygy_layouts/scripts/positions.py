#!/usr/bin/env python3
# Game-related probe positions for the timing runs: games played out from the start position by a random mover that
# prefers captures, until at most `pieces` men are left; from there every position of the game and all its children
# are written (a search around a game line), for up to 40 plies. Positions with an en passant square are skipped.
# usage: positions.py <out> <count> <pieces> <seed>
# Record (72 bytes, little-endian): white, black, kings, queens, rooks, bishops, knights, pawns (u64 each), turn, 7 pad.
import chess, random, struct, sys
out, n, pieces, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
random.seed(seed)
def rec(b):
    return struct.pack('<8QB7x', b.occupied_co[chess.WHITE], b.occupied_co[chess.BLACK], b.kings, b.queens, b.rooks, b.bishops, b.knights, b.pawns, int(b.turn))
def usable(b):
    return chess.popcount(b.occupied) <= pieces and b.ep_square is None and not b.castling_rights
def pick(b):
    moves = list(b.legal_moves)
    caps = [m for m in moves if b.is_capture(m)]
    return random.choice(caps) if caps and random.random() < 0.6 else random.choice(moves)
with open(out, 'wb') as f:
    k = 0
    while k < n:
        b = chess.Board()
        while not b.is_game_over() and chess.popcount(b.occupied) > pieces and b.ply() < 600: b.push(pick(b))
        for _ in range(40):
            if k >= n or b.is_game_over() or chess.popcount(b.occupied) < 3: break
            if usable(b): f.write(rec(b)); k += 1
            for m in list(b.legal_moves):
                b.push(m)
                if k < n and usable(b): f.write(rec(b)); k += 1
                b.pop()
            b.push(pick(b))
