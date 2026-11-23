//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Per-material building blocks for 6x6 international draughts (or the board and rules egdb_6x6.cpp was built for:
// EGDB_WIDTH, EGDB_HEIGHT, EGDB_6X6_CHECKERS; the numbers in the comments are those of 6x6), shared by thompson86/thompson86.cpp and
// tbgen/tbgen.cpp: the metrics (dtm, dtc, dtz) and the moves that end their count, the materials (named
// wp-bp-wm-bm) and their index (a product of four colex ranks), bit planes, groups of egdb_6x6 slices solved together
// and their unmoves, and the list of materials. Included after src/egdb_6x6.cpp (with its main renamed), whose
// positions, moves, slices and ranks it uses.

#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <array>
#include <atomic>
#include <filesystem>
#include <execution>
#include <new>
#include <type_traits>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

// ---------------------------------------------------------------- binomials up to C(N, N) (at least C(64, 64); the large
// ones wrap around, but only those with few pieces are used)

constexpr std::size_t CC_MAX = std::max(N, 64);
std::array<std::array<std::uint64_t, CC_MAX + 1>, CC_MAX + 1> CC{};

void init_cc()
{
        for (std::size_t n = 0; n <= CC_MAX; ++n) {
                CC[n][0] = 1;
                for (std::size_t k = 1; k <= n; ++k) CC[n][k] = CC[n - 1][k - 1] + (k <= n - 1 ? CC[n - 1][k] : 0);
        }
}

// ---------------------------------------------------------------- metric and style

enum class Metric { dtm, dtc, dtz };
Metric g_metric = Metric::dtm;
[[maybe_unused]] bool g_waves = false;                                   // thompson86's 1996 pass style (masked late)

// ---------------------------------------------------------------- granularity: how finely a material class is cut into groups
//
//   material        the whole material class, king and man moves inside (dtz: one total tempo at a time, since a man
//                   move ends the count there)
//   tempi           one (White tempo, Black tempo) pair: each colour's men's distance to promotion, king moves inside
//   configuration   one full configuration of the men, king moves inside
// Below material granularity every man move is an exit, into a group solved earlier (it lowers its colour's tempo).
enum class Granularity { material, tempi, configuration };
Granularity g_granularity = Granularity::configuration;

bool parse_granularity(std::string const& s)
{
        if (s == "material") g_granularity = Granularity::material;
        else if (s == "tempi") g_granularity = Granularity::tempi;
        else if (s == "configuration") g_granularity = Granularity::configuration;
        else return false;
        return true;
}

[[maybe_unused]] char const* granularity_name()
{
        switch (g_granularity) {
        case Granularity::material: return "material";
        case Granularity::tempi: return "tempi";
        case Granularity::configuration: return "configuration";
        }
        return "?";
}

// man moves stay inside a group only at material granularity, and not in dtz (a man move ends the count there)
bool men_inside() { return g_granularity == Granularity::material && g_metric != Metric::dtz; }

bool parse_metric(std::string const& s)
{
        if (s == "dtm") g_metric = Metric::dtm;
        else if (s == "dtc") g_metric = Metric::dtc;
        else if (s == "dtz") g_metric = Metric::dtz;
        else return false;
        return true;
}

bool is_capture(Move const& mv) { return !mv.captured.empty(); }
bool is_promotion(Pos const& p, Move const& mv) { return mv.next.k[p.stm].size() > p.k[p.stm].size(); }
bool is_man_move(Pos const& p, Move const& mv) { return !p.k[p.stm].contains(std::size_t(mv.from)); }

// a move that ends the count
bool goal_move(Pos const& p, Move const& mv)
{
        switch (g_metric) {
        case Metric::dtm: return false;
        case Metric::dtc: return is_capture(mv) || is_promotion(p, mv);
        case Metric::dtz: return is_capture(mv) || is_man_move(p, mv);
        }
        return false;
}

// a move that stays inside the group being solved
bool inside(Pos const& p, Move const& mv)
{
        if (is_capture(mv) || is_promotion(p, mv)) return false;
        if (!is_man_move(p, mv)) return true;                          // a king move without capture
        return men_inside();                                            // a man move: inside only at material granularity
}

// ---------------------------------------------------------------- materials and their index

struct Material {
        int wm, wk, bm, bk;
        auto operator<=>(Material const&) const = default;
        int pieces() const { return wm + wk + bm + bk; }
        int wp() const { return wm + wk; }
        int bp() const { return bm + bk; }
        std::string name() const                        // wp-bp-wm-bm
        {
                return std::format("{:02}-{:02}-{:02}-{:02}", wp(), bp(), wm, bm);
        }
        std::string tree() const                        // its directory, the subdivisions above it: db/np/wp-bp/
        {
                return std::format("{}/{:02}/{:02}-{:02}/", board_name(), pieces(), wp(), bp());
        }
        std::string stem() const                        // its files' names in that directory start with wm-bm
        {
                return std::format("{:02}-{:02}", wm, bm);
        }
        std::string path(std::string const& dir) const  // dir/db/np/wp-bp/wm-bm, to append -<kind>-<side>
        {
                return dir + "/" + tree() + stem();
        }
};

Material material_of(Pos const& p)
{
        return { int(p.m[0].size()), int(p.k[0].size()), int(p.m[1].size()), int(p.k[1].size()) };
}

