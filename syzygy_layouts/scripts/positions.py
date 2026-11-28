#!/usr/bin/env python3
# Game-related probe positions for the timing runs: games played out from the start position by a random mover that
# prefers captures, until at most `pieces` men are left; from there every position of the game and all its children
# are written (a search around a game line), for up to 40 plies. Positions with an en passant square are skipped.
# usage: positions.py <out> <count> <pieces> <seed> [materials [more materials]]
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
SYM = {'K': chess.KING, 'Q': chess.QUEEN, 'R': chess.ROOK, 'B': chess.BISHOP, 'N': chess.KNIGHT, 'P': chess.PAWN}
def signature(b):
    side = lambda c: ''.join(sorted(chess.piece_symbol(p.piece_type).upper() for p in b.piece_map().values() if p.color == c))
    return frozenset((side(chess.WHITE), side(chess.BLACK))), side(chess.WHITE) == side(chess.BLACK)
def random_position(material):
    w, bl = material.split('v')
    if random.random() < 0.5: w, bl = bl, w
    while True:
        b = chess.Board(None)
        ok = True
        for colour, pieces in ((chess.WHITE, w), (chess.BLACK, bl)):
            for c in pieces:
                sq = random.choice([q for q in chess.SQUARES if b.piece_at(q) is None and (c != 'P' or 0 < chess.square_rank(q) < 7)])
                b.set_piece_at(sq, chess.Piece(SYM[c], colour))
        b.turn = random.random() < 0.5
        if ok and b.is_valid(): return b
if len(sys.argv) > 5:
    # 6th argument: materials (KRPvKRP,KRBvKRN): games start from a random position of one of them and are followed, with
    # all children, for as long as the material stays one of them or of the 7th argument (the tables they capture into)
    materials = sys.argv[5].split(',')
    wanted = set()
    for m in materials + (sys.argv[6].split(',') if len(sys.argv) > 6 else []):
        w, bl = m.split('v'); wanted.add(frozenset((''.join(sorted(w)), ''.join(sorted(bl)))))
    inside = lambda b: signature(b)[0] in wanted and b.ep_square is None
    with open(out, 'wb') as f:
        k = 0
        while k < n:
            b = random_position(random.choice(materials))
            for _ in range(40):
                if k >= n or b.is_game_over() or not inside(b): break
                f.write(rec(b)); k += 1
                for m in list(b.legal_moves):
                    b.push(m)
                    if k < n and inside(b): f.write(rec(b)); k += 1
                    b.pop()
                quiet = [m for m in b.legal_moves if not b.is_capture(m) and not m.promotion]
                b.push(random.choice(quiet) if quiet and random.random() < 0.9 else random.choice(list(b.legal_moves)))
    sys.exit(0)
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
