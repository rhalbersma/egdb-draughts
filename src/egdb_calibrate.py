#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Collect the instrumentation of a build into a calibration file (synthesis section 7.4).

    python3 src/egdb_calibrate.py <data_dir> > calibration.tsv

Every build is a calibration run, as a profile-guided build: each material's log ends with a "stats" line (metric,
material, pieces, positions, groups, levels, frontier positions summed over the rounds, seconds in pass 0, in the
rounds, in writing, in all). A small build (fewer pieces) measures what the rules, the engine and the machine cost per
position and how the levels grow; src/egdb_layout.py --calibration extrapolates from it to the build to plan.
"""

import os
import sys

FIELDS = ['metric', 'material', 'pieces', 'positions', 'groups', 'levels', 'frontier', 'pass0_s', 'rounds_s', 'write_s',
          'total_s']


def main():
    root = sys.argv[1]
    rows = []
    for d, _, files in os.walk(root):
        for f in files:
            if f.endswith('.log'):
                with open(os.path.join(d, f)) as fh:
                    rows += [line.rstrip('\n').split('\t')[1:] for line in fh if line.startswith('stats\t')]
    if not rows:
        sys.exit(f'{root}: no stats lines (build logs of thompson86 or tbgen)')
    print('#' + '\t'.join(FIELDS))
    for r in sorted(rows, key=lambda r: (r[0], int(r[2]), r[1])):
        print('\t'.join(r))


if __name__ == '__main__':
    main()
