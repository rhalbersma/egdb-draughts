//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Exact endgame databases for international draughts on the 6x6 board, with men and kings.
//
// Rules: international draughts on 6x6: men move one square diagonally forward and capture forwards
// and backwards, flying kings, mandatory capture, majority capture (kings and men count the same),
// Turkish strike (captured pieces are removed after the capture and cannot be jumped twice), a man
// only promotes when it ends its move on the last row. The side to move without a legal move loses.
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
// FEN: W:W17,K16:B2,K15 (side to move, white pieces, black pieces; K = king).
// Values: "W<n>" win in n plies, "L<n>" loss in n plies, "D" draw (for the side to move).

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

namespace {

constexpr int N = 18;
constexpr std::uint32_t FULL = (1u << N) - 1;

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
}

constexpr std::uint32_t PROMO_WHITE = 0b111;                 // squares 1-3
constexpr std::uint32_t PROMO_BLACK = 0b111u << 15;          // squares 16-18

struct Pos {
        std::uint32_t m[2]{};   // men   [0] = white, [1] = black
        std::uint32_t k[2]{};   // kings
        int stm = 0;            // 0 = white to move, 1 = black to move

        std::uint32_t occ() const { return m[0] | m[1] | k[0] | k[1]; }
        bool operator==(Pos const&) const = default;
};

