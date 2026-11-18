# Group 1: classic chess EGDB papers (1977-2001)

Source notes: every PDF was read through the Google Drive text extractor. How complete each extraction was:

| File | Text extraction |
|---|---|
| 1977-DM-Strohlein-Zagler.pdf | Complete, but the OCR is very noisy. Most Boolean matrix formulas are garbled, and the formulas below are reconstructed from the surrounding prose. |
| 1985-ICCA-Herik-Herschberg.pdf | Complete (pp. 66-87), clean text. Not scanned-only, despite the 10 MB size. |
| 1986-ICCA-Thompson.pdf | Pages 132-139 only. The first page (p. 131: introduction and the "G" (Gödel) index function) is missing from the extraction. Table 1 and the move notation in the game lines are garbled. |
| 1996-ICCA-Thompson.pdf | Starts at p. 216. The opening page(s) (introduction and sections 1-2) are missing. Everything from section 2's hardware table onward is readable. |
| 1996-GoNC-Stiller.pdf | Complete (Games of No Chance pp. 151-192, about 115 KB of text). Matrix formulas are partly garbled but can be reconstructed. |
| 2001-ICGA-Stiller.pdf | Complete (2 pages). |

---

## 1977-DM-Strohlein-Zagler.pdf

1. **Citation**: Th. Ströhlein and L. Zagler, "Analyzing games by Boolean matrix iteration", *Discrete Mathematics* 19 (1977) 183-193. Received 1975. The computational details are deferred to Ströhlein's 1970 TU München dissertation, *Untersuchungen über kombinatorische Spiele*.
2. **Scope**: General finite win/draw/lose games with perfect information on a bipartite game graph (P = P_A ∪ P_B, Boolean transition matrix T = T_AB ∪ T_BA). Chess: "some chess endings with three and four men", all positions computed in 1969 on an AEG-Telefunken machine (OCR reads "TIM", probably the TR 4) at Leibniz-Rechenzentrum München. Endings: KRK, KQK, KRKB, KRKN, KQKR. Position counts and run times are not given.
3. **Metric**: The "win prediction number" m(p) = min{n : (a^[n] ∨ b^[n])(p)}, which is Steinhaus' remoteness function: the number of moves to a terminal win. The game is played until a king is captured, so m counts moves to *king capture*. The OCR for KRK reads "17 (for capturing king, i.e. 26 for mating)" (garbled). For 4-man endings it is a "modified" number: moves until a capture into a subgame that the capturer then wins. In effect a DTC. Per-position encoding is not discussed: the formalism is a family of Boolean vectors a^[n], b^[n] ∈ B^P.
4. **Indexing**: Naive. A position with m men is an element of S^m with S = {1..64}, plus a side-to-move index. There is no legality filter. Elements with two equal components (a piece captured) are kept as *terminal* positions rather than removed. Illegal positions (adjacent kings, the side not to move in check) are neutralised by "preassignments and some modifications of the chess rules": play until a king is captured, so checks can be ignored.
5. **Working storage**: Boolean vectors over all of P (one bit per position per vector): the terminal vectors a^0, b^0, d (draw), and the iterates w_A^[n], l_B^[n]. The paper stresses that "Algorithm 2.5 requires at each iteration step manipulating all possible positions", which caused capacity problems. RAM or disk: not discussed.
6. **Passes, in order**:
   - **init**: Terminal positions. King captured = terminal win for the capturer. Positions reached by an ordinary capture get the value of the already-computed subgame (this is **conversion**, folded into init as terminal vectors). Stalemates are found "before entering the iteration and adjoined to the vector of terminal draw positions".
   - **iteration (forward/"pull", full range)**: Theorem 2.3/2.4 and Algorithm 2.5. Reconstructed from the prose, because the formulas are garbled:
     - w_A^[n](p) for p ∈ P_A is true if p is a terminal win, or if some q with T(p,q) has l_B^[n-1](q). This is the Boolean product T_AB·l_B^[n-1] OR'd with w_A^0.
     - l_B^[n](p) for p ∈ P_B is true if p is nonterminal and *every* q with T(p,q) has w_A^[n](q). In matrix form this is ¬(T_BA·¬w_A^[n]), the complement-product-complement form.
     - Iterate until x^[n] = x^[n-1].
     - Remark 2.6 gives a monotonicity relation (w^[n] ⊆ ...) "of use in the realization", but the formula is unreadable.
   - Losing side technique: neither grandfather nor outcounting. Each iteration re-evaluates every position from its successors (a full re-scan as a Boolean matrix-vector product).
7. **Parallelism / memory access**: Not parallel. A Boolean vector formulation is natively bit-parallel (word-wide OR/AND), but the paper does not claim to exploit this.
8. **Draws, cycles, special rules**: Draws are whatever is never set (a ∨ b = 0). For "last-player-winning" games (no terminal draws), the fixpoint equation x = T·x̄ is a *kernel* equation. Iterating from x = 0 and from x = 1 gives the two extremal kernels, which bound every other kernel. The draw set is their difference. This makes the paper a formal treatment of cycles: an arbitrary kernel "may not determine all draw positions". Stalemate is a terminal draw. Repetition and the 50-move rule are not discussed. In KQKR the rook side can sometimes force stalemate, repetition or capture of the queen.
9. **Final storage / compression**: Not discussed.
10. **Verification**: Not discussed beyond chess plausibility of results (KQK ≤ 10 moves to mate; KQKR modified maximum 31; KRKB maximum 18; KRKN maximum 3, OCR uncertain).
11. **Notable tricks**:
    - (a) Treat each pass as Boolean set algebra over the whole state space. This is the conceptual ancestor of Thompson's bitmaps and Stiller's complement-predecessor-complement rule.
    - (b) Keep "captured" slots (two equal coordinates) as terminal positions that carry the subgame's value instead of special-casing them. This is a simple way to handle conversion inside one index space.
    - (c) Avoid legality tests by playing to king capture.
    - (d) The kernel view of draws.
    - (e) Optimal strategies via remoteness, with the minimax payoff (|play|+1)^-1.
