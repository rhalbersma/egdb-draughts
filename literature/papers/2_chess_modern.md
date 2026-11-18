# Group 2: chess EGTs, 1999 to 2016 (Heinz, Nalimov, Haworth et al., Bourzutschky-Tamplin, Huntington, Zakharov)

Reading status. All eight PDFs returned text through `mcp__Google_Drive__read_file_content`. Six were too large to return inline, so the tool saved them to disk. I extracted those with `jq` and read every prose section. The long appendix tables (statistics, PGN lines, index-size tables) were skimmed, not read cell by cell. The extracted text has some layout damage: superscripts, subscripts and multi-column tables got scrambled (for example "DTZ\n50" for DTZ50, and several BTH-2005 strategy formulas are unreadable). Where this matters it is flagged. Statements marked **[background, not verified from PDF]** come from general knowledge, not from the files. Statements marked **[inference]** are my own reasoning, not the authors' claims.

A key caveat for this group: **none of these papers gives full pseudocode for a construction.** The closest are NWH-1999 (a shared skeleton plus a description of the Nalimov and Wirth variants) and HH-2015 (a paragraph on sweep versus unmove). The 2016 Zakharov paper covers *compression* of the Lomonosov tables only. Its construction algorithm is in its references [1, 2], which are not in this set.

---

## 1999-ICGA-1-Heinz.pdf

1. **Citation**: E.A. Heinz (1999). "Endgame Databases and Efficient Index Schemes for Chess." *ICCA Journal* 22(1), pp. 22–32 (March 1999), a Note.
2. **Scope**: Chess, 3- and 4-piece databases, plus suggestions for 5- and 6-piece. Compares Edwards' tablebases (DTM, both sides to move, one byte per position, a single file access per probe), Thompson's databases (DTC, Huffman-compressed per endgame, weaker side to move only, 4 CDs from 1991), and Heinz's own schemes, used in DarkThought (WMCC 1997). Appendix C covers Nalimov's then-new tablebases (all 3–4-man; 5-man as of Feb 1999; built in "roughly one hour" for 3–4-man "on a fast personal computer").
3. **Metric**: Describes others' metrics: Thompson DTC, Edwards DTM (1 byte). Criticises DTC for engine use ("hard to use because they must be converted to reasonable scores that do not conflict with real mates") and criticises Thompson's coverage of weaker-side-to-move positions only. Heinz's own DBs are converted from Edwards' DTM files.
4. **Indexing** (the core of the paper):
   - Naive scheme: 64^k.
   - Pawnless: 8-fold symmetry confines one piece to a 10-square triangle. Edwards uses a1–d1–d4 and confines the *last* listed piece, which is "tricky" because the confined piece differs per endgame. Thompson uses a8–d8–d5 for the white King and adds diagonal symmetry when that King is on the long diagonal, enumerating only legal K–K placements: **462** instead of 10×64 = 640 (−27.81%). Thompson uses a 1024-entry (2 KB) lookup table instead of the minimal 640-entry one.
   - Pawns: queen-side-flank (QSF) confinement of one Pawn halves the range. Removing ranks 1 and 8 gives 24 squares for the QSF Pawn and 48 for the others (Edwards wastes 1/3 by keeping 32). Heinz enumerates legal K–K placements on the full board: **3612** of 4096 (−11.82%), via an 8 KB table `legal_kk_enum[Kb][Kw]`.
   - Directly rammed Pawns (opposite-colour Pawns on the same file, adjacent): one index covers both, with 20 slots per pair. KP·KP· = 3612×20 = 72,240.
   - En passant, option A: index the ep-capturable Pawn by its own back-rank square (+16.67% per Pawn). Option B: remap the 14 ep constellations into otherwise-illegal "both Pawns on the same square" slots (Figure 1 mapping), for zero space overhead. In the tight scheme, add 7 extra values to the first opposite-colour Pawn pair: 24×47+7 = **1135** (+0.6%).
   - "Equal locations" (no two men on one square), computed on the fly as suggested by Nalimov: I(P_n) = 48 − Σ_i [P_n > P_i] and I(X_n) = 64 − [X_n>K_b] − [X_n>K_w] − Σ[X_n>P] − Σ[X_n>X_i]. Savings: 3.13% (1st piece), 7.67% (2), 13.44% (3), 20.20% (4).
   - Equal pieces: sort the pair. C(64,2) = 2016 (−50.78%). With both Kings excluded, C(62,2) = **1891** (−53.83%). A second pair gets C(60,2) = 1770, so KXXKYY saves 80.05%. Pawn pairs: C(47,2) = 1081. KPPK with a QSF Pawn can only be pseudo-diagonalised: 576 versus an optimum of 564.
   - Equal material (KPKP, KQKQ, …): store only one side to move, and answer the other by vertical mirror plus colour swap.
   - Broken/illegal slots: not discussed beyond reusing illegal same-square slots for ep.
5. **Working storage during construction**: not discussed. Heinz's modified DBs were "derived … automatically from Edwards' tablebases with a double-checking conversion program". He notes that smaller index spaces may also mean "fewer elements to process" during construction.
6. **Passes**: not discussed. Background only: retrograde generation, reinvented in the 1970s; Stiller (1989) ran a massively parallel generator reusing Thompson's indexing.
7. **Parallelism / memory access**: Not discussed for construction. For probing: a single random file access per probe (Edwards) versus multiple accesses through Huffman decoding (Thompson), which "slow[s] the search down to a crawl".
8. **Draws, cycles, special rules**: Castling ignored (all DBs assume no castling rights). En passant handled in indexing. 50-move rule not discussed.
9. **Final storage / compression**: Thompson used Huffman per endgame. Edwards' files are uncompressed vectors. Heinz argues external compression makes probe cost unpredictable.
10. **Verification**: Only the "double-checking conversion program" (Edwards to Heinz format).
11. **Notable tricks**: (a) Legal-K–K enumeration tables (462 / 3612 / 1806). (b) Dynamic "subtract the number of already-placed men below me" ranking, which needs no tables. (c) Sorted like-piece pairs. (d) Reusing illegal slots for rare special states (ep), which suggests a draughts analogue: reuse impossible slots for flags. (e) Storing one side to move when material is symmetric (draughts with colour-flip symmetry: storing W-to-move only when w == b).
12. **Mapping to a single `for_each`**: The paper has no passes. Its index functions are pure, so `rank`/`unrank` can be called from any `par` lambda. The dynamic subtract-ranking is cheap enough to compute per element. An index with "holes" (illegal slots) needs a cheap legality predicate so that every pass can skip broken slots, or a one-off `for_each` that marks them broken (a race-free gather).

---

## 1999-ICGA-2-Heinz.pdf

