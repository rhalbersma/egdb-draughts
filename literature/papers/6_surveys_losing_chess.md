# Group 6: surveys on solving games, and losing chess (antichess)

All five PDFs were read in full through the Drive connector. None was empty or cut off. Two of the three "surveys" say little about the internals of retrograde analysis:
- **Uiterwijk–Herik 2000** is about the advantage of moving first and contains no retrograde content at all.
- **Herik–Uiterwijk–Rijswijck 2002** gives a taxonomy and lists games solved with databases, but describes no algorithm.

The Kishimoto–Müller chapter has a dedicated "Endgame Databases" section with an explicit 5-step algorithm. That section is covered in detail below.

The two Watkins notes are short (5 pages and 3 pages). They describe how tablebases were *used* inside a proof-number proof, and only a few lines about how the tablebases were *built*.

Statements marked **[background, not verified from PDF]** come from general knowledge, not from these files.

---

## 2000-IS-Uiterwijk-Herik.pdf

1. **Citation**: J.W.H.M. Uiterwijk, H.J. van den Herik, "The advantage of the initiative", *Information Sciences* 122 (2000) 43–58.
2. **Scope**:
   - The paper asks whether moving first ("the initiative") decides the outcome of two-player zero-sum games with perfect information.
   - Games solved for the paper:
     - mnk-games (k-in-a-row), with the program TTTTTT;
     - over 200 rectangular Domineering boards, with the program DOMI.
   - Its Table 2 lists known game values: Connect-Four 1, Qubic 1, Nine Men's Morris 0 (draw), 6×6 Othello 2, Go-Moku 1, and m×m Domineering. Here 1 is a first-player win, 2 a second-player win and 0 a draw.
   - Hardware: not discussed.
3. **Metric**:
   - Only game-theoretic values (first-player win, second-player win, draw).
   - It restates Singmaster's partition of positions:
     - **P-positions**: the previous player can force a win;
     - **N-positions**: the next player can force a win.
   - The win rule is "a position is N if *one* move leads to a P-position, and P if *all* moves lead to N-positions". This is the WDL recurrence that retrograde analysis implements, although the paper does not connect the two.
   - It defines **zugzwang** and **reciprocal (mutual) zugzwang**, quoting Nunn.
   - No distance metric.
4. **Indexing**: not discussed.
5. **Working storage during construction**: not discussed. The solvers are forward αβ searches with transposition tables. DOMI uses a "two-level" replacement scheme: each entry has two slots, the newest position is always stored, and the slot with the smaller subtree (node count) is overwritten.
6. **Passes, in order**: not discussed. There is no retrograde analysis in this paper. Nine Men's Morris appears only as a citation to Gasser.
7. **Parallelism / memory access**: not discussed.
8. **Draws, cycles and special rules**: Domineering has no draws. The remarks on draws concern mnk-games, where draws are shown with Hales–Jewett pairing strategies. Nothing is said about cycles.
9. **Final storage / compression**: not discussed.
10. **Verification**: Domineering results are checked against Berlekamp et al.'s published analyses.
11. **Notable tricks worth borrowing**:
    - The P/N recurrence as a compact statement of WDL.
    - Reciprocal zugzwang as a database-mining query: positions whose value flips when the side to move is swapped. A one-byte-per-position-per-side layout makes this a cheap map over the index range.
12. **Mapping to a single for_each per pass**: not applicable, because there are no construction passes. A "find mutual zugzwangs" query would be a race-free gather over the full index range: read `v[wtm][i]` and `v[btm][i]` and write a flag bit for `i`.

---

## 2002-AI-Herik-Uiterwijk-Rijswijck.pdf