constexpr bitboard DOM_WM = FULL - PROMO_WHITE;    // squares a white man may stand on (15 on 6x6)
constexpr bitboard DOM_BM = FULL - PROMO_BLACK;    // squares a black man may stand on (15 on 6x6)

// The index of a material's files. Joint (the default): the white men over the 15 squares a white man may stand on, the
// black men over theirs that the white men leave (an offset per white-men rank), the white kings over the empty
// squares, the black kings over what is left; every slot is a legal placement, as in egdb_6x6 (half the slots of the
// factorised index at 6 pieces, far fewer at 12). Factorised (-DEGDB_JOINT_INDEX=0): four independent colex ranks, the
// kings over all 18 squares; pieces may share a square, so some slots are illegal (one popcount tells them apart).
#ifndef EGDB_JOINT_INDEX
#define EGDB_JOINT_INDEX 1
#endif
// The joint index's two parts also number the positions of the groups (whichever index the files use): the men index of
// a configuration of the men (white men, then black men over what the white men leave) and the king index within it
// (white kings over the free squares, black kings over what is left). Every configuration of a material leaves the same
// number of free squares, so it holds the same number K = nk1 nk2 of king placements.
struct MatIndex {
        Material c;
        std::uint64_t swm, sbm, swk, sbk, size;
        std::uint64_t nk1 = 0, nk2 = 0, men = 0;                // king placements (white, black); configurations
        std::vector<std::uint64_t> off;                         // per white-men rank: its first men index

        explicit MatIndex(Material m)
        :
                c(m), swm(CC[DOM_WM.size()][std::size_t(m.wm)]), sbm(CC[DOM_BM.size()][std::size_t(m.bm)]), swk(CC[N][std::size_t(m.wk)]),
                sbk(CC[N][std::size_t(m.bk)]), size(swm * sbm * swk * sbk)
        {
                int const e = N - m.wm - m.bm;
                if (e >= m.wk + m.bk) {
                        nk1 = CC[std::size_t(e)][std::size_t(m.wk)];
                        nk2 = CC[std::size_t(e - m.wk)][std::size_t(m.bk)];
                }
                off.resize(swm + 1);
                for (std::uint64_t r = 0; r < swm; ++r) {
                        off[r] = men;
                        men += CC[(DOM_BM - unrank_in(r, m.wm, DOM_WM)).size()][std::size_t(m.bm)];
                }
                off[swm] = men;
#if EGDB_JOINT_INDEX
                size = men * nk1 * nk2;
#endif
        }

        std::uint64_t kings() const { return nk1 * nk2; }      // K: king placements per configuration

        std::uint64_t men_index(bitboard const& wm, bitboard const& bm) const
        {
                return off[rank_in(wm, DOM_WM)] + rank_in(bm, DOM_BM - wm);
        }

        std::uint64_t king_index(Pos const& p) const
        {
                bitboard const free = FULL - p.m[0] - p.m[1];
                return rank_in(p.k[0], free) * nk2 + rank_in(p.k[1], free - p.k[0]);
        }

        // the position of configuration mi (a men index) and king placement ki
        void place(std::uint64_t mi, std::uint64_t ki, int stm, Pos& p) const
        {
                std::uint64_t const rw = std::uint64_t(std::ranges::upper_bound(off, mi) - off.begin()) - 1;
                p.stm = stm;
                p.m[0] = unrank_in(rw, c.wm, DOM_WM);
                p.m[1] = unrank_in(mi - off[rw], c.bm, DOM_BM - p.m[0]);
                bitboard const free = FULL - p.m[0] - p.m[1];
                p.k[0] = unrank_in(ki / nk2, c.wk, free);
                p.k[1] = unrank_in(ki % nk2, c.bk, free - p.k[0]);
        }

        std::uint64_t index(Pos const& p) const
        {
#if EGDB_JOINT_INDEX
                return men_index(p.m[0], p.m[1]) * kings() + king_index(p);
#else
                return ((rank_in(p.m[0], DOM_WM) * sbm + rank_in(p.m[1], DOM_BM)) * swk + rank_in(p.k[0], FULL)) * sbk
                     + rank_in(p.k[1], FULL);
#endif
        }

        // the position of a slot; false for an illegal slot (factorised: pieces sharing a square)
        bool pos(std::uint64_t slot, int stm, Pos& p) const
        {
                p.stm = stm;
#if EGDB_JOINT_INDEX
                place(slot / kings(), slot % kings(), stm, p);
                return true;
#else
                std::uint64_t const rbk = slot % sbk, t1 = slot / sbk, rwk = t1 % swk, t2 = t1 / swk;
                p.m[0] = unrank_in(t2 / sbm, c.wm, DOM_WM);
                p.m[1] = unrank_in(t2 % sbm, c.bm, DOM_BM);
                p.k[0] = unrank_in(rwk, c.wk, FULL);
                p.k[1] = unrank_in(rbk, c.bk, FULL);
                return int((p.m[0] | p.m[1] | p.k[0] | p.k[1]).size()) == c.pieces();   // no two pieces on one square
#endif
        }
};

char const* metric_name()
{
        return g_metric == Metric::dtm ? "dtm" : g_metric == Metric::dtc ? "dtc" : "dtz";
}
// ---------------------------------------------------------------- the index of every material, memoised

