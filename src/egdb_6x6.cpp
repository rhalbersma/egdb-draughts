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
// FEN: W:W17,K16:B2,K15 (side to move, white pieces, black pieces; K = king).
// Values: "W<n>" win in n plies, "L<n>" loss in n plies, "D" draw (for the side to move).

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

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

struct Gen {
        Pos const& p;
        int us, them;
        std::uint32_t enemy, empty0;
        int best = 0;
        std::vector<Move>* out;
        int from = 0;
        bool is_king = false;

        Gen(Pos const& pp, std::vector<Move>* o) : p(pp), us(pp.stm), them(1 - pp.stm), out(o)
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

std::vector<Move> moves(Pos const& p)
{
        std::vector<Move> out;
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

std::uint16_t lookup_slice(Pos const& p0)
{
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

bool map_file(std::string const& path, std::size_t size, bool create)
{
        int fd = ::open(path.c_str(), create ? (O_RDWR | O_CREAT | O_TRUNC) : O_RDONLY, 0644);
        if (fd < 0) return false;
        if (create && ::ftruncate(fd, off_t(size)) != 0) { ::close(fd); return false; }
        void* p = ::mmap(nullptr, size, create ? (PROT_READ | PROT_WRITE) : PROT_READ, MAP_SHARED, fd, 0);
        ::close(fd);
        if (p == MAP_FAILED) return false;
        db2 = static_cast<std::uint8_t*>(p);
        db2_size = size;
        return true;
}

// add the layer of all slices with n pieces, with one byte of DTM per position, in a memory-mapped file
// (path) with an index (path.idx); the layers below must be prebuilt with DTM
bool extend(int n, int threads, std::string const& path)
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
        if (!map_file(path, total, true)) { std::cerr << "extend: cannot map " << path << std::endl; return false; }
        entries.insert(entries.end(), layer.begin(), layer.end());
        rebuild_table(entries);
        built_max = n;
        dtm_max = n;
        slice_max = std::max(slice_max, n);
        auto t0 = std::chrono::steady_clock::now();
        std::uint64_t done = 0;
        for (std::size_t g = 0; g < jobs.size();) {
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
                if (h == jobs.size() || jobs[h].men != jobs[g].men) {
                        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                        std::cerr << "extend: " << jobs[g].men << " men done, " << done << " / " << total << " positions, " << sec << " s" << std::endl;
                        ::msync(db2, db2_size, MS_ASYNC);
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
        if (!ok || !map_file(path, hdr[3], false)) return false;
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
        std::vector<std::uint64_t> keys;
        for (auto k : tab_key) if (k) keys.push_back(k - 1);
        for (auto const& [k, v] : extra) keys.push_back(k);
        std::uint64_t errors = 0;
        #pragma omp parallel for schedule(dynamic, 16) reduction(+ : errors) num_threads(threads)
        for (std::size_t i = 0; i < keys.size(); ++i) {
                Key key = Key::unpack(keys[i]);
                Meta s = meta_of(key);
                SliceRef r = slice_values(key);
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
                if (cmd == "extend" || cmd == "attach") {
                        int n = 10, threads = 4;
                        std::string path;
                        if (cmd == "extend") rs >> n >> threads;
                        rs >> path;
                        bool ok = cmd == "extend" ? extend(n, threads, path) : attach(path);
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
