#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The configure step of databases/CMakeLists.txt (synthesis section 7.3): what a build will write, and where.

    egdb_configure.py <builder> <pieces> <metrics> <data_dir> <jobs> [tree]

  - computes the disk a build needs, exactly, from the slot counts (a product of four binomials per material): the
    files of every material (cumulative: they are permanent), plus the scratch of the materials being written side by
    side (the depth sink's group-order files; a file under its temporary name becomes the final one by a rename), at
    most <jobs> of them, the largest;
  - computes the I/O a build does (rungs 0 to 2, synthesis section 7.4): every file written once, sequentially; at
    rung 1 the depth sink's group-order files written once more and read back; each build reads its inputs (the files
    of the materials its captures and promotions lead into: the depths for dtm, the outcomes for dtc and dtz) through
    memory maps, at best each input once (the page cache keeps it for every build that needs it), at worst once per
    build that reads it; the final checks read every file once;
  - probes the disk of <data_dir>: free bytes, and whether it is rotational (an HDD) or not (SSD, NVMe, RAM disk);
  - with `tree` (once the checks have passed): creates the tree of subdivisions for every metric,
    <data_dir>/<metric>/6x6/<np>/<wp>-<bp>/ and <data_dir>/wdl/6x6/<np>/<wp>-<bp>/ (the wdl files, shared).

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
    """the joint index of src/materials_6x6.hpp (the default): the white men on their 15 squares, the black men on theirs
    minus the white men (12 squares are in both domains), the kings on the remaining squares, black after white"""
    wm, wk, bm, bk = m
    men = sum(math.comb(12, k) * math.comb(3, wm - k) * math.comb(15 - k, bm) for k in range(wm + 1))
    e = 18 - wm - bm
    return men * math.comb(e, wk) * math.comb(e - wk, bk)


def material_bytes(m, builder):
    """the four files of one material and metric: two of bytes, two of outcome bits (thompson86: 1, syzygy: 2 per slot)"""
    s = slots(m)
    return 2 * s + 2 * (-(-s // 8) if builder == 'thompson86' else -(-s // 4))


def build_temp_bytes(m, builder):
    """while a material is written: the depth sink's group-order files (its files under temporary names become the
    final ones by a rename, so they are counted with the files already)"""
    return 2 * slots(m) * (1 if builder == 'thompson86' else 2)


def successors(m):
    """the materials one move leads into (as successor_materials in src/materials_6x6.hpp)"""
    out = set()
    for side in (0, 1):
        s = m if side == 0 else (m[2], m[3], m[0], m[1])
        back = (lambda x: x) if side == 0 else (lambda x: (x[2], x[3], x[0], x[1]))
        wm, wk, bm, bk = s
        if wm > 0:
            out.add(back((wm - 1, wk + 1, bm, bk)))
        for i in range(bm + 1):
            for j in range(bk + 1):
                if 0 < i + j < bm + bk:
                    out.add(back((wm, wk, bm - i, bk - j)))
                    if wm > 0:
                        out.add(back((wm - 1, wk + 1, bm - i, bk - j)))
    out.discard(m)
    return out


def canonical(m, builder):
    wm, wk, bm, bk = m
    return m if builder == 'thompson86' or not (bm, wm, bk, wk) < (wm, bm, wk, bk) else (bm, bk, wm, wk)


def input_bytes(m, builder, metric):
    """what one build of material m reads from one input material: its depths for dtm, its outcomes otherwise"""
    s = slots(m)
    outcome = 2 * (-(-s // 8) if builder == 'thompson86' else -(-s // 4))
    if metric == 'dtm':
        return 2 * s + (outcome if builder == 'syzygy' else 0)
    return outcome


def io(ms, builder, metrics):
    """bytes: written (rung 0), written and read again by the group-order files (rung 1), read (best, worst)"""
    files = sum(material_bytes(m, builder) for m in ms)
    groups = sum(2 * slots(m) * (1 if builder == 'thompson86' else 2) for m in ms)
    best = worst = 0
    for metric in metrics:
        used = set()
        for m in ms:
            for s in successors(m):
                if s[0] + s[1] == 0 or s[2] + s[3] == 0:
                    continue
                c = canonical(s, builder)
                worst += input_bytes(c, builder, metric)
                used.add(c)
        best += sum(input_bytes(c, builder, metric) for c in used)
    return len(metrics) * files, len(metrics) * groups, best + len(metrics) * files, worst + len(metrics) * files


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
    builder, n, metrics, data_dir, jobs = sys.argv[1], int(sys.argv[2]), sys.argv[3].split(';'), sys.argv[4], int(sys.argv[5])
    ms = materials(n, builder)
    if sys.argv[6:] == ['tree']:
        for metric in list(metrics) + ['wdl']:
            for m in ms:
                os.makedirs(os.path.join(data_dir, metric, tree(m)), exist_ok=True)
        return
    files = sum(material_bytes(m, builder) for m in ms)
    temp = sum(sorted((build_temp_bytes(m, builder) for m in ms), reverse=True)[:jobs])
    print(f'materials={len(ms)}')
    print(f'files_mb={len(metrics) * files / 2**20:.0f}')
    print(f'need_mb={(len(metrics) * files + temp) / 2**20:.0f}')
    print(f'scratch_mb={temp / 2**20:.0f}')
    written, grouped, read_best, read_worst = io(ms, builder, metrics)
    print(f'write_mb={written / 2**20:.0f}')
    print(f'write_groups_mb={grouped / 2**20:.0f}')
    print(f'read_best_mb={read_best / 2**20:.0f}')
    print(f'read_worst_mb={read_worst / 2**20:.0f}')
    print(f'free_mb={shutil.disk_usage(data_dir).free / 2**20:.0f}')
    print(f'rotational={rotational(data_dir)}')
    try:
        with open('/proc/cpuinfo') as f:
            print(f"ghz={next(float(l.split(':')[1]) / 1000 for l in f if l.startswith('cpu MHz')):.2f}")
    except (OSError, StopIteration, ValueError):
        pass


if __name__ == '__main__':
    main()
