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
   passes the first zeroing move or the end itself).

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

Two facts that keep counters correct, from Thompson 1996 and Herik 1985:

* a counter counts *moves out of* a position, and must equal the number of *unmoves into* it. That holds only when
  the index is not folded by symmetry inside the slice. `egdb_6x6` keeps the index unfolded inside a slice and folds
  the mirror only at storage, which is the right choice; keep it;
* when a slice is its own mirror, every image of a position must be written. The unit build already checks
  "mirrored positions disagree".

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
   layer: 3–8 µs per cold lookup, `compression_mmap_10.txt`). Outputs on disk are written sequentially, or densely
   within a window that fits in RAM, which the page cache turns into one write per page. Sparse random writes go to
   RAM only, to avoid thrashing the SSD: every scattered write dirties a whole 4 KiB page (the drive writes 4096
   bytes for one), pages may be flushed and rewritten repeatedly, the drive wears, and after an interruption nobody
   knows which scattered writes reached the disk. In the passes of §8 the only scattered writes are the rounds'
   pushes into predecessors and next-frontier bits, into the slice's arrays in RAM. When a slice no longer fits in
   RAM (all-king slices on 10×10), the remedies are finer slices or Wu-Liu-Hsu's batching: collect the updates, sort
   them by index, apply them in one sequential pass. In the current code the one write to a mapped file that is not
   in index order is the mirrored half of a symmetric material in `unit_build` / `mat_build`
   (`u.p[stored_offset(r, mirror(pos))] = b`); it fills the mirrored slice's range densely (at most about 17 MB on
   6×6), so it is harmless, and it becomes strictly sequential by mirroring into the slice's RAM buffer first.

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
build should not depend on it. The build never meets a range
limit (no syzygy-style `reduce_tables`, no fit check of depth plus counter, no escapes); the storage form is the
existing byte with escape codes (§10), converted once when a unit or material is written.

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

## 10. Storage

Keep what is there: blocks of 4096 positions with zstd per block, capture positions filled with the previous value
(Chinook's run-continuing fill, syzygy's and Zakharov's don't-care values: all three report large gains), **the
successive escape codes E … 254 with their extra bytes appended to each block in the same frame** (cdb version 2),
and a CRC per unit. **8 bits in storage, 16 bits in the build**: the escapes are a storage concern only, applied in
one conversion pass when a unit or material is written; the build never needs them.

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
