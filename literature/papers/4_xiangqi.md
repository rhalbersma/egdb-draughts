# Group 4: Xiangqi (Chinese chess) EGDB papers

Notation used below: WTM/BTM (RTM) = White/Black (Red) to move; "frontier" = set of positions decided in the latest layer; "gather" = each element reads others and writes only itself (race-free under `std::execution::par`); "scatter" = writes into other elements (needs atomics, a double buffer, or sort-and-combine).

How well each file was read:
- 1996-QM, 2001-IS, 2001-IEEE, 2002-MGoNC, 2002-CG-03, 2004-CG and 2004-slides came back as clean text.
- **1996-QM**: the pseudocode figures (Thompson's P1–P4, the old Wu-Beal algorithm, and the one-bitmap algorithm) are OCR garbage. Only the prose and the access-pattern analysis can be read. The same algorithm appears cleanly in 2002-MGoNC.
- **2001-IS**: Fig. 1 (the pseudocode) is an image and came back with no text. The prose description and the access summary did come through, and they match the MGoNC pseudocode.
- **2001-IEEE**: Table 1 pseudocode is badly garbled but readable when compared with MGoNC.
- **2000-CG-02 (Fang-Hsu-Hsu)**: the text layer has broken font encoding. Many first letters are missing, symbols are replaced by junk, and the tables are scrambled. The algorithm prose can be reconstructed. The exact sentinel byte values in Table 1 and the hardware/timing data could **not** be recovered.

---

## 1996-QM-Wu-Beal.pdf

1. **Citation**: Ren Wu, Don Beal. *Retrograde Analysis of some Chinese Chess Endgames.* Technical Report No. 731, Dept. of Computer Science, Queen Mary and Westfield College, University of London, December 1996 (ISSN 1369-1961).

2. **Scope**: Chinese chess, with a claimed applicability to chess ("5-men pawnless chess in less than a day on a 100 MHz Pentium", "makes 6-men feasible on a workstation"). Two 9-piece endgames are reported:
   - KAGP:KAAEE (code 100011-220000): 2,455,818,750 "all" configurations; database 152,806,500 entries; max DTC 32; build time 19:30:42 for DTC and 28:13:30 for DTM.
   - KEGP:KAAEE (010011-220000): 3,438,146,250 configurations; 232,848,000 entries; max DTC 26; build time 30:32:34 for DTC and 56:10:19 for DTM.
   - Each endgame has 71 subgames, all generated with no human intervention.
   - Hardware: Pentium 133, 32 MB RAM, Linux 1.2.13. The build times include a consistency check. The program is more than 13,000 lines of C, mostly the Gödel (index) coding.

3. **Metric**: Full information for both sides: win/draw/loss plus depth. Either DTC or DTM, chosen with a switch; they note DTC vs DTM "makes little difference" in cost. There are two databases, `wdb` (WTM) and `bdb` (BTM). Each entry holds a value code: illegal, unknown, win_in_n or lose_in_n. The bit width is not given; it is implicitly one byte, since they compare against "a byte per position".

4. **Indexing**: The Gödel number covers per-piece legal squares. "All" counts include illegal (overlapping) slots; "real" counts are the legal ones. Symmetry reduction and grouping are deferred to Wu's thesis. Illegal slots are marked *illegal* in DoInitialize.

5. **Working storage during construction**:
   - Both databases stay on disk and are only streamed.
   - RAM holds **one random-access bitmap** (1 bit/position). For full 5-man pawnless chess that is 120/8 = 15 MB.
   - Other bitmaps are streamed sequentially from disk.
   - Comparisons in the paper:
     - Thompson 1986 needs two randomly accessed bitmaps, about 1/4 of the database size (30 MB for 120 MB).
     - The authors' own earlier 1992 algorithm needed one database plus one bitmap randomly accessed, 9 bits/position (135 MB).

6. **Passes, in order** (from the prose, because the pseudocode is garbled):
   - **The authors' earlier algorithm, in three passes**:
     - *DoMaylose*: from new wins, generate predecessors and set a `maylose` bit (retrograde push).
     - *DoLose*: for each `maylose` bit, generate successors and check that all are WIN (grandfather verification).
     - *DoWin*: from new losses, generate predecessors and mark them win_in_n+1 (push).
   - **The one-bitmap algorithm**:
     - *DoInitialize* (init): one sequential scan. It marks illegal positions. Positions that are "actually in a subgame" get the subgame's value, as exact DTM or as win_in_0/lose_in_0 under DTC. Positions where one side can capture the opposing king become win_in_0.
     - *DoWinInOne* and *DoWinInN*: per depth n and per side, a fixed sequence of bitmap operations:
       - `GetBits(db, value)` extracts a bitmap by streaming the database.
       - `DoMayLose` is a retrograde push from the `wins` bitmap into a random `maylose` bitmap.
       - `AndBits` intersects that with *unknown*.
       - `DoLose` takes each bit of `maylose`, generates all successors, and checks them against the random `wins` bitmap (**grandfather verification**).
       - `AddBits` streams `lose_in_n` into the database.
       - `DoMayWin` is a push from the new losses into a random `maywin` bitmap, which is then ANDed with *unknown*, and `AddBits` writes win_in_n+1.
     - This is done for White and then Black at every n, until both `w_win[n]` and `b_win[n]` are empty.
     - The losing side is handled by **verification**, restricted to the frontier, i.e. only the predecessors of new wins. There are no counters.
     - Frontiers are bitmaps scanned sequentially.

7. **Parallelism / memory access**: No parallelism. Every pass is analysed by access pattern:
   - The streams (databases, `s_bits`) are sequential.
   - Only one bitmap (`r_bits`) is randomly accessed at any time.
   - Everything other than move generation is "simple load and Boolean operations over the files".
   - There is no sorting.

8. **Draws, cycles and special rules**: Positions that are never decided are draws. Repetition rules are not discussed.

9. **Final storage / compression**: The full database files (one per side) are stored on disk. Compression is not discussed.

10. **Verification**: A "consistency check" is included in the build times. The method is not described.

11. **Notable tricks**:
    - Express the whole construction as bitmap algebra over streamed files, with exactly one random-access bitmap.
    - Both sides' full results come out of one integrated loop.
    - The DTM/DTC switch costs almost nothing.
    - Subgame values are injected at init into slots of the main index, so the ply loop never probes other files.

12. **Mapping to a single for_each per pass**:
    - `GetBits`, `AndBits`, `AddBits`, `ClearBits`: elementwise maps over the full range (or per 64-bit word). Gather, `par_unseq`-safe.
    - `DoMayLose` and `DoMayWin`: a map over a frontier bitset that **scatters** bits into predecessors. Needs `atomic_ref<uint64_t>::fetch_or`. The operation is idempotent, so relaxed ordering is enough.
    - `DoLose`: a map over the candidate bitset, gathering successor values. It writes only its own bit, so to be race-free iterate over 64-bit *words* rather than over bits.

---

## 2001-IS-Wu-Beal.pdf

1. **Citation**: R. Wu, D.F. Beal. *Solving Chinese chess endgames by database construction.* Information Sciences 135 (2001) 207–228, Elsevier.

2. **Scope**:
   - Chinese chess endgame **aegp-aaee** (K+A+E+G+P vs K+2A+2E): 1,004,157,000 entries per side, with 143 subgames (144 including itself, about 7 billion entries in total).
   - The algorithm is presented as generic: "chess, Chinese chess, checkers".
   - Hardware: Pentium Pro 200, 128 MB, Windows NT 4.0. About **92 h** for aegp-aaee with both sides' full DTC.
   - For comparison, Nalimov's KQQKQQ (1.2 G entries) took 2 days on a 500 MHz server with 2 GB. Wu-Beal claim about 128 MB would suffice for that endgame with their algorithm.
   - Results: max DTC **95** (35 more than the 60-move limit). RTM: 70.52% win, 29.48% draw. BTM: 49.49% loss. Over real positions there are 5,211,916,080 per side.

3. **Metric**: A pair of databases W and B with full W/D/L and exact depth. DTC was used for aegp-aaee; DTM is "controlled by a boolean variable". Init values are illegal, unknown, win_in_0, lose_in_0 (DTC) or win_in_n, lose_in_n (DTM). Footnote 3: under DTM the main database must contain the subgame positions, and only those get values at init. The bit encoding is not given.

4. **Indexing** (section 3, quoted numbers):
   - The naive size 90^10 = 3.49×10^19 drops to 17,190,731,250 by restricting each piece to its legal squares: K 9, A 5, E 7, H/C/G 90, P 55.
   - Single-piece mirror on the "first piece" (G: 90→50) gives 9,550,406,250.
   - Identical pieces as combinations, C(5,2) = 10 and C(7,2) = 21, gives 2,946,982,500. With the mirror as well: 1,637,212,500.
   - **Piece grouping**: enumerate the joint placements of king plus palace pieces. kaa has 70 patterns (vs 90), or 38 mirror-reduced. The best group is **kae**, with 275 patterns, 138 mirror-reduced (49.82% saving, close to 50%).
   - Final index: 138 × 90 × 55 × 70 × 21 = **1,004,157,000**.
   - Table 3 lists group sizes. For example hh/cc/gg 4005→2045; rp3 26235→13135.
   - Illegal slots remain: 24.34% for RTM and 26.44% for BTM.

5. **Working storage during construction**: 1 bit/position of RAM ("one random-access bitmap"). The databases W and B and the bitmap S are streamed sequentially.

6. **Passes, in order** (from the prose; Fig. 1 not extractable):
   - **init**: DoInitialize scans everything, assigns illegal/unknown/terminal values, and injects the subgame values.
   - For each n, for each side:
     - Load win_in_n into S.
     - **Predecessor(S)**, a push, gives R.
     - S = unknown ∧ R gives the *maylose* set.
     - **ProveSuccessor**: generate all successors and require each to be in R = win_in_≤n. This is **grandfather verification**, restricted to the frontier.
     - Add the survivors as lose_in_n.
     - Predecessor of lose_in_n, ∧ unknown, gives win_in_n+1 (push), which is added.
     - Then the same for the other side. Repeat until nothing is added.
   - An advantage over running Thompson's algorithm twice: "already-known wins need not be tested to see if they are losses", and no post-processing is needed.
   - Access summary:
     - DoInitialize: sequential on W, B and S; random on R.
     - Load and Add: sequential.
     - Predecessor and ProveSuccessor: sequential on S, random on R.

7. **Parallelism / memory access**: Sequential streams plus one random bitmap. No parallelism.

8. **Draws, cycles and special rules**: Unresolved positions are draws. Repetition rules are not discussed. The 60-move limit is noted only to comment on DTC 95.

9. **Final storage / compression**: Pair of full databases. Compression not discussed.

10. **Verification**: Not discussed in this paper.

11. **Notable tricks**:
    - Piece-grouped joint enumeration with mirror applied to the group that gives the largest saving (kae).
    - Mirror applied only to one "first piece" to keep reflection cheap during move generation.

12. **Mapping to a single for_each per pass**: Same as the 1996 entry and the MGoNC entry.
    - Load/And/Add: gather maps over the full range.
    - Predecessor: scatter from a frontier bitset (atomic OR).
    - ProveSuccessor: gather over a candidate bitset, iterated per word.

---

## 2001-IEEE-Wu-Beal.pdf

1. **Citation**: Ren Wu (HP Labs), Don F. Beal (QMW). *Parallel Retrograde Analysis on Different Architecture.* Proc. 10th IEEE International Symposium on High Performance Distributed Computing (HPDC-10), 2001, pp. 356–362.

2. **Scope**:
   - A survey and model paper. Previous work covered: Nine Men's Morris (about 10^10 states), checkers (8-piece, 4.4×10^11), awari (stones < 35, 5.2×10^10), chess (5- and 6-piece), Chinese chess.
   - Example: aegp-aaee, about 6.8 billion states (3.4 billion per side). It took 92 h on a Pentium Pro 200 with 128 MB.

3. **Metric**: Same as the Wu-Beal algorithm, W/L/D plus depth for both sides.

4. **Indexing**: Defers to [17–19].

5. **Working storage during construction**:
   - 1 bit of RAM per state. Footnote: "actually only 1 bit for **two** states, one per side", because only one side is processed at a time.
   - Once the problem is decomposed into small subproblems, the spare memory "can be used to keep extra bitmaps", which reduces the number of sequential database scans.

6. **Passes, in order**: Table 1 gives the same loop as MGoNC: Load, Predecessor, &unknown, ProveSuccessor, Add, then Predecessor, &unknown, Add, for each side. **Cost analysis**:
   - At iteration i, Predecessor is called Ww_i + Bl_i times and ProveSuccessor at most b·Ww_i times.
   - Summed over everything: (b+1)·G generator calls, i.e. **O(bG)**, where b is the branching factor and G the number of states for both sides.
   - Verification for losses is worst-case b candidates per new win.

7. **Parallelism / memory access** (the core of the paper):
   - **Decomposition** by irreversible operators is "the first and most important" transformation:
     - Captures give the piece-configuration DAG: aegp-aaee becomes 144 subgames, the largest about 1 billion states per side.
     - Pawn-rank decomposition (the pawn never moves backward; 7 legal ranks) applied to the 72 subgames that contain a pawn gives **576 subgames**, the largest about **164 M** states per side.
     - Independent subgames give near-linear speedup.
   - **Models**:
     - **5.1**, one PU per subproblem: communication only at initialization.
     - **5.2**, many PUs with replicated data and dynamic job allocation: each PU broadcasts its new win/loss positions once per iteration, buffered and compressible. Communication is O(G) with broadcast and about O(2G) without. There is a duplicate-work overhead when two PUs share a common predecessor, worst case min(k,b)×. Numbers: PIII-866 generates about 800K positions/s; b = 15; 100BaseT carries about 3120K 32-bit messages/s; communication/computation ratio is 0.17 for k = 10 and 1.7 for k = 100.
     - **5.3**, centralized server: clients hold one bitmap plus a task buffer and download the other side's win bitmap twice per iteration (sparse and compressible). Communication O(2bG). The server is the bottleneck.
     - **5.4**, distributed data: b messages per Predecessor call and 2b (request + reply) per ProveSuccessor call. The number reads as O(2b²G). This needs a switched network. A 50 µs latency calls for about 8 KB buffers per PU. Static data split causes load imbalance.
     - **SMP**: threads share the global databases and bitmaps, "each processes part of the data at each iteration". Near-linear speedup while everything fits in RAM.
     - **Internet**: the server does all initialization, setting the values that depend on other subproblems, so clients can solve a subproblem with no communication.

8. **Draws, cycles and special rules**: Not discussed.

9. **Final storage / compression**: Not discussed. Compressing messages and bitmaps is mentioned only for communication.

10. **Verification**: Not discussed.

11. **Notable tricks**:
    - Decompose by **any** irreversible move, not only captures. Pawn rank, which is analogous to draughts man advancement/rank, gives a 7-way split.
    - The O(bG) cost model, counting generator calls.
    - Precompute cross-subproblem dependencies at init so each slice is self-contained.

12. **Mapping to a single for_each per pass**:
    - SMP model: Predecessor is a scatter (atomic bit OR) over a frontier. ProveSuccessor is a gather over candidates.
    - Model 5.2's duplicate-predecessor overhead is exactly the scatter collision. With a shared bitmap and atomic OR it disappears.
    - Subgame-level parallelism is an outer `for_each(par)` over independent DAG nodes in topological "waves", nested with inner passes.
    - The message-passing models (5.3, 5.4) are not single for_each passes.

---

## 2002-MGoNC-Wu-Beal.pdf

1. **Citation**: Ren Wu, Donald F. Beal. *A Memory Efficient Retrograde Algorithm and Its Application To Chinese Chess Endgames.* In *More Games of No Chance*, MSRI Publications Vol. 42, 2002, pp. 213–227.

2. **Scope**:
   - Chinese chess endgames in which one side has only defenders (aaee). There are 26 main endgames; with subgames, **378 databases, about 35 billion entries**.
   - Examples: aegp-aaee 1,004,157,000 (DTC 95); eegp-aaee 698,544,000; hpp/gpp/cpp-aaee 910,885,500 each; c-aaee 646,380 (DTC 18).
   - Same hardware as IS (Pentium Pro 200, 128 MB, 92 h for aegp-aaee).

3. **Metric**: A pair of databases with full depth. DTM or DTC selected by the boolean `dtm`. Value codes are illegal, unknown, WIN_IN_N(n) and LOSE_IN_N(n). Bits not stated.

4. **Indexing**: Same as IS:
   - legal squares (Table 1);
   - first-piece vertical mirror (Table 2: K 9→6, A 5→3, E 7→4, H/C/G 90→50, P 55→31);
   - multiple-piece combinations C(m,n);
   - piece grouping (Table 3: kaa 70/38, ka 40/21, kae 275/138, ke 62/32, kee 183/96, ee 21/12, hh/cc/gg 4005/2045, rp2/bp2 1485/765, rp3/bp3 26235/13135).
   - Rule: "always use the group with maximum savings to incorporate the symmetry reduction."

5. **Working storage during construction**:
   - `DATABASE W, B` with sequential access only.
   - `SBITS S`, a sequential-access bitmap.
   - `RBITS R`, the **only** random-access bitmap in RAM.
   - For 5-man pawnless chess, 15 MB is enough.

6. **Passes, in order** (verbatim pseudocode, White half; the Black half mirrors it):
   ```
   S = Load(W, WIN_IN_N(n));      // WTM win_in_n            [map over full range of W]
   R = Predecessor(S);             // BTM predecessors of S  [push from frontier S]
   S = Load(B, UNKNOWN);  S = S & R;   // BTM maylose_in_n   [map + AND]
   R = Load(W, WIN_<=_N(n));      // WTM win_in_n or less    [map]
   S = ProveSuccessor(S, R);      // BTM lose_in_n           [grandfather verification]
   B = Add(S, LOSE_IN_N(n));      // B += S                  [map]
   if (dtm) S = Load(B, LOSE_IN_N(n));   // include subgame-seeded lose_in_n
   R = Predecessor(S);            // WTM maybe win_in_n+1    [push]
   S = Load(W, UNKNOWN); S = S & R;      // WTM win_in_n+1
   W = Add(S, WIN_IN_N(n+1));
   ```
   - The loop is `while (!DoneWhite() && !DoneBlack())`. This is presumably a typo for `||`, since each half is guarded by `if (!DoneX())`.
   - Under DTM, the reload `if (dtm)` is needed because conversions seeded at init with exact depths add extra lose_in_n positions that ProveSuccessor never produced.
   - The losing side uses **verification**: all successors of a candidate must be in R. Candidates are the frontier unknown ∧ Pred(new wins).
   - Every pass is either a full sequential scan or a scan of a frontier bitmap.

7. **Parallelism / memory access**: None in this paper. Each pass touches at most one random bitmap; everything else is a sequential stream.

8. **Draws, cycles and special rules**: Undecided means draw. Special rules are not discussed. (Fang et al. later point out that these one-armed-side endgames are immune to them.)

9. **Final storage / compression**: Not discussed.

10. **Verification**: Not discussed.

11. **Notable tricks**:
    - The **two-bitmap ping-pong**: R alternates between "predecessors of the frontier" and "win_≤n". Value bands are *derived* by Load from the byte database each time, instead of keeping separate bitmaps.
    - Range predicates such as WIN_<=_N(n) make the "all successors win" test a single bit probe.

12. **Mapping to a single for_each per pass**:

    | Pass | Domain | Kind | `par` safety |
    |---|---|---|---|
    | Load / And / Add | full index range | gather (elementwise) | safe, vectorisable |
    | Predecessor | frontier bitset S | scatter into R | `atomic_ref<uint64_t>::fetch_or`, idempotent; or rewrite as pull: for each unknown, "any successor in S?" (gather, full range) |
    | ProveSuccessor | candidate bitset S | gather over R, writes own bit | safe if iterated per 64-bit word of S |

    All passes fit the for_each model. The pull form of Predecessor trades frontier locality for being free of races.

---

## 2000-CG-02-Fang-Hsu-Hsu.pdf

(Text extraction was heavily corrupted by font encoding. The content below is reconstructed from the readable prose. Tables 1 and 4–7 and the hardware details are unreadable.)

1. **Citation**: Haw-ren Fang, Tsan-sheng Hsu, Shun-chin Hsu. *Construction of Chinese Chess Endgame Databases by Retrograde Analysis.* Computers and Games (CG 2000), LNCS 2063, Springer 2001, pp. 96–114.

2. **Scope**:
   - **151** Chinese chess endgame databases with at most 2 attacking pieces and 4 defenders plus the two kings, and exactly **one armless side**. Sink databases are K + one attacker, because both-sides-armless is a draw.
   - Example finding: KNP(9)KGM is Red to win. There are only 431-something draws (digits garbled), and 219,090 positions are checkmates.
   - Hardware: **not recoverable from the PDF text**.

3. **Metric**:
   - Primarily **DTM** in plies, where the value means plies to *king capture*, exactly 2 more than conventional DTM. Values are 1 byte: **odd = win, even = loss**, with special codes for draw and temporary flags used during construction (the exact sentinel values are garbled; 255 appears).
   - **DTC** is used only when the value overflows the byte.
   - Under DTC, a loss child in a supporting database propagates "the smallest number indicating a win", and a win child propagates the smallest loss value. So DTC construction needs only the W/D/L of the supporting databases.
   - The 60-move rule is ignored.
   - The paper also discusses W/D/L (not infallible), DTC (infallible, imperfect) and DTM (perfect).

4. **Indexing**:
   - Split each side's pieces into subsets: R(s), N(s), C(s), P(s), and "the rest" (king plus defenders).
   - Each subset gets a local index from an ordering of its placements. Its "unit weight" is the number of arrangements.
   - Subsets are ordered with Red before Black, and within a side as rest, R, N, C, P.
   - Index = Σ (local index × product of the preceding unit weights), i.e. **mixed radix**.
   - Conjugate (mirror) redundancy is removed by **reordering exactly one subset's placements** so that no conjugate pairs remain. Reducing more than one subset would make the index ambiguous. Table 2 reports ratios of about 50–56%, e.g. 90→50 for a single R/N/C, 4005→2045 for RR/NN/CC, 1485→765 for PP. They usually pick the attacking subset.
   - 4- and 5-pawn subsets are excluded.

5. **Working storage during construction**:
   - Per state, two variables: **UnknownChildren**, the count of children not yet finalized, with a special value *Propagated*; and **BestValue**, the best value propagated so far, initially *Unknown*.
   - Edges are never stored; moves are generated on demand.
   - Apparently everything is in RAM. The 2002 paper says "our current code uses a large amount of main memory".

6. **Passes, in order**:
   - **init**: one full scan.
     - End states (king capturable, kings facing, stalemate = loss in xiangqi) get UnknownChildren = 0 and BestValue from Table 1.
     - Other states get BestValue = Unknown and UnknownChildren = their number of children.
     - In bottom-up mode, BestValue starts as the best value over children in supporting databases (**conversion** is folded into init). UnknownChildren counts only children inside the current database. The maximum such initial BestValue is recorded, and the supporting databases are released.
   - **propagation**: repeated **full traversals** with traversal index n. The invariant is that after traversal n, every non-draw state with value ≤ n has propagated.
     - During traversal n, a state with value n pushes value+1 to all parents (via unmoves). Parents update BestValue if the new value is better and decrement UnknownChildren.
     - A loss state (even n) is final when UnknownChildren = 0.
     - A win state (odd n) with BestValue = n propagates **even if its counter is non-zero**, because no later value can beat it.
     - A propagated state's counter is set to *Propagated*.
     - Stop when a traversal changes nothing. In bottom-up mode, always run at least as many rounds as the recorded maximum initial BestValue, because seeded values can still fire later.
   - **final**: set unpropagated states to draw.
   - The losing side uses **outcounting** (UnknownChildren). Wu-Liu-Hsu classify this as "category III with the queue replaced by repeated scanning".
   - The frontier is implicit: positions whose BestValue equals n and whose counter is not *Propagated*, found by scanning the full array.

7. **Parallelism / memory access**: Sequential, in RAM, with random access to parents. No disk algorithm.

8. **Draws, cycles and special rules**:
   - Section 4 classifies the special rules.
   - **Group 1** (forbidden sequences: perpetual check, perpetual chase) can turn a "draw" into a loss.
   - **Group 2** (permitted sequences, e.g. king or pawns chasing endlessly) leaves draws as draws.
   - Checkmate, king capture and stalemate outcomes are never affected, so stored wins and losses are always correct; only the draws can be wrong.
   - **Lemma-style argument**: if one side is armless it cannot check or chase forever. The attacking side, if it cannot win, can shuffle king or defenders instead of violating a rule. So plain retrograde analysis is exact for one-armless-side endgames.

9. **Final storage / compression**: 1 byte per position. Database sizes are given in units whose size is garbled. "Useless" databases (where an infallible strategy follows from the subgames) are identified. Heinz-style knowledgeable encoding is listed as future work.

10. **Verification**: An independent single traversal. Each end state is checked against Table 1. Each non-end state must equal **the best value propagated from its children**. All DTM databases were verified.

11. **Notable tricks**:
    - Odd/even ply parity encodes win/loss in one byte, so DTM ordering is plain integer comparison within a class.
    - A win propagates as soon as BestValue equals the current layer, without waiting for its counter.
    - The minimum number of rounds is set from the seeded supporting values.
    - A *Propagated* sentinel shares the counter field.
    - Mirror reduction on one subset only.

12. **Mapping to a single for_each per pass**:
    - init: full-range map. Gather from children and supporting databases.
    - Each traversal n: a `for_each` over the full range that **scatters** to parents (decrement counter, improve BestValue). Under `par` it needs `atomic_ref<uint8_t>::fetch_sub` and a CAS loop for "better value".
      - Determinism: to stay layer-synchronous, propagate only states that had value n at the *start* of traversal n. Either snapshot the frontier into a bitset first, or mark newly set n+1 states in a separate bitset (double buffer).
      - The "last decrementer" (the one whose fetch_sub returns 1) owns the decision to finalize the parent as a loss, which is race-free.
    - final and verification: full-range gathers.

---

## 2002-CG-03-Fang-Hsu-Hsu.pdf

1. **Citation**: Haw-ren Fang, Tsan-sheng Hsu, Shun-chin Hsu. *Indefinite Sequence of Moves in Chinese Chess Endgames.* Computers and Games (CG 2002), LNCS 2883, Springer 2003, pp. 264–279.

2. **Scope**:
   - Chinese chess endgames in which **both sides have attacking pieces**. 43 "important" databases are listed, e.g. KRKNGGMM with 251,181,481 legal positions.
   - Selection rules: both sides armed; at least one non-pawn attacker; with only two attackers, neither is a pawn.
   - Hardware: not stated.

3. **Metric**:
   - 1 byte. 0 = draw, odd i = win in i plies, even j = loss in j plies, in DTM or DTC.
   - New values for checking-indefinitely positions: **WinByDirectCheckingIndefinitely = 255** and **LossByDirectCheckingIndefinitely = 254**.
   - **Distance-to-check**: value = 255 − (plies to a direct-check win state). It propagates value − 1 to parents. Parity still gives win or loss.
   - Preference order: win by mate beats win by check; **loss by check is preferred to loss by mate**.
   - k-th order checking indefinitely would use the largest odd value below the minimum of order k−1. Only **first order** is implemented, because of the 1-byte limit.
   - When the byte overflows they switch to "DTC/check". Only KRKNGMM and KRKNGGMM needed it, in which case all supporting wins propagate one value and all supporting losses one value.

4. **Indexing**: As in the 2000 paper. Not re-described.

5. **Working storage during construction**: Uses a "large amount of main memory". Candidate flags (win candidate, loss candidate) are kept per position. The layout is not detailed.

6. **Passes, in order**:
   - (a) Original **init** and **propagation** (counter-based, as in the 2000 paper). This gives the "propagated state graph" G_S, in which every unknown vertex has at least one unknown child and no loss child.
   - (b) **repeat** until no unknown changes:
     - **Find primitive non-mutual checking-indefinitely positions**:
       1. *Init win candidates*, one full traversal: an unknown state where **the side to move is in check** becomes a win candidate. Every move into it was a checking move.
       2. *Init loss candidates*, a second traversal: an unknown state with **a parent that is a win candidate**, i.e. a child of a win candidate. A state may be both, which signals mutual perpetual check.
       3. *Pruning*, alternating traversals until there is no change:
          - odd traversals drop any loss candidate that has a child that is neither a win candidate nor a win state;
          - even traversals drop any win candidate with no loss-candidate child.
          - This computes a **greatest fixpoint** (a maximal subgraph). The pattern need not be periodic; the paper's example sequence is ABAABAAAB….
       4. *Final*: loss candidates become loss (254) and win candidates become win (255). Any dual-status candidates would be mutual checking; none were found in the 43 databases.
     - **Propagation of checking indefinitely**: an ordinary retrograde propagation seeded by those positions, with distance-to-check values ("indirect checking indefinitely").
   - (c) **final**: remaining unknowns become draws.
   - Detection is entirely **forward/pull**: successor scans over the candidate sets. Propagation is push with outcounting, as before.
   - Databases are still built bottom-up. Checking-indefinitely values can also propagate in from the supporting databases.

7. **Parallelism / memory access**: Not discussed. Everything is in RAM.

8. **Draws, cycles and special rules** (the core of the paper):
   - Rule summary (Chinese rule book [2]), applied to a sequence with a position repeated 3 times:
     1. If only one side checks indefinitely, the checker loses.
     2. Otherwise, a side whose plies are all forbidden loses if the opponent's plies are not all forbidden.
     3. Otherwise, draw.
   - Mutual perpetual check, and mutual chasing, is a draw.
   - Checking every **other** move (Fig. 2b) is not "mutual", so the side that checks every move loses.
   - Why plain retrograde analysis breaks: those positions end up "unknown" and are declared draws, when they are actually wins for the side being checked.
   - Formal R* primitive pattern: a maximal subgraph of the unknown part of G_S in which:
     - every vertex has out-degree ≥ 1;
     - the defender's vertices include all their unknown children and have no draw children;
     - each attacker vertex keeps at least one edge;
     - every defender move checks.
   - Perpetual *chase* and mutual check need much larger material (at least KRGGKCPGG ≈ 4.5×10^10, KRCPKRCGM > 2×10^13) and are ignored.
   - Distance-to-check rates reach **17.31%** and 13.05% in some databases.

9. **Final storage / compression**: 1 byte/position. A separate table records the order indices per database. Heinz's knowledgeable encoding is future work.

10. **Verification**: Each value must be the **best of the values propagated from its children** under the extended ordering (five rules). The only exception is direct-check win states, which must have at least one direct-check loss child and no better loss child. The verifier catches propagation bugs and hardware faults, but assumes the detection phase itself was correct.

11. **Notable tricks**:
    - Candidate seeding by a cheap local predicate ("in check").
    - Alternating gather-prune to a fixpoint, which finds non-periodic forced cycles without enumerating cycles.
    - Encoding a second distance family in the top of the byte range, counting downward from 255, while keeping parity.
    - An explicit preference order between families, so that verification remains "value = best over children".

12. **Mapping to a single for_each per pass**:
    - Win-candidate init: full-range map (local predicate). Gather.
    - Loss-candidate init: either scatter from win candidates to their *children* using forward moves (idempotent atomic bit-set), or gather over unmoves (any parent a win candidate?).
    - Each prune traversal: `for_each` over the candidate bitset, gathering children's flags and clearing its own bit. This is race-free per 64-bit word. Because pruning is **monotone** (flags only clear), even the odd and even steps can be fused and run asynchronously ("chaotic iteration"); the result is the same greatest fixpoint. Loop until nothing changes (a `transform_reduce` "changed" count).
    - Mark: map. Distance-to-check propagation: the same scatter-with-counter pattern as the 2000 paper.
    - The outer repeat-until-no-change and the higher-order iteration are host loops around these passes, not one for_each.

---

## 2004-CG-Wu-Liu-Hsu.pdf (merged with 2004-slides-Wu-Liu-Hsu.pdf)

1. **Citation**: Ping-hsun Wu, Ping-Yi Liu, Tsan-sheng Hsu. *An External-Memory Retrograde Analysis Algorithm.* Computers and Games (CG 2004), LNCS 3846, Springer 2006, pp. 145–160. The CG 2004 slides by the same authors are merged here; they add nothing new except the worked block-access example.

2. **Scope**:
   - Chinese chess endgames with one side armed: **396 databases, 70 GB**.
   - Largest: **KCPGGMMKGGMM, 8,785,969,200 positions** after symmetry removal, built in 78:36:54 (about 79 h). Previously the largest was KCPGMKGGMM with 2,143,092,600 positions, built by Wu-Beal in about 13 h on a 1 GHz P3 with 256 MB.
   - Record values: max **DTM 116 / DTC 96** in KCPGGMKGGMM, up from 114/94.
   - Hardware: P4 2.2 GHz, **1 GB RAM**, FreeBSD 4.5. The position count exceeds the number of RAM *bits*, so Wu-Beal's 1-bit scheme cannot run it efficiently.

3. **Metric**: DTM or DTC, both supported. Value classes: win_in_i, lose_in_j, draw, unknown. A stalemate is L0 and a checkmate is W1. Bits per value = ⌈log2 α_A⌉, where α_A is the number of distinct values. The experiments use 1-byte records, and the size column of Table 6 is consistent with 1 byte/position (that table is garbled).

4. **Indexing**: Not described. Symmetric positions are removed.

5. **Working storage during construction**:
   - On **disk**: the value plus nchild per position, ⌈log2 α⌉ + ⌈log2 β⌉ bits, where β is the maximum number of children.
   - Disk-resident **update lists** of 4- or 5-byte indices (5 bytes for indices ≥ 2^32).
   - RAM is used only for external-sort runs. The update list U "is usually larger than the endgame being constructed for the first few iterations".

6. **Passes, in order**:
   - **Taxonomy of sequential algorithms**:
     - **I**, repeated forward checking (Nalimov): scan the whole database calling update_f until nothing changes. This is a pull over the full range.
     - **II**, layered backward propagation with forward checking (Thompson 1996, Wu-Beal 2001): push L_i→W_{i+1}, then verify_loss on the parents of W_{i+1}. This is grandfather verification.
     - **III**, backward propagation with unknown-children counting through a FIFO queue (Gasser; Fang et al., who scan instead of using a queue): outcounting, not layered.
     - **IV**, layered backward propagation with counting (Allis et al.; Bal & Allis): outcounting in layers.
   - Lemma 1/2: in a layered scheme a position is **stable the first time it becomes a win**. A loss is stable when nchild = 0.
   - **Implemented algorithm (IV, external memory)**. Init seeds L0 (stalemates) and W1 (checkmates); the initial nchild values use the Hsu-Liu partitioning scheme. Then, per layer i:
     1. *Expand*: for each L_i position, generate unmoves and append the parent indices to U (a disk file).
     2. *Sort and combine*: external merge sort of U, then remove duplicates.
     3. *Update*: one sequential pass over the database in index order. If a parent's value improves, put it in W_{i+1} and mark it stable.
     4. *Expand*: for each W_{i+1} position, append its parents to U.
     5. *Sort*: if p occurs p_d times, collapse to (p, p_d).
     6. *Update*: sequential pass. nchild(p) −= p_d; if it reaches 0, put p in L_{i+1}.
     7. i++.
   - The losing side uses **outcounting with multiplicity decrements**.
   - Frontiers are disk lists of indices for L_i and W_{i+1}.

7. **Parallelism / memory access**:
   - The "disk updating problem": random updates cost n − (n−1)·B/N block reads and writes (Lemma 4). Sorted, deduplicated updates touch at most N/B blocks, and at worst read the file once (Lemma 5: n_d = N(1−(1−1/N)^n) distinct records).
   - Measured, with B = 10,240 one-byte records per block: 10^6 updates in a 100·2^20-record file take **3787.17 s random (999,905 blocks) vs 6.46 s sorted (10,240 blocks)**.
   - External merge sort (Vitter): one merge phase when W ≥ R, so the list is read and written twice; O(n log n). 500 M items cost 1.3 µs per 4-byte item on disk vs 0.76 µs in RAM, i.e. 2–3× slower.
   - Sort plus sequential update is 30–300× faster than random updates for N = 25·2^20, up to 600× for larger N.
   - Per position: 26 µs on average. About ¼ of the time is unmove generation and **about 4% is sorting**.
   - Solves Thompson's three objections to category II/IV:
     - init cost, via partitioning;
     - random disk access, via batching;
     - restartability: restart from layer i given a snapshot after layer i−1.
   - Single machine only. Korf's delayed duplicate detection is cited as an independent analogue.

8. **Draws, cycles and special rules**: Positions never stable become draws. The armed-vs-armless setting avoids the special rules. Extending to two armed sides and to tie-breaking rules is future work (slides, p. 23).

9. **Final storage / compression**: 70 GB total for 396 databases. Compression not discussed.

10. **Verification**: Not discussed here. It cites the Hsu-Liu 2002 graph-theoretic verification framework.

11. **Notable tricks**:
    - **Convert scatter into sort + combine + sequential gather.** Multiplicity counting (p_d) lets one counter update absorb many decrements.
    - Layer-granular checkpoints.
    - The explicit taxonomy I–IV, useful for positioning the user's design: egdb_fast.cpp is essentially category III/IV in RAM.

12. **Mapping to a single for_each per pass**:
    - *Expand*: `for_each(par)` over the frontier list. It produces a variable number of parents per element, so it needs either per-thread output buffers or a count pass, `exclusive_scan`, and a fill pass. Each element writes to its own slot range, so it is race-free.
    - *Sort and combine*: `std::sort(par)` followed by a run-length reduce (reduce_by_key). These are standard parallel algorithms, but not a for_each.
    - *Update*: `for_each(par)` over the **unique** sorted keys. Each key is a distinct position, so writes are race-free with **no atomics**. This is a gather of (key, multiplicity).
    - Overall this is the canonical race-free replacement for an atomic-decrement scatter, at the cost of O(n log n) sorting and the memory for U. In RAM, a scatter with `atomic_ref::fetch_sub` is cheaper. Sorting also improves cache locality when the database is far larger than the last-level cache.

---

## Group synthesis

**Primitives this group introduces**

1. **Bitmap-algebra retrograde with one random bitmap** (Wu-Beal 1996/2001/2002):
   - Each ply is a fixed script of elementwise stream operations (Load value-band, AND, Add) plus two generator passes: Predecessor (push into a bitmap) and ProveSuccessor (all successors in R).
   - Both sides and both DTM/DTC come from one loop.
   - Peak RAM is 1 bit/position, or 1 bit per two positions when the sides are processed alternately.
   - Cost: (b+1)G generator calls.
2. **Frontier-restricted grandfather verification**: candidates = unknown ∧ Pred(new wins), then verified against a range bitmap win_≤n. No counters are needed.
3. **Decomposition by irreversible moves**: captures, plus pawn rank (aegp-aaee goes from 144 to 576 slices, largest 164 M states per side). Gives subproblem-level parallelism and self-contained slices once cross-slice values are injected at init.
4. **Compact xiangqi indexing**:
   - per-piece legal-square domains;
   - mirror on one subset/"first piece" only;
   - identical pieces as C(m,n);
   - **joint piece-group enumeration** with mirror (kae 275→138);
   - mixed-radix composition of subset local indices (Fang).
5. **Counter-based propagation by full-array traversals** (Fang 2000):
   - UnknownChildren + BestValue per state, with a *Propagated* sentinel;
   - layer invariant "after traversal n, all values ≤ n have propagated";
   - wins fire as soon as they equal the layer value;
   - minimum rounds = max seeded value.
6. **One-byte odd/even ply encoding** (odd = win, even = loss, 0 = draw). DTC fallback on overflow. DTC conversion seeds use the smallest win/loss value, so only W/D/L of the subgames is needed.
7. **Repetition-rule handling** (Fang 2002):
   - Classify the rules into forbidden and permitted groups. Only forbidden-sequence rules (perpetual check) corrupt draws. Endgames with one armless side are provably clean.
   - Detection: seed candidates with "side to move in check", then alternate gather-pruning to a **greatest fixpoint**, then seed values 255/254 and propagate **distance-to-check** counting down, with an explicit preference order (mate-win > check-win; check-loss > mate-loss), iterated by order k.
8. **Verification as "value = best over children"** under an extended total order (Fang 2000, 2002). It is a single full-range gather pass.
9. **Taxonomy I–IV of retrograde algorithms** (Wu-Liu-Hsu): forward-checking scan; layered push + verification; queue + counting; layered + counting. Includes stability lemmas (a win is stable when first set in a layered scheme).
10. **Sort-and-combine batching of unmoves** (Wu-Liu-Hsu): expand the frontier into a parent list, external sort, run-length combine, then a sequential (race-free) update with multiplicity decrement. Up to about 600× over random disk updates. Gives layer-granular restart.
11. **Parallel cost models** (Wu-Beal IEEE): replicated data with broadcast of new decisions, O(G) communication; centralized server, O(2bG); distributed data, about O(2b²G); SMP threads over shared bitmaps.

**Lineage**
- Thompson 1986 (bitmaps, two random bitmaps) → Wu-Beal 1993/1996 earlier full-information algorithm (9 bits/position) → Wu-Beal **one-bitmap** algorithm (TR 1996) → republished with indexing and results in IS 2001, ICCA 2001 and MGoNC 2002 → parallel and decomposition analysis in HPDC 2001.
- Gasser-style outcounting → Fang-Hsu-Hsu 2000 (counter plus full scans, 1-byte parity encoding, verification) → Fang-Hsu-Hsu 2002 (adds a greatest-fixpoint detection layer for perpetual check on top of the same propagation).
- Allis/Bal-Allis layered counting (category IV) + Hsu-Liu partitioning + Vitter external sorting → Wu-Liu-Hsu 2004. It explicitly places Wu-Beal in category II and Fang in category III, and replaces Wu-Beal's "1 bit in RAM" bound with "disk-resident everything, sorted batches".

**Takeaways for a `for_each(policy, …)` design**
- Full-range value-band extraction, ANDs and verification passes are all **gathers**.
- Predecessor marking is an **idempotent scatter**. Use atomic OR, or rewrite it as pull over unknowns.
- Counter decrements are a **non-idempotent scatter**: use `atomic_ref::fetch_sub` with "last decrementer decides", or Wu-Liu-Hsu's sort + reduce_by_key + unique-key update, which needs no atomics.
- Fixpoint pruning (perpetual check) is a **monotone gather**, safe under asynchronous `par`.
- Layer synchrony needs a frontier snapshot or a double-buffered next-frontier bitset.