std::map<Material, MatIndex> g_index;

MatIndex const& mat_index(Material m)
{
        auto it = g_index.find(m);
        if (it == g_index.end()) it = g_index.emplace(m, MatIndex(m)).first;
        return it->second;
}

// a finished file, memory-mapped read-only (random reads)
std::uint8_t const* map_file(std::string const& path, std::size_t size)
{
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) { std::cerr << "missing " << path << std::endl; std::exit(1); }
        struct stat st{};
        ::fstat(fd, &st);
        if (std::size_t(st.st_size) != size) { std::cerr << "wrong size " << path << std::endl; std::exit(1); }
        void* m = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        ::close(fd);
        if (m == MAP_FAILED) { std::cerr << "mmap failed " << path << std::endl; std::exit(1); }
        return static_cast<std::uint8_t const*>(m);
}

// ---------------------------------------------------------------- bit planes

// ---------------------------------------------------------------- seq or par, over blocks of positions
//
// Every pass is one std::for_each over blocks of B indices, with the build's execution policy (std::execution::seq or
// std::execution::par, chosen once in main and passed down). The default, block_size, is a whole number of 64-bit words, so a
// block owns the words of its positions in every bit plane: a pass that writes bit planes needs B a multiple of 64 (or
// atomic writes).
// The cache line, as the compiler knows it for the target (64 bytes on x86-64). A block of B indices covers B / 8 bytes
// of a bit plane and B bytes of a byte array: with B a multiple of 8 lines, no two blocks share a cache line in any of
// them, so parallel blocks never write the same line (no false sharing) and own whole 64-bit words.
// (GCC warns that the value follows -march/-mtune, which matters only where it enters an ABI; here it is only asserted)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#endif
inline constexpr std::size_t cache_line = std::hardware_destructive_interference_size;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

// The default block: 4096 indices (8 cache lines of a bit plane). Passes with per-block state (pass 0's lists) size and
// index it by block_size and num_blocks, so they always agree with for_blocks. EGDB_BLOCK_SIZE overrides it at compile
// time, to measure.
#ifndef EGDB_BLOCK_SIZE
#define EGDB_BLOCK_SIZE 4096
#endif
inline constexpr std::uint64_t block_size = EGDB_BLOCK_SIZE;
constexpr std::uint64_t num_blocks(std::uint64_t n) { return (n + block_size - 1) / block_size; }

template<class ExecutionPolicy>
constexpr bool is_parallel_policy = !std::is_same_v<std::remove_cvref_t<ExecutionPolicy>, std::execution::sequenced_policy>;

template<std::uint64_t B = block_size, class ExecutionPolicy, class F>
void for_blocks(ExecutionPolicy&& policy, std::uint64_t n, F&& f)
{
        static_assert(B > 0 && B % (8 * cache_line) == 0, "for_blocks: a block covers whole cache lines of every bit plane");
        std::vector<std::uint64_t> starts;
        for (std::uint64_t s = 0; s < n; s += B) starts.push_back(s);
        auto const run = [&](std::uint64_t s) { f(s, std::min(n, s + B)); };
        std::for_each(policy, starts.begin(), starts.end(), run);
}

struct Plane {
        std::vector<std::uint64_t> w;
        explicit Plane(std::uint64_t n = 0) : w((n + 63) / 64, 0) {}
        bool test(std::uint64_t i) const { return w[i >> 6] >> (i & 63) & 1; }
        void set(std::uint64_t i) { w[i >> 6] |= std::uint64_t(1) << (i & 63); }
        void set_atomic(std::uint64_t i)                // a scatter from several threads (idempotent)
        {
                std::atomic_ref<std::uint64_t>(w[i >> 6]).fetch_or(std::uint64_t(1) << (i & 63), std::memory_order_relaxed);
        }
        template<class ExecutionPolicy, class F>
        void for_each_par(ExecutionPolicy&& policy, F&& f) const        // over the set bits, one std::for_each over words
        {
                for_blocks(policy, w.size(), [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t k = lo; k < hi; ++k)
                                for (std::uint64_t b = w[k]; b; b &= b - 1) f(k * 64 + std::uint64_t(std::countr_zero(b)));
                });
        }
        template<class ExecutionPolicy>
        void mask_par(ExecutionPolicy&& policy, Plane const& keep, bool complement)     // this &= keep (or ~keep), one std::for_each over words
        {
                for_blocks(policy, w.size(), [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t k = lo; k < hi; ++k) w[k] &= complement ? ~keep.w[k] : keep.w[k];
                });
        }
        void clear() { std::ranges::fill(w, 0); }
        std::uint64_t count() const { std::uint64_t n = 0; for (auto x : w) n += std::uint64_t(std::popcount(x)); return n; }
        bool empty() const { return std::ranges::all_of(w, [](std::uint64_t x) { return x == 0; }); }
        void mask(Plane const& keep, bool complement)   // this &= keep (or ~keep): one sequential pass
        {
                for (std::size_t k = 0; k < w.size(); ++k) w[k] &= complement ? ~keep.w[k] : keep.w[k];
        }
        template<class F> void for_each(F&& f) const
        {
                for (std::size_t k = 0; k < w.size(); ++k)
                        for (std::uint64_t b = w[k]; b; b &= b - 1) f(std::uint64_t(k) * 64 + std::uint64_t(std::countr_zero(b)));
        }
};

