# Thompson style builds (1986 and 1996) for 6×6 international draughts

`thompson86.cpp` builds endgame databases in the manner of K. Thompson, "Retrograde analysis of certain endgames",
ICCA Journal 9(3), 1986, and "6-piece endgames", ICCA Journal 19(4), 1996, adapted to draughts and to the design
notes in [`literature/egdb_primitives.md`](../literature/egdb_primitives.md). Its metrics, materials, index, groups
and unmoves are in `src/materials_6x6.hpp`, shared with the two-sided [`syzygy`](../syzygy/) builder.

**Four files per material** (shown for dtm; dtc and dtz alike), all one-sided (White's wins and Black's losses):

| file | side to move | per position | value | 0 means |
|---|---|---|---|---|
| `dmW-wtm` | White | one byte | k: a White win in 2k − 1 plies (dcW, dzW: n, a win in n plies) | not a win (draw, loss or illegal slot) |
| `dmL-btm` | Black | one byte | k: Black lost in 2(k − 1) plies (dcL, dzL: n + 1, lost in n plies; a loss can take 0 plies: Black has no move) | not lost |
| `isW-wtm` | White | one bit | 1: White wins | not a win |
| `isL-btm` | Black | one bit | 1: Black is lost | not lost |

**Exact plies, compactly.** A dtm win always takes an odd number of plies and a dtm loss an even number, so dtm stores
moves, Thompson's "win in k": up to 509 plies in a byte. In dtc and dtz a goal move by the losing side (Black's forced
capture) can end a White win on an even ply, so they store plies. The build works in plies; only the files differ.

The notation, shared with [`syzygy`](../syzygy/): a file is `6x6/np/wp-bp/wm-bm-<kind>-<side to move>`. A **full result**
holds win, draw and loss: `dtm` (with depth) or `wdl` (outcome only). A **half result** holds one of them: `dmW`,
`dmL` (with depth), `isW`, `isL` (outcome only, one bit). The same for dtc and dtz (`dcW`, `dcL`, `dzW`, `dzL`); the bit files carry no
metric, since they are the same for all three, so the files of each metric go into a directory of their own,
`<dir>/<metric>`. A one-sided build stores half results: White's wins with White to move, Black's losses with Black to
move.

**Why depths, and why bits.** `dmW` is the database: the dtm, dtc or dtz of every White-to-move win, which you need to
play the win out. `dmL` holds the Black-to-move loss depths, which a dtm build needs (a White capture wins in 1 + the
loss depth of the position it leads to) and which you need to play the defence. The bit files are the one-sided WDL,
a by-product of the bytes. They are all that dtc and dtz need from the materials that a move leads into: every move
out of a material is a goal move there, after which only the outcome counts. So the dtc and dtz Snakemake rules read
only the bit files of the captures and promotions; dtm reads the bytes.

The `dmL` file is Thompson 1996's RES, the Black-to-move loss distances. With it every lookup is a single read: after
a White move into `dmL`, after a Black move into `dmW`, without a ply of search. Three metrics, which differ only in
the moves that end the count (by either side, as long as the win is kept):

| metric | the count ends at |
|---|---|
| `dtm` | the end of the game (Black has no move) |
| `dtc` | the end, or a capture or promotion: a conversion into another material |
| `dtz` | the end, or a capture or man move: a zeroing move of the 25-move rule |

A White win can end with Black's forced capture, so in dtc and dtz a win can take an even number of plies.