12. **Mapping to a single for_each per pass**: Both half-steps are pure **gathers over the full index range**:
    - `for_each(par, all_A, p -> w[p] = w0[p] || any(succ(p), l_prev))`
    - `for_each(par, all_B, p -> l[p] = nonterminal(p) && all(succ(p), w))`
    
    Each element writes only itself, so the passes are race-free under `par`. With bitsets the unit of work must be a whole 64-bit word (iterate over word indices), so that two threads never read-modify-write the same word. Depth is exact only if the iterate is double-buffered by level (Jacobi). An in-place sweep would still converge to the right WDL but would lose the depth n (Thompson 1996 makes the same point about the Chinook sweep). The cost is a full sweep per ply, which is fine for draughts with few plies and fatal for depths of 200 or more.

---

## 1985-ICCA-Herik-Herschberg.pdf

1. **Citation**: H.J. van den Herik and I.S. Herschberg, "The construction of an omniscient endgame data base", *ICCA Journal* 8(2), June 1985, pp. 66-87. Implementation largely by S.T. Dekker (earlier Dutch publications 1982-1984).
2. **Scope**: Chess KBNK only. 64^4 = 16,777,216 raw positions, reduced by a factor of 6.4 to 2,621,440 entries.
   - Hardware: Amdahl 470 V/7-B (16 MB memory, 14 ns cycle), Pascal.
   - Run time: 28 min 58 s, "less than 1 ms per position".
   - Results: max mate-in-33 (the "maximin"); 10,822,184 legitimate won positions, 53,320 drawn and 5,901,712 illegitimate (unreduced counts).
3. **Metric**: DTM in White moves (WTM only). One signed byte per entry (MATINGDISTANCE):
   - -1 = illegitimate;
   - 0 = legitimate and undecided, so draw at the end;
   - n > 0 = mate in n.
   
   Black-to-move positions are *not stored*. BTM values are implied (mate in n with BTM equals the minimum over Black moves). The authors note BTM queries need "an additional 1-ply search".
4. **Indexing**: A 4-D array MATINGDISTANCE[WN 0..63, WK 0..63, WB 0..31, BK 0..19].
   - White to move.
   - The bishop is restricted to dark squares (WB indexes only the 32 dark squares), via the vertical midline reflection.
   - The BK is restricted to a 20-square trapezium a1-h1-e4-d4, via diagonal reflections.
   - Lookup tables WBSI[x], BKSI[x] map squares to indices. Illegal slots stay in the array and are marked -1.
   - Positions with the BK on the square of the WB or WN are deliberately *legitimate* with value 0. They represent "Black just captured", a drawn KBK/KNK, and remain 0 (draw) forever.
5. **Working storage**: The single 2.6 MB byte array in RAM. There are no bitmaps, no counters and no BTM table. A ~3 MB working set fit in the 16 MB machine.
6. **Passes, in order**:
   - **init (full range)**: Every entry is set to -1 if illegitimate (Black in check with WTM, two white pieces on one square, kings equal or adjacent) and 0 otherwise.
   - **mate-in-1 (forward/pull, full range)**: Examine every 0-valued position and set it to 1 if White mates in 1.
   - **retrograde loop (push with verification), for n = 1, 2, ... until no new position**: For each mate-in-n WTM position (the text says "WITH all mate-in-n positions DO"; how they are enumerated is not stated, presumably by scanning for value n), run RETRACE:
     - (1) Generate the BK *unmoves* to get BTM antecedents.
     - (2) **Grandfather/verification**: for each antecedent, generate all its BK moves. The move is "forced" iff no BK move reaches a WTM entry with value 0 (undecided/draw, which includes a capture) or value n+1 (found in this same pass, hence slower). Entries of -1 (illegal: into check or next to the WK) are ignored.
     - (3) If forced, COMPLETE generates all *white unmoves* from that BTM position and writes n+1 into every WTM predecessor that is still 0. Assignments > 0 are final.
   - Losing side: classic grandfather verification. Wins: push via unmoves.
   - Conversion: none needed for KBNK. Captures go to drawn subgames, encoded statically as 0.
7. **Parallelism / memory access**: Serial. Random access into the in-RAM array. No I/O issues.
8. **Draws, cycles, special rules**: Everything still 0 at the end is a draw: the capture slots, "ineluctable degeneracy" (forced loss of a piece), and positions where White must choose between stalemate and loss of a piece. The 50-move rule is discussed only as motivation. Repetition is implicit (never decided means draw).
9. **Final storage / compression**: The raw byte array. No compression.
10. **Verification**:
    - Cross-check against Thompson, who claimed 32. Their analysis program found the error in Thompson's line, and Thompson later admitted a bug in his program.
    - Comparison with book lines (Euwe/Donner deviate by one move).
    - Manual spot checks.
    - Simplicity of the high-level source.
    
    Their own first run was wrong. It left zeros in won positions because COMPLETE lacked extra reflections for the BK on a main diagonal (REFLECT C/D).
11. **Notable tricks**:
    - (a) Store only one side to move and derive the other by a 1-ply search. This halves memory, and the BTM layer exists only transiently inside RETRACE.
    - (b) Level-tagged in-place values: the verification test "successor == 0 or == n+1 means not forced" makes the result independent of the order in which the n+1 writes land within a pass.
    - (c) Capture-into-draw encoded as a never-touched 0 slot.
    - (d) **A cautionary bug**: with symmetry reduction, a position lying on a symmetry axis has several images. Every push must write all symmetric images (or canonicalise), or errors "compound disastrously" at later depths. This is the same class of problem Thompson 1996 hit with move counters.
