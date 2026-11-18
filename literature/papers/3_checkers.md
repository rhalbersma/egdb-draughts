# Group 3: Checkers (Chinook lineage + Trice–Dodgen)

Reading status: all nine PDFs returned text through `read_file_content` and were read in full.
- The 1994 and 2003 Trice–Dodgen files were too large for one tool result. Both were dumped to disk and read in chunks to the end.
- Some PDF artefacts remain:
  - The 1994 Appendix B formulas are garbled in the extraction.
  - The 2005 IJCAI paper lost most of its exponents (for example "10^20"). Where a number was garbled, it is reconstructed from the other papers and marked.
  - The 2007 Science paper has no Supporting Online Material in the PDF. Its verification details live there, so they are missing.

Notes marked **[my analysis]** are my inferences for the `std::for_each` mapping. They are not claims made by the papers.

---

## 1992-ACF-bulletin-Lake-Schaeffer-Treloar.pdf

1. **Citation**: R. Lake, J. Schaeffer, N. Treloar, "The 3B1b3W Endgame", *ACF Bulletin*, 1992 (a short article for players).
2. **Scope**: The 3B1b3W endgame: 3 Black kings plus 1 Black checker against 3 White kings (7 pieces).
   - At the time, Chinook had solved all 6-piece positions (2.7 billion) and was working through the 7-piece set (34.8 billion).
   - The 3B1b3W endgame is broken down by the rank of the Black checker (ranks 1–7). Every rank slice has exactly **58,902,480** positions per side to move.
   - **[my analysis]** This equals 4·C(31,3)·C(28,3): the checker on one of the 4 squares of its rank, then the 3 Black kings, then the 3 White kings. So each rank slice is a dense combinatorial block.
   - Hardware and build time: not discussed.
3. **Metric**: WDL is stored.
   - The article also reports a **moves-to-conversion** figure. A conversion is "any capture or checker advance", i.e. an irreversible move.
   - That figure is explicitly a **lower bound**: "our program takes a shortcut which prevents us from knowing with certainty how many moves it takes… the actual number may be slightly greater."
   - The longest case is 68 moves (136 ply) for 3B1b(7)3W. Other ranks: 36, 42, 41, 41, 26, 26 moves.
   - Bits: not discussed.
4. **Indexing**: Slices by the rank of the checker. Positions are counted "Black to move" and "White to move" separately.
   - "No Capture" counts exclude positions where the side to move has an immediate capture. These are reported as "the positions of interest": roughly 28–33 M of the 58.9 M per slice for Black to move, and 25–32 M for White to move.
5. **Working storage**: not discussed.
6. **Passes**: Described only informally: "finds all positions that it can prove lead to a win or loss in 1 move, then 2, then 3, etc. When there are no more wins or losses to be found, the remaining positions are drawn."
   - Conversion targets are the smaller (6-piece) databases and higher-rank slices.
   - **[my analysis]** The "shortcut" fits the forward in-place sweeps (Gauss–Seidel) of Lake et al. 1994. A position can be resolved in an earlier pass than its true depth because a child decided earlier in the same sweep is already visible. So the pass number at which a position is decided is only a lower bound on its distance to conversion. The 2003 paper says the same about its parallel non-capture phase.
7. **Parallelism**: not discussed.
8. **Draws**: Positions still undecided at the fixpoint are draws. There is also a long discussion of the 40-move rule: Diagram 1 needs at least 68 moves without conversion. No rule is built into the database.
9. **Final storage**: not discussed. Capture positions are excluded from the reported statistics.
10. **Verification**: All 3B1b3W positions from Boland's *Masterpieces* and *Famous Positions* were fed to Chinook. Boland's win/loss/draw classifications were all correct. The lines of play were not checked.
11. **Notable tricks**: Slicing by man rank gives equal-sized, dense slices. Capture positions are treated as "not of interest".
12. **for_each mapping**: The method is not described here; see 1994. The lesson for the user: if you want an exact DTC/DTM from pass numbers, the sweep must be Jacobi-style (read only values decided in earlier passes). In-place sweeps under `par` make the pass count both a lower bound and nondeterministic.

---

## 1994-ACC-07-Lake-Schaeffer-Lu.pdf

1. **Citation**: R. Lake, J. Schaeffer, P. Lu, "Solving Large Retrograde Analysis Problems Using a Network of Workstations", *Advances in Computer Chess 7*, 1994, pp. 135–162.
2. **Scope**:
   - Target: 150 billion positions, i.e. all 1–7-piece databases plus the 4b4w subset of the 8-piece databases (111,378,534,401 positions).
   - Sizes by piece count: 4 pieces 7,092,774; 5 pieces 148,688,232; 6 pieces 2,503,611,964; 7 pieces 34,779,531,480; 8 pieces 406,309,208,481.
   - Hardware: 25–30 departmental workstations, up to 90 at peak (heterogeneous Sun, SGI, HP and MIPS), plus a BBN TC2000 shared-memory machine.
   - Throughput: about 300 M positions/day during term, 425 M/day on average.
   - Estimate: 5b3w (186 billion positions) would take more than a year.
3. **Metric**: WDL only. No distance is stored because of size.
   - During construction each position has **2 bits**: WIN, LOSS, DRAW and UNKNOWN.
   - After the Lookups pass, DRAW means "**≥ draw**" (a lower bound) and UNKNOWN means "no lower bound". The value of a capture is the maximum of win > draw > loss.