1. **Citation**: E.A. Heinz (1999). "Knowledgeable Encoding and Querying of Endgame Databases." *ICCA Journal* 22(2), pp. 81–97 (June 1999). The first page is numbered 81. The end page is not visible in the extract; **[background, not verified from PDF]** it is 81–97.
2. **Scope**: Chess, all 3- and 4-piece endgames, held entirely in RAM by DarkThought (more than 800K nps on a 600 MHz Alpha; more than 1M nps at WMCC 1997 on a 767 MHz Alpha) and probed at all nodes, including quiescence search. Total size **15,839,880 bytes ≈ 15.1 MB**, compared with more than 173 MB for Edwards' non-trivial 3–4-man DTM tablebases (about 11-fold).
3. **Metric**: Domain-dependent encodings of WDL or DTM per endgame (Table 1):
   - **0 bits**: endgames that are trivially drawn or generally won, with rare exceptions caught by "recognizer" rules that *fail* back to search. Examples: KBK, KNK, KBBK, KBNK, KQxK.
   - **1 bit, "stronger side wins / not"**: KBKP, KNKP (KP side wins), KBPK, KNPK, KPPK, KQKB, KQKN, KRKB, KRKN.
   - **2 bits, D/L/W**: KPKP, KQKP, KQKQ, KQKR, KRKP, KRKR.
   - **4-bit DTM**: KQK (0..10). KRK uses 0..15, with "lost/mate in 2" sharing a code with mate-in-1 (a deliberate small imprecision).
   - **8-bit DTM**: KPK (0..28; 5 bits would suffice).
   - The 1-bit encodings save 7.2 MB (47.7%) compared with plain 2-bit WDL.
4. **Indexing**: Heinz-1999-1 schemes (462×62, 462×62×61, 462×1891, 3612×24, 3612×24×61, 3612×24×47, 3612×576). Equal-material endgames store one side to move only. Using Edwards' indexing instead would give 27.3 MB (+80.9%); Thompson's would give 19.2 MB (+27.3%).
5. **Working storage during construction**: Not discussed. The DBs are produced by a conversion tool from Edwards' files, not by retrograde analysis.
6. **Passes**: Not discussed (no construction). The only "pass" is the conversion: for each index, read Edwards' value, re-index, and pack bits. It is validated "by means of exhaustive comparisons with the original tablebase data".
7. **Parallelism / memory access**: The paper is about making probes RAM-resident because disks "might be able to serve a few thousand [random reads per second] at best". Both sides to move are interleaved in one bit vector: `pos_bit_index = pos_index × sides × bits`, `res_bit_index = pos_bit_index + bits × stronger_stm`, and the value is `(db[res/8] >> (res%8)) & (2^bits − 1)`.
8. **Draws, cycles, special rules**: Not discussed for construction.
9. **Final storage / compression**: The knowledgeable encoding above. The related-work section cites Chinook's 2-bit WDL for 4.44×10^11 positions (1–8 pieces), purging "capture and capture-threat positions" by storing a fixed value in their slots before run-length encoding, and Gasser's RLE/LZW for Nine Men's Morris.
10. **Verification**: Exhaustive comparison with Edwards' tables. Stress tests of the knowledgeable scoring against Edwards' DTM tables.
11. **Notable tricks**: (a) Choosing per endgame between 0, 1, 2, 4 or 8 bits. (b) Packing both sides to move into one bit vector. (c) **Don't-care slots for positions resolvable by a cheap rule or a small probe.** In draughts this means capture positions (captures are mandatory and always leave the slice), following Chinook as cited here and generalised by Zakharov 2016. (d) Using recognizer "fail" as an escape hatch.
12. **Mapping to a single `for_each`**: The conversion/packing pass is a gather over the full index range, but packing sub-byte values into shared bytes is a *write race* under `par`. Partition so that each task owns whole bytes or words (iterate over output words, each assembling 8/bits or 64/bits entries), or use `atomic_ref<uint64_t>::fetch_or`. The don't-care marking pass ("value is implied by a capture") is a race-free gather over the full range.

---

## 1999-ICGA-Nalimov-Wirth-Haworth.pdf

1. **Citation**: E.V. Nalimov, C. Wirth, G.McC. Haworth (1999). "KQQKQQ and the Kasparov-World Game." *ICCA Journal* 22(4), pp. 195–212.
2. **Scope**: Chess, the 6-man pawnless KQQKQQ ("4Q"), built on request in days for the 1999 Kasparov–World game. Two independent builds:
   - **Nalimov (DTM)** on a 2 GB, 500 MHz server, "completely 'in store' and avoiding all disc thrashing". Generation plus self-check took **2 days**.
   - **Wirth (DTC)** with Gasser's RetroEngine on a 1 GB, 450 MHz G3 PowerMac, with about **100 GB of disk traffic** and **5 days**.
   - Stiller's earlier 6-man work used the SIMD CM-2/CM-200 (64K processors), but the data was lost.
   - Both authors first stress-tested their codes on KNNKNN and KRRKRR. Nalimov then built KBBKBB and all six KxxKyy.
   - maxDTM 100 ply (btm), maxDTC 88 ply.
3. **Metric**: DTM (Nalimov) versus DTC (Wirth). Table 4 introduces a goal/strategy/metric nomenclature: GV/V (value only, EV table), GF/DTF (Distance to "FIDE event" = Pawn push, capture or mate, i.e. what is later called DTZ), GC/DTC (mate, capture or P-conversion), GM/DTM, and GR (mate within the FIDE 50-move rule). The extracted row gives "DTR = DTF". That cell is garbled, and BTH-2005 later *redefines* DTR, see below.
   - Depth conventions: an EM entry "wins in m moves" means 2m−1 plies. For DTC this cannot be inferred: it could be 2m−1 or 2m, because the loser may be the side forced to convert (position NP5).
   - The authors note that DTM/DTC/DTF alone cannot win every position winnable under the 50-move rule (positions M4, BN1-2, NP1-4).
   - Storage width per entry: not stated.
4. **Indexing**:
   - Naive 64^6 = 68,719,476,736 cells, too big for a 4 GB 32-bit address space. Conditions: C1 (one man per square), C_KK (Kings not adjacent), C_ad (files a–d), C_C (equal forces mapped to btm), C_P (Pawns on ranks 2–7), C_8 (octant a1–d1–d4), C_D/C_TE (diagonal case, other King in the a1–h1–h8 triangle).
   - **N_K = 33 + 3×(30+58+55) = 462** and **N_KP = 4×60 + 24×58 + 36×55 = 1806**. "Explicit management" of K–K positions replaces the n-cube with an n-space of lower dimension.
   - C_LM (like men as a set, a factor L!) and C_UC (no unblockable check by the stm). Under C_UC, a Queen is excluded from 3, 5 or 8 squares depending on the King. This splits the index into many sub-index n-spaces, each recorded in a **metatable** of start offsets and dimensions.
   - 4Q: 1,546,346,340 "notionally considered" positions. Wirth's R = 462×62!/(58!·4!·2!·2!) = 1,546,346,340. Nalimov's R = {33×59×58 + [58×57×56 + (30+55)×54×53]×3} × (60×59)/(2!·2!) = **1,237,357,440** (−19.98%).
   - **Broken** positions (term from Edwards et al. 1995): positions seen to be illegal, redundant, or failing condition C. With both Kings on the a1–h8 diagonal, a position and its reflection both satisfy C. The "best way to remove one is to mark the higher-indexed position as broken". This makes each equivalence class unique and saves about 4.35% (~21/483). Nalimov keeps both copies; Wirth marks the higher one broken.
   - Partitioning by Pawn formation into sub-endgames is recommended (citing Lake, Schaeffer & Lu 1994).