12. **Mapping to a single for_each per pass**:
    - init and mate-in-1 are full-range gathers (race-free).
    - The retrograde pass is a **map over the frontier** {value == n}. Its frontier would be a full scan with a filter, or better a bitset or bucket.
    - Inside each step, the verification part is a pure gather (reads only).
    - The COMPLETE step is a **scatter** into WTM predecessors. The write is idempotent ("if 0, write n+1"), and the forced test treats an n+1 already present the same as a 0. So concurrent execution gives the same result as serial, *provided* the byte loads and stores are atomic (`std::atomic_ref<uint8_t>` relaxed load + CAS 0→n+1). Without atomics it is formally a data race in C++, although benign on real hardware.
    - The algorithm can be turned into a race-free form with no atomics. Make it two pull passes per ply over WTM/BTM bitmaps: BTM-lost-in-n (gather: all moves into won) and then WTM-win-in-(n+1) (gather: any move into lost). That is the Strohlein/Stiller form.

---

## 1986-ICCA-Thompson.pdf

*(Extraction starts at p. 132. Page 131, which defines the "G" Gödel index function and the introduction, is missing.)*

1. **Citation**: K. Thompson, "Retrograde analysis of certain endgames", *ICCA Journal* 9(3), September 1986, pp. 131-139.
2. **Scope**:
   - Chess 5-piece endgames, pawnless and with one pawn. Table 1 is garbled. From the prose and background: KQKBB (71 moves), KRBKR, KBBKN, KBNKN, and the promotion targets needed for KQPKQ and KRPKR.
   - Pawnless index size: 121,110,528 positions per side-to-move bitmap. [Inference, not stated in the extracted text: 121,110,528 = 462 × 64^3, i.e. 462 symmetry-reduced king pairs × 3 free men, consistent with the 462 of Thompson 1996.]
   - Hardware: Sequent Balance 8000, 12 × NS32032, 16 MB shared memory, on average 4 processors in background.
   - Run time: 2-3 weeks real time per pawnless endgame, about 6 weeks for a pawn database (excluding subgames).
3. **Metric**: DTM ("Max to Mate", single pass) or DTC ("Max to Win", two-pass: mate *or* capture into a won subgame, "precisely the conditions for the 50-move rule"). For pawn endgames the metric extends to "capture, mate or pawn move", i.e. DTZ. Final encoding: one byte per position. Byte j = i if bit j is in W_i, and 0 if the position is not a White win. White-to-move results only.
4. **Indexing**:
   - Pawnless: the G function (the defining page is missing). Thompson notes it counts some positions more than once (symmetric duplicates), which skews the win percentages.
   - Pawn: size 83,886,080 per rank slice. The G function loses symmetry. The pawn is on one of four files (left-right mirror), "one rank at a time", and each pawn position is solved as its own sub-game. After the six per-rank result files (83 MB each) are built, they are inverted into four rank-oriented files of 117 MB each, which is a final re-slicing. Illegal slots are kept in the index.
5. **Working storage**: All on **disk** as bit-map files of 121,110,528 bits:
   - W: all known WTM wins;
   - B: all known BTM losses;
   - W_i: newest WTM win-in-i;
   - B_i: newest BTM lose-in-i;
   - J: temporary superset of B_{i+1}.
   
   Plus the byte result file (updated incrementally after each W_i). Total about 12 × 121M bits ≈ 175 MB of secondary storage. RAM use: 1 MB per processor used as a cache on the randomly accessed files. Pawn databases need about 120 MB per rank.
6. **Passes, in order** (four programs P1-P4, plus P5 for pawns):
   - **P1 init (full range)**: Loop over all G positions, decode each, and test for Black checkmated. Set those bits in B_0. Then B = B_0, W = 0, i = 0.
   - **P2 retrograde push (frontier B_i)**: For each position in B_i, generate White *unmoves*. Each predecessor not in W goes into W_{i+1}. Afterwards W |= W_{i+1} and the result file is updated.
   - **P3 retrograde push (frontier W_{i+1})**: Generate Black unmoves. Each predecessor not in B goes into J_{i+1}, the candidates ("lose if Black wanted to mate himself").
   - **P4 verification (frontier J_{i+1}, forward/pull)**: Generate Black *successors* of each candidate. If all are in W, set the bit in B_{i+1}. Then B |= B_{i+1}.
   - Loop: i++, repeat P2, P3, P4 until no new positions.
   - Losing side: **grandfather/verification** against the cumulative W bitmap. Thompson's version: frontier = only the predecessors of the newest wins, and the test is a bitmap membership test.
   - **Conversion ("two-pass Max-to-Win")**: The first pass solves all sub-games with fewer men. The second pass seeds B_0 with "these positions" (BTM positions that are lost by conversion) together with mates.
   - **Pawn slicing**:
     - Solve all promotion sub-games first and combine them into W_80 (WTM, pawn on the 8th, win in any number of moves).
     - P5 makes pawn *unmoves* to create B_70, which is combined with mates on the 7th and wins without the pawn.
     - Iterate P2-P4 for the rank (again in two passes for full material), then P5 backs up into B_60, and so on.
     - At rank 2, the files for ranks 3 and 4 (as the garbled text implies) must be combined because of the double step.
   - Unmove generator semantics:
     - It is illegal to start in check but legal to unmove into check.
     - It is illegal to capture but legal to *uncapture* by leaving an enemy piece behind.
     - Un-castling and un-en-passant do not occur. Un-promotion is handled via P5.
