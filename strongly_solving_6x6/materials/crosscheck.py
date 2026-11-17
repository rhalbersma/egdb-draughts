#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Compare a per-material store (Snakefile in this directory) with a unit store (strongly_solving_6x6/international):
for every material, a hash of all its positions without capture with their values, keyed by the position (not by its
offset), so equal for the same values in any layout. Uses the unitvalues and matvalues commands of egdb_6x6.

usage: crosscheck.py path/to/egdb_6x6 --units data/units_checkers --materials data/materials_checkers [--nmax 8] [--threads 4]
"""

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from materials import materials, name  # noqa: E402


def run(engine, commands):
    out = subprocess.run([engine], input=''.join(c + '\n' for c in commands), capture_output=True, text=True).stdout
    errors = [x for x in out.splitlines() if x.startswith('error')]
    if errors:
        raise RuntimeError('\n'.join(errors))
    # material wp bp wm bm positions <count> <hash>
    return {'-'.join(f'{int(t):02}' for t in x.split()[1:5]): (int(x.split()[6]), x.split()[7])
            for x in out.splitlines() if x.startswith('material ')}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--units', required=True)
    ap.add_argument('--materials', required=True)
    ap.add_argument('--nmax', type=int, default=8)
    ap.add_argument('--threads', type=int, default=4)
    a = ap.parse_args()
    mats = [name(m) for m in materials(a.nmax)]
    units = sorted({(sum(int(t) for t in m.split('-')[:2]), sum(int(t) for t in m.split('-')[2:])) for m in mats})
    u = run(a.engine, [f'unitvalues {a.units} {n} {m} {a.threads}' for n, m in units])
    v = run(a.engine, [f'matvalues {a.materials} {m.replace("-", " ")} {a.threads}' for m in mats])
    bad = [m for m in mats if u.get(m) != v.get(m)]
    for m in bad:
        print('DIFFERENT', m, 'units', u.get(m), 'materials', v.get(m))
    print(f'{len(mats) - len(bad)} of {len(mats)} materials equal ({sum(c for c, _ in v.values()):,} positions without capture)')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