1. **Citation**: H.J. van den Herik, J.W.H.M. Uiterwijk, J. van Rijswijck, "Games solved: Now and in the future", *Artificial Intelligence* 134 (2002) 277–311.
2. **Scope**: A survey of solved two-player zero-sum games with perfect information. The content relevant to databases:
   - **Nine Men's Morris** (Gasser 1995): all 28 middle-game and endgame databases w-b, with 3 ≤ w, b ≤ 9, totalling 7,673,759,269 states after removing unreachable and symmetric positions. The databases were "independently checked". An 18-ply forward search from the start position, probing the 9-9, 9-8 and 8-8 databases, then proved a draw.
   - **Awari**: a 13-stone database (LITHIDION) at first, then 35 stones [van der Goot; Lincke–Marzetta], then 38. A 39-stone database was under construction. The 36-stone database shows that Awari with 3 stones per pit is a draw.
   - **Kalah(m,n)** up to 6 pits and 5 stones: weakly solved by combining endgame databases with search. Kalah(6,4) is a first-player win.
   - **Checkers (CHINOOK)**:
     - endgame databases for all positions with ≤ 8 pieces: 443,748,401,247 positions, compressed into 6 GB with real-time decompression;
     - a "middle-game database" of positions solved during play (as many as 22 pieces);
     - deep searches to verify opening lines.
   - **Chess**:
     - Thompson built all important 5-piece endgames by retrograde analysis;
     - Stiller used parallel construction;
     - Nalimov-style indexing [82: Nalimov, Haworth, Heinz, "Space-efficient indexing of chess endgame tables"];
     - several 6-piece endgames existed by the late 1990s;
     - the longest distance-to-mate known then was 262 moves (KRNKNN).
   - **Xiang Qi**: 151 endgame databases [Fang–Hsu–Hsu; Wu–Beal].
   - Complexity table (state space / game tree): Draughts 10^30 / 10^54, Checkers 10^21 / 10^31, Nine Men's Morris 10^10 / 10^50, Awari ≈ 10^12 / 10^32.
   - Bound for solvability by complete enumeration: 10^11 in 1994, projected by Moore's law to about 3×10^12.
   - Hardware: not discussed.
3. **Metric**: Retrograde analysis is defined as storing "for each position … the number of moves towards the best reachable goal".
   - In chess this is moves to mate *or conversion*.
   - "Checkers databases sometimes only contain an indication of won, drawn, or lost."
   - Perfect play: the winner minimises the distance, the loser maximises it.
   - Named statistics: **max-to-mate, max-to-conversion, max-to-zeroing-move, max-to-the-rule** [ref 51]. These correspond to DTM, DTC, DTZ and DTR.
   - It recounts the 50-move rule history: the rule was extended for some endgames, then reverted to 50 moves after databases revealed wins hundreds of moves long.
4. **Indexing**: Only a citation to Nalimov et al.'s space-efficient indexing for chess.
5. **Working storage during construction**: not discussed. It cites Lincke–Marzetta, "Large endgame databases with limited memory space", for Awari.
6. **Passes, in order**: not discussed, beyond "a database is constructed by starting in terminal positions and then working backwards" [Herik–Herschberg 1985].
7. **Parallelism / memory access**: Stiller's parallel chess construction is mentioned, with no details.
8. **Draws, cycles and special rules**:
   - Nine Men's Morris: a repeated position is a draw.
   - The 50-move rule history above.
   - Nothing on how draws or cycles are handled algorithmically.