7. **Parallelism / memory access**:
   - Shared-memory multiprocessor. Work is divided by giving each processor every N-th position (cyclic distribution).
   - Disk traffic was "drastically reduced by allocating a megabyte of real memory per processor as a cache on the files that were randomly accessed".
   - Access pattern: the frontier file is read sequentially, while the target bitmaps (W or B membership, the W_{i+1}/J_{i+1} writes) are accessed randomly.
   - Bit-parallelism: bitmaps allow OR-merging whole files (W |= W_{i+1}).
8. **Draws, cycles, special rules**:
   - Anything not in W at the fixpoint is not a White win (draw or loss). Only the White-wins side is solved.
   - The 50-move rule is matched by the DTC/DTZ metric. A Black win would need the colours swapped.
   - QPKQ: under-promotions were "accidentally discarded" (a clerical error), so the 7th-rank maxima may be slightly off.
9. **Final storage / compression**: An uncompressed byte-per-position result file (121 MB pawnless). The pawn files are inverted to rank-oriented files.
10. **Verification**: Not described in the extracted pages. Comparison with Komissarchik-Futer KQPKQ analysis shows a disputed position (claimed win in 13 vs ≥ 23). The paper acknowledges a known error in an earlier KBNK program (32 vs 33).
11. **Notable tricks**:
    - (a) **Frontier bitmaps plus cumulative "summation" bitmaps**: newest-level sets drive the next level, and cumulative sets give O(1) "already known" filtering.
    - (b) Depth is kept out of the working set: the byte result file is written once per level from W_i, so working storage per position is about 12 bits on disk.
    - (c) A separate candidate set J and then a verify pass, so verification only touches the predecessors of the newest wins.
    - (d) Waves by material (sub-games first) and by pawn rank (P5 un-promotion and pawn unmoves seed the next slice).
    - (e) An unmove generator with uncapture, used to step from a sub-game into the parent game.
    - (f) Cyclic distribution of positions across processors, plus a per-processor cache for the random-access file.
12. **Mapping to a single for_each per pass**:
    - **P1**: full-range gather; writes only its own bit, so iterate over 64-bit words to stay race-free.
    - **P2/P3**: maps over a **frontier bitset** that **scatter** into W_{i+1} / J_{i+1}. The writes are idempotent bit-sets (OR), so `std::atomic_ref<uint64_t>::fetch_or(relaxed)` on the target word is enough, with no ordering issues and no lost updates. The "not in W" filter reads a bitmap that is frozen during the pass (W is updated after P2), so it is race-free. Alternatively, drop the filter and AND later, as in Thompson 1996's I/O trick.
    - **P4**: map over the frontier J that is a **gather** (reads W, writes only its own bit in B_{i+1}). Race-free if each task owns whole words of B_{i+1}, i.e. iterate J by word index and loop over the set bits inside each word.
    - **Merges** (W |= W_{i+1}, B |= B_{i+1}, writing the result byte from W_i): trivial full-range maps over words.
    - **P2 rewritten as a pull**: P2 can be turned into a full-range gather (WTM x is new if it has any move into B_i), which removes the atomics but costs a full sweep. This is the usual frontier-push vs full-pull trade-off.
    - Every pass can be expressed as a single for_each. The only shared state written concurrently is OR-into-bitset.

---

## 1996-ICCA-Thompson.pdf

*(Extraction starts at p. 216. The opening page(s) are missing. Text from the hardware table onward is complete.)*

1. **Citation**: K. Thompson, "6-piece endgames", *ICCA Journal* 19(4), December 1996, pp. 215-226 (transcribed talk).
2. **Scope**:
   - Chess 6-piece, pawnless. Finished KRBKBN: max-to-win 98, agreeing with Stiller; it finished on pass 70 of wave 6 after about 3 months, plus 1.5 months lost to a failed algorithm.
   - Index: 462 king-pair placements (reflections, rotations, legality) × 64^4 = 7.75 × 10^9 (about 8G) slots. One bitmap is about 970 MB.
   - Machine table: 1985: 1 MIP, 16 MB memory, 500 MB disk, 1 MB/s I/O. 1996: 150 MIP, 250 MB memory, 50 GB disk, 10 MB/s I/O. The CPU gained 150× but I/O only 10×.