4. **Indexing**: Perfect, gap-free combinatorial index per slice, with a de-index function (Appendix B). Only Black-to-move is stored per database. White-to-move is the **mirror** database (colours reversed, board flipped).
   - **Subdivision levels**:
     - (a) By piece count N.
     - (b) By material per side, e.g. 4b4w or 5b3w.
     - (c) By king/checker counts in the form `bk wk bc wc`. The node graph runs from 4400 down to 0044, because promotion only increases the number of kings.
     - (d) By **leading-checker rank per side** (0–6): 49 slices when both sides have checkers, 7 when only one does. All-king nodes are unsliced. Slices are named like `3212.53`.
   - **Squares**: 0–31 counted from Black's side; rank i covers squares 4i…4i+3. Black checkers with leading rank r: C(4(r+1), nbp) − C(4r, nbp) configurations.
   - **White checkers**: the count depends on each Black-checker configuration i, as Avail = 4(rwp+1) − #(Black checkers on squares ≥ 4(7−rwp)). Then NumWP_i = C(Avail_rwp, nwp) − C(Avail_{rwp−1}, nwp), stored per configuration.
   - **Kings**: C(32−nbp−nwp, nbk) and C(32−nbp−nwp−nbk, nwk).
   - **Tuple order**: tuples are ranked from least to most significant as wk, bk, wp, bp.
   - The extracted formula reads NumPos = MaxBP×MaxWP×MaxBK×MaxWK. From the procedure it must actually be (Σ_i NumWP_i)×MaxBK×MaxWK; treat the printed form as garbled.
   - **No symmetry is used**: "because of the presence of checkers and the non-symmetric nature of the board, one cannot take advantage of symmetries".
   - **Sizes**: node 3212 has 11,799,496,800 positions (5.9 GB with its mirror). Its largest slice, 3212.06, has 484,520,400 (243 MB with mirror). The largest 4b4w slice is 2222.66, which is its own mirror: 1,142,505,000 positions, 286 MB. Slices 2222.65 and 2222.56 are mirrors of each other at 957,738,600 each (479 MB together).
5. **Working storage**:
   - 2 bits per position, for the slice and its mirror together.
   - Paging: 8 KB pages of 32,768 positions (page = index >> 15). Each worker uses 250–1,000 page buffers (2–8 MB) under a modified LRU: when the sequential scan crosses a page boundary, the old page is marked least recently used.
   - Dirty pages are flushed periodically as checkpoints.
   - The position queue (4 bytes per entry) lives on disk.
   - Process size 8–15 MB, working set 3–4 MB.
6. **Passes, in order** (Fig. 4, run for each slice):
   1. Set all positions to UNKNOWN.
   2. **DoCaptures** on the original, then on the mirror. Type: **conversion**, full range. For every UNKNOWN position that has a capture, take the maximum over the capture children in the ≤N−1-piece databases. About half of all positions are capture positions.
   3. **DoWinningMoves** on the original, then on the mirror. Type: **retrograde push**, one shot.
      - For every LOSS position, all predecessors become WIN in the other-side array.
      - This resolves 5–10% of the positions in the non-queue passes.
      - The pseudo-code calls `NonCaptureMoves` and `MakeMove`, but the text says reverse moves. Read it as unmoves.
   4. **DoLookups** on the original, then on the mirror. Type: **conversion**, full range over non-capture UNKNOWN positions. Only the conversion moves are evaluated (leading-checker advance or promotion leaving the slice).
      - Writes WIN if some conversion child is a LOSS.
      - Writes DRAW (meaning ≥D) if some conversion child is a draw.
      - Writes LOSS if every move is a conversion and every child is a loss.
      - The case "no moves at all" falls into this branch, so **terminal (blocked) losses are initialised here**.
   5. **DoNoIo**, alternating original and mirror. Type: **forward/pull**, full range over UNKNOWN and ≥D positions.
      - Only non-conversion moves are evaluated (king moves, non-leading checker moves).
      - A value never drops below the bound set by DoLookups. UNKNOWN can become W or L; ≥D can only become W. Every change is queued.
      - Repeat until the number of changes per pass is "small".
   6. On the final mirror pass, generate the queue. **DoQueue**: type **retrograde, frontier as an on-disk list**. For each queued position, generate its unmoves and re-evaluate each predecessor with the DoNoIo logic (a pull at the predecessor). Newly decided positions are enqueued. Repeat until the queue is empty.
      - The text frames this as "forward until resolutions become small, then backward". The best choice depends on the ratio of unresolved to resolved positions (citing Gasser).
   7. Remaining UNKNOWN and ≥D positions become DRAW.
   8. Compress, then **DoVerify** on the compressed slices.
   - **Losing side**: there is **no counter (outcounting)**. A loss is detected by a full successor re-scan, either in a full sweep or for each queued predecessor. This is grandfather-like re-verification.
7. **Parallelism / memory access**:
   - **Independence levels**:
     - (i) Material subsets of the same N are independent.
     - (ii) Nodes at the same level of the king graph are independent, except mirror pairs (e.g. 2222, 3113/1331 and 4004/0440 can run together).
     - (iii) Rank slices along the same anti-diagonal of the 7×7 (Black rank, White rank) grid are independent, computed from (6,6) downward.
     - (iv) DoCaptures can run for any number of slices ahead of time, since it reads only the ≤N−1-piece databases.
   - **Mini-slices**: each slice is cut into about 10 M positions, aligned to 32,768-position pages. Each worker *writes only its own interval* and may read anything.
   - **Locality**: "the choice of the forward approach is critical": it keeps writes in a contiguous range. The backward queue has no locality and was run **sequentially**.
   - **Job control**: four job queues (prime, non-prime, slow, small), crash and reboot detection by ping, automatic restart from checkpoints. Database traffic was about 75% of the department's network traffic.
   - **On the TC2000**: the slice and its mirror sit in shared memory, with barrier synchronisation between iterations.
