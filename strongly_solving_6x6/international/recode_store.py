#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Recode a finished 6x6 unit store from .cdb version 1 (draw byte 253) to version 2 (draw byte 255, escape codes for
DTM above 254 plies), with the unitrecode command of egdb_6x6, and regenerate the checksums.

Every unit listed in <dir>/crc32c.txt is recoded in place (a unit that is already version 2 is left as it is): each
block is decoded, its draws recoded, compressed again and decoded once more to check it. The values do not change, but
the stored bytes do, so the CRC32C of every unit changes: <dir>/crc32c.txt and the .crc files of the Snakemake workflow
are written anew. With --values, unitvalues (a hash of the values, independent of the encoding) is computed before and
after, as an extra check (it reads every position, about as long as a verification).

Run it on a finished store only, not while a build is running.

usage: recode_store.py path/to/egdb_6x6 [--dir data/units] [--threads 4] [--values]
"""

import argparse
import os
import subprocess
from pathlib import Path


def engine(exe, commands):
    out = subprocess.run([exe], input=''.join(c + '\n' for c in commands), capture_output=True, text=True).stdout
    errors = [line for line in out.splitlines() if line.startswith('error')]
    if errors:
        raise RuntimeError('\n'.join(errors))
    return out.splitlines()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--dir', default='data/units')
    ap.add_argument('--threads', type=int, default=4)
    ap.add_argument('--values', action='store_true', help='compare unitvalues before and after')
    a = ap.parse_args()
    d = Path(a.dir)
    units = [tuple(map(int, line.split()[:2])) for line in (d / 'crc32c.txt').read_text().splitlines() if line.strip()]
    rows = []
    for n, m in units:
        before = engine(a.engine, [f'unitvalues {d} {n} {m} {a.threads}']) if a.values else None
        out = engine(a.engine, [f'unitrecode {d} {n} {m} {a.threads}', f'unitcrc {d} {n} {m}'])
        recoded = next(line for line in out if line.startswith('recoded'))
        crc = next(line for line in out if line.startswith('crc32c '))[len('crc32c '):]
        if a.values:
            after = engine(a.engine, [f'unitvalues {d} {n} {m} {a.threads}'])
            same = [x for x in before if x.startswith('values')] == [x for x in after if x.startswith('values')]
            if not same:
                raise RuntimeError(f'{n}/{m}: values differ after recoding')
        print(recoded, '|', crc, '| values equal' if a.values else '', flush=True)
        rows.append(crc + '\n')
        (d / f'6x6-{n:02}-{m:02}.crc').write_text(crc + '\n')
    tmp = d / 'crc32c.txt.tmp'
    tmp.write_text(''.join(rows))
    os.replace(tmp, d / 'crc32c.txt')
    print(f'{len(rows)} units recoded, {d / "crc32c.txt"} rewritten')


if __name__ == '__main__':
    main()
