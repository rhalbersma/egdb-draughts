# Thompson style builds (1986 and 1996) for 6×6 international draughts

`thompson86.cpp` builds endgame databases in the manner of K. Thompson, "Retrograde analysis of certain endgames",
ICCA Journal 9(3), 1986, and "6-piece endgames", ICCA Journal 19(4), 1996, adapted to draughts and to the design
notes in [`literature/egdb_primitives.md`](../literature/egdb_primitives.md).

**One byte per position with White to move**: the number of plies to the goal of a White win, 0 for "not a win"
(draw, loss or illegal slot). Black-to-move values are not stored: one ply of search over White-to-move results
gives them. Three metrics, which differ only in the moves that end the count (by either side, as long as the win is
kept):

| metric | the count ends at |
|---|---|
| `dtm` | the end of the game (Black has no move) |
| `dtc` | the end, or a capture or promotion: a conversion into another material |
| `dtz` | the end, or a capture or man move: a zeroing move of the 25-move rule |

A White win can end with Black's forced capture, so in dtc and dtz a win can take an even number of plies.

**One file per material.** A material (WM white men, WK white kings, BM black men, BK black kings) is the file
`mat-WM-WK-BM-BK.<metric>`, indexed by a product of four colex ranks, one per colour and type: the men over the 15
squares a man may stand on, the kings over all 18 squares. Pieces of one colour and type never share a square, the
two colours may (those slots are illegal and never visited), as in Thompson's 462 × 64ⁿ. Every move that leaves the
material, a capture or a promotion of either side, is a lookup into the file of the material it leads into: virtual
addressing into finished files instead of copies parked inside a bigger run. The files form a DAG through captures
and promotions, which Snakemake builds in order; a capture of a side's last piece ends the game and needs no file.

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

**Memory and disk**: per material the result bytes and per group a handful of bit planes in RAM; finished materials
are read through read-only memory maps; the only disk write is one sequential write of a finished material (to a
temporary name, then renamed).

## Build and run

```bash
g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o build/thompson86
```

```bash
snakemake -s thompson86/Snakefile --cores 4 --config engine=build/thompson86 dir=data/thompson86 pieces=6 metric=dtm style=86
```

or with CMake, target `thompson86`. The last Snakemake job (`checkall`) builds `egdb_6x6`'s databases in memory and
compares every position of every material, both sides to move: dtm exactly; dtc and dtz their wins and losses, and every
White-to-move value against the value recomputed from its successors. `thompson86 order <dtz> <dtc> <dtm> n` checks
dtz ≤ dtc ≤ dtm for every White win.

## Results (6 pieces, 4 cores)

155 materials, 97,762,498 positions (both sides to move) in 97,780,705 slots (one byte each, 97.8 MB per metric),
20,482,891 White-to-move wins.

| style | metric | wall time, builds | CPU summed | longest win | checks |
|---|---|---|---|---|---|
| 1986 | dtm | 14.3 s | 51.1 s | 69 plies | every value equals `egdb_6x6`'s DTM |
| 1986 | dtc | 13.8 s | 49.0 s | 43 plies | wins and losses equal `egdb_6x6`'s; every value equals its recomputation from the successors |
| 1986 | dtz | 13.5 s | 48.4 s | 27 plies | the same |
| 1996 | dtm | 13.4 s | 47.8 s | 69 plies | every value equals `egdb_6x6`'s DTM |
| 1996 | dtc | 11.9 s | 42.3 s | 43 plies | as for 1986 |
| 1996 | dtz | 13.3 s | 48.0 s | 27 plies | as for 1986 |

* the two styles give **byte-identical files** for every material and every metric;
* **dtz ≤ dtc ≤ dtm** holds for every one of the 20,482,891 White wins, with the same wins in all three; dtz = dtc
  in 16,667,806 of them, dtc = dtm in 3,662,469;
* the 1996 waves are 6–14 % faster for dtm and dtc, where man moves are handled inside the rounds instead of being
  looked up in pass 0, and no faster for dtz, where man moves end the count and the waves are only groups of slices;
* the longest dtm win, 69 plies, is in 4-0-0-2 (four white men against two black kings);
* 3 and 4 pieces check out the same way.

## Per material against runs (6 pieces, 1986 style, dtm)

The earlier version built **runs**: a material read as capacities, holding every smaller material with the captured
pieces parked on graveyard squares above the board (contiguous layers by the colex rank formula), with only
promotions leaving a run. Same results, same checks; per material:

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
| `thompson86`, 6 pieces, dtm, 1986 style | 97,762,498 | 51.1 s | 0.52 µs | 14.3 s (4 Snakemake jobs) | DTM for White to move, 98 MB of slots |

The Thompson build is about 2.3× slower, nearly all of it from **one-sidedness**: a material only finds White's
wins, so Black's wins come from the file of its mirror (colours swapped), and both are built (1.9× the positions);
`egdb_6x6` builds one of each mirror pair and gets both sides to move from it. The slots are twice the White-to-move
positions (illegal slots where pieces of the two colours share a square), but cost no time, because nothing visits
them.
