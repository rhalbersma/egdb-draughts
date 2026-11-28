## Sizes

230 sub-tables in 54 tables, 21 of them a single constant; 37,492,099,680 index values in all.

### All tables

| Variant | Coded data | Padding | Index | Grammar and code tables | Total | Against a |
|---|---|---|---|---|---|---|
| a: Syzygy as is | 880.76 | 5.03 | 31.91 | 1.15 | 918.86 MB | 100.0 % |
| b: Huffman, u16 pages | 880.76 | 9.09 | 29.73 | 1.17 | 920.75 MB | 100.2 % |
| c0: 12-bit, u16 pages | 1043.99 | 17.01 | 35.30 | 1.14 | 1097.44 MB | 119.4 % |
| c1: 12-bit, u32 pages | 1043.99 | 16.99 | 71.84 | 1.14 | 1133.96 MB | 123.4 % |
| c2: 12-bit, Elias-Fano pages | 1043.99 | 17.01 | 35.30 | 1.14 | 1097.44 MB | 119.4 % |
| d0: rebuilt grammar, u16 pages | 969.84 | 15.81 | 32.79 | 1.31 | 1019.76 MB | 111.0 % |
| d2: rebuilt grammar, Elias-Fano pages | 960.51 | 15.71 | 32.62 | 1.38 | 1010.22 MB | 109.9 % |

### Only the 46 sub-tables of at least 1 MiB in the Syzygy file (page rounding plays no part)

| Variant | Coded data | Padding | Index | Grammar and code tables | Total | Against a |
|---|---|---|---|---|---|---|
| a: Syzygy as is | 859.99 | 4.81 | 31.04 | 0.57 | 896.40 MB | 100.0 % |
| b: Huffman, u16 pages | 859.99 | 8.42 | 28.89 | 0.57 | 897.88 MB | 100.2 % |
| c0: 12-bit, u16 pages | 1002.88 | 16.03 | 33.90 | 0.57 | 1053.37 MB | 117.5 % |
| c1: 12-bit, u32 pages | 1002.88 | 16.03 | 68.99 | 0.57 | 1088.46 MB | 121.4 % |
| c2: 12-bit, Elias-Fano pages | 1002.88 | 16.03 | 33.90 | 0.57 | 1053.37 MB | 117.5 % |
| d0: rebuilt grammar, u16 pages | 946.14 | 15.13 | 31.98 | 0.57 | 993.82 MB | 110.9 % |
| d2: rebuilt grammar, Elias-Fano pages | 940.70 | 15.05 | 31.88 | 0.57 | 988.19 MB | 110.2 % |

### Only the 43 sub-tables below 4 KiB in the Syzygy file (each takes at least one page here)

| Variant | Coded data | Padding | Index | Grammar and code tables | Total | Against a |
|---|---|---|---|---|---|---|
| a: Syzygy as is | 0.06 | 0.00 | 0.00 | 0.01 | 0.08 MB | 100.0 % |
| b: Huffman, u16 pages | 0.06 | 0.11 | 0.01 | 0.01 | 0.19 MB | 248.6 % |
| c0: 12-bit, u16 pages | 0.32 | 0.09 | 0.01 | 0.01 | 0.44 MB | 571.4 % |
| c1: 12-bit, u32 pages | 0.32 | 0.09 | 0.03 | 0.01 | 0.44 MB | 582.1 % |
| c2: 12-bit, Elias-Fano pages | 0.32 | 0.09 | 0.01 | 0.01 | 0.44 MB | 571.4 % |
| d0: rebuilt grammar, u16 pages | 0.10 | 0.07 | 0.01 | 0.02 | 0.20 MB | 263.5 % |
| d2: rebuilt grammar, Elias-Fano pages | 0.04 | 0.13 | 0.01 | 0.03 | 0.20 MB | 265.7 % |

### Details

