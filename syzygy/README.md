# Syzygy style builds for 6×6 international draughts

`syzygy.cpp` builds endgame databases in the manner of R. de Man's `rtbgen` (the generator of the Syzygy tablebases
for chess), adapted to draughts and to the design notes in
[`literature/egdb_primitives.md`](../literature/egdb_primitives.md). It shares its metrics, materials, index, groups
and unmoves with [`thompson86`](../thompson86/) (`src/materials_6x6.hpp`); what differs is the sidedness and the
passes.

| | `thompson86` | `syzygy` |
|---|---|---|
| sides | one, half results: White-to-move wins (`wtm.dtm-W`), Black-to-move losses (`btm.dtm-L`) | two, full results: both sides to move, wins, draws and losses (`wtm.dtm`, `btm.dtm`) |
| materials (≤ 6 pieces) | 155: every material and its colour mirror | 82: one of each mirror pair, the other looked up as its mirror image |
| per round | bit planes: the new wins and losses as frontiers | the same frontiers (default), or as `rtbgen` full scans of the byte array for the values of the previous ply (`rounds=scan`) |
| losses | candidates verified forward (1996 style: masked late) | `rtbgen`'s `check_loss` on every predecessor of a new win |
| outcomes, a by-product | half: `wtm.bit-W`, `btm.bit-L`, one bit per position | full: `wtm.wdl`, `btm.wdl`, two bits per position |

## What is stored

Per material, named wp-bp-wm-bm (white pieces, black pieces, white men, black men, two digits each):

| file | contents |
|---|---|
| `6x6-wp-bp-wm-bm.wtm.dtm` | one byte per slot, White to move: the unsigned depth |
| `6x6-wp-bp-wm-bm.btm.dtm` | one byte per slot, Black to move: the unsigned depth |
| `6x6-wp-bp-wm-bm.wtm.wdl` | two bits per slot (0 draw, 1 loss, 2 win), White to move |
| `6x6-wp-bp-wm-bm.btm.wdl` | two bits per slot, Black to move |

Shown for dtm; dtc and dtz alike (`wtm.dtc`, ...). These are **full results** (win, draw and loss), where
`thompson86` stores **half results** (`dtm-W`, `dtm-L`, `bit-W`, `bit-L`). The `wdl` files carry no metric: they are
the same for all three, so the files of each metric go into a directory of their own, `<dir>/<metric>`.

The value for the side to move, in the build: 0 a draw (or an illegal slot), 2n a win in n plies, 2n + 1 a loss in n
plies (n = 0: no move), 16 bits. **Stored: an unsigned byte, its sign in the `wdl` file** of the same side to move:

| | byte | range in one byte |
|---|---|---|
| dtm | k: a win in 2k − 1 plies, or a loss in 2k plies (0: lost in 0 plies, no move) | 509 plies of win, 510 of loss |
| dtc, dtz | n: a win or a loss in n plies | 255 plies |

A dtm win always takes an odd and a dtm loss an even number of plies, so dtm stores moves; in dtc and dtz a goal move
by the losing side can end a win on an even ply, so they store plies. A draw is 0 in both files. The two-sided files
reach the range of `thompson86`'s half results, which need no sign since each holds only wins or only losses. A depth
probe reads the `wdl` bits too (search reads them first anyway). Exact plies either way. The three metrics are those
of `thompson86`:

| metric | the count ends at |
|---|---|
| `dtm` | the end of the game |
| `dtc` | the end, or a capture or promotion |
| `dtz` | the end, or a capture or man move |

**WDL as a by-product.** The `wdl` files are the sign of every value, the same for all three metrics, next to the
byte files of the same side to move (slot i in bits 2(i mod 4) of byte ⌊i / 4⌋). They are all that dtc and dtz need
from the materials that a move leads into: every move out of a material is a goal move there, after which only the
outcome counts. So their Snakemake rules read only the `wdl` files of the captures and promotions; dtm reads `wtm.dtm`
and `btm.dtm`, because it needs the depths. A dtz build gives WDL and DTZ together, as the Syzygy tablebases do.

## Passes

