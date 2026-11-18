#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Give a unit store built with units_6x6.py the files with which the Snakefile sees the finished steps.

For every step in <dir>/state.json the file of that step is made (.built, .verified, .compared, .crc from crc32c.txt,
.done; the .cdb of the compress step must be there), with modification times that increase in build order, so that
Snakemake does not take an earlier file for a newer one. A step that is not in state.json is left to Snakemake (an
interrupted build continues from its checkpoint).

usage: units_markers.py [--dir data/units] [--nmax 12]
"""

import argparse
import json
import os
import time

from units_6x6 import STEPS, units

FILES = {'build': '.built', 'verify_raw': '.verified', 'compress': '.cdb', 'compare': '.compared', 'crc': '.crc',
         'delete_raw': '.done'}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dir', default='data/units')
    ap.add_argument('--nmax', type=int, default=12)
    a = ap.parse_args()
    state = json.load(open(os.path.join(a.dir, 'state.json')))
    crc = {}
    for line in open(os.path.join(a.dir, 'crc32c.txt')):
        n, m, *_ = line.split()
        crc[(int(n), int(m))] = line
    t = time.time() - 10 * 24 * 3600     # all files in the past, one second apart, in build order
    made = 0
    for n, m in units(a.nmax):
        done = state.get(f'{n}/{m}', [])
        for s in STEPS:
            if s not in done:
                break
            path = os.path.join(a.dir, f'6x6-{n:02}-{m:02}{FILES[s]}')
            if s == 'compress':
                assert os.path.exists(path), f'{path} missing'
            elif s == 'crc':
                with open(path, 'w') as f:
                    f.write(crc[(n, m)])
            elif not os.path.exists(path):
                open(path, 'w').close()
            os.utime(path, (t, t))
            t += 1
            made += 1
    os.utime(os.path.join(a.dir, 'crc32c.txt'), (t, t))
    print(f'{made} files of finished steps')


if __name__ == '__main__':
    main()