| Variant | Codes | Pages | Lines of 64 / 32 / 16 bytes (sub-tables) | Pages with 3 / 4 header lines | Symbols split at subdivisions (a: at his block ends) |
|---|---|---|---|---|---|
| a: Syzygy as is | 695,993,413 | 0 | 149 / 60 / 0 | 0 / 0 | 1143430 |
| b: Huffman, u16 pages | 695,993,481 | 224,288 | 149 / 60 / 0 | 0 / 0 | 68 |
| c0: 12-bit, u16 pages | 695,993,481 | 267,388 | 209 / 0 / 0 | 0 / 0 | 68 |
| c1: 12-bit, u32 pages | 695,993,481 | 276,297 | 209 / 0 / 0 | 0 / 0 | 68 |
| c2: 12-bit, Elias-Fano pages | 695,993,481 | 267,388 | 209 / 0 / 0 | 0 / 0 | 68 |
| d0: rebuilt grammar, u16 pages | 646,966,725 | 248,402 | 209 / 0 / 0 | 0 / 0 | 0 |
| d2: rebuilt grammar, Elias-Fano pages | 640,404,859 | 246,058 | 209 / 0 / 0 | 2173 / 0 | 0 |

## Probe performance

### Sequential indices, index to value

| Variant | Warm, 1 thread: mean | p50 | p90 | p99 | Warm, 4 threads | Cold, 1 thread: mean | p50 | p99 | Cold, 4 threads |
|---|---|---|---|---|---|---|---|---|---|
| a: Syzygy as is | 186 ns | 190 ns | 303 ns | 440 ns | 21.53 M/s | 0.3 us | 0.2 us | 0.5 us | 8.87 M/s |
| b: Huffman, u16 pages | 190 ns | 202 ns | 302 ns | 461 ns | 16.40 M/s | 0.3 us | 0.2 us | 0.5 us | 11.36 M/s |
| c0: 12-bit, u16 pages | 118 ns | 138 ns | 162 ns | 193 ns | 34.97 M/s | 0.2 us | 0.2 us | 0.3 us | 14.29 M/s |
| c1: 12-bit, u32 pages | 118 ns | 148 ns | 173 ns | 207 ns | 30.14 M/s | 0.2 us | 0.2 us | 0.3 us | 16.51 M/s |
| c2: 12-bit, Elias-Fano pages | 126 ns | 155 ns | 186 ns | 230 ns | 29.77 M/s | 0.2 us | 0.2 us | 0.2 us | 15.32 M/s |
| d0: rebuilt grammar, u16 pages | 146 ns | 153 ns | 190 ns | 240 ns | 28.46 M/s | 2.0 us | 0.1 us | 0.2 us | 10.11 M/s |
| d2: rebuilt grammar, Elias-Fano pages | 136 ns | 164 ns | 193 ns | 232 ns | 29.29 M/s | 0.3 us | 0.2 us | 0.3 us | 14.52 M/s |

Checksums of the values returned: identical for all variants.

### Uniform random indices, index to value

| Variant | Warm, 1 thread: mean | p50 | p90 | p99 | Warm, 4 threads | Cold, 1 thread: mean | p50 | p99 | Cold, 4 threads |
|---|---|---|---|---|---|---|---|---|---|
| a: Syzygy as is | 1086 ns | 1013 ns | 1417 ns | 1967 ns | 4.10 M/s | 31.2 us | 37.4 us | 120.0 us | 0.19 M/s |
| b: Huffman, u16 pages | 1172 ns | 1117 ns | 1573 ns | 2279 ns | 3.50 M/s | 27.6 us | 37.5 us | 88.3 us | 0.21 M/s |
| c0: 12-bit, u16 pages | 778 ns | 781 ns | 1121 ns | 1853 ns | 5.24 M/s | 27.8 us | 31.3 us | 81.5 us | 0.20 M/s |
| c1: 12-bit, u32 pages | 829 ns | 800 ns | 1113 ns | 1939 ns | 5.18 M/s | 29.4 us | 36.9 us | 89.9 us | 0.19 M/s |
| c2: 12-bit, Elias-Fano pages | 939 ns | 827 ns | 1135 ns | 1871 ns | 5.12 M/s | 31.7 us | 36.9 us | 92.2 us | 0.20 M/s |
| d0: rebuilt grammar, u16 pages | 698 ns | 711 ns | 975 ns | 1438 ns | 6.01 M/s | 24.8 us | 36.1 us | 73.5 us | 0.23 M/s |
| d2: rebuilt grammar, Elias-Fano pages | 804 ns | 816 ns | 1122 ns | 1696 ns | 5.41 M/s | 26.0 us | 30.7 us | 82.8 us | 0.23 M/s |

