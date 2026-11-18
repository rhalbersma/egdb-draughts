//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Per-material building blocks for 6x6 international draughts, shared by thompson86/thompson86.cpp and
// syzygy/syzygy.cpp: the metrics (dtm, dtc, dtz) and the moves that end their count, the materials (named
// wp-bp-wm-bm) and their index (a product of four colex ranks), bit planes, groups of egdb_6x6 slices solved together
// and their unmoves, and the list of materials. Included after src/egdb_6x6.cpp (with its main renamed), whose
// positions, moves, slices and ranks it uses.

#pragma once

#include <algorithm>
#include <atomic>
#include <execution>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

// ---------------------------------------------------------------- binomials up to C(32, 32)

std::array<std::array<std::uint64_t, 33>, 33> CC{};

void init_cc()
{
        for (std::size_t n = 0; n <= 32; ++n) {
                CC[n][0] = 1;
                for (std::size_t k = 1; k <= n; ++k) CC[n][k] = CC[n - 1][k - 1] + (k <= n - 1 ? CC[n - 1][k] : 0);
        }
}

// ---------------------------------------------------------------- metric and style

enum class Metric { dtm, dtc, dtz };
Metric g_metric = Metric::dtm;
bool g_waves = false;                                   // the 1996 style

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
        return g_waves && g_metric != Metric::dtz;                      // a man move: inside a wave, unless it ends the count
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
};

Material material_of(Pos const& p)
{
        return { int(p.m[0].size()), int(p.k[0].size()), int(p.m[1].size()), int(p.k[1].size()) };
}

constexpr bitboard DOM_WM = FULL - PROMO_WHITE;    // squares a white man may stand on (15)
constexpr bitboard DOM_BM = FULL - PROMO_BLACK;    // squares a black man may stand on (15)

struct MatIndex {
        std::uint64_t swm, sbm, swk, sbk, size;

        explicit MatIndex(Material m)
        :
                swm(CC[15][std::size_t(m.wm)]), sbm(CC[15][std::size_t(m.bm)]), swk(CC[18][std::size_t(m.wk)]),
                sbk(CC[18][std::size_t(m.bk)]), size(swm * sbm * swk * sbk)
        {}

        std::uint64_t index(Pos const& p) const
        {
                return ((rank_in(p.m[0], DOM_WM) * sbm + rank_in(p.m[1], DOM_BM)) * swk + rank_in(p.k[0], FULL)) * sbk
                     + rank_in(p.k[1], FULL);
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
// Every pass is one std::for_each over blocks of 4096 indices (a whole number of 64-bit words, so a block owns the
// words of its positions in every bit plane), sequential or parallel.
bool g_par = false;

template<class F>
void for_blocks(std::uint64_t n, F&& f)
{
        constexpr std::uint64_t B = 4096;
        std::vector<std::uint64_t> starts;
        for (std::uint64_t s = 0; s < n; s += B) starts.push_back(s);
        auto const run = [&](std::uint64_t s) { f(s, std::min(n, s + B)); };
        if (g_par) std::for_each(std::execution::par, starts.begin(), starts.end(), run);
        else std::for_each(std::execution::seq, starts.begin(), starts.end(), run);
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
        template<class F> void for_each_par(F&& f) const        // over the set bits, one std::for_each over words
        {
                for_blocks(w.size(), [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t k = lo; k < hi; ++k)
                                for (std::uint64_t b = w[k]; b; b &= b - 1) f(k * 64 + std::uint64_t(std::countr_zero(b)));
                });
        }
        void mask_par(Plane const& keep, bool complement)       // this &= keep (or ~keep), one std::for_each over words
        {
                for_blocks(w.size(), [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t k = lo; k < hi; ++k) w[k] &= complement ? ~keep.w[k] : keep.w[k];
                });
        }
        void clear() { std::ranges::fill(w, 0); }
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

// ---------------------------------------------------------------- a group of slices, solved together

struct Group {
        std::vector<Meta> units;
        std::vector<std::uint64_t> base;                                // first index of each slice
        std::unordered_map<std::uint64_t, std::size_t> slot;            // slice key -> its number in the group
        std::uint64_t P = 0;                                            // positions per side to move

        void add(Meta const& m) { slot[m.key.pack()] = units.size(); units.push_back(m); base.push_back(P); P += m.P; }

        std::uint64_t index(Pos const& p) const
        {
                std::size_t const u = slot.at(key_of(p).pack());
                return base[u] + index_of(units[u], p);
        }

        Pos pos(std::uint64_t g, int stm) const
        {
                std::size_t const u = std::size_t(std::ranges::upper_bound(base, g) - base.begin()) - 1;
                return pos_of(units[u], g - base[u], stm);
        }
};

// the positions of the group from which a move inside the group leads to q: king unmoves, and in a wave man unmoves
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
                        }
        if (!g_waves || g_metric == Metric::dtz) return;
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

std::vector<Group> groups_of(Material c)
{
        std::vector<std::pair<std::pair<int, std::uint64_t>, Meta>> units;     // (distance, key): the slices of the material
        for (std::uint32_t w = u32(DOM_WM);; w = (w - 1) & u32(DOM_WM)) {
                bitboard const wm = bb_of(w);
                if (int(wm.size()) == c.wm) {
                        std::uint32_t const bdom = u32(DOM_BM - wm);
                        for (std::uint32_t b = bdom;; b = (b - 1) & bdom) {
                                bitboard const bm = bb_of(b);
                                if (int(bm.size()) == c.bm) {
                                        Key const k{ wm, bm, c.wk, c.bk };
                                        Meta const m = meta_of(k);
                                        if (m.P != 0) units.push_back({ { distance(k), k.pack() }, m });
                                }
                                if (b == 0) break;
                        }
                }
                if (w == 0) break;
        }
        std::ranges::sort(units, {}, [](auto const& u) { return u.first; });
        // 1986: one slice per group; 1996: the whole material as one wave (dtz: one wave per distance)
        auto wave = [](std::pair<int, std::uint64_t> const& k) { return g_metric == Metric::dtz ? k.first : 0; };
        std::vector<Group> out;
        for (std::size_t i = 0; i < units.size(); ++i) {
                if (out.empty() || !g_waves || wave(units[i].first) != wave(units[i - 1].first)) out.emplace_back();
                out.back().add(units[i].second);
        }
        return out;
}

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