// ---------------------------------------------------------------- a group of configurations, solved together
//
// A group is a set of configurations of the men (each with all its K king placements), numbered by MatIndex's men and
// king indices: the whole material (`all`: the group's index is the joint index), or a list of men indices in ascending
// order (local index = the rank of the men index in the list x K + the king index). The configurations of a group are
// those with a given total tempo (`distance`, -1: any) and White tempo (`white`, -1: any), or the one configuration
// `only`. The sizes come from counting (tempo_counts) and the lists are generated on demand (load) by walking the rows
// for the exact tempi, so a build holds only the lists it needs: none for a whole material, one group's for dtz.
struct Group {
        MatIndex const* mi = nullptr;
        bool all = false;
        int distance = -1, white = -1;
        std::uint64_t only = ~std::uint64_t(0);
        std::uint64_t K = 0;                                            // king placements per configuration
        std::uint64_t count = 0;                                        // configurations
        std::uint64_t P = 0;                                            // positions per side to move: count x K
        std::vector<std::uint64_t> men;                                 // the men indices (after load), ascending

        bool loaded() const { return all || men.size() == count; }
        void load();
        void release() { if (!all) std::vector<std::uint64_t>().swap(men); }
        std::uint64_t configurations() const { return count; }

        std::uint64_t index(Pos const& p) const
        {
                std::uint64_t const m = mi->men_index(p.m[0], p.m[1]);
                std::uint64_t const u = all ? m : std::uint64_t(std::ranges::lower_bound(men, m) - men.begin());
                return u * K + mi->king_index(p);
        }

        Pos pos(std::uint64_t g, int stm) const
        {
                Pos p;
                mi->place(all ? g / K : men[g / K], g % K, stm, p);
                return p;
        }
};

// the tempo of a colour's men: the rows they still have to go (White up to row 0, Black down to the last row)
int white_tempo(bitboard const& wm) { int t = 0; for (auto const sq : wm) t += row_of(int(sq)); return t; }
int black_tempo(bitboard const& bm) { int t = 0; for (auto const sq : bm) t += BOARD_H - 1 - row_of(int(sq)); return t; }

// every k-subset of mask whose squares add up to tempo t, row by row (weight(r): the tempo of a square in row r)
template<class W, class F>
void for_each_subset_with_tempo(bitboard const& mask, int k, int t, W&& weight, F&& f)
{
        auto rec = [&](auto&& self, int r, int k_left, int t_left, bitboard const& acc) -> void {
                if (k_left == 0) { if (t_left == 0) f(acc); return; }
                if (r == BOARD_H) return;
                int hi = 0, lo = BOARD_H;                       // the tempi still within reach: rows r.. hold weights in [lo, hi]
                for (int q = r; q < BOARD_H; ++q) { hi = std::max(hi, weight(q)); lo = std::min(lo, weight(q)); }
                if (t_left > k_left * hi || t_left < k_left * lo) return;
                bitboard const a = mask & row_squares(r);
                int const w = weight(r);
                for (int j = 0; j <= std::min(k_left, int(a.size())) && j * w <= t_left; ++j)
                        for_each_subset(a, j, [&](bitboard const& s) { self(self, r + 1, k_left - j, t_left - j * w, acc | s); });
        };
        rec(rec, 0, k, t, bitboard{});
}

void Group::load()
{
        if (loaded()) return;
        men.clear();
        men.reserve(count);
        if (only != ~std::uint64_t(0)) men.push_back(only);
        else {
                Material const c = mi->c;
                auto const wrow = [](int r) { return r; };
                auto const brow = [](int r) { return BOARD_H - 1 - r; };
                for (int tw = 0; tw <= distance; ++tw) {
                        if (white >= 0 && tw != white) continue;
                        for_each_subset_with_tempo(DOM_WM, c.wm, tw, wrow, [&](bitboard const& wm) {
                                for_each_subset_with_tempo(DOM_BM - wm, c.bm, distance - tw, brow, [&](bitboard const& bm) {
                                        men.push_back(mi->men_index(wm, bm));
                                });
                        });
                }
                std::ranges::sort(men);
        }
        if (men.size() != count) { std::cerr << "group: " << men.size() << " configurations generated, " << count << " counted" << std::endl; std::abort(); }
}

// the number of configurations of a material's men per (White tempo, Black tempo), by counting row by row: a row's
// squares are alike (the same tempo for each colour), so i white and j black men go into a row of ROW squares in
// C(n, i) C(n - i, j) ways (n: the row's squares) (no white man on the top row, no black man on the bottom row)
std::map<std::pair<int, int>, std::uint64_t> tempo_counts(Material c)
{
        std::map<std::tuple<int, int, int, int>, std::uint64_t> cur{ { { 0, 0, 0, 0 }, 1 } };     // (white men, black men, tw, tb)
        for (int r = 0; r < BOARD_H; ++r) {
                std::map<std::tuple<int, int, int, int>, std::uint64_t> next;
                for (auto const& [st, n] : cur) {
                        auto const [iw, ib, tw, tb] = st;
                        int const row = int(row_squares(r).size());
                        for (int i = 0; iw + i <= c.wm && i <= row && (r > 0 || i == 0); ++i)
                                for (int j = 0; ib + j <= c.bm && i + j <= row && (r < BOARD_H - 1 || j == 0); ++j)
                                        next[{ iw + i, ib + j, tw + i * r, tb + j * (BOARD_H - 1 - r) }]
                                                += n * CC[std::size_t(row)][std::size_t(i)] * CC[std::size_t(row - i)][std::size_t(j)];
                }
                cur = std::move(next);
        }
        std::map<std::pair<int, int>, std::uint64_t> out;
        for (auto const& [st, n] : cur)
                if (std::get<0>(st) == c.wm && std::get<1>(st) == c.bm) out[{ std::get<2>(st), std::get<3>(st) }] += n;     // every man placed
        return out;
}

