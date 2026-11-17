//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Exact endgame databases for international draughts on the 6x6 board, with men and kings, or (built with
// -DEGDB_6X6_CHECKERS=1, as egdb_6x6_checkers) for English checkers on the same board.
//
// Rules: international draughts on 6x6: men move one square diagonally forward and capture forwards
// and backwards, flying kings, mandatory capture, majority capture (kings and men count the same),
// Turkish strike (captured pieces are removed after the capture and cannot be jumped twice), a man
// only promotes when it ends its move on the last row. The side to move without a legal move loses.
// Checkers: kings move and capture one square at a time, men capture forwards only, capture is mandatory
// but any capture may be chosen (no majority rule), a capture is completed; a man that reaches the last row
// ends there and is crowned (it cannot capture backwards, so this needs no rule of its own). The board, the
// numbering and the start position are the same (tabula's draughts::micro, dctl's rectangular<6, 6>).
//
// Squares 1..18 as in "Strategie der 36 velden" (H.J. Brascamp): square 1 = b6 (top left),
// 16 = a1 (bottom left, a dark square), the long diagonal is 16-13-11-8-6-3. White starts on 13-18
// and moves up, black starts on 1-6 and moves down.
//
// Database organisation: a "slice" fixes the squares of the white and black men and the number of
// white and black kings. Within a slice only king moves without capture keep the position in the slice;
// every other move (man move, capture, promotion) goes to a slice with fewer pieces, fewer men or men
// further advanced, so the slices form an acyclic graph. Each slice is solved by leapfrog retrograde
// analysis with one byte of depth-to-mate (DTM, plies until the side to move has no move left) per
// position; positions that are never decided are draws (repetition).
//
// All slices up to `built_max` pieces can be built in advance (`build`) and saved; slices with more
// pieces are solved on demand. Slices hold every position with at most `slice_max` pieces and every
// position in which both sides have a king. Other positions cannot repeat (the side without kings must
// move a man or capture), so their exact values, with DTM, come from a forward alpha-beta search down
// to the slices; no graph-history problems arise.
//
// Protocol (stdin, one command per line):
//   build <n> [t] [d]  build all slices with at most n pieces, with t threads; above d pieces WDL only
//   save <file>     save the prebuilt slices;  load <file>  load them
//   extend <n> [t] <file>  add the layer of n-piece slices with DTM in a memory-mapped file (+ .idx)
//   resume <n> [t] <file>  continue an interrupted extend from its last checkpoint
//   attach <file>   attach such a layer after loading the layers below
//   pos <FEN>       value of the position and of every legal move (exact DTM)
//   posw <FEN>      win / draw / loss of the position and of every legal move (faster above the slices)
//   wdl <FEN>       win / draw / loss of the position
//   moves <FEN>     legal moves with the resulting positions (no values)
//   dtm <FEN>       value of the position only
//   ondemand <0|1>  allow solving slices that are not prebuilt (default 1); with 0 such a query
//                   answers "error not in database"
//   pv <FEN>        principal variation
//   slice <FEN>     statistics of the slice containing the position
//   slicemax <n>    largest number of pieces for which every position comes from the slices
//   verify [t]      recompute every value in every slice from its successors
//   stats           number of slices and positions computed so far
//   store <dir>     use the compressed units in dir (all positions with at most 12 pieces, at most 6 per side;
//                   units that are not there answer "error not in database")
//   unitbuild <dir> <n> <m> [t] [GB]     build unit (n pieces, m men) as a raw file (continues an interrupted build);
//                                        t threads, at most GB of memory for the threads (0 = no limit)
//   unitverify <dir> <n> <m> raw|cdb [t] recompute the values of the unit from their successors
//   unitcompress <dir> <n> <m> [t]       compress the raw file of the unit
//   unitcompare <dir> <n> <m> [t]        compare every block of the compressed unit with the raw file
//   unitcrc <dir> <n> <m>                CRC32C of the stored bytes of the compressed unit (independent of zstd)
//   unitmax <dir> <n> <m> [t]            per material the longest win and the longest loss, with a position for each
//   unitvalues <dir> <n> <m> [t]         a checksum of the values of the positions without capture (independent of how
//                                        the values are encoded: compares a store built with EGDB_DTM8_MAX to a normal one)
//   perft <depth> [FEN] [dups]           leaf counts to each depth from the position (default: the start position);
//                                        with "dups" every capture sequence counts, also when two give the same
//                                        position (as the checkers perft of dctl and the forum thread)
// FEN: W:W17,K16:B2,K15 (side to move, white pieces, black pieces; K = king).
// Values: "W<n>" win in n plies, "L<n>" loss in n plies, "D" draw (for the side to move).

#include <xstd/bits/bit/bit_compress.hpp>          // bit_compress
#include <xstd/bits/bit/bit_convert.hpp>           // bit_convert
#include <xstd/bits/bit/bit_expand.hpp>            // bit_expand
#include <xstd/bits/bit_set/bit_fixed_set.hpp>     // bit_fixed_set
#include <xstd/bits/bit_type_traits/bit_align.hpp> // bit_align
#include <xstd/bits/bit_type_traits/bit_least.hpp> // bit_least
#include <xstd/ints/bit/bit_reverse.hpp>           // bit_reverse
#include <algorithm>
#include <array>
#include <barrier>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <nmmintrin.h>
#include <omp.h>
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <execution>
#include "durable_file.hpp"

namespace {

constexpr int N = 18;

// the rules (see the top of the file)
#ifndef EGDB_6X6_CHECKERS
#define EGDB_6X6_CHECKERS 0
#endif
constexpr bool CHECKERS = EGDB_6X6_CHECKERS != 0;
constexpr bool LONG_KINGS = !CHECKERS;            // kings move and capture over any distance
constexpr bool MEN_CAPTURE_BACK = !CHECKERS;      // men capture backwards too
constexpr bool MAJORITY = !CHECKERS;              // the capture of the most pieces is compulsory
bool keep_duplicates = false;                     // perft only: keep moves that give the same position

// bitboards: the 18 squares in one std::uint32_t block (bit_least), widened to its 32 bits (bit_align); the full
// board is FULL, so a complement is FULL - x
using bitboard = xstd::bit_align<xstd::bit_least<xstd::bit_fixed_set<N>>>;
constexpr bitboard bb_of(std::uint32_t x) { return xstd::bit_convert<bitboard>(x); }
constexpr std::uint32_t u32(bitboard const& x) { return xstd::bit_convert<std::uint32_t>(x); }
bitboard square_bb(int s) { return bitboard{ std::size_t(s) }; }
constexpr bitboard FULL = bb_of((1u << N) - 1);

int row_of(int s) { return s / 3; }                                    // 0 = top row (squares 1-3)
int col_of(int s) { return 2 * (s % 3) + (row_of(s) % 2 == 0 ? 1 : 0); } // 0 = file a
int square_at(int r, int c)
{
        if (r < 0 || r >= 6 || c < 0 || c >= 6 || (r + c) % 2 == 0) return -1;
        return r * 3 + c / 2;
}

// directions: 0 = up-left, 1 = up-right, 2 = down-left, 3 = down-right
constexpr int DR[4] = { -1, -1, 1, 1 };
constexpr int DC[4] = { -1, 1, -1, 1 };

std::array<std::array<std::vector<int>, 4>, N> rays;
std::array<std::array<std::uint64_t, N + 1>, N + 1> binom{};

// colex ranks of the kings packed onto consecutive bits (xstd::bit_compress / bit_expand): the rank of a packed subset
// does not depend on the mask it was packed from, so one table of 2^18 ranks serves every mask, and per number of
// kings k one table of the packed subsets in rank order (2^18 entries over all k)
std::vector<std::uint32_t> rank_table, unrank_table;
std::array<std::uint32_t, N + 2> unrank_offset{};

void init_tables()
{
        for (int s = 0; s < N; ++s)
                for (int d = 0; d < 4; ++d)
                        for (int r = row_of(s) + DR[d], c = col_of(s) + DC[d]; square_at(r, c) >= 0; r += DR[d], c += DC[d])
                                rays[s][d].push_back(square_at(r, c));
        for (int n = 0; n <= N; ++n) {
                binom[n][0] = 1;
                for (int k = 1; k <= n; ++k) binom[n][k] = binom[n - 1][k - 1] + (k <= n - 1 ? binom[n - 1][k] : 0);
        }
        rank_table.assign(std::size_t(1) << N, 0);
        unrank_table.assign(std::size_t(1) << N, 0);
        for (std::uint32_t c = 0; c < (1u << N); ++c) {
                std::uint64_t r = 0;
                int i = 0;
                for (auto const sq : bb_of(c)) r += binom[std::size_t(sq)][std::size_t(++i)];
                rank_table[c] = std::uint32_t(r);
        }
        for (int k = 0; k <= N; ++k) unrank_offset[std::size_t(k) + 1] = unrank_offset[std::size_t(k)] + std::uint32_t(binom[N][std::size_t(k)]);
        for (std::uint32_t c = 0; c < (1u << N); ++c) unrank_table[unrank_offset[bb_of(c).size()] + rank_table[c]] = c;
}

constexpr bitboard PROMO_WHITE = bb_of(0b111);                // squares 1-3
constexpr bitboard PROMO_BLACK = bb_of(0b111u << 15);         // squares 16-18

struct Pos {
        bitboard m[2]{};        // men   [0] = white, [1] = black
        bitboard k[2]{};        // kings
        int stm = 0;            // 0 = white to move, 1 = black to move

        bitboard occ() const { return m[0] | m[1] | k[0] | k[1]; }
        bool operator==(Pos const&) const = default;
};

// 180 degree rotation with colours swapped: square n <-> 19 - n, white <-> black (the 32 bits reversed, shifted down)
bitboard flip(bitboard const& x)
{
        return bb_of(xstd::bit_reverse(u32(x)) >> (32 - N));
}

Pos mirror(Pos const& p)
{
        Pos q;
        q.m[0] = flip(p.m[1]); q.m[1] = flip(p.m[0]);
        q.k[0] = flip(p.k[1]); q.k[1] = flip(p.k[0]);
        q.stm = 1 - p.stm;
        return q;
}

// ---------------------------------------------------------------- move generation

struct Move {
        int from, to;
        bitboard captured;
        Pos next;
};

// the moves of a position, without heap allocation (at most 12 pieces on 18 squares: far fewer than 128 moves)
struct MoveList {
        static constexpr std::size_t CAP = 128;
        alignas(Move) unsigned char buf[CAP * sizeof(Move)];   // uninitialised: no zeroing of 128 moves per call
        std::size_t n = 0;
        Move* data() { return std::launder(reinterpret_cast<Move*>(buf)); }
        Move const* data() const { return std::launder(reinterpret_cast<Move const*>(buf)); }
        Move* begin() { return data(); }
        Move* end() { return data() + n; }
        Move const* begin() const { return data(); }
        Move const* end() const { return data() + n; }
        std::size_t size() const { return n; }
        bool empty() const { return n == 0; }
        Move& operator[](std::size_t i) { return data()[i]; }
        Move const& operator[](std::size_t i) const { return data()[i]; }
        void clear() { n = 0; }
        void push_back(Move const& m)
        {
                if (n == CAP) { std::cerr << "too many moves" << std::endl; std::abort(); }
                ::new (static_cast<void*>(data() + n)) Move(m);
                ++n;
        }
};

struct Gen {
        Pos const& p;
        int us, them;
        bitboard enemy, empty0;
        int best = 0;
        MoveList* out;
        int from = 0;
        bool is_king = false;

        Gen(Pos const& pp, MoveList* o) : p(pp), us(pp.stm), them(1 - pp.stm), out(o)
        {
                enemy = p.m[them] | p.k[them];
        }

        void emit(int to, bitboard const& cap, int n)
        {
                if constexpr (!MAJORITY) n = n > 0 ? 1 : 0;     // every capture counts the same, but a capture is compulsory
                if (n < best) return;
                if (n > best) { best = n; out->clear(); }
                Pos q = p;
                std::size_t const fb = std::size_t(from), tb = std::size_t(to);
                bool promote = !is_king && (us == 0 ? PROMO_WHITE : PROMO_BLACK).contains(tb);
                if (is_king) { q.k[us].erase(fb); q.k[us].insert(tb); }
                else if (promote) { q.m[us].erase(fb); q.k[us].insert(tb); }
                else { q.m[us].erase(fb); q.m[us].insert(tb); }
                q.m[them] -= cap; q.k[them] -= cap;
                q.stm = them;
                if (!keep_duplicates)
                        for (auto const& mv : *out) if (mv.next == q) return;  // same resulting position
                out->push_back({ from, to, cap, q });
        }

        // empty: squares that are empty during this capture (origin is vacated; captured pieces stay)
        void capture(int s, bitboard const& empty, bitboard const& cap, int n)
        {
                bool any = false;
                for (int d = 0; d < 4; ++d) {
                        if (!MEN_CAPTURE_BACK && !is_king && (us == 0 ? d >= 2 : d < 2)) continue;
                        auto const& ray = rays[s][d];
                        std::size_t i = 0;
                        if (LONG_KINGS && is_king) while (i < ray.size() && empty.contains(std::size_t(ray[i]))) ++i;
                        if (i >= ray.size()) continue;
                        int v = ray[i];
                        if (!enemy.contains(std::size_t(v)) || cap.contains(std::size_t(v))) continue;
                        std::size_t j = i + 1;
                        if (j >= ray.size() || !empty.contains(std::size_t(ray[j]))) continue;
                        for (; j < ray.size() && empty.contains(std::size_t(ray[j])); ++j) {
                                any = true;
                                capture(ray[j], empty, cap | square_bb(v), n + 1);
                                if (!LONG_KINGS || !is_king) break;
                        }
                }
                if (!any && n > 0) emit(s, cap, n);
        }

