# On-disk layouts for Syzygy WDL tables: a comparison

A cache-line layout with fixed 12-bit codes halves the table lookup time of the Syzygy WDL format and costs 19 % more
space. A grammar rebuilt for fixed-width codes brings that cost down to 10 % at the same speed. The page index and the
page header make no measurable difference.

All numbers are from one run on the machine described at the end. The full tables are in `results/tables.md`, the raw
rows in `results/bench.csv` and `results/sizes_all.csv`.

## What was compared

Tables: KRPvKRP, KRBvKRN and KPPvKPP (published 6-piece WDL files, verified against `checksums/wdl6.txt` of
syzygy1/tb) and the 51 tables of 3 to 5 pieces they capture into (`results/dependent_tables.txt`; generated here with
`rtbgen -w` and verified against `checksums/wdl345.txt`). Together: 54 tables, 230 sub-tables of which 21 are a single
constant, 37.5 billion index values. The largest sub-table has 6.77 billion.

Every variant keeps his index function and piece order, works per sub-table, and stores the grammar in his 3-byte
format. The variants b, c and d put a sub-table in 4 KiB pages that are self-contained: a header that says where each
line starts, then lines. A separate small index finds the page.

| | Grammar | Codes | Lines | Page header |
|---|---|---|---|---|
| a | his | his Huffman codes | his blocks of 64 or 32 bytes | none: his sparse index and block sizes; probed with Fathom |
| b | his | his Huffman codes | his block size | `u16` positions per line |
| c0 | his | 12 bits | 64 bytes, 42 codes | `u16` positions per line (2 header lines) |
| c1 | his | 12 bits | 64 bytes, 42 codes | `u32` running totals (4 header lines) |
| c2 | his | 12 bits | 64 bytes, 42 codes | Elias-Fano list of line starts (2 header lines, more if needed) |
| d0 | rebuilt, symbols up to 1,560 positions | 12 or 10 bits | 64 bytes | as c0 |
| d2 | rebuilt, symbols up to 65,536 positions | 12 or 10 bits | 64 bytes | as c2 |

## Correctness

Before any timing, each variant was checked against Fathom on all 230 sub-tables: 37.5 billion positions decoded in
order through the page headers, every index of the tables of 3 and 4 pieces looked up (138 million), and 10 million
random lookups per larger sub-table (1.02 billion). No failures in any of b, c0, c1, c2, d0, d2 (`results/check.txt`).

The benchmarks return identical checksums for all variants on every trace.

## Sizes

| Variant | Coded data | Padding | Index | Total | Against a |
|---|---|---|---|---|---|
| a | 880.8 MB | 5.0 MB | 31.9 MB | 918.9 MB | 100.0 % |
| b | 880.8 MB | 9.1 MB | 29.7 MB | 920.8 MB | 100.2 % |
| c0 | 1,044.0 MB | 17.0 MB | 35.3 MB | 1,097.4 MB | 119.4 % |
| c1 | 1,044.0 MB | 17.0 MB | 71.8 MB | 1,134.0 MB | 123.4 % |
| c2 | 1,044.0 MB | 17.0 MB | 35.3 MB | 1,097.4 MB | 119.4 % |
| d0 | 969.8 MB | 15.8 MB | 32.8 MB | 1,019.8 MB | 111.0 % |
| d2 | 960.5 MB | 15.7 MB | 32.6 MB | 1,010.2 MB | 109.9 % |

"Index" is everything that locates a line: his sparse index and block sizes for a; page headers and the page index for
the others. The grammar and code tables are about 1.2 MB in every variant.

## Warm lookups, one thread

| Variant | Sequential, mean | Uniform, mean | Game trace, p50 | Game trace, mean | Positions end to end, p50 |
|---|---|---|---|---|---|
| a | 186 ns | 1,086 ns | 547 ns | 740 ns | 828 ns |
| b | 190 ns | 1,172 ns | 612 ns | 660 ns | 947 ns |
| c0 | 118 ns | 778 ns | 280 ns | 344 ns | 579 ns |
| c1 | 118 ns | 829 ns | 261 ns | 341 ns | 591 ns |
| c2 | 126 ns | 939 ns | 271 ns | 346 ns | 576 ns |
| d0 | 146 ns | 698 ns | 273 ns | 330 ns | 555 ns |
| d2 | 136 ns | 804 ns | 285 ns | 357 ns | 575 ns |

The first three columns time the table lookup alone (index to value). "End to end" is Fathom's whole WDL probe of a
position, with its own position indexing and capture search, and only the table lookup replaced.

With four threads on the game trace: a 6.0, b 6.1, c0 12.5, c1 12.2, c2 11.7, d0 11.9, d2 11.5 million lookups per
second.

## The comparisons

**a against b: the page layout.** Putting his codes in self-contained 4 KiB pages costs 0.2 % in size. The lookup is 2 %
to 12 % slower than Fathom's own. His sparse index and the page index come out about level; both are small next to the
Huffman decode.

**b against c0: Huffman or fixed 12-bit codes.** This is the one large effect. Dropping Huffman costs 19 % in size and
halves the lookup on the game trace (612 to 280 ns). End to end the probe goes from 947 to 579 ns.