3. **Metric**: DTC ("max-to-win": until the first capture, per the 50-move rule definition), built in waves. RES is a byte vector holding the pass number at which a position was set (8 bits, "10 or 11 [bits], depending on how you count" in total working cost per position). Thompson notes a one-ply ambiguity for positions where *Black* makes the converting capture, and argues the literature needs a unified convention.
4. **Indexing**: Gird = {kgird ∈ [0, 462), pgird ∈ [0, 64^(NPIECE-2))} (`KH = 462`, `PH = 1L << (6*(NPIECE-2))`). Illegal positions are left in ("too hard to remove them and it is not worth doing the effort") and skipped by an `illeg()` test. A position is held as a 6-byte piece-square vector, converted by convg2p/convp2g.
5. **Working storage**: Disk files of 8G bits each: B, W, X (frontiers), BTM, WTM (summation bitmaps), plus the RES byte vector. RAM (250 MB) is used as a big hash/cache for the *randomly accessed* file of a pass. Total working cost per position is about 10-11 bits.
6. **Passes, in order** (Fig. 2; I/O-enhanced Fig. 3 in brackets):
   - **1. init (full range)**: `B = btmmate(~0); BTM = 0; WTM = 0; n = 0`. Loop over all kgird × pgird: skip illegal positions, skip if Black is not in check, skip if White is in check. Then call `bmove(hpost)`; the callback aborts on the first legal Black move. If no move exists, set the bit (Fig. 9).
   - **2. record**: `BTM |= B; RES = res(B, n); n++`.
   - **3. retrograde push (frontier B, sequential read)**: `W = wunmove(B) & ~WTM; WTM |= W`. [I/O-enhanced: `W = wunmove(B)` with no AND, so only W is randomly written.]
   - **4. retrograde push (frontier W)**: `X = bunmove(W) & ~BTM`. [Enhanced: `X = bunmove(W & ~WTM)`, where the AND is done sequentially on the source.]
   - **5. verification / forward pull (frontier X)**: `B = bmove(X) ⇒ WTM`, i.e. keep x if *every* Black move lands in WTM. [Enhanced: `B = bmove(X & ~BTM) ⇒ WTM`.] Then go to 2.
   - Losing side: **grandfather verification** against the WTM summation bitmap.
   - **Waves (conversion)**: Wave w = 3, 4, 5, 6 is the number of pieces. Each wave reuses the program restricted to w-piece subsets; pieces can be GONE, and the unmove code has explicit uncapture branches (`if (posn.p[2] == GONE) {...}`). Initialization between waves (Fig. 13): `B = btmmate(~0) | BTM(previous wave); BTM = WTM = 0`. This is insufficient because of positions where *Black* is forced to capture into a lost sub-game (Diagram 1, 24...Kxg6). The enhanced initialization (Fig. 14) adds `X = bunmove(WTM); B |= bmove(X) ⇒ WTM`, i.e. one extra Black-unmove plus verify pass from the previous wave's WTM. How the GONE state is encoded in the 64^4 index is not stated in the text.
   - **Alternative 1 ("Chinook algorithm", Lake-Schaeffer-Lu 1994)**: 1-2 bits per position. Repeatedly sweep all positions sequentially *in place*, deciding unknown positions from their successors, until nothing changes. Cheap, fast and partitionable, but it "walks over itself", so there is no depth/RES and no way to play perfectly. Thompson rejects it for chess.
   - **Alternative 2: outcounting, which Thompson says "does not work"**:
     - The RES slot holds a counter of unproven successors until the result is known, plus 1 flag bit (counter or result) and the W bit, about 10 bits.
     - Step 5 disappears: decrement on each back-up and resolve at zero.
     - The three problems he lists:
       - (i) each back-up now fetches 10 bits instead of 1, so the random I/O is 10× larger;
       - (ii) **because of symmetry reduction and index transforms, the number of moves out of a position ≠ the number of unmoves into it**, so the counters must be initialised by counting *unmoves*. That is an extra full pass, about 3 days;
       - (iii) it is not restartable after an interruption, since partial decrements are unknown.
7. **Parallelism / memory access**:
   - Init is "parallelizable. We only divide the main for loop into little pieces, there is no random access".
   - Every retrograde pass reads its source frontier **sequentially** (getseq returns the next set bit) and writes exactly one target bitmap **randomly**. The goal of the Fig. 3 rewrite is to make exactly one random-access stream per pass.
   - Code generation: `mksub` emits piece-configuration-specific C for `wattack/battack`, `wmove/bmove` and `wumove/bumove`. These are callback-driven: the callback returns non-zero to abort, which is used for early exit in mate and verify tests. Edge-bit tables (`edge[sq]` with UP1/UP2/... bits) gate offsets, and sliders repeat the test.
8. **Draws, cycles, special rules**: Untouched means not a White win. Draws are not separated from Black wins (one-sided solve). The 50-move rule motivates DTC. There is the one-ply convention issue for Black-forced conversions.
9. **Final storage / compression**: Attempts to compress were "disappointing". No portable format yet.
10. **Verification**: KRBKBN maximum agrees with Stiller (98). The BTM-lose counts were not yet double-checked.
11. **Notable tricks**:
    - (a) **Delayed masking**: don't AND with a randomly accessed bitmap during a scatter. Write a superset and AND it sequentially at the start of the next pass. This applies directly to cache/NUMA locality.
    - (b) Exactly one random-access stream per pass.
    - (c) Callback move generators with early abort for "exists"/"forall" tests.
    - (d) Generated per-material code.
    - (e) Waves with the extra Black-unmove+verify pass, so Black-forced conversions are picked up.
    - (f) The outcounting warning: with symmetry-folded indices, initialize counters with the *unmove* multiplicity (or count distinct canonical successors consistently on both sides). Counters also kill restartability.
12. **Mapping to a single for_each per pass**:
    - **Step 1 (init)**: full-range gather, race-free (own bit; split by words).
    - **Step 2 (record)**: full-range elementwise map over words (`BTM |= B`, RES[i] = n where B[i]), race-free.
    - **Steps 3/4**: frontier-bitset maps that **scatter** into W / X. They are idempotent ORs, so atomic `fetch_or` on words (or per-thread private bitmaps OR-reduced afterwards) suffices.
    - **Step 5**: frontier map that is a **gather** (reads WTM, writes own bit of B), race-free with word-granular tasks.
    - **Chinook alternative**: an in-place full-range sweep. Under `par` its result is still a correct WDL fixpoint (monotone), but non-deterministic in the order of discovery. It is not a clean per-ply for_each, and it yields no depth.
    - **Outcounting alternative**: a scatter with read-modify-write (`fetch_sub`, then claim at zero). Each decrement needs an atomic, and the thread that observes 1→0 owns the resolution, so it is race-free with atomics but not lock-free-trivial on bytes. This is exactly the pattern in the user's `egdb_fast.cpp`.

---

## 1996-GoNC-Stiller.pdf