// the positions of the group from which a move inside the group leads to q: king unmoves (along a free diagonal with
// long kings, one square with short kings: English checkers, egdb_6x6's LONG_KINGS), and in a wave man unmoves (men
// move one square forward in both games). Captures and promotions leave the group, so their rules never enter here
template<class F>
void for_each_inside_predecessor(Group const& g, Pos const& q, F&& f)
{
        int const mover = 1 - q.stm;
        bitboard const occ = q.occ();
        for (auto const t : q.k[mover])
                for (int d = 0; d < 4; ++d)
                        for (int fr : rays[std::size_t(t)][std::size_t(d)]) {
                                if (occ.contains(std::size_t(fr))) break;
                                Pos pr = q;
                                pr.k[mover].erase(t);
                                pr.k[mover].insert(std::size_t(fr));
                                pr.stm = mover;
                                f(g.index(pr));
                                if (!LONG_KINGS) break;                 // short kings: one square back
                        }
        if (!men_inside()) return;
        for (auto const t : q.m[mover])
                for (int d = mover == 0 ? 2 : 0; d < (mover == 0 ? 4 : 2); ++d) {     // a man came from behind
                        auto const& ray = rays[std::size_t(t)][std::size_t(d)];
                        if (ray.empty() || occ.contains(std::size_t(ray[0]))) continue;
                        Pos pr = q;
                        pr.m[mover].erase(t);
                        pr.m[mover].insert(std::size_t(ray[0]));
                        pr.stm = mover;
                        f(g.index(pr));
                }
}

// ---------------------------------------------------------------- the groups of a material, in dependency order

// in dependency order: a man move lowers its colour's tempo (and the total), so smaller totals come first
std::vector<Group> groups_of(Material c)
{
        MatIndex const& mi = mat_index(c);
        std::vector<Group> out;
        std::uint64_t const K = mi.kings();
        if (K == 0 || mi.men == 0) return out;
        auto const make = [&](int distance, int white, std::uint64_t count) {
                Group g;
                g.mi = &mi; g.distance = distance; g.white = white; g.K = K; g.count = count; g.P = count * K;
                return g;
        };
        if (g_granularity == Granularity::material && g_metric != Metric::dtz) {
                Group g = make(-1, -1, mi.men);
                g.all = true;
                out.push_back(std::move(g));
        } else if (g_granularity == Granularity::configuration) {   // one group per configuration (the 1986 style)
                struct One { int distance, white; std::uint64_t m; };
                std::vector<One> ones;
                for_each_subset(DOM_WM, c.wm, [&](bitboard const& wm) {
                        int const tw = white_tempo(wm);
                        for_each_subset(DOM_BM - wm, c.bm, [&](bitboard const& bm) {
                                ones.push_back({ tw + black_tempo(bm), tw, mi.men_index(wm, bm) });
                        });
                });
                std::ranges::sort(ones, {}, [](One const& o) { return std::tuple(o.distance, o.white, o.m); });
                for (One const& o : ones) { Group g = make(o.distance, o.white, 1); g.only = o.m; out.push_back(std::move(g)); }
        } else {                                                        // tempi: (total, White's); dtz material: total
                bool const by_white = g_granularity == Granularity::tempi;
                std::map<std::pair<int, int>, std::uint64_t> by;        // (total, White's or -1) -> configurations
                for (auto const& [t, n] : tempo_counts(c)) by[{ t.first + t.second, by_white ? t.first : -1 }] += n;
                for (auto const& [k, n] : by) out.push_back(make(k.first, k.second, n));
        }
        return out;
}

// the groups with their lists loaded (for the checks, which visit every position of every group)
std::vector<Group> loaded_groups_of(Material c)
{
        auto groups = groups_of(c);
        for (auto& g : groups) g.load();
        return groups;
}

// the group of a position: its number in groups_of's list
struct GroupFinder {
        std::map<std::tuple<int, int, std::uint64_t>, std::size_t> at;
        explicit GroupFinder(std::vector<Group> const& groups)
        {
                for (std::size_t k = 0; k < groups.size(); ++k) at[{ groups[k].distance, groups[k].white, groups[k].only }] = k;
        }
        std::size_t operator()(std::vector<Group> const& groups, Pos const& p) const
        {
                if (groups.size() == 1 && groups[0].all) return 0;
                Group const& g0 = groups[0];
                int const tw = white_tempo(p.m[0]), t = tw + black_tempo(p.m[1]);
                std::uint64_t const only = g0.only == ~std::uint64_t(0) ? g0.only : g0.mi->men_index(p.m[0], p.m[1]);
                auto const it = at.find({ t, g0.white >= 0 || g0.only != ~std::uint64_t(0) ? tw : -1, only });
                return it == at.end() ? groups.size() : it->second;
        }
};