9. **Final storage / compression**: CHINOOK: 8 pieces in 6 GB, decompressed in real time during search.
10. **Verification**: Gasser's databases were "independently checked". There are no details.
11. **Notable tricks worth borrowing**:
    - **Database-guided forward search** to close the gap between the largest database and the start position (Gasser's 18-ply search; CHINOOK).
    - A **middle-game database** that caches positions solved by search.
    - Real-time decompression so that search can probe large databases.
    - Taxonomy: **convergent vs divergent** games (only convergent games admit endgame databases). Games are placed on two axes, state-space complexity and game-tree complexity, giving categories 1–4. Retrograde analysis and brute force fit category 2: low state space, high game tree.
12. **Mapping to a single for_each per pass**: not applicable, because no passes are described. The forward search over databases is a probe (a read-only gather) and has no construction pass.

---

## 2015-HBDGET-Kishimoto-Muller.pdf

1. **Citation**: A. Kishimoto, M. Müller, "Game Solvers", in R. Nakatsu et al. (eds.), *Handbook of Digital Games and Entertainment Technologies*, Springer Singapore, 2015, DOI 10.1007/978-981-4560-52-8_35-1.
2. **Scope**: A general chapter.
   - Definitions:
     - levels of solving (Allis): **ultra-weakly, weakly, strongly solved**;
     - AND/OR trees, proof and disproof trees.
   - Forward search: αβ, PNS, df-pn, PN², SmallTreeGC, overestimation, Graph-History Interaction (GHI) and the infinite-loop problem.
   - A 1-page section on endgame databases.
   - Solved games: Awari (Romein–Bal 2003, *strongly* solved; the database holds exact scores of all legal positions), checkers (Schaeffer et al. 2007), Connect-Four, Go-Moku, small-board Go, Othello, Hex and Amazons.
   - Chess databases exist for most 3–7-piece positions.
   - Hardware: not discussed.
3. **Metric**:
   - The algorithm is stated for WDL: "win, loss, or draw, and a repeated position is defined to be a draw by the rule of that game".
   - In the AND/OR formalism a node is win, loss or unknown. "For the sake of simplicity, the value of draw is defined to be the value of loss if possible game outcomes are not explicitly defined." This is the usual *draw-as-loss binarisation* for proof search.
   - The proof and disproof numbers of terminal nodes are given: (0, ∞) if proven, (∞, 0) if disproven.
   - No distance metric is discussed.
4. **Indexing**: not discussed. Step 1 below says only "a set containing all positions, that may include unreachable ones from the root".
5. **Working storage during construction**:
   - Retrograde analysis "requires large CPU and storage resources … large amounts of both main memory and hard disk space".
   - **Paging**: choose which part of the database stays in RAM, like an operating system's virtual memory (Lake et al. 1994; Romein–Bal 2003).
   - Memory reduction "at the cost of a small computational overhead" (Lincke 2002; Romein–Bal 2003). No details are given.
6. **Passes, in order**: The chapter's algorithm, verbatim in substance:
   1. Let S be the set of all positions, possibly including unreachable ones.
   2. **init**: assign the rule-determined value to all terminal nodes, and *unknown* to all others.
   3. **forward / successor scan ("pull")**: "For each node n ∈ S with value unknown, check if n's value can be determined from its children." Example given: an OR node is a win if at least one child is a win. The dual rule is implied but not written: a node is a loss if all children are losses for the side to move.
   4. **Repeat step 3 until the number of nodes with value unknown no longer decreases.** This is a fixpoint over full sweeps.
   5. **Mark every node still unknown as a draw.** Such a node is either unreachable or neither side can force a result.

   Notes on the algorithm:
   - **Losing side**: decided by **verification**. The node is re-examined, and declared lost only when *all* its children are known wins for the opponent. There are no counters and no predecessor generation.
   - **Coverage**: every pass runs over the full set S, restricted to the nodes still unknown. There is no frontier.
   - **Alternation**: there is only one pass type, repeated until nothing changes.
   - There is no conversion pass, and no predecessor ("push") variant. The chapter notes only that retrograde analysis is a "backward search … starting from terminal nodes and progressing toward the root".
7. **Parallelism / memory access**:
   - **Romein–Bal 2003**: "an asynchronous distributed-memory parallel algorithm that overlaps database computation and processor communication via network".
   - **Lake et al. 1994**: "split the search space into a set of small slices that can be solved easily and independently".
   - Paging and disk-I/O reduction as in item 5.
8. **Draws, cycles and special rules**:
   - Repetition is a draw by assumption, so cycles resolve to draws through the final "unknown → draw" step.
   - For *forward* search over graphs the chapter names three problems:
     - **overestimation** of proof numbers in DAGs (double counting);
     - **GHI** (Palay 1983; Campbell 1985): a value that depends on path history is stored as if it depended on the position only;
     - **the infinite-loop problem** of df-pn in cyclic graphs.

     Solutions are deferred to Kishimoto et al. 2012.
   - Draws in proof search are folded into loss.
9. **Final storage / compression**: Schaeffer et al. 2003 "not only compress databases space-efficiently but also decompress the databases in real-time so that forward search can use them both time- and memory-efficiently".
10. **Verification**: "The correctness of databases must be verified, since both software and hardware errors including disk and network errors may occur during a large-scale computation" (Schaeffer et al. 2003; Romein–Bal 2003). No method is given.
11. **Notable tricks worth borrowing**:
    - **How databases interact with forward search**: databases let forward search "obtain the game-theoretic value of a node without deeper search", so a database probe makes a node terminal.
    - Checkers solver architecture:
      - a disk-based PNS **front-end manager** drives a memory-only df-pn **back-end prover** (with a transposition table and SmallTreeGC);
      - αβ scores with an iterative threshold *th* classify nodes as "likely win" or "likely loss" when *s ≥ th* or *s ≤ −th*, and *th* is raised until the proof uses only true values;
      - endgame databases are the true leaves.
    - **PN²**: a two-level search in which the second-level tree is discarded and only the root's (pn, dn) are returned.
    - **Heuristic initialisation** of (pn, dn), e.g. by mobility (number of legal moves). df-pn+ raises its thresholds to compensate.
    - **The 1+ε trick** against transposition-table thrashing.
    - **Tree simulation** (Kawano): re-use the proof of a similar node.
    - **Dominance**: if node n is proven and node m dominates n, then m is proven too.
    - **Slicing** (Lake) and paging as decomposition strategies.
12. **Mapping to a single for_each per pass**:
    - **init** (step 2): a pure **map over the full index range**. It reads only position i and writes only v[i], so it is race-free under `par`.
    - **pull sweep** (steps 3–4): a **gather over the full index range**. It skips decided entries, generates moves, reads children, and writes only v[i].
      - Race-free under `par` provided v is either double-buffered (Jacobi iteration) or updated in place with monotone writes (Gauss–Seidel iteration).
      - Monotone means unknown → decided only, with each value in its own byte, so every read sees either "unknown" or a final value. In-place updating is then safe for WDL, and may converge in fewer sweeps.
      - In-place updating is **not** safe if the pass must also record a *distance* (DTM/DTC) consistent with the sweep number. A position decided in sweep k could otherwise read a child decided in the same sweep and get the wrong depth. Two remedies: double-buffer, or stamp each value with its sweep number and treat values from the current sweep as unknown.
      - The termination test "number unknown no longer decreases" is a `transform_reduce` (count) over the same range. It fuses naturally into the sweep as a reduction.
    - **Final draw marking** (step 5): a map over the full range, writing unknown → draw.
    - Every pass in this formulation is a gather, so the algorithm needs **no atomics**. The cost is Θ(|S| × sweeps) move generations, versus Θ(edges) for a frontier/push method. No pass resists the for_each form.
    - The parallel decompositions cited (Romein–Bal asynchronous message passing, Lake's slicing) are distributed schemes outside the single-for_each model. Slicing does map onto an outer sequential loop over slices, with an inner `for_each(par)` per slice.

---

## 2015-ICGA-Watkins.pdf

(The file name says 2015, but the PDF is *ICGA Journal* **37(2), June 2014**, pp. 106–110.)

1. **Citation**: Mark Watkins, "Solved Openings in Losing Chess", *ICGA Journal* 37(2), June 2014, 106–110 (Notes).
2. **Scope**: Losing chess (antichess) under joint FICS/International rules:
   - captures are compulsory, and the player chooses among captures;
   - the king is an ordinary piece and can be captured;
   - pawns may promote to a king;
   - there is no castling;
   - stalemate is a draw unless both rulesets call it a win.

   Results:
   - proofs that 1.e3 wins against 18 of Black's 20 replies, with proof-tree sizes from 33 nodes (d5, d6) to 82,084,261 nodes (b5, plus 3,860,735 transposition arcs);
   - for b5: 75.7 M internal nodes, 6.4 M terminal nodes, of which 1,415,338 are in 4-unit tablebases.

   Hardware: a 6-core Phenom 1065T, later Xeon E5-2670 clusters (176 cores, ≤ 200 cores in use). 1.e3 b5 took about 2–3 CPU-years.

   Tablebases:
   - adapted from the RobboBases of the IPPOLIT project;
   - 4-unit TBs build in about 2 hours, 5-unit in a couple of weeks;
   - a few relevant 6-unit TBs were built for 1.e3 e6.
3. **Metric**: **DTC (distance-to-conversion)**, "following the RobboBases". Inside the proof-number search only WDL is used, from a 2-bit-per-entry array; no further definition is given.
4. **Indexing**:
   - The RobboBases' fast **index differencing** (incremental index updates) "depends on the king-structure of normal chess, so we instead chose to re-compute always the index from scratch".
   - The RobboBases' "BlockedPawn counting as one unit" idea was omitted.
   - No other details.
5. **Working storage during construction**: not discussed. For probing, the 4-unit TBs were "a flattened array of 2 bits per entry", about **800 MB**, which "allows fairly fast access".
6. **Passes, in order**: not discussed for tablebase construction. The paper says only that the RobboBase code was adapted with "nontrivial modifications".
   - [background, not verified from PDF] With compulsory capture and a DTC metric, a builder of this kind has a conversion step: capture (and promotion) successors are resolved by probing already-built smaller tablebases. Retrograde iterations within the material slice follow.

   What *is* described is how the tablebases fit into the forward proof:
   - **Two-level search (essentially PN²)**:
     - an **upper-level tree** whose leaves are evaluated by a PN-search of 10^7 nodes (later 10^8);
     - the PN-search returns (pn, dn) for the node and its children, and the evaluation is the ratio pn/dn;
     - children whose subtree holds ≥ 70% of the parent's nodes are copied up recursively, so forced lines yield many upper-level nodes from one search.
   - **Terminal criteria inside the PN-search**:
     - side to move has no move;
     - **≤ 4 units left (tablebase probe)**;
     - repetition;
     - opposite-colour-bishop "draw-or-better for Black".
   - **(pn, dn) initialisation** by mobility: pn = 1, dn = number of legal moves.
   - **Leaf selection in the upper level**: a random walk from the root, weighted by the minimaxed ratio (cf. Shoham–Toledo). This also provides parallelism: independent PN-searches run on randomly chosen leaves.
   - **5-unit TBs** only adjusted the upper-level evaluation. The relevant subtrees were still expanded until they reached 4-unit TBs, because the goal was "final proof trees which needed only the 4-unit TBs".
   - A **final pass** reassembles the saved PN proof trees into one proof tree, which is then verified (item 10).
7. **Parallelism / memory access**:
   - **Tablebase builder**: the RobboBases' "**SMP-slicing** exploited the king structure, and with this not directly available we just ran it on one core". The construction was therefore **sequential**.
   - **Proof search**: embarrassingly parallel. Independent PN-searches run on upper-level leaves (6 at a time at first, later about 176–200). The author calls the efficacy "debatable" near the start and the end of a proof.
   - **Probing**: a 2-bit flat array, so a probe is one random read.
8. **Draws, cycles and special rules**:
   - **Transpositions inside the PN-search are identified only when the reversible-move count is zero.** This "had the advantage of dispelling any loops" at the cost of missed merges. It is a direct GHI and cycle avoidance. In the upper-level tree, known White wins were also identified.
   - **Draws were declared Black wins** (binarisation, "quite advantageous"). Repetition leaves then become disproofs, and opposite-colour-bishop endings (lone black bishop against a white bishop of the other colour) become Black wins.
   - Possible refinements mentioned for forced en passant and for promotions followed by a forced recapture.
   - The stalemate rule differs between rulesets (International: the player to move wins; FICS: fewer pieces wins).
9. **Final storage / compression**: Not discussed for the tablebases, beyond the 2-bit flat array for 4-unit. For 5-unit TBs, compression in the style of Schaeffer et al. 2003 is suggested; chess 5-unit TBs take about 450 MB (Shredderbases, RobboTripleBases, Syzygy).
10. **Verification**: of the **proof tree**, not of the tablebases. The checks:
    1. all Black moves are considered at Black nodes;
    2. nodes identified as transpositions really are the same position;
    3. terminal nodes are White wins, either by stalemate or wipe-out, or by a 4-unit TB.

    Further checks:
    - Frâncu transferred the 1.e3 Nc6 proof to FICS rules with Nilatac, giving another partial verification.
    - The author admits weak independence: the same move generator and tablebases were used for search and verification.
11. **Notable tricks worth borrowing**:
    - **Target a small, verifiable tablebase set.** The final certificate depends only on 4-unit TBs. Larger TBs guide the search but are not trusted in the proof.
    - **Separate the probing format from the build format**: a flat 2-bit WDL array for hot probes, DTC for building.
    - **Reversible-counter-zero transposition merging** as a cheap guard against cycles and GHI. In draughts this corresponds to merging only after captures or man moves.
    - **Draw ⇒ loss for the prover side** to obtain a binary search.
    - **Recompute the index from scratch** rather than updating it incrementally when the symmetry structure does not allow incremental updates. This makes each element's work independent, which suits a `par` map.
12. **Mapping to a single for_each per pass**:
    - **Tablebase construction**: not described, so it cannot be mapped from this paper. The builder actually ran on one core, because its parallel slicing depended on chess king structure.
    - **Final 2-bit flattening**: a **map over the full index range** with packing. Writing 4 entries per byte means each `for_each` element should be a whole byte (or word) of output, or the pass needs atomics. Iterating over output words, i.e. a gather of 4 or 32 source entries each, keeps it race-free.
    - The forward proof machinery (PN-search, upper-level tree, random-walk leaf selection) is *not* a data-parallel map. It is best-first tree search: task parallel, with shared tree updates.
    - Proof-tree verification is a **gather over the node array**: each node checks its own children and tablebase leaves and writes only its own error flag (a `transform_reduce`). It is therefore race-free under `par`.

---

## 2017-ICGA-Watkins.pdf

1. **Citation**: Mark Watkins, "Losing Chess: 1. e3 Wins for White", *ICGA Journal* 39 (2017) 123–125, DOI 10.3233/ICG-170017.
2. **Scope**: Completes the weak solution: 1.e3 wins against all 20 replies.
   - Proof sizes:
     - 1.e3 c5: 217,046,510 nodes (of which 2…Qc7 is 36.7 M);
     - 1.e3 b6: 448,616,089 nodes;
     - total: **863,301,867 nodes + 66,187,848 transposition pointers**;
     - after a revision project: 733,320,537 nodes + 54,208,096 pointers, with a maximum reversible run of 45 ply.
   - Hardware: cluster-capable code, typically about 128 cores. In total about 200–300 core-years at about 2 M pn-nodes/s, or about 10^16 positions searched. That is about 10^8 PN-searches of size 10^8, of which about 0.5% were used.
   - Tablebases:
     - the 4-man set (400 MB), standard in the proof;
     - **6-man TBs** such as lone black king against White without queens or bishops;
     - targeted **4-vs-KK and 4-vs-KN** TBs, built to "see from afar" whether a line was workable.
3. **Metric**: not restated; the 2014 note gives DTC. In the proof itself, terminal nodes are classified as:
   - in tablebases: 23,448,460;
   - White has lost everything: 18,117,361;
   - White stalemated with men left: 17,186,408.
4. **Indexing**: not discussed.
5. **Working storage during construction**: not discussed. "4-man tablebases, which themselves take up only 400 MB" (the 2014 note says about 800 MB for the 2-bit flat probe array; the difference is not explained in either PDF).
6. **Passes, in order**: not discussed for tablebase construction. Proof side: an automated PN² search plus human-guided expansion.
   - Targeted TBs were built *during* the proof to evaluate a crucial line. For 1.e3 c5 2.Bb5 Qc7, 19.g3 was found to win with the help of the 4-vs-KK and 4-vs-KN TBs.
7. **Parallelism / memory access**: Cluster parallelism of independent PN-searches (about 128 cores). Nothing about the tablebase builder.
8. **Draws, cycles and special rules**:
   - Long reversible sequences were reduced in the revision project (maximum 45 ply).
   - **About 87% of internal black-to-move nodes have exactly one legal move**, which is typical of forced-capture games.
9. **Final storage / compression**: The proof is stored compactly at **6 bytes per node**, then compressed with bzip2 to 1.3 GB.
10. **Verification**: As in 2014. In addition, the proof is browsable on Lichess, and Klaas Steenhuis re-ran parts with the software.
11. **Notable tricks worth borrowing**:
    - **Build targeted tablebases on demand** for the material signatures a hard line reaches. The trade-off between tablebase size and proof size is quantified: the proof holds 79.8 M positions with 5 men and 90.4 M with 6, so standard 5- and 6-man TBs would cut it by about 20%.
    - **Forced-capture chains** make proof trees mostly single-child chains (about 87% at Black nodes). The same holds for draughts proof trees, and probably favours compact chain encoding.
    - A cost comparison with the checkers solution: about 100× more positions searched. The author suggests the difference comes from proving a win rather than a draw.
12. **Mapping to a single for_each per pass**: No construction passes are described. The only data-parallel step implied is proof verification, a gather over nodes with a reduction to an error count. The search itself is task-parallel best-first search, not a map.

---

## Group synthesis

### Distinct primitives this group adds

1. **Full-sweep pull fixpoint (Kishimoto–Müller)**:
   - init map → repeated `for_each(par, all indices)` gather ("if unknown, generate moves, read children, decide self") → stop when a fused count-reduction of unknowns stops decreasing → map unknown → draw.
   - No atomics and no frontier. In-place monotone WDL updates are safe under `par`.
   - DTM/DTC needs a double buffer or sweep stamps to keep distances exact.
   - This is the reference "simplest correct" algorithm, against which the frontier/counter method in `egdb_fast.cpp` can be differentially tested.
2. **Verification as a gather + reduction**: the losing side is decided by re-checking all children, rather than by counters. The same primitive serves as the final database verification pass. `egdb_fast.cpp` already has one ("recompute every value from its children"), and it fits `std::transform_reduce(par, …)` exactly.
3. **Binarisation** (draw ⇒ loss for the prover; Kishimoto–Müller, Watkins): solve "does side X win?" as a Boolean problem. WDL then becomes two Boolean bit-planes (win-for-white, win-for-black), each computable with bitset-only passes; draw is "neither".
4. **Packing / flattening map**: convert byte values to a 2-bit WDL probe array (Watkins). This is race-free only if each `for_each` element owns a whole output word, i.e. it iterates over output words and gathers 4 or 32 inputs.
5. **Database probe as a terminal oracle in forward search** (all three surveys, Watkins): a read-only gather, never a scatter.
6. **Cycle-safe transposition merging only at zero reversible counter** (Watkins). In draughts, merge only after captures or man moves.
7. **Slicing and paging** (Lake; Romein–Bal): an outer *sequential* loop over slices or pages, with an inner `for_each(par)`. This is the shape for databases larger than RAM.
8. **Recompute the index from scratch per element** rather than differentially (Watkins). This keeps every element independent, which `par` maps require.

### Vocabulary worth adopting

- **Levels of solving** (Allis): ultra-weakly, weakly and strongly solved.
- **Game types**: convergent vs divergent games (only convergent games admit endgame databases); **state-space vs game-tree complexity**, with categories 1–4.
- **Position types**: P-position / N-position (Singmaster), i.e. lost / won for the side to move. OR / AND node; proof tree / disproof tree.
- **Distance metrics**: DTM / DTC / DTZ / DTR. Their maxima are named max-to-mate, max-to-conversion, max-to-zeroing-move and max-to-the-rule (Herik et al. 2002). **Watkins used DTC.**
- **Pass vocabulary** for this project:
  - **init** (terminal / illegal map);
  - **conversion** (probe smaller databases);
  - **pull sweep** (full-range gather);
  - **push / unmove frontier** (scatter with counters);
  - **verification sweep** (gather + reduce).
- **Forward-search terms**: front-end manager / back-end prover (the checkers solver); PN² and upper-level tree; GHI; overestimation; the infinite-loop problem.
- **Units**: Watkins' count of men, used to name tablebase tiers ("4-unit TBs").
- **Reciprocal zugzwang**: a database-mining query that compares a position's values with White to move and with Black to move.

### What this group does *not* give

- No paper here describes a predecessor ("push") pass, move counters, or race handling under scatter.
- The Watkins notes say nothing about how the RobboBase-derived builder handled compulsory capture internally, beyond indexing and single-core execution.

For push and counter algorithms, rely on the other groups' papers (Thompson, Lake–Schaeffer–Lu, Romein–Bal, Gasser, and the checkers 10-piece paper).
