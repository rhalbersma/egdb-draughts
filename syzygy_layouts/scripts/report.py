#!/usr/bin/env python3
# Turns the CSV files of a run ($WORK/results) into the tables of REPORT.md (printed to stdout).
# usage: report.py <results dir>
import csv, glob, os, sys
from collections import defaultdict

res = sys.argv[1]
ORDER = ['a', 'b', 'c0', 'c1', 'c2', 'd0', 'd1', 'd2']
NAME = {'a': 'a: Syzygy as is', 'b': 'b: Huffman, u16 pages', 'c0': 'c0: 12-bit, u16 pages', 'c1': 'c1: 12-bit, u32 pages', 'c2': 'c2: 12-bit, Elias-Fano pages',
        'd0': 'd0: rebuilt grammar, u16 pages', 'd1': 'd1: rebuilt grammar, u32 pages', 'd2': 'd2: rebuilt grammar, Elias-Fano pages'}

def mb(x): return '%.2f' % (x / 1e6)

sizes = {}
for path in glob.glob(os.path.join(res, 'sizes_*.csv')):
    if path.endswith('sizes_all.csv'): continue
    rows = list(csv.DictReader(open(path)))
    if rows: sizes[rows[0]['variant']] = rows
variants = [v for v in ORDER if v in sizes]

def total(rows, key, pred=lambda r: True): return sum(int(r[key]) for r in rows if pred(r))

def size_table(title, pred):
    print('### ' + title + '\n')
    print('| Variant | Coded data | Padding | Index | Grammar and code tables | Total | Against a |')
    print('|---|---|---|---|---|---|---|')
    base = total(sizes['a'], 'total_bytes', pred) if 'a' in sizes else 0
    for v in variants:
        r = sizes[v]
        t = total(r, 'total_bytes', pred)
        print('| %s | %s | %s | %s | %s | %s MB | %s |' % (NAME[v], mb(total(r, 'coded_bytes', pred)), mb(total(r, 'padding_bytes', pred)), mb(total(r, 'index_bytes', pred)),
                                                          mb(total(r, 'grammar_bytes', pred)), mb(t), '%.1f %%' % (100.0 * t / base) if base else ''))
    print()

if sizes:
    a = sizes.get('a', next(iter(sizes.values())))
    n_sub = len(a); n_const = sum(1 for r in a if r['const'] == '1')
    print('## Sizes\n')
    print('%d sub-tables in %d tables, %d of them a single constant; %s index values in all.\n' % (n_sub, len({r['table'] for r in a}), n_const, format(total(a, 'tb_size'), ',')))
    size_table('All tables', lambda r: True)
    big = {(r['table'], r['subtable']) for r in a if int(r['total_bytes']) >= 1 << 20}
    if len(big) != n_sub - n_const: size_table('Only the %d sub-tables of at least 1 MiB in the Syzygy file (page rounding plays no part)' % len(big), lambda r: (r['table'], r['subtable']) in big)
    small = {(r['table'], r['subtable']) for r in a if r['const'] == '0' and int(r['total_bytes']) < 4096}
    if small: size_table('Only the %d sub-tables below 4 KiB in the Syzygy file (each takes at least one page here)' % len(small), lambda r: (r['table'], r['subtable']) in small)
    print('### Details\n')
    print('| Variant | Codes | Pages | Lines of 64 / 32 / 16 bytes (sub-tables) | Pages with 3 / 4 header lines | Symbols split at subdivisions (a: at his block ends) |')
    print('|---|---|---|---|---|---|')
    for v in variants:
        r = sizes[v]
        lb = defaultdict(int)
        for x in r:
            if x['const'] == '0': lb[x['line_bytes']] += 1
        print('| %s | %s | %s | %d / %d / %d | %s / %s | %s |' % (NAME[v], format(total(r, 'codes'), ','), format(total(r, 'pages'), ','), lb['64'], lb['32'], lb['16'],
                                                                 total(r, 'header3'), total(r, 'header4'), total(r, 'split_symbols')))
    print()
    with open(os.path.join(res, 'sizes_all.csv'), 'w', newline='') as f:
        w = None
        for v in variants:
            for r in sizes[v]:
                if w is None: w = csv.DictWriter(f, fieldnames=list(r.keys())); w.writeheader()
                w.writerow(r)

bench_path = os.path.join(res, 'bench.csv')
if os.path.exists(bench_path):
    rows = list(csv.DictReader(open(bench_path)))
    key = lambda r: (r['trace'], r['kind'], r['cache'], r['threads'], r['variant'])
    b = {key(r): r for r in rows}
    bv = [v for v in ORDER if any(r['variant'] == v for r in rows)]
    print('## Probe performance\n')
    for trace, kind, title in (('seq', 'index', 'Sequential indices, index to value'), ('uniform', 'index', 'Uniform random indices, index to value'),
                               ('game', 'index', 'Game-related probes, index to value'), ('game', 'position', 'Game-related positions, end to end')):
        print('### ' + title + '\n')
        print('| Variant | Warm, 1 thread: mean | p50 | p90 | p99 | Warm, 4 threads | Cold, 1 thread: mean | p50 | p99 | Cold, 4 threads |')
        print('|---|---|---|---|---|---|---|---|---|---|')
        for v in bv:
            w1, w4, c1, c4 = (b.get((trace, kind, c, t, v)) for c, t in (('warm', '1'), ('warm', '4'), ('cold', '1'), ('cold', '4')))
            ns = lambda r, k: ('%.0f ns' % float(r[k])) if r else ''
            us = lambda r, k: ('%.1f us' % (float(r[k]) / 1e3)) if r else ''
            rate = lambda r: ('%.2f M/s' % (float(r['probes_per_s']) / 1e6)) if r else ''
            print('| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |' % (NAME[v], ns(w1, 'mean_ns'), ns(w1, 'p50_ns'), ns(w1, 'p90_ns'), ns(w1, 'p99_ns'), rate(w4),
                                                                          us(c1, 'mean_ns'), us(c1, 'p50_ns'), us(c1, 'p99_ns'), rate(c4)))
        sums = {r['checksum'] for r in rows if r['trace'] == trace and r['kind'] == kind and r['cache'] == 'warm' and r['threads'] == '1'}
        print('\nChecksums of the values returned: %s.\n' % ('identical for all variants' if len(sums) == 1 else 'DIFFERENT: ' + ', '.join(sorted(sums))))
