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
#include <array>
#include <atomic>
#include <filesystem>
#include <execution>
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
[[maybe_unused]] bool g_waves = false;                                   // thompson86's 1996 pass style (masked late)

// ---------------------------------------------------------------- granularity: how finely a material class is cut into groups
//
//   class   the whole material class, king and man moves inside (dtz: one layer at a time, a man move ending the count)
//   layer   one total tempo (the men's distance to promotion, summed over both colours), king moves inside
//   cell    one (White distance, Black distance) cell of the tempo grid, king moves inside
//   slice   one full configuration of the men, king moves inside
// Below class granularity every man move is an exit, into a group solved earlier (it lowers its colour's distance).
enum class Granularity { klass, layer, cell, slice };
Granularity g_granularity = Granularity::slice;

bool parse_granularity(std::string const& s)
{
        if (s == "class") g_granularity = Granularity::klass;
        else if (s == "layer") g_granularity = Granularity::layer;
        else if (s == "cell") g_granularity = Granularity::cell;
        else if (s == "slice") g_granularity = Granularity::slice;
        else return false;
        return true;
}

[[maybe_unused]] char const* granularity_name()
{
        switch (g_granularity) {
        case Granularity::klass: return "class";
        case Granularity::layer: return "layer";
        case Granularity::cell: return "cell";
        case Granularity::slice: return "slice";
        }
        return "?";
}

// man moves stay inside a group only at class granularity, and not in dtz (a man move ends the count there)
bool men_inside() { return g_granularity == Granularity::klass && g_metric != Metric::dtz; }

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
        return men_inside();                                            // a man move: inside only at class granularity
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