5. **Working storage during construction**: Val[i] holds value and depth per position. Wirth: InitialValue = **number of successor positions within the target endgame profile**, i.e. an outcount in the same cell, plus an implicit "changed in last iteration" set. Nalimov: Val only. Widths are not given. Results: Nalimov W/D/L = 417,439,889 / 129,060,182 / 136,739,434 with 554,117,935 broken after indexing (44.78%). Wirth W/D/L = 407,438,945 / 125,942,240 / 133,753,410 with 879,211,745 broken (56.86%).
6. **Passes, in order** (common skeleton, quoted almost verbatim from §3):
   1. **init** (full range): determine R; set `Val[i] ← broken` for all i; then for every not-broken i set `Val[i] ← InitialValue`. Nalimov: *draw*, unless the side not to move is in check, in which case broken. Wirth: the successor count. Wirth also marks broken the higher-indexed diagonal duplicates and sntm-in-check positions.
   2. **init + conversion** (full range, one pass): for each unbroken i, if stm is mated set Val = 0. If stm "can convert to win", set Depth1. If stm "must convert to lose", set Depth2. Nalimov: Depth1 = DTM(converted)+1 and Depth2 = DTM(converted), probing the 5-man tables. Wirth: Depth = 1 ply for both, since DTC counts to conversion.
   3. **Iterate** `while ChangeFlag: for each necessary i: Evaluate(Val[i], neighbours)`.
      - **Nalimov: forward / successor-scan ("pull") over all positions each iteration.** "It is necessary to visit each unbroken Position(i) in each search and Evaluate infers or re-infers its value and depth from those of its … successors." The rules are: win if any successor is a loss for the sntm, loss if all successors are sntm wins. The process terminates when a sweep changes nothing. HH-2015 adds that Nalimov gave "a position a depth at the earliest opportunity but lower[ed] it later as required", so this is a *chaotic relaxation*: values can be revised within the sweep, and the number of sweeps is not tied to depth. No losing-side technique is needed beyond re-reading all successors, a **full re-scan**.
      - **Wirth: retrograde / predecessor ("push") over a frontier.** "The necessary positions are only those whose information changed in the last iteration." Newly decided stm **losses** are unmoved (no uncaptures) and their predecessors become sntm **wins**. Newly decided stm **wins** decrement each predecessor's count of not-yet-proved-losing successors, and a predecessor is marked "lost in n plies" when its count reaches zero. This is **outcounting** (credited to Thompson 1986), and resolved positions form the next frontier. One iteration corresponds to one ply.
   4. **verification** (full range): `Compatibility-Check(Val[i], successors)` for every unbroken i, see item 10.
7. **Parallelism / memory access**: Nalimov needed everything in RAM (2 GB) because the full-sweep pull reads successors at random. Wirth's frontier-based RetroEngine streamed to and from disk (100 GB of traffic). The verification is explicitly described as a "**parallelisable forward pass**". Stiller mapped the position set onto SIMD hardware.
8. **Draws, cycles, special rules**: Draw is the initial value (Nalimov). Anything not decided when iteration stops stays a draw (Nalimov) or keeps a non-zero count (Wirth); cycles therefore resolve as draws implicitly. The 50-move rule is discussed only as a limitation: KQQKQQ's maximum is exactly 50 moves, so EF/ER tables were not needed. Castling is not encoded. En passant is not relevant in this endgame.
9. **Final storage / compression**: Not detailed here (see NHH-2000/2001). Both EGTs store wtm and btm.
10. **Verification**: A forward pass checks for each position that "a position won by the stm in p plies is succeeded by positions lost in ≤ p−1 plies" and that "a position lost … in p plies is succeeded only by positions won in ≥ p−1 plies". This "ensures that any unmove generator is the exact inverse, within the target profile, of the move generator, and that no random errors have occurred", but "cannot check any software which is common to itself and the generation process". Further checks: cross-checks between two independent programs and two metrics (W/D/L totals agree to within broken-position conventions), agreement with Stiller's summaries and with Edwards' DTM tables, and identical mutual-zugzwang sets. The paper warns about cosmic rays, power faults and system software errors (Gasser 1995; Wirth & Nievergelt 1999).
11. **Notable tricks**: (a) A shared skeleton in which only InitialValue, Depth1/Depth2, "necessary i" and Evaluate change between pull and push. (b) Storing outcounts in the value cell. (c) Marking the higher-indexed symmetric duplicate broken. (d) A metatable of sub-index n-spaces. (e) Partitioning Pawn endgames by Pawn structure; in draughts this means partitioning by the men configuration, because men only move forward. (f) Building the same table independently under a second metric to cross-check.
12. **Mapping to a single `for_each`**:
    - init/conversion: a **gather** over the full range (writes only Val[i], reads smaller DBs). Race-free under `par`.
    - Nalimov sweep: written as in-place relaxation, it reads neighbours that other threads are writing. Under the C++ memory model that is a data race on plain bytes, though algorithmically benign because it is monotone. **[inference]** To make it a clean `par` gather, make it *level-synchronous*: pass n assigns only depth n (win-in-n if some successor is loss-in-(n−1); loss-in-n if every successor is a win of depth ≤ n−1 and at least one equals n−1). Every value it reads was then fixed in earlier passes. A concurrent undecided→n write is indistinguishable to readers who only test "≤ n−1", so it is race-free with `std::atomic_ref<uint8_t>` relaxed loads and stores, or with a double buffer. Cost is O(N × branching) per ply, so it suits shallow tables only.
    - Wirth push: a **scatter**. Decrementing predecessors' counters needs `atomic_ref<uint16_t>::fetch_sub`, with the thread that sees 1→0 claiming the position. Marking winning predecessors is idempotent (all writers store the same depth), but formally needs a compare-exchange from "undecided" so that exactly one thread inserts the position into the next frontier. The next frontier is a bitset written with `atomic fetch_or`. `for_each` runs over the set bits of the current frontier. This is the structure of the user's `egdb_fast.cpp` (16-bit `cnt` arrays).
    - Verification: a gather over the full range with an error-count reduction, trivially `par` (`transform_reduce`).

---

## 2000-ICGA-Nalimov-Haworth-Heinz.pdf