        void run()
        {
                bitboard const occ = p.occ();
                bitboard const pieces = p.m[us] | p.k[us];
                // captures
                for (auto const sq : pieces) {
                        from = int(sq);
                        is_king = p.k[us].contains(sq);
                        capture(from, (FULL - occ) | square_bb(from), bitboard{}, 0);
                }
                if (best > 0) return;
                // quiet moves
                for (auto const sq : pieces) {
                        from = int(sq);
                        is_king = p.k[us].contains(sq);
                        for (int d = 0; d < 4; ++d) {
                                if (!is_king && (us == 0 ? d >= 2 : d < 2)) continue;
                                for (int t : rays[from][d]) {
                                        if (occ.contains(std::size_t(t))) break;
                                        emit(t, bitboard{}, 0);
                                        if (!LONG_KINGS || !is_king) break;
                                }
                        }
                }
        }
};

MoveList moves(Pos const& p)
{
        MoveList out;
        Gen g(p, &out);
        g.run();
        return out;
}

bool has_capture(Pos const& p)
{
        int us = p.stm, them = 1 - us;
        bitboard const enemy = p.m[them] | p.k[them];
        bitboard const empty = FULL - p.occ();
        for (auto const sq : p.m[us]) {
                for (int d = 0; d < 4; ++d) {
                        if (!MEN_CAPTURE_BACK && (us == 0 ? d >= 2 : d < 2)) continue;
                        auto const& r = rays[std::size_t(sq)][std::size_t(d)];
                        if (r.size() >= 2 && enemy.contains(std::size_t(r[0])) && empty.contains(std::size_t(r[1]))) return true;
                }
        }
        for (auto const sq : p.k[us]) {
                for (int d = 0; d < 4; ++d) {
                        auto const& r = rays[std::size_t(sq)][std::size_t(d)];
                        std::size_t i = 0;
                        if constexpr (LONG_KINGS) while (i < r.size() && empty.contains(std::size_t(r[i]))) ++i;
                        if (i + 1 < r.size() && enemy.contains(std::size_t(r[i])) && empty.contains(std::size_t(r[i + 1]))) return true;
                }
        }
        return false;
}

// ---------------------------------------------------------------- values

// Values are stored in one byte: the DTM in plies (odd = win, even = loss for the side to move),
// DRAW8 or UNKNOWN8. The public interface uses 16 bits (DRAW = 0xFFFF) so that memoised positions
// above the slices are not limited to 252 plies.
constexpr std::uint8_t MAXDTM8 = 252;
constexpr std::uint8_t DRAW8 = 253;
constexpr std::uint8_t UNKNOWN8 = 254;
constexpr std::uint8_t NONE8 = 255;
constexpr std::uint16_t DRAW = 0xFFFF;
constexpr int INF = 1 << 30;

std::uint16_t widen(std::uint8_t v) { return v == DRAW8 ? DRAW : v; }

std::string show(std::uint16_t v)
{
        if (v == DRAW) return "D";
        return (v & 1 ? "W" : "L") + std::to_string(v);
}

// ---------------------------------------------------------------- slices

struct Key {
        bitboard wm, bm;
        int nwk, nbk;
        std::uint64_t pack() const { return std::uint64_t(u32(wm)) | std::uint64_t(u32(bm)) << 18 | std::uint64_t(nwk) << 36 | std::uint64_t(nbk) << 40; }
        static Key unpack(std::uint64_t x)
        {
                constexpr std::uint32_t low = (1u << N) - 1;
                return { bb_of(std::uint32_t(x) & low), bb_of(std::uint32_t(x >> 18) & low), int(x >> 36 & 15), int(x >> 40 & 15) };
        }
};

Key key_of(Pos const& p)
{
        return { p.m[0], p.m[1], int(p.k[0].size()), int(p.k[1].size()) };
}

int pieces(Key const& k) { return int(k.wm.size() + k.bm.size()) + k.nwk + k.nbk; }

// slices are stored for one of each pair (slice, mirror(slice)) only
bool canonical_key(Key const& k)
{
        Key m{ flip(k.bm), flip(k.wm), k.nbk, k.nwk };
        return k.pack() <= m.pack();
}

struct Meta {
        Key key;
        bitboard free;
        std::uint64_t nb;        // C(F - nwk, nbk)
        std::uint64_t P;         // positions per side to move
};

Meta meta_of(Key const& key)
{
        Meta s;
        s.key = key;
        s.free = FULL - (key.wm | key.bm);
        int F = int(s.free.size());
        s.nb = key.nwk <= F ? binom[F - key.nwk][key.nbk] : 0;
        s.P = key.nwk <= F ? binom[F][key.nwk] * s.nb : 0;
        return s;
}

// colex rank of x within mask (x subset of mask): x packed onto the low bits (PEXT), then one table entry
std::uint64_t rank_in(bitboard const& x, bitboard const& mask)
{
        return rank_table[xstd::bit_compress<std::uint32_t>(x, mask)];
}

// the k-subset of mask with colex rank r: one table entry, spread over the mask (PDEP)
bitboard unrank_in(std::uint64_t r, int k, bitboard const& mask)
{
        return xstd::bit_expand(unrank_table[unrank_offset[std::size_t(k)] + r], mask);
}

std::uint64_t index_of(Meta const& s, Pos const& p)
{
        std::uint64_t rw = rank_in(p.k[0], s.free);
        std::uint64_t rb = rank_in(p.k[1], s.free - p.k[0]);
        return rw * s.nb + rb;
}

Pos pos_of(Meta const& s, std::uint64_t idx, int stm)
{
        Pos p;
        p.m[0] = s.key.wm; p.m[1] = s.key.bm;
        p.k[0] = unrank_in(idx / s.nb, s.key.nwk, s.free);
        p.k[1] = unrank_in(idx % s.nb, s.key.nbk, s.free - p.k[0]);
        p.stm = stm;
        return p;
}

// ---------------------------------------------------------------- storage
//
// Prebuilt slices (every canonical slice with at most `built_max` pieces) are found through an
// open-addressing table key -> offset. Slices with at most `dtm_max` pieces store one byte of DTM per
// position in `db`; larger prebuilt slices store only win / draw / loss in two bits per position in
// `wdlbits` (Syzygy style), each slice starting at a 64-bit word so that slices can be written in
// parallel. Slices needed beyond the prebuilt ones (positions with kings on both sides and more pieces)
// are solved on demand and kept in `extra` with 16-bit values.
//
// Values above `dtm_max` pieces are pseudo distances: the win / draw / loss is exact, the number is not
// (a WDL slice reports a win as 1 and a loss as 0).

int built_max = 0;
int dtm_max = 99;
int slice_max = 8;
std::vector<std::uint8_t> db;
std::vector<std::uint64_t> wdlbits;                // 2 bits per position: 0 = loss, 1 = win, 2 = draw
constexpr std::uint64_t WDL_FLAG = 1ull << 63;
constexpr std::uint64_t DB2_FLAG = 1ull << 62;    // offset into db2: a layer of DTM slices in a memory-mapped file
std::uint8_t* db2 = nullptr;
std::size_t db2_size = 0;
std::vector<std::uint64_t> tab_key, tab_off;      // tab_key = pack + 1 (0 = empty); tab_off = byte offset or WDL_FLAG | position offset
std::unordered_map<std::uint64_t, std::vector<std::uint16_t>> extra;
std::uint64_t extra_positions = 0;
bool allow_on_demand = true;   // false: a slice that is not prebuilt is an error (no on-demand solving)
struct NotInDatabase {};

struct SliceRef {
        std::uint8_t* dtm = nullptr;          // DTM bytes
        std::uint16_t const* val16 = nullptr; // on-demand slice
        std::uint64_t wdl = ~0ull;            // position offset in wdlbits
};

std::uint16_t ref_value(SliceRef const& r, std::uint64_t i)
{
        if (r.dtm) return widen(r.dtm[i]);
        if (r.val16) return r.val16[i];
        std::uint64_t j = r.wdl + i;
        unsigned c = unsigned(wdlbits[j >> 5] >> ((j & 31) * 2)) & 3;
        return c == 2 ? DRAW : std::uint16_t(c);
}

std::size_t tab_slot(std::uint64_t k)
{
        std::uint64_t h = k * 0x9E3779B97F4A7C15ull;
        return std::size_t(h >> 20) & (tab_key.size() - 1);
}

bool find_prebuilt(std::uint64_t k, SliceRef& r)
{
        if (tab_key.empty()) return false;
        for (std::size_t i = tab_slot(k);; i = (i + 1) & (tab_key.size() - 1)) {
                if (tab_key[i] == k + 1) {
                        if (tab_off[i] & WDL_FLAG) r.wdl = tab_off[i] & ~WDL_FLAG;
                        else if (tab_off[i] & DB2_FLAG) r.dtm = db2 + (tab_off[i] & ~DB2_FLAG);
                        else r.dtm = db.data() + tab_off[i];
                        return true;
                }
                if (tab_key[i] == 0) return false;
        }
}

void solve_slice(Meta const& s, std::uint16_t* out);

SliceRef slice_values(Key const& key)
{
        std::uint64_t k = key.pack();
        SliceRef r;
        if (pieces(key) <= built_max && find_prebuilt(k, r)) return r;
        auto it = extra.find(k);
        if (it == extra.end()) {
                if (!allow_on_demand) throw NotInDatabase{};
                Meta s = meta_of(key);
                std::vector<std::uint16_t> val(2 * s.P);
                solve_slice(s, val.data());
                extra_positions += 2 * s.P;
                if ((extra.size() + 1) % 10000 == 0)
                        std::cerr << "on demand: " << extra.size() + 1 << " slices, " << extra_positions << " positions" << std::endl;
                it = extra.emplace(k, std::move(val)).first;
        }
        r.val16 = it->second.data();
        return r;
}

// ---------------------------------------------------------------- lookup
//
// Positions with at most `slice_max` pieces, or with kings on both sides, come from the slices: only
// when both sides have a king can a position repeat. Other positions cannot repeat (the side without
// kings has to move a man or capture), so their exact value, with DTM, follows from a forward
// alpha-beta search down to the slices (see below).

using PosKey = unsigned __int128;
PosKey pack(Pos const& p)
{
        return PosKey(u32(p.m[0])) | PosKey(u32(p.m[1])) << 18 | PosKey(u32(p.k[0])) << 36 | PosKey(u32(p.k[1])) << 54 | PosKey(p.stm) << 72;
}
struct PosKeyHash {
        std::size_t operator()(PosKey x) const
        {
                std::uint64_t h = std::uint64_t(x) * 0x9E3779B97F4A7C15ull ^ std::uint64_t(x >> 64) * 0xC2B2AE3D27D4EB4Full;
                return std::size_t(h ^ (h >> 31));
        }
};

// ---------------------------------------------------------------- forward alpha-beta with exact DTM
//
// Scores for the side to move: win in n plies = WSC - n, loss in n plies = -(WSC - n), draw = 0.
// A child's score s becomes up(s) for the parent (a win in n for the child is a loss in n + 1 for the
// parent and vice versa); a window bound b of the parent becomes down(b) for the child (the inverse of
// the decreasing function up). The leaves (slices) are exact and the searched positions cannot repeat,
// so the tree is acyclic and alpha-beta with a full window returns the exact DTM; the transposition
// table keeps lower and upper bounds relative to the position itself, so no ply correction is needed.

constexpr int WSC = 30000, INFS = 32000;
int to_score(std::uint16_t v) { return v == DRAW ? 0 : (v & 1) ? WSC - v : -(WSC - v); }
std::uint16_t from_score(int s) { return s == 0 ? DRAW : std::uint16_t(s > 0 ? WSC - s : WSC + s); }
int up(int s) { return s > 0 ? 1 - s : s < 0 ? -s - 1 : 0; }
int down(int b)
{
        if (b >= INFS) return -INFS;
        if (b <= -INFS) return INFS;
        return b > 0 ? -b - 1 : b < 0 ? 1 - b : 0;
}
struct Bounds { std::int16_t lo = -INFS, hi = INFS; };
std::unordered_map<PosKey, Bounds, PosKeyHash> tt;
std::uint64_t ab_nodes = 0;

bool in_slices(Pos const& p)
{
        return int(p.occ().size()) <= slice_max || (!p.k[0].empty() && !p.k[1].empty());
}

std::uint16_t from_successors(Pos const& p);
bool store_on = false;                 // positions come from the store of compressed units (see below)
std::uint16_t store_lookup(Pos const& p);

std::uint16_t lookup_slice(Pos const& p0)
{
        if (store_on) return store_lookup(p0);
        Pos const p = canonical_key(key_of(p0)) ? p0 : mirror(p0);
        Key const key = key_of(p);
        thread_local SliceRef last;
        thread_local std::uint64_t last_key = ~0ull;
        thread_local Meta last_meta;
        if (key.pack() != last_key) { last = slice_values(key); last_key = key.pack(); last_meta = meta_of(key); }
        return ref_value(last, p.stm * last_meta.P + index_of(last_meta, p));
}

int alphabeta(Pos const& p, int alpha, int beta)
{
        if (in_slices(p)) return to_score((lookup_slice(p)));
        PosKey const key = std::min(pack(p), pack(mirror(p)));
        if (auto it = tt.find(key); it != tt.end()) {
                Bounds const b = it->second;
                if (b.lo == b.hi || b.lo >= beta) return b.lo;
                if (b.hi <= alpha) return b.hi;
                alpha = std::max(alpha, int(b.lo));
                beta = std::min(beta, int(b.hi));
        }
        if (++ab_nodes % 1000000 == 0) std::cerr << "alphabeta: " << ab_nodes << " nodes, tt " << tt.size() << std::endl;
        auto ms = moves(p);
        int best = -INFS;
        if (ms.empty()) best = -WSC;
        // order: children with a known exact value first (best first), then captures, then the rest
        std::vector<std::pair<int, std::size_t>> order;
        std::vector<int> exact(ms.size(), INFS);
        for (std::size_t i = 0; i < ms.size(); ++i) {
                int pri;
                if (in_slices(ms[i].next)) { exact[i] = up(to_score((lookup_slice(ms[i].next)))); pri = exact[i]; }
                else pri = !ms[i].captured.empty() ? -WSC + 1000 : -WSC + 500;
                order.push_back({ -pri, i });
        }
        std::sort(order.begin(), order.end());
        int const alpha0 = alpha;
        for (auto const& [pri, i] : order) {
                int a = std::max(alpha, best);
                int s = exact[i] != INFS ? exact[i] : up(alphabeta(ms[i].next, down(beta), down(a)));
                best = std::max(best, s);
                if (best >= beta) break;
        }
        Bounds& b = tt[key];
        if (best <= alpha0) b.hi = std::int16_t(std::min(int(b.hi), best));
        else if (best >= beta) b.lo = std::int16_t(std::max(int(b.lo), best));
        else b.lo = b.hi = std::int16_t(best);
        return best;
}

std::uint16_t lookup(Pos const& p0)
{
        if (in_slices(p0)) return (lookup_slice(p0));
        return from_score(alphabeta(p0, -INFS, INFS));
}

// win / draw / loss only: two null-window searches (prunes far more than the exact DTM search)
char wdl(Pos const& p)
{
        if (in_slices(p)) { std::uint16_t v = (lookup_slice(p)); return v == DRAW ? 'D' : (v & 1) ? 'W' : 'L'; }
        if (alphabeta(p, 0, 1) >= 1) return 'W';
        if (alphabeta(p, -1, 0) <= -1) return 'L';
        return 'D';
}

// value of a position recomputed from the values of its successors
std::uint16_t from_successors(Pos const& p)
{
        int best_win = INF, max_loss = -1;
        bool draw = false;
        for (auto const& mv : moves(p)) {
                std::uint16_t v = lookup(mv.next);
                if (v == DRAW) draw = true;
                else if (v & 1) max_loss = std::max(max_loss, v + 1);
                else best_win = std::min(best_win, v + 1);
        }
        return best_win < INF ? std::uint16_t(best_win) : draw ? DRAW : std::uint16_t(std::max(max_loss, 0));
}

// ---------------------------------------------------------------- retrograde analysis of one slice
//
// Leapfrog iterations with one byte per position:
//   pass 0   every position: moves that leave the slice (man moves, captures, promotions) are looked up;
//            positions without king moves inside the slice are decided at once; for the others the
//            fastest win through an exit is remembered (temporary byte).
//   n odd    wins in n: predecessors (king unmoves) of positions lost in n - 1, and positions whose best
//            exit wins in n.
//   n even   losses: the predecessors of positions won in n - 1 are collected as candidates in a bitmap
//            and then verified by forward move generation: lost iff every move leads to a position won
//            for the opponent; the DTM is one more than the slowest of those wins (it can exceed n: the
//            position is then decided early, with its exact value, and acts as frontier in that iteration).
// The DTM array is updated with the new results in every iteration; what remains undecided is a draw.

bool internal_move(Pos const& p, Move const& mv)
{
        return mv.captured.empty() && p.k[p.stm].contains(std::size_t(mv.from));
}

template<class F>
void for_each_predecessor(Meta const& s, Pos const& q, F&& f)
{
        int mover = 1 - q.stm;
        bitboard const occ = q.occ();
        for (auto const t : q.k[mover]) {
                for (int d = 0; d < 4; ++d)
                        for (int fr : rays[std::size_t(t)][std::size_t(d)]) {
                                if (occ.contains(std::size_t(fr))) break;
                                Pos pr = q;
                                pr.k[mover].erase(t);
                                pr.k[mover].insert(std::size_t(fr));
                                pr.stm = mover;
                                if (!has_capture(pr)) f(std::uint64_t(mover) * s.P + index_of(s, pr));  // captures are compulsory
                                if (!LONG_KINGS) break;
                        }
        }
}

constexpr std::uint16_t UNKNOWN16 = 0xFFFE;
constexpr std::uint16_t NONE16 = 0xFFFF;

void solve_slice(Meta const& s, std::uint16_t* out)
{
        std::uint64_t T = 2 * s.P;
        std::vector<std::uint16_t> ewin(T, NONE16);
        int maxd = 0;
        for (std::uint64_t i = 0; i < T; ++i) {
                Pos p = pos_of(s, i % s.P, int(i / s.P));
                int c = 0, ew = INF, el = 0;
                bool ed = false;
                for (auto const& mv : moves(p)) {
                        if (internal_move(p, mv)) { ++c; continue; }
                        std::uint16_t v = lookup(mv.next);
                        if (v == DRAW) ed = true;
                        else if (v & 1) el = std::max(el, v + 1);
                        else ew = std::min(ew, v + 1);
                }
                if (c == 0) {
                        out[i] = ew < INF ? std::uint16_t(ew) : ed ? DRAW : std::uint16_t(el);
                        if (out[i] != DRAW) maxd = std::max(maxd, int(out[i]));
                } else {
                        out[i] = UNKNOWN16;
                        if (ew < INF) { ewin[i] = std::uint16_t(ew); maxd = std::max(maxd, ew); }
                }
        }
        if (maxd >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
        std::vector<std::uint64_t> cand((T + 63) / 64);
        for (int n = 1; n <= maxd + 1; ++n) {
                std::uint16_t const prev = std::uint16_t(n - 1);
                bool const wins = n & 1;
                for (std::uint64_t i = 0; i < T; ++i) {
                        if (out[i] != prev) continue;
                        Pos q = pos_of(s, i % s.P, int(i / s.P));
                        for_each_predecessor(s, q, [&](std::uint64_t j) {
                                if (out[j] != UNKNOWN16) return;
                                if (wins) { out[j] = std::uint16_t(n); maxd = std::max(maxd, n); }
                                else cand[j / 64] |= 1ull << (j % 64);
                        });
                }
                if (wins) {
                        for (std::uint64_t i = 0; i < T; ++i)
                                if (ewin[i] == n && out[i] == UNKNOWN16) { out[i] = std::uint16_t(n); maxd = std::max(maxd, n); }
                        continue;
                }
                for (std::uint64_t w = 0; w < cand.size(); ++w) {
                        for (std::uint64_t b = cand[w]; b; b &= b - 1) {
                                std::uint64_t j = w * 64 + std::uint64_t(__builtin_ctzll(b));
                                if (out[j] != UNKNOWN16) continue;
                                Pos p = pos_of(s, j % s.P, int(j / s.P));
                                int worst = 0;
                                bool lost = true;
                                for (auto const& mv : moves(p)) {
                                        std::uint16_t v = internal_move(p, mv)
                                                ? out[std::uint64_t(1 - p.stm) * s.P + index_of(s, mv.next)]
                                                : lookup(mv.next);
                                        if (v >= UNKNOWN16 || !(v & 1)) { lost = false; break; }
                                        worst = std::max(worst, int(v));
                                }
                                if (lost) {
                                        out[j] = std::uint16_t(worst + 1);
                                        maxd = std::max(maxd, worst + 1);
                                }
                        }
                        cand[w] = 0;
                }
                if (maxd >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
        }
        for (std::uint64_t i = 0; i < T; ++i)
                if (out[i] == UNKNOWN16) out[i] = DRAW;
}

// ---------------------------------------------------------------- one slice by pull passes, in parallel
//
// The same values as solve_slice, for one big slice with all cores (std::execution::par, no other synchronisation).
// Every round reads the values of the previous round and writes a new array, each position only its own entry:
//   pass 0   as in solve_slice; besides the fastest exit win (ewin) also eloss, for a position whose exits all lose
//            for the side to move (all lead to a win of the opponent): one more than the slowest of those wins
//   n odd    an undecided position is won in n if its fastest exit wins in n or a king move reaches a position lost
//            in n - 1
//   n even   an undecided position whose exits all lose is lost if every king move reaches a won position; its DTM is
//            the larger of eloss and one more than the slowest of those wins (it can exceed n, as in solve_slice)
// Each round scans every undecided position (more work than the frontier of solve_slice), in chunks of 4096.

void solve_slice_pull(Meta const& s, std::uint16_t* out)
{
        std::uint64_t const T = 2 * s.P;
        std::vector<std::uint64_t> chunks;
        for (std::uint64_t c = 0; c < T; c += 4096) chunks.push_back(c);
        std::vector<std::uint16_t> cur(T), nxt(T), ewin(T, NONE16), eloss(T, NONE16);
        auto const top = [](int a, int b) { return std::max(a, b); };
        int maxd = std::transform_reduce(std::execution::par, chunks.begin(), chunks.end(), 0, top, [&](std::uint64_t c0) {
                int md = 0;
                for (std::uint64_t i = c0; i < std::min(T, c0 + 4096); ++i) {
                        Pos const p = pos_of(s, i % s.P, int(i / s.P));
                        int c = 0, ew = INF, el = 0;
                        bool ed = false;
                        for (auto const& mv : moves(p)) {
                                if (internal_move(p, mv)) { ++c; continue; }
                                std::uint16_t const v = lookup(mv.next);
                                if (v == DRAW) ed = true;
                                else if (v & 1) el = std::max(el, v + 1);
                                else ew = std::min(ew, v + 1);
                        }
                        if (c == 0) {
                                cur[i] = ew < INF ? std::uint16_t(ew) : ed ? DRAW : std::uint16_t(el);
                                if (cur[i] != DRAW) md = std::max(md, int(cur[i]));
                        } else {
                                cur[i] = UNKNOWN16;
                                if (ew < INF) { ewin[i] = std::uint16_t(ew); md = std::max(md, ew); }
                                else if (!ed) eloss[i] = std::uint16_t(el);
                        }
                }
                return md;
        });
        if (maxd >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
        for (int n = 1; n <= maxd + 1; ++n) {
                bool const wins = n & 1;
                std::uint16_t const prev = std::uint16_t(n - 1);
                int const md = std::transform_reduce(std::execution::par, chunks.begin(), chunks.end(), 0, top, [&](std::uint64_t c0) {
                        int md = 0;
                        for (std::uint64_t i = c0; i < std::min(T, c0 + 4096); ++i) {
                                std::uint16_t v = cur[i];
                                if (v == UNKNOWN16 && (wins || eloss[i] != NONE16)) {
                                        Pos const p = pos_of(s, i % s.P, int(i / s.P));
                                        std::uint64_t const other = std::uint64_t(1 - p.stm) * s.P;
                                        if (wins) {
                                                bool won = ewin[i] == n;
                                                for (auto const& mv : moves(p)) {
                                                        if (won) break;
                                                        if (internal_move(p, mv) && cur[other + index_of(s, mv.next)] == prev) won = true;
                                                }
                                                if (won) { v = std::uint16_t(n); md = std::max(md, n); }
                                        } else {
                                                int d = eloss[i];
                                                bool lost = true;
                                                for (auto const& mv : moves(p)) {
                                                        if (!internal_move(p, mv)) continue;
                                                        std::uint16_t const w = cur[other + index_of(s, mv.next)];
                                                        if (w >= UNKNOWN16 || !(w & 1)) { lost = false; break; }
                                                        d = std::max(d, int(w) + 1);
                                                }
                                                if (lost) { v = std::uint16_t(d); md = std::max(md, d); }
                                        }
                                }
                                nxt[i] = v;
                        }
                        return md;
                });
                maxd = std::max(maxd, md);
                if (maxd >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
                cur.swap(nxt);
        }
        for (std::uint64_t i = 0; i < T; ++i) out[i] = cur[i] == UNKNOWN16 ? DRAW : cur[i];
}

// ---------------------------------------------------------------- one slice solved by several threads
//
// A persistent pool of std::jthread workers (the calling thread takes part as worker 0), so that the per-thread caches
// (blocks, resolved captures) survive from one slice to the next. Synchronisation is std::mutex/condition_variable for
// starting and finishing a job and std::barrier between the phases of a job, all understood by ThreadSanitizer.

class Pool {
public:
        explicit Pool(int n) : n_(n)
        {
                for (int t = 1; t < n; ++t) th_.emplace_back([this, t] { loop(t); });
        }
        ~Pool()
        {
                { std::lock_guard l(m_); stop_ = true; ++gen_; }
                cv_.notify_all();
                th_.clear();                    // join the workers while the members they use still exist
        }
        int size() const { return n_; }
        void run(std::function<void(int)> const& job)
        {
                {
                        std::lock_guard l(m_);
                        job_ = &job;
                        left_ = n_ - 1;
                        ++gen_;
                }
                cv_.notify_all();
                job(0);
                std::unique_lock l(m_);
                done_.wait(l, [&] { return left_ == 0; });
                job_ = nullptr;
        }
private:
        void loop(int t)
        {
                std::uint64_t seen = 0;
                for (;;) {
                        std::function<void(int)> const* j;
                        {
                                std::unique_lock l(m_);
                                cv_.wait(l, [&] { return gen_ != seen; });
                                seen = gen_;
                                if (stop_) return;
                                j = job_;
                        }
                        (*j)(t);
                        std::lock_guard l(m_);
                        if (--left_ == 0) done_.notify_one();
                }
        }
        int n_;
        std::mutex m_;
        std::condition_variable cv_, done_;
        std::uint64_t gen_ = 0;
        int left_ = 0;
        bool stop_ = false;
        std::function<void(int)> const* job_ = nullptr;
        std::vector<std::jthread> th_;          // last: destroyed (joined) first
};

// The leapfrog of solve_slice, level-synchronous and race-free, with the same result for any number of threads (and
// the same as solve_slice). The positions are cut into blocks of 512 positions (8 words of a bitmap, one cache line of
// bits). The cheap phases (scan, ewin) give thread t a fixed contiguous range of blocks; the others hand out blocks one
// at a time, and a thread writes only the positions and words of the blocks it claimed. Per iteration n:
//   scan    each thread marks its positions with value n - 1 in the frontier bitmap (own words only);
//   push    from its frontier positions each thread walks the king unmoves: a predecessor that is still unknown becomes
//           a win in n (n odd: atomic_ref<uint16_t> relaxed load and store, every writer writes the same value) or a
//           loss candidate (n even: atomic_ref<uint64_t> relaxed fetch_or on the candidate bitmap); nothing else is
//           written in this phase;
//   n odd   each thread applies the wins through an exit (ewin) to its own positions;
//   n even  each thread verifies its own candidates (successors are only read in this phase) into a private list,
//           and after a barrier writes the losses to its own positions. A successor candidate decided in the same
//           phase would read as unknown or as a loss: either way "not lost", as in the sequential order.
// Barriers separate the phases, so no value is read while another thread writes it.
void solve_slice_par(Meta const& s, std::uint16_t* out, Pool& pool)
{
        static_assert(sizeof(std::size_t) == 8, "64-bit indices");
        static_assert(std::atomic_ref<std::uint64_t>::is_always_lock_free && std::atomic_ref<std::uint16_t>::is_always_lock_free);
        std::uint64_t const T = 2 * s.P, W = (T + 63) / 64, B = (W + 7) / 8;     // positions, words, blocks of 8 words
        int const nt = pool.size();
        std::vector<std::uint16_t> ewin(T, NONE16);
        std::vector<std::uint64_t> front(W, 0), cand(W, 0);
        std::vector<int> tmax(static_cast<std::size_t>(nt), 0);
        std::vector<std::vector<std::pair<std::uint64_t, std::uint16_t>>> res(static_cast<std::size_t>(nt));
        std::barrier<> bar(nt);
        // blocks handed out one at a time in the phases whose cost per position varies (pass 0, push, verify): the
        // writes of a block stay with the thread that claimed it; the counters are reset in the scan phase, between
        // barriers
        std::atomic<std::uint64_t> next0{ 0 }, next_push{ 0 }, next_verify{ 0 };
        pool.run([&](int t) {
                std::uint64_t const w0 = std::min(W, B * std::uint64_t(t) / std::uint64_t(nt) * 8);
                std::uint64_t const w1 = std::min(W, B * std::uint64_t(t + 1) / std::uint64_t(nt) * 8);
                std::uint64_t const i0 = std::min(T, w0 * 64), i1 = std::min(T, w1 * 64);
                auto claim = [&](std::atomic<std::uint64_t>& next, std::uint64_t& wa, std::uint64_t& wb) {
                        std::uint64_t const k = next.fetch_add(1, std::memory_order_relaxed);
                        if (k >= B) return false;
                        wa = k * 8; wb = std::min(W, wa + 8);
                        return true;
                };
                int mx = 0;
                std::uint64_t wa = 0, wb = 0;
                // pass 0: exits looked up; positions without king moves inside the slice decided at once
                while (claim(next0, wa, wb))
                for (std::uint64_t i = wa * 64; i < std::min(T, wb * 64); ++i) {
                        Pos p = pos_of(s, i % s.P, int(i / s.P));
                        int c = 0, ew = INF, el = 0;
                        bool ed = false;
                        for (auto const& mv : moves(p)) {
                                if (internal_move(p, mv)) { ++c; continue; }
                                std::uint16_t v = lookup(mv.next);
                                if (v == DRAW) ed = true;
                                else if (v & 1) el = std::max(el, v + 1);
                                else ew = std::min(ew, v + 1);
                        }
                        if (c == 0) {
                                out[i] = ew < INF ? std::uint16_t(ew) : ed ? DRAW : std::uint16_t(el);
                                if (out[i] != DRAW) mx = std::max(mx, int(out[i]));
                        } else {
                                out[i] = UNKNOWN16;
                                if (ew < INF) { ewin[i] = std::uint16_t(ew); mx = std::max(mx, ew); }
                        }
                }
                tmax[std::size_t(t)] = mx;
                bar.arrive_and_wait();
                auto global_max = [&] { int g = 0; for (int x : tmax) g = std::max(g, x); return g; };
                if (global_max() >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
                for (int n = 1; n <= global_max() + 1; ++n) {
                        std::uint16_t const prev = std::uint16_t(n - 1);
                        bool const wins = n & 1;
                        if (t == 0) { next_push.store(0, std::memory_order_relaxed); next_verify.store(0, std::memory_order_relaxed); }
                        for (std::uint64_t w = w0; w < w1; ++w) {             // scan
                                std::uint64_t bits = 0;
                                for (std::uint64_t i = w * 64; i < std::min(T, w * 64 + 64); ++i)
                                        if (out[i] == prev) bits |= 1ull << (i - w * 64);
                                front[w] = bits;
                        }
                        bar.arrive_and_wait();
                        while (claim(next_push, wa, wb))                    // push
                        for (std::uint64_t w = wa; w < wb; ++w)
                                for (std::uint64_t b = front[w]; b; b &= b - 1) {
                                        std::uint64_t const i = w * 64 + std::uint64_t(__builtin_ctzll(b));
                                        for_each_predecessor(s, pos_of(s, i % s.P, int(i / s.P)), [&](std::uint64_t j) {
                                                std::atomic_ref<std::uint16_t> a(out[j]);
                                                if (a.load(std::memory_order_relaxed) != UNKNOWN16) return;
                                                if (wins) { a.store(std::uint16_t(n), std::memory_order_relaxed); mx = std::max(mx, n); }
                                                else std::atomic_ref<std::uint64_t>(cand[j / 64]).fetch_or(1ull << (j % 64), std::memory_order_relaxed);
                                        });
                                }
                        bar.arrive_and_wait();
                        if (wins) {
                                for (std::uint64_t i = i0; i < i1; ++i)
                                        if (ewin[i] == n && out[i] == UNKNOWN16) { out[i] = std::uint16_t(n); mx = std::max(mx, n); }
                        } else {
                                auto& r = res[std::size_t(t)];
                                r.clear();
                                while (claim(next_verify, wa, wb))
                                for (std::uint64_t w = wa; w < wb; ++w)
                                        for (std::uint64_t b = cand[w]; b; b &= b - 1) {
                                                std::uint64_t const j = w * 64 + std::uint64_t(__builtin_ctzll(b));
                                                if (out[j] != UNKNOWN16) continue;
                                                Pos p = pos_of(s, j % s.P, int(j / s.P));
                                                int worst = 0;
                                                bool lost = true;
                                                for (auto const& mv : moves(p)) {
                                                        std::uint16_t v = internal_move(p, mv)
                                                                ? out[std::uint64_t(1 - p.stm) * s.P + index_of(s, mv.next)]
                                                                : lookup(mv.next);
                                                        if (v >= UNKNOWN16 || !(v & 1)) { lost = false; break; }
                                                        worst = std::max(worst, int(v));
                                                }
                                                if (lost) r.push_back({ j, std::uint16_t(worst + 1) });
                                        }
                                bar.arrive_and_wait();
                                for (auto const& [j, v] : r) { out[j] = v; mx = std::max(mx, int(v)); }
                                for (std::uint64_t w = w0; w < w1; ++w) cand[w] = 0;
                        }
                        tmax[std::size_t(t)] = mx;
                        bar.arrive_and_wait();
                        if (global_max() >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
                }
                for (std::uint64_t i = i0; i < i1; ++i)
                        if (out[i] == UNKNOWN16) out[i] = DRAW;
        });
}

// ---------------------------------------------------------------- building all slices up to n pieces

struct Job { std::uint64_t key; int level_n, men, dist; std::uint64_t P; };

int distance(Key const& k)
{
        int d = 0;
        for (auto const sq : k.wm) d += int(sq) / 3;           // white: rows to row 0
        for (auto const sq : k.bm) d += 5 - int(sq) / 3;       // black: rows to row 5
        return d;
}

// all canonical slices with at most nmax pieces; slices with more than dmax pieces are stored as WDL
// all canonical slices with nmin..nmax pieces, in dependency order
std::vector<Job> make_jobs(int nmin, int nmax)
{
        std::vector<Job> jobs;
        // the subsets of a mask by the integer enumeration s = (s - 1) & mask, from the mask down to the empty set
        std::uint32_t const allowW = u32(FULL - PROMO_WHITE), allowB = u32(FULL - PROMO_BLACK);
        std::vector<bitboard> wsets;
        for (std::uint32_t w = allowW;; w = (w - 1) & allowW) {
                if (int(bb_of(w).size()) <= nmax) wsets.push_back(bb_of(w));
                if (w == 0) break;
        }
        for (bitboard const& wm : wsets) {
                std::uint32_t const ab = allowB & ~u32(wm);
                int rest0 = nmax - int(wm.size());
                for (std::uint32_t b = ab;; b = (b - 1) & ab) {
                        bitboard const bm = bb_of(b);
                        int rest = rest0 - int(bm.size());
                        if (rest >= 0)
                                for (int nwk = 0; nwk <= rest; ++nwk)
                                        for (int nbk = 0; nwk + nbk <= rest; ++nbk) {
                                                Key k{ wm, bm, nwk, nbk };
                                                if (pieces(k) < std::max(1, nmin) || !canonical_key(k)) continue;
                                                Meta m = meta_of(k);
                                                if (m.P == 0) continue;
                                                jobs.push_back({ k.pack(), pieces(k), int((wm | bm).size()), distance(k), m.P });
                                        }
                        if (b == 0) break;
                }
        }
        std::sort(jobs.begin(), jobs.end(), [](Job const& a, Job const& b) {
                return std::tie(a.level_n, a.men, a.dist, a.key) < std::tie(b.level_n, b.men, b.dist, b.key);
        });
        return jobs;
}

void build(int nmax, int threads, int dmax)
{
        std::vector<Job> jobs = make_jobs(1, nmax);
        std::size_t cap = 1;
        while (cap < 2 * jobs.size()) cap <<= 1;
        tab_key.assign(cap, 0);
        tab_off.assign(cap, 0);
        std::uint64_t bytes = 0, wpos = 0, total = 0;
        for (auto const& j : jobs) {
                std::size_t i = tab_slot(j.key);
                while (tab_key[i]) i = (i + 1) & (cap - 1);
                tab_key[i] = j.key + 1;
                if (j.level_n <= dmax) { tab_off[i] = bytes; bytes += 2 * j.P; }
                else { tab_off[i] = WDL_FLAG | wpos; wpos += (2 * j.P + 31) / 32 * 32; }
                total += 2 * j.P;
        }
        std::cerr << "build: " << jobs.size() << " slices, " << total << " positions; DTM " << bytes / 1e9
                  << " GB, WDL " << wpos / 4 / 1e9 << " GB" << std::endl;
        db.assign(bytes, UNKNOWN8);
        wdlbits.assign(wpos / 32, 0);
        built_max = nmax;
        dtm_max = dmax;
        slice_max = std::max(slice_max, nmax);
        auto t0 = std::chrono::steady_clock::now();
        std::uint64_t done = 0;
        for (std::size_t g = 0; g < jobs.size();) {
                std::size_t h = g;
                while (h < jobs.size() && jobs[h].level_n == jobs[g].level_n && jobs[h].men == jobs[g].men && jobs[h].dist == jobs[g].dist) ++h;
                #pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
                for (std::size_t i = g; i < h; ++i) {
                        Meta m = meta_of(Key::unpack(jobs[i].key));
                        std::vector<std::uint16_t> tmp(2 * m.P);
                        solve_slice(m, tmp.data());
                        SliceRef r;
                        find_prebuilt(jobs[i].key, r);
                        if (r.dtm) {
                                for (std::uint64_t x = 0; x < tmp.size(); ++x) {
                                        if (tmp[x] != DRAW && tmp[x] > MAXDTM8) { std::cerr << "DTM overflow (" << tmp[x] << ")" << std::endl; std::abort(); }
                                        r.dtm[x] = tmp[x] == DRAW ? DRAW8 : std::uint8_t(tmp[x]);
                                }
                        } else {
                                for (std::uint64_t x = 0; x < tmp.size(); ++x) {
                                        std::uint64_t c = tmp[x] == DRAW ? 2 : (tmp[x] & 1);
                                        std::uint64_t y = r.wdl + x;
                                        wdlbits[y >> 5] |= c << ((y & 31) * 2);
                                }
                        }
                }
                for (std::size_t i = g; i < h; ++i) done += 2 * jobs[i].P;
                if (h == jobs.size() || jobs[h].level_n != jobs[g].level_n || jobs[h].men != jobs[g].men) {
                        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                        std::cerr << "build: " << jobs[g].level_n << " pieces, " << jobs[g].men << " men done, " << done << " / " << total
                                  << " positions, " << sec << " s" << std::endl;
                }
                g = h;
        }
}

// insert (key, offset) pairs into a fresh table with room for all of them
void rebuild_table(std::vector<std::pair<std::uint64_t, std::uint64_t>> const& entries)
{
        std::size_t cap = 1;
        while (cap < 2 * entries.size()) cap <<= 1;
        tab_key.assign(cap, 0);
        tab_off.assign(cap, 0);
        for (auto const& [k, off] : entries) {
                std::size_t i = tab_slot(k);
                while (tab_key[i]) i = (i + 1) & (cap - 1);
                tab_key[i] = k + 1;
                tab_off[i] = off;
        }
}

std::vector<std::pair<std::uint64_t, std::uint64_t>> table_entries()
{
        std::vector<std::pair<std::uint64_t, std::uint64_t>> e;
        for (std::size_t i = 0; i < tab_key.size(); ++i)
                if (tab_key[i]) e.push_back({ tab_key[i] - 1, tab_off[i] });
        return e;
}

// mode 0: read only; 1: create (truncate) for writing; 2: open existing for writing (resume)
bool map_file(std::string const& path, std::size_t size, int mode)
{
        int fd = ::open(path.c_str(), mode == 0 ? O_RDONLY : mode == 1 ? (O_RDWR | O_CREAT | O_TRUNC) : O_RDWR, 0644);
        if (fd < 0) return false;
        if (mode == 1 && ::ftruncate(fd, off_t(size)) != 0) { ::close(fd); return false; }
        if (mode == 2) {
                struct stat st;
                if (::fstat(fd, &st) != 0 || std::size_t(st.st_size) != size) { ::close(fd); return false; }
        }
        void* p = ::mmap(nullptr, size, mode == 0 ? PROT_READ : (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
        ::close(fd);
        if (p == MAP_FAILED) return false;
        db2 = static_cast<std::uint8_t*>(p);
        db2_size = size;
        return true;
}

// add the layer of all slices with n pieces, with one byte of DTM per position, in a memory-mapped file
// (path) with an index (path.idx); the layers below must be prebuilt with DTM. After every group of
// slices the file is synced and the group is recorded in path.progress, so that a run that is
// interrupted can continue where it stopped (resume = true).
bool extend(int n, int threads, std::string const& path, bool resume = false)
{
        if (built_max != n - 1 || dtm_max < n - 1) { std::cerr << "extend: need DTM slices up to " << n - 1 << std::endl; return false; }
        std::vector<Job> jobs = make_jobs(n, n);
        std::sort(jobs.begin(), jobs.end(), [](Job const& a, Job const& b) {
                return std::tie(a.level_n, a.men, a.dist, a.key) < std::tie(b.level_n, b.men, b.dist, b.key);
        });
        auto entries = table_entries();
        std::vector<std::pair<std::uint64_t, std::uint64_t>> layer;
        std::uint64_t total = 0;
        for (auto const& j : jobs) { layer.push_back({ j.key, DB2_FLAG | total }); total += 2 * j.P; }
        std::cerr << "extend: " << jobs.size() << " slices, " << total << " positions (" << total / 1e9 << " GB) in " << path << std::endl;
        std::size_t start_job = 0;
        if (resume) {
                std::FILE* pf = std::fopen((path + ".progress").c_str(), "r");
                unsigned long long done_jobs = 0;
                if (pf) { if (std::fscanf(pf, "%llu", &done_jobs) != 1) done_jobs = 0; std::fclose(pf); }
                start_job = std::size_t(done_jobs);
                if (!map_file(path, total, 2)) { std::cerr << "extend: cannot reopen " << path << std::endl; return false; }
                std::cerr << "extend: resuming after " << start_job << " of " << jobs.size() << " slices" << std::endl;
        } else if (!map_file(path, total, 1)) { std::cerr << "extend: cannot map " << path << std::endl; return false; }
        entries.insert(entries.end(), layer.begin(), layer.end());
        rebuild_table(entries);
        built_max = n;
        dtm_max = n;
        slice_max = std::max(slice_max, n);
        auto t0 = std::chrono::steady_clock::now();
        std::uint64_t done = 0;
        for (std::size_t i = 0; i < start_job; ++i) done += 2 * jobs[i].P;
        for (std::size_t g = start_job; g < jobs.size();) {
                std::size_t h = g;
                while (h < jobs.size() && jobs[h].men == jobs[g].men && jobs[h].dist == jobs[g].dist) ++h;
                #pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
                for (std::size_t i = g; i < h; ++i) {
                        Meta m = meta_of(Key::unpack(jobs[i].key));
                        std::vector<std::uint16_t> tmp(2 * m.P);
                        solve_slice(m, tmp.data());
                        SliceRef r;
                        find_prebuilt(jobs[i].key, r);
                        for (std::uint64_t x = 0; x < tmp.size(); ++x) {
                                if (tmp[x] != DRAW && tmp[x] > MAXDTM8) { std::cerr << "DTM overflow (" << tmp[x] << ")" << std::endl; std::abort(); }
                                r.dtm[x] = tmp[x] == DRAW ? DRAW8 : std::uint8_t(tmp[x]);
                        }
                }
                for (std::size_t i = g; i < h; ++i) done += 2 * jobs[i].P;
                // checkpoint: the slices of this group are on disk before the progress file says so
                ::msync(db2, db2_size, MS_SYNC);
                if (std::FILE* pf = std::fopen((path + ".progress.tmp").c_str(), "w")) {
                        std::fprintf(pf, "%llu\n", (unsigned long long)h);
                        sync_file(pf);
                        std::fclose(pf);
                        durable_rename(path + ".progress.tmp", path + ".progress");
                }
                if (h == jobs.size() || jobs[h].men != jobs[g].men) {
                        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                        std::cerr << "extend: " << jobs[g].men << " men done, " << done << " / " << total << " positions, " << sec << " s" << std::endl;
                }
                g = h;
        }
        ::msync(db2, db2_size, MS_SYNC);
        std::FILE* f = std::fopen((path + ".idx").c_str(), "wb");
        if (!f) return false;
        std::uint64_t hdr[4] = { 0x3678364244424747ull, std::uint64_t(n), layer.size(), total };
        std::fwrite(hdr, sizeof hdr, 1, f);
        std::fwrite(layer.data(), sizeof layer[0], layer.size(), f);
        return std::fclose(f) == 0;
}

// attach a layer made by extend (after loading the layers below)
bool attach(std::string const& path)
{
        std::FILE* f = std::fopen((path + ".idx").c_str(), "rb");
        if (!f) return false;
        std::uint64_t hdr[4];
        bool ok = std::fread(hdr, sizeof hdr, 1, f) == 1 && hdr[0] == 0x3678364244424747ull && built_max == int(hdr[1]) - 1;
        std::vector<std::pair<std::uint64_t, std::uint64_t>> layer(ok ? hdr[2] : 0);
        if (ok) ok = std::fread(layer.data(), sizeof layer[0], layer.size(), f) == layer.size();
        std::fclose(f);
        if (!ok || !map_file(path, hdr[3], 0)) return false;
        auto entries = table_entries();
        entries.insert(entries.end(), layer.begin(), layer.end());
        rebuild_table(entries);
        built_max = int(hdr[1]);
        dtm_max = std::max(dtm_max, built_max);
        slice_max = std::max(slice_max, built_max);
        return true;
}

// file format v2: magic, built_max, dtm_max, table size, db size, wdlbits words, table keys, table offsets, db, wdlbits
// (v1, without dtm_max and wdlbits, can still be loaded)
constexpr std::uint64_t MAGIC1 = 0x3678364244424745ull, MAGIC2 = 0x3678364244424746ull;

bool save(std::string const& path)
{
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) return false;
        std::uint64_t hdr[6] = { MAGIC2, std::uint64_t(built_max), std::uint64_t(dtm_max), tab_key.size(), db.size(), wdlbits.size() };
        std::fwrite(hdr, sizeof hdr, 1, f);
        std::fwrite(tab_key.data(), 8, tab_key.size(), f);
        std::fwrite(tab_off.data(), 8, tab_off.size(), f);
        std::fwrite(db.data(), 1, db.size(), f);
        std::fwrite(wdlbits.data(), 8, wdlbits.size(), f);
        return std::fclose(f) == 0;
}

bool load(std::string const& path)
{
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        std::uint64_t magic = 0, hdr[5] = {};
        bool ok = std::fread(&magic, 8, 1, f) == 1;
        if (ok && magic == MAGIC1) {
                ok = std::fread(hdr, 8, 3, f) == 3;
                hdr[3] = hdr[2]; hdr[2] = hdr[1]; hdr[1] = 99; hdr[4] = 0;   // built_max, dtm_max, table, db, wdl
        } else if (ok && magic == MAGIC2) {
                ok = std::fread(hdr, 8, 5, f) == 5;
        } else ok = false;
        if (ok) {
                tab_key.resize(hdr[2]); tab_off.resize(hdr[2]); db.resize(hdr[3]); wdlbits.resize(hdr[4]);
                ok = std::fread(tab_key.data(), 8, hdr[2], f) == hdr[2] && std::fread(tab_off.data(), 8, hdr[2], f) == hdr[2]
                  && std::fread(db.data(), 1, hdr[3], f) == hdr[3] && std::fread(wdlbits.data(), 8, hdr[4], f) == hdr[4];
                built_max = int(hdr[0]);
                dtm_max = int(hdr[1]);
                slice_max = std::max(slice_max, built_max);
        }
        std::fclose(f);
        return ok;
}

char wdl_of_value(std::uint16_t v) { return v == DRAW ? 'D' : (v & 1) ? 'W' : 'L'; }

// recompute every value of every prebuilt and extra slice from its successors (WDL only above dtm_max)
std::uint64_t verify(int threads)
{
        // in storage order (the order in which the slices were built), so that successor slices in the
        // memory-mapped layer are read with good locality
        std::vector<std::pair<std::uint64_t, std::uint64_t>> order;
        for (std::size_t i = 0; i < tab_key.size(); ++i)
                if (tab_key[i]) order.push_back({ tab_off[i], tab_key[i] - 1 });
        std::sort(order.begin(), order.end());
        std::vector<std::uint64_t> keys;
        for (auto const& [off, k] : order) keys.push_back(k);
        for (auto const& [k, v] : extra) keys.push_back(k);
        std::uint64_t errors = 0, done = 0;
        auto t0 = std::chrono::steady_clock::now();
        #pragma omp parallel for schedule(dynamic, 64) reduction(+ : errors) num_threads(threads)
        for (std::size_t i = 0; i < keys.size(); ++i) {
                Key key = Key::unpack(keys[i]);
                Meta s = meta_of(key);
                SliceRef r = slice_values(key);
                std::uint64_t d;
                #pragma omp atomic capture
                d = ++done;
                if (d % 1000000 == 0) {
                        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                        #pragma omp critical
                        std::cerr << "verify: " << d << " / " << keys.size() << " slices, " << sec << " s" << std::endl;
                }
                bool exact = r.dtm != nullptr;
                for (std::uint64_t j = 0; j < 2 * s.P; ++j) {
                        std::uint16_t a = ref_value(r, j), b = from_successors(pos_of(s, j % s.P, int(j / s.P)));
                        if (exact ? a != b : wdl_of_value(a) != wdl_of_value(b)) ++errors;
                }
        }
        return errors;
}

// ---------------------------------------------------------------- I/O

bool parse_fen(std::string const& fen, Pos& p)
{
        p = Pos{};
        std::string f;
        for (char ch : fen) if (!isspace(static_cast<unsigned char>(ch))) f += ch;
        if (f.empty()) return false;
        p.stm = (f[0] == 'B') ? 1 : 0;
        std::size_t i = f.find(':');
        while (i != std::string::npos && i + 1 < f.size()) {
                char colour = f[i + 1];
                int side = colour == 'W' ? 0 : 1;
                std::size_t j = f.find(':', i + 1);
                std::string list = f.substr(i + 2, j == std::string::npos ? std::string::npos : j - i - 2);
                std::stringstream ss(list);
                std::string item;
                while (std::getline(ss, item, ',')) {
                        if (item.empty()) continue;
                        bool king = item[0] == 'K';
                        int sq = std::stoi(king ? item.substr(1) : item) - 1;
                        if (sq < 0 || sq >= N) return false;
                        (king ? p.k[side] : p.m[side]).insert(std::size_t(sq));
                }
                i = j;
        }
        // men on their own promotion row are illegal
        if (!(p.m[0] & PROMO_WHITE).empty() || !(p.m[1] & PROMO_BLACK).empty()) return false;
        return true;
}

std::string fen_of(Pos const& p)
{
        std::string s = p.stm ? "B" : "W";
        for (int side = 0; side < 2; ++side) {
                s += side ? ":B" : ":W";
                bool first = true;
                for (int sq = 0; sq < N; ++sq) {
                        bool m = p.m[side].contains(std::size_t(sq)), k = p.k[side].contains(std::size_t(sq));
                        if (!m && !k) continue;
                        if (!first) s += ',';
                        first = false;
                        if (k) s += 'K';
                        s += std::to_string(sq + 1);
                }
        }
        return s;
}

std::string notation(Move const& mv)
{
        return std::to_string(mv.from + 1) + (!mv.captured.empty() ? "x" : "-") + std::to_string(mv.to + 1);
}

// ---------------------------------------------------------------- store: units of compressed DTM
//
// The databases with at most 12 pieces and at most 6 pieces per side are kept in "units": one unit per number of
// pieces n and number of men m. A unit holds the materials (white men, black men, white kings, black kings) with
// those counts in which both sides have a piece, one of each pair (material, mirrored material); a symmetric
// material keeps white to move only (black to move is the mirror image). Within a material the positions are
// numbered without gaps as [placement of the men][side to move][placement of the kings]: the men by a
// combinatorial rank (men_rank), the kings as within a slice (index_of). Every slice is a contiguous range, so no
// table of slices is needed, and the stored positions are exactly the positions of the counts with white to move.
//
// Positions in which a side has no pieces are not stored: the side to move without pieces has lost (0 plies), and
// a position in which the side not to move has no pieces cannot arise in a game; its value follows from its moves.
//
// A unit is built as a raw file of one byte of DTM per position (unit_build); a unit depends only on the units with
// fewer pieces and on the unit with the same number of pieces and one man fewer (a promotion), so the units are
// built in the order (n, m). After the checks the raw file is compressed (unit_compress): blocks of 4096 positions,
// each compressed on its own with zstd -19 (frames without magic number and content size), in which the positions
// where the side to move can capture repeat the previous value of the block; their values follow from the captures
// and are resolved at lookup (captures are compulsory, so a capture position takes the best of its captures).

constexpr bitboard MEN_W = FULL - PROMO_WHITE;            // squares where a white man can stand
constexpr bitboard MEN_B = FULL - PROMO_BLACK;            // squares where a black man can stand
constexpr bitboard MEN_MID = MEN_W & MEN_B;               // squares 4-15
constexpr std::uint64_t BLOCK = 4096;                     // positions (bytes) per compressed block
constexpr std::uint64_t CDB_MAGIC = 0x3678364244424331ull;     // version 1: draw 253, no escapes (still read)
constexpr std::uint64_t CDB_MAGIC2 = 0x3678364244424332ull;    // version 2: draw 255, escape codes E..254 (see ESC8)
constexpr int ZLEVEL = 19;

// placements of nwm white and nbm black men with j white men on squares 16-18
std::uint64_t men_part(int nwm, int nbm, int j)
{
        if (j > 3 || j > nwm || nwm - j > 12 || nbm > 15 - (nwm - j)) return 0;
        return binom[3][j] * binom[12][nwm - j] * binom[15 - (nwm - j)][nbm];
}

std::uint64_t men_count(int nwm, int nbm)
{
        std::uint64_t c = 0;
        for (int j = 0; j <= 3; ++j) c += men_part(nwm, nbm, j);
        return c;
}

// rank of a placement of men: [white men on 16-18 (count j)][white men on 16-18][white men on 4-15][black men]
std::uint64_t men_rank(bitboard const& wm, bitboard const& bm)
{
        int const nwm = int(wm.size()), nbm = int(bm.size()), j = int((wm & PROMO_BLACK).size());
        std::uint64_t r = 0;
        for (int i = 0; i < j; ++i) r += men_part(nwm, nbm, i);
        std::uint64_t const cm = binom[12][nwm - j], cb = binom[15 - (nwm - j)][nbm];
        return r + (rank_in(wm & PROMO_BLACK, PROMO_BLACK) * cm + rank_in(wm & MEN_MID, MEN_MID)) * cb + rank_in(bm, MEN_B - wm);
}

void men_unrank(int nwm, int nbm, std::uint64_t r, bitboard& wm, bitboard& bm)
{
        int j = 0;
        while (r >= men_part(nwm, nbm, j)) r -= men_part(nwm, nbm, j++);
        std::uint64_t const cm = binom[12][nwm - j], cb = binom[15 - (nwm - j)][nbm];
        std::uint64_t const c = r % cb;
        r /= cb;
        wm = unrank_in(r / cm, j, PROMO_BLACK) | unrank_in(r % cm, nwm - j, MEN_MID);
        bm = unrank_in(c, nbm, MEN_B - wm);
}

struct UnitMat {
        int wm, bm, wk, bk;          // numbers of white men, black men, white kings, black kings
        std::uint64_t base, men, P;  // first position, placements of the men, placements of the kings
        int S;                       // sides to move stored: 2, or 1 for a symmetric material (white to move)
        std::uint64_t code() const { return std::uint64_t(wm) | std::uint64_t(bm) << 8 | std::uint64_t(wk) << 16 | std::uint64_t(bk) << 24 | std::uint64_t(S) << 32; }
        std::uint64_t size() const { return men * std::uint64_t(S) * P; }
};

// Material mode (the mat* commands): the store holds one file per material, 6x6-wp-bp-wm-bm (white pieces, black
// pieces, white men, black men), each a .cdb with a material table of one entry. While only_mat is set, unit_layout and
// unit_path for its (n, m) give that material and its file, so that the unit functions (build, verify, compress, ...)
// work on it unchanged; store_open then opens the files of all finished materials.
struct OnlyMat {
        bool on = false;
        int wm = 0, bm = 0, wk = 0, bk = 0;
        int n() const { return wm + bm + wk + bk; }
        int m() const { return wm + bm; }
} only_mat;

// a material as stored: at most 6 pieces and at least one per side, and not the colour mirror of a smaller one
bool stored_material(int wm, int bm, int wk, int bk)
{
        if (wm < 0 || bm < 0 || wk < 0 || bk < 0 || wm + wk < 1 || bm + bk < 1 || wm + wk > 6 || bm + bk > 6) return false;
        return !(std::tuple(bm, wm, bk, wk) < std::tuple(wm, bm, wk, bk));
}

std::vector<UnitMat> mat_layout(int wm, int bm, int wk, int bk)
{
        int const F = N - wm - bm;
        bool const sym = std::tuple(wm, bm, wk, bk) == std::tuple(bm, wm, bk, wk);
        return { UnitMat{ wm, bm, wk, bk, 0, men_count(wm, bm), binom[F][wk] * binom[F - wk][bk], sym ? 1 : 2 } };
}

std::vector<UnitMat> unit_layout(int n, int m)
{
        if (only_mat.on && n == only_mat.n() && m == only_mat.m()) return mat_layout(only_mat.wm, only_mat.bm, only_mat.wk, only_mat.bk);
        std::vector<UnitMat> v;
        std::uint64_t base = 0;
        for (int wm = 0; wm <= m; ++wm)
                for (int wk = 0; wk <= n - m; ++wk) {
                        int const bm = m - wm, bk = n - m - wk;
                        if (wm + wk > 6 || bm + bk > 6 || wm + wk == 0 || bm + bk == 0) continue;
                        auto const a = std::tuple(wm, bm, wk, bk), b = std::tuple(bm, wm, bk, wk);
                        if (b < a) continue;
                        int const F = N - m;
                        UnitMat u{ wm, bm, wk, bk, base, men_count(wm, bm), binom[F][wk] * binom[F - wk][bk], a == b ? 1 : 2 };
                        base += u.size();
                        v.push_back(u);
                }
        return v;
}

std::uint64_t unit_positions(std::vector<UnitMat> const& lay) { return lay.empty() ? 0 : lay.back().base + lay.back().size(); }

// the stored representative of a position: the canonical material, white to move for a symmetric one
Pos canon(Pos const& p)
{
        auto const a = std::tuple(p.m[0].size(), p.m[1].size(), p.k[0].size(), p.k[1].size());
        auto const b = std::tuple(std::get<1>(a), std::get<0>(a), std::get<3>(a), std::get<2>(a));
        if (a < b) return p;
        if (b < a) return mirror(p);
        return p.stm == 0 ? p : mirror(p);
}

std::uint64_t king_index(Pos const& p)
{
        bitboard const free = FULL - (p.m[0] | p.m[1]);
        std::size_t const F = free.size();
        std::uint64_t const nb = binom[F - p.k[0].size()][p.k[1].size()];
        return rank_in(p.k[0], free) * nb + rank_in(p.k[1], free - p.k[0]);
}

struct Unit {
        int n = 0, m = 0;
        bool raw = false;
        std::uint8_t* p = nullptr;          // the mapped file
        std::size_t size = 0;
        std::uint64_t npos = 0, nblocks = 0;
        std::uint64_t const* off = nullptr; // compressed: block offsets (nblocks + 1) relative to data
        std::uint8_t const* data = nullptr;
        std::uint64_t id = 0;               // unique per opened unit (block cache tag)
        std::uint64_t const* ovf = nullptr; // raw: overflow table, sorted entries offset << 16 | DTM (see ESC8)
        std::size_t novf = 0;
        int version = 0;                    // compressed: 1 (draw 253, no escapes) or 2 (draw 255, escapes from esc)
        unsigned esc = 255;                 // compressed version 2: the first escape code E (255: no escapes)
};

// Long distances.
// Raw unit (during a build): DTM up to dtm8_max (251; EGDB_DTM8_MAX lowers it, to test on small units), above that the
// byte ESC8 with the exact value in an overflow table kept in memory (raw_ovf) and in the file .ovf next to the raw
// file, written at every checkpoint. Draws are DRAW8 (253), positions not yet decided UNKNOWN8 (254).
// Compressed unit (version 2): one code byte per position, 255 a draw, 0 .. E - 1 the DTM itself, E .. 254 escape codes:
// code c with extra byte x is DTM E + 256 (c - E) + x. The extra bytes follow the block's position bytes in the same
// zstd frame, the k-th escape of the block taking the k-th extra byte. E (header word 7) is chosen per unit as large as
// possible: 255 (no escapes) when no DTM exceeds 254, else 255 - k with k codes reaching 254 + 255 k plies (EGDB_ESC
// forces a smaller E, to test the escapes on small units).
constexpr std::uint8_t ESC8 = 252;
constexpr std::uint8_t DRAW_CODE = 255;
int const dtm8_max = [] { char const* e = std::getenv("EGDB_DTM8_MAX"); return std::clamp(e ? std::atoi(e) : 251, 1, 251); }();
std::vector<std::uint64_t> raw_ovf;

std::vector<std::uint64_t> read_ovf(std::string const& path)
{
        std::vector<std::uint64_t> t;
        if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
                std::uint64_t x;
                while (std::fread(&x, 8, 1, f) == 1) t.push_back(x);
                std::fclose(f);
        }
        return t;
}

bool write_ovf(std::string const& path, std::vector<std::uint64_t> const& t)
{
        if (t.empty()) { std::remove(path.c_str()); return true; }
        std::string const tmp = path + ".tmp";
        std::FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) return false;
        bool ok = std::fwrite(t.data(), 8, t.size(), f) == t.size() && sync_file(f);
        ok = std::fclose(f) == 0 && ok;
        return ok && durable_rename(tmp, path);
}

std::uint16_t ovf_find(std::uint64_t const* t, std::size_t n, std::uint64_t off)
{
        std::uint64_t const* it = std::lower_bound(t, t + n, off << 16);
        if (it == t + n || (*it >> 16) != off) { std::cerr << "store: no overflow entry for offset " << off << std::endl; std::abort(); }
        return std::uint16_t(*it & 0xFFFF);
}

// the value of a stored byte at offset off of unit u
std::uint16_t value_of(Unit const& u, std::uint64_t off, std::uint8_t b)
{
        return b == ESC8 ? ovf_find(u.ovf, u.novf, off) : widen(b);
}

struct MatRef { int unit = -1; int S = 0; std::uint64_t base = 0, P = 0; };

std::vector<Unit> units;
MatRef mat_ref[7][7][7][7];
std::uint64_t next_unit_id = 1;
std::uint64_t store_gen = 0;           // changes whenever the store is opened again (capture cache tag)

std::string mat_path(std::string const& dir, int wm, int bm, int wk, int bk, char const* ext)
{
        char name[40];   // 6x6-wp-bp-wm-bm
        std::snprintf(name, sizeof name, "/6x6-%02d-%02d-%02d-%02d", wm + wk, bm + bk, wm, bm);
        return dir + name + ext;
}

std::string unit_path(std::string const& dir, int n, int m, char const* ext)
{
        if (only_mat.on && n == only_mat.n() && m == only_mat.m()) return mat_path(dir, only_mat.wm, only_mat.bm, only_mat.wk, only_mat.bk, ext);
        char name[32];   // 6x6-nn-mm: with leading zeros the names sort in build order
        std::snprintf(name, sizeof name, "/6x6-%02d-%02d", n, m);
        return dir + name + ext;
}

// mode 0: read only; 1: create (truncate) for writing; 2: open existing for writing
std::uint8_t* map_any(std::string const& path, std::size_t& size, int mode, std::size_t want = 0)
{
        int fd = ::open(path.c_str(), mode == 0 ? O_RDONLY : mode == 1 ? (O_RDWR | O_CREAT | O_TRUNC) : O_RDWR, 0644);
        if (fd < 0) return nullptr;
        if (mode == 1 && ::ftruncate(fd, off_t(want)) != 0) { ::close(fd); return nullptr; }
        struct stat st;
        if (::fstat(fd, &st) != 0 || (want && std::size_t(st.st_size) != want) || st.st_size == 0) { ::close(fd); return nullptr; }
        size = std::size_t(st.st_size);
        void* q = ::mmap(nullptr, size, mode == 0 ? PROT_READ : (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);
        ::close(fd);
        return q == MAP_FAILED ? nullptr : static_cast<std::uint8_t*>(q);
}

void store_close()
{
        for (auto& u : units) ::munmap(u.p, u.size);
        units.clear();
        for (auto& a : mat_ref) for (auto& b : a) for (auto& c : b) for (auto& d : c) d = MatRef{};
}

// open a unit: raw (mode 0 read only, 1 create, 2 existing for writing) or compressed (mode 3)
bool unit_open(std::string const& dir, int n, int m, int mode)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) return true;
        Unit u;
        u.n = n; u.m = m; u.npos = unit_positions(lay); u.raw = mode != 3; u.id = next_unit_id++;
        if (u.raw) {
                u.p = map_any(unit_path(dir, n, m, ".raw"), u.size, mode, u.npos);
                if (!u.p) { std::cerr << "store: cannot map " << unit_path(dir, n, m, ".raw") << std::endl; return false; }
                if (mode == 1) { raw_ovf.clear(); std::remove(unit_path(dir, n, m, ".ovf").c_str()); }
                else raw_ovf = read_ovf(unit_path(dir, n, m, ".ovf"));
                u.ovf = raw_ovf.data(); u.novf = raw_ovf.size();
        } else {
                std::string const path = unit_path(dir, n, m, ".cdb");
                u.p = map_any(path, u.size, 0);
                if (!u.p) { std::cerr << "store: cannot map " << path << std::endl; return false; }
                auto const* h = reinterpret_cast<std::uint64_t const*>(u.p);
                u.version = u.size >= 64 && h[0] == CDB_MAGIC2 ? 2 : 1;
                bool ok = u.size >= 64 && (h[0] == CDB_MAGIC || h[0] == CDB_MAGIC2) && h[1] == std::uint64_t(n) && h[2] == std::uint64_t(m) && h[3] == lay.size()
                       && h[4] == u.npos && h[5] == BLOCK && h[6] == (u.npos + BLOCK - 1) / BLOCK;
                for (std::size_t i = 0; ok && i < lay.size(); ++i) ok = h[8 + 2 * i] == lay[i].code() && h[9 + 2 * i] == lay[i].base;
                if (!ok) { std::cerr << "store: bad header in " << path << std::endl; ::munmap(u.p, u.size); return false; }
                u.nblocks = h[6];
                u.off = h + 8 + 2 * lay.size();
                u.data = reinterpret_cast<std::uint8_t const*>(u.off + u.nblocks + 1);
                u.esc = u.version == 2 ? unsigned(h[7]) : 255;
                if (u.version == 2 && (u.esc < 1 || u.esc > 255)) { std::cerr << "store: bad escape code in " << path << std::endl; ::munmap(u.p, u.size); return false; }
                if (std::size_t(u.data - u.p) + u.off[u.nblocks] != u.size) { std::cerr << "store: bad size of " << path << std::endl; ::munmap(u.p, u.size); return false; }
        }
        units.push_back(u);
        for (auto const& x : lay) mat_ref[x.wm][x.bm][x.wk][x.bk] = MatRef{ int(units.size()) - 1, x.S, x.base, x.P };
        return true;
}

bool unit_before(int n1, int m1, int n2, int m2) { return n1 < n2 || (n1 == n2 && m1 < m2); }

// open the compressed units before (n, m); own: -1 nothing more, else unit_open's mode for (n, m) itself;
// partial: skip units that are not there (their positions answer "not in database")
bool store_open(std::string const& dir, int n, int m, int own, bool partial = false)
{
        store_close();
        if (only_mat.on) {      // material mode: every finished material (a missing one answers "not in database")
                OnlyMat const self = only_mat;
                for (int wm = 0; wm <= 6; ++wm) for (int bm = 0; bm <= 6; ++bm) for (int wk = 0; wk <= 6; ++wk) for (int bk = 0; bk <= 6; ++bk) {
                        if (!stored_material(wm, bm, wk, bk) || std::tuple(wm, bm, wk, bk) == std::tuple(self.wm, self.bm, self.wk, self.bk)) continue;
                        if (::access(mat_path(dir, wm, bm, wk, bk, ".cdb").c_str(), R_OK) != 0) continue;
                        only_mat = OnlyMat{ true, wm, bm, wk, bk };
                        bool const ok = unit_open(dir, wm + bm + wk + bk, wm + bm, 3);
                        only_mat = self;
                        if (!ok) return false;
                }
                if (own >= 0 && !unit_open(dir, n, m, own)) return false;
                store_on = true;
                store_gen = next_unit_id;
                slice_max = std::max(slice_max, 12);
                return true;
        }
        for (int a = 2; a <= 12; ++a)
                for (int b = 0; b <= a; ++b) {
                        if (!unit_before(a, b, n, m)) continue;
                        if (partial && !unit_layout(a, b).empty() && ::access(unit_path(dir, a, b, ".cdb").c_str(), R_OK) != 0) continue;
                        if (!unit_open(dir, a, b, 3)) return false;
                }
        if (own >= 0 && !unit_open(dir, n, m, own)) return false;
        store_on = true;
        store_gen = next_unit_id;
        slice_max = std::max(slice_max, 12);
        return true;
}

// zstd contexts and a block cache per thread (direct mapped, 2^cache_bits blocks of 4 KiB; EGDB_CACHE_BITS, default 14 = 64 MiB)
ZSTD_DCtx* new_dctx()
{
        ZSTD_DCtx* dx = ZSTD_createDCtx();
        ZSTD_DCtx_setParameter(dx, ZSTD_d_format, ZSTD_f_zstd1_magicless);
        return dx;
}

ZSTD_CCtx* new_cctx()
{
        ZSTD_CCtx* cx = ZSTD_createCCtx();
        ZSTD_CCtx_setParameter(cx, ZSTD_c_compressionLevel, ZLEVEL);
        ZSTD_CCtx_setParameter(cx, ZSTD_c_format, ZSTD_f_zstd1_magicless);
        ZSTD_CCtx_setParameter(cx, ZSTD_c_contentSizeFlag, 0);
        ZSTD_CCtx_setParameter(cx, ZSTD_c_checksumFlag, 0);
        ZSTD_CCtx_setParameter(cx, ZSTD_c_dictIDFlag, 0);
        return cx;
}

// a whole decoded block: the bytes of its positions, then the extra bytes of its escapes (out holds 2 * BLOCK bytes)
std::size_t decode_block(ZSTD_DCtx* dx, Unit const& u, std::uint64_t b, std::uint8_t* out)
{
        std::size_t const want = std::size_t(std::min(BLOCK, u.npos - b * BLOCK));
        std::size_t const r = ZSTD_decompressDCtx(dx, out, 2 * BLOCK, u.data + u.off[b], std::size_t(u.off[b + 1] - u.off[b]));
        if (ZSTD_isError(r) || r < want || (r > want && u.esc == 255)) {
                std::cerr << "store: block " << b << " of unit " << u.n << "/" << u.m << " does not decode" << std::endl;
                std::abort();
        }
        return r;
}

// a code byte without its extra byte: the value, or ESCAPED
constexpr std::uint16_t ESCAPED = 0xFFFE;
std::uint16_t code_value(Unit const& u, std::uint8_t c)
{
        if (u.version == 1) return c == DRAW8 ? DRAW : c;
        if (c == DRAW_CODE) return DRAW;
        return c < u.esc ? c : ESCAPED;
}

std::uint16_t escape_value(Unit const& u, std::uint8_t c, std::uint8_t x) { return std::uint16_t(u.esc + 256 * (c - u.esc) + x); }

// The block cache keeps the position bytes of a block in a slot of 4 KiB. A block with escapes does not fit there: it is
// decoded once more into a scratch buffer, its position bytes copied to the slot and its extra bytes kept beside it,
// with a bitset of its escaped positions and prefix counts, so that the extra byte of position i is number
// prefix[i / 64] + popcount(bits[i / 64] & ((1 << i % 64) - 1)).
struct Escapes {
        std::array<std::uint64_t, BLOCK / 64> bits{};
        std::array<std::uint16_t, BLOCK / 64> prefix{};
        std::vector<std::uint8_t> extra;
        std::uint8_t at(std::size_t i) const
        {
                std::size_t const w = i / 64;
                std::uint64_t const below = bits[w] & ((std::uint64_t(1) << (i % 64)) - 1);
                return extra[prefix[w] + std::size_t(std::popcount(below))];
        }
};

std::size_t cache_bits = [] { char const* e = std::getenv("EGDB_CACHE_BITS"); return std::size_t(e ? std::atoi(e) : 14); }();
std::atomic<std::uint64_t> cache_hits{0}, cache_misses{0};
struct BlockCache {
        std::size_t const SLOTS = std::size_t(1) << cache_bits;
        std::vector<std::uint64_t> tag = std::vector<std::uint64_t>(SLOTS, 0);
        std::vector<std::uint8_t> buf = std::vector<std::uint8_t>(SLOTS * BLOCK);
        std::vector<std::unique_ptr<Escapes>> esc = std::vector<std::unique_ptr<Escapes>>(SLOTS);
        std::vector<std::uint8_t> has_esc = std::vector<std::uint8_t>(SLOTS, 0);
        std::vector<std::uint8_t> scratch = std::vector<std::uint8_t>(2 * BLOCK);
        ZSTD_DCtx* dx = new_dctx();
        std::uint64_t hits = 0, misses = 0;
        ~BlockCache() { ZSTD_freeDCtx(dx); }
        std::size_t slot(Unit const& u, std::uint64_t b)
        {
                std::uint64_t const id = u.id << 40 | b;
                std::size_t const s = std::size_t((id * 0x9E3779B97F4A7C15ull) >> (64 - cache_bits));
                if (tag[s] == id) { ++hits; return s; }
                ++misses;
                std::uint8_t* d = buf.data() + s * BLOCK;
                std::size_t const want = std::size_t(std::min(BLOCK, u.npos - b * BLOCK));
                std::uint8_t const* src = d;
                std::size_t r = ZSTD_decompressDCtx(dx, d, BLOCK, u.data + u.off[b], std::size_t(u.off[b + 1] - u.off[b]));
                if (ZSTD_isError(r)) {          // extra bytes that do not fit the slot: decode the whole block
                        r = decode_block(dx, u, b, scratch.data());
                        std::memcpy(d, scratch.data(), want);
                        src = scratch.data();
                } else if (r < want || (r > want && u.esc == 255)) {
                        std::cerr << "store: block " << b << " of unit " << u.n << "/" << u.m << " does not decode" << std::endl;
                        std::abort();
                }
                has_esc[s] = r > want;
                if (r > want) {
                        if (!esc[s]) esc[s] = std::make_unique<Escapes>();
                        Escapes& e = *esc[s];
                        e.bits.fill(0);
                        for (std::size_t i = 0; i < want; ++i)
                                if (code_value(u, src[i]) == ESCAPED) e.bits[i / 64] |= std::uint64_t(1) << (i % 64);
                        std::uint16_t run = 0;
                        for (std::size_t w = 0; w < BLOCK / 64; ++w) { e.prefix[w] = run; run = std::uint16_t(run + std::popcount(e.bits[w])); }
                        e.extra.assign(src + want, src + r);
                        if (run != r - want) { std::cerr << "store: escapes of block " << b << " of unit " << u.n << "/" << u.m << " do not match" << std::endl; std::abort(); }
                }
                tag[s] = id;
                return s;
        }
        std::uint8_t byte(Unit const& u, std::uint64_t off) { return buf[slot(u, off / BLOCK) * BLOCK + off % BLOCK]; }
        std::uint16_t value(Unit const& u, std::uint64_t off)
        {
                std::size_t const s = slot(u, off / BLOCK), i = std::size_t(off % BLOCK);
                std::uint8_t const c = buf[s * BLOCK + i];
                std::uint16_t const v = code_value(u, c);
                return v != ESCAPED ? v : escape_value(u, c, esc[s]->at(i));
        }
};
thread_local BlockCache block_cache;

std::uint64_t stored_offset(MatRef const& r, Pos const& q)
{
        return r.base + (men_rank(q.m[0], q.m[1]) * std::uint64_t(r.S) + std::uint64_t(r.S == 2 ? q.stm : 0)) * r.P + king_index(q);
}

MatRef const& mat_of(Pos const& q)
{
        return mat_ref[q.m[0].size()][q.m[1].size()][q.k[0].size()][q.k[1].size()];
}

// the values of capture positions resolved through compressed units, per thread (direct mapped, 2^20 entries):
// a capture position is often a successor of several positions that are built or checked one after the other
struct CaptureCache {
        static constexpr std::size_t SLOTS = std::size_t(1) << 20;
        std::vector<PosKey> key = std::vector<PosKey>(SLOTS, 0);
        std::vector<std::uint64_t> gen = std::vector<std::uint64_t>(SLOTS, 0);
        std::vector<std::uint16_t> val = std::vector<std::uint16_t>(SLOTS, 0);
};
thread_local CaptureCache capture_cache;

std::uint16_t resolve_capture(Pos const& q)
{
        PosKey const k = pack(q);
        std::size_t const s = PosKeyHash{}(k) & (CaptureCache::SLOTS - 1);
        CaptureCache& c = capture_cache;
        if (c.key[s] == k && c.gen[s] == store_gen) return c.val[s];
        std::uint16_t const v = from_successors(q);
        c.key[s] = k; c.gen[s] = store_gen; c.val[s] = v;
        return v;
}

std::uint16_t store_lookup(Pos const& p)
{
        int const w = int((p.m[0] | p.k[0]).size()), b = int((p.m[1] | p.k[1]).size());
        if ((p.stm ? b : w) == 0) return 0;                     // no pieces: no moves
        if (w == 0 || b == 0) return from_successors(p);
        if (w > 6 || b > 6) throw NotInDatabase{};
        Pos const q = canon(p);
        MatRef const& r = mat_of(q);
        if (r.unit < 0) throw NotInDatabase{};
        Unit const& u = units[std::size_t(r.unit)];
        if (!u.raw && has_capture(q)) return resolve_capture(q);
        std::uint64_t const off = stored_offset(r, q);
        if (!u.raw) return block_cache.value(u, off);
        std::uint8_t const v = u.p[off];
        if (v > DRAW8) { std::cerr << "store: no value yet for " << fen_of(p) << std::endl; std::abort(); }
        return value_of(u, off, v);
}

// the stored positions of a unit in storage order: one slice (placement of the men, side to move) at a time
struct Cursor {
        std::vector<UnitMat> const& lay;
        std::size_t mi = 0;
        std::uint64_t slot = 0, k = 0;     // slot = men rank * S + side to move; k = placement of the kings
        Meta meta{};
        int stm = 0;
        explicit Cursor(std::vector<UnitMat> const& l) : lay(l) {}
        void set_slice()
        {
                if (mi >= lay.size()) return;
                UnitMat const& x = lay[mi];
                bitboard wm, bm;
                men_unrank(x.wm, x.bm, slot / std::uint64_t(x.S), wm, bm);
                stm = x.S == 2 ? int(slot % 2) : 0;
                meta = meta_of(Key{ wm, bm, x.wk, x.bk });
        }
        void seek(std::uint64_t off)
        {
                mi = 0;
                while (mi + 1 < lay.size() && lay[mi + 1].base <= off) ++mi;
                std::uint64_t const rel = off - lay[mi].base;
                slot = rel / lay[mi].P;
                k = rel % lay[mi].P;
                set_slice();
        }
        Pos pos() const { return pos_of(meta, k, stm); }
        void next()
        {
                if (++k < lay[mi].P) return;
                k = 0;
                if (++slot == lay[mi].men * std::uint64_t(lay[mi].S)) { slot = 0; ++mi; }
                set_slice();
        }
};

// the block as stored: capture positions repeat the previous value of the block
// returns the size: the block's positions, then the extra bytes of its escapes (out holds 2 * BLOCK bytes)
std::size_t filled_block(std::vector<UnitMat> const& lay, std::uint8_t const* raw, std::vector<std::uint64_t> const& ovf,
                         std::uint64_t npos, std::uint64_t b, unsigned esc, std::uint8_t* out)
{
        Cursor c(lay);
        c.seek(b * BLOCK);
        std::uint64_t const end = std::min(npos, (b + 1) * BLOCK);
        std::size_t extra = std::size_t(end - b * BLOCK);
        std::uint16_t prev = 0;
        for (std::uint64_t o = b * BLOCK; o < end; ++o, c.next()) {
                std::uint16_t v = raw[o] == ESC8 ? ovf_find(ovf.data(), ovf.size(), o) : widen(raw[o]);
                if (o > b * BLOCK && has_capture(c.pos())) v = prev;
                prev = v;
                std::uint8_t code = v == DRAW ? DRAW_CODE : std::uint8_t(v);
                if (v != DRAW && v >= esc) {
                        unsigned const e = unsigned(v) - esc, c = esc + e / 256;
                        if (c > 254) { std::cerr << "DTM overflow (" << v << ")" << std::endl; std::abort(); }
                        code = std::uint8_t(c);
                        out[extra++] = std::uint8_t(e % 256);
                }
                out[o - b * BLOCK] = code;
        }
        return extra;
}

double seconds_since(std::chrono::steady_clock::time_point t0)
{
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

void write_progress(std::string const& path, std::string const& text)
{
        std::string const tmp = path + ".tmp";
        if (std::FILE* f = std::fopen(tmp.c_str(), "w")) {
                std::fputs(text.c_str(), f);
                sync_file(f);
                std::fclose(f);
                durable_rename(tmp, path);
        }
}

// build unit (n, m) as a raw file, with the units before it compressed; resumes from the progress file if there is one
// slices with at least this many positions that are more than one thread's share of their group are solved by all
// threads together (solve_slice_par)
std::uint64_t const BIG_SLICE = [] { char const* e = std::getenv("EGDB_BIG_SLICE"); return e ? std::uint64_t(std::atoll(e)) : std::uint64_t(1) << 20; }();   // EGDB_BIG_SLICE: for tests

// memory per thread for the slices spread over the threads: the solver's arrays (2 + 2 bytes and a bit per position)
// for the largest such slice, and the per-thread caches (blocks and resolved captures)
std::uint64_t thread_bytes(std::uint64_t largest)
{
        return largest * 4 + largest / 8 + (std::uint64_t(1) << cache_bits) * (2 * BLOCK + 8) + CaptureCache::SLOTS * 26;
}

// Build the material of only_mat (material mode): its slices one wave (distance) at a time, every dependency read
// from the finished materials or from earlier waves of this one. threads == 1: one slice after another (solve_slice).
// threads > 1: the slices of a wave side by side (std::for_each with std::execution::par), and a big slice by itself
// with all cores (solve_slice_pull). No other synchronisation: every slice writes only its own positions.
bool mat_build(std::string const& dir, int threads)
{
        int const n = only_mat.n(), m = only_mat.m();
        auto const lay = unit_layout(n, m);
        std::uint64_t const npos = unit_positions(lay);
        std::string const raw = unit_path(dir, n, m, ".raw"), prog = raw + ".progress";
        std::size_t start = 0;
        bool resume = false;
        if (std::FILE* f = std::fopen(prog.c_str(), "r")) {
                unsigned long long s = 0;
                if (std::fscanf(f, "%llu", &s) == 1) { start = std::size_t(s); resume = true; }
                std::fclose(f);
        }
        if (!store_open(dir, n, m, resume ? 2 : 1)) return false;
        Unit& u = units.back();
        if (!resume) {
                std::memset(u.p, UNKNOWN8, std::size_t(npos));
                write_progress(prog, "0\n");
        }
        std::vector<std::pair<int, std::uint64_t>> jobs;       // (distance, slice key), in build order
        for (auto const& x : lay)
                for (std::uint64_t r = 0; r < x.men; ++r) {
                        bitboard wm, bm;
                        men_unrank(x.wm, x.bm, r, wm, bm);
                        Key const k{ wm, bm, x.wk, x.bk };
                        if (x.S == 1 && !canonical_key(k)) continue;
                        jobs.push_back({ distance(k), k.pack() });
                }
        std::sort(jobs.begin(), jobs.end());
        std::cerr << "material " << only_mat.wm + only_mat.wk << "-" << only_mat.bm + only_mat.bk << "-" << only_mat.wm << "-" << only_mat.bm
                  << ": " << jobs.size() << " slices, " << npos << " positions, " << threads << (threads == 1 ? " thread" : " threads")
                  << (resume ? ", resuming after slice " + std::to_string(start) : std::string()) << std::endl;
        auto const t0 = std::chrono::steady_clock::now();
        std::string const ovf_path = unit_path(dir, n, m, ".ovf");
        // write a solved slice (for a symmetric material the positions with black to move go to the mirrored slice);
        // values above dtm8_max go to the slice's own overflow list
        auto store = [&](std::size_t i, Meta const& s, std::vector<std::uint16_t> const& tmp, std::vector<std::uint64_t>& ovf) {
                auto put = [&](std::uint64_t off, std::uint16_t v) {
                        if (v == DRAW) return DRAW8;
                        if (v > dtm8_max) { ovf.push_back(off << 16 | v); return ESC8; }
                        return std::uint8_t(v);
                };
                std::uint64_t bad = 0;
                MatRef const r = mat_of(pos_of(s, 0, 0));
                std::uint64_t const base = stored_offset(r, pos_of(s, 0, 0));
                for (std::uint64_t x = 0; x < std::uint64_t(r.S) * s.P; ++x) u.p[base + x] = put(base + x, tmp[x]);
                if (r.S == 1)
                        for (std::uint64_t x = 0; x < s.P; ++x) {
                                Pos const q = mirror(pos_of(s, x, 1));
                                std::uint64_t const o = stored_offset(r, q);
                                std::uint8_t const b = put(o, tmp[s.P + x]);
                                if (u.p[o] != UNKNOWN8 && u.p[o] != b && Key::unpack(jobs[i].second).pack() == key_of(q).pack()) ++bad;
                                u.p[o] = b;
                        }
                return bad;
        };
        for (std::size_t g = start; g < jobs.size();) {
                std::size_t h = g;
                while (h < jobs.size() && jobs[h].first == jobs[g].first) ++h;
                std::vector<std::vector<std::uint64_t>> ovf(h - g);
                std::vector<std::uint64_t> bad(h - g, 0);
                std::vector<std::size_t> big, small;
                for (std::size_t i = g; i < h; ++i)
                        (threads > 1 && 2 * meta_of(Key::unpack(jobs[i].second)).P >= BIG_SLICE ? big : small).push_back(i);
                for (std::size_t i : big) {
                        Meta const s = meta_of(Key::unpack(jobs[i].second));
                        std::vector<std::uint16_t> tmp(2 * s.P);
                        solve_slice_pull(s, tmp.data());
                        bad[i - g] = store(i, s, tmp, ovf[i - g]);
                }
                auto solve = [&](std::size_t i) {
                        Meta const s = meta_of(Key::unpack(jobs[i].second));
                        std::vector<std::uint16_t> tmp(2 * s.P);
                        solve_slice(s, tmp.data());
                        bad[i - g] = store(i, s, tmp, ovf[i - g]);
                };
                if (threads > 1) std::for_each(std::execution::par, small.begin(), small.end(), solve);
                else std::for_each(small.begin(), small.end(), solve);
                std::uint64_t nbad = 0;
                for (auto b : bad) nbad += b;
                if (nbad) { std::cerr << "material: " << nbad << " mirrored positions disagree" << std::endl; return false; }
                std::size_t const old = raw_ovf.size();
                for (auto const& q : ovf) raw_ovf.insert(raw_ovf.end(), q.begin(), q.end());
                std::sort(raw_ovf.begin() + std::ptrdiff_t(old), raw_ovf.end());
                std::inplace_merge(raw_ovf.begin(), raw_ovf.begin() + std::ptrdiff_t(old), raw_ovf.end());
                raw_ovf.erase(std::unique(raw_ovf.begin(), raw_ovf.end()), raw_ovf.end());
                u.ovf = raw_ovf.data(); u.novf = raw_ovf.size();
                ::msync(u.p, u.size, MS_SYNC);
                if (!write_ovf(ovf_path, raw_ovf)) { std::cerr << "material: cannot write " << ovf_path << std::endl; return false; }
                write_progress(prog, std::to_string(h) + "\n");
                g = h;
        }
        std::uint64_t const unknown = std::uint64_t(std::count(u.p, u.p + npos, UNKNOWN8));
        if (unknown) { std::cerr << "material: " << unknown << " positions without value" << std::endl; return false; }
        write_progress(prog, std::to_string(jobs.size()) + " complete\n");
        std::cout << "built " << only_mat.wm + only_mat.wk << " " << only_mat.bm + only_mat.bk << " " << only_mat.wm << " " << only_mat.bm
                  << " positions " << npos << " slices " << jobs.size() << " overflow " << raw_ovf.size() << " seconds " << seconds_since(t0) << '\n';
        return true;
}

bool unit_build(std::string const& dir, int n, int m, int threads, double mem_gb = 0)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        std::uint64_t const npos = unit_positions(lay);
        std::string const raw = unit_path(dir, n, m, ".raw"), prog = raw + ".progress";
        std::size_t start = 0;
        bool resume = false;
        if (std::FILE* f = std::fopen(prog.c_str(), "r")) {
                unsigned long long s = 0;
                if (std::fscanf(f, "%llu", &s) == 1) { start = std::size_t(s); resume = true; }
                std::fclose(f);
        }
        if (!store_open(dir, n, m, resume ? 2 : 1)) return false;
        Unit& u = units.back();
        if (!resume) {
                #pragma omp parallel for schedule(static) num_threads(threads)
                for (std::uint64_t c = 0; c < (npos + (1u << 24) - 1) >> 24; ++c)
                        std::memset(u.p + (c << 24), UNKNOWN8, std::size_t(std::min<std::uint64_t>(1u << 24, npos - (c << 24))));
                write_progress(prog, "0\n");
        }
        // the slices to solve: every slice of an asymmetric material, the canonical slices of a symmetric one
        // (whose positions with black to move are stored as the mirrored slice with white to move)
        std::vector<std::pair<int, std::uint64_t>> jobs;
        for (auto const& x : lay)
                for (std::uint64_t r = 0; r < x.men; ++r) {
                        bitboard wm, bm;
                        men_unrank(x.wm, x.bm, r, wm, bm);
                        Key const k{ wm, bm, x.wk, x.bk };
                        if (x.S == 1 && !canonical_key(k)) continue;
                        jobs.push_back({ distance(k), k.pack() });
                }
        std::sort(jobs.begin(), jobs.end());
        // the number of threads for the small slices from the memory budget (0 = no budget)
        int small_threads = threads;
        if (mem_gb > 0) {
                std::uint64_t largest = 0;
                for (auto const& [d, k] : jobs) largest = std::max(largest, 2 * meta_of(Key::unpack(k)).P);
                small_threads = int(std::clamp<double>(mem_gb * 1e9 / double(thread_bytes(largest)), 1, threads));
        }
        static std::unique_ptr<Pool> pool;
        if (!pool || pool->size() != threads) { pool.reset(); pool = std::make_unique<Pool>(threads); }
        std::cerr << "unit " << n << "/" << m << ": " << lay.size() << " materials, " << jobs.size() << " slices, " << npos << " positions"
                  << (resume ? ", resuming after slice " + std::to_string(start) : std::string()) << std::endl;
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t bad = 0;
        // values above dtm8_max: the byte ESC8 and an entry for the overflow table, collected per thread and merged after
        // each distance group (a slice reads only earlier groups, whose entries are merged by then)
        std::string const ovf_path = unit_path(dir, n, m, ".ovf");
        std::vector<std::vector<std::uint64_t>> pending(std::size_t(std::max(threads, small_threads)) + 1);
        auto put = [&](std::uint64_t off, std::uint16_t v) {
                if (v == DRAW) return std::pair(off, DRAW8);
                if (v > dtm8_max) {
                        pending[std::size_t(omp_get_thread_num())].push_back(off << 16 | v);
                        return std::pair(off, ESC8);
                }
                return std::pair(off, std::uint8_t(v));
        };
        auto merge_pending = [&] {
                std::size_t const old = raw_ovf.size();
                for (auto& q : pending) { raw_ovf.insert(raw_ovf.end(), q.begin(), q.end()); q.clear(); }
                std::sort(raw_ovf.begin() + std::ptrdiff_t(old), raw_ovf.end());
                std::inplace_merge(raw_ovf.begin(), raw_ovf.begin() + std::ptrdiff_t(old), raw_ovf.end());
                raw_ovf.erase(std::unique(raw_ovf.begin(), raw_ovf.end()), raw_ovf.end());
                u.ovf = raw_ovf.data(); u.novf = raw_ovf.size();
        };
        // write a solved slice to the unit (a slice is a contiguous range; for a symmetric material the positions with
        // black to move go to the mirrored slice, which is never solved itself)
        auto store = [&](std::size_t i, Meta const& s, std::vector<std::uint16_t> const& tmp) {
                std::uint64_t bad1 = 0;
                MatRef const r = mat_of(pos_of(s, 0, 0));
                std::uint64_t const base = stored_offset(r, pos_of(s, 0, 0));    // king placement 0, white to move
                std::uint64_t const sides = std::uint64_t(r.S);
                for (std::uint64_t x = 0; x < sides * s.P; ++x) { auto [o, b] = put(base + x, tmp[x]); u.p[o] = b; }
                if (r.S == 1)    // black to move: the mirrored position with white to move (in this slice or its mirror)
                        for (std::uint64_t x = 0; x < s.P; ++x) {
                                Pos const q = mirror(pos_of(s, x, 1));
                                auto [o, b] = put(stored_offset(r, q), tmp[s.P + x]);
                                if (u.p[o] != UNKNOWN8 && u.p[o] != b && Key::unpack(jobs[i].second).pack() == key_of(q).pack()) ++bad1;
                                u.p[o] = b;
                        }
                return bad1;
        };
        for (std::size_t g = start; g < jobs.size();) {
                std::size_t h = g;
                while (h < jobs.size() && jobs[h].first == jobs[g].first) ++h;
                // a slice that alone is more than one thread's share of the group is solved by all threads together
                std::uint64_t group = 0;
                for (std::size_t i = g; i < h; ++i) group += 2 * meta_of(Key::unpack(jobs[i].second)).P;
                std::vector<std::size_t> big, small;
                for (std::size_t i = g; i < h; ++i) {
                        std::uint64_t const T = 2 * meta_of(Key::unpack(jobs[i].second)).P;
                        (T >= BIG_SLICE && T * std::uint64_t(threads) > group ? big : small).push_back(i);
                }
                for (std::size_t i : big) {                                     // one at a time, all threads
                        Meta const s = meta_of(Key::unpack(jobs[i].second));
                        std::vector<std::uint16_t> tmp(2 * s.P);
                        solve_slice_par(s, tmp.data(), *pool);
                        bad += store(i, s, tmp);
                }
                #pragma omp parallel for schedule(dynamic, 1) reduction(+ : bad) num_threads(small_threads)
                for (std::size_t k = 0; k < small.size(); ++k) {
                        Meta const s = meta_of(Key::unpack(jobs[small[k]].second));
                        std::vector<std::uint16_t> tmp(2 * s.P);
                        solve_slice(s, tmp.data());
                        bad += store(small[k], s, tmp);
                }
                if (bad) { std::cerr << "unit " << n << "/" << m << ": " << bad << " mirrored positions disagree" << std::endl; return false; }
                merge_pending();
                ::msync(u.p, u.size, MS_SYNC);
                if (!write_ovf(ovf_path, raw_ovf)) { std::cerr << "unit " << n << "/" << m << ": cannot write " << ovf_path << std::endl; return false; }
                write_progress(prog, std::to_string(h) + "\n");
                std::cerr << "unit " << n << "/" << m << ": distance " << jobs[g].first << " done, " << h << " / " << jobs.size() << " slices, "
                          << seconds_since(t0) << " s" << std::endl;
                g = h;
        }
        std::uint64_t unknown = 0;
        #pragma omp parallel for schedule(static) reduction(+ : unknown) num_threads(threads)
        for (std::uint64_t o = 0; o < npos; ++o) unknown += u.p[o] == UNKNOWN8;
        if (unknown) { std::cerr << "unit " << n << "/" << m << ": " << unknown << " positions without value" << std::endl; return false; }
        write_progress(prog, std::to_string(jobs.size()) + " complete\n");
        std::cout << "built " << n << " " << m << " positions " << npos << " slices " << jobs.size() << " overflow " << raw_ovf.size()
                  << " seconds " << seconds_since(t0) << '\n';
        return true;
}

// every stored position of unit (n, m): f(cursor, offset) for each (cursor.pos() is the position), in parallel over
// chunks of positions
template <class F>
void for_each_stored(std::vector<UnitMat> const& lay, int threads, char const* what, F&& f)
{
        std::uint64_t const npos = unit_positions(lay), chunk = 1 << 16, nc = (npos + chunk - 1) / chunk;
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t done = 0;
        std::uint64_t const step = std::max<std::uint64_t>(1, nc / 20);
        #pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
        for (std::uint64_t j = 0; j < nc; ++j) {
                Cursor c(lay);
                c.seek(j * chunk);
                std::uint64_t const end = std::min(npos, (j + 1) * chunk);
                for (std::uint64_t o = j * chunk; o < end; ++o, c.next()) f(std::as_const(c), o);
                std::uint64_t d;
                #pragma omp atomic capture
                d = ++done;
                if (d % step == 0) {
                        #pragma omp critical
                        std::cerr << what << ": " << d << " / " << nc << " chunks, " << seconds_since(t0) << " s" << std::endl;
                }
        }
}

// self-consistency: every stored value equals the value recomputed from the successors (raw: every position;
// compressed: the positions without capture, since a capture position is resolved from its successors anyway).
// In a raw unit a king move without capture stays in the slice, with the other side to move: for an asymmetric
// material (both sides to move stored) that successor is read directly from the neighbouring half of the slice;
// all other successors (man moves, captures, promotions, and king moves of a symmetric material, whose positions
// with black to move are stored mirrored) go through the general lookup.
bool unit_verify(std::string const& dir, int n, int m, bool compressed, int threads)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        if (!store_open(dir, n, m, compressed ? 3 : 0)) return false;
        Unit const& u = units.back();
        std::uint64_t checked = 0, skipped = 0, errors = 0;
        auto const t0 = std::chrono::steady_clock::now();
        std::vector<std::uint64_t> c(3 * std::size_t(threads), 0);
        for_each_stored(lay, threads, "verify", [&](Cursor const& cur, std::uint64_t off) {
                std::uint64_t* t = &c[3 * std::size_t(omp_get_thread_num())];
                Pos const q = cur.pos();
                if (compressed && has_capture(q)) { ++t[1]; return; }
                std::uint16_t b;
                if (!compressed && cur.lay[cur.mi].S == 2) {
                        std::uint64_t const P = cur.meta.P, other = off - cur.k + (cur.stm ? 0 : 2 * P) - P;   // other side to move
                        int best_win = INF, max_loss = -1;
                        bool draw = false;
                        for (auto const& mv : moves(q)) {
                                std::uint64_t const o = other + index_of(cur.meta, mv.next);
                                std::uint16_t const v = internal_move(q, mv) ? value_of(u, o, u.p[o]) : lookup(mv.next);
                                if (v == DRAW) draw = true;
                                else if (v & 1) max_loss = std::max(max_loss, v + 1);
                                else best_win = std::min(best_win, v + 1);
                        }
                        b = best_win < INF ? std::uint16_t(best_win) : draw ? DRAW : std::uint16_t(std::max(max_loss, 0));
                } else b = from_successors(q);
                std::uint16_t const a = compressed ? block_cache.value(u, off) : value_of(u, off, u.p[off]);
                ++t[0];
                if (a != b) {
                        if (++t[2] <= 10) {
                                #pragma omp critical
                                std::cerr << "verify: " << fen_of(q) << " stored " << show(a) << ", from the successors " << show(b) << std::endl;
                        }
                }
        });
        for (int i = 0; i < threads; ++i) { checked += c[3 * i]; skipped += c[3 * i + 1]; errors += c[3 * i + 2]; }
        double const sec = seconds_since(t0);
        #pragma omp parallel num_threads(threads)
        {
                cache_hits += block_cache.hits; cache_misses += block_cache.misses;
                block_cache.hits = block_cache.misses = 0;
        }
        std::cout << "verify " << n << " " << m << (compressed ? " cdb" : " raw") << " positions " << unit_positions(lay) << " checked " << checked
                  << " captures " << skipped << " errors " << errors << " seconds " << sec << " block_reads " << cache_hits + cache_misses
                  << " block_misses " << cache_misses << '\n';
        return errors == 0 && checked + skipped == unit_positions(lay);
}

// the longest win and the longest loss per material of a unit (stored positions, capture positions resolved):
// for each material the largest odd and the largest even DTM, each with the first stored position that has it
bool unit_max(std::string const& dir, int n, int m, int threads)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        if (!store_open(dir, n, m, 3)) return false;
        Unit const& u = units.back();
        struct Best { std::uint16_t v = 0; std::uint64_t off = ~0ull; };
        std::size_t const M = lay.size();
        std::vector<Best> win(std::size_t(threads) * M), loss(std::size_t(threads) * M);
        auto better = [](Best const& a, std::uint16_t v, std::uint64_t off) { return v > a.v || (v == a.v && off < a.off); };
        for_each_stored(lay, threads, "max", [&](Cursor const& cur, std::uint64_t off) {
                Pos const q = cur.pos();
                std::uint16_t const v = has_capture(q) ? lookup(q) : block_cache.value(u, off);
                if (v == DRAW) return;
                std::size_t const i = std::size_t(omp_get_thread_num()) * M + cur.mi;
                Best& b = (v & 1) ? win[i] : loss[i];
                if (better(b, v, off)) b = { v, off };
        });
        for (std::size_t mi = 0; mi < M; ++mi) {
                Best w, l;
                for (int t = 0; t < threads; ++t) {
                        Best const& a = win[std::size_t(t) * M + mi];
                        Best const& b = loss[std::size_t(t) * M + mi];
                        if (a.off != ~0ull && better(w, a.v, a.off)) w = a;
                        if (b.off != ~0ull && better(l, b.v, b.off)) l = b;
                }
                auto fen = [&](Best const& b) { if (b.off == ~0ull) return std::string("-"); Cursor c(lay); c.seek(b.off); return fen_of(c.pos()); };
                UnitMat const& x = lay[mi];
                std::cout << "max " << n << " " << m << " material " << x.wm << " " << x.bm << " " << x.wk << " " << x.bk
                          << " win " << (w.off == ~0ull ? "-" : show(w.v)) << " " << fen(w)
                          << " loss " << (l.off == ~0ull ? "-" : show(l.v)) << " " << fen(l) << '\n';
        }
        return true;
}

// a checksum of the values (not the stored bytes) of a compressed unit: every position without capture with its
// value (overflow entries resolved), order independent; equal for two stores of the same values in different encodings
bool unit_values(std::string const& dir, int n, int m, int threads)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        if (!store_open(dir, n, m, 3)) return false;
        Unit const& u = units.back();
        std::vector<std::uint64_t> sum(std::size_t(threads), 0), cnt(std::size_t(threads), 0);
        std::size_t const M = lay.size();
        std::vector<std::uint64_t> msum(std::size_t(threads) * M, 0), mcnt(std::size_t(threads) * M, 0);
        auto mix = [](std::uint64_t x) { x *= 0x9E3779B97F4A7C15ull; x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ull; x ^= x >> 32; return x; };
        for_each_stored(lay, threads, "values", [&](Cursor const& cur, std::uint64_t off) {
                Pos const q = cur.pos();
                if (has_capture(q)) return;
                std::uint16_t const v = block_cache.value(u, off);
                std::size_t const t = std::size_t(omp_get_thread_num());
                sum[t] += mix(off << 16 | v); ++cnt[t];
                PosKey const k = pack(q);       // keyed by the position, not its offset: the same in any layout
                msum[t * M + cur.mi] += mix(mix(std::uint64_t(k)) ^ std::uint64_t(k >> 64) ^ (std::uint64_t(v) << 48)); ++mcnt[t * M + cur.mi];
        });
        for (std::size_t mi = 0; mi < M; ++mi) {
                std::uint64_t a = 0, b = 0;
                for (int t = 0; t < threads; ++t) { a += msum[std::size_t(t) * M + mi]; b += mcnt[std::size_t(t) * M + mi]; }
                char h[24];
                std::snprintf(h, sizeof h, "%016llx", static_cast<unsigned long long>(a));
                UnitMat const& x = lay[mi];
                std::cout << "material " << x.wm + x.wk << " " << x.bm + x.bk << " " << x.wm << " " << x.bm << " positions " << b << " " << h << '\n';
        }
        std::uint64_t s = 0, c = 0;
        for (int t = 0; t < threads; ++t) { s += sum[std::size_t(t)]; c += cnt[std::size_t(t)]; }
        char hex[24];
        std::snprintf(hex, sizeof hex, "%016llx", static_cast<unsigned long long>(s));
        std::cout << "values " << n << " " << m << " positions " << c << " " << hex << '\n';
        return true;
}

// compress the raw file of unit (n, m) into blocks of 4096 positions (zstd -19)
// file: magic, n, m, materials, positions, block, blocks, 0; per material (code, base); offsets[blocks + 1]; data
bool unit_compress(std::string const& dir, int n, int m, int threads)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        std::uint64_t const npos = unit_positions(lay), nb = (npos + BLOCK - 1) / BLOCK;
        std::size_t rsize = 0;
        std::uint8_t* raw = map_any(unit_path(dir, n, m, ".raw"), rsize, 0, npos);
        if (!raw) return false;
        std::string const path = unit_path(dir, n, m, ".cdb"), tmp = path + ".tmp";
        std::vector<std::uint64_t> const ovf = read_ovf(unit_path(dir, n, m, ".ovf"));
        std::FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) return false;
        // E: the first escape code, as large as the longest DTM allows (255: no escapes)
        unsigned longest = 0;
        for (std::uint64_t o = 0; o < npos; ++o) if (raw[o] <= MAXDTM8 && raw[o] != ESC8) longest = std::max(longest, unsigned(raw[o]));
        for (std::uint64_t x : ovf) longest = std::max(longest, unsigned(x & 0xFFFF));
        unsigned const k = longest <= 254 ? 0 : (longest - 254 + 254) / 255;
        unsigned esc = 255 - k;
        if (char const* e = std::getenv("EGDB_ESC")) esc = std::min(esc, unsigned(std::clamp(std::atoi(e), 1, 255)));
        std::vector<std::uint64_t> head{ CDB_MAGIC2, std::uint64_t(n), std::uint64_t(m), lay.size(), npos, BLOCK, nb, esc };
        for (auto const& x : lay) { head.push_back(x.code()); head.push_back(x.base); }
        std::vector<std::uint64_t> off(nb + 1, 0);
        std::fwrite(head.data(), 8, head.size(), f);
        std::fwrite(off.data(), 8, off.size(), f);
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t const batch = 16384;
        std::vector<std::vector<std::uint8_t>> out(batch);
        std::uint64_t total = 0;
        for (std::uint64_t b0 = 0; b0 < nb; b0 += batch) {
                std::uint64_t const b1 = std::min(nb, b0 + batch);
                #pragma omp parallel num_threads(threads)
                {
                        ZSTD_CCtx* cx = new_cctx();
                        std::vector<std::uint8_t> blk(2 * BLOCK);
                        #pragma omp for schedule(dynamic, 16)
                        for (std::uint64_t b = b0; b < b1; ++b) {
                                std::size_t const len = filled_block(lay, raw, ovf, npos, b, esc, blk.data());
                                auto& o = out[b - b0];
                                o.resize(ZSTD_compressBound(len));
                                std::size_t const r = ZSTD_compress2(cx, o.data(), o.size(), blk.data(), len);
                                if (ZSTD_isError(r)) { std::cerr << "zstd: " << ZSTD_getErrorName(r) << std::endl; std::abort(); }
                                o.resize(r);
                        }
                        ZSTD_freeCCtx(cx);
                }
                for (std::uint64_t b = b0; b < b1; ++b) {
                        off[b] = total;
                        total += out[b - b0].size();
                        std::fwrite(out[b - b0].data(), 1, out[b - b0].size(), f);
                }
                if ((b0 / batch) % 64 == 63) std::cerr << "compress: " << b1 << " / " << nb << " blocks, " << seconds_since(t0) << " s" << std::endl;
        }
        off[nb] = total;
        bool ok = ::fseeko(f, off_t(head.size() * 8), SEEK_SET) == 0 && std::fwrite(off.data(), 8, off.size(), f) == off.size();
        ok = ok && sync_file(f);
        ok = std::fclose(f) == 0 && ok;
        ::munmap(raw, rsize);
        if (!ok || !durable_rename(tmp, path)) return false;
        std::uint64_t const fsize = head.size() * 8 + off.size() * 8 + total;
        std::cout << "compressed " << n << " " << m << " positions " << npos << " blocks " << nb << " bytes " << fsize
                  << " bits_per_position " << 8.0 * double(fsize) / double(npos) << " longest " << longest << " escape " << esc
                  << " seconds " << seconds_since(t0) << '\n';
        return true;
}