8. **Draws / cycles**: Draws are whatever remains undecided at the fixpoint, including ≥D bounds. No repetition rule is needed in the construction.
9. **Final storage / compression**:
   - **Capture positions (~50%) and "capture-threat" positions (positions where the side not to move could capture, a further ~25%) are omitted.** In practice their values are overwritten with the slice's dominant value, then the slice is run-length encoded.
   - The probe resolves a capture position by searching one more ply. A threat position needs a more complicated search.
   - Encoding: 5 values per byte (3^5 = 243 codes), with the remaining 13 byte codes used for runs.
   - Results: 3b3w 18 positions/byte, 4b3w 52, 4b4w 22; 150 billion positions in about 5.5 GB.
   - Access: the file is cut into 1 KB blocks. An `.idx` file holds the first position number of each block; the probe binary-searches it, then decodes the block sequentially. A cache of several thousand 1 KB blocks plus a position hash cache gives 80–95% in-memory hits.
   - A predictive evaluation function ("store only exceptions") was tried and lost to RLE.
10. **Verification**: **DoVerify** recomputes every value from its children and compares (a full-range gather). The authors admit it "does not detect isolated cycles". No errors are reported here. (The 2003 paper reports that Dodgen and Trice later found errors in the Chinook 7-piece databases.)
11. **Notable tricks**:
    - **Split I/O from iteration**: DoLookups folds every out-of-slice move into a 2-bit lower bound once, so the iterative phase never touches other databases.
    - ≥D is used as a lattice value.
    - Captures are resolved first and stay fixed afterwards.
    - DoWinningMoves gives a cheap one-shot push.
    - Rank slices give memory reduction and a DAG of parallel jobs.
    - Page-aligned mini-slices make parallel writes safe.
    - The forward sweep is used *because* its writes stay local.
12. **for_each mapping**:
    - **DoCaptures and DoLookups**: gather over the full index range. Each element reads only immutable smaller databases and other slices, and writes only itself. Race-free under `par`.
    - **DoWinningMoves**: scatter over the LOSS set, taken from a bitset or a filtered full range. Every write is the same value (UNKNOWN→WIN) into the *other-side* array. So it is idempotent and monotone: `std::atomic_ref<uint8_t>::store(relaxed)` or a CAS from UNKNOWN is enough, with no double buffer.
      - **[my analysis]** Guard the write: only write if the target is still UNKNOWN and has no capture. A predecessor that has a capture is not a real predecessor, because the capture is mandatory. The paper does not mention this.
    - **DoNoIo**: gather over the full range of side s that reads only side ¬s. Because a move always flips the side to move, alternating s gives **Jacobi double-buffering for free**: race-free and deterministic under `par`.
      - Exception: self-mirror slices (e.g. 2222.66) read and write one array. Use relaxed atomics there. The result stays correct because updates are monotone, but pass counts become nondeterministic.
    - **DoQueue**: as published, a sequential FIFO. Rewrite it as two for_each calls over bitsets:
      - (a) for_each over the frontier: generate unmoves and set bits in a "dirty" bitset. This scatters bits, so it needs `fetch_or` on words or per-thread bitsets that are merged afterwards.
      - (b) for_each over the dirty bitset: re-evaluate each position as a gather. Newly decided positions form the next frontier.
    - **Draw finalisation and DoVerify**: full-range maps; DoVerify uses a reduction to count errors.
    - Nothing here is inherently sequential except RLE compression, which can be parallelised per 1 KB block.

---

## 2003-ACG-10-Schaeffer-Bjornsson-Burch-Lake-Lu-Sutphen.pdf

1. **Citation**: J. Schaeffer, Y. Björnsson, N. Burch, R. Lake, P. Lu, S. Sutphen, "Building the Checkers 10-piece Endgame Databases", *Advances in Computer Games 10*, 2003/2004, pp. 193–210.
2. **Scope**:
   - All 9-piece databases (4,048,627,642,976 positions) plus the 5v5 subset of the 10-piece databases (8,586,481,972,128 positions in 21 king/checker subsets and 630 slices).
   - Total with the earlier databases: 13,144,833,586,271 positions, compressed to about 148 GB.
   - Built November 2001 to June 2003 (18 months), about 15 CPU-years normalised to 1.5 GHz.
   - **Hardware**:
     - Dual-processor AMD machines with 1–4 GB each.
     - Older 800 MHz machines pre-computed the capture phase.
     - Lookups, non-captures and verification ran on about 3 machines.
     - A 64-processor SGI Origin 3000 (500 MHz, 32 GB) handled the non-capture phase of the largest slices.
   - **Largest slice**: 3223.77 with its mirror 2332.77, 85,515,674,400 × 2 = 171,031,348,800 positions. That is about 40 GB at 2 bits per position, and took 2.3 SGI-days. Nine slices exceed 100 billion positions.
   - The 2–8-piece databases total 444 billion positions, built 1989–1993.
3. **Metric**: WDL. 2 bits during construction: W, L, D (meaning "at least a draw"), U.
   - Distance metrics were rejected: a byte per position would mean 13 TB raw.
   - Some slices needed more than 180 iterations, a "lower bound… probably very close" on the ply needed to convert.