1. **Citation**: E.V. Nalimov, G.McC. Haworth, E.A. Heinz (2000). "Space-efficient Indexing of Chess Endgame Tables." *ICGA Journal* 23(3), pp. 148–162.
2. **Scope**: All 3–5-man chess DTM EGTs (145 EGTs, "some 30" 6-man). The scheme was used by ten WCCC 1999 entrants. Total index: **30,612,186,969 elements** (wtm 14,702,353,093; btm 15,909,833,876). Alternatives: Heinz 33.6×10^9 (+9.81%), Thompson 51.9×10^9 (+69.45%), Edwards 74.1×10^9 (+142.04%). This is essentially the same text as the 2001 ACG-9 paper below. Differences found in the 2000 version: (a) a second ordering for like-men ranking and a "succinct formula" from a referee, see item 4; (b) an appendix of studies with "esoteric force" (Troitzky's 5 dark-square Bishops, Troitzky's KNNNNKQ, Bondar's four losing Queens).
3. **Metric**: DTM, with wtm and btm and both 1-0 and 0-1 wins stored. Bit width not stated. **[background, not verified from PDF]** Nalimov entries are 1 byte, with 16-bit versions for very deep 6-man tables (BTH-2005 confirms that depth >126 needed 16-bit entries in the NBT code).
4. **Indexing** (the core):
   - Placement order: stmK, sntmK, stm men (Q-R-B-N-P), then sntm men (Q-R-B-N-P).
   - K–K pairs explicit: **462** (pawnless) or **1806** (with Pawns) index subranges, each an n-space. Squares are numbered a1…h8 over the "available" ones.
   - For type i: q_i available squares (depending on stm, King squares and C_UC), k_i like men, d_i = C(q_i, k_i). Position index = ind_KK + Σ_i r_i × Π_{j>i} d_j (mixed radix).
   - Per-man 64×64 tables give each sntmK square's available-square numbering; the numbers are decremented once the stmK square is known. Tables take about **200 KB per 3-2 endgame and up to 350 KB per 4-1**.
   - Unblockable checks by the stm are excluded from the index, "the chief reason for the compactness".
   - Different types of stm men may *share* squares in the index, and Pawns may sit on pieces' squares. Such slots, and checks that should be blockable but are not, are marked broken at initialisation.
   - Like-men ranking: (i) "advance highest" ordering with an iterative algorithm; (ii) "retreat lowest" ordering with **r = Σ_j C(s_j, j)** for sorted squares s_1<…<s_k. This is the combinatorial number system, i.e. the *colex rank*, exactly what the user's `colex_rank` implements.
   - First and last broken entries of an addressable subrange are trimmed (rebasing the baseline for the first).
   - En passant: a **separate zone** after the main stm index (14 Pawn placements instead of 2,256; x2 and x3 must be empty). Castling would generalise to 2^5 = 32 feature-subset zones (the extract says "25", a lost superscript).
   - KQRK wtm example (Table 6): naive 16,777,216; Edwards 2,621,440; Thompson 1,892,352; Heinz 1,747,284; **Nalimov 1,500,276**.
   - Equal material: only the btm half is computed, and access flips colours.
   - Residual broken fractions remain large, for example KQQQK wtm 62.94% and KQQK wtm 48.17% (Table 14).
5. **Working storage during construction**: Not discussed beyond "initialisation phase" broken-marking.
6. **Passes**: Only the init pass is mentioned: broken marking, e.g. "if the sntmK is in check, their indexes are marked as broken during the initialisation phase". There is also a post-generation "separate self-consistency phase". For the generation loop see NWH-1999.
7. **Parallelism / memory access**: At probe time, placing stm men first clusters a searching side's probes. An LRU cache of EGT values gives "much better caching behaviour … particularly with parallel search on SMPs". Construction parallelism: not discussed.
8. **Draws, cycles, special rules**: En passant is encoded and castling is not. The 50-move rule is ignored (pure DTM).
9. **Final storage / compression**: Files are compressed in **8 KB blocks** ("common sequences and Huffman coding"), decompressed in memory. The block size is chosen for runtime performance, not space.
10. **Verification**: Nalimov's post-generation self-consistency phase. Mutual-zugzwang counts (=/1-0, 0-1/=, 0-1/1-0) match Wirth's independent EGTs for all 2–5-man endgames. The maxDTM figures were confirmed by Conrady, Karrer and Rasmussen, with two corrections.
11. **Notable tricks**: (a) K–K-pair outer subranges with an offset table. (b) Mixed-radix n-space per subrange. (c) Colex ranking of like men. (d) Excluding unblockable checks, i.e. position-dependent per-man domains, which have a draughts analogue in position-dependent square domains. (e) Separate index zones for rare flags. (f) Accepting some broken slots in exchange for a simple, fast index. (g) Trimming leading and trailing broken entries. (h) Future work: placing Pawns first (Karrer 2000) and accounting for prior stm men.
12. **Mapping to a single `for_each`**: The broken-marking init is a race-free gather over the full range. Because subranges have different dimensions, a flat `for_each` over [0, N) needs `unrank(i)` to binary-search the subrange offset table (cheap, read-only). Alternatively, nest: `for_each(par, KK pairs)` with an inner sequential loop over the n-space, which gives coarser load balance. Broken entries should be skipped cheaply in every later pass. A one-bit-per-slot "valid" bitset is the natural frontier mask.

---

## 2001-ACC-09-Nalimov-Haworth-Heinz.pdf

1. **Citation**: E.V. Nalimov, G.McC. Haworth, E.A. Heinz (2001). "Space-Efficient Indexing of Endgame Tables for Chess." In *Advances in Computer Games 9* (eds. van den Herik, Monien), IKAT Maastricht, pp. 93–113.
2. **Scope**: Same content as the 2000 ICGA paper: all 3–5-man DTM EGTs and the Nalimov index. This version includes the full appendix tables: Table 8 (maxDTM per endgame, e.g. KPPKP 127 plies 1-0 wtm, KBNKN 107, KNNKP 115), Tables 9–13 (index sizes compared with Thompson/Edwards/Heinz per endgame), and Tables 14a/b (broken counts per endgame).
3. **Metric**: DTM, wtm and btm, 1-0 and 0-1.
4. **Indexing**: As in the 2000 entry. Additional detail from Table 2: a 22-row constraint matrix (C_8, C_KK, C_TE, C_KKnP = 462, C_ad, C_KKP = 1806, C_P, C_EP, C_LM, C_S1..S7 square-sharing constraints, C_UC unblockable checks, C_F/C_L first/last index not broken), showing which of Thompson, Edwards, Heinz, Wirth and Nalimov used each. Nalimov uses the most, including C_S3 (no K+Pawn on one square), C_UC and C_L. Like-men ranking: only the "advance highest" algorithm, not the succinct formula. Worked example: wtm KRRNKP with wK a1 and bK c2 gives 1653 (Rooks) × 57 (N) × 47 (P) = 4,428,387 entries in that subrange.
5. **Working storage during construction**: Not discussed.
6. **Passes**: Not discussed beyond "initialisation phase" broken marking and a "separate self-consistency phase".
7. **Parallelism / memory access**: LRU probe cache; stm-first placement for access clustering.
8. **Draws, cycles, special rules**: En passant zone, no castling, no 50-move rule.
9. **Final storage / compression**: 8 KB blocks with Huffman plus common-sequence coding.
10. **Verification**: Self-consistency phase; zugzwang-count agreement with Wirth; maxDTM figures confirmed independently.
11. **Notable tricks**: As in the 2000 entry. Tables 14a/b are useful as a quantitative warning: a fast but leaky index can leave 25–60% broken slots in Q-heavy endgames, which costs both construction time and compressed size.
12. **Mapping to a single `for_each`**: As in the 2000 entry. The useful lesson for the `for_each` design is that broken slots are cheap to skip in a full-range gather but expensive if every pass must re-run move generation to discover them. Mark them once in a `valid` bitset, and have each full-range pass iterate over the set bits of `valid & ~decided`.

---

## 2005-TCS-Bourzutschky-Tamplin-Haworth.pdf

1. **Citation**: M.S. Bourzutschky, J.A. Tamplin, G.McC. Haworth (2005). "Chess endgames: 6-man data and strategy." *Theoretical Computer Science* 349(2), pp. 140–157. doi:10.1016/j.tcs.2005.09.043.
2. **Scope**: Chess. Complete 3–5-man DTC/DTZ/DTZ50 EGTs (earlier work) and all **6-man pawnless** endgames (3-3 and 4-2) to DTC (≡ DTZ, since there are no Pawns) and DTZ50, built by the "NBT" code (Nalimov → Bourzutschky → Tamplin) on single- and multi-processor UNIX systems. About 0.6 TB of EGT data was nearly lost in a RAID crash and recovered thanks to MD5 sums. Extreme example: KRNKNN has maxDTC 243 moves. The 3–6-man DTM set ("EMs") will be about 1.45 TB.
3. **Metric** (central to the paper):
   - **DTC**: depth to conversion, i.e. a change of force and/or mate.
   - **DTZ**: depth to move-count zeroing, i.e. a Pawn push, force change and/or mate ("when a move-counter is set to zero again"). For pawnless endgames DTZ ≡ DTC.
   - **DTZ_k / DTZ50**: dtz_k = dtz unless a k-move rule allowing a draw claim sets the value to draw. DTZ50 ≥ DTZ.
   - **DTM_k**: DTM under a k-move rule (named but not detailed).
   - **DTR** (defined here): "dtr = the least k for which a position is won or lost, given a k-move drawing rule", with 0 ≤ dtr ≤ dtm. A DTR EGT lets an attacker win any position winnable under *any* k-move rule, which makes DTZ50 tables unnecessary. Because successive positions on a winning line can share a DTR value, a companion metric **DTZ_R** (dtz while minimaxing dtr) is needed "while generating and using the DTR EGTs".
   - Depth is measured "consistently in winner's moves". The code does not assume the winner effects the conversion, and does not let the loser make a voluntary but unavailing capture.
   - Size comparison (Table 2, relative to DTM): 3–5-man DTC 62.13%, DTZ 50.24%, DTZ50 delta-file 0.53%. 6-man pawnless DTC = DTZ 56.37%. 3–6-man DTZ 56.17%, DTZ + EdZ50Z 57.28%.
   - Encoding: up to 1 byte. Depths >126 required **16-bit entries**.
4. **Indexing**: The Nalimov index and file format, inherited.
5. **Working storage during construction**: Not detailed. The code needed support for virtual stores and files above 2 GB and for counts above 2^31−1.
6. **Passes** (only what the paper states, plus marked inference):
   - The NBT code generalises Nalimov's DTM code by "generalising some DTM-specific aspects of the algorithm, as well as making the obvious changes to the iterative formula for deriving depth". It "retains the efficiencies of the DTM-code while requiring **maxDTC rather than maxDTM cycles**" (e.g. KQBNKN: maxDTM 107 versus maxDTC 6). This tells us the iteration count of Nalimov's loop is tied to the maximum depth, i.e. one cycle per depth level, at least in NBT.
   - DTZ with Pawns: "Because EGT generation to the DTZ metric has not been implemented generically as a sequence of 'fixed pawn structure' sub-EGT generations, this is not so for DTZ(k) computations". In other words, the cycle saving does not carry over to DTZ in Pawn endgames. **[inference]** The ideal DTZ build partitions by Pawn structure and builds each sub-table separately, with Pawn moves treated like captures as zeroing moves into already-solved sub-tables, i.e. conversion.
   - **[inference]** How each metric changes the passes:
     - **DTC/DTZ**: conversion moves (captures, promotions; for DTZ also Pawn pushes) seed depth 1 *regardless of the sub-table's depth*. Only win/loss is read from the smaller table, so the init/conversion pass writes "win in 1" / "lose in 1 (forced)" and the depth recursion restarts from 0 at each zeroing move.
     - **DTZ50**: in the same pass structure, any position with dtz > 100 plies becomes a draw. With pure DTZ50 (pc = 0 at entry) this is a cap of 100 plies on the iteration (`for ply in 1..100`). Remaining undecided positions are draws. Wins with DTZ > 50 moves become draws, and positions whose only quick wins vanish get *delayed* deeper wins (DTZ50 > DTZ).
     - **DTR**: a two-metric search ("two metrics are used in parallel and the process is not as efficient as that for DTC, DTM and potentially DTZ"). "The computation of DTR … remains a future challenge". No algorithm is given.
7. **Parallelism / memory access**: Multi-processor runs. Tamplin had to "synchronise multiple processes more rigorously", but no further details are given.
8. **Draws, cycles, special rules**: The core subject is the 50-move rule:
   - 63 of 135 6-man pawnless endgames are affected. Examples: KBBKNN 1-0 has 50.15% (wtm) and 70.98% (btm) of nominal wins frustrated; KBBBKN has 11.28%/26.80% of wins delayed.
   - maxDTZ50 > maxDTZ only for KQNKBB, KQQKBB, KBBBKN and KBNNKN.
   - 50-move-rule draws are "frustrated wins". Undecided positions are draws.
   - Strategy section: SM−, SC−, SZ− (and combinations) can fail to win 50-move-winnable positions. SZ50− always succeeds. Compound strategies such as SZ0Z50−Z− are proposed. Several formulas are garbled in the extract.
9. **Final storage / compression**: Nalimov compression format. **EdZ50Z delta table**: since DTZ50 ≥ DTZ, store only 0 (same as DTZ, or DTZ>50 which already implies a draw), 1 ("DTZ ≤ 50 but new draw"), or (DTZ50−DTZ)+1 (delayed). This is null for many endgames and 0.53% of DTM size for 3–5-man. A DTZ_R/DTR set could be derived in the same way (EdZ_R Z, EdRZ_R).
10. **Verification**:
    - MD5 signature on every file at creation.
    - Ex verified by "Nalimov's standard test" (the successor-consistency forward pass).
    - **Cross-metric consistency**: counts of all positions identical to the predicted index ranges, theoretical values identical between EM and EZ with dtm ≥ dtz, and between EZ50 and EZ with dtz50 ≥ dtz or "EZ win/loss = EZ50 draw".
    - DTZ statistics compared with Stiller and Thompson. Published DTZ lines replayed.
    - Two discrepancies with Thompson explained: KBNNKN 27 → 28, with forced Black conversion. Legal-but-unreachable positions affect statistics (e.g. an impossible double check in KQQKNN).
    - Recommendation: **self-identifying files**, a header with author, code version, metric, completion degree, date and compression, because "manual file-management can be a source of error".
11. **Notable tricks**: (a) One generator with a metric-specific depth recursion: DTM/DTC/DTZ/DTZ_k differ only in what the conversion step writes and when depth restarts. (b) The delta-table idea: store a derived metric as a tiny diff against a base metric. (c) Cross-metric invariants (dtm ≥ dtz, dtz50 ≥ dtz, same W/L sets) as cheap global checks. (d) Iteration count bounded by max depth in the chosen metric; DTC/DTZ shrink it drastically. **[inference]** In draughts, DTZ-like zeroing at man moves and captures (both irreversible) would make the per-slice loop very short.
12. **Mapping to a single `for_each`**:
    - Conversion/init: a gather over the full range. For DTC/DTZ it writes only "win/loss in 1" (no sub-table depth needed), so it is race-free under `par`.
    - Per-ply passes: same as NWH-1999. Either the level-synchronous pull gather (race-free with relaxed atomics) or the frontier push scatter (atomics on counters and a bitset). For DTZ50 the outer loop is simply capped at 100 plies, and the final "undecided → draw" step is a full-range map.
    - The EdZ50Z delta is a pure map `delta[i] = f(dtz[i], dtz50[i])` over the full range, race-free.
    - Cross-metric checks: a `transform_reduce(par)` over the full range.
    - **[inference]** DTR does *not* fit one `for_each` per ply. It needs either an outer loop over k (rebuild DTZ_k for each k, or binary search) or a state per position that is a function of the remaining move budget (see HH-2015). Each inner pass can still be a gather or scatter, but there are k_max × depth passes.

---

## 2015-ICGA-Huntington-Haworth.pdf

1. **Citation**: G. Huntington, G.McC. Haworth (2015). "Depth to Mate and the 50-Move Rule." *ICGA Journal* 38(2), pp. 93–98 (a Note). The reference list inside the paper cites itself as 38(1), pp. 50–55, and the CentAUR cover gives 38(2), 93–98.
2. **Scope**: Chess, all sub-6-man (5-man) endgames plus some 6-man ones (KBBKNN, KNNKNP, KNNKPP), building **DTM50** EGTs that are valid for *any* ply count pc (0..99). Implemented in **Haskell**. Hardware: not stated.
3. **Metric**:
   - **DTM50,pc**: DTM moderated by the FIDE 50-move rule *and* the current ply count pc, with plies remaining pr ≡ 100 − pc. "A position's dtm50,pc may increase as pc increases until the win becomes a … '50mr-draw'". Even a mate in 2 plies is frustrated if 99 plies have elapsed. These are "the first [EGTs] to provide depths for any value of pc".
   - 5-way value scale: ±2 unconditional 50mr win/loss, ±1 50mr-draw (win under DTM but not under the rule), 0 draw.
   - DTZ is defined as "minimaxed depth to the zeroing of the ply-count, i.e., to a pawn-push, capture and/or mate".
   - Storage: an array whose cells store "the possible mate lengths for various ranges of PC", plus an "overflow" hash table for values that do not fit a cell. Cell size is fixed in advance: too small means heavy overflow use, too large wastes memory. KNNKP has up to **25 different mate depths** for one position depending on pc.
4. **Indexing**: Not discussed.
5. **Working storage during construction**: The same array-of-ranges plus overflow hash as above. Wu & Beal (cited) needed only **2 bits per position** in pure unmove mode by deferring all "mate in m+1" until every "mate in m" is found, so the ply is implicit in the cycle number. That is 4–8× less memory than 1–2-byte depths.
6. **Passes, in order** (from §2, the clearest statement in this group of the pull-versus-push trade-off):
   - General principles: "(a) successor endgames' EGTs are computed first, and (b) a position can be given a depth temporarily when one of its immediate successors has been given a depth, but definitively only after enough of its successors have been given their definitive depth."
   - **init**: identify mated (0-ply) positions.
   - Then: "Positions which can be assigned a depth may be found by repeated, linear sweeps of the whole endgame and the first sweeps efficiently net many positions. Shortly though, the more selective and efficient method is to 'unmove' from the 'frontier' of positions which have just been given a depth in the last cycle." So the paper recommends a **hybrid: full-range forward sweeps (pull) for the first, dense levels, then frontier-based retrograde (push)** once the frontier becomes sparse.
   - Nalimov: "linear sweeps rather than … 'unmove' … giving a position a depth at the earliest opportunity but lowering it later as required". Many re-lowerings happened in KNNKP.
   - Wu & Beal: unmove only, level-synchronous.
   - **Huntington (DTM50)**: "reverts to the principle of assigning depths as soon as possible. **Each algorithm cycle corresponds to a specific number, pr, of ply remaining: pr effectively increases from 0 to 100** during the computation. Lower values of DTM50 become possible when more ply are available … By noting on what cycle a DTM50 value is set, the various ply-ranges corresponding to a specific dtm50 can be identified for each chess position."
   - **[inference]** Pass structure for DTM50: an outer loop over the budget pr = 1..100. Inside it, a position's value at budget pr depends on (i) successors via *non-zeroing* moves at budget pr−1, and (ii) successors via *zeroing* moves (captures, Pawn pushes, which reset the counter) at full budget, i.e. a conversion lookup into fully solved tables. Zeroing successors that stay within the same endgame (Pawn pushes) need the pc = 0 values of other Pawn-structure partitions, which also argues for partitioning by Pawn structure. The per-position state is a monotone step function pr → dtm, hence the variable-length "ranges" cell.
   - Losing-side technique: not stated.
7. **Parallelism / memory access**: Not discussed. Haskell pitfalls are noted: memory leaks, and "small changes can have a large effect on performance".
8. **Draws, cycles, special rules**: The 50-move rule and ply count are the subject. Sub-5-man is unaffected because maxDTM < 100 ply. maxDTM50,pc=0 can be *greater* than maxDTM (KPPKP, KPPKQ, KQPKQ, …) when a deeper but quicker-to-zero line is needed, or *smaller* (KNNKP, KBBKN, …) when deep wins become draws. The 50mr's impact grows with the number of men (maxDTZ above 80, 240 and 510 moves for s6m, s7m and s8m chess). Positions undecided at pr = 100 are draws.
9. **Final storage / compression**: Array cells plus overflow hash. Datasets are published (PGN and spreadsheets).
10. **Verification**: The authors use a functional language for confidence in correctness, citing data-integrity concerns (Hurd & Haworth 2010, "Data assurance in opaque computations"). maxDTZ50 figures inferred from the DTM50 EGTs "confirm previous figures (Tamplin and Haworth, 2004)", which is a cross-metric check.
11. **Notable tricks**: (a) **Hybrid sweep-then-frontier** (pull while the frontier is dense, push once sparse). This is the direction-optimising BFS idea and fits `for_each` well (see 12). (b) Wu & Beal 2-bit, level-synchronous unmove: depth implicit in the cycle, so storage is just {undecided, win-frontier, loss-frontier, decided}. (c) Recording the cycle at which a value changes, to recover a whole budget-dependent profile in one build. (d) A fixed-width cell plus overflow hash for rare variable-length data.
12. **Mapping to a single `for_each`**:
    - **Dense phase (pull)**: `for_each(par, undecided-bitset)`. Each position reads successors (frozen values from earlier levels) and writes only itself: a gather, race-free if the level is synchronous.
    - **Sparse phase (push)**: `for_each(par, frontier-bitset)`. Unmove and write to predecessors: a scatter needing atomics (counter `fetch_sub`, CAS claim, atomic `fetch_or` into the next-frontier bitset).
    - The switch criterion can be |frontier| / |undecided|. **[inference]** The pull variant of the losing-side test does not need counters: an undecided position is "lost in n" iff all successors are decided wins with max depth n−1. That costs a full successor scan per candidate, the grandfather/verification flavour applied as a gather.
    - DTM50 with a pr loop: each pr step can be one gather over the full range writing a per-position "dtm at pr" (double-buffered: pr−1 → pr). Merging into the variable-length ranges cell, and especially inserting into the overflow hash, is *not* a clean per-element write: a concurrent hash table or a per-thread buffer merged afterwards is needed. A fixed-width "last-changed-at-pr" encoding avoids that.

---

## 2016-CMC-Zakharov-Malkovskii-Shchukin.pdf

1. **Citation**: V.B. Zakharov, M.G. Mal'kovskii, V.Yu. Shchukin (2016). "Compression of Underdetermined Data in a 7-Piece Chess Table." *Moscow University Computational Mathematics and Cybernetics* 40(1), pp. 47–52 (translated from *Vestnik MGU, Ser. 15*, 2016, No. 1, pp. 46–54). DOI 10.3103/S0278641916010076.
2. **Scope**: Chess, the **Lomonosov 7-piece tables** (2012, MSU). Debugging runs were on an IBM Blue Gene/P and the main computation on the *Lomonosov* supercomputer. More than **1 PB uncompressed, more than 100 TB compressed**. **This paper is about compression and random-access format, not construction.** "The algorithm for obtaining the tables was described in [1, 2]" (Zakharov & Makhnychev 2010, "A retroanalysis algorithm for supercomputer systems on the example of playing chess"; and 2013, "Creating tables of chess 7-piece endgames on the Lomonosov supercomputer"). Those papers are **not** in this group, so the construction passes cannot be extracted from this PDF. The only construction-related statements are that "the mechanism of data exchange between the supercomputer units enabled us to minimize the downtime when waiting for the data from other units", and that this was "the first time … an algorithm of retrograde analysis … was adapted for use in supercomputers".
3. **Metric**:
   - **Full tables**: DTM in plies, **2 bytes per value** (12 bits suffice for the worst endgames, but compression makes 12 versus 16 bits irrelevant). The full tables store moves, ≈ (d+1)/2, rather than plies d; the formula is partly garbled in the extract. Winner information is duplicated from the ternary tables.
   - **Ternary tables**: WDL (−1/0/1). Lookup is two-phase: ternary first, then the full table only if the result is not a draw.
   - **One-sided** tables (one side to move only) were considered; restoring the missing side costs about 10× in access speed.
   - No best moves are stored.
4. **Indexing**: The baseline formula is index = (((((p7·64 + p6)·64 + p5)…)·64 + p1). The real indexer has symmetry handling "described by thousands lines of program code" and "eliminated most illegal positions, something that had never been done before". Maximum indices are "several percent lower" than Nalimov's 6-man tables, but "further reduction … results in a drastic increase in the volume of computations when generating tables". The new format *simplified* the indexer and made the **order of pieces in the formula configurable**. This improves compression by 25–30% on average and by 2–3× between best and worst orders for some endgames, even though the uncompressed size grew. Illegal slots must stay in the array ("cannot simply remove such values … as this would upset the indexer's operation").
5. **Working storage during construction**: "Compression occurs in the main storage immediately after constructing the table section without recording uncompressed data to the disk." Construction-time format: 2 MB blocks with LZMA, since sequential reads dominate during construction. Bytes per position during construction and any bitmaps: not discussed.
6. **Passes**: Construction passes are **not discussed** (see item 2). Post-construction compression passes:
   1. **Underdetermined marking** (a full-range map). Ternary tables: illegal positions become `*` (any value). Wins achievable by a capture (a_w) become `*`, because they are restorable by probing capture successors (which have fewer pieces and are "two orders of magnitude faster" to probe). Draws where a capture draws (a_d) become `#` ∈ {−1, 0}, since a non-capture might still win. Full tables: draws and illegal positions become `*`. A win where some capture gives mate in n becomes "any value ≥ n", and a loss becomes "any value ≤ n". The decoder takes the min (wins) or max (losses) of the stored value and the capture-derived values.
   2. **Initial replacement**. For a run 'AB XX…X CD': if A = B and all X can be A, use A; else if C = D, use C; else B; else C; for runs longer than 5, use the globally most frequent value. For full tables, short (2–31) runs are filled with the most frequent symbol and longer runs are filled with the most frequent pairs.
   3. **Pair replacement** during RE-PAIR: an underdetermined pair whose specification matches the most frequent pair AB is replaced too.
   4. **Final replacement**: fill the remaining `*`/`#`, then re-run pairing.
   5. Huffman coding.
7. **Parallelism / memory access**: The construction ran on a distributed-memory supercomputer with inter-node data exchange (no details). For access: block size 16 KB costs +45% size compared with 2 MB blocks but makes random access usable. Speed grows roughly linearly as blocks shrink to 20 KB and keeps improving down to 1 KB. RE-PAIR with **one dictionary per file** allows **1 KB** blocks, and ternary blocks of **128 bytes compressed (~1 KB raw)**.
8. **Draws, cycles, special rules**: The 50-move rule is not discussed. Draws are stored only in the ternary tables; in the full tables they are don't-care.
9. **Final storage / compression**: The subject of the paper. Ternary 7-piece tables went from **15 TB to 7 TB**. Full 5-piece tables went from 5.82 to 4.83 GB (Alg. 2, 83%, access speed 80%). Full 6-piece: 952 to 740 GB (77%, 84%). 6-piece Lomonosov <0.8 TB versus Nalimov 2005 >1.1 TB with 8 KB blocks, about 30% smaller and "several times faster" to access. Conclusions: LZMA and other per-block-dictionary compressors are unsuitable below 16 KB; data reordering can matter multifold; underdetermined coding gives "tens of percent".
10. **Verification**: not discussed.
11. **Notable tricks** (directly relevant to draughts, where captures are mandatory and always leave the slice): (a) **Every capture position is a don't-care in storage.** In draughts the value of a position with a capture available is fully determined by smaller slices, which is stronger than in chess, where a capture only *bounds* the value. (b) Bounded don't-cares for DTM (≥ n / ≤ n). (c) A configurable piece order in the index to maximise run length. (d) A single shared dictionary with tiny blocks (RE-PAIR). (e) Two-tier probing: WDL first, depth second. (f) Compressing each slice in RAM right after construction.
12. **Mapping to a single `for_each`**: Construction cannot be mapped from this paper (not described). Post-processing: the underdetermined-marking pass is a **gather** over the full range (each position probes its own capture successors and writes only its own symbol), race-free under `par`. The user's code already keeps a per-position `flags` capture bit, so the pass reduces to `sym[i] = flags[i] ? DONT_CARE : val[i]`. Initial replacement reads neighbours within a run, which is a sequential scan but parallel across independent blocks (`for_each(par, blocks)`). RE-PAIR with a global dictionary is inherently sequential, or needs a global frequency reduction followed by parallel per-block substitution.

**[background, not verified from PDF]** The Lomonosov build (Zakharov & Makhnychev) is reported to be DTM with no 50-move rule, distributed over many nodes, with the table partitioned so that each node owns a slice and exchanges predecessor updates by message passing. I did not verify this from any PDF in this set.

---

## Group synthesis

### Distinct primitives introduced or documented in this group

1. **Legal-K–K-pair outer index** (Thompson 462; Heinz 3612 full-board; Nalimov 1806 a–d with Pawns), with per-pair offset tables. In draughts there are no kings-as-royalty, but the analogue is an outer index over a constrained "anchor" configuration with an offset table.
2. **Colex / combinatorial-number-system ranking of like men** (Heinz 1999-1 sorted pairs → NHH-2000 r = Σ C(s_j, j)) combined with **dynamic "subtract earlier-placed men below me" ranking** (Heinz/Nalimov). The user's `colex_rank(white)·C(N−w, b) + colex_rank(compress(black, white))` is exactly this lineage.
3. **Mixed-radix n-space subranges with position-dependent domains** (Nalimov: unblockable-check exclusion, a metatable of sub-indexes). Trades a smaller index for more complex `unrank`.
4. **Broken slots as a first-class state**: mark illegal, redundant or symmetric-duplicate slots in init (the "higher-indexed duplicate is broken" rule), trim leading and trailing broken entries, accept 25–60% residual broken in exchange for a fast index (NHH Table 14).
5. **Separate index zones for rare flags** (en passant, castling) and reuse of illegal slots for them (Heinz).
6. **One side to move for colour-symmetric material** (Heinz; Nalimov "btm half only").
7. **Two construction schemes sharing one skeleton** (NWH-1999):
   - (a) **Nalimov full-range forward sweep (pull)**, assigning early and lowering later (chaotic relaxation), with cycles bounded by max depth (BTH 2005: "maxDTC rather than maxDTM cycles").
   - (b) **Wirth/Gasser RetroEngine frontier push with outcounting** (Thompson 1986 lineage), successor count kept in the value cell, frontier = "changed in last iteration", disk-streamable.
   - (c) **Wu & Beal level-synchronous unmove with 2 bits per position** (cited in HH-2015).
   - (d) **Hybrid: dense sweeps first, frontier unmove later** (HH-2015).
8. **Metric family and how it changes the passes** (NWH-1999 → BTH-2005 → HH-2015): DTM (sub-table depth +1 at conversion) → DTC/DTF = DTZ (depth restarts at each conversion/zeroing move; fewer cycles; needs Pawn-structure partitioning to keep that advantage with Pawns) → DTZ50 (cap at 100 plies, undecided → draw) → DTR (least k; needs two metrics in parallel; unsolved in 2005) → DTM50,pc (outer loop over plies-remaining 0..100, with a per-position step function of pr).
9. **Delta tables between metrics** (EdZ50Z: 0 / 1 / Δ+1, 0.53% of DTM size).
10. **Verification primitives**: a parallelisable forward successor-consistency pass (checks the move/unmove inverse property); independent second implementation in a second metric; cross-metric invariants (dtm ≥ dtz, dtz50 ≥ dtz, equal W/L sets, equal mutual-zugzwang counts); position counts equal to predicted index ranges; MD5 on every file; self-identifying file headers.
11. **Storage-side primitives**: knowledgeable 0/1/2/4/8-bit encodings with recognizer fallbacks (Heinz); WDL-first two-tier probing; **underdetermined / don't-care values for capture-resolvable and illegal positions** (Chinook → Heinz-cited → Zakharov's bounded ≥n / ≤n); index-order tuning for run length; small blocks with a single shared dictionary (RE-PAIR); compress in RAM right after building a slice.

### Lineage

- Thompson 1986 (462 K–K, outcounting retrograde, DTC) → Edwards 1994/95 (DTM, both sides, flat vectors, "broken" term) → **Heinz 1999-1** (3612, colex pairs, dynamic ranking, ep remap) → **Nalimov 1998–2001** (1806, C_UC, n-space metatable, ep zone; DTM by full-range sweeps; 8 KB Huffman blocks) → NBT code (**BTH 2005**: same engine, DTC/DTZ/DTZ50, 16-bit depths) → **HH 2015** (DTM50 for all pc, Haskell, sweep-then-unmove).
- Gasser's RetroEngine (Nine Men's Morris) → Wirth (KPPKP, 4Q DTC; frontier plus outcount) as the push counterpart to Nalimov's pull in **NWH 1999**.
- Chinook's capture-purged storage → Heinz knowledgeable encoding → **Zakharov 2016** underdetermined RE-PAIR compression on top of the Lomonosov 7-man build (construction in Zakharov & Makhnychev 2010/2013, not in this set).