1. **Citation**: L. Stiller, "Multilinear algebra and chess endgames", in R.J. Nowakowski (ed.), *Games of No Chance*, MSRI Publications 29, Cambridge University Press, 1996, pp. 151-192.
2. **Scope**:
   - Chess. All pawnless 5-piece and many one-pawn 5-piece endgames (earlier version, Stiller 1989: KBNKN 77 max-to-win). Many pawnless 6-piece endgames (Table 2, over 40 endgames), e.g. KRNKNN 243, KRBKNN 223 (a general win), KRNKBN 190, KQNKRR 153, KQRKQR 92.
   - State space: 462 · 64^4 ≈ 7.7 × 10^9 hyperboard points per endgame, of which 6,185,385,360 = 462·62·61·60·59 are non-overlapping placements. Opposite-coloured bishops: 1.67 × 10^9.
   - Hardware: 64K-processor CM-2/CM-200 with 8 GB RAM, hypercube interconnect, 7 MHz (10 MHz CM-200).
   - Run time: about 1200 s initialisation and 111-172 s per iteration (K_i → K_{i+1}). Per-node time about 6000× faster than classical 5-piece programs. Thompson reported his best serial code as only about 700× slower.
3. **Metric**:
   - Distance-to-win (DTC: moves until White can force conversion into a won sub-game) is used for the reported results. DTM was implemented for 5-piece ("not very illuminating").
   - The working set is Boolean: v_i = WTM positions from which White wins within i moves.
   - A distance per point "can be stored so that a two-ply search permits optimum play". **Stored Gray-coded, so incrementing the value modifies only one bit.** [Interpretation: with a bit-sliced counter, each iteration flips exactly one bit-plane under a mask.]
4. **Indexing**:
   - The "hyperboard" B_k = (V_8 ⊗ V_8)^{⊗k}: a k-fold tensor power of the 8×8 board, i.e. a 2k-dimensional cube of side 8 with 64^k points. A position is the basis element c_1 ⊗ ... ⊗ c_k (one square per piece). A set of positions is a Boolean vector.
   - "Holes" (two pieces on one square) are zeroed by the diagonal projection L.
   - Symmetry: the black king (first factor) is confined to the 10-square fundamental octant. Storage is 10 · 64^(k-1) (C/D_4 ⊗ C^{⊗(k-1)}). Only 1/8 of the hyperboard is physically stored.
   - Identical pieces could be folded via symmetric powers Sym^j C. This is noted as possible but the groups were too small to need machinery.
   - Captures and pawns: each sub-game (piece subset) and each pawn position has its own hyperboard.
5. **Working storage**: Bit vectors over the whole (symmetry-reduced) hyperboard, all in RAM. The 7.7 G-node state space is "near the maximum that the target architecture could store in RAM" (8 GB). There are no move counters.
6. **Passes, in order**:
   - **init**: mates (and, by implication, conversion seeds from sub-game hyperboards). About 1200 s. Captures, stalemate, castling and the 50-move rule are disregarded "for simplicity of exposition" in the core equations.
   - **iteration, one combined Boolean step per ply** (backup rule, §6.3):
     - **v_{i+1} = X_White( ¬ X_Black( ¬ v_i ) )**, where X_White = Σ over White pieces s of X_{S_s,s}, X_Black is defined likewise, and ¬ is set complement.
     - Read inside-out:
       - (a) ¬v_i = WTM positions not yet known won.
       - (b) X_Black(·) = BTM positions with at least one Black move into such a position (without captures, unmove and move are the same relation).
       - (c) Its complement = BTM positions all of whose moves lead into v_i, i.e. lost. This includes "modulo stalemate" no-move positions, which must be corrected separately.
       - (d) X_White(·) = WTM positions with a move into a lost BTM position.
     - Iterate until v_i = v_{i+1}. The final i is the maximin.
     - Stiller 2001 phrases the same rule as "the predecessors of the complement of the predecessors of the complement" and notes that "Thompson's backup rule was different".
     - Losing side: **neither grandfather nor outcounting**. It is a full-state Boolean complement-predecessor-complement, i.e. "forall" computed as ¬∃¬. Every pass runs over **all positions** (no frontier).
   - **Unmove operators as structured matrices** (Table 1). U_s = I_{64^(s-1)} ⊗ (E_8 ⊗ I_8) ⊗ I_{64^(k-s)} is an end-off shift of piece s by one square along one board axis. The operators are averaged over C_4 (rotations) or D_4:
     - X_R,s = ∫_{C4} L U_s (I + L U_s)^6. A rook slides by up to 7 masked shifts, and L kills paths through occupied squares (blocking).
     - X_B,s = ∫_{D4} L U_s (I + L U_s r U_s)^6 (diagonal steps).
     - X_N,s = L ∫_{D4} U_s · r(U_s^2).
     - X_K,s = L ∫_{C4} (U_s + U_s r U_s).
     - X_Q = X_R + X_B.
     
     Moving the black king outside its octant applies the D_4 element that brings it back, transforming the whole hyperboard. This is a communication pattern along the Cayley graph of D_4.
   - **conversion**: "uncapture" = an unmove in the parent game, then a diagonal matrix, a sequence of stride permutations and a broadcast (SPREAD; a tensor with the 1×64 all-ones matrix). It is inter-hyperboard communication. Un-promotion communicates between a quotient (symmetry-reduced) hyperboard and a full (pawn) hyperboard.
7. **Parallelism / memory access**:
   - Massively data-parallel SIMD. Each Boolean step is a sequence of whole-array shifts, masks (L), ORs and complements.
   - Shifts map onto the hypercube via Gray-coded coordinates (one hop for neighbours). Code is generated from the matrix factorization: I_m ⊗ B means "each of m processors runs B on its local block" (parallel), and A ⊗ I_m means "A on vectors of length m" (vectorized). Stride permutations trade one for the other (the Commutation Theorem).
   - The access pattern is regular and *oblivious*: data-independent, with no random access at all. The same factorization was vectorized by Wendroff et al. (1993) and ported to superscalar CPUs (per Stiller 2001).