// CRC32C (Castagnoli, as in iSCSI and ext4) with the SSE4.2 instruction
std::uint32_t crc32c(std::uint32_t crc, std::uint8_t const* p, std::size_t n)
{
        std::uint64_t c = ~crc;
        std::size_t i = 0;
        for (; i + 8 <= n; i += 8) { std::uint64_t x; std::memcpy(&x, p + i, 8); c = _mm_crc32_u64(c, x); }
        for (; i < n; ++i) c = _mm_crc32_u8(std::uint32_t(c), p[i]);
        return ~std::uint32_t(c);
}

// the checksum of a compressed unit: CRC32C of its stored bytes in order (every block decoded, capture positions as
// filled in), so independent of the zstd version that compressed it; equal for every build of the same unit
bool unit_crc(std::string const& dir, int n, int m)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        if (!store_open(dir, n, m, 3)) return false;
        Unit const& u = units.back();
        ZSTD_DCtx* dx = new_dctx();
        std::vector<std::uint8_t> buf(2 * BLOCK);
        std::uint32_t crc = 0;
        for (std::uint64_t b = 0; b < u.nblocks; ++b) crc = crc32c(crc, buf.data(), decode_block(dx, u, b, buf.data()));   // with extra bytes
        ZSTD_freeDCtx(dx);
        char hex[16];
        std::snprintf(hex, sizeof hex, "%08x", crc);
        std::cout << "crc32c " << n << " " << m << " positions " << u.npos << " " << hex << '\n';
        return true;
}