4. **Indexing**:
   - **Decomposition**: by number of pieces, then material per side, then kings per side, then **leading rank per side (ranks 1–7)**. Positions with a leading checker on rank R are computed before those with rank R−1.
   - **Slice counts**: 49 per node, but 28 for self-mirror nodes (4411, 3322, 2233, 1144, where (a,b) and (b,a) are paired), 42 = 7×6 where one side has 5 checkers, 7 or 6 for one-sided nodes, and 21 for 0055. Notation: `bkwkbcwc.br.wr`.
   - **64-bit indices** were needed (the largest single database, 2233.77, has 104 billion positions, so 37 bits). Index tables stay 32-bit where possible to save memory.
   - **Further subdivision** (not used here): Dodgen and Trice also slice by the **configuration of checkers on the first rank**.
5. **Working storage**:
   - 2 bits per position; up to 40 GB for one slice.
   - The program does its own paging with 200 MB of page buffers, which gives one disk I/O per 500 probes (a 499/500 hit rate).
   - Captures, lookups and verification need ≥300 GB of local disk and are I/O-bound. The non-capture phase is CPU-bound and has handled 25 GB files "in only a few days".
6. **Passes, in order** (Table 3, with share of wall time):
   1. **Captures**: 15% of wall time; **conversion**, sequential full range, reads the 2…N−1-piece databases. Sets W/L/D for capture positions. Surprisingly, it resolves more than 60% of positions. It can run far ahead of the other phases.
   2. **Lookups**: 24%; **conversion**, sequential full range. Evaluates all moves *by the leading checker* (into already computed slices). Writes W, L (only if every legal move is a leading-checker move and all lose) or ≥D. Resolves 10–15% of positions; about 3 lookups per position.
   3. **Non-captures**: 20%; **forward/pull**, repeated sweeps over U and ≥D positions, considering only king moves and non-leading checker moves. Resolves W/L only. When changes become "small", it switches to a **queue** of changed positions: only their predecessors are re-examined. When no more changes occur, every U and ≥D position is a draw. Runs on POSIX threads with the index range "equally divided between the processors".
   4. **Compress**: 1%.
   5. **Verify**: 40%; full range, on the *compressed* data, checks consistency within the slice and against earlier databases.
   - **Losing side**: a full successor re-scan; no counters.
7. **Parallelism / memory**:
   - Threads split the index range.
   - Many slices run concurrently, scheduled by the TrellisDAG tool. Data-file management stayed manual and error-prone.
   - **Data placement**: slices that lead into one another sit close together in the file.
   - **Use of compressed data**: lookups and verification read the compressed databases, which forces small searches for omitted capture and threat positions. The authors chose this deliberately: they refused to trust unverified uncompressed data.
   - **Suggestion**: do not slice small databases (e.g. 1045, 45 billion positions in 42 slices). Compute them whole and do lookups only for leading checkers on rank 7.
8. **Draws**: U and ≥D positions left at the fixpoint are draws. The authors warn that WDL alone may let a program "cycle around" in won 10-piece positions without making progress.
9. **Final storage / compression**:
   - Capture *and threatened-capture* positions are removed; removing threats improves compression about 4×. Their slots get "the W/L/D value that continues the current run".
   - The data becomes (value, run-length) pairs. The next run's value is predicted to equal the value of the run before the previous one (correct about 95% of the time), so each run is encoded as a length, plus a "miss" symbol when the prediction fails.
   - The lengths use a length-limited Huffman code: 20-bit maximum code length, a 1,048,576-entry decode table, maximum run about 10,000 with an escape to a 32-bit length. One fixed code is shared by all files.
   - 3 bits of header seed the first two predictions.
   - **Results**: 2–8 pieces shrink from 5.6 GB to 2.7 GB (155 positions/byte); 9 pieces 16.8 GB (227/byte); 10-piece 5v5 125 GB (65/byte); old 4v4 22/byte.
   - Capture positions get I/O at most once (in the Captures phase) and are never verified, because they are not stored.
10. **Verification**:
    - The fast self-consistency scan was replaced by full verification against the compressed data and the earlier databases.
    - **Errors found**: Dodgen and Trice's independent 8-piece computation (October 2001) exposed **a few thousand wrong positions in Chinook's 7-piece databases, even though they had passed every Chinook verification test**: "internally consistent, but wrong". Independent recomputation is the only strong check.
    - **Hardware faults**: a faulty CPU, bad memory, a disk crash, network errors and operator errors.
    - **Mitigations**: every computation logged; a CRC on every network copy (failing about once a month); a 32-bit CRC per 1,024-byte block, checked on every read (this found a program bug and silent read failures); everything archived on tape (restored once).
    - Some slices still failed verification with no logged error.
11. **Notable tricks**:
    - Captures run far ahead of the other phases.
    - Lookups-then-pure-CPU iteration, with D used as a ≥D bound.
    - Self-mirror slice pairing.
    - Huffman run lengths with value prediction.
    - Per-block CRCs.
    - Verify against the compressed form.
    - Consider whole-node builds for small nodes.
12. **for_each mapping**: The same as 1994.
    - Captures and Lookups: gathers over the full range.
    - Non-captures: gather over the full range. Race-free if it alternates sides; with in-place threads it is still correct but nondeterministic, which is exactly why the authors "cannot know the exact number of ply".
    - Queue phase: a frontier bitset, scatter-mark predecessors with atomic OR, then gather over the marked set.
    - Verify: a full-range gather plus a reduction.
    - Compression: sequential within a block, because of run continuation and prediction state; parallel across blocks.

