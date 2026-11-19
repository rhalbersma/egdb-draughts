#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The configure step of databases/CMakeLists.txt (synthesis section 7.3): what a build will write, and where.

    egdb_configure.py <builder> <pieces> <metrics> <data_dir> [tree]

  - computes the disk a build needs, exactly, from the slot counts (a product of four binomials per material): the
    files of every material, plus the largest material's temporary copies while it is being written;
  - probes the disk of <data_dir>: free bytes, and whether it is rotational (an HDD) or not (SSD, NVMe, RAM disk);
  - with `tree` (once the checks have passed): creates the tree of subdivisions for every metric,
    <data_dir>/<metric>/6x6/<np>/<wp>-<bp>/.

Prints key=value lines for CMake. The engines are not needed: they are only compiled at build time, after this step.
"""

import math
import os
import shutil
import sys


def materials(n, builder):
    """(wm, wk, bm, bk) with at most n pieces, both sides on the board; syzygy: one of each mirror pair"""
    out = []
    for wm in range(n + 1):
        for wk in range(n + 1 - wm):
            for bm in range(n + 1 - wm - wk):
                for bk in range(n + 1 - wm - wk - bm):
                    if wm + wk == 0 or bm + bk == 0:
                        continue
                    if builder == 'syzygy' and (bm, wm, bk, wk) < (wm, bm, wk, bk):
                        continue
                    out.append((wm, wk, bm, bk))
    return out


def slots(m):
    wm, wk, bm, bk = m
    return math.comb(15, wm) * math.comb(15, bm) * math.comb(18, wk) * math.comb(18, bk)


def material_bytes(m, builder):
    """the four files of one material and metric: two of bytes, two of outcome bits (thompson86: 1, syzygy: 2 per slot)"""
    s = slots(m)
    return 2 * s + 2 * (-(-s // 8) if builder == 'thompson86' else -(-s // 4))


def build_temp_bytes(m, builder):
    """while a material is written: its files under temporary names, and the depth sink's group-order files"""
    return material_bytes(m, builder) + 2 * slots(m) * (1 if builder == 'thompson86' else 2)


def tree(m):
    wm, wk, bm, bk = m
    wp, bp = wm + wk, bm + bk
    return f'6x6/{wp + bp:02}/{wp:02}-{bp:02}'


def rotational(path):
    """1 for a spinning disk, 0 for SSD, NVMe or RAM, -1 if unknown (from /sys/block/<device>/queue/rotational)"""
    try:
        st = os.stat(path)
        dev = f'{os.major(st.st_dev)}:{os.minor(st.st_dev)}'
        sysdev = os.path.realpath(f'/sys/dev/block/{dev}')
        for d in (sysdev, os.path.dirname(sysdev)):              # a partition's queue is its disk's
            q = os.path.join(d, 'queue', 'rotational')
            if os.path.exists(q):
                with open(q) as f:
                    return int(f.read().strip())
    except OSError:
        pass
    return -1


def main():
    builder, n, metrics, data_dir = sys.argv[1], int(sys.argv[2]), sys.argv[3].split(';'), sys.argv[4]
    ms = materials(n, builder)
    if sys.argv[5:] == ['tree']:
        for metric in metrics:
            for m in ms:
                os.makedirs(os.path.join(data_dir, metric, tree(m)), exist_ok=True)
        return
    files = sum(material_bytes(m, builder) for m in ms)
    temp = max(build_temp_bytes(m, builder) for m in ms)
    print(f'materials={len(ms)}')
    print(f'files_mb={len(metrics) * files / 2**20:.0f}')
    print(f'need_mb={(len(metrics) * files + temp) / 2**20:.0f}')
    print(f'free_mb={shutil.disk_usage(data_dir).free / 2**20:.0f}')
    print(f'rotational={rotational(data_dir)}')


if __name__ == '__main__':
    main()