// recode a compressed unit of version 1 (draw 253) to version 2 (draw 255, no escapes: version 1 has no DTM above 252):
// every block decoded, its draws recoded, compressed again and decoded once more to check it; the values stay the same
bool unit_recode(std::string const& dir, int n, int m, int threads)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        if (!store_open(dir, n, m, 3)) return false;
        Unit const& u = units.back();
        if (u.version == 2) { std::cout << "recoded " << n << " " << m << " already version 2\n"; return true; }
        std::string const path = unit_path(dir, n, m, ".cdb"), tmp = path + ".tmp";
        std::FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) return false;
        std::size_t const nhead = std::size_t(u.data - u.p) / 8 - (u.nblocks + 1);   // header words before the offsets
        std::vector<std::uint64_t> head(nhead);
        std::memcpy(head.data(), u.p, nhead * 8);
        head[0] = CDB_MAGIC2;
        head[7] = 255;
        std::vector<std::uint64_t> off(u.nblocks + 1, 0);
        std::fwrite(head.data(), 8, head.size(), f);
        std::fwrite(off.data(), 8, off.size(), f);
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t const nb = u.nblocks, batch = 16384;
        std::vector<std::vector<std::uint8_t>> out(batch);
        std::uint64_t total = 0, draws = 0;
        std::atomic<bool> bad{false};
        for (std::uint64_t b0 = 0; b0 < nb; b0 += batch) {
                std::uint64_t const b1 = std::min(nb, b0 + batch);
                #pragma omp parallel num_threads(threads) reduction(+ : draws)
                {
                        ZSTD_CCtx* cx = new_cctx();
                        ZSTD_DCtx* dx = new_dctx();
                        std::vector<std::uint8_t> blk(2 * BLOCK), back(2 * BLOCK);
                        #pragma omp for schedule(dynamic, 16)
                        for (std::uint64_t b = b0; b < b1; ++b) {
                                std::size_t const len = decode_block(dx, u, b, blk.data());
                                for (std::size_t i = 0; i < len; ++i) {
                                        if (blk[i] == DRAW8) { blk[i] = DRAW_CODE; ++draws; }
                                        else if (blk[i] > DRAW8) bad = true;
                                }
                                auto& o = out[b - b0];
                                o.resize(ZSTD_compressBound(len));
                                std::size_t const r = ZSTD_compress2(cx, o.data(), o.size(), blk.data(), len);
                                if (ZSTD_isError(r)) { std::cerr << "zstd: " << ZSTD_getErrorName(r) << std::endl; std::abort(); }
                                o.resize(r);
                                std::size_t const g = ZSTD_decompressDCtx(dx, back.data(), back.size(), o.data(), o.size());
                                if (g != len || std::memcmp(back.data(), blk.data(), len) != 0) bad = true;
                        }
                        ZSTD_freeDCtx(dx);
                        ZSTD_freeCCtx(cx);
                }
                for (std::uint64_t b = b0; b < b1; ++b) {
                        off[b] = total;
                        total += out[b - b0].size();
                        std::fwrite(out[b - b0].data(), 1, out[b - b0].size(), f);
                }
        }
        off[nb] = total;
        bool ok = !bad && ::fseeko(f, off_t(head.size() * 8), SEEK_SET) == 0 && std::fwrite(off.data(), 8, off.size(), f) == off.size();
        ok = ok && sync_file(f);
        ok = std::fclose(f) == 0 && ok;
        if (!ok) { std::remove(tmp.c_str()); std::cerr << "recode: unit " << n << "/" << m << " failed" << std::endl; return false; }
        if (!durable_rename(tmp, path)) return false;
        std::cout << "recoded " << n << " " << m << " positions " << u.npos << " draws " << draws << " bytes "
                  << (head.size() + off.size()) * 8 + total << " seconds " << seconds_since(t0) << '\n';
        return true;
}

