# Syzygy style builds for 6×6 international draughts

`syzygy.cpp` builds endgame databases in the manner of R. de Man's `rtbgen` (the generator of the Syzygy tablebases
for chess), adapted to draughts and to the design notes in
[`literature/egdb_primitives.md`](../literature/egdb_primitives.md). It shares its metrics, materials, index, groups
and unmoves with [`thompson86`](../thompson86/) (`src/materials_6x6.hpp`); what differs is the sidedness and the
passes.

| | `thompson86` | `syzygy` |
|---|---|---|
| sides | one: White-to-move wins (`.w`), Black-to-move losses (`.l`) | two: both sides to move, wins, draws and losses |
| materials (≤ 6 pieces) | 155: every material and its colour mirror | 82: one of each mirror pair, the other looked up as its mirror image |
| per round | bit planes: the new wins and losses as frontiers | byte arrays scanned in full for the values of the previous ply, as `rtbgen` |
| losses | candidates verified forward (1996 style: masked late) | `rtbgen`'s `check_loss` on every predecessor of a new win |
| WDL | not stored (one-sided) | a by-product: `.wtm.wdl` and `.btm.wdl`, two bits per position |

## What is stored

Per material, named wp-bp-wm-bm (white pieces, black pieces, white men, black men, two digits each):

| file | contents |
|---|---|
| `6x6-wp-bp-wm-bm.<metric>.wtm` | one byte per slot, White to move |
| `6x6-wp-bp-wm-bm.<metric>.btm` | one byte per slot, Black to move |
| `6x6-wp-bp-wm-bm.<metric>.wtm.wdl` | two bits per slot (0 draw, 1 loss, 2 win), White to move |
| `6x6-wp-bp-wm-bm.<metric>.btm.wdl` | two bits per slot, Black to move |

The value for the side to move: 0 a draw (or an illegal slot), 2n a win in n plies, 2n + 1 a loss in n plies (n = 0:
no move). Exact plies, 16 bits during the build, one byte stored (up to 127 plies). The three metrics are those of
`thompson86`:

| metric | the count ends at |
|---|---|
| `dtm` | the end of the game |
| `dtc` | the end, or a capture or promotion |
| `dtz` | the end, or a capture or man move |

**WDL as a by-product.** The `.wdl` files are the sign of every value, the same for all three metrics, next to the
byte files of the same side to move (slot i in bits 2(i mod 4) of byte ⌊i / 4⌋). They are all that dtc and dtz need
from the materials that a move leads into: every move out of a material is a goal move there, after which only the
outcome counts. So their Snakemake rules read only the `.wdl` files of the captures and promotions; dtm reads `.wtm`
and `.btm`, because it needs the depths. A dtz build gives WDL and DTZ together, as the Syzygy tablebases do.

## Passes

Captures and promotions (and in dtz man moves, which end the count) are looked up forward in pass 0, in finished
files: no uncaptures. A dtm or dtc material is one group, with king moves and man moves inside; a dtz material is
solved one distance at a time (as `thompson86`'s 1996 style), with only king moves inside.

* **pass 0**: every position looks up its moves out of the group. Without a move inside it is decided at once
  (captures are compulsory); otherwise it keeps its fastest win and its slowest loss through an exit (no loss if an
  exit draws or wins).
* **round n, losses in n**: the array is scanned for wins in n − 1; each unmoves to its predecessors, and each
  undecided predecessor whose exits lose by n is checked forward (`check_loss`): lost iff every move inside leads to a
  win in fewer than n plies. Also every undecided position whose exits lose in exactly n.
* **round n, wins in n**: the array is scanned for losses in n − 1; their undecided predecessors win in n, and so does
  every undecided position whose exit wins in n.

The losses of a round read only the wins of earlier rounds, and the wins only the losses of earlier rounds, so each
half is one `std::for_each` over blocks of positions, `seq` or `par` (every writer writes the same value, through
relaxed `std::atomic_ref`): the files are the same either way. Positions decided in pass 0 can have any depth; the
scan finds them as a frontier at their own level, and `check_loss` must not count a win deeper than n − 1.

Disk, as rule 6: the group's arrays in RAM; finished materials read through read-only memory maps; each file written
once, sequentially, to a temporary name and then renamed.

## Build and run

```bash
g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include syzygy/syzygy.cpp -lzstd -ltbb -o build/syzygy
```

```bash
snakemake -s syzygy/Snakefile --cores 4 --config engine=build/syzygy dir=data/syzygy pieces=6 metric=dtz
```

or with CMake, target `syzygy`. `policy=par` runs each build with `std::execution::par`. The last Snakemake job
(`checkall`) builds `egdb_6x6`'s databases in memory and compares every position of every material, both sides to
move: dtm exactly, dtc and dtz their wins and losses; every value against its recomputation from the successors; the
`.wdl` files against the values. Further checks:

* `syzygy cmp86 <dir> <dir_thompson86> n <metric>`: for every material with at most n pieces, both colours, the
  `thompson86` files (`.w` the White-to-move wins, `.l` the Black-to-move losses) against the two-sided values;
* `syzygy order <dtz> <dtc> <dtm> n`: dtz ≤ dtc ≤ dtm for every win and every loss, and the same WDL and `.wdl` files
  in all three.

## Results (6 pieces, 4 cores)

82 materials, 53,366,312 positions (both sides to move), 2 × 53.8 MB of bytes and 2 × 13.5 MB of `.wdl` per metric;
21,650,623 wins and 11,988,249 losses (both sides to move).

| metric | wall time, builds (4 jobs, `seq`) | CPU summed | wall time, one job at a time, `par` | longest win | longest loss |
|---|---|---|---|---|---|
| dtm | 15.5 s | 44.8 s | 16.5 s | 69 plies (Black to move, 02-04-00-04) | 68 plies (White to move, 02-04-00-04) |
| dtc | 12.9 s | 34.8 s | 13.0 s | 43 plies (Black to move, 02-03-01-02) | 42 plies (White to move, 02-03-01-02) |
| dtz | 9.2 s | 28.8 s | 11.7 s | 27 plies (White to move, 03-03-01-02) | 28 plies (Black to move, 03-03-01-02) |

* `checkall`: every material checks out for all three metrics;
* `cmp86`: all 97,762,498 positions of `thompson86`'s 155 materials agree, for all three metrics: the two-sided
  values contain the one-sided files exactly;
* `order`: dtz ≤ dtc ≤ dtm for every win and loss; the `.wdl` files of the three metrics are identical;
* `seq` and `par` give identical files.

**Against `thompson86` and `egdb_6x6`** (dtm, 4 cores):

| | positions | CPU | per position | wall time |
|---|---|---|---|---|
| `egdb_6x6 build 6` (two-sided, slices, leapfrog) | 51,237,726 | 22.6 s | 0.44 µs | 6.6 s |
| `thompson86` (one-sided, bit planes, 1986 style) | 97,762,498 | 34.3 s | 0.35 µs | 10.0 s |
| `syzygy` (two-sided, scans) | 53,366,312 | 44.8 s | 0.84 µs | 15.5 s |

Two-sidedness halves the positions, but for dtm the scans cost more than that saves: every round scans the whole
group twice, and a dtm material is one group with up to 69 rounds. Frontier bit planes (as in `thompson86` and
`egdb_6x6`) visit only the positions of the previous ply. The dtz waves have few rounds each, so there the scans are
cheap: 28.8 s of CPU against 31.5 s for `thompson86`'s dtz (1986 style), which builds twice the positions. (`egdb_6x6` stores a symmetric material with White to move
only, hence its slightly smaller count.)