**c0, c1, c2: the page header.** c0 and c2 are the same size to the byte, since no Elias-Fano header needed a third
line with his 256-position symbols. c1 is 3.3 % larger. Over six runs the game-trace p50 was 267 to 312 ns for c0, 260
to 278 ns for c1 and 271 to 297 ns for c2 (`results/repeat_trace.txt`), so the headers cannot be told apart by speed.
By the rule fixed beforehand (size first, then warm p50 on the game trace), c1 is out on size and c0 and c2 tie. c0 was
taken as the simpler of the two.

**c0 against d: the grammar.** A grammar rebuilt for fixed-width codes takes the cost from 119.4 % to 111.0 % (d0) or
109.9 % (d2) of the Syzygy size, with lookup times that match c0 within noise. Allowing symbols of up to 65,536
positions instead of 1,560 gains one more point and makes 2,173 pages use a third header line.

## Cold lookups

| Variant | Uniform: mean | p50 | p99 | Game trace: mean | p99 |
|---|---|---|---|---|---|
| a | 31.2 us | 37.4 us | 120.0 us | 8.8 us | 95.0 us |
| b | 27.6 us | 37.5 us | 88.3 us | 8.1 us | 59.0 us |
| c0 | 27.8 us | 31.3 us | 81.5 us | 8.3 us | 59.0 us |
| c1 | 29.4 us | 36.9 us | 89.9 us | 8.3 us | 60.7 us |
| c2 | 31.7 us | 36.9 us | 92.2 us | 8.8 us | 68.4 us |
| d0 | 24.8 us | 36.1 us | 73.5 us | 7.9 us | 59.3 us |
| d2 | 26.0 us | 30.7 us | 82.8 us | 7.6 us | 58.3 us |

After dropping the page cache, one thread, 200,000 lookups. A cold lookup is one page read in every layout and the
means do not differ beyond noise. The page layouts have a lower p99 than the Syzygy file (about 60 to 90 us against 95
to 120 us), which fits a lookup there touching the index, the block sizes and the data in three places of one file.

All data files are mapped with read-ahead off, as Fathom maps its files. With the kernel's default read-ahead the page
layouts looked three times faster cold; those rows are not used.

## Limits

- Cold numbers describe this machine's virtual disk, which answers a random 4 KiB read in about 30 us. They say little
  about a real disk.
- Four cores. Timings are single runs; where repeated, the p50 moved by up to 15 %.
- The game trace: 4 million positions from random play that starts in one of the three 6-piece materials and follows
  captures into the dependent tables, each position with all its children; 6.1 million table lookups. It is not a
  search log of an engine.
- d starts at a disadvantage. His grammar was built together with the choice of values for the positions that do not
  matter; d is rebuilt from his finished values. For sub-tables above about 2^29 positions d builds its grammar on a
  sample and applies it in chunks of 2^22 positions, since this machine cannot hold a 6-piece sub-table in memory.
- b cannot always cut a symbol at a subdivision. Some of his symbols occur only inside other symbols and have no
  Huffman code. At 2^31 positions per subdivision all 68 cuts were possible. With subdivisions shrunk to 2^17 as a
  stress test, c0, c1 and c2 pass on the full set, b fails on one sub-table (KQRvKP) where a single value has no code.
  For d that stress test was run on nine small tables only.
- A sub-table takes at least one 4 KiB page. On the 43 sub-tables below 4 KiB in the Syzygy file the page layouts are
  2.5 to 6 times larger. That is 0.4 MB in all here, but on the complete set of 3 to 5 pieces, where small and highly
  compressible tables dominate, c0 is 143 % of the Syzygy size.
- Subdivisions of 2^15 positions with pages confined to them (so that all in-page numbers fit 16 bits) were measured
  on the 5-piece set and dropped: 4,289 MB against 566 MB, because most subdivisions hold far less than one page.

## Machine and versions

4 virtual cores of an Intel Xeon at 2.1 GHz (L2 8 MiB), 15 GB RAM, Linux 6.18.44, page size 4,096, a virtual disk
(`vda`), g++ 13.3 with `-O3 -march=native`.

syzygy1/tb at `0bb8aeee525f364bb750f96df312a1a7c9b54398`, built as its Makefile says (`-O3 -march=native -flto`,
`-DBMI2 -DUSE_POPCNT -DTBPIECES=7`); tables made with `rtbgen -w -t 4` and `rtbgenp -w -t 4`. Fathom at
`c9c6fef0dddc05d2e242c183acf5833149ab676d`, used as a patched copy. The generated and downloaded files are never
written to.

## Reproducing

`scripts/setup.sh` clones and builds both at those commits and builds the tool. `scripts/generate.sh` makes the tables
of 3 to 5 pieces. `scripts/run.sh` converts, checks and measures a directory of tables (`TB`, `VARIANTS`, `MATERIALS`,
`REACHED`, `STEPS`, `CACHES`), and `scripts/report.py` prints the tables from its CSV files. The three 6-piece files
were downloaded and checked with `tbcheck`.