// compare the compressed unit with the raw one: every block decoded and compared byte for byte with the block as it
// should be stored (the raw values, capture positions filled in). Looking up every position through the compressed
// unit as well would add nothing: a position without capture reads exactly such a byte, and a capture position is
// resolved through the units with fewer pieces, which is the computation that checked its raw value (unitverify).
bool unit_compare(std::string const& dir, int n, int m, int threads)
{
        auto const lay = unit_layout(n, m);
        if (lay.empty()) { std::cout << "empty " << n << " " << m << '\n'; return true; }
        std::uint64_t const npos = unit_positions(lay);
        if (!store_open(dir, n, m, 3)) return false;
        Unit const& u = units.back();
        std::size_t rsize = 0;
        std::uint8_t* raw = map_any(unit_path(dir, n, m, ".raw"), rsize, 0, npos);
        if (!raw) return false;
        std::vector<std::uint64_t> const ovf = read_ovf(unit_path(dir, n, m, ".ovf"));
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t bad_bytes = 0, bytes = 0;
        #pragma omp parallel num_threads(threads) reduction(+ : bad_bytes, bytes)
        {
                ZSTD_DCtx* dx = new_dctx();
                std::vector<std::uint8_t> want(2 * BLOCK), got(2 * BLOCK);
                #pragma omp for schedule(dynamic, 64)
                for (std::uint64_t b = 0; b < u.nblocks; ++b) {
                        std::size_t const g = decode_block(dx, u, b, got.data());
                        std::size_t const w = filled_block(lay, raw, ovf, npos, b, u.esc, want.data());
                        for (std::size_t i = 0; i < std::max(g, w); ++i) bad_bytes += i >= g || i >= w || want[i] != got[i];
                        bytes += std::size_t(std::min(BLOCK, npos - b * BLOCK));
                }
                ZSTD_freeDCtx(dx);
        }
        ::munmap(raw, rsize);
        std::cout << "compare " << n << " " << m << " positions " << npos << " blocks " << u.nblocks << " bytes_compared " << bytes
                  << " byte_errors " << bad_bytes << " overflow " << ovf.size() << " seconds " << seconds_since(t0) << '\n';
        return bad_bytes == 0 && bytes == npos;
}