std::vector<Group> groups_of(Material c)
{
        struct SliceUnit { int distance, white; std::uint64_t key; Meta meta; };      // a slice, with its distances
        std::vector<SliceUnit> units;
        for (std::uint32_t w = u32(DOM_WM);; w = (w - 1) & u32(DOM_WM)) {
                bitboard const wm = bb_of(w);
                if (int(wm.size()) == c.wm) {
                        std::uint32_t const bdom = u32(DOM_BM - wm);
                        for (std::uint32_t b = bdom;; b = (b - 1) & bdom) {
                                bitboard const bm = bb_of(b);
                                if (int(bm.size()) == c.bm) {
                                        Key const k{ wm, bm, c.wk, c.bk };
                                        Meta const m = meta_of(k);
                                        int white = 0;
                                        for (auto const sq : wm) white += int(sq) / 3;
                                        if (m.P != 0) units.push_back({ distance(k), white, k.pack(), m });
                                }
                                if (b == 0) break;
                        }
                }
                if (w == 0) break;
        }
        // in dependency order: a man move lowers the total distance (and its colour's), so smaller totals come first
        std::ranges::sort(units, {}, [](SliceUnit const& u) { return std::tuple(u.distance, u.white, u.key); });
        auto const group_key = [](SliceUnit const& u) -> std::uint64_t {    // a new group whenever this changes
                switch (g_granularity) {
                case Granularity::klass: return g_metric == Metric::dtz ? std::uint64_t(u.distance) : 0;
                case Granularity::layer: return std::uint64_t(u.distance);
                case Granularity::cell: return std::uint64_t(u.distance) << 32 | std::uint64_t(u.white);
                case Granularity::slice: return u.key;
                }
                return u.key;
        };
        std::vector<Group> out;
        for (std::size_t i = 0; i < units.size(); ++i) {
                if (out.empty() || group_key(units[i]) != group_key(units[i - 1])) out.emplace_back();
                out.back().add(units[i].meta);
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

// ---------------------------------------------------------------- depth sinks: where the depths of the material being built go
//
// The rounds only put depths (V: a byte in thompson86, a 16-bit value in syzygy) and never read them back for their own
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
        struct Slice { std::uint64_t offset; Meta const* meta; std::size_t group; };

        Material c_;
        MatIndex const& mi_;
        std::vector<Group> const& groups_;
        std::vector<std::uint64_t> gbase_;                              // offset of each group in the group-order files
        std::unordered_map<std::uint64_t, Slice> slice_;                // slice key -> where its positions are
        std::vector<char> done_;                                        // finished groups
        std::array<std::string, 2> path_;
        std::array<int, 2> fd_{ -1, -1 };
        std::array<V const*, 2> map_{ nullptr, nullptr };
        std::uint64_t total_ = 0;
        std::size_t k_ = 0;
        std::array<std::vector<V>, 2> buf_;                  // the current group, both sides to move

        Slice const* find(Pos const& p) const           // with the last slice cached per thread
        {
                thread_local GroupSink const* owner = nullptr;
                thread_local std::uint64_t last_key = 0;
                thread_local Slice const* last = nullptr;
                std::uint64_t const key = key_of(p).pack();
                if (owner != this || key != last_key) {
                        auto const it = slice_.find(key);
                        last = it == slice_.end() ? nullptr : &it->second;
                        owner = this;
                        last_key = key;
                }
                return last;
        }

public:
        GroupSink(std::string const& dir, Material c, MatIndex const& mi, std::vector<Group> const& groups)
        :
                c_(c), mi_(mi), groups_(groups), done_(groups.size(), 0)
        {
                for (std::size_t k = 0; k < groups.size(); ++k) {
                        gbase_.push_back(total_);
                        for (std::size_t u = 0; u < groups[k].units.size(); ++u)
                                slice_[groups[k].units[u].key.pack()] = { total_ + groups[k].base[u], &groups[k].units[u], k };
                        total_ += groups[k].P;
                }
                for (int side : { 0, 1 }) {     // one file per side, in group order, sized up front, mapped once read-only
                        path_[std::size_t(side)] = dir + "/tmp-6x6-" + c.name() + "." + metric_name() + (side == 0 ? ".wtm" : ".btm");
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
                Slice const* s = find(p);
                if (!s || !done_[s->group]) return 0;
                return map_[std::size_t(side)][s->offset + index_of(*s->meta, p)];
        }

        V at(int side, std::uint64_t slot) const override  // un-rank the slot, then read its group
        {
                std::uint64_t const rbk = slot % mi_.sbk, t1 = slot / mi_.sbk, rwk = t1 % mi_.swk, t2 = t1 / mi_.swk;
                std::uint64_t const rbm = t2 % mi_.sbm, rwm = t2 / mi_.sbm;
                Pos p;
                p.m[0] = unrank_in(rwm, c_.wm, DOM_WM);
                p.m[1] = unrank_in(rbm, c_.bm, DOM_BM);
                p.k[0] = unrank_in(rwk, c_.wk, FULL);
                p.k[1] = unrank_in(rbk, c_.bk, FULL);
                p.stm = side;
                bitboard const pieces[] = { p.m[0], p.m[1], p.k[0], p.k[1] };
                for (int i = 0; i < 4; ++i)
                        for (int j = i + 1; j < 4; ++j)
                                if (!(pieces[i] & pieces[j]).empty()) return 0;         // an illegal slot
                return read(side, p);
        }
};

// ---------------------------------------------------------------- the layers of a material class's configuration DAG
//
// The full-configuration slices of a material class, by distance (the rows its men still have to go, summed over both
// colours). Every man move lowers the distance by exactly one, so layer d leads only into layer d - 1 of the same class
// (and into other classes by captures and promotions): built layer by layer, only two adjacent layers are ever live.
struct Layer { int distance = 0; std::uint64_t slices = 0, positions = 0; };   // positions per side to move

[[maybe_unused]] std::vector<Layer> layers_of(Material c)
{
        std::map<int, Layer> by;
        for (std::uint32_t w = u32(DOM_WM);; w = (w - 1) & u32(DOM_WM)) {
                bitboard const wm = bb_of(w);
                if (int(wm.size()) == c.wm) {
                        std::uint32_t const bdom = u32(DOM_BM - wm);
                        for (std::uint32_t b = bdom;; b = (b - 1) & bdom) {
                                bitboard const bm = bb_of(b);
                                if (int(bm.size()) == c.bm) {
                                        Key const k{ wm, bm, c.wk, c.bk };
                                        Meta const m = meta_of(k);
                                        if (m.P != 0) {
                                                Layer& l = by[distance(k)];
                                                l.distance = distance(k);
                                                ++l.slices;
                                                l.positions += m.P;
                                        }
                                }
                                if (b == 0) break;
                        }
                }
                if (w == 0) break;
        }
        std::vector<Layer> out;
        for (auto const& [d, l] : by) out.push_back(l);
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
