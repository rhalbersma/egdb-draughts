# Thompson style builds (1986 and 1996) for 6×6 international draughts

`thompson86.cpp` builds endgame databases in the manner of K. Thompson, "Retrograde analysis of certain endgames",
ICCA Journal 9(3), 1986, and "6-piece endgames", ICCA Journal 19(4), 1996, adapted to draughts and to the design
notes in [`literature/egdb_primitives.md`](../literature/egdb_primitives.md). Its metrics, materials, index, groups
and unmoves are in `src/materials_6x6.hpp`, shared with the two-sided [`syzygy`](../syzygy/) builder.

**Four files per material**, all one-sided (White's wins and Black's losses):

| file | side to move | per position | value | 0 means |
|---|---|---|---|---|
| `.w` | White | one byte | the number of plies to the goal of a White win | not a win (draw, loss or illegal slot) |
| `.l` | Black | one byte | 1 + the number of plies to the goal of Black's loss (a loss can take 0 plies: Black has no move) | not lost |
| `.w.bit` | White | one bit | 1: White wins | not a win |
| `.l.bit` | Black | one bit | 1: Black is lost | not lost |

**Why depths, and why bits.** `.w` is the database: the dtm, dtc or dtz of every White-to-move win, which you need to
play the win out. `.l` holds the Black-to-move loss depths, which a dtm build needs (a White capture wins in 1 + the
loss depth of the position it leads to) and which you need to play the defence. The bit files are the one-sided WDL,
a by-product of the bytes. They are all that dtc and dtz need from the materials that a move leads into: every move
out of a material is a goal move there, after which only the outcome counts. So the dtc and dtz Snakemake rules read
only the bit files of the captures and promotions; dtm reads the bytes.

The `.l` file is Thompson 1996's RES, the Black-to-move loss distances. With it every lookup is a single read: after
a White move into `.l`, after a Black move into `.w`, without a ply of search. Three metrics, which differ only in
the moves that end the count (by either side, as long as the win is kept):

| metric | the count ends at |
|---|---|
| `dtm` | the end of the game (Black has no move) |
| `dtc` | the end, or a capture or promotion: a conversion into another material |
| `dtz` | the end, or a capture or man move: a zeroing move of the 25-move rule |

A White win can end with Black's forced capture, so in dtc and dtz a win can take an even number of plies.