8. **Draws, cycles, special rules**: Whatever is not in v at the fixpoint is not a White win (one-sided, like Thompson). Stalemate needs explicit correction ("modulo stalemate"). The 50-move rule is discussed: DTC values ≥ 50 are affected, and an m-move rule would change values for positions with distance-to-win greater than m. Mutual zugzwangs (WTM draws, BTM loses) are counted in Table 2 (column Z).
9. **Final storage / compression**: The 6-piece results mostly were not saved (Thompson 1996 says Stiller "could not save the answers", except KRBKNN). Distances are Gray-coded. Source code: ftp.cs.jhu.edu/pub/stiller/snark.
10. **Verification**: Not much in this paper. Results agree with Thompson's 5-piece work and later 6-piece work (see Stiller 2001 for the protocol).
11. **Notable tricks**:
    - (a) **Move generation as array shifts of the whole state space.** A piece move for *all* positions at once is a shift along one coordinate of a 2k-dimensional array; sliders are repeated masked shifts. For bitboard-indexed draughts with a naive 32^k or 50^k index this applies directly. With the user's colex/combinatorial index it does not, because shifts are not contiguous in colex order.
    - (b) **The ∀ as ¬∃¬ backup** gives a pure gather/OR formulation with no counters and no verification.
    - (c) Symmetry via a group action on the stored fundamental domain, and canonicalisation only when the special (king) piece leaves the domain.
    - (d) Gray-coded / bit-sliced depth counters.
    - (e) Express the algorithm algebraically and generate code per architecture from the factorization.
12. **Mapping to a single for_each per pass**:
    - Every operator is a **gather over the full index range**: (U_s v)[x] = v[x − e_s], so each output element reads neighbours and writes only itself, race-free.
    - One ply can be written as two full-range for_eachs over word-blocked bitsets:
      - `for_each(par, btm_words, w -> lost[w] = AND over black moves of v)` (or OR of ¬v, then complemented, i.e. the "all successors won" test as a gather);
      - `for_each(par, wtm_words, w -> v_next[w] = v[w] | OR over white moves of lost)`.
    - Double buffering (v and v_next) gives exact depth and deterministic results.
    - In the pure shift form, the inner loop is itself SIMD over 64 positions per word *only if* the index is laid out so that a piece move is a fixed word/bit offset (naive per-piece coordinates). With colex ranking you would decode, generate moves and re-encode per position. That is still a gather and still race-free, but not bit-parallel.
    - Cost: a full sweep per ply regardless of frontier size. That is acceptable when depth is small or the machine is wide.
    - Symmetry canonicalisation of the king makes the gather read from a transformed index, which is still a gather.
    - The uncapture/broadcast is a gather from the sub-game array (read-only, already built), so it is race-free.
    - No pass in this scheme requires scatter.

---

## 2001-ICGA-Stiller.pdf

1. **Citation**: L.B. Stiller, "Retrograde analysis: software architecture", *ICGA Journal* 24(2), June 2001, pp. 86-87.
2. **Scope**: A retrospective on the software of his CM-2/CM-5 and serial 6-piece chess builder (about 12,000 lines of C). Sizes are not given (see Stiller 1996).
3. **Metric**: Not discussed (see Stiller 1996: DTC).
4. **Indexing**: The "hyperboard": the cross product of 6 chessboards. Piece motion is "movement along a plane of a hypercube", described as "a 12-dimensional analog of the sliding-piece move generation in Thompson's BELLE".
5. **Working storage**: Not discussed beyond RAM constraints. Memory allocation is managed by module R.
6. **Passes, in order**: The one algorithmic statement: "The set of White-to-win-in-k+1 moves is the predecessors of the complement of the predecessors of the complement of the set of White-to-win-in-k moves, modulo stalemate. Thompson's backup rule was different." This is a full-state Boolean **pull/gather** step per ply (see Stiller 1996). "The first observation allows for efficient parallel decomposition; the second [the hyperboard] for efficient communication." There are no frontier, grandfather or counter steps.
7. **Parallelism / memory access**:
   - Connection Machines plus serial ports. The program is P = X + Y = L + R + Y:
     - L chooses the matrix factorization;
     - R manages resources (memory allocation);
     - Y moves bits in the hyperboard (performs the matrix multiplications), e.g. "tilt the hyperboard to the left in dimension 5".
   - Changing the factorization ported it to single-CPU superscalar machines: "suitable for high throughput in a vector or superscalar single-CPU architecture".
8. **Draws, cycles, special rules**: "modulo stalemate" only. Otherwise not discussed.
9. **Final storage / compression**: All disk I/O is checksummed. A locking protocol makes file writes atomic.
10. **Verification** (the main content):
    - (a) Machine primitives replaced by stubs that validate arguments and record memory and call statistics. A **simulated run** with a guessed iteration count checks R (memory and time budgets) and argument validity on any workstation.
    - (b) **Compile for a 4×4 board** and run against the real library to debug L. There was extensive analysis of 4×4 6-piece games.
    - (c) **Multiple runs with different mapping geometries** of hyperboard onto physical RAM, to detect transient hardware faults.
    - (d) Checksummed, atomic disk I/O, so "any transient single point of failure, disk, RAM, or CPU, should not have resulted in erroneous output".
    - (e) Results confirmed by Thompson's 2000 6-piece tables.
11. **Notable tricks**:
    - Separate *what moves where* (factorization) from *how bits move* (backend), so the plan can be simulated without the hardware.
    - Small-board builds as a test oracle. For draughts: build the same code on 4×4/6×6 boards and cross-check against a naive builder. The user already has `egdb_6x6.cpp`.
    - Rerun with a different data layout or thread decomposition and compare results bit-for-bit, which catches races and hardware faults.
    - Checksum every file.
