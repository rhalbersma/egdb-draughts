# Kings-only endgame databases for draughts and checkers on arbitrary boards

Distance-to-mate (DTM) endgame databases for positions with only kings, on any rectangular
chequered board `B × L` (optionally with squares removed), for two rule sets:

* **international draughts**: flying kings, mandatory capture, majority capture, Turkish strike;
* **English checkers**: short kings (one square), mandatory capture, free choice among captures.

The databases were used to check, extend and refine the results of a 1988 Dutch book on
"three kings against one" on boards of every size, whose results were all found by hand.
This page collects the programs, the tables and the most interesting findings.

* [Programs](#programs)
* [Conventions](#conventions)
* [International draughts: 2 kings vs 1](#international-draughts-2-kings-vs-1)
* [International draughts: 3 kings vs 1](#international-draughts-3-kings-vs-1)
* [Trapping positions: why the families work for every board size](#trapping-positions-why-the-families-work-for-every-board-size)
* [10×10 with one square removed](#1010-with-one-square-removed)
* [Comparison with the 1988 book](#comparison-with-the-1988-book)
* [English checkers](#english-checkers)
* [Reproducing everything](#reproducing-everything)

---

## Programs

| Program | Data structure | Algorithm | Notes |
|---|---|---|---|
| [`src/egdb.cpp`](src/egdb.cpp) | one `std::map` per material balance, key = (sorted white squares, sorted black squares) | forward iteration per ply | deliberately simple, easy to check |
| [`src/egdb_fast.cpp`](src/egdb_fast.cpp) | `xstd::bit_fixed_set` bitboards, compact combinatorial index, one byte per position and side to move | retrograde analysis with move counters, OpenMP | 200–450× faster, boards up to 512 squares |

Both use [tabula](https://github.com/rhalbersma/tabula) for the board geometry (`tabula::chequer`
with width, height and parity), produce **byte-identical** text output, and print the same statistics.

The fast version:

* **bitboards**: [`xstd::bit_fixed_set<N>`](https://github.com/rhalbersma/xstd-bits) with precomputed rays per square
  and direction; the engine is a template on the bitboard type and the program picks the narrowest
  `N ∈ {64, 128, 192, 256, 384, 512}` that holds every playable square, so any board up to 512 squares works;
* **compact indexing**: `index = colex_rank(white) · C(N − w, b) + colex_rank(black′)`, where `black′`
  renumbers the black squares among the squares white does not occupy, so every array slot is a legal position;
* **retrograde analysis**: positions with a capture are resolved immediately from the smaller
  databases; after that only the predecessors (unmoves) of newly decided positions are visited;
* output as text (`db_W_B.txt`, one line per position) and as a compact binary file (`db_W_B.bin`).

### Validation

* the 1-1, 2-1 and 3-1 databases on 10×10 reproduce all published statistics and example positions of
  [mdgsoftware.nl](https://mdgsoftware.nl/draughts/stats/) exactly (pages 0101, 0201, 0102, 0301 and 0103);
* every run ends with a verification pass that recomputes every value from its successors (0 errors in all runs);
* the fast and the simple program give byte-identical files on ten different boards
  (lakes, odd widths, both parities, both rule sets);
* results are invariant under mirroring and transposing the board.

### Speed (one 4-core machine)

| Database | `egdb` | `egdb_fast` |
|---|---|---|
| 10×10, 3-1 (with 2-1, 1-1) | 126 s | 0.5 s |
| 10×12, 3-1 | 372 s | 1.7 s |
| 9×17, 3-1 | ~40 min | 5.1 s |
| 12×21, 3-1 (126 squares, 40 M positions) | — | 22 s |
| all 154 runs of the full re-run (international grid, lake boards, checkers) | many hours | 7 min |
| 12×23, 3-1 (138 squares, 58 M positions, `bit_fixed_set<192>`) | — | 101 s |

Against a hand-rolled `unsigned __int128` bitboard, `bit_fixed_set<64>` is exactly as fast (≤ 64 squares) and
`bit_fixed_set<128>` ranges from 15% faster to 35% slower (65–128 squares; 10×12: 0.96 s vs 1.12 s, 9×17: 4.2 s vs 3.1 s).

### Usage

Requires a C++23 compiler (GCC 14 or later), CMake 3.30 or later, OpenMP and the Boost.Container headers;
[tabula](https://github.com/rhalbersma/tabula) and [xstd-bits](https://github.com/rhalbersma/xstd-bits) are fetched by CMake.

```
cmake -B build && cmake --build build
./build/egdb_fast --board international --out data/international --query W:WK2,K10,K45:BK50
./build/egdb_fast --width 15 --height 8 --parity 1                    # 3-1 on 8×15
./build/egdb_fast --board international --lake g7                    # 10×10 without g7
./build/egdb_fast --width 9 --height 8 --rules checkers --white 3 --black 2
```

`--query` prints the value, every move with its value (algebraic and numeric notation) and a principal variation.

---

## Conventions

* **Board** `B × L`: `B` is the shorter side, `L` the longer one. In the diagrams the long side is horizontal.
* **Parity** (tabula): **p1** = the corner squares are playable (a1 is a dark square), **p0** = they are not.
  This only matters for odd × odd boards; otherwise both parities are mirror images.
* **Squares**: numbered 1..N in PDN order (square 1 top left) in FEN and in the text files; algebraic (a1 bottom left) in move sequences.
* **DTM** in plies for the side to move: `+n` win in n plies, `-n` loss in n plies (`-0`: no moves left), `D` draw.
  Purely game-theoretic: no move-count rules.
* **"Won"** for a board means: every position in which neither side can capture is won for the side with more kings to move.
* In the tables: `21` = won, longest win 21 plies · `21*` = won provided the stronger side occupies the long
  diagonal(s) · `(21)` = drawn (the number is the longest win among the positions that are won) · `13/15` = p0/p1.

---

## International draughts: 2 kings vs 1

```
 B \ L−B     0        1       2        3
 3         3/3*       7      11/9      11
 4          15*      13      13       21
 5        (7)/(3)    27     (9)/15    (9)
 6          (7)      (9)    (11)      (9)
 7        (7)/(5)    (9)    (9)/(11)  (9)
 8          (7)      (9)    (11)      (9)
 9        (7)/(5)    (9)    (9)/(11)  (9)
10          (7)      (9)    (11)      (9)
11        (7)/(5)    (9)    (9)/(11)  (9)
12          (7)      (9)    (11)      (9)
```

* For `B ≤ 4` (degenerate boards) two kings always win, on the square boards provided they hold the long diagonal(s).
* For `B ≥ 5` only two boards are won: **5×6** (new) and **5×7 p1** (known). From `B = 6` on everything is drawn,
  and the remaining wins are short combinations whose length does not depend on the board size.
* With the single king to move there are a few exceptions on 5×6 (8) and 5×7 (16): double attacks (forks).

**5×6, longest win (27 plies)**: `W:WK1,K8:BK7`
```
    +-------------+
  5 | W   .   .   |
  4 |   .   .   . |
  3 | B   W   .   |
  2 |   .   .   . |
  1 | .   .   .   |
    +-------------+
      a b c d e f
```
1. c3-d4 a3-c5 2. d4-b2 c5-e3 3. a5-b4 e3-c5 4. b4-a3 c5-e3 5. b2-e5 e3-f2 6. a3-c5 f2-e1 7. e5-f4 e1-c3
8. f4-c1 c3-e5 9. c5-e3 e5-c3 10. c1-a3 c3-e5 11. e3-f4 e5-d4 12. **a3-b2** d4xa1 13. f4-e5 a1-b2 14. e5xa1

The long tempo play serves one purpose: sacrifice so that black captures *into the corner*, then place the
second king behind it on the corner diagonal. On 5×7 p1 the same final motif appears after 15 plies:
`W:WK1,K9:BK2` 1. c3-e5 c5-a3 2. a5-d2 a3-c5 3. d2-c1 c5-f2 4. e5-g3 f2-c5 5. c1-a3 c5-d4 6. **a3-b2** d4xa1 7. g3-e5 a1-b2 8. e5xa1.
More than 80% of all quiet wins on these two boards end with black capturing into a corner.

---

## International draughts: 3 kings vs 1

```
 B \ L−B    0       1      2       3      4       5      6       7      8       9     10      11
 3        0/3      5     7/9      9    9/11     13   15/15
 4         9*     11     11      13     17      23     29      37
 5      11/15*    23   13/13     15   17/15     17   19/19     21   23/23
 6        17*     15     17      19     19      21     25      27     29      33
 7      19/17*    19   21/19     23   25/25     35   49/21     53  (73)/67  (73) (73)/(43)
 8        25*†    19     23      31    (23)    (33)   (21)     25    (19)    (19)   (21)    (19)
 9     (15)/(15)  21  (19)/21   (27) (17)/(19) (31) (19)/(21) (21)  (19)/23  (19) (19)/(19) (21)
10       (15)     21     25     (27)   (19)    (31)   (21)    (21)   (19)     27   (19)    (19)
11     (15)/(15)  21  (19)/21   (27) (17)/(19) (31) (19)/(21) (21) (19)/(21) (19)  (19)/25  (19)
12       (15)     21     25     (27)   (19)    (31)   (21)    (21)   (19)    (21)          27
```
`†`: on 8×8 there are 40 positions (2 up to symmetry) with the single king to move that draw by a fork,
even with white on the long diagonal. Blank: not computed.

The 10×10 numbers agree with mdgsoftware.nl: with three kings to move 486,444 wins and 434,756 draws;
with the single king to move 16,812 wins, 863,802 draws and 40,586 losses.

### The three families

| Family | Result (B = 3 … 12) |
|---|---|
| **L = B + 1** | won on every board |
| **L = B + 2** | won, except the "wrong" colouring p0 for B = 9 and 11 |
| **L = 2B − 1** | won, except p0 for B = 9 and 11 (10×19: 27 plies, 11×21 p1: 25 plies, 12×23: 27 plies) |

The colouring rule of the book is confirmed: from B = 9 on, for odd B only the colouring with playable corners (p1) wins.
On B = 7 the wrong colouring still wins (5×9 p0, 7×13 p0) because the general method still reaches that far.

### The general method on narrow boards

* **B = 5, 6**: every non-square board is won (checked up to 5×13 and 6×15). Three kings in a column
  close off the full width and can be pushed along the board; the longest win grows by about 2 plies per extra column.
* **B = 7**: won up to **7×14** and **7×15 p1** (book: up to 7×11). From L = 16 on it is drawn.
  The longest wins jump from 25 to 35, 49, 53 and 67–73 plies: the space becomes tight.
* **B = 8**: won at L = 9, 10, 11 and 15 (= 2B − 1), drawn at L = 12–14 and 16–19.
* **B ≥ 9**: only the families remain.

### Square boards: the long diagonal

On square boards with B ≥ 5 the single king holds the long diagonal(s). Splitting the quiet positions with
three kings to move by who occupies the long diagonal(s) (draws / positions):

| Board | White on the long diagonal(s) | Nobody | Only black |
|---|---|---|---|
| 6×6 | 0 / 3,154 | 0 / 816 | 784 / 812 |
| 8×8 | 0 / 33,364 (40 forks with black to move) | 0 / 17,184 | 10,696 / 11,084 |
| 5×5 p1 (two diagonals, white on both) | 0 / 540 | | |
| 7×7 p1 (white on both) | 0 / 6,032 | | |
| 9×9 p1 (white on both) | 32,492 / 39,328 | | |
| 10×10 | 177,548 / 206,466 | | |

Up to B = 8 the long line decides; from B = 9 on three kings cannot win even when they hold it.
With one long diagonal the condition amounts to "black does not hold it" (self-evident for a method that
starts by occupying it); with two diagonals white really needs both.

---

## Trapping positions: why the families work for every board size

Each family has a trapping position for the three white kings that is fixed **relative to a corner or to the V**.
Kept at the same place on every board size, the conversion always takes exactly the same number of plies
(longest loss with black to move / longest win with white to move, over every square of the black king where neither side can capture):

| Family | White kings | Boards checked | Black to move | White to move |
|---|---|---|---|---|
| **B + 1** | b2, (L−1, B−2), (L−3, B−4) — e.g. 10×11: b2, k9, i7 | 6×7 … 11×12 | 10 | 9 |
| **B + 2** | b2, (L−1, B−3), (L−3, B−5) — e.g. 10×12: b2, l8, j6 | 6×8 … 10×12 | 12 | 11 |
| **2B − 1** | one above each leg of the V, one below its apex — e.g. 10×19: b2, j8, r2 | 5×9 … 9×17 | 8 | 7 |

No draw is left on any of these boards. The strong side builds the trapping position from an arbitrary position
and then converts quickly: on 9×17 the longest win is 23 plies, of which at most 8 are the conversion.

**The V** (only for `L = 2B − 1`): the diagonals from the two corners of one long side meet exactly in the
middle of the opposite side. Shorter, and the legs cross below the edge; longer, and they hit the edge at two different squares.
On 6×11:
```
    +-----------------------+
  6 |   .   .   *   .   .   |
  5 | .   .   *   *   .   . |
  4 |   .   *   .   *   .   |
  3 | .   *   .   .   *   . |
  2 |   *   .   .   .   *   |
  1 | *   .   .   .   .   * |
    +-----------------------+
      a b c d e f g h i j k
```
Even B always has the two corners; odd B has them only with colouring p1. The trapping map is exactly symmetric about the apex file.

**B + 1 and B + 2**: the king at b2 stands on the long diagonal, whose other end is an entrance at the top right;
the two kings at the top right form a half trap with it and the upper one can close the long line.
Black king top left: b2 sacrifices and black is caught. Black king bottom right: the second king sacrifices and the upper one closes in.
On 10×11 (book diagram 8) every square of the black king is won with white to move within 5 plies,
**except h10, the hatched square in the book, which takes 9 plies**:
```
    +---------------------------------------------+
 10 |     +3      +3      +5      +9      +3      |
  9 | +3      +3      +3      +1x     +1x      W  |
  8 |     +3      +3      +3      +1x     +3      |
  7 | +3      +3      +3      +1x      W      +5  |
  6 |     +3      +3      +1x     +1x     +1x     |
  5 | +3      +3      +1x     +1x     +5      +5  |
  4 |     +3      +1x     +1x     +5      +5      |
  3 | +3      +1x     +1x     +5      +5      +5  |
  2 |      W      +1x     +5      +5      +5      |
  1 | +3      +3      +5      +5      +5      +5  |
    +---------------------------------------------+
      a   b   c   d   e   f   g   h   i   j   k
```
(DTM per square of the black king with white to move; `x` = white can capture.)

---

## 10×10 with one square removed

One square per symmetry class (the 4 symmetries of the board give 15 classes):

| Removed | Symmetric squares | Book | Database | Longest win | Quiet draws (3 kings to move) |
|---|---|---|---|---|---|
| 1 (b10) | a9, j2, i1 | W | **W** | 23 | 0 |
| 2 (d10) | a7, j4, g1 | D | **D** | (33) | 64,675 |
| 3 (f10) | j6, a5, e1 | ? | **D** | (23) | 337,341 |
| 4 (h10) | j8, a3, c1 | D | **D** | (29) | 66,013 |
| 5 (j10, Mertens) | a1 | W | **W** | 21 | 0 |
| 7 (c9) | b8, i3, h2 | W | **W** | 25 | 0 |
| 8 (e9) | b6, i5, f2 | D | **D** | (31) | 64,192 |
| 9 (g9) | i7, b4, d2 | D | **D** | (27) | 66,653 |
| 10 (i9) | b2 | W | **W if j10 is empty** | (19) | 29,875, all with a piece on the isolated square j10 |
| 12 (d8) | c7, h4, g3 | ? | **D** | (35) | 66,365 |
| 13 (f8) | h6, c5, e3 | ? | **D** | (37) | 67,259 |
| 14 (h8) | c3 | W | **W except one zugzwang** | 33 | 2 (one position and its mirror image) |
| 18 (e7) | d6, g5, f4 | D | **D** | (35) | 67,660 |
| 19 (g7) | d4 | ? | **W** | 33 | 0 |
| 23 (f6) | e5 | W | **W** | 35 | 0 |

* Removing i9 isolates j10: a king there can never move and never be captured.
* The single exception with h8 removed is `W:WK4,K5,K10:BK15` (white h10, j10, i9; black j8) with white to move:
  i9 and j10 are locked in, every move of h10 allows a double capture (1. h10-g9 j8xh10xf8) and 1-1 is drawn.
  With black to move the same position is lost in 34 plies.

---

## Comparison with the 1988 book

**Confirmed**: everything the book states firmly — 2-1 for B ≤ 4 (with the long-line condition on 3×3 and 4×4) and on 5×7 p1;
3-1 on B = 5, 6 for every L > B (square boards with the long-line condition, explicit on 5×5 p1);
B = 7 for L = 7–11; B = 8 for L = 9, 10, 11, 15; the families B + 1, B + 2 and 2B − 1 with the colouring rule;
the lake boards 1, 2, 4, 5, 7, 8, 9, 18, 23.

**New**:

| Material | Board | Result | Longest win |
|---|---|---|---|
| 2-1 | 5×6 | won | 27 plies |
| 3-1 | 7×12, 7×13 p0, 7×14, 7×15 p1 | won (general method) | 35, 49, 53, 67 |
| 3-1 | 7×15 p0, 7×16, 7×17 | drawn: the limit of B = 7 | |
| 3-1 | 10×10 without g7 (19) | won | 33 |
| 3-1 | 10×10 without f10, d8, f8 (3, 12, 13) | drawn | |

**Refined**: 10×10 without i9 (isolated j10), without h8 (one artificial zugzwang), 8×8 with white on the long line
(40 forks with black to move), and the exact limits of every row up to B = 12.

---

## English checkers

### 2 kings vs 1

Drawn on 4×4. On every larger board it is won, except for a fixed handful of positions (8 or 10, independent of the board size)
in which the two white kings are locked in the single corner and the black king keeps them there by opposition.
On odd × odd boards with playable corners there are no exceptions at all. The longest win grows steadily:
33 plies on 8×8, 41 on 10×10, 49 on 12×12, 61 on 12×15.

### 3 kings vs 2

| Board | Quiet draws (3 kings to move) | Longest win | Black in both double corners: draws |
|---|---|---|---|
| 6×7 | 273 | 63 | 0% |
| 7×8 | 792 (0.16%) | 75 | 0% |
| 8×8 | 1,252 (0.11%) | 67 | 0% |
| 8×9 | 87,670 (4.0%) | 79 | 50% (opposition) |
| 9×10 | 344,907 (4.4%) | 93 | 50% (opposition) |
| 8×10 | 288,748 | 97 | ~100% |
| 10×10 | 1,012,242 (7.1%) | 115 | ~100% |
| 10×11 | 1,810,258 (7.5%) | 109 | ~100% |
| 10×12 | 2,744,990 | 127 | ~100% |
| 7×9 p0 (four double corners) | 124,148 (13.2%) | 93 | see below |
| 9×9 p0 (four double corners) | 1,201,460 (29.5%) | 99 | see below |

**Small boards: blockades.** On 7×8 and 8×8 3-2 is won generically. The few draws occur almost only when white is pushed
against the edge (draw rate 0.0% with no white king on the edge, 0.5% with three), and black holding both double corners
never draws. Example on 7×8, `W:WK1,K2,K3:BK10,K11`:
```
    +-----------------+
  7 | W   W   W   .   |
  6 |   .   .   .   . |
  5 | .   B   B   .   |
  4 |   .   .   .   . |
  3 | .   .   .   .   |
  2 |   .   .   .   . |
  1 | .   .   .   .   |
    +-----------------+
      a b c d e f g h
```
Four of the five white moves lose a king at once (a7-b6 c5xa7, c7-d6 e5xc7, e7-d6 c5xe7, e7-f6 e5xg7). After 1. c7-b6
black has to find the only drawing move 1... e5-d4 (everything else loses in 6–34 plies), after which white can keep its
kings forever but never breaks out: black cannot force a capture in any of the 901,577 positions reachable without captures.

**Larger boards: the double-corner fortress.** A king on the two edge squares of a double corner can never be captured
and always has a waiting move. On 8×9 and 9×10 a black king in each double corner draws in exactly half of the positions,
and that half is decided by **opposition**: the parity of the sum of the ranks of all pieces flips with every move, and kings
cannot lose a tempo, so who has the opposition is fixed from the start.

| Black in both double corners | Parity 0 | Parity 1 |
|---|---|---|
| 8×9 | 99.5% draws | 0.0% draws |
| 9×10 | 99.8% | 0.0% |
| 10×11 | 99.9% | 99.9% |

8×9, black on a7 and i7 (shuttling with b8 and h8), white c7, e7, g7:
```
    +-------------------+
  8 |   .   .   .   .   |
  7 | B   W   W   W   B |
  6 |   .   .   .   .   |
  5 | .   .   .   .   . |
  4 |   .   .   .   .   |
  3 | .   .   .   .   . |
  2 |   .   .   .   .   |
  1 | .   .   .   .   . |
    +-------------------+
      a b c d e f g h i
```
With white to move every move draws: black just shuttles a7-b8-a7. With black to move black is in zugzwang and loses in
28 plies: 1... a7-b8 2. **g7-f8!** b8xd6 3. e7xc5 and 2-1 wins. With the white king on d6 instead of c7, white wins with
1. **d6-c7!**, reaching the same position with black to move.

**The distance rule.** What decides is the distance between the two corners of the double corners black occupies
(`D` = largest, `d` = smallest coordinate difference; `d = 0` when they are on the same side), not whether they lie on the same side:

| Boards | D × d | Fortress |
|---|---|---|
| 6×7, 7×8, 7×9 p0 (vertical pair) | 6 × 0 | falls |
| 8×8 | 7 × 7 | falls |
| 8×9, 9×10, 9×9 p0 (same side), 7×9 p0 (horizontal) | 8 × 0 | opposition decides |
| 7×9 p0 (diagonal pair) | 8 × 6 | opposition decides |
| 9×9 p0 (diagonal pair) | 8 × 8 | holds |
| 8×10, 10×10, 10×11, 10×12 | ≥ 9 | holds |

`D ≤ 7`: the fortress falls · `D = 8`: opposition decides, unless also `d = 8` · `D ≥ 9`: the fortress holds.
On 9×9 p0, freezing one corner already takes all three white kings (one on the shuttle square, one blocking the exit,
one guarding the blocker), so the other black king simply shuttles:
```
    +-------------------+
  9 |   .   .   .   .   |
  8 | .   .   .   .   B |
  7 |   .   .   .   .   |
  6 | .   .   .   .   . |
  5 |   .   .   .   .   |
  4 | .   .   .   .   . |
  3 |   .   W   .   .   |
  2 | W   W   .   .   . |
  1 |   B   .   .   .   |
    +-------------------+
      a b c d e f g h i
```

---

## Reproducing everything

```
cmake -B build && cmake --build build
scripts/run_all.sh                 # all databases, a few minutes; writes data/ and data/summary.txt
```

* [`results/summary.txt`](results/summary.txt): one line per database run (board, time, verification, draws, longest win).
* [`scripts/trap.py`](scripts/trap.py): DTM per square of the black king for a fixed placement of the white kings.
* [`scripts/edge_corner.py`](scripts/edge_corner.py): checkers 3-2 draws by white kings on the edge × black kings in double corners.
* [`scripts/checkers_draws.py`](scripts/checkers_draws.py): classifies checkers draws (immediate shots versus blockades).
* [`scripts/force_capture.py`](scripts/force_capture.py): can black force the win of a white king, and how fast.