// the materials that one move can lead into: a promotion, or a capture of at least one piece and not the last (the
// capturing man possibly promoted), by either side (as deps in the Snakefiles)
inline Material mirror_of(Material m) { return { m.bm, m.bk, m.wm, m.wk }; }

std::vector<Material> successor_materials(Material c)
{
        std::vector<Material> out;
        auto const add = [&](Material m) { if (m != c && std::ranges::find(out, m) == out.end()) out.push_back(m); };
        for (int side : { 0, 1 }) {
                Material const s = side == 0 ? c : mirror_of(c);               // the mover as White
                auto const back = [&](Material m) { return side == 0 ? m : mirror_of(m); };
                if (s.wm > 0) add(back({ s.wm - 1, s.wk + 1, s.bm, s.bk }));
                for (int i = 0; i <= s.bm; ++i)
                        for (int j = 0; j <= s.bk; ++j) {
                                if (i + j == 0 || i + j == s.bm + s.bk) continue;
                                add(back({ s.wm, s.wk, s.bm - i, s.bk - j }));
                                if (s.wm > 0) add(back({ s.wm - 1, s.wk + 1, s.bm - i, s.bk - j }));
                        }
        }
        return out;
}

// ---------------------------------------------------------------- depth sinks: where the depths of the material being built go
//
// The rounds only put depths (V: a byte in thompson86, a 16-bit value in tbgen) and never read them back for their own
// group; moves out of a group read the depths of earlier, finished groups of the same material. Two backends:
//   array    the whole material's bytes in RAM, indexed like its files (rung 0)
//   groups   only the current group's bytes in RAM; each finished group is appended to one file per side in group
//            order (strictly sequential writes), mapped once read-only for the lookups: a finished group is an input
//            like a finished material. (A file per group would need two memory maps per group, and the 1986 style has
//            tens of thousands of groups per material, more than the kernel's map limit.)
// Either way the material's files are written by one sequential pass in index order that gathers each slot's byte.
template<class V>
class DepthSink
{
public:
        virtual ~DepthSink() = default;
        virtual bool keeps_groups() const { return false; }         // reads finished groups by their lists (keep them loaded)
        virtual void begin_group(std::size_t k, Group const& g) = 0;
        virtual void put(int side, std::uint64_t local, Pos const& p, V value) = 0;      // local: index in the group
        virtual void end_group() = 0;
        virtual V read(int side, Pos const& p) const = 0;            // a finished group's byte, else 0
        virtual V at(int side, std::uint64_t slot) const = 0;        // the byte of a slot of the material
};

template<class V>
class ArraySink final : public DepthSink<V>
{
        MatIndex const& mi_;
        std::array<std::vector<V>, 2> res_;

public:
        explicit ArraySink(MatIndex const& mi) : mi_(mi) { for (auto& r : res_) r.assign(mi.size, 0); }
        void begin_group(std::size_t, Group const&) override {}
        void put(int side, std::uint64_t, Pos const& p, V value) override { res_[std::size_t(side)][mi_.index(p)] = value; }
        void end_group() override {}
        V read(int side, Pos const& p) const override { return res_[std::size_t(side)][mi_.index(p)]; }
        V at(int side, std::uint64_t slot) const override { return res_[std::size_t(side)][slot]; }
};

template<class V>
class GroupSink final : public DepthSink<V>
{
        Material c_;
        MatIndex const& mi_;
        std::vector<Group> const& groups_;
        GroupFinder find_;                                              // the group of a position
        std::vector<std::uint64_t> gbase_;                              // offset of each group in the group-order files
        std::vector<char> done_;                                        // finished groups
        std::array<std::string, 2> path_;
        std::array<int, 2> fd_{ -1, -1 };
        std::array<V const*, 2> map_{ nullptr, nullptr };
        std::uint64_t total_ = 0;
        std::size_t k_ = 0;
        std::array<std::vector<V>, 2> buf_;                  // the current group, both sides to move

public:
        GroupSink(std::string const& dir, Material c, MatIndex const& mi, std::vector<Group> const& groups)
        :
                c_(c), mi_(mi), groups_(groups), find_(groups), done_(groups.size(), 0)
        {
                for (std::size_t k = 0; k < groups.size(); ++k) {
                        gbase_.push_back(total_);
                        total_ += groups[k].P;
                }
                for (int side : { 0, 1 }) {     // one file per side, in group order, sized up front, mapped once read-only
                        path_[std::size_t(side)] = dir + "/" + c.tree() + "tmp-" + c.stem() + "-" + metric_name() + (side == 0 ? "-wtm" : "-btm");
                        int const fd = ::open(path_[std::size_t(side)].c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0 || ::ftruncate(fd, off_t(total_ * sizeof(V))) != 0) { std::cerr << "cannot create " << path_[std::size_t(side)] << std::endl; std::exit(1); }
                        fd_[std::size_t(side)] = fd;
                        if (total_ == 0) continue;
                        void* m = ::mmap(nullptr, total_ * sizeof(V), PROT_READ, MAP_SHARED, fd, 0);
                        if (m == MAP_FAILED) { std::cerr << "mmap failed " << path_[std::size_t(side)] << std::endl; std::exit(1); }
                        map_[std::size_t(side)] = static_cast<V const*>(m);
                }
        }