### For the user's "one `for_each(policy, …)` per pass" design: summary

| Pass | Range | Gather/scatter | `par`-safe as is? |
|---|---|---|---|
| Broken/illegal marking, init (terminal), conversion via smaller slices | full index | gather | yes |
| DTC/DTZ conversion (write only win/loss-in-1) | full index | gather | yes |
| Nalimov in-place relaxation sweep | full index | gather, but reads neighbours being written | formally no (data race); yes if made level-synchronous with `atomic_ref` relaxed bytes or a double buffer |
| Level-synchronous pull for ply n (win: some succ = L(n−1); loss: all succ ∈ W(≤n−1)) | undecided bitset | gather | yes |
| Wirth push: losses → mark predecessors won | frontier bitset | scatter (idempotent) | needs CAS claim + atomic `fetch_or` into next frontier |
| Wirth push: wins → decrement predecessor counters | frontier bitset | scatter | needs `atomic_ref<uint16_t>::fetch_sub`; thread seeing 1→0 claims |
| Undecided → draw (and DTZ50 cap) | full index | map | yes |
| Verification (successor consistency, cross-metric invariants) | full index | gather + reduction | yes (`transform_reduce`) |
| Delta table / don't-care marking for compression | full index | map | yes |
| Packing sub-byte values | output words | gather per word | yes if partitioned by output word |
| DTR (least k) / DTM50 with variable-length per-position profile | k × ply, or pr loop | gather per step, but variable-length output | per-step gathers yes; overflow hash/cell merge needs a concurrent structure or a per-thread buffer |
| RE-PAIR global dictionary | whole file | sequential | no (only per-block substitution parallelises) |
