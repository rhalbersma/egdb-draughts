# egdb_6x6: the state of the design (inventory for a redesign)

A snapshot of what `src/egdb_6x6.cpp` and the pipelines do now, the names they use, the data they hold and the order in
which they work, with the measurements that bear on a redesign. Written to start defining the primitives properly; it
describes, it does not prescribe.

## 1. Terms as used in the code

| term | in the code | meaning |
|---|---|---|
| position | `Pos` | men `m[2]`, kings `k[2]` (18-square bitboards, `xstd::bit_fixed_set`), side to move `stm` (0 white) |
| mirror | `mirror(p)`, `flip(bb)` | 180 degree rotation with the colours swapped and the side to move swapped (square n <-> 19 - n) |
| material | `wp-bp-wm-bm` in file names; `UnitMat{wm, bm, wk, bk}` in the engine | the numbers of pieces and men per side; at most 6 per side. Only one of a material and its mirror is stored (`stored_material`) |
| unit | `(n, m)`, `Unit` | all materials with n pieces of which m are men (the storage of `units_6x6.py`); with white to move only |
| slice | `Key{wm, bm, nwk, nbk}`, `Meta{key, free, nb, P}` | a fixed placement of the men and the numbers of kings per side; both sides to move. Only one of a slice and its mirror is stored (`canonical_key`) |
| index | `index_of(Meta, Pos)`, `pos_of(Meta, idx, stm)` | colex rank of the white kings in the free squares times `nb` plus the rank of the black kings in the rest: a bijection, no invalid entries, no symmetry folding |
| P, T | `Meta::P`, `T = 2 P` | positions per side to move; positions of a slice (both sides) |
| S | `UnitMat::S` | sides to move stored for a material: 2, or 1 for a material equal to its mirror. (`S` in the profile files is the slice size T: a clash) |
| internal move | `internal_move(p, mv)` | a king move without capture: stays in the slice |
| exit | every other move | capture, man move, promotion: leads to another slice of the same material or to another material |
| distance | `distance(Key)` | rows the men still have to go; slices are built in increasing distance (a man move goes to a smaller distance) |
| value | 16 bits in the solvers | DTM in plies, odd = win and even = loss for the side to move, `DRAW = 0xFFFF`, `UNKNOWN16 = 0xFFFE` |
| exit value | `ex[i]` / `ewin` / `eloss` | the best exit for the side to move: fastest exit win, else (all exits lose) the slowest exit loss + 1; `NONE16 = 0xFFFF` for "a drawing exit" (equal to `DRAW`: a clash) |
| frontier | `front[n - 1]` | the positions with value n - 1: the source of round n |
| pass 0 | | the one walk over every position of a slice (moves, exit lookups, counts) |
| round n | | n odd: wins in n; n even: losses in n |
| count | `cnt[i]` | internal king moves not yet known to lose (out-counting) |
| big slice | `BIG_SLICE` (units: T >= 2^20 and more than one thread's share), `big` (materials: a material of all kings or one man with at least 4M positions runs with all cores) | two different thresholds with the same name |

Value encodings (three):
- solvers: 16 bits as above;
- raw file during a build: 1 byte, DTM up to 251, `ESC8 = 252` (exact value in the `.ovf` table), `DRAW8 = 253`,
  `UNKNOWN8 = 254`;
- compressed `.cdb`: version 1 draw 253; version 2 draw 255, DTM 0 .. E - 1, escape codes E .. 254 with an extra byte.
  Capture positions are not stored: they repeat the previous value of their 4 KiB block (`filled_block`), so a lookup
  of a capture position recomputes it (`resolve_capture`, `from_successors`, a per-thread cache of 2^20 entries).

## 2. Work order of a build

Pipelines (both Snakemake, one engine process per step):
- units (`international/Snakefile`, or the driver `units_6x6.py`): per unit (n, m) build, verify raw, compress, compare,
  crc, delete raw; every unit waits for the previous one. This built the international store through 12 pieces.
- materials (`materials/Snakefile`): the same steps per material, started as soon as the materials it captures or
  promotes into are done. Used for all solver experiments.

One build step (`unit_build` / `mat_build`):
1. open the store: every finished material or unit, compressed (lookups go through a 64 MiB block cache per thread);
2. the slices of the unit or material, sorted by distance; a group of equal distance is independent;
3. per group: big slices one at a time with all threads, the others one per thread;
4. per slice: solve (below), then write both sides to the raw file (a side may come from the mirror slice);
5. per group: msync and a checkpoint (`.raw.progress`), so that an interrupted build continues.

One slice, work-list solver (`solve_slice_hybrid`, the default in the materials pipeline):
1. pass 0, every position: generate all moves (`moves`), count the internal ones, look up every exit (`lookup` ->
   `store_lookup`: canonical form, material, offset, block cache; a capture position is resolved). No internal move
   (always so with a capture: captures are compulsory): decided at once. Else the count and the exit value are kept.
2. lists: frontier by value, exit wins by value, undecided (exit value even: "lossable", or not).
3. round n = 1, 2, ... while undecided positions remain and n <= max value + 1:
   - retrograde: every frontier position generates its predecessors (king un-moves of the other side); win round: an
     undecided predecessor is won in n (compare-exchange), then the exit wins in n; loss round: the predecessor's count
     goes down, at zero it is lost in max(exit value, n);
   - or forward (adaptive mode): every undecided position examines its internal moves.
4. what is still undecided is a draw.

Classic solvers (`EGDB_SOLVER=classic` in materials; always in units): `solve_slice` (one thread) and `solve_slice_par`
(units, big slices) or `solve_slice_pull` (materials, big slices). Same pass 0; each round scans the whole slice for the
frontier (and in win rounds the exit wins); loss rounds mark candidates in a bitmap and check each once by generating
its moves (the grandfather scheme). `solve_slice_pull` scans every undecided position every round (forward).

## 3. Data per position

| stage | data |
|---|---|
| work-list solver | `cur` 2 B, `ex` 2 B, `cnt` 1 B; lists of 4 B per entry (frontier by value, exit wins by value, undecided); per task `got`/`keep` |
| `solve_slice` | `out` 2 B, `ewin` 2 B, candidate bitmap 1 bit |
| `solve_slice_par` | `out` 2 B, `ewin` 2 B, frontier and candidate bitmaps 1 bit each |
| `solve_slice_pull` | `cur`, `nxt`, `ewin`, `eloss` 2 B each |
| raw file | 1 B (+ `.ovf` for long distances) |
| compressed | 4 KiB blocks, zstd -19, capture positions filled |
| per thread | block cache 2^14 x 4 KiB = 64 MiB; capture cache 2^20 x 26 B = 26 MiB |

For comparison, syzygy (talkchess "endgame table generation", 2022) holds one byte per position during generation:
distances plus codes `ILLEGAL`, `BROKEN`, `UNKNOWN`, `CHANGED` (possibly lost, checked in the next iteration),
`CAPT_WIN`, `CAPT_DRAW`, `CAPT_CLOSS`, `MATE`; its metric (DTZ) restarts at every capture, so a capture needs no depth.

## 4. Switches (experiments; environment variables)

`EGDB_SOLVER` (classic / hybrid), `EGDB_SWITCH` (forward / retrograde / auto), `EGDB_COSTS` (6 constants of the rule),
`EGDB_COUNT=0` (loss rounds check every predecessor instead of counting), `EGDB_PROFILE` (every round both ways, timed;
analysed by `materials/switch.py`), `EGDB_BIG_SLICE`, `EGDB_CACHE_BITS`, `EGDB_DTM8_MAX`, `EGDB_ESC`.

## 5. Measurements (checkers through 8 pieces, materials pipeline, 4 cores, build seconds summed over materials)

| what | result |
|---|---|
| classic | 1376 s |
| work lists, retrograde, without counting | 1204.5 s |
| work lists, retrograde, counting | 1032.3 s (twice: 1035.8 s) |
| work lists, adaptive forward/retrograde | rounds 16% shorter (320.9 s -> 269.4 s); build 4% shorter |
| split of a build (retrograde) | pass 0 and lists 673 s, rounds 321 s, of about 1000 s |
| profile of 04-04-01-02 (53 s) | pass 0 69%: capture resolution 25%, other lookups 17%, own move generation 22%; rounds 26%, of which the capture test on predecessors 14% (since removed: rounds 14.4 s -> 7.7 s) |
| lookups of 04-04-01-02 | 211.7M; 69.7M land on a capture position, 26.0M distinct; the capture cache hits 10% (a well-mixed hash: 32%, but slower, from random memory access) |
| memory for a value per position of the materials a build looks up | 0.15 GiB at 8 pieces, up to 19.7 GiB at 12 pieces (06-06-02-04) |

## 6. Open decisions

1. one pipeline (materials) and one storage format, or units kept as storage;
2. one slice solver (retrograde on work lists with counting, serial and parallel) and the classic solvers removed;
3. the adaptive forward rounds kept (4% of a build) or removed with their profile mode, constants and `switch.py`;
4. the true values of capture positions stored in the compressed files (removes capture resolution and the capture test
   from every lookup; costs compressed size: being measured);
5. frontier and undecided positions as bitsets in index order (xstd set-bit iteration) instead of lists, for locality
   in the large 11 and 12 piece slices;
6. fewer bytes per position during a build (syzygy-style codes in one value byte), for memory at 12 pieces;
7. the metric: DTM across materials (now), or distance to conversion (a capture or man move restarts the count, as
   syzygy's DTZ), which would let a capture carry only its WDL.