---

## 2003-ACG-10-Trice-Dodgen.pdf

1. **Citation**: E. Trice, G. Dodgen, "The 7-Piece Perfect Play Lookup Database for the Game of Checkers", *Advances in Computer Games 10*, 2003/2004, pp. 211–230.
2. **Scope**: 2- to 7-piece "Perfect Play Lookup" (PPL) databases. Table 2 lists every slice with ≤4 vs ≤3 pieces: position counts, longest win and loss, and a sample position.
   - Longest win: **253 ply** (1K+3C vs 2K+1C, Black to move with 8-11).
   - The largest listed slice is 2K+2C vs 2K+1C, with 1,415,939,616 positions.
   - Hardware and build time: not discussed (they mention "personal computer resources").
3. **Metric**: Three database types are compared.
   - **GTV** (WDL): 2 bits during construction (4 positions/byte), 5 positions/byte after.
   - **DTC** (distance to conversion, i.e. plies to a crowning or a capture): one byte per position.
     - Combined with WDL: 2 bits WDL + 6 bits depth.
     - **Parity trick**: store ⌊depth/2⌋ and reconstruct depth as 2v+1 for a win and 2v for a loss. This doubles the range to 127 plies in 6 bits, or 511 plies in 8 bits when WDL comes from a separate GTV database.
     - Draw positions can be dropped from a DTC database.
   - **PPL = MTW** (moves to win, a DTM-like metric across conversions): one byte per position.
   - The paper calls the conversion metric "DTC". "MTC" appears only in the cited Cash/Miller and Gilbert material.
4. **Indexing**: They credit Chinook's indexing function ("applying an indexing function to create a sparsely populated matrix") and built their own GTV databases with it. Slices are listed by material (kings and checkers per side). The first-rank checker configuration subdivision is mentioned only in the Schaeffer 2003 paper. No details here.
5. **Working storage**: A byte per position for DTC and PPL; "much more RAM and disk… compared to a GTV database". PPL compresses worse than GTV. Otherwise not discussed.
6. **Passes**:
   - **DTC**: "nearly identical" to GTV.
     - (1) **Jump pass** (conversion): capture wins get depth 1.
     - (2) **Crowning moves** (conversion): stored as "conversion in 1 ply" whatever the result.
     - (3) Repeated **forward/pull passes**: when a W/L is determined, the *iteration number* is stored.
     - Conversion depth is **not inherited** across slices; each slice is computed independently.
   - **PPL**: built bottom-up from the 2-piece database.
     - Repeated **full passes** that back up MTW: a win takes min(MTW over losing children) + 1, a loss takes max(MTW over winning children) + 1.
     - Values can change from one iteration to the next ("a complex sorting procedure"). The procedure only stops after "a pass over the entire database that produces no changes".
     - Wins by blocking (the opponent has no move), with or without conversion, must be handled.
     - It requires a correct GTV database first.
   - **Losing side**: full successor re-scan; no counters.
7. **Parallelism**: not discussed.
8. **Draws / cycles**: Draws come from the GTV database. Draws can be omitted from DTC and PPL storage. The paper shows how GTV-only programs (WYLLIE, KINGSROW, at depths of 31–63 ply) fail to win 253-ply positions, through hash-table repetition saturation and 66–68-ply cycles. There is no draw-rule handling in the databases.
9. **Final storage**: PPL post-compression is "not nearly as good as GTV"; no scheme is given.
10. **Verification**: "The verification routine [of PPL] virtually guarantees that the GTV database used in the process is correct."
    - Their independent 8-piece computation found the Chinook 7-piece error (reported in the Schaeffer 2003 paper).
    - They also corrected Payne's 1756 analysis of Fourth Position (81 ply instead of 85, plus a sub-optimal defence costing 12 ply).
11. **Notable tricks**:
    - Win/loss parity halving for distance bytes.
    - DTC as a by-product of pass numbers.
    - DTC's weakness: it prefers a quick conversion over a quicker win (Fig. 4).
    - Using the PPL build as a cross-check of the WDL database.
