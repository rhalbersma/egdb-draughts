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


def diagram(fen, numbers=True):
    """Framed diagram with rank numbers and file letters; with numbers=True the numbered board is
    printed next to it, two characters per square."""
    cell = {}
    for part in fen.split(':')[1:]:
        side = part[0]
        for it in part[1:].split(','):
            if it:
                king = it[0] == 'K'
                cell[int(it[1:] if king else it)] = (side.upper() if king else side.lower())
    gap = '     '
    num_rule = '+' + '-' * 19 + '+'
    lines = ['    +-------------+' + (gap + num_rule if numbers else '')]
    for r in range(6):                       # r = 0 is row 6 (squares 1-3)
        row, nums = [' '] * 12, []
        for c in range(6):
            dark = (r + c) % 2 == 1          # b6 is a dark square
            sq = r * 3 + c // 2 + 1
            if dark:
                row[2 * c] = cell.get(sq, '.')
            nums.append(f'{sq:2d}' if dark else '  ')
        line = f'  {6 - r} | ' + ''.join(row) + '|'
        if numbers:
            line += gap + '| ' + ' '.join(nums) + ' |'
        lines.append(line.rstrip())
    lines += ['    +-------------+' + (gap + num_rule if numbers else ''),
              '      a b c d e f']
    return '\n'.join(lines)


if __name__ == '__main__':
    for f in sys.argv[1:]:
        print(diagram(f))
        print()