**One file pair per material.** A material is named wp-bp-wm-bm (white pieces, black pieces, white men, black men,
two digits each, as in `egdb_6x6`'s material mode); its files are `6x6-wp-bp-wm-bm.<metric>.w` and `.l`, indexed by a
product of four colex ranks, one per colour and type: the men over the 15 squares a man may stand on, the kings over
all 18 squares. Pieces of one colour and type never share a square, the two colours may (those slots are illegal and
never visited), as in Thompson's 462 × 64ⁿ. Every move that leaves the material, a capture or a promotion of either
side, is a lookup into the files of the material it leads into: virtual addressing into finished files instead of
copies parked inside a bigger run. The files form a DAG through captures and promotions, which Snakemake builds in
order; a capture of a side's last piece ends the game and needs no file.

**Two styles**, the same results byte for byte:

| | 1986 (`style=86`) | 1996 (`style=96`) |
|---|---|---|
| group solved together | one slice of `egdb_6x6` (a placement of the men and the numbers of kings), in order of pieces on the board, then distance | a wave: every slice with the same number of pieces (for dtz also the same distance, since a man move ends the count there) |
| moves inside the group | king moves without capture | king moves and man moves without capture or promotion (dtz: king moves only) |
| unmoves in the rounds | king unmoves | king unmoves and man unmoves |
| passes | each pass tests its target at once | masked late, as in Thompson's I/O-enhanced algorithm: P2 writes a superset, P3 and P4 mask it with `~won`, `~lost` |

In both, every move that leaves a group (captures, promotions, and in the 1986 style man moves) is looked up forward
in pass 0, in a finished group or material file: **no uncaptures anywhere**. A position without moves inside its group is
decided there (captures are compulsory); the others keep their exit results as seeds at their depth. The rounds run
by ply level d: W_d from the unmoves of B_{d−1} plus exits that win in d plies; B_d from the unmoves of W_{d−1},
verified (every move inside leads into W, and the exits are lost by d), plus positions decided in pass 0 at d.

**Memory and disk**: per material its two byte arrays and per group a handful of bit planes in RAM; finished
materials are read through read-only memory maps; the only disk writes are one sequential write per file of a finished
material (to a temporary name, then renamed).

## Build and run

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

| style | metric | wall time, builds | CPU summed | longest win (`.w`) | longest loss (`.l`) |
|---|---|---|---|---|---|
| 1986 | dtm | 10.0 s | 34.3 s | 69 plies (04-02-04-00) | 68 plies (04-02-03-00) |
| 1986 | dtc | 9.9 s | 32.4 s | 43 plies (03-02-02-01) | 42 plies (03-02-02-01) |
| 1986 | dtz | 9.4 s | 31.5 s | 27 plies (03-03-01-02) | 28 plies (03-03-01-02) |
| 1996 | dtm | 10.4 s | 35.2 s | 69 plies | 68 plies |
| 1996 | dtc | 9.4 s | 31.6 s | 43 plies | 42 plies |
| 1996 | dtz | 10.6 s | 34.7 s | 27 plies | 28 plies |

* **checks**: dtm equals `egdb_6x6`'s DTM in every position, both sides to move; dtc and dtz have `egdb_6x6`'s wins
  and losses; for all three metrics every `.w` value equals its recomputation by one ply over `.l`, and every `.l`
  value its recomputation by one ply over `.w`;
* the two styles give **byte-identical files** for every material, metric and file type, and the `.w` files are
  byte-identical to those of the build that stored only `.w`;
* **dtz ≤ dtc ≤ dtm** holds for every White win and every Black loss, with the same wins and losses in all three
  (wins: dtz = dtc in 16,667,806, dtc = dtm in 3,662,469; losses: dtz = dtc in 7,693,843, dtc = dtm in 1,107,200);
* a dtz loss can be a ply longer than the longest dtz win: Black makes a king move that does not zero, then White
  needs 27 plies;
* the bit files (12.2 MB per side and metric) equal the bytes everywhere, and are the same for all three metrics and
  both styles; rebuilding with them left every byte file unchanged;
* 3 and 4 pieces check out the same way.

**Storing `.l` costs a second file and saves a third of the CPU.** With only `.w`, every White move out of a group
(captures, promotions and, in the 1986 style, man moves) was looked up by one ply of Black moves over `.w`; with `.l`
it is one read. The 1986 dtm build went from 51.1 s to 34.3 s of CPU, 14.3 s to 10.0 s wall. That ply of search was
also what the 1996 waves saved by keeping man moves inside the rounds: with `.l` the two styles cost the same, within
the noise of the timings.

## Per material against runs (6 pieces, 1986 style, dtm)

The earlier version built **runs**: a material read as capacities, holding every smaller material with the captured
pieces parked on graveyard squares above the board (contiguous layers by the colex rank formula), with only
promotions leaving a run. Same results, same checks; per material (only `.w` stored, before `.l` was added):

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
| `thompson86`, 6 pieces, dtm, 1986 style | 97,762,498 | 34.3 s | 0.35 µs | 10.0 s (4 Snakemake jobs) | DTM for White to move (`.w`) and Black to move (`.l`), 2 × 98 MB of slots |

Per position the Thompson build is now cheaper (no ply of search for lookups); it is about 1.5× slower in total
because of **one-sidedness**: a material only finds White's wins and Black's losses, so Black's wins and White's
losses come from the files of its mirror (colours swapped), and both are built (1.9× the positions); `egdb_6x6`
builds one of each mirror pair and gets both sides to move, wins and losses, from it. The slots are twice the
positions per side to move (illegal slots where pieces of the two colours share a square), but cost no time, because
nothing visits them.
