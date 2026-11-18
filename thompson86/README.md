# A Thompson (1986) style build for 6×6 international draughts

`thompson86.cpp` builds endgame databases in the manner of K. Thompson, "Retrograde analysis of certain endgames",
ICCA Journal 9(3), 1986, adapted to draughts and to the design notes in
[`literature/egdb_primitives.md`](../literature/egdb_primitives.md):

* **one byte per position with White to move**: the number of White moves to a win (DTM: a win in i moves is
  2i − 1 plies), 0 for "not a win" (draw, loss or illegal slot). Black-to-move values are not stored: one ply of
  search over White-to-move results gives them (lost in max(i) moves when every Black move leads to a White win in i);
* **runs**: a run is a material read as capacities (WM white men, WK white kings, BM black men, BK black kings). It
  holds every position with at most that many pieces of each colour and type; captured pieces are parked on graveyard
  squares above the board (per colour and type a colex rank over the squares where the piece may stand plus one
  graveyard square per piece, the parked pieces filling the graveyard in order, so every parked layer is a contiguous
  block whose offset follows from the rank formula);
* **captures forward, no uncaptures**: units (the slices of `egdb_6x6`: a placement of the men and the numbers of
  kings) are built in order of pieces on the board and distance of the men to promotion. Pass 0 looks up every move
  that leaves the unit (captures, man moves, promotions) in a finished unit or run; positions without king moves
  inside the unit are decided there (captures are compulsory);
* **Thompson's rounds on bit planes**: P2 (White king unmoves from the Black losses of the last level), P3 (Black
  king unmoves from the new White wins: candidates), P4 (a candidate is lost when every Black king move leads into W
  and its exits are lost already); exit results enter at their depth (deferred seeds), so the depths are exact;
* **the promotion DAG via Snakemake**: a run needs the run where a white man has promoted and the run where a black
  man has; captures add no edges. All runs of n pieces cover every material with at most n pieces;
* **memory and disk**: per run the result bytes and per unit a handful of bit planes in RAM; finished runs are read
  through read-only memory maps; the only disk write is one sequential write of a finished run.

## Build and run

```bash
g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o build/thompson86
```

```bash
snakemake -s thompson86/Snakefile --cores 4 --config engine=build/thompson86 dir=data/thompson86 pieces=6
```

or with CMake, target `thompson86`. The last Snakemake job (`checkall`) builds `egdb_6x6`'s databases in memory and
compares every position of every run, both sides to move, with its DTM.

## Results (4 cores)

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