**One file pair per material.** A material is named wp-bp-wm-bm (white pieces, black pieces, white men, black men,
two digits each, as in `egdb_6x6`'s material mode); its files are `6x6/np/wp-bp/wm-bm-dmW-wtm` and so on (np: the number of pieces), indexed by a
product of four colex ranks, one per colour and type: the men over the 15 squares a man may stand on, the kings over
all 18 squares. Pieces of one colour and type never share a square, the two colours may (those slots are illegal and
never visited), as in Thompson's 462 × 64ⁿ. Every move that leaves the material, a capture or a promotion of either
side, is a lookup into the files of the material it leads into: virtual addressing into finished files instead of
copies parked inside a bigger run. The files form a DAG through captures and promotions, which Snakemake builds in
order; a capture of a side's last piece ends the game and needs no file.

**Granularity** (`granularity=material|tempi|configuration`, shared with `syzygy`, synthesis §1.1): how finely a
material class is cut into groups: the whole material class, one (White tempo, Black tempo) cell (a colour's tempo is
its men's distance to promotion, summed), or one configuration of the men. The default is `material` for both styles, the nearest to Thompson's runs (below);
any granularity works with either style's passes, and below `material` every man move is an exit into an earlier
group. `tempi` gives the smallest largest group: rung 0 at most 49 MB at 6 pieces
(1996 style, 4,897 groups), against 95 MB at `material` (155) or `configuration` (830,793). `thompson86 tempi n`
prints every material class's tempi cells with their configurations and positions.

**Thompson's own granularity was coarser: a run.** Both papers index a material together with all its subsets: in 1986
a captured piece is encoded on the White king's square, an otherwise illegal slot; in 1996 a piece can be GONE, and
the unmove code has explicit uncapture branches. So captures stay inside the bitmaps, as uncaptures. In 1986 dtm is
one pass over the whole run, every subset at once, since a capture does not end the count; dtc ("max to win") takes
two passes, the subsets first, then the full material seeded with the conversions. In 1996 dtc goes in waves by number
of pieces (3, 4, 5, 6), each seeded from the previous wave (his Figures 13 and 14). Only the pawn endgames of 1986 were
sliced, one pawn rank at a time, after the promotion subgames; with a single pawn the rank is its tempo, so that is
`granularity=tempi`. This builder first built runs too (captured pieces parked on graveyard squares, see "Per material
against runs" below), then moved to per-material files with virtual addressing: every position computed once instead
of 1.43 times. The styles below are Thompson's passes; the granularities are ours.

**Two styles**, the same results byte for byte:

| | 1986 (`style=86`) | 1996 (`style=96`) |
|---|---|---|
| Thompson's group | a run: the material and all its subsets, dtm in one pass | a run, dtc in waves by number of pieces |
| our group (default) | the material class: king and man moves inside, king and man unmoves in the rounds (dtz: one total tempo at a time, king moves only) | the same |
| passes | each pass tests its target at once | masked late, as in Thompson's I/O-enhanced algorithm: P2 writes a superset, P3 and P4 mask it with `~won`, `~lost` |

In both, every move that leaves a group (captures, promotions, and below `material` man moves) is looked up forward
in pass 0, in a finished group or material file: **no uncaptures anywhere**. A position without moves inside its group is
decided there (captures are compulsory); the others keep their exit results as seeds at their depth. The rounds run
by ply level d: W_d from the unmoves of B_{d−1} plus exits that win in d plies; B_d from the unmoves of W_{d−1},
verified (every move inside leads into W, and the exits are lost by d), plus positions decided in pass 0 at d.

**Memory and disk**: per group a handful of bit planes in RAM; finished materials are read through read-only memory
maps. The depth bytes of the material being built go to a **depth sink** (`sink=` in Snakemake, the last argument of
`build`):

| sink | depth bytes in RAM | finished groups of the material | at the end |
|---|---|---|---|
| `groups` (default) | the current group's only | appended to one file per side in group order (strictly sequential writes), mapped once read-only for the lookups that moves out of a group make into earlier groups | |
| `array` | the whole material's | read from the array | |
| both | | | each of the four files written once, sequentially, in index order, gathering every slot's byte (from the group-order files: un-rank the slot, find its group, read) |

A file per group would need two memory maps per group, and `granularity=configuration` has tens of thousands of groups
in the largest materials: more than the kernel's limit of 65,530 maps per process. At 6 pieces both sinks give
byte-identical files; `groups` costs 10–15 % more CPU (dtm 38.1 s against 33.8 s, 1986 style, configurations) for the group lookups
and the gathering pass, and the largest material (03-03-01-01, 5.27 M slots per side) peaks at 19.3 MB instead of
24.9 MB of RSS (13.2 MB of it the program's own tables). This is rung 1 of the memory ladder in
[`literature/egdb_primitives.md`](../literature/egdb_primitives.md) §7.2.

**Memory-aware scheduling** (synthesis §7.3, step 1). `thompson86 plan n <metric> <style>` prints, per material, its
slots, positions, groups, largest group and the MB a build needs at rung 0 (`array` sink) and rung 1 (`groups` sink),
estimated from the engine's data structures and calibrated against measured peaks (the seed lists at their worst-case
bound, 24 bytes per position). The Snakefile runs it at parse time (`src/snakemake_plan.py`): each build claims its
rung-0 need, or the whole budget if that does not fit (`resources: mem_mb`), with a priority from the critical path; the
budget is `--resources mem_mb=N` (else `config mem_mb`, else 75 % of physical memory). With `sink=auto` (the default)
the engine takes the array if rung 0 fits its claim, else the group files, and says so in its log, with its peak RSS.
At 6 pieces with a 60 MB budget: 1996 style dtm, 149 materials on the array, 6 on group files, 5 over the budget
even at rung 1; at `granularity=configuration`, 3 all-men materials over the budget, because there the metadata of their ~100,000
one-position configurations dominates, not the depth bytes, so rung 1 cannot help. Files byte-identical to the unbudgeted
builds; no build's peak above its estimate.

**Every pass is one `std::for_each`** over blocks of 4096 positions (`for_blocks`, shared with `syzygy`), sequential
or parallel (`policy=seq|par`, the last argument of `build`). Pass 0 gives each block the words of its positions in
every bit plane and its own seed lists, merged afterwards; the pushes (P2, P3) and the seeds set bits with an atomic
OR; verification (P4) and the commit iterate whole words of the planes they write; the sink is written at distinct
positions. A build opens every file it can read before the passes, so the parallel passes only read the shared maps.
Both policies give byte-identical files (4 pieces: both styles, both sinks, all metrics; 6 pieces: 1986 style, configurations).

Where `par` pays depends on the group: a 1996-style dtm material is one group, and one build at a time takes 21.1 s
with `par` against 43.0 s with `seq` on 4 cores. A configuration group is mostly smaller than one block,
so `par` has nothing to split and only adds the thread pool's overhead (41.8 s against 38 s of `seq` CPU); there the
parallelism is across materials, four `seq` builds at a time in Snakemake (12.6 s wall for dtm), or with `granularity=tempi`
across the larger groups of a tempi cell.

## Build and run

With CMake's two steps, which check the machine (cores, memory, disk) and write the Snakemake profile, see
[`databases/`](../databases/): `cmake -S databases -B build/x -DBUILDER=thompson86 -DPIECES=6`, then `cmake --build build/x`.
By hand:

```bash
g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o build/thompson86
```

```bash
snakemake -s thompson86/Snakefile --cores 4 --config engine=build/thompson86 dir=data/thompson86 pieces=6 metric=dtm style=86
```

or with CMake, target `thompson86`. The last Snakemake job (`checkall`) builds `egdb_6x6`'s databases in memory and
compares every position of every material, both sides to move: dtm exactly; dtc and dtz their wins and losses. For
all three metrics every value of both byte files is also compared with the value recomputed by one ply over the other
file, and the bit files with the bytes. `thompson86 order <dtz> <dtc> <dtm> n` checks dtz ≤ dtc ≤ dtm for every White
win and every Black loss, and that the bit files of the three metrics are identical.

## Results (6 pieces, 4 cores)

155 materials, 97,762,498 positions (both sides to move) in 2 × 97,780,705 slots (one byte each, 97.8 MB per file
type per metric), 20,482,891 White-to-move wins and 11,884,425 Black-to-move losses.

Measured with the 1986 style at `granularity=configuration` (then its default) and the 1996 style at `material`:

| style | metric | wall time, builds | CPU summed | longest win (`dmW`) | longest loss (`dmL`) |
|---|---|---|---|---|---|
| 1986 | dtm | 10.0 s | 34.3 s | 69 plies (04-02-04-00) | 68 plies (04-02-03-00) |
| 1986 | dtc | 9.9 s | 32.4 s | 43 plies (03-02-02-01) | 42 plies (03-02-02-01) |
| 1986 | dtz | 9.4 s | 31.5 s | 27 plies (03-03-01-02) | 28 plies (03-03-01-02) |
| 1996 | dtm | 10.4 s | 35.2 s | 69 plies | 68 plies |
| 1996 | dtc | 9.4 s | 31.6 s | 43 plies | 42 plies |
| 1996 | dtz | 10.6 s | 34.7 s | 27 plies | 28 plies |

**Granularity costs little.** Timed back to back (array sink, 4 Snakemake jobs, including Snakemake and the plan at
parse time), every combination byte-identical:

| style | granularity | dtm: wall, CPU | dtc: wall, CPU | dtz: wall, CPU |
|---|---|---|---|---|
| 1986 | `material` (default) | 12.9 s, 45.6 s | 11.7 s, 41.8 s | 10.6 s, 38.5 s |
| 1986 | `configuration` | 12.2 s, 43.0 s | 11.2 s, 39.6 s | 10.7 s, 38.3 s |
| 1996 | `material` (default) | 12.6 s, 45.3 s | 11.2 s, 40.7 s | 10.6 s, 38.0 s |
| 1996 | `configuration` | 12.7 s, 44.4 s | 10.6 s, 37.9 s | 10.8 s, 38.3 s |

* **checks**: dtm equals `egdb_6x6`'s DTM in every position, both sides to move; dtc and dtz have `egdb_6x6`'s wins
  and losses; for all three metrics every `dmW` value equals its recomputation by one ply over `dmL`, and every `dmL`
  value its recomputation by one ply over `dmW`;
* the two styles give **byte-identical files** for every material, metric and file type, and the `dmW` files are
  byte-identical to those of the build that stored only `dmW`;
* **dtz ≤ dtc ≤ dtm** holds for every White win and every Black loss, with the same wins and losses in all three
  (wins: dtz = dtc in 16,667,806, dtc = dtm in 3,662,469; losses: dtz = dtc in 7,693,843, dtc = dtm in 1,107,200);
* a dtz loss can be a ply longer than the longest dtz win: Black makes a king move that does not zero, then White
  needs 27 plies;
* the bit files (12.2 MB per side and metric) equal the bytes everywhere, and are the same for all three metrics and
  both styles; rebuilding with them left every byte file unchanged;
* 3 and 4 pieces check out the same way.

**Storing `dmL` costs a second file and saves a third of the CPU.** With only `dmW`, every White move out of a group
(captures, promotions and, in the 1986 style at configurations, man moves) was looked up by one ply of Black moves over `dmW`; with `dmL`
it is one read. The 1986 dtm build went from 51.1 s to 34.3 s of CPU, 14.3 s to 10.0 s wall. That ply of search was
also what the 1996 waves saved by keeping man moves inside the rounds: with `dmL` the two styles cost the same, within
the noise of the timings.

## Per material against runs (6 pieces, 1986 style, dtm)

The earlier version built **runs**: a material read as capacities, holding every smaller material with the captured
pieces parked on graveyard squares above the board (contiguous layers by the colex rank formula), with only
promotions leaving a run. Same results, same checks; per material (only `dmW` stored, before `dmL` was added):

| | runs | per material |
|---|---|---|
| files | 70 | 155 |
| positions computed | 139,310,092 | 97,762,498 (each once) |
| slots | 193,682,790 | 97,780,705 |
| CPU summed | 68.1 s | 51.1 s |
| wall time, builds | 21.6 s | 14.3 s |

The runs computed every smaller material again inside each run whose captures reach it (1.43× the positions) and
added the gaps between parked layers to the slots; per material files compute every position once and look up
captures in the files they lead into. Whether the lookups go through memory maps or through files read into RAM made
no measurable difference.

## Against `egdb_6x6` (6 pieces, 4 cores)

| | positions computed | CPU | per position | wall time | stored |
|---|---|---|---|---|---|
| `egdb_6x6 build 6` | 51,237,726 (one of each mirror pair, both sides to move) | 22.6 s | 0.44 µs | 6.6 s (4 threads) | DTM for both sides to move, 51 MB |
| `thompson86`, 6 pieces, dtm, 1986 style, configurations | 97,762,498 | 34.3 s | 0.35 µs | 10.0 s (4 Snakemake jobs) | DTM for White to move (`dmW`) and Black to move (`dmL`), 2 × 98 MB of slots |

Per position the Thompson build is now cheaper (no ply of search for lookups); it is about 1.5× slower in total
because of **one-sidedness**: a material only finds White's wins and Black's losses, so Black's wins and White's
losses come from the files of its mirror (colours swapped), and both are built (1.9× the positions); `egdb_6x6`
builds one of each mirror pair and gets both sides to move, wins and losses, from it. The slots are twice the
positions per side to move (illegal slots where pieces of the two colours share a square), but cost no time, because
nothing visits them.