12. **for_each mapping**:
    - DTC passes: full-range gathers.
      - **[my analysis]** An exact DTC from "iteration number" requires Jacobi semantics. Either alternate sides (reading only the other side's array), or stamp each value with the pass that decided it and treat values decided in the current pass as unknown.
    - PPL relaxation: a full-range gather iterated to a fixpoint. It is not monotone as described ("values subject to change"), so in-place parallel updates give no determinism guarantee.
      - **[my analysis]** Prefer the standard level-synchronous DTM: pass n decides exactly the depth-n positions. Its frontier is a bitset, and each pass is a gather (pull) or a scatter of decrements (counters).

---

## 2005-ACG-11-Bjornsson-Schaeffer-Sturtevant.pdf

1. **Citation**: Y. Björnsson, J. Schaeffer, N. R. Sturtevant, "Partial Information Endgame Databases", *Advances in Computer Games 11*, LNCS 4250, 2006, pp. 11–22. (The 2007 ICGA paper cites it as "Imperfect Information Endgame Databases".)
2. **Scope**: The motivation is to build only the useful slices of the 11-piece databases.
   - The full 11-piece set is 259 trillion positions, of which 6v5 is 118 trillion. But 6 checkers vs 5 checkers alone is only 25 billion, and 6v5 with at most 1 king is 300 billion.
   - About 20% of non-database positions in the prover are 11-piece positions; more than 90% of those are 6v5 with 0–1 kings.
   - **Validation**: the 5-checkers vs 5-checkers 10-piece database was rebuilt from only the 2–9-piece databases, and the 9-piece 4v5 and 5v4 checker databases were also built. Hardware and time: not discussed.
3. **Metric**: Bounded WDL.
   - Final values: WW, TT, LL (exact), plus LT, LW, TW (lower/upper bounds).
   - Values used during construction: uLW, uTW, uTT, uLT. These "would become" the corresponding bound if the analysis stopped now. The initial value is uTT (the old "unknown", which decides to a draw).
   - That makes 10 states. **[my analysis]** 4 bits per position; the paper gives no encoding.
4. **Indexing**: Chinook's slicing by leading rank (results reported per Black/White leading-rank pair). Positions whose children lie in uncomputed databases (e.g. a second king appears) get **LW**.
5. **Working storage**: not discussed beyond the value set.
6. **Passes**: Generic retrograde pseudo-code (Table 1). Repeat full sweeps; for each position that is not completed, set value = max(negated children values) and count changes, until no change. Then apply `decide` to every position.
   - **max is a lattice join** on the partial order of Fig. 2. Lower bound = max of lower bounds, upper bound = max of upper bounds, e.g. max(TT, LW) = TW. The full 10×10 table is given (Table 2).
   - **Conversion**: lookups into computed databases, with LW for children in uncomputed databases. The classes are unchanged: init and conversion, then **forward/pull** sweeps to a fixpoint. The 5v5 build took up to 36 passes.
   - Losing side: full re-scan.
7. **Parallelism**: not discussed; implemented inside the Chinook builder.
8. **Draws / cycles**: The key subtlety is a repetition cycle next to an unknown escape.
   - A cycle with an LW exit must end as TW/LT around the cycle, not TT.
   - The uX values carry this information. A sketch of a correctness proof is given: bounds stay sound, and a uTT can only come from a repetition.
9. **Final storage**: not discussed.
10. **Verification**: Every 5v5 bound was checked against the true 10-piece values; all were consistent.
    - Resolution rates (win/loss/draw counts 1, a ≥ or ≤ bound counts ½): 16% at leading ranks 7/7, rising to 100% at 2/2.
    - The 9-piece tables show a strong asymmetry, e.g. the side with 4 to move at 7/6 resolves 10%, while the side with 5 to move resolves 66%.
11. **Notable tricks**:
    - The bound lattice lets a slice be built **without its prerequisites**.
    - The algorithm is the standard one with only the operators `max`, `completed` and `decide` redefined.
    - Proposed: a small (about 5-ply) search at boundary positions to tighten bounds.
12. **for_each mapping**:
    - Each sweep: a full-range gather of a lattice join; race-free with a btm/wtm split.
    - The join is monotone in the information order. **[my analysis]** So in-place relaxed-atomic updates still converge, just nondeterministically.
    - The final `decide` pass is an elementwise map.
    - The passes need frontier tracking just like plain WDL; one dirty bitset per pass.

---

## 1996-GoNC-Schaeffer-Lake.pdf

1. **Citation**: J. Schaeffer, R. Lake, "Solving the Game of Checkers", *Games of No Chance*, MSRI Publications 29, 1996, pp. 119–133.
2. **Databases**: All positions with ≤8 pieces (443,748,401,247) are solved, built by the method of Lake et al. 1994. The 9-piece build had started, with "only a small percent of the 10^12 positions" expected in 1995.
3. **State-space count**: Num(b,B,w,W,f) = C(4,f)·C(24,b−f)·C(28−(b−f),w)·C(32−b−w,B)·C(32−b−w−B,W). Here f is the number of Black checkers on Black's first rank (squares no White checker can use), and 28 is the number of legal checker squares. Summing gives 5×10^20 positions in total. A heuristic "reasonable" subset (material difference <3, a cap on kings) is just under 10^18.
4. **Interface with search**:
   - The databases are a leaf oracle for Chinook's alpha-beta search.
   - A "middlegame database" stores solved positions with more than 8 pieces, found by deep searches that reach database positions (e.g. a 22-piece win).
   - The proof plan has three prongs: bottom-up databases, a middle set of solved positions, and top-down lines seeded from the 144 three-move ballots.
   - Alpha-beta is used as a prover with win = +∞, draw = 0 and loss/unproven = −∞. Proof-number search (PNS) is compared; alpha-beta usually wins.
5. **Draw differentiation**: drawn positions are scored within ±1/3 of a checker.
6. **Verification / build method**: not discussed beyond the citation.
7. **for_each note**: The Num() formula is a ready-made slice-size formula. It is an alternative to leading-rank slicing: slicing on f (first-rank checkers) is the subdivision Dodgen and Trice used.

---

## 2005-IJCAI-Schaeffer-Bjornsson-Burch-Kishimoto-Muller-Lake-Lu-Sutphen.pdf

1. **Citation**: J. Schaeffer et al., "Solving Checkers", *IJCAI 2005*, pp. 292–297. Many exponents are garbled in the extraction.
2. **Databases**: All databases with ≤10 pieces, any mix of kings and checkers: 39 trillion positions (3.9×10^13; the extraction is garbled, value taken from the 2007 papers). Built by the method of Schaeffer et al. 2003; WDL only.
   - Difficulty evidence: Trice–Dodgen's 253-ply 7-piece win; more than 180 ply to conversion in the Chinook builds; a 279-ply win in 5K vs 3K+2C (Gilbert).
3. **Interface with search**:
   - A front-end proof-tree manager (PNS with heuristic proof numbers, iterating on a heuristic threshold: MIN = 125, INC = 5; "likely win/loss") farms positions to back-end solvers.
   - Back end: Chinook alpha-beta for 25 s (17–23 ply), then Df-pn for 100 s, each answering "is it a win / at most a draw" and "is it a loss / at least a draw".
   - Database probes dominate the run time. With a 4 GB machine (most of it disk cache), 99% of database probes need no I/O. CPU use is rarely above 10%.
   - Only local disk (300 GB) is usable; the programs detect and recover from I/O errors.
   - GHI is handled by Kishimoto–Müller (2004). No move history is passed across the front/back boundary.
4. **Verification**: Ed Gilbert (KINGSROW) independently verified the 9-piece databases and 30% of the 10-piece databases (ongoing).
   - Back-end results were partly re-verified with a different solver. The front-end tree is consistent.
   - The proof agreed with all 428 positions in Fortman's *Basic Checkers*.
5. **Results**: White Doctor = draw. 841,810 back-end positions; minimal proof tree 223,178 positions; longest front-end line 67 ply; about 7 processors on average.
6. **Plans**: "A new algorithm will allow us to compute parts of the 11-piece database, getting 50% of the benefits for 1% of the computing effort" (the partial-information databases).

---

## 2007-ICGA-Schaeffer.pdf

1. **Citation**: J. Schaeffer, "Game Over: Black to Play and Draw in Checkers", *ICGA Journal* 30(4), December 2007, pp. 187–197.
2. **Database chronology**:
   - 1989: 4-piece databases (7 M positions). Ken Thompson computed the 5-piece databases (149 M) first.
   - 1990: 6-piece (2.5 billion). 1991: 7-piece (35 billion). 1993: 4v4 (111 billion). 1996: all 8-piece (406 billion).
   - **1997**: the 9-piece build was stopped. It hit 32-bit limits: "the code was not working".
   - **2001**: restart with 64-bit tools.
   - **2002**: the 9-piece build was aborted because "a special case in the code was not being properly handled", then restarted from scratch and finished that year (4 trillion).
   - 2004: 5v5 (8.5 trillion). 2005: the full 10-piece set (6v4, 7v3, 8v2, 9v1): 39,271,258,813,439 positions in total.
3. **Build and size**: Retrograde analysis per Schaeffer et al. 2003, split into thousands of sub-computations. Mostly 10–20 workstations; the larger subsets on a 64-processor SGI with **64 GB** (the 2003 paper says 32 GB).
   - Compressed to about 250 GB.
   - A compression scheme 30% better exists but is not used, because decompression is too slow for real-time search.
   - WDL only. The longest known 10-piece win is 279 ply.
4. **Partial 11-piece databases**: Built with "fringe" positions (those entering ≥2 kings) set to unknown. Search trees often shrank about 2×, but the extra 10 GB destroyed I/O locality, cancelling the gain. **They are not used in the final solver.**
5. **Verification**: Gilbert's independent build differed from Chinook by **one win** in one subset (January 2005). Chinook was correct; Gilbert's file had been corrupted by an unverified copy.
6. **Search interface**: Chinook alpha-beta for 15 s (finds proven wins and losses, not draws), then Df-pn for 100 s (proven, bounded or unknown).
   - **Proof-number trick**: the solver returns a warning flag when the end of its principal variation contains a king; the manager raises the proof number for that line.
   - Branching factor is 1.20 in capture positions and 7.94 otherwise.
7. **Results**: 19 openings were solved (Table 2). The deepest minimal-tree line is 94 ply.

---

## 2007-Science-Schaeffer.pdf

1. **Citation**: J. Schaeffer, N. Burch, Y. Björnsson, A. Kishimoto, M. Müller, R. Lake, P. Lu, S. Sutphen, "Checkers Is Solved", *Science* 317 (2007), pp. 1518–1522 (Science Express version, 19 July 2007). The Supporting Online Material is not in the PDF.
2. **Databases**:
   - **Size**: all positions with ≤10 pieces, 39,271,258,813,439 in total.
   - **Compression**: 237 GB, i.e. 154 positions per byte, with "rapid localized real-time decompression".
   - **History**: the 8-piece databases took 1989–1996; in 2001 rebuilding them took one month. The 10-piece set finished in 2005.
   - **Construction**: described only at the level of retrograde analysis by number of pieces, with draws by repetition. The details are in the 2003 paper.
   - **Parallelism**: in 1992 more than 200 processors were in use at peak.
3. **Interface with the proof search**:
   - Back-end solvers do alpha-beta (15 s nominal) and then Df-pn (100 s). They return proven, partially proven or heuristic values.
   - Only the top of the proof tree is stored, about 10^7 positions. When a user queries a line past that point, the solver re-searches into the databases (about 2 minutes per query).
   - The forward effort is about 10^14 positions.
4. **Verification**: "verifying all computation results and doing consistency checks", plus independent verification of parts of the work (details in the SOM, which is not available here).
   - Probabilistic argument: an error 40 ply deep is unlikely to propagate to the root.
5. **Results**: draw. 19 openings; 15,123,711 solver searches; minimal tree 3,301,807 positions; longest line 154 ply.

---

## Group synthesis

### Distinct primitives introduced by this group

1. **Capture-first conversion pass (DoCaptures)**: Mandatory capture means every position with a capture is resolved once, as the maximum over children in the ≤N−1-piece databases, and stays fixed. About 50% of positions, rising to 60% at 10 pieces. *Full-range gather; read-only inputs; runs ahead of everything else.*
2. **Multi-level slicing**:
   - **Levels**: piece count, then material, then kings per side (the promotion DAG), then **leading-checker rank per side** (7×7 = 49 slices). Slices are computed from rank (6,6) downward. Each slice is paired with its colour-reversed mirror (btm/wtm), and self-mirror slices are paired with themselves.
   - **Parallel schedule**: same-level nodes and anti-diagonals of the rank grid run in parallel.
   - **Alternatives**: first-rank checker configuration (Dodgen–Trice, and the f term in the 1996 count), and building small nodes whole (a 2003 suggestion).
3. **Lookups pass with a ≥D lattice value**: All out-of-slice non-capture moves (leading-checker advances and promotions) are folded once into a 2-bit lower bound {L, U, ≥D, W}, so the iterative phase never does external I/O. *Full-range gather.*
4. **Pull sweeps alternating btm/wtm to a fixpoint (DoNoIo / Non-captures)**: Only within-slice moves are evaluated. Values are monotone (U→W/L, ≥D→W). This is a natural Jacobi double buffer because each pass writes one side's array and reads only the other's. In-place threads make pass counts, and thus derived DTC, lower bounds that vary from run to run (1992 and 2003).
5. **Hybrid switch to a backward frontier (DoQueue)**: Once changes are "small", only predecessors of the positions just decided are re-evaluated by a full pull (re-scan, **no counters**). The frontier is a 4-byte-per-entry queue on disk, run sequentially because it has no locality. For the user this becomes a frontier bitset, then an atomic-OR "dirty" bitset of predecessors, then a gather over the dirty set.
6. **One-shot loss push (DoWinningMoves)**: Every predecessor of a LOSS becomes WIN. This is an idempotent scatter of a single value, so it is safe with relaxed atomic stores. Add a guard for "target UNKNOWN and has no capture".
7. **Verification**: three increasing levels.
   - Self-consistency gather (1994; cannot detect isolated cycles).
   - Verification of the compressed form against all earlier databases (2003, 40% of build time).
   - **Independent recomputation** (Dodgen–Trice found Chinook 7-piece errors that passed every internal check; Gilbert's cross-check found a corrupted copy).
   - System-level: per-1 KB-block CRCs, CRC on every copy, tape archives.
8. **Storage that omits capture and capture-threat positions**: Their slots are refilled with the dominant value (1994) or the run-continuing value (2003), then RLE is applied: 5 values per byte plus 13 run codes (1994), then predicted-value Huffman run lengths (2003). Fixed 1 KB blocks with an index of starting positions give random access. Probes resolve omitted positions by a small search.
9. **Distance metrics in one byte (Trice–Dodgen)**:
   - DTC = the pass number at which a position is decided, not inherited across slices.
   - PPL/MTW = min/max relaxation to a fixpoint on top of a correct WDL database.
   - Win/loss parity halving extends the range to 511 plies.
10. **Partial-information lattice (Björnsson et al.)**: Bounds {LL, TT, WW, LT, LW, TW} plus construction-time {uLW, uTW, uTT, uLT}, with max as a lattice join and LW for children in uncomputed databases. It is the same pull fixpoint with redefined operators, and handles repetition cycles that have unknown exits.
11. **Database/search interface**: The databases are a leaf oracle; capture positions are resolved by one extra ply; caching achieves ≥99% hits. The I/O locality of the databases matters more than their coverage (the 11-piece partial databases were dropped because of I/O). GHI handling is needed for repetition draws in forward proofs.

### Lineage

- Thompson 1986 (chess, cited) and Gasser (forward vs backward choice, cited) lead to the **Lake–Schaeffer–Lu 1994** pass structure: Captures, WinningMoves, Lookups, NoIo and the queue, plus rank slicing, mini-slices and RLE without captures.
  - The 1992 ACF bulletin is an output of that builder. Its "lower-bound conversion" figures are an artefact of in-place sweeps.
- **Schaeffer et al. 2003**: the same five phases scaled to 10 pieces.
  - 64-bit indices.
  - Lookups restricted to leading-checker moves.
  - Huffman/prediction compression.
  - Verification against compressed data.
  - Shared-memory threads over index ranges.
- **Trice–Dodgen 2003**: an independent line built on Chinook's indexing. It added distance metrics (DTC, MTW) and the cross-check that exposed Chinook's 7-piece errors. That cross-check motivated the 2003 verification emphasis and Gilbert's later re-verification.
- **Björnsson–Schaeffer–Sturtevant 2005**: Chinook's builder with a bound lattice, so slices can be built out of order. Partial 11-piece databases were then built but not used in the final proof (2007 ICGA).
- **1996, 2005 and 2007 proof papers**: consume the ≤10-piece WDL databases as the backward half of a hybrid proof. They add no new construction methods.

### Implications for a single-`for_each`-per-pass design (draughts)

- **Gathers over the full range**: Captures, Lookups, NoIo sweeps, Draw finalisation and Verify. The btm/wtm split makes NoIo Jacobi automatically.
- **Scatters**: WinningMoves (same value written each time, so relaxed atomics suffice) and frontier→predecessor marking (atomic OR on bitset words).
- **Counter decrements** (the user's current outcounting design): an atomic `fetch_sub` scatter over a frontier bitset. None of these papers uses counters; they pay for the loss test by re-scanning instead.
- **Not one for_each**:
  - The sequential FIFO DoQueue as published (rewrite it as frontier bitsets).
  - Run-continuation compression (parallel only per block).
  - Trice–Dodgen's non-monotone MTW relaxation (replace it with level-synchronous DTM).
