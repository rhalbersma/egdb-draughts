#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Text diagram of a 6x6 position (FEN W:W17,K16:B2,K15), in the style of the README:
w / b = white / black man, W / B = white / black king, . = empty dark square.

usage: board.py FEN [FEN ...]
"""

import sys


def diagram(fen):
    cell = {}
    for part in fen.split(':')[1:]:
        side = part[0]
        for it in part[1:].split(','):
            if it:
                king = it[0] == 'K'
                cell[int(it[1:] if king else it)] = (side.upper() if king else side.lower())
    lines = ['    +-------------+']
    for r in range(6):                       # r = 0 is row 6 (squares 1-3)
        row = [' '] * 12
        for c in range(6):
            if (r + c) % 2 == 1:             # dark square: b6 is dark
                sq = r * 3 + c // 2 + 1
                row[2 * c] = cell.get(sq, '.')
        lines.append(f'  {6 - r} | ' + ''.join(row) + '|')
    lines += ['    +-------------+', '      a b c d e f']
    return '\n'.join(lines)


if __name__ == '__main__':
    for f in sys.argv[1:]:
        print(diagram(f))
        print()