12. **Mapping to a single for_each per pass**: The complement-predecessor-complement step is two full-range **gathers** (see Stiller 1996 entry), so it maps directly to `for_each(par, ...)` over word-blocked bitsets, race-free. The "rerun with different geometry" check maps naturally to running `seq` vs `par` (or different chunkings) and diffing outputs.

---

## Group synthesis

### Distinct primitives introduced in this group

1. **Boolean fixpoint over the whole state space** (Ströhlein 1970/1977). Win-in-n and lose-in-n vectors are computed by the matrix iteration w^[n] = w^0 ∨ T·l^[n-1], l^[n] = ¬(T·¬w^[n]).
   - Pass type: full-range **pull/gather**, double-buffered per ply.
   - Draws: whatever stays undecided; formally, the gap between the extremal kernels.
   - Conversion: "captured" slots become terminal positions that carry the sub-game value.
2. **Level-tagged in-place byte array with verification** (van den Herik-Herschberg-Dekker 1985).
   - Only one side to move is stored; the other side is a transient 1-ply layer.
   - Frontier = entries equal to n.
   - Push via unmoves, then a **grandfather "forced" check** (no successor equal to 0 or n+1), then COMPLETE writes n+1.
   - Order-independent within a ply thanks to the n+1 tag.
   - Teaches the symmetry-image pitfall.
3. **Thompson bitmap pipeline** (1986).
   - Per ply: B_i → (White unmove, scatter) → W_{i+1} → (Black unmove, scatter) → J → (Black move, all ∈ W, gather/verify) → B_{i+1}.
   - Uses **cumulative summation bitmaps** for O(1) "known" tests and a separate byte RES file written per level.
   - Unmove generator with **uncapture**.
   - **Waves** (sub-games first; seed B_0 with conversion losses) for DTC/DTZ.
   - **Pawn-rank slicing** with un-promotion and pawn-unmove passes (P5).
   - Cyclic work split plus a cache for the random-access file.
4. **I/O-shaped Thompson** (1996).
   - **Delayed masking**: scatter into a superset, then AND sequentially on the next read, so each pass has one sequential source and one random target.
   - Extra Black-unmove+verify pass between waves for Black-forced conversions.
   - Generated per-material move/unmove code with abort callbacks.
   - Documents two alternatives:
     - **in-place sweeps** (Chinook; WDL only, no depth);
     - **outcounting** (counter in the result slot). It fails in his setting because of 10× random I/O, a moves-out ≠ unmoves-in mismatch under symmetry folding, and no restartability.
5. **Stiller's complement-predecessor-complement backup** (1989-1996; restated 2001).
   - v_{i+1} = Pred_W(¬Pred_B(¬v_i)), modulo stalemate.
   - Moves become **whole-array shifts** on a 2k-dimensional "hyperboard" (sliders = repeated masked shifts; holes masked by a projection L).
   - Symmetry via a D_4 group action on a stored fundamental domain.
   - Uncapture = broadcast from the sub-game array.
   - Gray-coded per-node depth.
   - Fully oblivious, **no scatter, no counters, no frontier**, hence ideal for SIMD/bit-parallel or `par`.
6. **Verification protocols**:
   - Cross-checking independent programs: Herik vs Thompson found real bugs on both sides.
   - Stub/simulated runs, small-board (4×4) builds, reruns with different data-to-hardware mappings, checksummed atomic I/O (Stiller 2001).
   - Missing here: the modern "recompute every value from children" pass, which the user already has.

### Lineage

- Ströhlein (1970 thesis; 1977 paper) gives the first EGDBs as Boolean matrix iteration. It was ignored, and Clarke reinvented the method in 1977.
- Thompson built 4-piece databases privately from about 1975 (KBNK etc.).
- Van den Herik/Herschberg/Dekker independently built KBNK as a byte array plus verification (1982-85). A cross-check exposed Thompson's off-by-one bug.
- Thompson 1986 publishes the bitmap frontier pipeline with waves and pawn slicing on a 12-CPU shared-memory machine.
- Stiller 1989 parallelises Thompson's scheme on the Connection Machine but changes the backup rule to the oblivious complement-predecessor-complement form. Stiller 1991-96 adds the multilinear-algebra formalism and 6-piece results (KRNKNN 243). Stiller 2001 is the retrospective on software verification.
- Thompson 1996 scales the 1986 pipeline to 6 pieces on one disk-bound workstation. He optimises for I/O, compares against Chinook's in-place sweep (Lake-Schaeffer-Lu 1994), rejects outcounting, and confirms Stiller's KRBKBN 98.

### Relevance to "one for_each(policy) per pass" in egdb_draughts

- **Gather-only formulations are race-free under `par` with no atomics** (Ströhlein, Stiller, Thompson's verify step P4/step 5). Requirements:
  - double-buffered levels;
  - tasks that own whole 64-bit words when writing bitsets.
  
  The cost is a full sweep (or a frontier sweep for verify) per ply.
- **Scatter passes** (Thompson P2/P3; Herik COMPLETE; the user's current outcounting loop) are idempotent ORs or tagged first-writer-wins stores. They map to `std::atomic_ref<uint64_t>::fetch_or` (bitsets) or to a relaxed CAS on `uint8_t` "UNKNOWN → lv+1". Alternatives: Thompson's trick of scattering into a superset and masking afterwards, or per-thread private bitmaps OR-reduced at the end.
- **Outcounting under `par`** needs an atomic `fetch_sub` on the counter and "claim at zero". Thompson's warning applies if symmetry folding is ever added: counters must count *unmoves into* the canonical index, not *moves out*.
- **A hybrid fits the user's design best**:
  - frontier bitsets per level (Thompson);
  - push-to-candidates with `fetch_or`;
  - a gather-verify over the candidate bitset, which removes counters entirely and makes every pass a race-free for_each except the OR-scatter;
  - alternatively, Stiller's full-range two-gather ply when the frontier is large.
