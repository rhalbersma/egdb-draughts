# Group 5: Other games (Nine Men's Morris, Awari, Pentago, Tiger and Goats, Breakthrough)

Read from the user's Google Drive via `read_file_content` (text extraction). Statements marked
"[background, not verified from PDF]" come from general knowledge, not from the extracted text.
Classification vocabulary: **init / conversion / forward (pull) / retrograde (push)**; for the
`for_each` mapping, **gather** = writes only its own element (race-free under `par`),
**scatter** = writes other elements (needs atomics, a double buffer, or a bitset with atomic OR).

---

## 1991-HPAI-Gasser.pdf

1. **Citation**: Ralph Gasser, 1991, *Heuristic Programming in Artificial Intelligence 2* (Second Computer Olympiad proceedings; the extracted text does not name the volume, the year/venue comes from the file name), "Applying Retrograde Analysis to Nine Men's Morris".
2. **Scope**: Nine Men's Morris mid/endgame subgames 3-3, 4-3, 4-4, 5-3, 5-4, 6-3, 5-5, 6-4 (W-B = stones on the board). Sizes (uncompressed file bytes = positions): 3-3 210,140; 4-3 1,725,960; 4-4 3,667,665; 5-3 5,484,540; 5-4 21,938,160; 6-3 14,319,168; 5-5 32,907,240; 6-4 53,696,880. Macintosh IIx, 8 MB RAM, 30-40 MB of free disk; minutes (4-4) to weeks (6-4). Longest win: 157 plies (6-4). 3-3 is only 0.2 % draws.
3. **Metric**: depth to win/loss in plies (DTM-like: "plies until a win/loss is finally reached", across subgame boundaries, not distance-to-conversion). One byte per position "to differentiate between 256 plies".
4. **Indexing**: not described in detail in this paper (positions with W/B colours swapped are mapped onto the W-B subgame by swapping colours). Slicing by stone counts (W-B subgames). The author also tried to put "similar" boards (same depth) at adjacent file positions to help RLE.
5. **Working storage**: one byte per position, held **on disk, run-length compressed**, with a sparse index into the compressed file for random access. The construction runs directly on the compressed file.
6. **Passes, in order**:
   - **init** (full scan): terminal positions = side to move blocked (loss) or side to move can close a mill reducing the opponent to 2 stones (win, i.e. a **conversion** folded into init). Trick: decide blocked-ness for whole families at once: given the white stones, compute the minimum number of black stones needed to block them (e.g. 9 for the example) and the sizes of the empty regions they partition (1, 2, 17 -> black blocked only if it has 1,2,3,17,18,19,20 stones) and skip all 4,845 black placements.
   - per ply *d*: **loss backup** = retrograde (push) from the positions lost at depth *d*: every predecessor is a win at *d*+1.
   - **win backup** = retrograde from the wins at depth *d*, with **grandfather / verification**: for each unidentified predecessor generate all its successors; loss iff all are wins. Cost O(#Wins x AvgPred x AvgSucc).
   - Alternative win backup = **forward (pull)** full scan over the *unidentified* positions, checking successors: O(#Unidentified x AvgSucc). Use it when #Wins x AvgPred > #Unidentified (example: 7-3, 30 M positions, 900 K wins at ply 3, AvgPred about 50, so the pull scan is used for the losses at ply 4).
   - Termination when no new wins/losses; the rest are draws.
   The frontier is "all positions with value == d" found by a sequential scan of the file (no separate bitset).
7. **Parallelism / memory access**: sequential; disk-bound. Sequential scans benefit from compression (smaller file); random probes go through the index plus a short sequential decode; working-set argument: the file is small early (when most positions are found) and grows later when fewer probes are needed.
8. **Draws, cycles**: repetition = draw; positions never decided are draws.
9. **Final storage / compression**: run-length plus variable-length code: one byte stores a value 0-125 (7 bits) with an "X" bit meaning run length 1; a second byte extends values to 381 plies; run lengths in 1, 2 or 4 bytes. Ratios: 3-3 2.3x, 4-3 10.8x, 4-4 5,137x, 5-3 65x, 5-4 590x, 6-3 24x, 5-5 338x, 6-4 4.2x.
10. **Verification**: not discussed.
11. **Notable tricks**: (a) choose push vs pull per ply from a cost model; (b) family-level terminal detection (lower bound on blockers); (c) retrograde analysis directly on RLE data; (d) suggestion that WDL-only would be faster.
12. **Mapping to for_each**:
    - init: gather over the full index range (each position writes itself). The family trick is a gather over the *white-placement* index, writing a contiguous block of black placements: still race-free if blocks are disjoint.
    - loss backup (push): scatter over a frontier (value == d); writes `win(d+1)` into predecessors. Idempotent store of the same value, so a benign race; with a bitset frontier it needs atomic OR or a next-frontier double buffer.
    - win backup with verification (push form): scatter (writes predecessors after verifying them). The **pull form** (scan unidentified, check all successors are wins, write self) is a pure gather over the full range and is race-free if it reads a frozen snapshot of the previous ply (double buffer, or encode the ply in the value so that "win at <= d" is stable).
    - Running on RLE data cannot be expressed as `par` random access without decompressing first.

---

## 1996-CI-Gasser.pdf

1. **Citation**: Ralph Gasser, 1996, *Computational Intelligence* 12(1), "Solving Nine Men's Morris".
2. **Scope**: all 28 mid/endgame subspaces 3-3 ... 9-9; 7,673,759,269 legal states in total (Table 2, e.g. 3-3 56,922; 4-4 3,225,597; 6-5 144,232,144; 8-7 1,046,720,480; 9-9 95,978,501). Computed 1989 (Mac IIx) to 1993 (Mac Quadra 800). Also ported to Cray X-MP/28, VAX 9000, RS/6000, 16 Transputers, 30-processor MUSIC DSP system, DEC 3000 Alpha; only the Alpha beat the Mac (multi-user low priority, poor integer performance, parallel machines lacked memory and disk). The game value (draw) then came from an 18-ply alpha-beta opening search (about 3 weeks on the Quadra, 72 MB RAM).
3. **Metric**: depth of win/loss (plies), one byte per state. Byte coding (Table "Information coding"): low values = "loss in 0, win in 1, loss in 2, win in 3, ..."; high values count down from 255: 255 = unknown (all successors unknown), 254 = unknown with 1 successor unknown, 253 = 2 unknown, 252 = 3 unknown... i.e. **value and counter share one byte**, growing towards each other. They collided in 8-7 (had to manually scan and lower high counts).
4. **Indexing**: subspaces by (white stones, black stones); dependency DAG: 7-5 needs 7-4 and 6-5 (mill closure removes one stone). Index = f(W)·C(24-|W|, |B|) + g(B given W) + h(side): f ranks white sets modulo the 16-element symmetry group (5 symmetry axes; Polya cycle index used to count; non-symmetric placements for 1..9 stones: 4, 30, 158, 757, 2,830, 8,774, 22,188, 46,879, 82,880); g ranks black stones among the remaining points; h = side-to-move offset (zero when stone counts are equal: colours flipped so White moves). Improved f' only enumerates white sets with no closed mill when applicable (38,040 instead of 82,880 for 9 stones). Range 9,074,932,579 (18.26 % excess) -> 8,102,965,583 (5.59 % excess) with f'. Unreachable states (e.g. closed mill but opponent has 9 stones) are kept as slots. 3-3 extra ring symmetry ignored.
5. **Working storage**: one byte per state (Val/Count union), on disk on the Mac; the paper does not detail paging.
6. **Passes, in order**:
   - **init** (scan; made output-proportional by the family trick): blocked side to move = loss; side to move can close a mill reducing the opponent to 2 = win (**conversion** folded in; positions with < 3 stones are not in the state space); also losses in 2 plies for the 3-stone side (opponent has two disjoint open mills; or two open mills and side to move cannot close a mill; or side to move must remove a stone blocking an opponent mill). Other mill closures that stay >= 3 stones go to a smaller subspace: [background, not verified from PDF: these are resolved by probing the smaller subspaces during counting/propagation]. Blocked positions are rare (4-4: 6 of 3,225,597).
   - **retrograde, work-list driven** (the "Backupable" queue): v1 (Thompson 1986) = for each decided state, for each predecessor: if state lost -> pred win; if state won -> **verification** (scan all successors of pred). Complexity O(E·f).
   - v2 = **outcounting**: Count[pred] initialised lazily (first time a won child reaches it, count all successors), decrement per won child, loss at 0. Complexity O(E + S·f) = O(E). Depth computed too (a modified version; not shown).
   - Draws = remaining unknown.
   The frontier is a queue ("Backupable"), not ply-synchronous in the pseudo-code; the depth version must process in ply order [inferred].
7. **Parallelism / memory access**: sequential; the parallel ports failed on I/O. Opening search: databases packed 8 states/byte (1 bit) with two searches (win? / loss?), only 9-9, 9-8, 8-8 used (115 MB), bounds by assuming missing databases are won for one side, transposition table only at ply 16 (to cut disk I/O), 8-ply results cached.
8. **Draws, cycles**: undecided after propagation = draw. Rule choices: one stone removed for a double mill; removing from mills allowed if all opponent stones are in mills.
9. **Final storage**: 1 byte/state with depth; 1 bit/state (two separate bitmaps) for the opening search.
10. **Verification**: independent forward-search verifier on 30 SUN SPARCs, about 3 months (Balz 1994): checks W/L/D consistency only (not depth). Found half a dozen hardware errors and a class of draw-instead-of-loss errors from a code "improvement" that affected many thousands of positions. Shared code (index function, initial values) is acknowledged as a residual risk.
11. **Notable tricks**: Val/Count union in one byte; lazy counter initialisation; mill-free white-set ranking; output-proportional init; 1-bit win/loss bitmaps for search.
12. **Mapping to for_each**:
    - init: gather over the full range (race-free).
    - outcounting propagation: scatter (decrement predecessor counters, set predecessor values). Under `par` it needs `std::atomic_ref<uint8_t>` fetch_sub on the merged byte; the thread whose decrement reaches 0 owns the "becomes lost" transition. Lazy counter initialisation inside the scatter is racy (two threads may both see Count==0 and both count): initialise the counters in a separate gather pass first.
    - "win" marking from a lost child: idempotent store, a benign race; use compare-exchange from "unknown" so that exactly one thread adds the position to the next frontier.
    - The work-list version is asynchronous; to get `for_each` passes, restructure it into ply-synchronous frontiers (bitset of positions decided at ply d) -> one scatter pass per ply.
    - The verification check (v1) has a gather-shaped inner part: it can be restated as a pull pass over the unknowns.

---

## 1996-GoNC-Gasser.pdf

1. **Citation**: Ralph Gasser, 1996, *Games of No Chance* (MSRI Publications vol. 29, pp. 101-113), "Solving Nine Men's Morris".
2. **Scope**: same work as the CI paper (shorter version): 28 subspaces, 7,673,759,269 states mapped into 9,074,932,579 indices; machines and dates as above. Mentions an 8-7 position with a mill closure after 187 plies and a 3-3 loss in 26.
3. **Metric**: depth of win/loss; Val/Count union in one byte (Table 1): 0 = loss in 0, 1 = win in 1, 2 = loss in 2, 3 = win in 3, ...; 252..255 = "draw" with 3/2/1/all successors unknown.
4. **Indexing**: subspaces by stone counts with dependency DAG (Figure 4), symmetry about 16-fold (5 axes, one redundant: pi4 = pi1 o pi2 o pi3), unreachable states noted; hash range 9,074,932,579 (the improved f' is only in the CI version).
5. **Working storage**: one byte per state (Val and Count as a union).
6. **Passes, in order** (Table 2 pseudo-code): init over all states (Val = Initialize(state), Count = 0; terminal/decided states placed in Backupable; the extracted text says "if Val[state] = draw then Put", which reads like an OCR or typo inversion: decided states must be the seeds); all non-decided states set to "draw"; then **retrograde** work-list with **outcounting** with a lazily initialised Count (count successors the first time a won child arrives, then decrement; 0 -> loss). Lost child -> predecessor win. Init special cases as in the CI paper (blocked, mill to 2 stones, 3-stone losses in 2).
7. **Parallelism**: none effective (same ports list as CI).
8. **Draws**: default value "draw" overwritten by proofs; repetition = draw.
9. **Final storage**: as CI (1 bit per state, 8/byte, for the opening search; 9-9, 9-8, 8-8 = 115 MB; TT at ply 16; 8-ply intermediate database).
10. **Verification**: as CI (30 Sparcstations, 3 months, W/L/D only, found about 6 hardware errors and a code bug).
11. **Notable tricks**: identical to CI; the "draw as default, Count only meaningful while draw" framing is the cleanest statement of the merged byte.
12. **Mapping to for_each**: as CI. Only the init pass is a gather; propagation is a scatter with atomic decrement; reorganise the work-list into per-ply frontier passes.

---

## 1995-ACM-Bal-Allis.pdf

1. **Citation**: Henri Bal and Victor Allis, 1995, (ACM venue per file name; the extracted text is a preprint without venue), "Parallel Retrograde Analysis on a Distributed System".
2. **Scope**: Awari endgame databases by number of stones on the board, 13-21 stones. 17 stones: 51,895,935 entries, 50 min on 64 processors vs about 40 h on 1 (speedup 48). 21 stones: over 354 M entries, under 12 h on 60 processors (would need 400 MB on one machine). Hardware: 64 SPARC Classic clones (50 MHz, 32 MB each), 10 Mbit/s Ethernet, Amoeba OS, Orca language.
3. **Metric**: score (game value in -N..N, N = stones on board): best obtainable capture difference for the side to move. Stored with a minimal number of bits (17 stones: 6 bits). Value through a move = -child + points captured by that move.
4. **Indexing**: Gödel numbering (Allis et al. 1991): lexicographic order of pit contents, rank via precomputed binomials, and the inverse. No symmetry mentioned. Slicing by stones on the board (captures leave the slice).
5. **Working storage**: per entry BestValue, UnknownChildren and Status {Unknown, Known, Propagated}; for awari the two extra fields fit in 3 more bits: 9 bits/position, 57 MB for 17 stones. All in distributed RAM.
6. **Passes, in order**:
   - **init** (full scan): count children -> UnknownChildren; end nodes get their value and Status=Known; others BestValue = -N, Unknown.
   - **for U = N downto 1** (one phase per **score threshold**): scan the local partition; any entry that is Known, or Unknown with BestValue == U (cannot be beaten any more), is **propagated**: **retrograde push** of `-value + moveValue` to every parent via unmove; the parent does `BestValue = max(...)`, `UnknownChildren -= 1`, and becomes final if UnknownChildren == 0 or BestValue == UpperBound, propagating recursively (a depth-first cascade, not ply-synchronous).
   - **final**: every remaining Unknown = 0 (draw by cycle).
   So the counting generalises WDL outcounting to scores: a max-accumulator plus a child counter, with a decreasing upper bound U as the "can't be beaten" criterion. Proof that after pass U all positions of value U are final is cited to Allis et al. 1991. Captures (conversion): the paper only says the unmove function generates predecessors "through either a normal move or a capture move" (for a capture unmove it must add stones to the board), and the move value enters as `-child + points`. How child values in smaller databases enter the counts is not described [background, not verified from PDF: in Allis et al. 1991 they are known at init].
7. **Parallelism / memory access**: database partitioned across processors; **cyclic distribution** (entry g on processor g mod p) to randomise load (block distribution was badly imbalanced: 17 stones on 64 CPUs went 4,113 s -> 2,976 s). Update messages (address 4 B + value 1 B) are buffered per destination and **combined** (combining factor 12-52, e.g. 27.7 for 17 stones / 64 CPUs). A worker thread plus a communication thread per node (asynchronous sends; minimum batch of 20 unless idle; biggest buffer first). Local parents updated directly. Termination detection per iteration; dynamically grown buffers avoid deadlock. Messages are order-independent (commutative max/decrement), which is what makes deferral legal.
8. **Draws, cycles**: a cycle = draw with 0 points to both sides; all unresolved after the U=1 phase are set to 0.
9. **Final storage**: minimal bits per value (17 stones: 6 bits, 38 MB on disk).
10. **Verification**: not discussed.
11. **Notable tricks**: message combining of commutative updates; cyclic (mod p) ownership; "final if BestValue reaches the current upper bound" early cut; Status=Propagated to guarantee exactly-once notification.
12. **Mapping to for_each**:
    - init: gather over the full range.
    - threshold phase U: scan is a map over the full range selecting "propagatable" entries (could be a frontier bitset); the push is a **scatter** of commutative updates (max on BestValue, decrement on UnknownChildren). In shared memory this needs atomics (fetch_max via CAS, fetch_sub) or the Bal-Allis trick itself: bucket updates by owner (thread-local buffers keyed by index range) and apply them per owner in a second, race-free pass (a two-phase scatter -> sort/bucket -> gather). The recursive cascade does not fit a single `for_each`; replace it with an iterate-until-empty loop of frontier passes inside each threshold.
    - final: gather over the full range.

---

## 2000-ICGA-Lincke-Marzetta.pdf

1. **Citation**: Thomas R. Lincke and Ambros Marzetta, 2000, *ICGA Journal* 23(3), pp. 131-138, "Large Endgame Databases with Limited Memory Space".
2. **Scope**: Awari databases by stones on board, all up to 35 stones (35 = 13.5 GB) on a **1 GB RAM** machine; 36 (17.5 G configurations) and 37 (22.5 G) under construction. 27 stones = 1.2 G, 34 = 10 G. 2-bit algorithm: 29 (2.3 GB) and 30 (3.1 GB); 1-bit: 31 (4.3 GB), 32 (5.7), 33 (7.7); 1-bit + dual indexing + caching: 34 (10 GB), 35 (13.5 GB). Run times not given.
3. **Metric**: stone difference (score) of a configuration (captured stones ignored), range -N..N, stored in 8 bits (7 would do). Argues WDL-per-capture-state would need 2·(48-N+1) bits, so a score is more compact.
4. **Indexing**: binomial count C(N+11, 11) configurations per N; unreachable configurations (e.g. all pits of non-mover occupied; 47 stones) are *not* excluded. No board symmetry; colour symmetry: compute South-to-move only and rotate the board 180 degrees after every move (this rotation destroys index locality). Slicing by N (48 separate databases 0..46, 48).
5. **Working storage**: 1 byte/configuration (fits); 2 bits in RAM (WLD-k); final scheme = **1 bit in RAM (W/U) + 2 bits on disk (W/L/D/U)**, disk only accessed sequentially. Plus a precomputed "support database" of capture-move values.
6. **Passes, in order**: the score database is decomposed into **N WLD-k databases** (value >= k win, <= -k loss, else draw); value = derived from the sequence of WLD-k results. Per k:
   - **Preprocess (init + conversion)**, one sequential scan: terminal values and capture moves resolved from the precomputed support database (capture values from smaller databases computed once for all k). Winning capture or won terminal -> RAM bit W; drawn terminal or some capture draws -> disk D (really ">= draw"; safe because any later win arrives via back-propagation); lost terminal or all moves are losing captures -> disk L and **push W** to all predecessors' RAM bits. Configurations already D in WLD-(k-1) set to D directly (redundancy between thresholds).
   - **Calculation**: repeated full sequential scans until no change: for each configuration with disk value U (and RAM bit U): **forward (pull)** check: if all non-capturing successors are W (RAM bits, random access) -> disk L, and **push** W into all predecessors' RAM bits (random access). This is "grandfather"-style **verification** (successor check), not counting; no counters are stored.
   - **Postprocess**: merge RAM and disk values (U -> draw).
   The 2-bit variant (Figure 4) is the same loop with both bits in RAM. The WLD-k passes are independent (N-way parallelism) but run sequentially to reuse results.
7. **Parallelism / memory access**: all disk accesses sequential; all random accesses (successor check, predecessor marking) hit the RAM bitmap. **Dual indexing**: RAM bits are indexed for North-to-move, disk values for South-to-move, so no board rotation is needed when stepping from a disk configuration to its successors/predecessors; this cut cache misses by at least 3 orders of magnitude (16 KB blocks, LRU), good with 10 % of the bitmap cached, enabling databases 80x larger than RAM. Rule: fits -> 1 byte; RAM >= 1/4 DB -> 2-bit; >= 1/8 -> 1-bit; >= 1/80 -> 1-bit + dual indexing.
8. **Draws, cycles**: repetition divides the remaining stones between players; unresolved at the end of WLD-k = draw for that threshold.
9. **Final storage**: 8 bits per configuration; no compression found for awari.
10. **Verification**: not discussed.
11. **Notable tricks**: score -> threshold-wise WDL decomposition (lets one use 1-2 bit WDL machinery for valued games; monotonicity between thresholds seeds the next pass); 1-bit-in-RAM + sequential disk (Thompson-style); precomputed conversion ("support") database; dual indexing so that the index of a successor is close to the index of its parent.
12. **Mapping to for_each**:
    - Preprocess: gather over the full range for the configuration's own disk value; the "push W to predecessors" part is a scatter into a **bitset** (atomic `fetch_or` on 64-bit words, idempotent, race-free with atomics).
    - Calculation pass: the successor check is a gather (reads RAM bits, writes its own disk value); the W push is a bitset scatter (atomic OR). Because W bits only ever turn on (monotone), reading a bit set "earlier in the same pass" is harmless (chaotic relaxation, still converges to the same fixpoint), so `par` + atomic OR is correct without double buffering.
    - Alternative purely-gather formulation: drop the push and let every pass do "am I W? (some successor L)" and "am I L? (all successors W)" by pulling: a full-range gather, at the cost of more passes.
    - Postprocess: gather.

---
## 2000-ICGA-Irving-Donkers-Uiterwijk.pdf

Note: this file is **"Solving Kalah"**, not an Awari paper. Kalah is a mancala relative of Awari, but it has no repetitions.

1. **Citation**: Geoffrey Irving, Jeroen Donkers and Jos Uiterwijk, 2000, *ICGA Journal* 23(3), pp. 139-147, "Solving Kalah".
2. **Scope**: Kalah(m,n) (m holes per side, n counters per hole). Full-game databases for small instances (largest Kalah(4,3): 4,604,996 nodes in the game graph); larger instances (4 <= m <= 6, n <= 6, except 6,6) weakly solved by MTD(f) search plus endgame databases of up to 20 active counters for 6 holes. Kalah(6,5) solved in 4.7 h and Kalah(6,4) in 35 s on an 800 MHz Athlon with 256 MB (107 MB 20-counter database, 96 MB TT with 8 M entries). Endgame database sizes for Kalah(6,n): 17 counters 51,694,042 positions (24.6 MB) ... 30 counters 11,054,221,305 (5,271 MB).
3. **Metric**: score = stones the side to move will capture from the active counters (captured counters do not re-enter play); 4 bits per entry, "a lower bound is stored if necessary" (values saturate).
4. **Indexing**: combinatorial ranking of configurations with n active counters on 2m holes, excluding positions with one side empty: Size(m,n) = C(n+2m, 2m) - 2·C(n+m, m) + 1 = O(n^{2m}). No unused slots; unreachable positions included. Side to move normalised (mover vs waiting player). Slicing by number of active counters (2..20).
5. **Working storage**: whole endgame database in RAM (4 bits/entry). The full-game graphs were built explicitly ("complete game graph built from the opening position, then values computed backwards"); in-memory up to Kalah(4,3), beyond that disk-bound.
6. **Passes, in order**:
   - Endgame databases: **forward (pull) only, no retrograde analysis**: "the values of the positions were not calculated by performing reverse moves, but by performing a forward minimax search for every position" because unmoves are costly in mancala games. Since Kalah has no cycles and every capture lowers the slice, but non-capturing sowing stays in the slice, the forward search inside a slice recurses through same-slice positions until it reaches captures (which probe smaller slices: **conversion**) or the end of the game. [How the within-slice recursion is ordered or memoised is not described.]
   - Full-game databases: stage 1 forward enumeration of the game graph from the start; stage 2 backward value computation. Layering: subgraphs g(p,q) by captured counts form layers l(r), r = p+q; edges only go within a subgraph or down to a lower layer.
   No losing-side technique is needed (score maximisation over children, acyclic).
7. **Parallelism / memory access**: sequential; a Cilk-5 parallel version (Young Brothers Wait) was planned for Kalah(6,6).
8. **Draws, cycles**: Kalah has no repetitions, so the graph is acyclic; draws are just score 0.
9. **Final storage**: 4 bits per entry, uncompressed.
10. **Verification**: not discussed.
11. **Notable tricks**: for acyclic, slice-monotone games the database can be filled by pure forward evaluation; dense combinatorial index with no holes; safe futility pruning from score bounds; enhanced transposition cutoff extended to database probes and futility (tree up to 8x smaller, running time about 3x faster for Kalah(6,4)).
12. **Mapping to for_each**: a forward pull is a gather (writes only itself) **if** every successor read is already final. Within a Kalah slice, non-capturing moves stay in the slice, so a single full-range gather does not work without an order. Either (a) recursive memoised search (not a `for_each`), or (b) a topological level order inside the slice (for example by a potential that strictly decreases along non-capturing moves, if one exists: [not discussed in the paper]), then one gather `for_each` per level. Conversion probes into smaller slices are read-only gathers.

---

## 2002-ICGA-Romein-Bal.pdf

1. **Citation**: John W. Romein and Henri E. Bal, 2002, *ICGA Journal* 25(3), pp. 162-165 (Notes), "Awari is Solved".
2. **Scope**: all of awari: 889,063,398,406 positions, one database per stone count 0..48 (47 skipped: unreachable). The 48-stone database has over 200 billion positions. 144 Pentium III 1.0 GHz processors, 72 GB distributed RAM, 1.4 TB disk, Myrinet. 51 hours total. 1.0 petabit exchanged. Result: draw. Final database on the web server: 778 GB.
3. **Metric**: score (eventual stone difference under optimal play); exact encoding is in the IEEE paper.
4. **Indexing**: not detailed (refers to the companion paper).
5. **Working storage**: notes that Bal-Allis (1995) would need more than 350 GB; the new algorithm "uses the memories economically" (details in the companion paper).
6. **Passes**: not detailed in this note: "retrograde analysis ... starting in the final positions, and searching backward". Key idea stated: **migrate work instead of data**: if A needs data from B, A sends the work to B asynchronously and continues (as in transposition-table-driven scheduling, Romein et al. 2002).
7. **Parallelism**: asynchronous work migration; overlaps processors, disks, memory and network.
8. **Draws, cycles**: rules appendix: repetition divides the remaining stones evenly; a move must leave the opponent a move if possible; the player who cannot move loses the remaining stones.
9. **Final storage**: 778 GB database served via Java applet.
10. **Verification**: refers to the companion paper.
11. **Notable tricks**: work migration; the bulk of the note re-annotates the Olympiad 2000 games (Marvin 82 % correct moves, Softwari 87 %).
12. **Mapping to for_each**: no algorithmic detail beyond the IEEE paper; see below.

---

## 2003-IEEE-Romein-Bal.pdf

1. **Citation**: John W. Romein and Henri E. Bal, 2003, *IEEE Computer* (per file name; the extracted text has no venue line), "Solving the Game of Awari using Parallel Retrograde Analysis".
2. **Scope**: 889,063,398,406 positions; per-stone-count databases; 48 stones = 203,648,015,936 entries (178 GB); 46 stones 136.9 G, 41 stones 47.1 G. 72 dual Pentium III 1 GHz nodes (IBM), 1 GB RAM + 20 GB IDE disk each, Myrinet 2 Gb/s, C + GM. 48-stone database 15.2 h, all databases 51 h. 20-30 MB/s per node during processing; 130 TB communicated; about 10 TB disk I/O.
3. **Metric**: score in -48..48 (7 bits per entry). Score(p) = max over children c of (CapturedStones(p,c) - Score(c)). Terminal: negated number of stones on board. Statistics: 60.0 % of positions winning, 11.3 % drawn, mean score +2.29; in 22 % of positions with a capture available, a non-capture is better.
4. **Indexing**: Gödel numbers modified to skip unreachable positions (all six pits of the non-mover occupied), saving up to 27 % in time and size. South-to-move only (rotate the board). Slicing by stones on board; within a slice the database cannot be split further because of cycles. Parents and children map far apart, so access is random.
5. **Working storage**: up to 41 stones: Bal-Allis algorithm, 10 bits/entry in RAM. 42-48 stones: new algorithm, **2 bits/entry in RAM** (States: Win, DrawOrWin, Loss, Unknown), plus Scores (7 bits) on disk accessed sequentially; about 3x slower but 5x less memory. Precomputed auxiliary capture-score database (from Lincke-Marzetta). Disk handled by mmap hints, aggressive prefetching, lazy write-back.
6. **Passes, in order** (per database, for Bound = 1..NrStones: one **WDL window (-Bound, Bound)** per threshold):
   - **PreProcess (init + conversion)**: for each Unknown entry, DoCaptures: best capture score from the auxiliary database; `>= Bound` -> Win; otherwise if it exceeds -Bound -> DrawOrWin [the extracted comparison operators are partly garbled by OCR]; if there are no non-capturing moves and the position is lost -> Loss and **push** Win to all parents. Terminal positions are losses.
   - **Process**: one sequential scan; for each Unknown entry, check whether all non-capture children are Win (**verification / grandfather check**, no counters); if so -> Loss, then for each non-capture parent not already Win: set Win, and **recursively** re-test each grandparent (DoNonCaptures on grandparents). Because of the recursion, a single scan suffices (unlike Lincke-Marzetta's repeated scans).
   - **PostProcess**: Win -> Scores += 1, Loss -> Scores -= 1, then reset to Unknown; DrawOrWin stays (drawn positions remain drawn for all larger windows); Unknown -> DrawOrWin.
   - Score = sum over thresholds of (+1 win / -1 loss), i.e. an **accumulate-over-thresholds** decomposition of a valued game into N WDL problems. Iterations are independent in principle but run in increasing order to reuse "draw stays draw".
   - Parallel version of the verification: the owner of U generates child 1 and sends "check child" (msg 1); the child's owner continues to child 2 if it is a Win, else drops the message; at the end the original owner gets "is a loss" (msg 2), marks it and sends "parent is a win" (msg 3) to the parents' owners, who then send "check grandparents" (msg 4). This is the **work-migration** form of the successor check (a chained gather).
7. **Parallelism / memory access**: entries interleaved (CPU 2 stores 2, 146, 290, ... with 144 CPUs) for load balance; all communication is asynchronous and batched per destination; global termination detection; disk only sequential.
8. **Draws, cycles**: nodes in cycles without an outlet score 0 (repetition divides the remaining stones). Per window, whatever is still Unknown/DrawOrWin after processing is a draw for that window. Choosing a move from scores alone is not always enough (must avoid repetition when ahead; may need a search; [8]).
9. **Final storage**: 7 bits per entry; 778 GB total (from the note).
10. **Verification**: ECC memory, CRC on network and disk; the 2-bit and 10-bit implementations gave identical databases up to 41 stones; a separate parent/child consistency checker; full recomputation on 64 instead of 72 nodes with bitwise comparison; comparison with Lincke-Marzetta up to 36 stones (on databases rebuilt to include unreachable positions).
11. **Notable tricks**: score = sum of WDL threshold outcomes (2 bits in RAM regardless of value range); "draw stays draw" monotonicity between thresholds; recursive grandparent re-check inside a single scan; work migration instead of remote reads; modulo-p interleaving; skip-unreachable Gödel numbering.
12. **Mapping to for_each**:
    - PreProcess: full-range gather for the own state; the "parents := Win" push is a scatter into a 2-bit array (needs atomic CAS on the containing word; Win is monotone within a window).
    - Process: the successor test is a gather; the Win push is a scatter; the recursive grandparent re-test is a cascade that does not map to one `for_each`. Restructure as Lincke-Marzetta-style repeated full-range passes until fixpoint (each pass = gather test + atomic-OR scatter of Win bits), or as frontier passes: frontier = new Loss bitset -> scatter Win bits to predecessors -> the candidate set (predecessors of new Wins) becomes the next bitset to test.
    - PostProcess: full-range gather (own entry only), race-free.
    - Thresholds: a sequential outer loop of N rounds of passes; the score accumulator is a per-element gather.

---
## 2014-ArXiv-Irving.pdf

1. **Citation**: Geoffrey Irving, 2014, arXiv (published version: [background, not verified from PDF: IEEE/ACM workshop proceedings 2014]), "Pentago is a First Player Win: Strongly Solving a Game Using Parallel In-Core Retrograde Analysis".
2. **Scope**: Pentago (6x6 board, four 3x3 quadrants; place a stone then rotate one quadrant by 90 degrees; five in a row wins). 3,009,081,623,421,558 (3.0e15) states with symmetry removed, all but 0.3 % reachable. Computed in about 4 hours on 98,304 threads (3·2^15; 49,152 cores, 2,048 nodes) of NERSC's Cray XC30 Edison; slices 35 -> 19 took 2.7 h; 18 and 17 written before the 4 h wall clock limit; two smaller jobs (192 and 128 nodes) finished down to slice 0. Output: 3.7 TB database for 0-18 stones (LZMA). Peak memory 213 TB uncompressed for two adjacent slices. Result: first player win (corner openings tie).
3. **Metric**: WDL (tie possible: simultaneous fives or full board). 2 bits per position: bit 1 = win vs loss/tie, bit 2 = win/tie vs loss (avoids ternary arithmetic). Positions are stored **rotation-abstracted**: for each board b, a function f_b: L -> {-1,0,1} over the 256 quadrant-rotation states, packed as two 256-bit tables (64 bytes).
4. **Indexing**: slices by stone count (the game is **forward-only/divergent**: every move adds a stone, so no cycles and no unmoves needed). Within a slice: **sections** = number of black and white stones in each of the four quadrants, keeping only sections lexicographically minimal under the global symmetry D4 (at most 8,239 sections per slice); a section is a 4-D array indexed by (quadrant state a, b, c, d) with each quadrant state rotation-minimal and ordered by a precomputed table, so that reflected pairs are adjacent (keeps 8x8x8x8 blocks invariant under all symmetries). Symmetry group: local Z4^4 (256 quadrant rotations, abstracted into the value function) semidirect D4 = 2048 elements. Overcount: 5.4 % from rotation abstraction + 9.3 % from section-level symmetry = 15.2 %. Slice 36 is computed on demand.
5. **Working storage**: two adjacent slices in RAM (n+1 input, n output), all blocks compressed with Snappy (average ratio 0.26, assumed 0.4), with a manual compacting allocator to fight fragmentation. Blocks typically 256 KB uncompressed (8^4 boards x 64 B). No disk until the final slices.
6. **Passes, in order**:
   - **init**: terminal positions are folded into the evaluation (five-in-a-row predicates f_c(s); 36 stones = game over). Slice 36 computed on demand.
   - For n = 35 down to 0, one **backward sweep** per slice, which is a pure **forward / successor-scan (pull)** in the per-position sense: v_c(s) = max over placements a of (1 if a makes five, else max over rotations b of h_c(b)), with h = -v of the opponent unless fives appear. Work unit = **block line**: an output line in section n (k x 1 x 1 x 1 blocks) depends on exactly one input line in a child section of slice n+1 (the quadrant where the stone is placed), possibly after a symmetry transform. Steps: **gather** the input line (request blocks from owners), **compute** (rmax over the 256 rotation states, five-in-a-row masks, symmetry transforms), **scatter** the output blocks to their owners, who **combine with max** (each output block receives up to 4 contributions, one per quadrant).
   - No losing-side technique is needed (acyclic, one pass per level). No frontier: every pass covers the full slice.
7. **Parallelism / memory access**: MPI + Pthreads (8 ranks per node, each 1 communication thread + 5 workers). **Deterministic pseudorandom partitioning**: a random-access permutation (Black-Rogaway cipher) of block lines across ranks, each block randomly assigned to one of its four lines (Threefry RNG), O(1) owner lookup from a shared seed; load within 10-20 % for big slices (89 % of the work in slices 20-28). Asynchronous: at most 5 block lines in flight per rank (request inputs, compute when they arrive, send outputs, merge incoming outputs). Asynchrony is needed only because of memory: with 4x the RAM it could be bulk-synchronous epochs. Workers idle 16.4 % (I/O excluded), 51 % including I/O. SSE: rmax on a 256-bit table in 152 instructions (0.6 instructions per position); 1.81x faster than 64-bit code on Edison. An out-of-core version estimated 2.25x slower (3.6 PB I/O uncompressed).
8. **Draws, cycles**: no cycles; ties come from the rules (simultaneous fives or full board).
9. **Final storage**: 0-18 stones written with LZMA per block (.pentago "supertensor" format); positions with more than 18 stones recomputed on demand by a serial solver (16 s per 18-stone position's children).
10. **Verification**: unit tests and "bootstrap" tests comparing the rotation-abstracted routines with simple per-board routines; backward vs forward-search comparison near the end and near the start (randomised values at slice 4 or 5); random sample boards per slice written during the run and checked by forward search for slices >= 20; per-slice W/L/T counts; ECC (SECDED) hardware; LZMA checksums on files.
11. **Notable tricks**: rotation abstraction (evaluate 256 related positions at once as bit tables: SIMD over positions); section/block layout where one move changes only one coordinate (branching factor for communication 12.2 -> 4); scatter-with-max combine; two-slice working set; pseudorandom ownership; per-block compression in RAM.
12. **Mapping to for_each**: for a forward-only game the whole construction is one gather per level: `for_each(par, slice_n.begin(), slice_n.end(), [](idx){ v[idx] = max over children in slice n+1 })`: race-free, since it reads only the previous (frozen) slice and writes only itself. Irving's line decomposition turns it into a scatter-with-max (each line writes into blocks owned by others): this is a performance layout, not a necessity; in shared memory one can keep the gather form, or use the line form with per-quadrant partial buffers combined by a final gather (max over 4 buffers). Relevance to draughts: only as a template for an acyclic slice (e.g. one where every move is a capture or irreversible move); the in-slice cycles of draughts kings rule out a single pull pass.

---

## 2009-GoNC3-Lim-Nievergelt.pdf

1. **Citation**: Lim Yew Jin and Jürg Nievergelt, 2009, *Games of No Chance 3* (MSRI Publications vol. 56, pp. 163-176), "Tigers and Goats is a draw".
2. **Scope**: Bagha Chal (5x5 grid, 4 tigers, 20 goats; 39-ply placement phase then sliding phase; Tiger wins by capturing 5 goats, loses if it cannot move). State space split into S0 (placement phase: 3,316,529,500 board images, about 33 billion positions) and S1..S5 (sliding phase with 21-k goats and k empty points): positions 66,962; 666,350; 4,211,390; 18,939,930; 64,376,340, total **88,260,972** symmetry-reduced positions, solved by retrograde analysis (in the earlier work, Lim & Nievergelt 2004, ICGA Journal 27(3)). The new part is the placement phase: forward search plus a "halfway" database of 216,519,984 positions computed on demand (of about 1.61e9 possible), five months on an Apple PowerMac G5 1.8 GHz. Result: draw.
3. **Metric**: WDL only (win = 5 captures, loss = Tiger immobile, draw = neither side can force a win). Halfway database: **1 bit per position** (boolean "Tiger can at least draw" or "Goat can at least draw", depending on the search).
4. **Indexing**: symmetry reduction by the 8 square symmetries (Polya counting used for exact sizes). The sliding-phase index function is not described in this paper. Slicing by number of goats captured (S_k -> S_{k+1} monotone during the sliding phase; in S0 captures can go back and forth relative to empty points).
5. **Working storage**: not discussed for the retrograde phase in this paper (it cites "[Wu 2002] R. Wu and D. F. Beal, A memory efficient retrograde algorithm and its application to Chinese Chess endgames" as the method used). [background, not verified from PDF: the Wu-Beal algorithm keeps no per-position counters; it stores compact WDL information and re-verifies the losing side by successor checks, which is the "bitmap / no counters" approach the caller refers to.] In the forward phase the endgame database and the halfway database (1 bit/position) are kept in RAM.
6. **Passes, in order**:
   - **Sliding-phase databases (retrograde)**: per subspace S1 -> S5 (fewer to more empty points; captures move from S_k to S_{k+1}, which are **conversions** into an already solved subspace). Pass structure is not described in this paper beyond "using retrograde analysis [Wu 2002]". The statistics in Table 2 (W/D/L per side to move and per capture count) show that WDL alone is stored.
   - **Placement phase (forward)**: 39-ply alpha-beta-style proof searches; "cut the tree in half" by pitting a selective heuristic player (neural network, trained by co-evolution) against an exhaustive opponent to prove hypotheses like "Goat can at least draw"; a **halfway database** at ply 19 (Tiger to move after 10 goats placed; 6-10 goats on board, i.e. S11-S15) filled **on the fly**: when the search meets an unknown halfway position, it runs a full-width search to the endgame database (at most 20 plies) and stores the 1-bit result; only positions needed to "trap" the search are computed (216 M of 1.6 G). Verification reruns used intermediate databases at plies 21/31 (Tiger) and 23 (Goat).
   - The losing-side technique for retrograde: not discussed in this PDF (see background note above).
7. **Parallelism / memory access**: earlier results used an 8-node Linux cluster with synchronous distributed search; the final run was sequential (AltiVec SIMD for the neural network, precomputed move/index/symmetry tables). Symmetry-avoiding move generation: among moves producing symmetric children keep only the lowest-index one (ply 6: 18.6 M -> 2.4 M generated vs 93,812 distinct).
8. **Draws, cycles**: draw by repetition = position where neither side can force a win and each can avoid a loss (i.e. undecided after retrograde analysis). Goat with 18 or more goats can be blocked (loses) only in exceptional cooperative cases.
9. **Final storage**: 1 bit per halfway position; endgame database format not described.
10. **Verification**: two independent re-searches with separately coded C programs (2 months): one from the root using only win-or-draw halfway positions; one re-proving those halfway positions against the endgame database. A third program "re-proved" the results with aggressive pruning (positions with >= 2 goats captured pruned): "Goat can at least draw" in about 7 h (7,735,443,119 nodes); "Tiger can at least draw" in about 48 h (40,521,418,103 nodes) with 2 GB of intermediate databases.
11. **Notable tricks**: one-sided 1-bit databases ("X can at least draw") suffice to prove a draw with two searches (as in Gasser's opening search); lazily filled partial midgame database as a search cut; heuristic-vs-exhaustive proof searches; symmetry-avoiding move generation.
12. **Mapping to for_each**: the retrograde part (method not described here) would map like the other WDL schemes. The on-demand halfway database and the proof searches are tree searches, not `for_each` passes. The one transferable data-parallel idea is the **1-bit "at least draw" bitmap**, which turns a WDL database into two independent monotone boolean fixpoints (each computable by bit-parallel gather passes).

---

## 2011-ACG-Cazenave-Jouandeau-Saffidine.pdf

1. **Citation**: Abdallah Saffidine, Nicolas Jouandeau and Tristan Cazenave, 2011 (published 2012), *Advances in Computer Games 13* (ACG 2011, Tilburg), LNCS, pp. 196-207, "Solving Breakthrough with Race Patterns and Job-Level Proof Number Search" (HAL copy hal-01499675).
2. **Scope**: Breakthrough on small boards: 5x4, 4x5, 5x5 and **6x5** solved (all second-player wins). 6x5: 25,638 s with 10 k-descent client searches, 47,134 s with 100 k. Cluster: 17 machines with quad-core 3.2 GHz i5 and 4 GB each; up to 64 clients plus a master.
3. **Metric**: win/loss only (proof-number search; Breakthrough has no draws).
4. **Indexing**: none (tree search; no database). State-space bound 2^{2m}·3^{(n-2)m}; Haugland's earlier retrograde analysis (5x5 and 3x7 solved) skipped positions with a pawn on the second-to-last row (win in 1), reducing the space to 2^{4m}·3^{(n-4)m} (factor (3/2)^{2m}, 58 for m = 5). [Haugland's work is only cited.]
5. **Working storage**: server keeps the main PN tree (expanded and "touched" nodes; e.g. 4x5 with 64 clients: 232 k expanded, 1.93 M touched); clients run bounded PN searches (1 k-100 k descents).
6. **Passes**: not a pass-based construction. Algorithm: **Parallel PN²** (PPN²): a best-first PN tree on the server; each most-proving, not reserved leaf is sent to a client that runs a bounded PN search and sends **partial (PN, DN) updates** every p descents (best p = 100: 186 s vs 263 s without partial updates on 4x5); reserved leaves are flagged (no virtual loss, so 0/infinity stay attractors). **Race patterns**: 26 hand-written static patterns (up to 4x3, order up to 2) with cell types occupied/free/passive/crossable/don't-care; if the lowest-order matching patterns of the two sides overlap only on don't-care cells, the race is decided statically (up to 4 moves, in general 2n moves for order-n patterns) except in zugzwang. Patterns cut solving time 5.96x (4x5) and 9.85x (5x5).
7. **Parallelism**: master/worker job-level parallelism; speedup 18.3 with 64 clients on 4x5 (1 k descents).
8. **Draws, cycles**: no draws, no cycles (pawns only move forward).
9. **Final storage**: not discussed (no database).
10. **Verification**: not discussed; pattern correctness is only hand-checked ("we had to limit the number of patterns used to keep confidence in their correctness").
11. **Notable tricks**: static early-termination patterns for races (a database-free "distance" bound); partial-result streaming in job-level PN search.
12. **Mapping to for_each**: PN search is not a data-parallel map. The pattern matcher, however, is a per-position predicate, so a retrograde builder could use it as a gather in the init pass (mark statically decided races). Haugland's "do not store win-in-1 positions" is an index-space reduction that could also apply to draughts (e.g. men about to promote).

---

## Group synthesis

### Distinct primitives in this group

1. **Successor verification ("grandfather" check)**: Gasser 1991 (win backup), Thompson-style v1 in Gasser 1996, Lincke-Marzetta 2000, Romein-Bal 2003. Losing-side test = "all successors are wins", recomputed whenever needed. Needs no counters, so 1-2 bits per position suffice.
2. **Outcounting with the counter merged into the value byte**: Gasser 1996 (Val/Count union; values grow from 0 up, counts from 255 down; collision in 8-7). Lazy counter initialisation on the first won child.
3. **Counter + best-value accumulator for valued games**: Bal-Allis 1995 (UnknownChildren + BestValue + Status, early finalisation at BestValue == current upper bound U; phases U = N..1; leftovers = 0).
4. **Score = sum of WDL threshold problems**: Lincke-Marzetta 2000 (WLD-k databases, "draw at k means draw at k+1", support database for captures), Romein-Bal 2003 (window (-Bound, Bound), Scores += ±1 per round; 2 bits in RAM, 7 bits on disk).
5. **Push vs pull selected per ply by cost**: Gasser 1991 (#Wins·AvgPred > #Unidentified -> scan unknowns and pull).
6. **Memory hierarchy split**: random access only to a small RAM bitmap, sequential access to disk (Lincke-Marzetta; Romein-Bal with prefetch/mmap hints). **Dual indexing**: index the RAM bits by the side that the random accesses use (no board rotation, so locality improves by >= 1000x in cache misses).
7. **Distributed push with commutative, combinable updates**: Bal-Allis (buffer per destination, combining factor 12-52, cyclic g mod p ownership), Romein-Bal (work migration: send the remaining computation, not a read request; chained successor checks across owners).
8. **Forward-only level sweep (pull per level, no unmoves)**: Irving 2014 Pentago (slice n from slice n+1, 2 slices in RAM), Irving-Donkers-Uiterwijk 2000 Kalah (forward minimax per configuration because unmoves are costly).
9. **Layout for locality of moves**: Pentago sections/blocks where a move changes one coordinate (one input line per output line); deterministic pseudorandom ownership; per-block compression in RAM; scatter-with-max combine.
10. **SIMD over related positions**: Pentago rotation abstraction (a 256-position bit table per board; rmax in 152 SSE instructions); Gasser's family-level init (all black placements for one white placement at once).
11. **Compression during construction**: Gasser 1991 (retrograde analysis on RLE files with a sparse index), Irving 2014 (Snappy blocks in RAM).
12. **1-bit one-sided databases for proofs**: Gasser 1996 opening search (two bitmaps, two searches), Lim-Nievergelt 2009 ("X can at least draw", on-demand halfway database).
13. **Verification by an independent algorithm**: forward checker (Gasser/Balz, Romein-Bal consistency checker), cross-implementation and cross-processor-count bitwise comparison (Romein-Bal), sampling plus forward search (Irving).

### Lineage

- Thompson 1986 (verification) -> Gasser 1991 (verification, plus the pull alternative) -> Gasser 1996 (outcounting, merged byte, O(E)).
- Allis-van der Meulen-van den Herik 1991 (awari databases, counters + best value, Gödel numbers) -> Bal-Allis 1995 (distributed version, message combining) -> Romein-Bal 2002/2003 (10-bit Bal-Allis up to 41 stones; 2-bit threshold algorithm above, work migration).
- Thompson 1986 (1 bit in RAM) -> Lincke-Marzetta 2000 (WLD-k, 1 bit RAM + 2 bits disk, dual indexing) -> Romein-Bal 2003 (2-bit parallel variant, single scan with recursive grandparent re-check, cross-checked against Lincke's databases up to 36 stones).
- Kalah 2000 (forward evaluation for acyclic mancala) and Pentago 2014 (forward level sweep at 3e15 scale) form the acyclic branch: no counters, no unmoves, one pull pass per level.
- Wu-Beal 2002 (memory-efficient retrograde analysis, cited) -> Lim-Nievergelt 2004/2009 Tiger and Goats endgames; Gasser's 1-bit two-search trick -> Lim-Nievergelt's 1-bit halfway database.

### What this means for `std::for_each(policy, ...)` passes in egdb_fast

- **Race-free gathers**: init/terminal marking, conversion probes into smaller slices (read-only), pull-style "all successors won?" checks, postprocess/merge, per-level forward sweeps in acyclic slices, threshold accumulation (Scores += ±1).
- **Scatters that need atomics or a double buffer**: pushing wins to predecessors (idempotent and monotone: atomic OR into a bitset, or compare-exchange from "unknown" so exactly one thread enqueues), outcounting decrements (atomic fetch_sub; the thread that reaches 0 owns the transition; counters initialised in a separate gather pass, not lazily), best-value max (CAS loop).
- **Do not fit a single for_each**: recursive cascades (Bal-Allis propagate, Romein-Bal grandparent recursion, Gasser's work-list). Convert them to ply-synchronous frontier passes (bitset of positions decided at ply d -> scatter into the next frontier) or to repeated full-range pull passes until a fixpoint (Lincke-Marzetta style). Both are expressible as `for_each` per pass; the pull variant needs no atomics at all, at the cost of more passes, and is correct under `par` with chaotic relaxation because the W/L sets grow monotonically, though the ply/depth value then needs a double buffer (or a "decided at ply < d" test) to stay exact.
- Valued games add one outer sequential loop (thresholds), not a different pass shape.