Checksums of the values returned: identical for all variants.

### Game-related probes, index to value

| Variant | Warm, 1 thread: mean | p50 | p90 | p99 | Warm, 4 threads | Cold, 1 thread: mean | p50 | p99 | Cold, 4 threads |
|---|---|---|---|---|---|---|---|---|---|
| a: Syzygy as is | 740 ns | 547 ns | 1204 ns | 3712 ns | 6.00 M/s | 8.8 us | 0.6 us | 95.0 us | 0.47 M/s |
| b: Huffman, u16 pages | 660 ns | 612 ns | 1263 ns | 1884 ns | 6.07 M/s | 8.1 us | 0.8 us | 59.0 us | 0.57 M/s |
| c0: 12-bit, u16 pages | 344 ns | 280 ns | 696 ns | 1098 ns | 12.51 M/s | 8.3 us | 0.4 us | 59.0 us | 0.49 M/s |
| c1: 12-bit, u32 pages | 341 ns | 261 ns | 698 ns | 1071 ns | 12.21 M/s | 8.3 us | 0.4 us | 60.7 us | 0.50 M/s |
| c2: 12-bit, Elias-Fano pages | 346 ns | 271 ns | 736 ns | 1138 ns | 11.66 M/s | 8.8 us | 0.4 us | 68.4 us | 0.50 M/s |
| d0: rebuilt grammar, u16 pages | 330 ns | 273 ns | 643 ns | 990 ns | 11.93 M/s | 7.9 us | 0.4 us | 59.3 us | 0.59 M/s |
| d2: rebuilt grammar, Elias-Fano pages | 357 ns | 285 ns | 735 ns | 1140 ns | 11.54 M/s | 7.6 us | 0.4 us | 58.3 us | 0.55 M/s |

Checksums of the values returned: identical for all variants.

### Game-related positions, end to end

| Variant | Warm, 1 thread: mean | p50 | p90 | p99 | Warm, 4 threads | Cold, 1 thread: mean | p50 | p99 | Cold, 4 threads |
|---|---|---|---|---|---|---|---|---|---|
| a: Syzygy as is | 1123 ns | 828 ns | 2155 ns | 3855 ns | 3.39 M/s | 12.4 us | 1.2 us | 101.8 us | 0.40 M/s |
| b: Huffman, u16 pages | 1246 ns | 947 ns | 2484 ns | 4387 ns | 3.07 M/s | 11.4 us | 1.3 us | 76.7 us | 0.44 M/s |
| c0: 12-bit, u16 pages | 712 ns | 579 ns | 1488 ns | 2707 ns | 6.15 M/s | 13.6 us | 1.0 us | 86.7 us | 0.32 M/s |
| c1: 12-bit, u32 pages | 724 ns | 591 ns | 1540 ns | 2953 ns | 5.59 M/s | 11.6 us | 0.9 us | 75.4 us | 0.38 M/s |
| c2: 12-bit, Elias-Fano pages | 749 ns | 576 ns | 1492 ns | 2596 ns | 5.73 M/s | 11.8 us | 0.9 us | 78.1 us | 0.35 M/s |
| d0: rebuilt grammar, u16 pages | 724 ns | 555 ns | 1425 ns | 2618 ns | 5.53 M/s | 10.5 us | 0.8 us | 71.2 us | 0.42 M/s |
| d2: rebuilt grammar, Elias-Fano pages | 716 ns | 575 ns | 1483 ns | 2597 ns | 5.71 M/s | 10.5 us | 0.8 us | 71.2 us | 0.47 M/s |

Checksums of the values returned: identical for all variants.

