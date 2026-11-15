#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""Build, check and compress every unit of the 6x6 databases (at most 12 pieces, at most 6 per side) with egdb_6x6.

A unit holds the positions with n pieces of which m are men. The units are processed in the order (n, m), each in
these steps, every step a separate run of egdb_6x6:
  1. build         the unit as a raw file, one byte of DTM per position (the units before it compressed);
  2. verify raw    every position: the stored value equals the value recomputed from its successors;
  3. compress      blocks of 4096 positions, zstd -19, capture positions filled in;
  4. compare       compressed against raw, both ways, every position;
  5. delete raw.
(A self-consistency check of the compressed unit adds nothing to steps 2 and 4: after them every compressed value
equals a verified raw value. It is available as `unitverify <dir> <n> <m> cdb`.)
The state is kept in <dir>/state.json, so that an interrupted run continues where it stopped (an interrupted build
continues from its last checkpoint). Results go to <dir>/units.log (one JSON object per step).

usage: units_6x6.py path/to/egdb_6x6 [--dir data/units] [--nmax 12] [--threads 4]
"""

import argparse
import json
import os
import subprocess
import sys
import time

STEPS = ['build', 'verify_raw', 'compress', 'compare', 'delete_raw']


def units(nmax):
    return [(n, m) for n in range(2, nmax + 1) for m in range(0, n + 1)]


def run(engine, command, err):
    t0 = time.time()
    p = subprocess.run([engine], input=command + '\n', capture_output=True, text=True)
    err.write(p.stderr)
    err.flush()
    lines = [x for x in p.stdout.splitlines() if x != 'end']
    ok = p.returncode == 0 and bool(lines) and not lines[-1].startswith('error')
    return ok, lines, time.time() - t0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('engine')
    ap.add_argument('--dir', default='data/units')
    ap.add_argument('--nmax', type=int, default=12)
    ap.add_argument('--threads', type=int, default=4)
    a = ap.parse_args()
    os.makedirs(a.dir, exist_ok=True)
    state_path = os.path.join(a.dir, 'state.json')
    state = json.load(open(state_path)) if os.path.exists(state_path) else {}
    log = open(os.path.join(a.dir, 'units.log'), 'a')
    err = open(os.path.join(a.dir, 'engine.err'), 'a')
    d, t = a.dir, a.threads
    for n, m in units(a.nmax):
        key = f'{n}/{m}'
        done = state.setdefault(key, [])
        for step in STEPS:
            if step in done:
                continue
            raw = os.path.join(d, f'6x6_{n}_{m}.raw')
            if step == 'build':
                cmd = f'unitbuild {d} {n} {m} {t}'
            elif step == 'verify_raw':
                cmd = f'unitverify {d} {n} {m} raw {t}'
            elif step == 'compress':
                cmd = f'unitcompress {d} {n} {m} {t}'
            elif step == 'compare':
                cmd = f'unitcompare {d} {n} {m} {t}'
            elif step == 'verify_cdb':
                cmd = f'unitverify {d} {n} {m} cdb {t}'
            else:
                cmd = None
            if cmd is None:
                for f in (raw, raw + '.progress'):
                    if os.path.exists(f):
                        os.remove(f)
                ok, lines, sec = True, ['deleted ' + raw], 0.0
            else:
                print(time.strftime('%H:%M:%S'), key, step, flush=True)
                ok, lines, sec = run(a.engine, cmd, err)
            log.write(json.dumps({'unit': key, 'step': step, 'ok': ok, 'out': lines, 'seconds': round(sec, 1),
                                  'time': time.strftime('%Y-%m-%d %H:%M:%S')}) + '\n')
            log.flush()
            if not ok:
                print(f'{key} {step} FAILED: {lines}', flush=True)
                sys.exit(1)
            done.append(step)
            json.dump(state, open(state_path + '.tmp', 'w'), indent=0)
            os.replace(state_path + '.tmp', state_path)
    print('all units done', flush=True)


if __name__ == '__main__':
    main()