        ~GroupSink() override
        {
                for (int side : { 0, 1 }) {
                        if (map_[std::size_t(side)]) ::munmap(const_cast<V*>(map_[std::size_t(side)]), total_ * sizeof(V));
                        if (fd_[std::size_t(side)] >= 0) ::close(fd_[std::size_t(side)]);
                        std::filesystem::remove(path_[std::size_t(side)]);
                }
        }

        bool keeps_groups() const override { return true; }

        void begin_group(std::size_t k, Group const& g) override
        {
                k_ = k;
                for (auto& b : buf_) b.assign(g.P, 0);
        }

        void put(int side, std::uint64_t local, Pos const&, V value) override { buf_[std::size_t(side)][local] = value; }

        void end_group() override                                       // append the group: the writes go in group order
        {
                for (int side : { 0, 1 }) {
                        auto const& b = buf_[std::size_t(side)];
                        auto const* bytes = reinterpret_cast<char const*>(b.data());
                        std::uint64_t const size = b.size() * sizeof(V);
                        for (std::uint64_t done = 0; done < size;) {
                                ssize_t const n = ::pwrite(fd_[std::size_t(side)], bytes + done, size - done, off_t(gbase_[k_] * sizeof(V) + done));
                                if (n <= 0) { std::cerr << "write failed " << path_[std::size_t(side)] << std::endl; std::exit(1); }
                                done += std::uint64_t(n);
                        }
                        std::vector<V>().swap(buf_[std::size_t(side)]);
                }
                done_[k_] = 1;
        }

        V read(int side, Pos const& p) const override
        {
                std::size_t const k = find_(groups_, p);
                if (k >= groups_.size() || !done_[k]) return 0;
                return map_[std::size_t(side)][gbase_[k] + groups_[k].index(p)];
        }

        V at(int side, std::uint64_t slot) const override  // un-rank the slot, then read its group
        {
                Pos p;
                if (!mi_.pos(slot, side, p)) return 0;                  // an illegal slot
                return read(side, p);
        }
};

// ---------------------------------------------------------------- the tempi of a material class
//
// The configurations of a material class's men, by tempi: White's tempo (the rows its men still have to go) and
// Black's. The tempi factorize by colour: a white man move goes from (tw, tb) to (tw - 1, tb), a black one to
// (tw, tb - 1), so the pairs with the same total tw + tb never lead into each other, and building in order of the total
// keeps at most two adjacent totals live (other material classes are reached by captures and promotions).
struct Tempi { int white = 0, black = 0; std::uint64_t configurations = 0, positions = 0; };   // positions per side to move

[[maybe_unused]] std::vector<Tempi> tempi_of(Material c)
{
        std::uint64_t const K = mat_index(c).kings();
        std::map<std::pair<int, int>, Tempi> by;                        // by (total, white)
        if (K != 0)
                for (auto const& [t, n] : tempo_counts(c)) {
                        Tempi& x = by[{ t.first + t.second, t.first }];
                        x.white = t.first;
                        x.black = t.second;
                        x.configurations = n;
                        x.positions = n * K;
                }
        std::vector<Tempi> out;
        for (auto const& [key, t] : by) out.push_back(t);
        return out;
}

// ---------------------------------------------------------------- memory: what a build needs, by rung, and its measured peak
//
// A build holds the program and its tables (FIXED_BYTES), the metadata of its material's groups, what persists for the
// whole material (rung 0: its depth bytes or values), and the largest group's transient state (bit planes, seeds,
// value arrays). Each engine estimates these from its own data structures (memory_need), the Snakefiles turn them into
// job claims (`resources: mem_mb`), and each build logs its peak resident set so the estimates can be checked.
constexpr std::uint64_t FIXED_BYTES = std::uint64_t(16) << 20;  // the binary and its tables: 13.2 MB measured

struct MemoryNeed {
        std::uint64_t slots = 0, positions = 0, groups = 0, slices = 0, max_group = 0;  // max_group: positions per side
        std::uint64_t listed = 0, max_listed = 0;               // configurations in listed groups (not whole materials), the most in one
        void add(Group const& g)
        {
                ++groups;
                positions += 2 * g.P;
                max_group = std::max(max_group, g.P);
                slices += g.configurations();
                if (!g.all) { listed += g.count; max_listed = std::max(max_listed, g.count); }
        }
        // the groups' own memory: about 128 bytes a group, and 8 bytes per configuration of a listed group (its men
        // index); only the group being solved holds its list, except with the group sink, which reads the finished
        // groups through their lists
        std::uint64_t meta(bool group_sink) const { return 128 * groups + 8 * (group_sink ? listed : max_listed); }
        std::uint64_t rung0 = 0, rung1 = 0;                                             // bytes
};

inline std::uint64_t to_mb(std::uint64_t bytes) { return (bytes + (std::uint64_t(1) << 20) - 1) >> 20; }

inline std::uint64_t peak_rss_mb()
{
        rusage r{};
        ::getrusage(RUSAGE_SELF, &r);
        return (std::uint64_t(r.ru_maxrss) + 1023) >> 10;       // ru_maxrss in KB on Linux
}

