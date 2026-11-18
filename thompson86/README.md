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

**Runs.** A run is a material (WM white men, WK white kings, BM black men, BK black kings) read as capacities: it
holds every position with at most that many pieces of each colour and type, the captured ones parked on graveyard
squares above the board (per colour and type a colex rank over the squares where the piece may stand plus one
graveyard square per piece, the parked pieces filling the graveyard in order, so every parked layer is a contiguous
block whose offset follows from the rank formula). Promotions leave the run; the runs form a DAG through promotions,
which Snakemake builds in order. All runs of n pieces cover every material with at most n pieces.

**Two styles**, the same results byte for byte:

| | 1986 (`style=86`) | 1996 (`style=96`) |
|---|---|---|
| group solved together | one slice of `egdb_6x6` (a placement of the men and the numbers of kings), in order of pieces on the board, then distance | a wave: every slice with the same number of pieces (for dtz also the same distance, since a man move ends the count there) |
| moves inside the group | king moves without capture | king moves and man moves without capture or promotion (dtz: king moves only) |
| unmoves in the rounds | king unmoves | king unmoves and man unmoves |
| passes | each pass tests its target at once | masked late, as in Thompson's I/O-enhanced algorithm: P2 writes a superset, P3 and P4 mask it with `~won`, `~lost` |

In both, every move that leaves a group (captures, promotions, and in the 1986 style man moves) is looked up forward
in pass 0, in a finished group or run: **no uncaptures anywhere**. A position without moves inside its group is
decided there (captures are compulsory); the others keep their exit results as seeds at their depth. The rounds run
by ply level d: W_d from the unmoves of B_{d−1} plus exits that win in d plies; B_d from the unmoves of W_{d−1},
verified (every move inside leads into W, and the exits are lost by d), plus positions decided in pass 0 at d.

**Memory and disk**: per run the result bytes and per group a handful of bit planes in RAM; finished runs are read
through read-only memory maps; the only disk write is one sequential write of a finished run.

## Build and run

```bash
g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o build/thompson86
```

```bash
snakemake -s thompson86/Snakefile --cores 4 --config engine=build/thompson86 dir=data/thompson86 pieces=6 metric=dtm style=86
```

or with CMake, target `thompson86`. The last Snakemake job (`checkall`) builds `egdb_6x6`'s databases in memory and
compares every position of every run, both sides to move: dtm exactly; dtc and dtz their wins and losses, and every
White-to-move value against the value recomputed from its successors. `thompson86 order <dtz> <dtc> <dtm> n` checks
dtz ≤ dtc ≤ dtm for every White win.

## Results of the first version (dtm, 1986 style, 4 cores)

| pieces | runs | positions (both sides to move) | slots | wall time, build and check | differences from `egdb_6x6` |
|---|---|---|---|---|---|
| 2 / 3 / 4 | 4 / 12 / 25 | | | < 1 s each | 0 |
| 5 | 44 | | | 5.6 s | 0 |
| 6 | 70 | 139,310,092 | 193,682,790 | 44 s (the builds alone: 18.7 s) | **0** |

At 6 pieces the runs hold 1,618,608 units and 28,928,639 White-to-move wins; the files take 186 MB. The longest win is
35 moves (69 plies), in the run 4-0-0-2 (four white men against two black kings, with its smaller materials).
Every position of every run, White to move (the stored byte) and Black to move (one ply of search over the stored
bytes), equals `egdb_6x6`'s DTM.

## Against `egdb_6x6` (6 pieces, 4 cores)

| | positions computed | CPU | per position | wall time | stored |
|---|---|---|---|---|---|
| `egdb_6x6 build 6` | 51,237,726 (one of each mirror pair, both sides to move) | 22.6 s | 0.44 µs | 6.6 s (4 threads) | DTM for both sides to move, 51 MB |
| `thompson86`, 6 pieces | 139,310,092 | 67.2 s | 0.48 µs | 18.7 s (4 Snakemake jobs) | DTM for White to move, 186 MB of slots |

Per position the two cost the same; the Thompson build is about 3× slower because it computes 2.7× as many
positions:

* **2× from one-sidedness**: a run only finds White's wins, so Black's wins in a material come from the run of its
  mirror (colours swapped), and both are built; `egdb_6x6` builds one of each mirror pair and gets both sides to move
  from it;
* **about 1.36× from the runs' duplication**: a smaller material lies inside every run whose captures reach it, and is
  computed in each of them.

The slots add a further 1.4× to the file size (illegal slots where pieces share a square, and the gaps between the
parked layers), but cost no time, because nothing ever visits them.
