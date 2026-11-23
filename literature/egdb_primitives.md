# Endgame-database construction: primitives and trade-offs for draughts

Synthesis of the 47 files in the Drive folders *AI / endgame databases* (30) and *AI / solving games* (17),
of the source of the syzygy generator (`syzygy1/tb`, `src/rtbgen.c`, `src/generic.c`, `src/tbgen.c`), and of the
current code (`src/egdb_fast.cpp`, `src/egdb_6x6.cpp`, and `strongly_solving_6x6/DESIGN_STATE.md` on the branch
`claude/sweet-hypatia-i05bm9`). The per-paper notes are in [`papers/`](papers/) (one file per group, twelve fixed
headings per paper). They were extracted from the PDF text and flag every gap (garbled OCR, missing pages); a
statement marked *[bg]* there or here is background knowledge, not verified from a source in this set.

The goal throughout: **one `std::for_each(policy, …)` per pass**, with `policy` either `seq` or `par`, so the question
for every primitive is what the pass ranges over and whether it writes only the element it visits.

* [1. Vocabulary](#1-vocabulary)
* [2. The two lineages](#2-the-two-lineages)
* [3. Catalogue of primitives](#3-catalogue-of-primitives)
* [4. Who uses what](#4-who-uses-what)
* [5. Metrics: WDL, DTZ, DTM in draughts](#5-metrics-wdl-dtz-dtm-in-draughts)
* [6. Data per position](#6-data-per-position)
* [7. Rules for one `for_each` per pass](#7-rules-for-one-for_each-per-pass)
* [8. Best of breed for draughts](#8-best-of-breed-for-draughts)
* [9. Value encodings in domain language](#9-value-encodings-in-domain-language)
* [10. Storage](#10-storage)
* [11. Verification](#11-verification)
* [12. The open decisions of DESIGN_STATE.md](#12-the-open-decisions-of-design_statemd)
* [13. Measurements still needed](#13-measurements-still-needed)
* [Appendix: the papers in one line each](#appendix-the-papers-in-one-line-each)

---

## Settled decisions (the defaults of the 6×6 builders)

Decided in the design discussions and implemented in `tbgen/` (the two-sided builder: `itbgen` for international draughts, `ctbgen` for English
checkers). Change one only on purpose, and update this list with it.

| topic | decision | where |
|---|---|---|
| values | exact plies for every metric, never rounded or halved; no draw-rule band | §7.5, §10 |
| board | a compile-time parameter: any tabula draughts board, of any size (bitboards are `xstd::bit_fixed_set` aligned to blocks of 64 bits, as in `egdb_fast`; ranks by PEXT/PDEP a block at a time; one block keeps the integer fast paths): `EGDB_WIDTH` × `EGDB_HEIGHT`, `EGDB_PARITY` (a1 dark or light), `EGDB_LAKE` (removed squares), or a tabula preset (`EGDB_BOARD`); squares numbered as in PDN; the rules by `EGDB_6X6_CHECKERS`; the tree starts with the board's name (`tbgen board`: `10x10-j10`, `8x8-p0`). The colour mirror is computed: the 180° rotation, else the reflection in the middle row (odd height), else none, and then every material is built in both colours. Checked by perft (8×8 checkers, 10×10 international) and against `egdb_fast` (kings only) | `src/egdb_6x6.cpp`, CMake `egdb_board()` |
| index | the joint index: legal placements only (white men, black men over what the white men leave, kings over the free squares), ranked with PEXT (`bit_compress`, physical to logical squares) and colex tables (up to 20 squares; beyond, sums of binomials), unranked with PDEP; men outermost | `src/materials_6x6.hpp` (`EGDB_JOINT_INDEX=1`) |
| build cell | 8 bits per position and side (cell8), reductions every 240 codes into value-preserving zstd snapshots, merged as a transform-reduce over the windows | §7.5, `rounds=cell8` |
| loss checks | delayed (rtbgen's `CHANGED`): mark the candidates, check each once | cell8 and frontier modes |
| inputs | read only in pass 0; DTM builds read only the DTM build files (self-contained: 1 + plies, 0 a draw), DTC and DTZ builds only the WDL build files | `tbgen/tbgen.cpp` |
| build format | both sides, every position stored (captures, pending captures, draws); zstd blocks of 4 KiB read through a per-thread block cache (64 MiB); escape tokens beyond the direct range (E = 241, one extra byte, up to 3,840) | `store=compressed` |
| sink | the 8-bit sink in anonymous memory, each file written once, sequentially: no random writes | `sink=bytes` |
| WDL | one shared directory `<dir>/wdl/`, written by the first metric built, compared (not rewritten) by the others | Snakefile |
| metrics | WDL + DTM to solve; DTZ before DTC; DTC reuses DTZ for kings-only materials | Snakefile |
| probe format | one side to move; captures and draws as don't-cares (pending captures planned); depth without class (from WDL): DTM in moves, DTC and DTZ in plies, 16 escape tokens, no draw code; zstd blocks of 4 KiB; converted once the materials a material leads into are converted, then its depth build files are deleted | §10, `probe=1`, `tbgen convert` |
| fills | don't-cares take the dominant or the run-extending value, never an escape token | §10 |
| parallelism | `for_blocks<B>(policy, n, f)`, B a multiple of 8 cache lines; Snakemake `policy=auto`: the largest materials (at least 1 / (4 × cores) of all positions, those at the end of the critical path) with `std::execution::par` on all cores, alone; the rest on one core each, side by side as far as `--cores` and the memory claims allow | `src/materials_6x6.hpp`, Snakefile |
| layout | `<dir>/<metric>/<board>/<np>/<wp>-<bp>/<wm>-<bm>-<kind>-<side>.z` (board 6x6, 8x8, 10x10, …), `<dir>/wdl/…`, `<dir>/probe/<kind>/…` | Snakefile |

## 1. Vocabulary

| term | meaning |
|---|---|
| **slice** | a part of a database closed under the moves that are not conversions; here: a placement of the men and the numbers of kings per side. Only king moves without capture stay inside |
| **exit** (conversion, zeroing move) | every other move: capture, man move, promotion. Leads to an already finished slice |
| **init** | terminal positions (side to move has no move: lost), illegal or broken index slots |
| **conversion** | values of exits, by probing finished slices |
| **pull** (forward, successor scan) | visit a position, generate its *moves*, read the successors, write the position itself |
| **push** (retrograde, predecessor) | visit a decided position, generate its *unmoves*, write into the predecessors |
| **win step** | a position with a successor lost in n − 1 is won in n (any one successor suffices) |
| **loss step** | a position is lost in n when all successors are won, the slowest in n − 1. Needs a test over *all* successors, done in one of three ways: |
| — **verification** ("grandfather") | push marks candidates; a pull over each candidate checks all its successors (Thompson 1986, Herik 1985, Wu-Beal, Gasser 1991, Lincke-Marzetta, Romein-Bal 2003, syzygy) |
| — **outcounting** | a counter of successors not yet known to be won; the push decrements it, zero means lost (Gasser 1996, Wirth, Bal-Allis, Fang 2000, `egdb_fast`, `egdb_6x6` work lists) |
| — **re-scan** | full pull sweeps until nothing changes (Ströhlein, Chinook, Nalimov, Kishimoto-Müller) |
| **frontier** | the positions decided in the previous round. *Implicit*: found by scanning for value = n − 1. *Explicit*: queue, list or bitset |
| **level-synchronous** vs **chaotic** | rounds that see only results of earlier rounds (exact depths) vs in-place updates that see results of the same round (exact WDL, depth only a lower bound: Lake-Schaeffer-Treloar 1992, Schaeffer 2003) |
| **finalisation** | what is still undecided is a draw |
| **verification pass** | recompute every value from its successors after the build |

Wu-Liu-Hsu (2004) classify retrograde algorithms in four categories, a useful shorthand:
**I** forward scan to a fixpoint (Chinook, Nalimov); **II** layered push plus verification (Thompson, Wu-Beal,
syzygy); **III** queue plus counting (Gasser 1996, Fang 2000); **IV** layered plus counting (Bal-Allis, Wirth).

### 1.1 Units of a build: subdivisions, slices, groups, jobs

Two kinds of unit, by the kind of irreversible move that separates them.

**Subdivisions** fix piece counts; captures and promotions lead from one to another:

| subdivision | fixed | example |
|---|---|---|
| n pieces | the total number of pieces | the 6-piece positions |
| pieces per side | white pieces and black pieces | 4 against 2 |
| **material class** ("material" for short) | white men, white kings, black men, black kings | 03-02-02-01 in wp-bp-wm-bm (white pieces, black pieces, white men, black men): White 2 men and 1 king, Black 1 man and 1 king |

The material class is the unit of the files (`6x6/np/wp-bp/wm-bm-*`, every position at its index of four colex ranks);
the material classes form a DAG through captures and promotions.

**Names follow the subdivisions.** The subdivisions are the directories of a tree, `db/np/wp-bp/` (the database, the
number of pieces, pieces per side), and a material class's files are named by what is left: men per side, a kind of
three characters (`dtm`, `dtc`, `dtz`, `wdl` for full results; `dmW`, `dmL`, `dcW`, ... and `isW`, `isL` for half
results) and the side to move: `6x6/06/04-02/02-01-dmW-wtm`. The tree shows the dependencies: a capture always leads
into a lower `np` directory, a promotion keeps the piece counts and stays inside its `wp-bp` directory. These files
are permanent; a later compression may pack a directory into one file of a coarser granularity. A slice adds its
tempi, `wm-bm-<kind>-<side>-wt-bt` (e.g. `6x6/06/04-02/02-01-dmW-wtm-03-01`): a temporary file, consolidated into the
material class's file as soon as the class is resolved.

**Slices** split a material class by the configuration of its irreversibly moving pieces (draughts and checkers men,
chess pawns); a man's or pawn's advance leads from one slice to another, so the slices of a material class form a DAG
too. Ours come in two granularities, the coarser a union of the finer:

| slice, labelled by | |
|---|---|
| the **tempi**: each colour's tempo, its men's distance to promotion summed over its men (rows still to go) | `thompson86`, `tbgen` (`granularity=tempi`) |
| the **configuration**: every man's square | `egdb_6x6`, `thompson86`, `tbgen` (`granularity=configuration`) |

The tempi factorize by colour: every man move lowers its own colour's tempo by exactly one, a white man move from
(tw, tb) to (tw − 1, tb), a black one to (tw, tb − 1). So the tempi are a grid, every man move leads to a neighbour
cell, and the cells on an anti-diagonal (the same total tw + tb) never lead into each other. For 03-03-03-03: 13 × 13
cells (each colour's three men need 3 to 15 rows), 167 of them non-empty, against 118,587 configurations.

Chinook and Gilbert label their slices by **leading ranks** instead: each colour's most advanced man (Chinook's
"slice", Gilbert's first tier), refined by the men on that rank and then by the penultimate rank (Gilbert's tiers 2
and 3; Dodgen–Trice). We do not use them: a leading-rank slice must classify every man move (a rear man's move stays
inside, the leading man's leaves, a rear man overtaking the leading rank changes the slice), whereas every man move
leaves its tempi, so below material granularity the rounds need no man unmoves at all.

A kings-only material class is a single slice at both granularities.

**The metrics count inside these units.** DTC counts the moves inside a material class: the count ends at a capture
or a promotion, the moves that leave it. DTZ counts the moves inside a configuration: the count also ends at a man move,
the move that leaves it. DTM counts to the end of the game, across all of them.

**A group** is a set of configurations of one material class solved together: whole slices of one total tempo, or a
pack of one slice's configurations (or the whole material class). It is the unit of one solve, whose rounds share bit
planes and frontiers. **Slices and groups are kept apart**: the slice is the unit of the data, fixed by the rules (its
DAG is the tempi grid, its files are named by it, `<name>-wt-bt`), the group the unit of computation, cut to fit the
machine (its RAM and its cores, §7.4). A group may cover several slices (cells of one total tempo, which are
independent of each other) or part of one (a pack of its configurations, which are independent too); a solve writes
the slice files of the slices it covers, so the names stay the same whatever the machine. Chinook 1994 drew the same
line: the slice was its unit of data, the page-aligned mini-slice its unit of work. **A job** is what the scheduler (Snakemake) schedules: one command
with its inputs, outputs and resources. In our builders a job builds one material class (it lists the class's groups,
solves them in dependency order and writes its files); Chinook 1994's jobs were passes over mini-slices, Gilbert's
were subdivisions.

**A group** must be closed under the moves the rounds treat as inside; every other move is an exit into a group or
material class already finished, looked up in pass 0. King moves never leave a slice, man moves connect the slices
of a material class. The **granularity** says how finely a material class is cut into groups (`granularity=` in both
builders):

| granularity | a group is | moves inside | default for |
|---|---|---|---|
| (run) | a material class and all its subsets (Thompson 1986, 1996; not offered) | king and man moves, captures as uncaptures | |
| `material` | the whole material class (dtz: one total tempo at a time, since a man move ends the count) | king and man moves | `tbgen`, `thompson86` (both styles) |
| `tempi` | one (White tempo, Black tempo) cell | king moves | |
| `configuration` | one configuration of the men | king moves | `egdb_6x6` |

Above `material` there is one coarser granularity, which we do not offer: Thompson's **run**, a material class with all
its subsets in one index (1986: a captured piece on the White king's square; 1996: a GONE piece), captures inside as
uncaptures. His 1986 dtm is one pass over the whole run, every subset at once (dtm does not stop at a capture: depth
outer, material inner), his dtc two passes (subsets first) and his 1996 dtc waves by number of pieces. A run computes
every subset again inside each run whose captures reach it (1.43× the positions in `thompson86`'s first version), so we
take material outer, depth inner, and look the subsets up in their finished files. His 1986 pawn endgames were sliced
by the pawn's rank, which for a single pawn is its tempo.

Below `material` every man move is an exit into a group solved earlier (it lowers its colour's tempo), looked up in
pass 0 like a capture. At 6 pieces `tempi` gives the smallest largest group in both builders: rung 0 at most 58 MB in
`tbgen` (2,652 groups) and 49 MB in `thompson86` (4,897), against 94–103 MB at `material` (82 and 155 groups) or at
`configuration` (483,750 and 830,793 groups, where the metadata of the all-men materials' one-position configurations
dominates).

A build solves the groups of its material class one after another in dependency order, later groups looking up earlier
ones through the depth sink.

**The same levels in Chinook's and Gilbert's words** (read verbatim; the words shift from paper to paper):

| level | ours | Chinook 1994 (Lake–Schaeffer–Lu) | Chinook 2003 (Schaeffer et al., 10 pieces) | Gilbert 2005 (10-piece checkers) | Gilbert 2009 (8-piece draughts) |
|---|---|---|---|---|---|
| subdivision: n pieces | subdivision | "the 8-piece database" | "the N-piece database" ("by pieces") | "the 10-piece database" | "the 8 piece database" |
| subdivision: pieces per side | subdivision (in the material class's name) | "subsets, such as 6b versus 2w"; a "graph" holds "all positions composed of b black pieces and w white pieces" | **"by material"**: "subsets with a different number of pieces per side", "8 pieces versus 1, 7 versus 2" | — | "4x4 and 5x3 positions" |
| subdivision: material class | **material class** `wp-bp-wm-bm` | a **"node"** of the graph, a "subdatabase" named by four digits (black kings, white kings, black checkers, white checkers, e.g. 3212); "subdatabases according to the material, the number of checkers and kings" | "by number of kings": a "sub-database" or "subset", Table 1's "Database" column in "bk wk bc wc" notation (e.g. 3223; "the 1045 database") | a **"slice"** ("the slice with 2 black men, 3 black kings, 2 white men, and 3 white kings") | the "first subdivision", "by man and king counts" ("the subset of 1bm 3bk 2wm 2wk") |
| slice: leading rank | — (we slice by tempi) | a **"slice"**, e.g. 3212.53 (49 per node; kings-only nodes unsliced) | "by leading rank": a **"slice"**, "bk wk bc wc. br wr", e.g. 3223.77 ("the symmetric database 2233.77") | tier 1, "piece counts and leading ranks" (9 pieces: the leading *square*) | the "second subdivision", "the row of the most advanced black and white man" |
| slice: leading-rank configuration | — | — | "slices can be further sub-divided" (Dodgen–Trice: "the configuration of checkers on the first rank"), not used | tier 2, "configuration of the checkers on the leading rank" | the "third subdivision" |
| slice: penultimate rank | — | — | — | tier 3, "the penultimate rank of checkers" | — |
| slice: tempi | **tempi** (our builders) | — | — | — | — |
| slice: configuration | **configuration** (`egdb_6x6`, our builders) | — | — | — | — |
| one solve | **group** | a slice with its mirror | a slice: "A portion of the endgame database (a slice) is computed by resolving all positions" | a "subdivision" (tiers 2 and 3 only above 900 MB; 64,369) | a "subdivision" (about 200K) |
| a job (what is scheduled) | one material class | a job queue line: one pass over a slice or mini-slice | slices (TrellisDAG) | one subdivision per program instance | one subdivision per instance |
| a parallel index range | a block of 4096 positions | a **"mini-slice"**: page-aligned, about 10 million positions | POSIX threads dividing "the range of positions" equally | — | — |
| the other colour | tbgen: one of each mirror pair; thompson86: both | the **"mirror"** node (4112 ↔ 1421) | "its mirror database 2332.77 with white to move" | — | — |

In this vocabulary: Chinook's node (1994) or sub-database (2003) and Gilbert's 2005 "slice" are material classes;
Chinook 2003's "material" is the pieces-per-side subdivision; Chinook's "slice" is a leading-rank slice; Gilbert's
"subdivision" is whatever his tiers produced, a material class or a leading-rank slice of some refinement;
`egdb_6x6`'s slice is a configuration. Chinook and Gilbert slice from coarse to fine until a piece fits memory, by
leading ranks; we slice by tempi or configurations. All cut along irreversible moves (captures and promotions between
subdivisions, a man's advance between slices), which is what makes every scheme a DAG.

**Our jobs are material classes**: Snakemake runs them in parallel when none needs another, in the order of the DAG.
They are not smaller because the groups of a material class depend on each other densely and most are tiny
(at `granularity=configuration` a material class can have tens of thousands of groups): a job per group would be far too
fine-grained, and every job would open the same files again. The memory ladder (§7.2) follows the same split: the
job's memory claim is per material class, the rung is chosen per group inside it.

## 2. The two lineages

### Chinook → Gilbert (checkers, draughts)

Lake-Schaeffer-Lu 1994, Schaeffer et al. 2003 (details in [`papers/3_checkers.md`](papers/3_checkers.md)):

* **slicing**: pieces → material → kings per side → leading man's rank per side (49 slices per material), built from
  the most advanced ranks down; slices on an anti-diagonal of the rank grid in parallel;
* **WDL only**, 2 bits per position during the build, slice and mirror together, own paging (8 KB pages);
* passes: **Captures** (full-range pull into smaller databases, 50–60 % of positions, fixed from then on) →
  **WinningMoves** (one push: every predecessor of a loss is a win) → **Lookups** (full-range pull of the exits that
  stay in the material, folded into a 2-bit bound {L, U, ≥D, W}, so the iteration does no I/O) → **NoIo** (repeated
  in-place pull sweeps, alternating side to move, to a fixpoint) → late switch to a **queue** of newly decided
  positions whose predecessors are re-scanned. No counters anywhere;
* storage without capture positions (≈ 75 % with capture threats): their slots continue the current run, then RLE
  (1994) or Huffman runs (2003) in 1 KB blocks with a CRC each; a probe resolves them with a small search;
* verification was the weak spot: Trice-Dodgen's independent build found a few thousand wrong values in Chinook's
  7-piece databases that had passed every internal check.

Trice-Dodgen 2003 added distances to the same indexing: DTC as the pass number at which a position is decided
(one byte, win/loss parity halving to 511 ply). Björnsson-Schaeffer-Sturtevant 2005 kept the passes but replaced
WDL by a bound lattice, so a slice can be built before its dependencies.

*[bg]* Ed Gilbert's databases (Kingsrow: checkers to 10 pieces, international draughts WDL and later
moves-to-conversion tables) follow this design: Chinook-style slicing, 2-bit WDL, capture positions omitted, block
compression with a capture search at probe time. There is no paper on them in this set; the one fact here is from
the checkers papers: Gilbert's cross-check turned up a corrupted copy of a Chinook file.

### Thompson → syzygy (chess)

Thompson 1986 ([`papers/1_chess_classic.md`](papers/1_chess_classic.md)): **bitmaps** per ply,
B_i → (White unmoves, scatter) → W_{i+1} → (Black unmoves, scatter) → candidates → (all Black moves land in W:
**verification**, gather) → B_{i+1}; cumulative bitmaps W, B for O(1) "already known"; a depth byte written once per
level; **waves** (smaller endgames first, their results seed the start set) and pawn-rank slicing with un-promotion.
Thompson 1996 reshaped it for disk (delayed masking: scatter into a superset, AND on the next sequential read) and
explicitly rejected outcounting (random I/O, moves-out ≠ unmoves-in under symmetry folding, no restart).

syzygy, verified from the generator source (`rtbgen.c`, `generic.c`, `tbgen.c`):

* **one byte per position per side to move** (`table_w`, `table_b`), values plus codes: `ILLEGAL 0`, `CAPT_WIN 1`,
  `WIN_IN_ONE 2`, … , `LOSS_IN_ONE 0xf9`, `MATE 0xfa`, `CAPT_DRAW 0xfb`, `CAPT_CLOSS 0xfc`, `CHANGED 0xfd`,
  `UNKNOWN 0xfe`, `BROKEN 0xff`; no counters, no separate bitmaps;
* passes: `calc_broken` → `calc_captures_w/b` → `calc_mates` → `iterate()`;
* **captures are pushed from the smaller table**: `LOOP_CAPTS` runs over the index without the captured piece, places
  it, probes the subtable **once**, and `mark_capt_value` writes the result into every parent by generating the
  squares the capturer could have come from (un-captures), with an atomic compare-exchange that keeps the best value;
* `iterate()`: per ply, one call of `iter` per side to move, each a **full scan of the index range** split over
  threads (`run_threaded`). A 256-entry table `tbl[v]` decides what a value does in this ply:
  `CHANGED` → `check_loss` (all moves into wins of the opponent: **verification**), if lost write the loss and push
  wins to the predecessors (`mark_wins`, atomic compare-exchange keeping the smaller value); a win of the current
  depth → push `CHANGED` into unknown predecessors (`mark_changed`, `lock cmpxchg UNKNOWN → CHANGED`); `MATE` → push
  win-in-1. So the frontier is **implicit** (value window per ply), the candidate mark is a **value code**, and a
  ply is a single fused pass that verifies, pushes wins and pushes candidates;
* metric **DTZ50**: the count restarts at every capture or pawn move, so a capture only needs the WDL of the
  subtable (`probe_tb(..., -2, 2)`); "cursed" codes for wins and losses beyond the 50-move rule; when depths approach
  the byte, `reduce_tables` remaps the values and the loop continues;
* products: a WDL table and a DTZ table (*[bg]*: the DTZ table for one side to move only), compressed after a search
  for the piece order that compresses best, with don't-care values for positions decided by a capture.

### What each lineage got right

| | Chinook / Gilbert | Thompson / syzygy |
|---|---|---|
| slicing by irreversible moves | yes (rank of the leading man) | per material only (pawn files in the index) |
| exits resolved once, before iterating | yes (Captures, Lookups) | yes (captures, pushed) |
| metric | WDL (Trice-Dodgen: DTC) | DTZ50 + WDL |
| loss test | re-scan (pull sweeps) | verification fused into the scan |
| frontier | none, then a queue | implicit, value window per ply |
| working storage | 2 bits | 1 byte with codes |
| parallel | slices and page ranges | index ranges, atomic byte compare-exchange |
| storage | captures omitted, run-continuing fill | don't-care fill, best piece order |
| depth exact | no (in-place sweeps) | yes (level-synchronous) |

## 3. Catalogue of primitives

Each primitive with the papers that use it, and its shape as a pass. *Gather*: writes only the visited element
(race-free under `par`). *Scatter*: writes other elements (needs atomics, or a reformulation).

| # | primitive | papers | range | shape |
|---|---|---|---|---|
| P1 | broken / illegal marking | NWH 1999, NHH 2000, Herik 1985, syzygy | full | gather |
| P2 | terminal positions (no move = loss) | all | full | gather |
| P3 | conversion by pull: probe each exit | Chinook, Nalimov, Lincke-Marzetta, Romein-Bal, `egdb_6x6` | full | gather (reads finished slices only) |
| P4 | conversion by push: probe each child once, un-move into the parents | syzygy (un-captures), Stiller (broadcast), Thompson (waves, uncapture) | child index | scatter (best-value compare-exchange) |
| P5 | exits folded into a bound before iterating | Chinook Lookups {L,U,≥D,W}, Lincke-Marzetta support database | full | gather |
| P6 | win step, push from losses | Thompson, Herik, Wu-Beal, Chinook WinningMoves, syzygy `mark_wins` | frontier | scatter, idempotent |
| P7 | candidate marking, push from wins | Thompson, Wu-Beal, syzygy `mark_changed` | frontier | scatter, idempotent (bit or code) |
| P8 | verification of candidates | Thompson, Herik, Wu-Beal, Gasser 1991, Lincke-Marzetta, Romein-Bal 2003, syzygy `check_loss` | candidates | gather |
| P9 | outcounting | Gasser 1996, Wirth, Bal-Allis, Fang 2000, `egdb_fast`, `egdb_6x6` work lists | frontier | scatter, **not** idempotent (decrement; the thread reaching zero decides) |
| P10 | pull round (win and loss step by scanning undecided positions) | Ströhlein, Stiller, Chinook NoIo, Nalimov, Kishimoto-Müller, `solve_slice_pull` | full or undecided set | gather |
| P11 | push-or-pull per round by cost | Gasser 1991 (#wins · mean unmoves > #undecided → pull), Huntington-Haworth 2015 (sweeps first, then frontier), `egdb_6x6` adaptive mode | — | choice between P6–P9 and P10 |
| P12 | counter merged into the value byte | Gasser 1996 (values up from 0, counts down from 255; collided in 8-7) | — | storage trick for P9 |
| P13 | candidate as a value code instead of a bitmap | syzygy `CHANGED` | — | storage trick for P7 |
| P14 | sort and combine unmoves before updating | Wu-Liu-Hsu 2004 (≈ 600× over random disk updates) | frontier | turns a scatter into a gather over unique keys |
| P15 | delayed masking | Thompson 1996 | frontier | scatter into a superset, AND on the next sequential pass |
| P16 | score as a sum of WDL threshold problems | Lincke-Marzetta, Romein-Bal 2003 | — | outer loop over thresholds |
| P17 | partial-information lattice | Björnsson et al. 2005 | — | same passes, other operators |
| P18 | repetition rules as a greatest fixpoint | Fang-Hsu-Hsu 2002 (perpetual check) | full | monotone gather |
| P19 | finalisation (undecided → draw), DTZ50 cap | all; BTH 2005 | full | gather |
| P20 | verification pass (value = best over successors) | Nalimov, Fang 2000, Chinook, `egdb_fast`, `egdb_6x6` | full | gather + reduction |
| P21 | one side to move for a colour-symmetric material | Heinz, Nalimov, Chinook, `egdb_6x6` units | — | index |
| P22 | random access to a small RAM array only, the rest sequential | Wu-Beal (1 bit), Lincke-Marzetta (dual indexing, ≥ 1000× fewer misses) | — | layout |

## 4. Who uses what

| system | metric | working storage per position | loss test | frontier | parallel |
|---|---|---|---|---|---|
| Ströhlein 1977 | WDL+depth | bit vectors | re-scan (Boolean iteration) | none | — |
| Herik-Herschberg 1985 | DTM | 1 byte, one side to move | verification | implicit (value = n) | — |
| Thompson 1986 | DTC/DTM | bitmaps + depth byte file | verification | bitmap per ply | 12 CPUs |
| Stiller 1996 | DTM | bit arrays | complement-predecessor-complement | none | CM-2, bit-parallel |
| Nalimov 1999 | DTM | byte (16-bit depths in the later NBT code) | re-scan | none | — |
| Wirth 1999 | DTC | counter in the value cell | outcounting | changed set | — |
| Wu-Beal 1996–2002 | DTM/DTC | **1 bit in RAM**, rest streamed | verification | bitmap | SMP / distributed models |
| Fang-Hsu-Hsu 2000 | DTM | counter + best value | outcounting | implicit, full traversals | — |
| Wu-Liu-Hsu 2004 | DTM | value + counter on disk | outcounting | sorted update lists | external memory |
| Gasser 1991 | WDL+depth | RLE files | verification, or pull by cost | — | — |
| Gasser 1996 | WDL+depth | **1 byte**, counter merged | outcounting | work list | — |
| Bal-Allis 1995 | score | counter + best value | outcounting | per threshold | distributed, combined messages |
| Lincke-Marzetta 2000 | score (WDL-k) | 1 bit RAM + 2 bits disk | verification | full scans | — |
| Romein-Bal 2003 | score | 2 bits RAM + 7 bits disk | verification (recursive) | one scan | distributed, work migration |
| Chinook 1994/2003 | WDL | 2 bits | re-scan, late queue | none, then queue | slices, page ranges |
| Trice-Dodgen 2003 | DTC | 1 byte | re-scan | none | — |
| Kishimoto-Müller 2015 | WDL | — | re-scan | none | — |
| syzygy | DTZ50 + WDL | **1 byte with codes** | verification, fused | implicit (value window) | index ranges, atomic bytes |
| `egdb_fast` | DTM | per side to move: 1 byte + 16-bit counter | outcounting | bucket per level | init only (OpenMP) |
| `egdb_6x6` classic | DTM | 2 + 2 bytes + candidate bit | verification | implicit (scan for n − 1) | per slice |
| `egdb_6x6` work lists | DTM | 2 + 2 + 1 bytes + lists 4 B/entry | outcounting | lists by value | per slice; big slices inner |

## 5. Metrics: WDL, DTZ, DTM in draughts

The decisive observation for draughts: **a zeroing move in the syzygy sense (capture or man move) is exactly a move
that leaves the slice.** Inside a slice only kings move without capture. So:

| | WDL | DTZ (distance to zeroing) | DTM (distance to the end) |
|---|---|---|---|
| what an exit needs from the target | WDL | **WDL** | full DTM |
| a winning exit | win | **win in 1, decided in pass 0** | tentative win in DTM(target) + 1; a faster internal win can still beat it |
| losing exits (all moves that exit lose) | irrelevant unless no internal move | **irrelevant** unless no internal move (the loser prefers internal moves, which count one more) | the slowest one bounds the loss from below: a lost position can be decided at a depth > n |
| a drawing exit | the position cannot lose | the position cannot lose | the position cannot lose |
| exit information kept after pass 0 | none (it is in the code) | **none** (a code: "cannot lose") | the same codes, plus **deferred seeds**: "win in d if nothing faster", "the losing exits are used up in round b" |
| depth range | — | small (BTH 2005: KQBNKN maxDTM 107, maxDTC 6) | long, adds up across slices (escapes above 251 on 6×6) |
| dependency store during a build | 2 bits per position | **2 bits per position** | DTM per position (the 8-bit storage form with escapes) or the compressed store with capture resolution |
| rule relevance | — | *[bg]* the FMJD 25-move rule (only king moves without capture) and the 16-move rule count exactly these moves: DTZ is the draughts analogue of DTZ50 | analysis, books ("win in n") |
| slice self-contained? | yes, given WDL of exits | **yes, given WDL of exits** | no: depths of all finished slices flow in |

Consequences:

1. **WDL is free with DTZ** (the sign of DTZ), so a WDL-only build is never needed as a separate pass family.
   It only matters as a storage product.
2. **DTZ removes the exit-win lists** (`ewin`) and the exit-loss values (`eloss`): pass 0 decides every position with
   a winning exit, every capture position (captures are compulsory, so they have no internal move) and every
   position without internal moves; what remains carries one code.
3. **DTZ removes capture resolution from the build**: dependencies need only WDL, which can be held raw, 2 bits per
   position *including* the capture positions (their true values exist when a slice is built; only the compressed
   files drop them). `DESIGN_STATE.md` measures pass 0 at 69 % of a profiled build, of which capture resolution is
   25 % and other lookups 17 %, and the lookup memory at up to 19.7 GiB at 12 pieces for one byte per position; 2-bit
   WDL is a quarter of that, with no zstd decode and no capture search.
4. **DTM stays a separate build** with the same skeleton (section 8), and it can use the finished WDL: draws are
   known, so they are never visited; every lost position is known to be lost, so its counter is exact from the start
   and no candidate is ever wrong.
5. Invariants between the products, for verification: WDL(DTZ) = WDL(DTM); for a win DTZ ≤ DTM (the fastest end
   passes the first zeroing move or the end itself). With DTC in between: DTZ ≤ DTC ≤ DTM for every win, with the
   same wins in all three (checked on all 6-piece runs of `thompson86/`).

### What each metric needs from the endgames it leads into

DTC counts to a conversion (a capture or promotion: another material), DTZ to a zeroing move (a capture or man move:
the moves the 25-move rule resets on); both coincide when no men are left. A move that ends the count contributes one
ply whatever the depth of its target, so for such moves only the target's outcome matters, never its distance:

| build | needs from its targets | gives as a by-product |
|---|---|---|
| DTM | win / loss **with depth**, for every exit | its own depths |
| DTC | outcome for captures and promotions; for man moves (same material, another slice) the DTC depth when slices are built one by one, nothing when they share a wave | outcome of its own material |
| DTZ | outcome for every exit, man moves included | outcome of its own material |

The outcome comes in two strengths:

| | needs from the smaller endgames | gives as a by-product |
|---|---|---|
| one-sided DTC / DTZ (Thompson: White's wins and Black's losses only) | one-sided W: "does White win?", for both sides to move. A draw and a Black win are the same escape | one-sided W of its own material (the W and B planes) |
| two-sided DTC / DTZ (`egdb_6x6`, §8) | full WDL for both sides to move: negamax over the chain of §9.4 must tell a drawing exit (the position cannot lose: ≥D) from a losing one, and the loser's delay needs the losses | full WDL of its own material (the sign of DTZ) |

The total information is the same: full WDL of a material is the one-sided W of the material plus the one-sided W of
its mirror (colours swapped), Black's wins in X being White's wins in mirror(X). The difference is whether one build
covers both outcomes, or two mirrored one-sided builds each supply half and meet only in the WDL file. The store a
DTC or DTZ build reads should therefore be a two-bit WDL file, not a depth file: every exit lookup is then direct.
A one-sided depth byte holds depths that are never used, and its missing outcome costs one ply of search: a cut node
(∃, it stops at the first good successor) when the losses are stored and the wins are derived (Thompson 1996 records
Black-to-move loss distances), an all node (∀, a yes needs every successor) when the wins are stored and the losses
derived (Thompson 1986 records White-to-move win distances). Over a mirror-closed collection the two layouts make
exactly as many one-ply lookups (117,563,826 White exits against as many Black exits on the 6-piece runs); what
differs is which of them can stop early.

So a builder has **sidedness** as a parameter of its own, next to the metric: the metric decides which moves end
the count and what an exit contributes; sidedness decides which outcomes are tracked, and with them the value
encoding, the rounds (one colour's wins and the other's losses, or both), the dependency store it reads (one-sided W,
or full WDL), the symmetry (one-sided needs the run of a material and of its mirror; two-sided builds one of each
pair), and what is stored. `thompson86/` is one-sided × {DTM, DTC, DTZ} × {slice, wave}; the design of §8 is
two-sided × {DTZ, DTM} × slice.

This is also why Thompson 1986 needs one pass for DTM and two for DTC. In his single array the materials are solved
in one interleaved loop with uncaptures, and a position lost at depth j becomes known in round j. For DTM a capture
into it is worth j + 1 and is needed in exactly that round; for DTC it is worth 1 and is needed in round 1, but known
only in round j. So DTC needs the smaller endgames' outcomes complete first: his pass 1, the 1996 waves, or the
dependency order with forward lookups of §8 (where all three metrics need the order, because nothing is uncaptured).

## 6. Data per position

Per slice the working set is small on 6×6: a slice with F free squares and k, l kings has
C(F, k) · C(F − k, l) positions per side to move, at most C(18, 6) · C(12, 6) = 17.2 M (6 against 6 kings, no men).
**On 6×6 the memory pressure is the dependency store, not the working set** (`DESIGN_STATE.md` §5). On 10×10
all-king slices are large (4 against 4: C(50, 4) · C(46, 4) ≈ 3.8 · 10¹⁰), and there the bytes per position matter as
much as they do for syzygy: a 16-bit build value costs twice syzygy's byte, bought for one value type for every
metric and no range checks.

Options, from the literature:

| layout | bytes per position | source | fits |
|---|---|---|---|
| value byte with codes, implicit frontier | 1 | syzygy | DTZ, WDL |
| value byte with the counter merged into the free code range | 1 | Gasser 1996 | DTZ, WDL (if max depth + max internal moves < ≈ 250) |
| value byte + frontier bitsets | 1 + 2 bits | Thompson's bitmaps on a value array | DTZ, WDL |
| **value 16 bit with the counter merged + frontier bitsets** | 2 + 2 bits | Gasser 1996 widened; the choice here (§8) | **DTZ, DTM, WDL alike** |
| value 16 bit + counter byte + frontier bitsets | 3 + 2 bits | Fang 2000, Bal-Allis | DTM |
| 2 bits WDL | 0.25 | Chinook, Lincke-Marzetta | WDL; the dependency store |
| 1 bit in RAM, rest streamed | 0.125 | Wu-Beal | out-of-core builds (not needed on 6×6) |

### Symmetry reduction

On an even square board (6×6, 8×8, 10×10) four symmetries keep dark squares dark: the identity, the 180° rotation,
and reflection in each of the two long diagonals. Quarter turns and straight mirrors swap the colours of the squares
and do not qualify; on a rectangular board only the 180° rotation remains. Which ones apply depends on the pieces:

| slice | symmetries | reduction |
|---|---|---|
| kings only | all four: kings have no forward direction | **4×** |
| with men | only the 180° rotation with the colours swapped (the mirror `egdb_6x6` uses now), since men move forward | 2× |
| a material that is its own colour swap | in addition the side to move (`S = 1` in the units) | a further 2× on those materials |

The 4× falls exactly where it is needed: the all-king slices are the largest, and the ones that outgrow RAM on 10×10
(4 against 4 kings: 3.8 · 10¹⁰ positions per side to move, about 9.5 · 10⁹ folded, 19 GB as 16-bit cells). On 6×6 the
same four symmetries apply to the kings-only slices, but those are at most 17.2 M positions per side to move, so
folding them saves memory that is not needed.

Folding the index inside a slice brings back two pitfalls from the literature:

* **counters** (Thompson 1996): a counter counts *moves out of* a position and must equal the number of *unmoves into*
  it. In a folded index a position fixed by a symmetry can have two successors that are images of each other and fold
  onto one stored position, so the two numbers differ and a counter set from the move count never reaches zero.
  On folded slices, either set the counters by counting unmoves into the folded index (an extra full pass, as
  Thompson notes), or use the verification variant of the loss step (§8.2), which rechecks all moves and needs no
  counter;
* **images** (Herik-Herschberg 1985): their first run was wrong because a position on a symmetry axis has fewer
  distinct images, and not all of them were written. With only the canonical form stored this cannot happen, but every
  move and unmove must go through the canonical form, and the canonicalisation must be tested on exactly the positions
  on the axes. The unit build's check "mirrored positions disagree" is this test for the 2× mirror.

The cost per move is the canonical form: up to four images computed and the smallest kept. So: keep the index unfolded
inside slices with men (2× by the mirror, at storage, as now); fold the kings-only slices 4× once they are large.

## 7. Rules for one `for_each` per pass

1. **Two tables per slice, one per side to move.** Every move goes from one table to the other. A pull over
   table s reads only table 1 − s and writes only table s: race-free and exact without double buffering (Chinook's
   sweeps alternating the side to move, syzygy's `table_w` / `table_b`). A push from table s writes only table 1 − s.
2. **Iterate over 64-position words, not positions.** The element of the `for_each` is a word index; the body loops
   over its 64 positions (or its set bits). Then every write to a bitset at the visited index stays in a word owned
   by the element, without atomics, whatever chunking the policy picks. A `for_each` over single positions cannot
   promise that, because `par` chooses the chunk boundaries.
3. **Scatters are idempotent or claim with compare-exchange.** Writing W(n) into an unknown predecessor, marking a
   candidate, setting a frontier bit: `std::atomic_ref<std::uint16_t>::compare_exchange` on the value, `fetch_or` on
   the frontier word. A counter decrement is a compare-exchange loop that leaves decided values alone; the thread that
   takes it to zero decides the position and is the only one that can. Relaxed ordering is enough inside a pass; the
   end of the `for_each` is the synchronisation point between passes. Under `seq` these cost almost nothing, so the
   same code serves both policies. (Raw word access for `xstd::bit_set`, or an atomic insert, is needed for this;
   check what the library offers.)
4. **Level synchrony by thresholds, not buffers.** A round n accepts only values ≤ n − 1 as decided successors, so
   results of the same round (value n) never leak into it, even though they are visible.
5. **Nesting.** Slices of one (pieces, men, distance) group are independent: outer `for_each(par)` over small slices
   with `seq` inside; a big slice alone with `par` inside. The policy is a type, so the slice solver is a template on
   it and the scheduler dispatches per slice.
6. **Random reads anywhere, random writes only in RAM.** Every pass declares its read-only inputs and its outputs
   with their access pattern. Inputs may be read sequentially or randomly, from RAM or from disk: random disk reads go
   through the page cache or a block cache, and cost little with a slice's locality (measured on a memory-mapped 6×6
   layer: 3–8 µs per cold lookup, `compression_mmap_10.txt`). Outputs on disk are written strictly sequentially, in
   index order, with no exception for writes that happen to be dense. Random writes go to RAM only, to avoid
   thrashing the SSD: every scattered write dirties a whole 4 KiB page (the drive writes 4096 bytes for one), pages
   may be flushed and rewritten repeatedly, the drive wears, and after an interruption nobody knows which scattered
   writes reached the disk. A pass whose output is scattered therefore builds it in RAM and writes it out in order.
   (This is the rule for SSDs; an HDD may take random writes, at a seek each, and an SSD within a write budget: §7.4.) In the passes of §8 the only scattered writes are the rounds'
   pushes into predecessors and next-frontier bits, into the slice's arrays in RAM. When a slice no longer fits in
   RAM (all-king slices on 10×10), the remedies are the 4× symmetry reduction of kings-only slices (§6), a smaller
   cell, or Wu-Liu-Hsu's batching: the round appends its updates as (target, value) pairs, sorts them by target
   (externally if needed) and applies them in one sequential pass over the values; the only one of these that removes
   the scattered writes altogether. The current code breaks the rule in one place: for a symmetric
   material, `unit_build` / `mat_build` write the Black-to-move half through the mirror
   (`u.p[stored_offset(r, mirror(pos))] = b`), scattered over the mirrored slice's range of the mapped file. The fix
   needs no buffer: turn the scatter into a gather. The mirror is its own inverse, so walk the mirrored slice's range
   in index order and fetch each slot's value from the solved slice still in RAM (`tmp`): the random access moves to
   the read side, in RAM, and the disk write becomes strictly sequential. For a slice that is its own mirror the
   target range is its own White-to-move half, already written; there the gather becomes the existing consistency
   check (compare instead of write). The general form: when an output would be scattered, gather it instead (for
   each output slot in order, compute where its value comes from), whenever that inverse map is cheap.

### 7.1 When the build outgrows RAM: what each iteration writes

Rule 6 settles where random writes may go; this settles what a round must write to disk when a material no longer
fits in RAM. The state of a round splits in two (notation of §9.6, for a one-sided build; the same holds per side
to move for a two-sided one):

* **bit planes**, a few bits per position: the cumulative `isW-wtm` and `isL-btm` of the material being built
  (Thompson's W and B), the frontiers of the previous level (W_n, B_n) and the loss candidates (Thompson's J).
  Verification reads W and B at random; the pushes write the next frontiers and the candidates at random.
* **depth bytes**, 8 bits per position: `dmW-wtm` and `dmL-btm`. During the build of their own material they
  are only written, never read: a position's depth is the level at which it entered W or B. Only the builds of other
  materials read them, later, through memory maps.

So the depth files can be write-once, and the regime is decided by the bit planes:

| regime | bit planes | depth bytes | written per iteration |
|---|---|---|---|
| all in RAM (the 6×6 builds today) | RAM | RAM, one sequential write per file at the end | nothing |
| depths outgrow RAM | RAM | on disk, assembled at the end | the newly resolved positions only |
| bit planes outgrow RAM too (Thompson's regime) | on disk, random reads through a cache | on disk | the new positions, plus `W \|= W_n`, `B \|= B_n` as full sequential passes (2 × T/8 bytes) |

Every iteration produces new depth information, so once the depths are on disk every iteration must persist
something. Three ways to persist level n, with T positions and L levels:

| way | each iteration writes | over the build | at the end |
|---|---|---|---|
| merge pass (Thompson 1996's record step, `BTM \|= B; RES = res(B, n)`) | the whole byte file: read the old file and the frontier, write the new file | L × T bytes, plus as much read | nothing |
| dense frontier bitmaps | W_n and B_n as sequential files of T/8 bytes each, even when the frontier is nearly empty (the late levels) | L × T / 8 bytes per side to move | one pass reads the L bitmaps side by side, all sequentially, and writes each depth file once |
| frontier index lists | the sorted indices of the newly resolved positions, delta-coded, appended sequentially | about one small entry per resolved position: every position is resolved exactly once | an L-way merge of the lists writes each depth file once |

For scale, the 6-piece `thompson86` dtm build as if each material were one group (2 × 97.8 M slots, 32.4 M
resolved positions, up to 69 levels): merge passes 69 × 196 MB ≈ 13.5 GB written, plus as much read; dense frontier
bitmaps 69 × 2 × 12.2 MB ≈ 1.7 GB; index lists 32.4 M × about 2 bytes ≈ 65 MB; the final depth files 196 MB. With
lists the per-iteration writes cost less than the final files themselves, nothing written is ever rewritten, and the
depth files are assembled in one sequential pass. **Aim for the middle regime**: bit planes in RAM, frontier lists
appended to disk, one merge at the end. Only when the bit planes themselves do not fit does Thompson's machinery come
back: the cumulative bitmaps rewritten every level, and their random reads served from disk through a cache (or
batched as in rule 6).

### 7.2 Graceful memory degradation: the memory–algorithm hierarchy

A builder should not fail when a material outgrows RAM; it should switch to the next algorithm down a ladder whose
rungs keep less in memory. Three lineages have published such ladders, with their measured limits.

**Lincke (ETH thesis 2002, §2.6; Lincke–Marzetta 2000): bits per position kept in RAM**, for Awari databases on a
1 GB machine (values are scores, decomposed into WDL(k) threshold problems; WDL only, no depth):

| RAM, as a share of one byte per position | algorithm | built that way |
|---|---|---|
| all | one byte per configuration | up to 28 stones |
| ≥ 1/4 | two bits per configuration (W, L, D, U) in RAM | 29–30 stones (2.3, 3.1 GB) |
| ≥ 1/8 | one bit in RAM (W, U) + two bits on disk (W, L, D, U), the disk only streamed | 31–33 stones (4.3–7.7 GB) |
| ≥ 1/80 | the RAM bit cached (LRU, 16 KB blocks), with **dual indexing**: the RAM bits indexed for the other side to move, so successors land close together; ≥ 1000× fewer misses | 34–40 stones (10–47.6 GB) |

The rule behind the ladder: keep in RAM only the state that is accessed at random (for the losses, the win bit:
the one-ply forward look reads it, the push of new wins writes it), and stream everything else in index order.

**Wu–Beal (1996 TR, IS 2001, MGoNC 2002, HPDC 2001): random-access bitmaps per position**, for Chinese chess with
exact DTM or DTC for both sides to move (the byte databases W and B on disk, only streamed):

| algorithm | randomly accessed in RAM | per position |
|---|---|---|
| Thompson 1986 | two bitmaps | about 2 bits (1/4 of the byte database) |
| Wu–Beal 1992 | the byte database + one bitmap | 9 bits |
| Wu–Beal one-bitmap (1996–2002) | one bitmap R | 1 bit, or 1 bit per two positions with the sides processed alternately |
| the same, spare memory | extra bitmaps kept in RAM | fewer sequential database scans per level |

Depth survives because every level is a fixed script of streamed bitmap algebra: `Load(W, WIN_IN_N(n))` extracts
the frontier from the byte database, `Predecessor` pushes it into R, `ProveSuccessor` verifies the candidates against
`Load(W, WIN_≤N(n))`, `Add` streams the new layer back into the database. That is §7.1's merge pass: several full
sequential scans of the byte databases per level, paid for one random bit. 92 h for aegp-aaee (about 7 G entries
with its 143 subgames) on 128 MB.

**Wu–Liu–Hsu (CG 2004): nothing at random**, for databases with more positions than RAM has bits (8.8 G positions,
1 GB RAM): values and child counters on disk, every scattered update appended to a list, externally sorted, combined
by multiplicity, and applied in one sequential pass per layer (Allis-style layered counting). RAM holds only the sort
runs. Sorted updates measured 30–600× faster than random disk updates (10^6 updates: 3787 s random, 6.5 s sorted);
sorting is about 4 % of the time; restartable per layer.

**One ladder for our builders.** Two knobs: the **group** (the cumulative state is needed only for the group being
solved; everything outside it is a lookup into finished files, random reads, allowed by rule 6) and the **bits per
position in RAM** for that group. Taking the rungs in order as a group outgrows memory:

| rung | in RAM per position of the group | on disk | depth | from |
|---|---|---|---|---|
| 0 | everything (depth bytes, bit planes, seeds) | finished files only | exact | `thompson86`, `tbgen`, `egdb_6x6` today |
| 0′ | 1 byte: the 8-bit cell, statuses inside, rounds by scanning (§7.5) | snapshots, one per 240-ply window | exact | rtbgen, made value-preserving |
| 1 | bit planes (cumulative W/B, frontier, candidates): a few bits | depth as appended frontier lists, one merge at the end | exact | §7.1 |
| 2 | the same, for a smaller group: wave → slice (cheap once the half results are stored, since a move out is one read) | | exact | `thompson86`, `tbgen` (`granularity=tempi\|configuration`), `egdb_6x6` |
| 3 | 1 bit: the cumulative win bit; candidates and losses streamed | the byte database, merged per level (Wu–Beal), or the W/L/D/U state streamed (Lincke) | exact (Wu–Beal) or WDL (Lincke) | Wu–Beal, Lincke |
| 4 | a cached fraction of 1 bit, dual-indexed for locality | as rung 3 | as rung 3 | Lincke |
| 5 | sort runs only | everything; scatters as sorted batches | exact | Wu–Liu–Hsu |

Before rung 3, the 4× symmetry of kings-only slices (§6) shrinks the largest groups for free. Each rung keeps the
pass code and changes only where its inputs and outputs live, so a memory budget can pick the rung per group from
the sizes known before the build (`Meta.P` of the slices). Both builders now reach rung 1 (a depth sink with
group-order files, `sink=auto` by the job's claim). `tbgen` first had to split its 16-bit value array, which was both
depth store and cumulative state (about 7.5 bytes per position and side to move against rtbgen's 1 byte), into
cumulative bit planes for the state and write-only values in the sink; its frontier rounds now hold about 10 bit planes (one for the delayed check's candidates)
and the level lists, its scan mode (rtbgen's) still the value array, which its frontier scans. Both also offer groups
finer than the material class (`granularity=tempi|configuration` in both builders), rung 2.

### 7.3 Choosing by machine

A build is bound by CPU, RAM and I/O at once, and which of them binds depends on the group as much as on the machine.
Chinook's network-of-workstations build (Lake–Schaeffer–Lu 1994) ran workers of 8–15 MB with a 3–4 MB working set of
8 KB pages under LRU, chose forward sweeps *because* their writes stay local, ran the backward queue sequentially for
lack of locality, and made up 75 % of the department's network traffic; its 2003 successor reports the capture,
lookup and verification passes I/O-bound and the non-capture iteration CPU-bound, and notes that the binding resource
moved in a decade: "Some parts of the program that were previously I/O bound are now CPU bound (more memory to
eliminate costly I/O), while other parts that were CPU bound are now I/O bound (CPU speed has improved more than disk
speed). This meant that we had to re-profile the program". Ed Gilbert's two builds bracket
the range (`edgilbert.org`, notes in [`papers/3_checkers.md`](papers/3_checkers.md)):

* **10-piece English checkers (2005), RAM-bound**: five PCs with 1–2 GB of RAM, 120–250 GB disks, a 100 Mbit LAN;
  4.2 computer-years in 11.5 months. Every subdivision was cut until it fitted a 2 GB buffer and 32-bit indices,
  because beyond RAM "accessing the data on disk becomes horribly slow": his program could build a subdivision larger
  than RAM, but the all-kings slice took "only 2 days instead of several weeks" once mirror symmetry halved it into
  2 GB.
* **8-piece international draughts (2009), CPU-bound**: 16.5 trillion positions on 12 cores (a quad-core with 8 GB,
  then a dual quad-core Xeon); 2.65 CPU-years to build and 2.91 to verify in 8.8 months, about 198,000 positions per
  second per core. "Parallelizing the build is critical": about 200,000 subdivisions, typically hundreds buildable at
  any moment, enough for two or three times as many cores.

So the choices of §7.1–7.2 and of `seq` or `par` should not be made by hand per build, but by a planner
from a description of the machine and the sizes known before the build.

**The machine**, a small description file:

| quantity | used for |
|---|---|
| cores | `par` within a group against more jobs side by side |
| RAM, minus a reserve for the page cache of finished files | the job claims and the rung per group |
| per storage tier (RAM, SSD, HDD, network): sequential bandwidth, random-read and random-write latency, whether rotational, endurance (TB written) | where temporary files, lists and finished files go; whether random reads into finished files are cheap; which rungs are affordable (§7.4) |

Scattered writes go to RAM by default (rule 6): on an SSD every one rewrites a 4 KiB page and wears the drive, on
an HDD every one costs a seek. Random reads are cheap from RAM and the page cache, tolerable from an SSD, expensive
from an HDD. §7.4 refines this per disk: an HDD may take random writes (a seek, no wear), an SSD within a write budget
(its endurance), and the plan budgets the I/O as well as the space.

**Known before the build**, from the slice sizes (`Meta.P`) and the DAG: positions per group and per material, the
sizes of the finished files a material reads, the number of groups, and roughly the number of levels (bounded by the
byte; the previous metric's build gives a better estimate).

**Cost per pass, and the constants measured so far** (6 pieces, 4 cores, this machine):

| pass or choice | work | access | measured |
|---|---|---|---|
| whole build, `seq` | about 2P positions per group, moves generated, exits looked up | CPU, random reads into finished files | `thompson86` 0.35 µs per position; frontier `tbgen` 0.34 µs (dtm) and 0.26 µs (dtz) per settled cell |
| pass 0 lookups into finished files | a few per position | random reads | page cache: part of the above; cold memory map: 3–8 µs per lookup (§7, rule 6), so pass 0 explodes once the inputs fall out of the cache |
| rounds by full scans instead of frontiers | 2 × 2P per round, L rounds | sequential reads of the group | `tbgen` dtm +46 % (53.1 s against 36.4 s), dtz +14 % |
| depth sink, group-order files instead of the array | the gathering pass, slice lookups | sequential writes, random reads in RAM | +10–15 % CPU; the largest material's peak memory 19.3 MB instead of 24.9 MB |
| `par` within a group | the same work, split in blocks of 4096 positions | | whole-material groups: 2.0× on 4 cores (43.0 s → 21.1 s); configuration groups: none, only thread-pool overhead |
| scattered updates on disk | one per update | random against sorted | Wu–Liu–Hsu 2004: 10^6 updates 3787 s random against 6.5 s sorted |
| a group larger than RAM, cached from disk | the same work | random reads and writes through a cache | Gilbert 2005: the all-kings slice "several weeks" against 2 days in RAM |
| a value needed by many parents, searched each time | one search per parent | CPU | Gilbert 2009: verification slower than the build because capture successors were re-searched per parent ("would nearly double the speed" if cached); `thompson86`: storing `dmL-btm` cut dtm from 51.1 s to 34.3 s |

**The decisions, in order:**

1. **Granularity.** The knob that sets everything else: fine enough that a group fits its claim at a high rung, coarse
   enough that per-group overhead (files, maps, metadata, thread-pool start-up) stays small; for us `tempi`. Gilbert cut by piece
   counts and leading ranks, then by the configuration of the leading rank, then by the rank of the penultimate
   checker, but only above a 900 MB threshold: 64,369 subdivisions instead of almost 3 million, some 20 million files,
   on which "the build program will run out of file handles and choke"; our first group-file sink hit the kernel's
   65,530 memory maps for the same reason. Finer subdivision also feeds parallelism: hundreds of independent
   subdivisions kept 12 cores busy in the draughts build.
2. **The rung per group**, the highest that fits the job's claim (§7.2).
3. **The policy per group**: `par` when the group has several blocks per core, `seq` otherwise, with the parallelism
   taken from jobs side by side or from independent groups of one wave.
4. **Jobs side by side**: Snakemake with `--cores` and `--resources mem_mb=` (RAM minus the page-cache reserve), each
   job claiming its rung-0 need capped at that, so a big material waits for the whole machine instead of four jobs
   squeezing each other down a rung; priorities by the critical path of the DAG.
5. **Placement**: temporary group-order files and frontier lists on a tier with fast sequential writes; finished files
   where random reads are cheap. On an HDD, pass 0 sorts its lookups per finished file, turning them into a
   sequential read; for dtc and dtz the outcome files (8× or 4× smaller than the bytes) are what must stay cached.

**Which resource binds, and the response:**

| regime | binds | response |
|---|---|---|
| small groups, inputs cached | CPU (move generation) | more jobs side by side |
| a big group in RAM | CPU | `par` within the group |
| inputs larger than the page cache | random reads | sort the lookups per file; keep the outcome files cached; SSD rather than HDD |
| a group larger than its claim | RAM | a lower rung (§7.2) |
| at the bit-in-RAM rungs | sequential bandwidth: a few full streams per level | fewer levels per stream (Wu–Beal's spare bitmaps), faster tier |
| on an HDD | anything random | only the sequential rungs; sorted batches |

**The scheduler is not new either.** Lake–Schaeffer–Lu 1994 (§5.1 "Creating a Supercomputer", §5.2 "A Day in the
Life of a Network Supercomputer") built one by hand, because "keeping 30 or more machines busy required too much
manual labor":

| Chinook 1994 | Snakemake |
|---|---|
| a job queue file, one line per (pass, slice or mini-slice): `cap`, `winmove`, `lookup`, `noioo`, `noiom`, `que`, `build`, `ver` | rules, one job per wildcard value |
| `addq` splits a slice into page-aligned mini-slices of about 10 million positions ("manually adding work to the queues occasionally introduced errors") | the planner generating the jobs |
| dependencies known to the shell script (`noiom` waits for `noioo`, `que` for `noiom`, `build` for `que`), other slices' work inserted by hand between them so no processor waits | the DAG; ready jobs are found and started automatically, ordered by priority |
| four queues by machine class: `prime` (fast, enough memory, always available), `non-prime` (the same with daytime restrictions), `slow`, `small` (8 MB or less: mostly `DoCaptures`) | resources (`mem_mb`, cores) and per-machine profiles |
| a lock file per running job, output checked on exit, failed jobs "manually inspected" and reinserted from the start or the last checkpoint | job states, incomplete outputs, logs, `--rerun-incomplete`, `--retries` |
| a ping every minute; a crashed or rebooted machine's last job reinserted at the top of the queue, with a mail | executors for clusters; on one machine, not needed |
| an activity monitor suspending or killing jobs on staff workstations during the day | (not Snakemake's job: OS priorities) |

The difference in unit is instructive: Chinook's job was a pass over a mini-slice of a slice, so many machines could
share one big slice (disjoint, page-aligned write intervals, made possible by forward sweeps), at the price of
cross-machine dependencies between passes. Our job is a whole material on one machine, with the parallelism inside it
from `par` and across it from the DAG; splitting a material across machines would bring Chinook's pass-level jobs
back.

Chinook 2003 automated the dependencies and the hardware specialisation with TrellisDAG ("run I/O-intensive jobs on
machines with fast disks; run CPU-intensive jobs on machines with fast processors"), but not the data: "managing the
data turned out to be labour intensive and a source of potential errors. We were unable to find or build a usable tool
that could properly manage the data file dependencies, taking into account disk space constraints, in such a way as to
maximize throughput." File dependencies are what Snakemake is built on; disk space can be declared as a resource and
intermediate files marked temporary.

**To build**: the machine file (now `databases/machines/*.yaml` and the configure step's `machine.yaml`, read by
`src/egdb_layout.py --machine`, any key overridden on the command line to see what an upgrade buys); a planner (an extension of `strongly_solving_6x6/materials/materials.py`, which
already computes sizes and critical-path priorities) that assigns each job its claim, priority and policy and each
group its rung; engines that take those decisions as arguments and report, per pass, the measured time per position,
lookups per position and cache misses, so the constants above are calibrated per machine rather than assumed.

### 7.4 Disks: HDD and SSD, and the I/O a plan budgets

As RAM per group drops, I/O rises, not smoothly but in steps that get steeper down the ladder of §7.2:

| RAM per group | rung | I/O per build | cost |
|---|---|---|---|
| everything | 0 | read the inputs, write each file once, sequentially | the minimum |
| depths out | 1 | the same, plus group-order files written once, sequentially, and read back by one gathering pass | small: +10–15 % CPU at 6×6 |
| smaller groups | 2 | the same files; moves out of a group become lookups into finished files | small while those stay in the page cache |
| 1 bit per position | 3 | every iteration streams the other bit planes and the depths: about a dozen plane streams per level | I/O ∝ levels × size, sequential |
| less than 1 bit | 4 | RAM is a cache on the one randomly accessed plane; every miss goes to the disk | with RAM = r × the plane, about (1 − r) of the random accesses miss |
| a fraction of that | 5 | the scattered updates appended, sorted, applied in one sequential pass (Wu–Liu–Hsu) | more volume, sequential again, plus the sort |

Per level, `T_io ≈ S_seq / bandwidth + N_rand × (1 − r) × t_access`: S_seq the bytes streamed, N_rand the random
accesses (about the frontier times the moves per position), r the cached fraction of the randomly accessed plane.
Rung 3 makes the first term large, rung 4 the second. The early steps are nearly free (granularity and the depth sink
lower RAM without touching the per-level I/O, the extra lookups going to read-only files the page cache serves well);
the last ones are what took Thompson's and Gilbert's builds weeks to months.

**The two kinds of disk differ in what is expensive:**

| | random read | random write | sequential | wear |
|---|---|---|---|---|
| HDD | one seek, 5–10 ms (100–200 per second) | the same as a read | 100–250 MB/s | none |
| SSD (SATA / NVMe) | 20–100 µs (50 k / 500 k+ per second) | fast, but at least a 4 KiB page rewritten per update: 32,768× amplification for one bit | 0.5 / 3–7 GB/s | rated in TB written (a consumer 1 TB drive about 600 TBW) |

So rule 6 ("random writes only in RAM") is the rule for SSDs, where the limit is endurance, and a default rather than
a law: an HDD may take random writes (they cost a seek, not wear), and an SSD may take them within a write budget.
Per disk, the lowest rung:

* **SSD, no random writes**: rung 3. In each pass only one plane is written randomly (the scatter's target: W from the
  unmoves of B, the candidates from W; Thompson 1996's "exactly one random-access stream per pass"), so only that
  plane needs RAM, flushed sequentially after the pass; verification writes its own bits in source order, merges are
  sequential, depths are appended as frontier lists (§7.1), and delayed masking turns the scatter's filter into a
  sequential AND. With the finest granularity the floor is **1 bit per position of the largest configuration of men
  with all its king placements**, kings-only materials divided by their 4-fold symmetry: 8×8 checkers to 10 pieces,
  485 MiB (the all-kings 5v5 ÷ 4), 303 MiB for the largest configuration with men (`src/egdb_layout.py`). Below that
  without random writes: pull passes (W_{n+1} = "has a move into B_n", a full sweep in index order writing its own
  bits, reading B_n randomly, syzygy's `scan` mode) or sorted batches.
* **SSD with a write budget**: rung 4 with a write-back cache that coalesces a page's updates before writing it. The
  writes are about the misses on the written plane × 4 KiB; 600 TBW is about 1.5 × 10^11 page writes, ample for a
  coalescing cache, not for a small one under a large build.
* **HDD**: rung 4 is allowed, but every miss costs a seek, so it pays only while misses are rare; below the floor the
  better way is rung 5, whose logs, sorts and merges an HDD streams at full speed.

**Thompson's machines, replayed** (his own table: 1985, 1 MIPS, 16 MB, 500 MB disk, 1 MB/s; 1996, 150 MIPS, 250 MB,
50 GB, 10 MB/s):

| | 1986: 5 pieces, pawnless (KQKBB) | 1996: 6 pieces, pawnless (KRBKBN) |
|---|---|---|
| index, one bitmap | 462 × 64^3 = 121 M slots (87 % legal), 15.1 MB | 462 × 64^4 = 7,751 M slots (80 % legal), 969 MB |
| RAM | 16 MB, of which 1 MB per processor as cache (about 4 MB) | 250 MB = 0.26 bitmaps |
| working files | W, B, W_i, B_i, J and the result bytes: 197 MB, 39 % of the disk | B, W, X, BTM, WTM and RES: 12.6 GB, 25 % of the disk |
| rung | 4: RAM a cache on the one random stream | 4 |
| sequential I/O per iteration | at least 424 MB, 7 min | at least 27 GB, 45 min |
| bound for the build | 71 iterations: 0.3 days | 70 iterations: 2.2 days |
| reported | 2–3 weeks per endgame | wave 6 about 3 months |

Both ran with RAM about a quarter of one bitmap, on disks with ~10 ms seeks, and both took 40–70× their sequential
bound: the random stream's misses, three in four, dominated ("CPU gained 150× but I/O only 10×"). Even the 1-bit
floor did not fit, so his design (bitmaps on disk, RAM caching the single random stream) was the only one possible;
`thompson86` at rung 1 would have needed 3.3 GB in 1986 and 212 GB in 1996.

**A comfortable machine** holds the largest group, one configuration of men with all its king placements (kings-only
materials divided by their 4-fold symmetry), at 1 depth byte and 4 bit planes per position and side, both sides in
RAM: then every group stays at rungs 0–2 (granularity makes the rest fit), and the I/O is its minimum, the files once
and the inputs. `src/egdb_layout.py` reports it next to the 1-bit floor:

| | largest configuration (per side) | floor: 1 bit | comfortable: 1 byte + 4 planes, both sides |
|---|---|---|---|
| 6×6, 6 pieces | 93 k | 11 KiB | 272 KiB |
| 8×8 checkers, 10 pieces | all-kings 5v5 ÷ 4: 4.1 G | 485 MiB | 11.4 GiB: a 16 GB desktop |
| 10×10 draughts, 8 pieces | all-kings 4v4 ÷ 4: 9.4 G | 1.1 GiB | 26.3 GiB: a 32 GB machine |
| 10×10 draughts, 9 pieces | all-kings 5v4 ÷ 4: 78.9 G | 9.2 GiB | 220 GiB |

Gilbert's 2005 checkers build ran on 2 GB machines (rung 4 and half the symmetry for the all-kings slice), his 2009
8-piece draughts build four instances on one 8 GB machine (the page gives no memory per instance and says nothing about
the all-kings subdivisions). The model reproduces his position count, 4×4 + 5×3 + 3×5 = 6.35 + 5.08 + 5.08 = 16.50 T
per side to move ("16.5 trillion"); the largest group, all-kings 4v4, is 37.6 G positions per side, 8.8 GiB at 2 bits
(too large for 8 GB even with one instance alone on the machine, which the largest material surely had: 8 GB allows
1.7 bits per position), so it took symmetry (one diagonal: 4.4 GiB; both: 2.2 GiB), WDL packed five positions per byte
(1.6 bits, 7.5 GB, just fitting), or one bit per position and side at a time; the page does not say which. So in 2009 memory hardly bound, as he says ("CPU-bound"): his 200K subdivisions were for parallelism
("hundreds of subdivisions that can be built in parallel", enough for 12 cores), not for memory. A planner therefore
needs a second criterion: once the groups fit, refine further until the DAG is wide enough for the cores, as Gilbert
did by hand and Chinook 1994 with its mini-slices. `src/egdb_layout.py --cores` does it by a list schedule (a group's
time its positions, critical path first), halving the group limit until the target utilization is reached. For his
build (4×4, 5×3 and 3×5, 8 GB, one diagonal, 10 cores, his 197,529 positions per second per core): the coarsest layout
that fits, 2,154 groups, keeps the cores 65 % busy (149 days); limits of 16 G and 8 G positions give 2,738 groups at
87 % (111 days) and 4,136 groups at 96 % (101 days), fifty times fewer than his 200K subdivisions; the total work, 964
days on one core, is his 2.65 CPU-years. For his 2005 checkers build (900 MB, 5 machines) the memory layout alone keeps
the cores busy: there, memory was what bound. Both databases are now comfortable on a desktop, CPU-bound, at minimal
I/O. Gilbert's 12-piece checkers database (July 2026: "1.7 years to build and self-verify using a 16-core Xeon
workstation with 192 GB of ECC RAM") is the comfortable regime at scale: its largest group, the all-kings 6v6 (209 G
positions per side), takes 24–97 GiB for WDL depending on the symmetry used, within 192 GB. It holds every position
with at most 6 pieces per side (6v5 at 11 pieces, 6v6 at 12), as his 10-piece one held at most 5 (5v5 at 10): 528.2 T
against 10.9 T positions per side to move, 48.4 times, his "48 times". That is about 1.63 µs per position per core for
building and verifying together, some six times faster per core than in 2009, and about 180 positions per byte on
disk. At 10×10 and 9 pieces, the 16 kings-heavy materials that do not fit 32 GB take rung 3 or a coalescing
cache.

**The cost per position depends on the game**, so the plan measures it rather than carry a constant over: the
engine's `bench` samples every material and times the move generator (the core of every pass), with the branching
factor and the shares of captures and promotions (pass 0's lookups into other materials). One ratio, fixed by the
measured builds (thompson86 1.23, tbgen 2.39 move generations per position and side), turns a bench of any rules into
a cost per position. On the same 6×6 board, engine and machine, international draughts costs 0.284 µs per move
generation with 4.07 moves (24.5 % captures, 5.3 % promotions), English checkers 0.153 µs with 3.44 (15.2 %, 8.4 %):
flying kings, long captures and men capturing backwards make the first nearly twice as dear. Larger boards cost more
per position again (longer rays, more pieces to scan), so a build on another board needs a bench of an engine for it.

**Better still, every build is a calibration run**, as a profile-guided build: both engines time their passes and
count their levels and frontiers (a few popcounts per round), one `stats` line per material, and the plan extrapolates
from a small build of the same rules, engine and machine (fewer pieces) to the large one: the cost per position,
geometrically from the last two piece counts. Calibrated on thompson86's build to 5 pieces, the 6-piece materials come
out at 0.40 µs per position and side (measured 0.419) and about 36 s of CPU (40.5 s); tbgen's three metrics on 4 cores at 32 s, as measured (36.4 + 33.3 + 28.0 s of CPU). Gilbert's "average
build speed per CPU, 197,529 positions/sec" (5.06 µs per position on 2.4–2.5 GHz cores) is this kind of number:
whole-build throughput, his 16.5 T positions over 2.65 CPU-years, his passes, caching and I/O included.

**The number of passes cannot be predicted.** The levels are set by the longest win, an outcome of the build: the
10-piece checkers databases hold a win of 279 plies that nothing in the 9-piece ones foretold, and a linear fit of
thompson86's levels from 5 to 6 pieces gave 63 for dtc where the build took 43. Frontier builds (rungs 0–2) do not
depend on it: every position enters one frontier once, so their work grows with the positions, and the per-position
cost is what extrapolates. Whatever costs a full pass per level (rtbgen's scans, Chinook's sweeps, the streams of
rung 3) gets a range instead: at least the levels of the calibration build (larger materials are not shallower), at
most what the encoding holds (509 plies for dtm, 255 for dtc and dtz; WDL has no such bound). One more reason to stay
at the frontier rungs.

**What the plan budgets.** Not only disk space (cumulative files plus the scratch of the jobs side by side) but the
I/O: the configure step of `databases/` reports, for rungs 0–2,

* writes: every file once, sequentially; up to the group-order files on top, at rung 1, written once and read back;
* reads: each build's inputs through memory maps, at best each input once (the page cache keeping it for every build
  that reads it), at worst once per build that reads it; the checks read every file once;
* the disk time at the disk's sequential speed (`DISK_MBPS`, by default 150 MB/s for an HDD, 500 MB/s otherwise), and
  for an SSD the writes against its endurance (`DISK_TBW`).

`tbgen` to 6 pieces, all three metrics: 404 MB written (1.1 GB with group files), 590 MB to 1.5 GB read, 6 to 17 s of
disk time at 150 MB/s, 1 ppm of a 600 TBW drive. At the lower rungs the plan adds the per-level streams of rung 3 and
the random accesses of rung 4 (frontier × moves × (1 − r) × t_access): a build whose plan shows those terms dominant is
one to move to an SSD, to a finer granularity, or to sorted batches.

### 7.5 The 8-bit cell: one byte per position, reductions, value-preserving snapshots

Without outcounting (the tbgen builder: a loss candidate is verified forward, as rtbgen's `check_loss`), the build
needs **one byte per position and side to move**, and no bit planes: rtbgen's design, with its reduction step made
value-preserving and aligned with the storage escapes of §10.

**The cell.** S = 16 of the 256 codes are reserved, W = 240 are depth codes of the current window. Undecided is
**0**, so the cell array needs no initialisation pass: fresh anonymous memory (`mmap`, `calloc`) is all zero pages
(§9.3). The depths take 1 … 240, the other reserved codes 241 … 255:

| code | meaning |
|---|---|
| 0: undecided | the default; what is left at the end is a draw |
| `WON`, `LOST` | decided in an earlier window: the class only (to skip it, and for `check_loss`: is a successor won?) |
| not lossable | undecided, but an exit draws or wins (rtbgen's `CAPT_DRAW`; saves probing the exits again in every check) |
| `FRONT_WIN`, `FRONT_LOSS` | the last ply of the previous window, the frontier of the first round after a reduction; they become `WON`, `LOST` as that round's scan passes them |
| `DRAWN` | decided in pass 0 as a draw. Needed because the unmoves do not test compulsory capture: a position whose captures all leave the group (no move inside) still turns up as a predecessor, and must not look undecided |
| `PENDING` | undecided and lossable, but not before its slowest losing exit's level (DTM; in DTZ and DTC that level is 1); it becomes undecided (0) when the level comes round |
| `DEFER` | decided in pass 0 at a depth beyond the current window (DTM: captures into deep materials); recomputed from its exits when its window comes |
| `CHANGED` | a loss candidate of the current round (rtbgen's mark): the predecessors of the previous ply's wins are marked first, then each candidate is checked once, against all its successors, and becomes a loss or undecided again |
| 6 spare | never used in the build; they exist as storage tokens (§10) |
| 1 … 240: W depth codes | code c is offset c − 1 in the window. DTM: one code per ply (a win odd, a loss even: 240 plies per window); DTZ, DTC: two codes per ply, win and loss (no parity: 120 plies per window) |

What the cell does not need: "open" (an undecided position has a move inside, or pass 0 would have decided it;
the decided ones carry a depth, `DRAWN` or `DEFER`), loss in 0 (a depth), and, in draughts,
rtbgen's capture bounds (compulsory captures decide a position in pass 0; a winning exit in DTZ or DTC is a win in 1,
decided there too). DTM's exit seeds carry a level and stay sparse lists outside the cell. Chess without the 50-move
rule needs the same codes; with it, the bounds into a cursed win and a blessed loss and the cursed collapse classes come
on top, still within S.

**Rounds.** As rtbgen's `scan` mode: each round scans the group's bytes for the previous ply. In RAM a scan costs
nothing worth counting (a 20 GB group in 0.4 s at 50 GB/s; 600 scans in 4 minutes), and the work per position, the
unmoves and forward checks, is the same as with frontier bit planes. The cell is the rung-0 form for a group that fits
in RAM at 1 byte; when it does not, rungs 1 to 5 keep their bit planes and streams.

**Windows and reductions.** Window i holds the depths [W·i, W·(i+1)) in cell codes, counted from 0, not from the
group's lowest level: a DTM group whose seeds start deep simply starts in a later window. When a round would pass the
window's end, the build reduces:

1. **snapshot**: the window's bytes are written as they are, values unchanged; every position not decided in this
   window becomes `NOT_MINE` = 0, the undecided code itself (a value no extra byte takes): undecided positions are
   0 already, only `WON`, `LOST` and the statuses (241 … 255) need the staging buffer that maps them to 0. One plain
   zstd frame (`ZSTD_compressStream2`, multithreaded with `ZSTD_c_nbWorkers`); kept in RAM if it fits (mostly zeros,
   it compresses to long runs), written to disk if not;
2. **collapse**: every depth of the window becomes `WON` or `LOST`, the last ply `FRONT_WIN` or `FRONT_LOSS`.

A 6×6 DTC or DTZ build (at most 43 levels so far) never reduces; a deep DTM group reduces once per 240 plies.

**Merge.** One sequential pass over the final cells and all snapshots side by side, written straight into the
storage form: a position takes its byte from the one snapshot where it is not `NOT_MINE` (or from the final cells, if
decided in the last window). Windows 0 and 1 are translated once into direct storage codes (the class dropped: DTZ,
DTC take their sign from WDL; DTM goes to moves); window i ≥ 2 becomes **storage token 240 + (i − 2), and its snapshot
byte is the extra byte, verbatim**. Every value stays exact from the first round to the stored byte: no rounding, no
halving, no margin for an engine to keep, and the token is the window number: a multiple of the reduction interval.

**Implemented** as `tbgen build … cell8` (`solve_group_cell8`); `EGDB_CELL8_WINDOW=k` forces windows of k plies, to
exercise the reductions on small builds. Up to 6 pieces it agrees with egdb_6x6 and with every position's successors
for all three metrics, with the full window (no reductions) and with windows of 7 and 2 plies (up to 35 reductions
in one group). At 8 pieces (DTM, every material up to 7 pieces plus the 21 eight-piece ones with at most 2 men: 81
materials, 448 M positions, 4 jobs side by side) its files are byte-identical to the frontier mode's, and it is
faster: 87 s of wall time against 100 s (315 against 373 s summed over the builds). **The delayed check is the gain**:
checking a predecessor at every unmove of a won successor, as the frontier mode does, took 46.9 M loss checks in the
largest material, 04-04-01-01 (68 M positions, longest 73 plies, so no reduction at its real depths); marking first
and checking each candidate once took 33.7 M, and its rounds fell from 9.0 s to 7.3 s, below the frontier mode's
8.2 s despite the scans. With windows of 8 plies (9 reductions, about 0.45 s each: snapshot, collapse, deferred
positions recomputed; snapshots 2.8 MB each for 68 MB of cells) the files are again identical. The frontier mode
takes the same gain with a candidate bit plane: its rounds in 04-04-01-01 fall from 8.1 to 6.1 s (cell8 7.5 s, the
difference being the scans), and the subset builds in 84 s of wall time (311 s summed), byte-identical. With the delayed
check in both, the two modes are equally fast; cell8 is the one with 4× less group memory. Peak RSS falls 20–35 % (979 → 758 MB),
less than the cell alone would give: it is dominated by the 16-bit array sink of the whole material and the input maps.

**Limits.** The 16 tokens reach 2,160 plies (DTZ, DTC) and 4,320 plies (DTM), against the deepest known values of
every game: chess DTZ 1,193 plies (8 pieces, the opposing-pair tables) and DTM 1,097 plies (7 pieces, chesstb's
statistics); checkers 253 plies at 7 pieces (Trice–Dodgen) and 279 at 10 (Gilbert); 6×6 checkers 147 at 8 pieces.
A build that goes deeper stops with an error naming the material and its depth; the remedy is a larger S, not a side
path.

## 8. Best of breed for draughts

Take from syzygy: the metric split (WDL + DTZ), one value cell with codes, the fused round, level synchrony.
Take from Chinook: slicing by irreversible moves (finer here: by the placement of the men) and resolving all exits
once before the rounds. Take from Gasser 1996 and the `egdb_6x6` measurements: outcounting with the counter inside
the value cell. Take from Thompson: explicit frontier bitsets (the rounds touch only the frontier, not the slice).

**Every product keeps the exact depth in plies**, never rounded and never a bound. That excludes three things the
literature does: syzygy stores DTZ in moves rather than plies unless a table needs ply accuracy (`ply_accurate_win`,
`ply_accurate_loss`), which can be one ply off; Chinook's in-place sweeps decide positions in passes whose numbers are
only lower bounds on the depth (Lake-Schaeffer-Treloar 1992, Schaeffer et al. 2003); and Trice-Dodgen's DTC is
defined by such pass numbers. The rounds here are level-synchronous (§7, rule 4), so the round in which a position
is decided is its exact depth.

**Values are 16 bits during the build and 8 bits in storage**, for every metric. DTZ restarts at every exit, so its
depth is bounded by the longest phase of king moves inside one slice, not by the whole game (Bourzutschky-Tamplin-
Haworth: maxDTC 6 against maxDTM 107 in KQBNKN); it will usually fit a byte, but only a measurement says so, and the
build should not depend on it. With the counter inside the cell the build never meets a range
limit (no syzygy-style `reduce_tables`, no fit check of depth plus counter, no escapes). Without outcounting the
cell shrinks to 8 bits, with value-preserving reductions whose windows are the storage escape ranges (§7.5). Either
way the storage form is the byte with the escape tokens of §10.

### 8.1 The value cell (16 bits, per position and side to move)

The same layout for DTZ and DTM (exact numbers to be fixed; what matters is that decided values and codes are
separate ranges):

```
decided   win or loss with its depth          DTM: odd = win, even = loss (as now)
                                              DTZ: a loss flag bit, because DTZ parity does not separate wins from losses
                                              (a zeroing move can be the winner's or the loser's)
DRAW
NOLOSE    undecided, cannot lose any more (a drawing exit; for DTM also: a winning exit, see 8.3)
CNT(c)    undecided, lossable, c moves not yet known to be won (Gasser's counter, inside the cell)
```

Per position and side to move: 2 bytes, plus 2 bits for the frontier bitsets of the current and the next round.

### 8.2 DTZ (and with it WDL)

Passes, each a single `for_each`:

```
pass 0     for_each(policy, words(slice), [](w) {           // gather; reads finished slices (2-bit WDL store)
               for each position i in w, each side s:
                   moves = generate(i, s);  c = number of internal moves
                   if no moves:                     V[s][i] = L(0)
                   elif some exit wins:             V[s][i] = W(1)
                   elif c == 0:                     V[s][i] = some exit draws ? DRAW : L(1)
                   elif some exit draws:            V[s][i] = NOLOSE
                   else                             V[s][i] = CNT(c)
                   if V[s][i] is L(0):              F[s].set(i)             // depth 0: frontier of round 1
                   if V[s][i] is W(1) or L(1):      G[s].set(i)             // depth 1: frontier of round 2
           })                                                               // own words: no atomics; draws enter no frontier

round n    // n = 1, 2, …: F holds the decisions of depth n − 1, G collects depth n
           for_each(policy, words(F[0]) ∪ words(F[1]), [](w) {   // scatter; one pass per round
               for each q in w (side t, value v = V[t][q]):
                   for each predecessor p (side 1 − t) by king unmove:
                       if v is L(n − 1):   CAS  V[1−t][p]: undecided → W(n);         on success G[1−t].set(p) (atomic)
                       if v is W(n − 1):   CAS-loop V[1−t][p]: CNT(c) → CNT(c − 1);  c == 1 → L(n), G[1−t].set(p) (atomic)
               clear w in F                                                   // own word
           })
           swap(F, G);  stop when F is empty

final      for_each(policy, words(slice), [](w) { undecided → DRAW })          // gather
verify     transform_reduce(policy, words(slice), …)  value == best over successors   // gather + count
store      for_each(policy, blocks(slice), …)  16-bit values → bytes with escapes      // gather per output block
```

Why the round can be one pass: a predecessor can receive both a win (from a lost successor) and decrements (from won
successors) in the same round, but a position with a lost successor never reaches counter zero, and the win CAS
simply replaces whatever undecided code is there; a decrement that finds a decided value does nothing. Predecessors
with a capture are never touched, because pass 0 decided them (no capture test on predecessors, as on
`claude/sweet-hypatia-i05bm9` since `ceca9b3`).

The grandfather variant replaces `CNT(c)` by a single `LOSSABLE` code and the decrement by a candidate mark
(`LOSSABLE → CAND`), plus a second pass per round: `for_each(words(candidates))` verifies all internal successors
(gather; writes only itself and its own frontier word). It needs no counter and no non-idempotent scatter, at the
price of regenerating the candidates' moves. The measurements on 8-piece checkers (`DESIGN_STATE.md` §5: retrograde
with counting 1032 s, without 1204.5 s, classic 1376 s) favour counting; both fit the skeleton, so keep the
grandfather version as the alternative behind the same interface.

### 8.3 DTM: the same cell, the same round, plus deferred seeds

Exits now carry depths, which shows in two places, and both become **deferred seeds**: entries (round, side,
position) produced in pass 0, sorted by round, and processed as extra elements of that round's `for_each`.

* **A winning exit** wins in d = DTM(exit) + 1 unless an internal win comes first. Pass 0 writes `NOLOSE` (the
  position can no longer lose) and a seed *win at d*. In round d the seed does `CAS NOLOSE → W(d)`; if an internal
  win came earlier, the CAS finds a decided value and does nothing.
* **Losing exits** make a loss no faster than b = 1 + the slowest exit loss. Count all losing exits together as one
  extra virtual move: pass 0 writes `CNT(c + 1)` and a seed *decrement at round b − 1*. The counter then reaches zero
  exactly in round max(n, b), so the loss value is simply the round number, as for DTZ, and no bound is kept in the
  cell.

So the cell needs no tentative values and no loss bounds, and DTZ and DTM share the layout and the round; DTZ is
the case without seeds (its exit wins are W(1) in pass 0, its exit losses never matter). The seed lists replace
`ewin` / `eloss` (2 + 2 bytes per position now) by entries only for the positions with exits.

Build DTM **after** DTZ: with WDL known, drawn positions are never visited, and every lossable position is truly lost.

### 8.4 Pass 0 is one max over all moves: forced and optional exits

Pass 0 gives every position the best value over all its moves (the max on the chain of §9.4): each exit contributes
the exact value of its target, looked up in a finished database and seen from the mover, and the internal moves
together contribute U. Whether the result is final or only a bound then follows from the move list alone: with no
internal move, no U enters and the max is exact; with internal moves left, the exits only bound the value.

| game | exits that are forced (the max is exact) | exits that are optional (the max is a bound) |
|---|---|---|
| draughts, checkers | captures (compulsory), including a capture that ends on the back row and promotes | man moves, promotions |
| chess | none; a position without legal moves is terminal (mate, stalemate) | captures, pawn moves, promotions (including under-promotions and capture-promotions) |
| Stratego, known pieces | none; a side without a legal move has lost | attacks, whatever their outcome (attacker wins, loses, or both pieces go) |

What an optional exit settles:

| best exit | result | final? |
|---|---|---|
| wins | W | final for WDL and DTZ (a win by a zeroing move now cannot be beaten); for DTM a deferred seed (§8.3) |
| draws | ≥D (cannot lose) | a bound: the internal moves may still win |
| loses | nothing, unless no internal move is left | — |

Syzygy's `CAPT_WIN`, `CAPT_DRAW` and `CAPT_CLOSS`, written with a compare-and-swap that keeps the maximum, and
Chinook's Lookups (W, L only when every move is a lookup, else ≥D) are this table for chess captures and for
checkers' advancing leading checker. "Captures are compulsory" is not a special case in the engine: it only means
that the rules generate no internal move, so the rules concept needs nothing beyond the legal moves.

Where the exits lead also follows from the game: a capture or an attack goes to a smaller material; a promotion to
another material with the same number of pieces; a man or pawn move to another slice of the same material (another
placement of the men, or pawn structure). Stratego has neither men nor promotions: its slices are fixed by the
material and by the squares of the pieces that never move (bombs, flag), every exit is an attack, pass 0 yields
almost only bounds, and nearly all the work falls to the rounds, the opposite of a draughts endgame full of men.

### 8.5 Conversion (pass 0) is the real cost

| option | source | for draughts |
|---|---|---|
| pull: probe every exit | Chinook, `egdb_6x6` | simple; costs lookups (69 % of a build now) |
| dependency store raw, 2-bit WDL, capture positions included | syzygy's captures probe only the WDL of the subtable; Chinook's build worked on raw 2-bit data with its own paging | **for DTZ: removes capture resolution and zstd from pass 0** |
| push from the child: probe each child once, un-move into the parents | syzygy (captures) | good for **man moves** (un-move one step back) and promotions; **not for captures**: with majority capture an un-capture must still be checked to be a legal maximal capture |
| man-move exits as a near-sequential gather | — | a man move keeps the kings, so the target index in the child slice is an order-preserving remap of the king ranks over the changed set of free squares: read the child's raw array almost sequentially |
| sort and deduplicate probes | Wu-Liu-Hsu | 69.7 M of 211.7 M lookups land on a capture position, 26.0 M distinct (`DESIGN_STATE.md`): batch by target block |

## 9. Value encodings in domain language

The passes should not know how a value is stored. Each encoding is a type that names the values in the terms of
the game (a win in so many plies, a draw, unknown), and pass 0, the round, the verification pass and the store pass
are templates on it. One algorithm then serves every metric, every width (16-bit build cell, 8-bit store, 2-bit WDL)
and every on-disk convention.

### 9.1 Stored bytes are their own type

```cpp
enum class stored : std::uint8_t {};   // a byte as it is on disk or in the build cell
```

Code reads and writes raw elements (`std::span<stored>`, no proxy references) and converts on the right-hand side:
`raw[i] = E::W(p)`. Mixing a logical value with a stored byte (`raw[i] = 34`, `raw[i] == 0xFF`) does not compile,
`std::atomic_ref<stored>` works (a scoped enum is trivially copyable), and so do pointers, `std::fill` and every
other algorithm. A proxy `operator[]` would read as nicely but brings `vector<bool>`'s limits (`auto x = f[i]`
copies the proxy, no `&f[i]`, no `atomic_ref`).

### 9.2 One-sided depth: Thompson's result file

Thompson 1986 stores, for White to move, the number of moves to a White win, and 0 for "not a White win" (draw, loss
or illegal: his database is one-sided, see §2). The same information in three encodings, all with exact plies:

```cpp
struct direct            // 0..254: win in 2n + 1 plies; 0xFF: not a win
{
        static constexpr stored none{0xFF};
        static constexpr stored W(std::size_t plies) noexcept { return stored((plies - 1) / 2); }
        static constexpr std::optional<std::size_t> plies(stored s) noexcept
        {
                if (s == none) { return std::nullopt; }
                return 2 * std::to_underlying(s) + 1;
        }
};

struct complemented      // the same, stored as 0xFF - byte: "not a win" is 0x00
{
        static constexpr stored none{0x00};
        static constexpr stored W(std::size_t plies) noexcept { return stored(0xFF - (plies - 1) / 2); }
        static constexpr std::optional<std::size_t> plies(stored s) noexcept
        {
                if (s == none) { return std::nullopt; }
                return 2 * (0xFF - std::to_underlying(s)) + 1;
        }
};

struct thompson          // Thompson 1986: i = win in i moves (2i - 1 plies), 0 = not a win
{
        static constexpr stored none{0x00};
        static constexpr stored W(std::size_t plies) noexcept { return stored((plies + 1) / 2); }
        static constexpr std::optional<std::size_t> plies(stored s) noexcept
        {
                if (s == none) { return std::nullopt; }
                return 2 * std::to_underlying(s) - 1;
        }
};

static_assert(direct::plies(direct::W(69)) == 69 && !direct::plies(direct::none));
```

`W` takes plies (odd, at most 509), not moves, so call sites never see Thompson's moves or the byte's n; the hot
test "still not a win" stays `raw[i] == E::none`.

### 9.3 The background value: make "unknown" zero

A new file created and sized (`::open(O_CREAT | O_TRUNC)`, then `std::filesystem::resize_file`) is sparse: no disk
blocks, every byte reads as 0x00, and `mmap` hands out zero pages on first touch. A background that is not zero must
be written first:

```cpp
if constexpr (std::to_underlying(E::none) != 0) {
        std::fill(policy, data, data + n, E::none);      // touches every page, allocates every block
}
```

Measured here on 2 · 10⁹ positions: `direct` 1.0 s and 2,000,007,168 bytes on disk before a single result;
`complemented` and `thompson` no time and 8,192 bytes (the two pages written). (The fill ran on one thread: with
libstdc++, `std::execution::par` is parallel only with `-ltbb`.) `std::vector<std::uint8_t>(n)` is no shortcut: it
zero-fills eagerly, at the same cost as `(n, 0xFF)`; and a buffer that pass 0 overwrites completely needs no fill at
all (`std::make_unique_for_overwrite`).

The cost grows with the file: the fill is one full write of it before any result exists, every block allocated on
disk even where nothing will ever be written, and every page that pass 0 later rewrites goes to the disk twice. In RAM
that is memory bandwidth (seconds for 10¹¹ bytes); for a memory-mapped file it is the disk's write bandwidth, minutes
for the 10¹⁰–10¹¹-byte files of 10×10 or of the 12-piece 6×6 layer, and a doubling of what the disk has to absorb.

So every encoding should put its "unknown" (build) or "not a win" (one-sided store) at 0. For the depth encodings
this means an offset or a complement: the DTM convention L0 = 0 (no move, lost) collides with unknown = 0, so the
build cell stores decided values shifted (as Thompson's i = n + 1). `unit_build` now fills each fresh `.raw` with
`UNKNOWN8 = 254`; with unknown = 0 that pass goes, and the resume check that counts unknown bytes counts zeros instead
(or relies on the `.progress` checkpoint alone).

### 9.4 WDL as a signed interval chain

While a database is being built, a position's value is not one of L, D, W but the set of values still possible: an
interval over L < D < W. There are six (Björnsson-Schaeffer-Sturtevant 2005 name them LL, TT, WW, LT, LW, TW):

| interval | meaning | code |
|---|---|---|
| [L, L] | lost | −2 |
| [L, D] | cannot win | −1 |
| [L, W] | unknown | 0 |
| [D, W] | cannot lose (a drawing exit: `NOLOSE` in §8.1) | +1 |
| [W, W] | won | +2 |
| [D, D] | drawn | (outside the chain) |

The backup is exact on intervals: the parent's view of a child is −[lo, hi] = [−hi, −lo], and the best over the
moves is the componentwise maximum [max lo, max hi]. The five coded intervals form a chain, closed under both
operations, and with these signed codes both are plain arithmetic:

```cpp
enum class wdl : std::int8_t { L = -2, LD = -1, U = 0, DW = 1, W = 2 };

constexpr wdl operator-(wdl v) noexcept { return wdl(-std::to_underlying(v)); }   // W <-> L, DW <-> LD, U fixed

wdl best = wdl::L;                                    // negamax: exact on the chain
for (auto c : children) { best = std::max(best, -c); }
```

The sign answers the questions the passes ask: `v < 0` cannot win, `v == 0` unknown, `v > 0` cannot lose, ±2 decided.
Pass 0 is the same `max`: each exit contributes W, DW or L from the exact value of its target, the internal king
moves together contribute U, so a winning exit gives W, else a drawing exit DW, else open internal moves U, else L.

The exact draw [D, D] is incomparable with U and falls outside the chain; a move into an exact draw counts as DW (or,
seen from the other side, LD). That only loosens a bound, never makes a value wrong. Draws become exact at the end:
the final pass maps U, LD and DW to D = 0. So 0 means *unknown* during the build, where it is the free background of
§9.3, and *drawn* afterwards; the stored codes are L = −2, D = 0, W = +2, and negation stays `−x`.

No two-bit total order can be exact, because D ∨ U = [D, W] is neither D nor U; every 2-bit layout rounds. A packed
2-bit WDL build (Chinook's) keeps the four codes L, U, DW, W and stores LD as U: LD only arises inside a backup, as the
parent's view of a DW child, and storing it as U changes no decision (a position is lost only when all its moves lead
to wins), it only gives up marking that child's parent "cannot lose" early. The 16-bit build cell and an `int8_t`
hold the full chain anyway.

Depth fits the same scale: wins large and positive, faster ones larger, W(p) = WMAX − p; losses mirrored,
L(p) = −(WMAX − p); the small magnitudes reserved for the bounds and for draw/unknown. The backup becomes "negate and
step one ply toward zero", the `up()` that the forward search in `egdb_6x6.cpp` already applies to its `to_score`
values, so the build cell, the stored values and the search can share one signed scale.

### 9.5 The vocabulary of the two-sided build

| operation | used by |
|---|---|
| `W(p)`, `L(p)`, `D` (WDL: constants) | pass 0, the round |
| `U`, `nolose`, `count(c)` | the build cell only: undecided codes and Gasser's counter |
| negation / `backup` (§9.4): child W(p) → parent L(p + 1), child L(p) → parent W(p + 1), bounds swap | the round, verification |
| `better(a, b)` for the side to move: fastest win, then draw, then slowest loss | pass 0, verification |
| `plies(s)` → `std::optional<std::size_t>` | probing, statistics, the store pass |

The 16-bit build cell (§8.1) and the 8-bit store both model it, so the store pass is the generic
`out = Store::from(Build::decode(v))`, and the rounds are written once for DTZ, DTM and WDL. `E::W` is spelled the same
for an enumerator and for a static member or function of a struct, so a template written against the WDL enum also
accepts the depth encodings.

Where per-byte access ends: the escaped store (cdb v2) keeps the extra byte in the block's appendix, so its `W(p)` is
a block-level operation for depths beyond the escape base; a packed 2-bit WDL array holds four positions per byte,
so writes go per owned word (§7, rule 2) or through `fetch_and` / `fetch_or`. Neither matters to the build, which
uses neither.

### 9.6 Notation for what is stored: full and half results

A stored file is one **side to move** (`wtm`, `btm`) times one **kind**:

| | full result: win, draw and loss | half result: wins only | half result: losses only |
|---|---|---|---|
| depth (one byte) | `dtm` | `dmW` | `dmL` |
| outcome only | `wdl` (2 bits) | `isW` (1 bit) | `isL` (1 bit) |

The same for `dtc` and `dtz`; the outcome files carry no metric, since all metrics have the same wins, draws and
losses. A two-sided build stores full results (`dtm-wtm`, `dtm-btm`, `wdl-wtm`, `wdl-btm`). A one-sided build stores
half results: White's wins with White to move and Black's losses with Black to move (`dmW-wtm`, `dmL-btm`,
`isW-wtm`, `isL-btm`); the other halves, `dmL-wtm` and `dmW-btm`, are the colour mirror's `dmL-btm` and
`dmW-wtm`. Thompson 1986 kept `dmW-wtm` only; Thompson 1996 (dtc) kept `dcL-btm` (RES) with `isW-wtm` and
`isL-btm` (the summation bitmaps).

What a build reads from the materials that its moves lead into (a White move lands with Black to move, and vice
versa):

| build | after a White move out | after a Black move out |
|---|---|---|
| one-sided dtm | `dmL-btm` | `dmW-wtm` |
| one-sided dtc, dtz | `isL-btm` | `isW-wtm` |
| two-sided dtm | `dtm-btm` | `dtm-wtm` |
| two-sided dtc, dtz | `wdl-btm` | `wdl-wtm` |

Any cell that is not stored costs a ply of search per lookup (the half result from the other side to move's file).

So the outcome files have two different roles. For **dtm** they are a by-product, nice to have: the build reads
only depths, and the bits (or `wdl`) serve search, where a WDL probe touches a file 8× (or 4×) smaller than the
bytes. For **dtc and dtz** they are needed to build: the materials built on top read nothing else. Since the outcome
files are the same for every metric, any metric's build can supply them.

## 10. Storage

Keep what is there: blocks of 4096 positions with zstd per block, capture positions filled with the previous value
(Chinook's run-continuing fill, syzygy's and Zakharov's don't-care values: all three report large gains), **escape
tokens with their extra bytes appended to each block in the same frame** (below; egdb_6x6's cdb version 2 is the
precursor), and a CRC per unit. **8 bits in storage**; the build holds 16 bits with the counter in the cell (§8.1),
or the 8-bit cell whose windows are the escape ranges (§7.5).

**Full depth everywhere.** Depth files are exact, in the build and in storage: no rounding, no halving into moves for
DTZ or DTC, and no band of values beyond a draw rule (we build without one). Both chess generators give up a ply there:
syzygy's rtbgen without `-p` stores DTZ up to 100 plies in moves, unless a side has a position at exactly 100, and
always halves the cursed band beyond 100; chesstb keeps DTZ exact up to 100 but halves the cursed band when a table fits
one byte. The engine then needs a margin (Stockfish counts a win as certain only if DTZ + counter ≤ 99), and a second
limit (FIDE's automatic draw at 75 moves) falls inside the halved band. The one reduction allowed is lossless: a DTM
win is odd and a loss even, so a DTM can be stored in moves and its plies recovered from WDL (the thompson86 and tbgen
byte, up to 509 plies). DTZ and DTC have no such parity (either side's move can end the count) and stay in plies.

**Escapes: S = 16 tokens, one extra byte each, for every game.** Each position has one code byte. Codes 0 … 239
(E = W = 240) are direct; codes 240 … 255 are tokens, and token 240 + i is followed by one extra byte x, 1 … 240:

| metric | direct code | token 240 + i with extra byte x | deepest |
|---|---|---|---|
| DTZ, DTC | the depth in plies, 0 … 239, sign from WDL | 240 + 120 i + (x − 1) / 2 plies (x holds win/loss, as the build cell) | 2,160 plies |
| DTM | c = ⌊plies / 2⌋, sign from WDL (a win 2c + 1, a loss 2c plies): plies 0 … 479 | 480 + 240 i + x − 1 plies (the class is the parity) | 4,320 plies |

The token ranges are exactly the windows of the 8-bit build cell (§7.5): W = 240 codes each, and the extra byte is the
cell code of its window, copied from the build's snapshot. The extra byte never takes 0 (undecided in the cell,
`NOT_MINE` in a snapshot) nor 241 … 255 (the cell's other reserved codes). E is the same constant for every unit and game (it still
goes in the header); there is no overflow table, and a build deeper than the last token stops with an error (§7.5).

The extra bytes follow the block's position bytes in the same zstd frame, one per token, in position order, so the
j-th token of a block takes the j-th extra byte: a probe counts the tokens before its position (one SIMD pass over at
most a block, or egdb_6x6's per-block bitset with prefix counts) and reads that byte directly, with no sequence to
walk. A block without tokens has no extra bytes. Draughts and checkers (deepest known values around 300 plies) fit
DTM in the direct range and DTZ, DTC nearly so: tokens are rare.

egdb_6x6's cdb version 2 is the precursor: token c with extra byte x is E + 256 (c − E) + x, with E chosen per unit as
large as the longest value allows and 255 reserved for the draw (E = 255 − k, k tokens reaching 254 + 255 k plies).
The new form gives up 16 direct codes for a fixed E, ranges that match the build's windows, and no draw code (draws
are don't-cares, looked up in WDL). In egdb_6x6's raw build format, depths above 251 get the code 252 and their exact
value in an overflow table (`raw_ovf`, saved as `.ovf` next to the raw file).

**Don't-care fills** take either the dominant value (the most frequent direct code in the block) or the
run-extending value (the previous stored one, Chinook's fill), whichever compresses the block better (chesstb likewise
tries two fills per block and keeps the smaller), but **never an escape token**: a token needs an extra byte and would
shift every later extra byte of the block. Where the run-extending value is escaped, the fill takes the dominant value.

Products per material: **WDL** (2 bits, dense, the probe for search), **DTZ** (byte, escapes if ever needed), **DTM**
(byte + escapes). Options from syzygy worth measuring: DTZ for one side to move only (*[bg]*; the other side follows
exactly from one ply of search, cheap with compulsory capture: an exit counts 1 from the WDL of its target, an internal
move one more than the stored value; unlike syzygy, depths stay in plies, never in moves), and a search for the man/king order that compresses best (syzygy's
`permute_piece_wdl`); Zakharov adds bounded fills (≥ n, ≤ n) for DTM.

## 11. Verification

| check | source | status |
|---|---|---|
| every value equals the best over its successors | Nalimov, Fang 2000, Chinook | present (`verify`, `unitverify`) |
| `seq` and `par` give identical files; different chunkings give identical files | Romein-Bal (processor counts), Stiller 2001 | add: cheap once every pass is a `for_each` |
| an independent second program | Herik vs Thompson; Trice-Dodgen vs Chinook (found errors every internal check missed) | the classic solver and the pull round can serve as the second program |
| invariants between metrics: WDL(DTZ) = WDL(DTM), DTZ ≤ DTM for wins | Bourzutschky-Tamplin-Haworth, Haworth | add once both builds exist |
| CRC per block and per copy | Chinook 2003, NWH (MD5) | present (crc32c per unit) |
| statistics against published counts | `egdb_fast` (mdgsoftware.nl) | present for kings-only |

## 12. The open decisions of DESIGN_STATE.md

| # | decision | recommendation from the literature |
|---|---|---|
| 1 | one pipeline (materials) and one storage format | materials, as both lineages build: the unit of dependency is the material. Units can stay a packaging of the files |
| 2 | one slice solver | yes: the round of §8.2 (and §8.3 for DTM), counting by default, grandfather behind the same interface; the classic solvers go. Keep the pull round (P10, ~20 lines in this skeleton) as the test oracle |
| 3 | adaptive forward rounds (4 % of a build) | drop the six-constant model and `switch.py`. If switching stays, Gasser's one-term rule (#frontier · mean unmoves > #undecided → pull) |
| 4 | true values of capture positions | in the build-time dependency store: yes (raw, 2-bit WDL for DTZ builds); in the distributed compressed files: no, the fill is what makes them small |
| 5 | bitsets instead of lists | yes for DTZ: two live bitsets per side to move, iterated per 64-bit word. DTM keeps short sorted lists for the deferred seeds |
| 6 | fewer bytes per position | 2 bytes for every metric: a 16-bit cell with syzygy-style codes and Gasser's counter inside (now 5 bytes plus lists); 8 bits with escapes in storage. On 6×6 the dependency store matters more than the cell; on 10×10 both matter |
| 7 | the metric | both: WDL + DTZ in one build (slices self-contained, exits need only WDL), DTM in a second build that uses the WDL. DTZ is also the metric of the 25-move rule |

Naming clashes listed in `DESIGN_STATE.md` §1 (`S`, `big`, `NONE16 = DRAW`) disappear with one solver and one
value encoding per metric.

## 13. Measurements still needed

1. largest DTZ per slice and per unit (how often would the 8-bit storage of DTZ need escapes at all?);
2. pass 0 with a raw 2-bit WDL dependency store against the compressed store with capture resolution;
3. counting against grandfather in the one-pass round, `seq` and `par`, on 8-piece checkers and on the large 11–12
   piece international slices;
4. bitset frontier against syzygy's implicit frontier (a full scan per round) for DTM, where rounds run to 250+;
5. compressed size of DTZ against DTM per unit (syzygy's experience: DTZ compresses far better).

---

## Appendix: the papers in one line each

**Chess, classic** ([notes](papers/1_chess_classic.md)) ·
Ströhlein-Zagler 1977: Boolean matrix iteration over the whole state space, pull, draws as the remainder ·
Herik-Herschberg 1985: KBNK in a byte per position (one side to move), push + verification; a symmetry-image bug
in their first run, and a cross-check with Thompson that exposed a bug in his ·
Thompson 1986: bitmaps per ply, push + verification, waves for conversions, pawn-rank slicing ·
Thompson 1996: 6-piece on one disk-bound machine; delayed masking; rejects outcounting and in-place sweeps ·
Stiller 1996: Connection Machine; complement-predecessor-complement, moves as whole-array shifts, no frontier ·
Stiller 2001: retrospective on verifying parallel tablebase software.

**Chess, modern** ([notes](papers/2_chess_modern.md)) ·
Heinz 1999 (1): compact indexing, 3612 king pairs, colex ranking of like pieces ·
Heinz 1999 (2): knowledgeable 0/1/2/4/8-bit encodings for RAM-resident probes ·
Nalimov-Wirth-Haworth 1999: the shared skeleton of Nalimov's pull and Wirth's push with outcounting; metric definitions ·
Nalimov-Haworth-Heinz 2000, 2001: the Nalimov index (1806 king pairs, broken slots, en-passant zone) ·
Bourzutschky-Tamplin-Haworth 2005: DTC, DTZ, DTZ50, DTR and how each changes the build; maxDTC ≪ maxDTM ·
Huntington-Haworth 2015: DTM50 for every ply count; dense sweeps first, then the frontier ·
Zakharov-Malkovskii-Shchukin 2016: compression of the Lomonosov tables with underdetermined (don't-care, bounded) values.

**Checkers** ([notes](papers/3_checkers.md)) ·
Lake-Schaeffer-Treloar 1992: 8-piece statistics; pass numbers from in-place sweeps are only lower bounds ·
Lake-Schaeffer-Lu 1994: the Chinook builder: slicing, Captures / WinningMoves / Lookups / NoIo / queue, RLE without captures ·
Schaeffer et al. 2003: the same at 10 pieces; Huffman runs, verification against the compressed data, hardware errors ·
Trice-Dodgen 2003: an independent build; DTC in a byte; found errors in Chinook's 7-piece databases ·
Björnsson-Schaeffer-Sturtevant 2005: partial-information lattice, slices built before their dependencies ·
Schaeffer-Lake 1996, Schaeffer et al. 2005 (IJCAI), Schaeffer 2007 (ICGA), Schaeffer et al. 2007 (Science): the
proof that checkers is a draw; databases as the backward half, their I/O locality mattering more than their size.

**Xiangqi** ([notes](papers/4_xiangqi.md)) ·
Wu-Beal 1996, 2001 (IS), 2001 (IEEE), 2002 (MGoNC): retrograde analysis with one random-access bitmap in RAM,
push + verification, decomposition by irreversible moves, parallel cost models ·
Fang-Hsu-Hsu 2000: counter + best value, full traversals, odd/even byte encoding ·
Fang-Hsu-Hsu 2002: perpetual check as a greatest fixpoint on top of retrograde analysis ·
Wu-Liu-Hsu 2004 (+ slides): external-memory construction, sort-and-combine of unmoves; taxonomy I–IV.

**Nine Men's Morris, mancala, other games** ([notes](papers/5_morris_mancala_other.md)) ·
Gasser 1991: Morris databases; push or pull per ply by cost ·
Gasser 1996 (CI), 1996 (GoNC): outcounting with the counter in the value byte; Morris is a draw ·
Bal-Allis 1995: distributed awari databases; counter + best value, one phase per score bound, combined messages ·
Lincke-Marzetta 2000: large awari databases; score as WDL threshold problems, 1 bit RAM + disk, dual indexing ·
Irving-Donkers-Uiterwijk 2000: Kalah (no cycles), forward evaluation per configuration ·
Romein-Bal 2002, 2003: awari solved; 2-bit parallel retrograde analysis with work migration ·
Irving 2014: Pentago solved; forward level sweeps, block layout, compression in RAM ·
Lim-Nievergelt 2009: Tiger and Goats; WDL endgame databases plus a halfway database built on demand ·
Saffidine-Jouandeau-Cazenave 2011: Breakthrough on small boards by parallel proof-number search; race patterns (no database).

**Surveys, losing chess** ([notes](papers/6_surveys_losing_chess.md)) ·
Uiterwijk-van den Herik 2000: the advantage of the initiative; P/N positions, mutual zugzwang ·
van den Herik-Uiterwijk-van Rijswijck 2002: games solved, vocabulary (convergent games, max-to-mate / conversion / zeroing) ·
Kishimoto-Müller 2015: game solvers; the simplest correct retrograde algorithm (pull sweeps to a fixpoint) ·
Watkins 2014, 2017: losing chess solved; DTC tablebases adapted from the RobboBases, 4-unit tables probed in the proof.