Captures and promotions (and in dtz man moves, which end the count) are looked up forward in pass 0, in finished
files: no uncaptures. A dtm or dtc material is one group, with king moves and man moves inside; a dtz material is
solved one distance at a time (as `thompson86`'s 1996 style), with only king moves inside.

* **pass 0**: every position looks up its moves out of the group. Without a move inside it is decided at once
  (captures are compulsory); otherwise it keeps its fastest win and its slowest loss through an exit (no loss if an
  exit draws or wins).
* **round n, losses in n**: every win in n − 1 unmoves to its predecessors, and each
  undecided predecessor whose exits lose by n is checked forward (`check_loss`): lost iff every move inside leads to a
  win in fewer than n plies. Also every undecided position whose exits lose in exactly n.
* **round n, wins in n**: the undecided predecessors of the losses in n − 1 win in n, and so does every undecided
  position whose exit wins in n.

Two ways to find the wins and losses of the previous ply, with byte-identical files:

* **frontier** (default): bit planes of the wins and losses found in the previous round, and lists by level of the
  positions decided in pass 0 and of the exit seeds. A round visits only these.
* **scan** (`rounds=scan`), as `rtbgen`: every round scans the whole byte array twice, once for the wins and once for
  the losses of the previous ply.

The losses of a round read only the wins of earlier rounds, and the wins only the losses of earlier rounds, so each
half is one `std::for_each` over blocks of positions, `seq` or `par` (every writer writes the same value, through
relaxed `std::atomic_ref`, the frontier bits through `fetch_or`): the files are the same either way. Before the passes
a build opens every file it can read and fills the index memo, so the parallel passes only read those maps. Positions
decided in pass 0 can have any depth; they are a frontier at their own level, and `check_loss` must not count a win
deeper than n − 1.

Disk, as rule 6: the group's arrays in RAM; finished materials read through read-only memory maps; each file written
once, sequentially, to a temporary name and then renamed.

## Build and run

```bash
g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include syzygy/syzygy.cpp -lzstd -ltbb -o build/syzygy
```

```bash
snakemake -s syzygy/Snakefile --cores 4 --config engine=build/syzygy dir=data/syzygy pieces=6 metric=dtz
```

or with CMake, target `syzygy`. `policy=par` runs each build with `std::execution::par`; `rounds=scan` finds the
frontiers by full scans. The last Snakemake job
(`checkall`) builds `egdb_6x6`'s databases in memory and compares every position of every material, both sides to
move: dtm exactly, dtc and dtz their wins and losses; every value against its recomputation from the successors; the
`wdl` files against the values. Further checks:

* `syzygy cmp86 <dir> <dir_thompson86> n <metric>`: for every material with at most n pieces, both colours, the
  `thompson86` files (`dtm-W` the White-to-move wins, `dtm-L` the Black-to-move losses) against the two-sided values;
* `syzygy order <dtz> <dtc> <dtm> n`: dtz ≤ dtc ≤ dtm for every win and every loss, and the same WDL and `wdl` files
  in all three.

**Memory-aware scheduling** (synthesis §7.3, step 1): `syzygy plan n <metric>` prints each stored material's sizes and
the MB a build needs (only rung 0 exists), calibrated against measured peaks; the Snakefile turns it into job claims
(`resources: mem_mb`) and critical-path priorities, as in `thompson86`, and a build over its claim says so in its log,
with its peak RSS. At 6 pieces with an 80 MB budget: 81 materials fit, 03-03-01-01 (88 MB) runs over its claim; files
byte-identical to the unbudgeted build, no peak above its estimate.

## Results (6 pieces, 4 cores)

82 materials, 53,366,312 positions (both sides to move), 2 × 53.8 MB of bytes and 2 × 13.5 MB of `wdl` per metric;
21,650,623 wins and 11,988,249 losses (both sides to move).

| metric | frontier: wall time, builds (4 jobs, `seq`) | frontier: CPU summed | scan: wall time | scan: CPU summed | longest win | longest loss |
|---|---|---|---|---|---|---|
| dtm | 11.8 s | 36.4 s | 18.0 s | 53.1 s | 69 plies (Black to move, 02-04-00-04) | 68 plies (White to move, 02-04-00-04) |
| dtc | 10.7 s | 33.3 s | 13.0 s | 39.1 s | 43 plies (Black to move, 02-03-01-02) | 42 plies (White to move, 02-03-01-02) |
| dtz | 9.5 s | 28.0 s | 10.1 s | 32.0 s | 27 plies (White to move, 03-03-01-02) | 28 plies (Black to move, 03-03-01-02) |

The two modes were timed back to back; a second frontier run gave 36.3, 32.6 and 27.1 s of CPU. One job at a time
with `par` takes 13–16 s wall: the small materials do not keep four threads busy.

* `checkall`: every material checks out for all three metrics;
* `cmp86`: all 97,762,498 positions of `thompson86`'s 155 materials agree, for all three metrics: the two-sided
  values contain the one-sided files exactly;
* `order`: dtz ≤ dtc ≤ dtm for every win and loss; the `wdl` files of the three metrics are identical;
* `frontier` and `scan` give identical files, and so do `seq` and `par`.

**Against `thompson86` and `egdb_6x6`** (dtm, 4 cores):

| | positions | CPU | per position | wall time |
|---|---|---|---|---|
| `egdb_6x6 build 6` (two-sided, slices, leapfrog) | 51,237,726 | 22.6 s | 0.44 µs | 6.6 s |
| `thompson86` (one-sided, bit planes, 1986 style) | 97,762,498 | 34.3 s | 0.35 µs | 10.0 s |
| `syzygy`, frontier (two-sided, bit planes) | 53,366,312 | 36.4 s | 0.68 µs | 11.8 s |
| `syzygy`, scan (two-sided, full scans) | 53,366,312 | 53.1 s | 1.00 µs | 18.0 s |

The scans cost dtm a third of its time: a dtm material is one group with up to 69 rounds, and every round scans the
whole group twice. With frontiers `syzygy` is level with `thompson86` for dtm and ahead for dtz. A position here
settles both colours, so per settled cell (positions × colours) the frontier build costs 0.34 µs for dtm and 0.26 µs
for dtz, against `thompson86`'s 0.35 and 0.32. (`egdb_6x6` stores a symmetric material with White to move only,
hence its slightly smaller count.)