// one row of a plan: the material, its sizes, and its memory need per rung in MB (tab-separated, for the Snakefiles)
inline void print_plan_row(Material c, MemoryNeed const& m)
{
        std::cout << c.name() << '\t' << m.slots << '\t' << m.positions << '\t' << m.groups << '\t' << m.max_group << '\t'
                  << to_mb(m.rung0) << '\t' << to_mb(m.rung1) << '\n';
}

inline void print_plan_header()
{
        std::cout << "#material\tslots\tpositions\tgroups\tmax_group\trung0_mb\trung1_mb\n";
}

// every material with at most n pieces, both sides with a piece
std::vector<Material> materials_up_to(int n)
{
        std::vector<Material> out;
        for (int wm = 0; wm <= n; ++wm)
                for (int wk = 0; wm + wk <= n; ++wk)
                        for (int bm = 0; wm + wk + bm <= n; ++bm)
                                for (int bk = 0; wm + wk + bm + bk <= n; ++bk)
                                        if (wm + wk > 0 && bm + bk > 0) out.push_back({ wm, wk, bm, bk });
        return out;
}

} // namespace

// ---------------------------------------------------------------- instrumentation: every build is a calibration run
//
// As a profile-guided build, every build measures itself, at the cost of a few popcounts per round: per material its
// positions, groups, levels (the most rounds of a group), the frontier positions summed over the rounds, and the time
// in pass 0, in the rounds and in writing the files. One line per material ("stats", tab-separated) in the build's log;
// src/egdb_calibrate.py collects the lines of a small build, src/egdb_layout.py --calibration extrapolates from them.
struct BuildStats {
        std::uint64_t frontier = 0, levels = 0;
        double pass0 = 0, rounds = 0, write = 0;
};
inline BuildStats g_stats;

inline double seconds_since(std::chrono::steady_clock::time_point t0)
{
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

inline void print_stats(Material c, std::uint64_t positions, std::size_t groups, double total)
{
        std::cout << "stats\t" << metric_name() << '\t' << c.name() << '\t' << c.pieces() << '\t' << positions << '\t' << groups
                  << '\t' << g_stats.levels << '\t' << g_stats.frontier << '\t' << g_stats.pass0 << '\t' << g_stats.rounds << '\t'
                  << g_stats.write << '\t' << total << std::endl;
}

// ---------------------------------------------------------------- bench: what the rules cost per position (synthesis 7.4)
//
// The cost of a build per position depends on the game: the move generator (flying kings, long captures, how many
// moves), the branching factor, and how many moves leave the material (captures and promotions, each a lookup into
// another material's files). Per material, a uniform sample of its positions (both sides to move): the time of one
// move generation, the moves per position, and the share of captures and promotions among them; the last line is the
// average over all materials weighted by their positions. egdb_layout.py turns the weighted time into a cost per
// position with a ratio fixed by the measured 6x6 builds (--calibration).
inline void run_bench(int n, int samples)
{
        Granularity const keep = g_granularity;
        g_granularity = Granularity::material;                  // one group per material: uniform indices
        std::cout << "#material\tpositions\tsamples\tus_per_movegen\tmoves\tcapture_share\tpromotion_share\n";
        double tw = 0, tus = 0, tmoves = 0, tcap = 0, tpro = 0;
        std::uint64_t state = 0x9e3779b97f4a7c15ull;
        for (Material const c : materials_up_to(n)) {
                auto const groups = groups_of(c);
                if (groups.empty()) continue;
                Group const& g = groups[0];
                std::uint64_t const total = 2 * g.P;
                int const k = int(std::min<std::uint64_t>(std::uint64_t(samples), total));
                std::vector<Pos> sample;
                for (int i = 0; i < k; ++i) {
                        state ^= state << 13; state ^= state >> 7; state ^= state << 17;
                        std::uint64_t const r = state % total;
                        sample.push_back(g.pos(r % g.P, int(r / g.P)));
                }
                std::uint64_t moves_n = 0, caps = 0, pros = 0, sink = 0;
                for (Pos const& p : sample)
                        for (auto const& mv : moves(p)) {
                                ++moves_n;
                                caps += is_capture(mv);
                                pros += is_promotion(p, mv);
                        }
                int const reps = std::max(1, 200000 / std::max(k, 1));
                auto const t0 = std::chrono::steady_clock::now();
                for (int r = 0; r < reps; ++r)
                        for (Pos const& p : sample) sink += moves(p).n;
                double const us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count()
                                / double(reps) / double(k);
                if (sink == 42) std::cout << "";                // keep the timed loop
                double const mv = double(moves_n) / k, cs = moves_n ? double(caps) / double(moves_n) : 0,
                             ps = moves_n ? double(pros) / double(moves_n) : 0;
                std::cout << c.name() << '\t' << total << '\t' << k << '\t' << us << '\t' << mv << '\t' << cs << '\t' << ps << '\n';
                double const w = double(total);
                tw += w; tus += w * us; tmoves += w * mv; tcap += w * cs; tpro += w * ps;
        }
        std::cout << "all\t" << std::uint64_t(tw) << "\t-\t" << tus / tw << '\t' << tmoves / tw << '\t' << tcap / tw << '\t'
                  << tpro / tw << '\n';
        g_granularity = keep;
}