// order: best move first (fastest win, then draw, then slowest loss)
int score(std::uint16_t v_after)
{
        if (v_after == DRAW) return 0;
        if (!(v_after & 1)) return 100000 - v_after;   // opponent loses: we win
        return -100000 + v_after;                       // opponent wins: we lose
}

} // namespace

int main()
{
        std::ios::sync_with_stdio(false);
        init_tables();
        std::string line;
        while (std::getline(std::cin, line)) {
                std::stringstream ls(line);
                std::string cmd;
                ls >> cmd;
                std::string rest;
                std::getline(ls, rest);
                std::stringstream rs(rest);
                if (cmd == "build") {
                        int n = 8, threads = 4, dmax = 99;
                        rs >> n >> threads >> dmax;
                        build(n, threads, dmax);
                        std::cout << "built " << built_max << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "extend" || cmd == "resume" || cmd == "attach") {
                        int n = 10, threads = 4;
                        std::string path;
                        if (cmd != "attach") rs >> n >> threads;
                        rs >> path;
                        bool ok = cmd == "attach" ? attach(path) : extend(n, threads, path, cmd == "resume");
                        std::cout << (ok ? cmd + " ok " : "error " + cmd + " failed ") << path << " built " << built_max << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "save" || cmd == "load") {
                        std::string path;
                        rs >> path;
                        bool ok = cmd == "save" ? save(path) : load(path);
                        std::cout << (ok ? cmd + " ok " : "error " + cmd + " failed ") << path << " built " << built_max << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "ondemand") {
                        int on = 1;
                        rs >> on;
                        allow_on_demand = on != 0;
                        std::cout << "ondemand " << allow_on_demand << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "slicemax") {
                        rs >> slice_max;
                        slice_max = std::max(slice_max, built_max);
                        std::cout << "slicemax " << slice_max << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "store") {
                        std::string dir;
                        rs >> dir;
                        bool ok = store_open(dir, 13, 0, -1, true);
                        std::cout << (ok ? "store ok " : "error store failed ") << dir << " units " << units.size() << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "unitcrc") {
                        std::string dir;
                        int n = 0, m = 0;
                        rs >> dir >> n >> m;
                        bool ok = unit_crc(dir, n, m);
                        std::cout << (ok ? "unitcrc ok" : "error unitcrc failed") << "\nend" << std::endl;
                        continue;
                }
                if (cmd.rfind("mat", 0) == 0) {
                        // material mode: mat<step> <dir> <wp> <bp> <wm> <bm> [raw|cdb] [threads]
                        std::string dir, kind;
                        int wp = 0, bp = 0, wm = 0, bm = 0, threads = 1;
                        rs >> dir >> wp >> bp >> wm >> bm;
                        if (cmd == "matverify") rs >> kind;
                        rs >> threads;
                        threads = std::max(threads, 1);
                        bool ok = stored_material(wm, bm, wp - wm, bp - bm);
                        if (!ok) std::cout << "error not a stored material " << wp << "-" << bp << "-" << wm << "-" << bm << '\n';
                        else {
                                only_mat = OnlyMat{ true, wm, bm, wp - wm, bp - bm };
                                int const n = wp + bp, m = wm + bm;
                                ok = cmd == "matbuild" ? mat_build(dir, threads)
                                   : cmd == "matverify" ? unit_verify(dir, n, m, kind == "cdb", threads)
                                   : cmd == "matcompress" ? unit_compress(dir, n, m, threads)
                                   : cmd == "matcompare" ? unit_compare(dir, n, m, threads)
                                   : cmd == "matcrc" ? unit_crc(dir, n, m)
                                   : cmd == "matvalues" ? unit_values(dir, n, m, threads)
                                   : cmd == "matmax" ? unit_max(dir, n, m, threads)
                                   : (std::cout << "error unknown command " << cmd << '\n', false);
                                only_mat = OnlyMat{};
                        }
                        std::cout << (ok ? cmd + " ok" : "error " + cmd + " failed") << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "unitrecode") {
                        std::string dir;
                        int n = 0, m = 0, threads = 4;
                        rs >> dir >> n >> m >> threads;
                        bool const ok = unit_recode(dir, n, m, threads);
                        std::cout << (ok ? "unitrecode ok" : "error unitrecode failed") << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "unitvalues") {
                        std::string dir;
                        int n = 0, m = 0, threads = 4;
                        rs >> dir >> n >> m >> threads;
                        bool const ok = unit_values(dir, n, m, threads);
                        std::cout << (ok ? "unitvalues ok" : "error unitvalues failed") << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "unitmax") {
                        std::string dir;
                        int n = 0, m = 0, threads = 4;
                        rs >> dir >> n >> m >> threads;
                        bool const ok = unit_max(dir, n, m, threads);
                        std::cout << (ok ? "unitmax ok" : "error unitmax failed") << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "unitbuild" || cmd == "unitverify" || cmd == "unitcompress" || cmd == "unitcompare") {
                        std::string dir, kind;
                        int n = 0, m = 0, threads = 4;
                        double mem_gb = 0;
                        rs >> dir >> n >> m;
                        if (cmd == "unitverify") rs >> kind;
                        rs >> threads;
                        if (cmd == "unitbuild") rs >> mem_gb;
                        bool ok = cmd == "unitbuild" ? unit_build(dir, n, m, threads, mem_gb)
                                : cmd == "unitverify" ? unit_verify(dir, n, m, kind == "cdb", threads)
                                : cmd == "unitcompress" ? unit_compress(dir, n, m, threads)
                                : unit_compare(dir, n, m, threads);
                        std::cout << (ok ? cmd + " ok" : "error " + cmd + " failed") << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "perft") {
                        // perft <depth> [FEN] [dups]: leaf counts per depth from the position (default: the start position)
                        int depth = 0;
                        rs >> depth;
                        std::string fen = "W:W13,14,15,16,17,18:B1,2,3,4,5,6", word;
                        keep_duplicates = false;
                        while (rs >> word) { if (word == "dups") keep_duplicates = true; else fen = word; }
                        Pos p0;
                        if (!parse_fen(fen, p0)) { std::cout << "error bad fen\nend" << std::endl; keep_duplicates = false; continue; }
                        std::function<std::uint64_t(Pos const&, int)> perft = [&](Pos const& q, int d) -> std::uint64_t {
                                auto const ms = moves(q);
                                if (d == 1) return ms.size();
                                std::uint64_t c = 0;
                                for (auto const& mv : ms) c += perft(mv.next, d - 1);
                                return c;
                        };
                        for (int d = 1; d <= depth; ++d) std::cout << "perft " << d << " " << perft(p0, d) << std::endl;
                        keep_duplicates = false;
                        std::cout << "end" << std::endl;
                        continue;
                }
                if (cmd == "verify") {
                        int threads = 4;
                        rs >> threads;
                        std::cout << "errors " << verify(threads) << "\nend" << std::endl;
                        continue;
                }
                if (cmd == "stats") {
                        std::uint64_t ns = 0;
                        for (auto k : tab_key) ns += k != 0;
                        std::cout << "built " << built_max << " dtm " << dtm_max << " slices " << ns << " dtm bytes " << db.size() << " wdl bytes " << wdlbits.size() * 8
                                  << " extra slices " << extra.size() << " positions " << extra_positions
                                  << " tt " << tt.size() << " nodes " << ab_nodes << "\nend" << std::endl;
                        continue;
                }
                Pos p;
                if (!parse_fen(rest, p)) { std::cout << "error bad fen\nend" << std::endl; continue; }
                try {
                if (cmd == "dtm") {
                        std::uint16_t const v = lookup(p);
                        std::cout << "val " << show(v) << "\nend" << std::endl;
                } else if (cmd == "pos") {
                        std::cout << "val " << show(lookup(p)) << '\n';
                        auto ms = moves(p);
                        std::vector<std::pair<int, std::string>> rows;
                        for (auto const& mv : ms) {
                                std::uint16_t v = lookup(mv.next);
                                std::uint16_t mine = v == DRAW ? DRAW : std::uint16_t(v + 1);
                                rows.push_back({ score(v), "move " + notation(mv) + " " + show(mine) + " " + fen_of(mv.next) });
                        }
                        std::stable_sort(rows.begin(), rows.end(), [](auto const& a, auto const& b) { return a.first > b.first; });
                        for (auto const& r : rows) std::cout << r.second << '\n';
                        std::cout << "end" << std::endl;
                } else if (cmd == "moves") {
                        for (auto const& mv : moves(p)) std::cout << "move " << notation(mv) << ' ' << fen_of(mv.next) << '\n';
                        std::cout << "end" << std::endl;
                } else if (cmd == "wdl") {
                        std::cout << "wdl " << wdl(p) << "\nend" << std::endl;
                } else if (cmd == "posw") {
                        std::cout << "wdl " << wdl(p) << '\n';
                        for (auto const& mv : moves(p)) {
                                char c = wdl(mv.next);
                                std::cout << "move " << notation(mv) << ' ' << (c == 'W' ? 'L' : c == 'L' ? 'W' : 'D') << ' ' << fen_of(mv.next) << '\n';
                        }
                        std::cout << "end" << std::endl;
                } else if (cmd == "slice") {
                        if (!canonical_key(key_of(p))) p = mirror(p);
                        Key const key = key_of(p);
                        Meta const s = meta_of(key);
                        SliceRef const val = slice_values(key);
                        for (int side = 0; side < 2; ++side) {
                                std::uint64_t w = 0, d = 0, l = 0, qn = 0, qd = 0; int mw = 0, ml = 0;
                                for (std::uint64_t i = 0; i < s.P; ++i) {
                                        std::uint16_t v = ref_value(val, side * s.P + i);
                                        Pos q0 = pos_of(s, i, 0), q1 = pos_of(s, i, 1);
                                        if (!has_capture(q0) && !has_capture(q1)) { ++qn; qd += v == DRAW; }
                                        if (v == DRAW) ++d;
                                        else if (v & 1) { ++w; mw = std::max(mw, int(v)); }
                                        else { ++l; ml = std::max(ml, int(v)); }
                                }
                                std::cout << (side ? "black" : "white") << " to move: " << w << " wins (max " << mw << "), "
                                          << d << " draws, " << l << " losses (max " << ml << "); quiet: " << qd << " draws / " << qn << "\n";
                        }
                        std::cout << "end" << std::endl;
                } else if (cmd == "pv") {
                        std::cout << "val " << show(lookup(p)) << '\n' << "pv";
                        bool drawn = lookup(p) == DRAW;
                        for (int ply = 0; ply < 200 && !(drawn && ply >= 20); ++ply) {
                                auto ms = moves(p);
                                if (ms.empty()) break;
                                int bs = -INF; Move const* best = nullptr;
                                for (auto const& mv : ms) {
                                        int sc = score(lookup(mv.next));
                                        if (sc > bs) { bs = sc; best = &mv; }
                                }
                                std::cout << ' ' << notation(*best);
                                p = best->next;
                        }
                        std::cout << "\nend" << std::endl;
                } else {
                        std::cout << "error unknown command\nend" << std::endl;
                }
                } catch (NotInDatabase const&) {
                        std::cout << "error not in database\nend" << std::endl;
                }
        }
}