// 180 degree rotation with colours swapped: square n <-> 19 - n, white <-> black
std::uint32_t flip(std::uint32_t x)
{
        std::uint32_t y = 0;
        for (; x; x &= x - 1) y |= 1u << (N - 1 - __builtin_ctz(x));
        return y;
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
        std::uint32_t captured;
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
        std::uint32_t enemy, empty0;
        int best = 0;
        MoveList* out;
        int from = 0;
        bool is_king = false;

        Gen(Pos const& pp, MoveList* o) : p(pp), us(pp.stm), them(1 - pp.stm), out(o)
        {
                enemy = p.m[them] | p.k[them];
        }

        void emit(int to, std::uint32_t cap, int n)
        {
                if (n < best) return;
                if (n > best) { best = n; out->clear(); }
                Pos q = p;
                std::uint32_t fb = 1u << from, tb = 1u << to;
                bool promote = !is_king && ((us == 0 ? PROMO_WHITE : PROMO_BLACK) & tb);
                if (is_king) { q.k[us] = (q.k[us] & ~fb) | tb; }
                else if (promote) { q.m[us] &= ~fb; q.k[us] |= tb; }
                else { q.m[us] = (q.m[us] & ~fb) | tb; }
                q.m[them] &= ~cap; q.k[them] &= ~cap;
                q.stm = them;
                for (auto const& mv : *out) if (mv.next == q) return;  // same resulting position
                out->push_back({ from, to, cap, q });
        }

        // empty: squares that are empty during this capture (origin is vacated; captured pieces stay)
        void capture(int s, std::uint32_t empty, std::uint32_t cap, int n)
        {
                bool any = false;
                for (int d = 0; d < 4; ++d) {
                        auto const& ray = rays[s][d];
                        std::size_t i = 0;
                        if (is_king) while (i < ray.size() && (empty >> ray[i] & 1)) ++i;
                        if (i >= ray.size()) continue;
                        int v = ray[i];
                        if (!(enemy >> v & 1) || (cap >> v & 1)) continue;
                        std::size_t j = i + 1;
                        if (j >= ray.size() || !(empty >> ray[j] & 1)) continue;
                        for (; j < ray.size() && (empty >> ray[j] & 1); ++j) {
                                any = true;
                                capture(ray[j], empty, cap | (1u << v), n + 1);
                                if (!is_king) break;
                        }
                }
                if (!any && n > 0) emit(s, cap, n);
        }

        void run()
        {
                std::uint32_t occ = p.occ();
                std::uint32_t pieces = p.m[us] | p.k[us];
                // captures
                for (std::uint32_t b = pieces; b; b &= b - 1) {
                        from = __builtin_ctz(b);
                        is_king = p.k[us] >> from & 1;
                        capture(from, (~occ & FULL) | (1u << from), 0, 0);
                }
                if (best > 0) return;
                // quiet moves
                for (std::uint32_t b = pieces; b; b &= b - 1) {
                        from = __builtin_ctz(b);
                        is_king = p.k[us] >> from & 1;
                        for (int d = 0; d < 4; ++d) {
                                if (!is_king && (us == 0 ? d >= 2 : d < 2)) continue;
                                for (int t : rays[from][d]) {
                                        if (occ >> t & 1) break;
                                        emit(t, 0, 0);
                                        if (!is_king) break;
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
        std::uint32_t enemy = p.m[them] | p.k[them];
        std::uint32_t empty = ~p.occ() & FULL;
        for (std::uint32_t b = p.m[us]; b; b &= b - 1) {
                int s = __builtin_ctz(b);
                for (int d = 0; d < 4; ++d) {
                        auto const& r = rays[s][d];
                        if (r.size() >= 2 && (enemy >> r[0] & 1) && (empty >> r[1] & 1)) return true;
                }
        }
        for (std::uint32_t b = p.k[us]; b; b &= b - 1) {
                int s = __builtin_ctz(b);
                for (int d = 0; d < 4; ++d) {
                        auto const& r = rays[s][d];
                        std::size_t i = 0;
                        while (i < r.size() && (empty >> r[i] & 1)) ++i;
                        if (i + 1 < r.size() && (enemy >> r[i] & 1) && (empty >> r[i + 1] & 1)) return true;
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
        std::uint32_t wm, bm;
        int nwk, nbk;
        std::uint64_t pack() const { return std::uint64_t(wm) | std::uint64_t(bm) << 18 | std::uint64_t(nwk) << 36 | std::uint64_t(nbk) << 40; }
        static Key unpack(std::uint64_t x)
        {
                return { std::uint32_t(x) & FULL, std::uint32_t(x >> 18) & FULL, int(x >> 36 & 15), int(x >> 40 & 15) };
        }
};

Key key_of(Pos const& p)
{
        return { p.m[0], p.m[1], __builtin_popcount(p.k[0]), __builtin_popcount(p.k[1]) };
}

int pieces(Key const& k) { return __builtin_popcount(k.wm) + __builtin_popcount(k.bm) + k.nwk + k.nbk; }

// slices are stored for one of each pair (slice, mirror(slice)) only
bool canonical_key(Key const& k)
{
        Key m{ flip(k.bm), flip(k.wm), k.nbk, k.nwk };
        return k.pack() <= m.pack();
}

struct Meta {
        Key key;
        std::uint32_t free;
        std::uint64_t nb;        // C(F - nwk, nbk)
        std::uint64_t P;         // positions per side to move
};

Meta meta_of(Key const& key)
{
        Meta s;
        s.key = key;
        s.free = ~(key.wm | key.bm) & FULL;
        int F = __builtin_popcount(s.free);
        s.nb = key.nwk <= F ? binom[F - key.nwk][key.nbk] : 0;
        s.P = key.nwk <= F ? binom[F][key.nwk] * s.nb : 0;
        return s;
}

// colex rank of the bits of x within mask (x subset of mask)
std::uint64_t rank_in(std::uint32_t x, std::uint32_t mask)
{
        std::uint64_t r = 0;
        int pos = 0, i = 0;
        for (std::uint32_t b = mask; b; b &= b - 1, ++pos) {
                int sq = __builtin_ctz(b);
                if (x >> sq & 1) { ++i; r += binom[pos][i]; }
        }
        return r;
}

std::uint32_t unrank_in(std::uint64_t r, int k, std::uint32_t mask)
{
        int sq[N]; int n = 0;
        for (std::uint32_t b = mask; b; b &= b - 1) sq[n++] = __builtin_ctz(b);
        std::uint32_t x = 0;
        for (int i = k; i >= 1; --i) {
                int pos = i - 1;
                while (pos + 1 < n && binom[pos + 1][i] <= r) ++pos;
                r -= binom[pos][i];
                x |= 1u << sq[pos];
                n = pos;
        }
        return x;
}

std::uint64_t index_of(Meta const& s, Pos const& p)
{
        std::uint64_t rw = rank_in(p.k[0], s.free);
        std::uint64_t rb = rank_in(p.k[1], s.free & ~p.k[0]);
        return rw * s.nb + rb;
}

Pos pos_of(Meta const& s, std::uint64_t idx, int stm)
{
        Pos p;
        p.m[0] = s.key.wm; p.m[1] = s.key.bm;
        p.k[0] = unrank_in(idx / s.nb, s.key.nwk, s.free);
        p.k[1] = unrank_in(idx % s.nb, s.key.nbk, s.free & ~p.k[0]);
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
        return PosKey(p.m[0]) | PosKey(p.m[1]) << 18 | PosKey(p.k[0]) << 36 | PosKey(p.k[1]) << 54 | PosKey(p.stm) << 72;
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
        return __builtin_popcount(p.occ()) <= slice_max || (p.k[0] && p.k[1]);
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
                else pri = ms[i].captured ? -WSC + 1000 : -WSC + 500;
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
        return mv.captured == 0 && (p.k[p.stm] >> mv.from & 1);
}

template<class F>
void for_each_predecessor(Meta const& s, Pos const& q, F&& f)
{
        int mover = 1 - q.stm;
        std::uint32_t occ = q.occ();
        for (std::uint32_t b = q.k[mover]; b; b &= b - 1) {
                int t = __builtin_ctz(b);
                for (int d = 0; d < 4; ++d)
                        for (int fr : rays[t][d]) {
                                if (occ >> fr & 1) break;
                                Pos pr = q;
                                pr.k[mover] = (pr.k[mover] & ~(1u << t)) | (1u << fr);
                                pr.stm = mover;
                                if (has_capture(pr)) continue;   // captures are compulsory: no quiet move from pr
                                f(std::uint64_t(mover) * s.P + index_of(s, pr));
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
        for (std::uint32_t b = k.wm; b; b &= b - 1) d += __builtin_ctz(b) / 3;           // white: rows to row 0
        for (std::uint32_t b = k.bm; b; b &= b - 1) d += 5 - __builtin_ctz(b) / 3;       // black: rows to row 5
        return d;
}

// all canonical slices with at most nmax pieces; slices with more than dmax pieces are stored as WDL
// all canonical slices with nmin..nmax pieces, in dependency order
std::vector<Job> make_jobs(int nmin, int nmax)
{
        std::vector<Job> jobs;
        std::uint32_t const allowW = FULL & ~PROMO_WHITE, allowB = FULL & ~PROMO_BLACK;
        std::vector<std::uint32_t> wsets;
        for (std::uint32_t w = allowW;; w = (w - 1) & allowW) {
                if (__builtin_popcount(w) <= nmax) wsets.push_back(w);
                if (w == 0) break;
        }
        for (std::uint32_t wm : wsets) {
                std::uint32_t ab = allowB & ~wm;
                int rest0 = nmax - __builtin_popcount(wm);
                for (std::uint32_t bm = ab;; bm = (bm - 1) & ab) {
                        int rest = rest0 - __builtin_popcount(bm);
                        if (rest >= 0)
                                for (int nwk = 0; nwk <= rest; ++nwk)
                                        for (int nbk = 0; nwk + nbk <= rest; ++nbk) {
                                                Key k{ wm, bm, nwk, nbk };
                                                if (pieces(k) < std::max(1, nmin) || !canonical_key(k)) continue;
                                                Meta m = meta_of(k);
                                                if (m.P == 0) continue;
                                                jobs.push_back({ k.pack(), pieces(k), __builtin_popcount(wm | bm), distance(k), m.P });
                                        }
                        if (bm == 0) break;
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
                        std::fflush(pf);
                        ::fsync(::fileno(pf));
                        std::fclose(pf);
                        std::rename((path + ".progress.tmp").c_str(), (path + ".progress").c_str());
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
                        (king ? p.k[side] : p.m[side]) |= 1u << sq;
                }
                i = j;
        }
        // men on their own promotion row are illegal
        if ((p.m[0] & PROMO_WHITE) || (p.m[1] & PROMO_BLACK)) return false;
        return true;
}

std::string fen_of(Pos const& p)
{
        std::string s = p.stm ? "B" : "W";
        for (int side = 0; side < 2; ++side) {
                s += side ? ":B" : ":W";
                bool first = true;
                for (int sq = 0; sq < N; ++sq) {
                        bool m = p.m[side] >> sq & 1, k = p.k[side] >> sq & 1;
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
        return std::to_string(mv.from + 1) + (mv.captured ? "x" : "-") + std::to_string(mv.to + 1);
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

constexpr std::uint32_t MEN_W = FULL & ~PROMO_WHITE;      // squares where a white man can stand
constexpr std::uint32_t MEN_B = FULL & ~PROMO_BLACK;      // squares where a black man can stand
constexpr std::uint32_t MEN_MID = MEN_W & MEN_B;          // squares 4-15
constexpr std::uint64_t BLOCK = 4096;                     // positions (bytes) per compressed block
constexpr std::uint64_t CDB_MAGIC = 0x3678364244424331ull;
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
std::uint64_t men_rank(std::uint32_t wm, std::uint32_t bm)
{
        int const nwm = __builtin_popcount(wm), nbm = __builtin_popcount(bm), j = __builtin_popcount(wm & PROMO_BLACK);
        std::uint64_t r = 0;
        for (int i = 0; i < j; ++i) r += men_part(nwm, nbm, i);
        std::uint64_t const cm = binom[12][nwm - j], cb = binom[15 - (nwm - j)][nbm];
        return r + (rank_in(wm & PROMO_BLACK, PROMO_BLACK) * cm + rank_in(wm & MEN_MID, MEN_MID)) * cb + rank_in(bm, MEN_B & ~wm);
}

void men_unrank(int nwm, int nbm, std::uint64_t r, std::uint32_t& wm, std::uint32_t& bm)
{
        int j = 0;
        while (r >= men_part(nwm, nbm, j)) r -= men_part(nwm, nbm, j++);
        std::uint64_t const cm = binom[12][nwm - j], cb = binom[15 - (nwm - j)][nbm];
        std::uint64_t const c = r % cb;
        r /= cb;
        wm = unrank_in(r / cm, j, PROMO_BLACK) | unrank_in(r % cm, nwm - j, MEN_MID);
        bm = unrank_in(c, nbm, MEN_B & ~wm);
}

struct UnitMat {
        int wm, bm, wk, bk;          // numbers of white men, black men, white kings, black kings
        std::uint64_t base, men, P;  // first position, placements of the men, placements of the kings
        int S;                       // sides to move stored: 2, or 1 for a symmetric material (white to move)
        std::uint64_t code() const { return std::uint64_t(wm) | std::uint64_t(bm) << 8 | std::uint64_t(wk) << 16 | std::uint64_t(bk) << 24 | std::uint64_t(S) << 32; }
        std::uint64_t size() const { return men * std::uint64_t(S) * P; }
};

std::vector<UnitMat> unit_layout(int n, int m)
{
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
        auto const a = std::tuple(__builtin_popcount(p.m[0]), __builtin_popcount(p.m[1]), __builtin_popcount(p.k[0]), __builtin_popcount(p.k[1]));
        auto const b = std::tuple(std::get<1>(a), std::get<0>(a), std::get<3>(a), std::get<2>(a));
        if (a < b) return p;
        if (b < a) return mirror(p);
        return p.stm == 0 ? p : mirror(p);
}

std::uint64_t king_index(Pos const& p)
{
        std::uint32_t const free = ~(p.m[0] | p.m[1]) & FULL;
        int const F = __builtin_popcount(free);
        std::uint64_t const nb = binom[F - __builtin_popcount(p.k[0])][__builtin_popcount(p.k[1])];
        return rank_in(p.k[0], free) * nb + rank_in(p.k[1], free & ~p.k[0]);
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
};

struct MatRef { int unit = -1; int S = 0; std::uint64_t base = 0, P = 0; };

std::vector<Unit> units;
MatRef mat_ref[7][7][7][7];
std::uint64_t next_unit_id = 1;
std::uint64_t store_gen = 0;           // changes whenever the store is opened again (capture cache tag)

std::string unit_path(std::string const& dir, int n, int m, char const* ext)
{
        return dir + "/6x6_" + std::to_string(n) + "_" + std::to_string(m) + ext;
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
        } else {
                std::string const path = unit_path(dir, n, m, ".cdb");
                u.p = map_any(path, u.size, 0);
                if (!u.p) { std::cerr << "store: cannot map " << path << std::endl; return false; }
                auto const* h = reinterpret_cast<std::uint64_t const*>(u.p);
                bool ok = u.size >= 64 && h[0] == CDB_MAGIC && h[1] == std::uint64_t(n) && h[2] == std::uint64_t(m) && h[3] == lay.size()
                       && h[4] == u.npos && h[5] == BLOCK && h[6] == (u.npos + BLOCK - 1) / BLOCK;
                for (std::size_t i = 0; ok && i < lay.size(); ++i) ok = h[8 + 2 * i] == lay[i].code() && h[9 + 2 * i] == lay[i].base;
                if (!ok) { std::cerr << "store: bad header in " << path << std::endl; ::munmap(u.p, u.size); return false; }
                u.nblocks = h[6];
                u.off = h + 8 + 2 * lay.size();
                u.data = reinterpret_cast<std::uint8_t const*>(u.off + u.nblocks + 1);
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

void decode_block(ZSTD_DCtx* dx, Unit const& u, std::uint64_t b, std::uint8_t* out)
{
        std::size_t const want = std::size_t(std::min(BLOCK, u.npos - b * BLOCK));
        std::size_t const r = ZSTD_decompressDCtx(dx, out, BLOCK, u.data + u.off[b], std::size_t(u.off[b + 1] - u.off[b]));
        if (ZSTD_isError(r) || r != want) {
                std::cerr << "store: block " << b << " of unit " << u.n << "/" << u.m << " does not decode" << std::endl;
                std::abort();
        }
}

std::size_t cache_bits = [] { char const* e = std::getenv("EGDB_CACHE_BITS"); return std::size_t(e ? std::atoi(e) : 14); }();
std::atomic<std::uint64_t> cache_hits{0}, cache_misses{0};
struct BlockCache {
        std::size_t const SLOTS = std::size_t(1) << cache_bits;
        std::vector<std::uint64_t> tag = std::vector<std::uint64_t>(SLOTS, 0);
        std::vector<std::uint8_t> buf = std::vector<std::uint8_t>(SLOTS * BLOCK);
        ZSTD_DCtx* dx = new_dctx();
        std::uint64_t hits = 0, misses = 0;
        ~BlockCache() { ZSTD_freeDCtx(dx); }
        std::uint8_t byte(Unit const& u, std::uint64_t off)
        {
                std::uint64_t const b = off / BLOCK, id = u.id << 40 | b;
                std::size_t const s = std::size_t((id * 0x9E3779B97F4A7C15ull) >> (64 - cache_bits));
                std::uint8_t* d = buf.data() + s * BLOCK;
                if (tag[s] != id) { decode_block(dx, u, b, d); tag[s] = id; ++misses; }
                else ++hits;
                return d[off % BLOCK];
        }
};
thread_local BlockCache block_cache;

std::uint64_t stored_offset(MatRef const& r, Pos const& q)
{
        return r.base + (men_rank(q.m[0], q.m[1]) * std::uint64_t(r.S) + std::uint64_t(r.S == 2 ? q.stm : 0)) * r.P + king_index(q);
}

MatRef const& mat_of(Pos const& q)
{
        return mat_ref[__builtin_popcount(q.m[0])][__builtin_popcount(q.m[1])][__builtin_popcount(q.k[0])][__builtin_popcount(q.k[1])];
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
        int const w = __builtin_popcount(p.m[0] | p.k[0]), b = __builtin_popcount(p.m[1] | p.k[1]);
        if ((p.stm ? b : w) == 0) return 0;                     // no pieces: no moves
        if (w == 0 || b == 0) return from_successors(p);
        if (w > 6 || b > 6) throw NotInDatabase{};
        Pos const q = canon(p);
        MatRef const& r = mat_of(q);
        if (r.unit < 0) throw NotInDatabase{};
        Unit const& u = units[std::size_t(r.unit)];
        if (!u.raw && has_capture(q)) return resolve_capture(q);
        std::uint64_t const off = stored_offset(r, q);
        std::uint8_t const v = u.raw ? u.p[off] : block_cache.byte(u, off);
        if (v > DRAW8) { std::cerr << "store: no value yet for " << fen_of(p) << std::endl; std::abort(); }
        return widen(v);
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
                std::uint32_t wm, bm;
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
void filled_block(std::vector<UnitMat> const& lay, std::uint8_t const* raw, std::uint64_t npos, std::uint64_t b, std::uint8_t* out)
{
        Cursor c(lay);
        c.seek(b * BLOCK);
        std::uint64_t const end = std::min(npos, (b + 1) * BLOCK);
        int prev = -1;
        for (std::uint64_t o = b * BLOCK; o < end; ++o, c.next()) {
                int v = raw[o];
                if (prev >= 0 && has_capture(c.pos())) v = prev;
                out[o - b * BLOCK] = std::uint8_t(v);
                prev = v;
        }
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
                std::fflush(f);
                ::fsync(::fileno(f));
                std::fclose(f);
                std::rename(tmp.c_str(), path.c_str());
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
        return largest * 4 + largest / 8 + (std::uint64_t(1) << cache_bits) * (BLOCK + 8) + CaptureCache::SLOTS * 26;
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
                        std::uint32_t wm, bm;
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
        auto put = [&](std::uint64_t off, std::uint16_t v) {
                if (v != DRAW && v > MAXDTM8) { std::cerr << "DTM overflow (" << v << ")" << std::endl; std::abort(); }
                std::uint8_t const b = v == DRAW ? DRAW8 : std::uint8_t(v);
                return std::pair(off, b);
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
                ::msync(u.p, u.size, MS_SYNC);
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
        std::cout << "built " << n << " " << m << " positions " << npos << " slices " << jobs.size() << " seconds " << seconds_since(t0) << '\n';
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
                                std::uint16_t const v = internal_move(q, mv) ? widen(u.p[other + index_of(cur.meta, mv.next)]) : lookup(mv.next);
                                if (v == DRAW) draw = true;
                                else if (v & 1) max_loss = std::max(max_loss, v + 1);
                                else best_win = std::min(best_win, v + 1);
                        }
                        b = best_win < INF ? std::uint16_t(best_win) : draw ? DRAW : std::uint16_t(std::max(max_loss, 0));
                } else b = from_successors(q);
                std::uint16_t const a = widen(compressed ? block_cache.byte(u, off) : u.p[off]);
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
        std::FILE* f = std::fopen(tmp.c_str(), "wb");
        if (!f) return false;
        std::vector<std::uint64_t> head{ CDB_MAGIC, std::uint64_t(n), std::uint64_t(m), lay.size(), npos, BLOCK, nb, 0 };
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
                        std::vector<std::uint8_t> blk(BLOCK);
                        #pragma omp for schedule(dynamic, 16)
                        for (std::uint64_t b = b0; b < b1; ++b) {
                                filled_block(lay, raw, npos, b, blk.data());
                                std::size_t const len = std::size_t(std::min(BLOCK, npos - b * BLOCK));
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
        ok = ok && std::fflush(f) == 0 && ::fsync(::fileno(f)) == 0;
        ok = std::fclose(f) == 0 && ok;
        ::munmap(raw, rsize);
        if (!ok || std::rename(tmp.c_str(), path.c_str()) != 0) return false;
        std::uint64_t const fsize = head.size() * 8 + off.size() * 8 + total;
        std::cout << "compressed " << n << " " << m << " positions " << npos << " blocks " << nb << " bytes " << fsize
                  << " bits_per_position " << 8.0 * double(fsize) / double(npos) << " seconds " << seconds_since(t0) << '\n';
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
        std::vector<std::uint8_t> buf(BLOCK);
        std::uint32_t crc = 0;
        for (std::uint64_t b = 0; b < u.nblocks; ++b) {
                decode_block(dx, u, b, buf.data());
                crc = crc32c(crc, buf.data(), std::size_t(std::min(BLOCK, u.npos - b * BLOCK)));
        }
        ZSTD_freeDCtx(dx);
        char hex[16];
        std::snprintf(hex, sizeof hex, "%08x", crc);
        std::cout << "crc32c " << n << " " << m << " positions " << u.npos << " " << hex << '\n';
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
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t bad_bytes = 0, bytes = 0;
        #pragma omp parallel num_threads(threads) reduction(+ : bad_bytes, bytes)
        {
                ZSTD_DCtx* dx = new_dctx();
                std::vector<std::uint8_t> want(BLOCK), got(BLOCK);
                #pragma omp for schedule(dynamic, 64)
                for (std::uint64_t b = 0; b < u.nblocks; ++b) {
                        decode_block(dx, u, b, got.data());
                        filled_block(lay, raw, npos, b, want.data());
                        std::size_t const len = std::size_t(std::min(BLOCK, npos - b * BLOCK));
                        for (std::size_t i = 0; i < len; ++i) bad_bytes += want[i] != got[i];
                        bytes += len;
                }
                ZSTD_freeDCtx(dx);
        }
        ::munmap(raw, rsize);
        std::cout << "compare " << n << " " << m << " positions " << npos << " blocks " << u.nblocks << " bytes_compared " << bytes
                  << " byte_errors " << bad_bytes << " seconds " << seconds_since(t0) << '\n';
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
