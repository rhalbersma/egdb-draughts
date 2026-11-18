//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Thompson style builds (1986 and 1996) for 6x6 international draughts, for three metrics. Per material two files,
// one byte per position:
//   .w   White to move: the number of plies to the goal of a White win, 0 for "not a win" (draw, loss, illegal slot)
//   .l   Black to move: 1 + the number of plies to the goal of Black's loss, 0 for "not lost" (a loss can take 0 plies:
//        Black has no move), as Thompson 1996 kept the Black-to-move loss distances (RES)
// The goal:
//   dtm   the goal is the end of the game (Black has no move)
//   dtc   the end, or a capture or promotion (a conversion: another material) that keeps the win
//   dtz   the end, or a capture or man move (a zeroing move of the 25-move rule) that keeps the win
// A goal move by either side ends the count: a White win can end with Black's forced capture (an even number of plies).
//
// Materials. A material is named wp-bp-wm-bm (white pieces, black pieces, white men, black men, two digits each, as
// in egdb_6x6's material mode); every material with a piece on each side is built once, into files of its own,
// 6x6-wp-bp-wm-bm.<metric>.w and .l. Its index: per colour and type the colex rank over the squares where such a
// piece may stand (15 for men, 18 for kings),
//   index = ((rank_wm * C(15, bm) + rank_bm) * C(18, wp - wm) + rank_wk) * C(18, bp - bm) + rank_bk
// Slots where pieces of different colours or types share a square are never written (0). A move into another
// material (a capture, a promotion) is a lookup in that material's files at its own index: after a White move in .l,
// after a Black move in .w; the smaller
// materials are not copied or recomputed, the address of a position follows from its own pieces. The materials form
// a DAG through captures and promotions, which Snakemake builds in order (thompson86/Snakefile).
//
// Groups. The positions of a material are solved group by group, each group after every group its moves can leave to:
//   1986 style   a group is one slice of egdb_6x6 (a placement of the men and the numbers of kings), in order of
//                distance of the men to promotion; only king moves without capture stay inside
//   1996 style   a group is a wave: every slice of the material (for dtz: every slice with the same distance, since
//                a man move ends the count there); king moves and man moves without capture or
//                promotion stay inside; the passes mask late, as in Thompson's I/O-enhanced algorithm (Figure 3)
// In both, every move that leaves a group (captures, promotions, and in the 1986 style man moves) is looked up
// forward in pass 0, in a finished group or material: no uncaptures anywhere. A position without moves inside the group
// is decided there (captures are compulsory); the others keep their exit results as seeds at their depth.
// Rounds, level d = 1, 2, ... in plies, on bit planes (Thompson's P2 / P3 / P4):
//   W_d = unmoves of B_{d-1} into open White-to-move positions, plus exits that win in d
//   B_d = unmoves of W_{d-1} into open Black-to-move positions whose exits all lose by d, verified: every move inside
//         the group leads into W (levels < d); plus positions decided in pass 0 at d
// W_d goes into .w, B_d into .l: both are one-sided (White's wins, Black's losses), and every lookup is one read.
//
// Memory: the material's two result arrays (each written to disk once, sequentially, at the end) and bit planes per
// group. Disk: other materials are read through read-only memory maps (random reads); the only writes are sequential.
//
// build: g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include
//        -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o thompson86
// usage: thompson86 build <dir> wp bp wm bm <metric> <style>   build one material (its captures and promotions must exist)
//        thompson86 checkall <dir> n <metric> [threads]         every material of at most n pieces against egdb_6x6 (built in
//                                                               memory): dtm exactly, dtc / dtz their wins and losses,
//                                                               and every value of both files recomputed from its
//                                                               successors
//        thompson86 order <dir_dtz> <dir_dtc> <dir_dtm> n       dtz <= dtc <= dtm for every White-to-move win and every
//                                                               Black-to-move loss
//        thompson86 size wp bp wm bm                            the number of slots of a material
// metric: dtm, dtc, dtz; style: 86, 96

#define main egdb_6x6_main
#include "../src/egdb_6x6.cpp"
#undef main

#include <filesystem>
#include <fstream>
#include <map>

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

// side 0: .w (White to move, wins), side 1: .l (Black to move, losses)
std::string mat_path(std::string const& dir, Material m, int side)
{
        return dir + "/6x6-" + m.name() + "." + metric_name() + (side == 0 ? ".w" : ".l");
}

// ---------------------------------------------------------------- results: the material being built, and finished ones

std::string g_dir;
Material g_current{ -1, -1, -1, -1 };
std::array<std::vector<std::uint8_t>, 2> g_res;     // the material being built, .w and .l (written to disk at the end)
std::map<std::pair<Material, int>, std::uint8_t const*> g_files;    // finished materials, memory-mapped read-only
std::map<Material, MatIndex> g_index;

MatIndex const& mat_index(Material m)
{
        auto it = g_index.find(m);
        if (it == g_index.end()) it = g_index.emplace(m, MatIndex(m)).first;
        return it->second;
}

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

std::uint8_t const* mat_data(Material m, int side)
{
        if (m == g_current) return g_res[std::size_t(side)].data();
        auto it = g_files.find({ m, side });
        if (it != g_files.end()) return it->second;
        return g_files[{ m, side }] = map_file(mat_path(g_dir, m, side), mat_index(m).size);
}

// White to move: plies to the goal of a White win, 0 if not a win
int lookup_wtm(Pos const& p)
{
        if ((p.m[0] | p.k[0]).empty() || (p.m[1] | p.k[1]).empty()) return 0;   // a side without pieces: not a White win to look up
        Material const m = material_of(p);
        return mat_data(m, 0)[mat_index(m).index(p)];
}

// Black to move: plies to the goal of Black's loss, -1 if Black is not lost
int lookup_btm_lost(Pos const& p)
{
        if ((p.m[1] | p.k[1]).empty()) return 0;          // Black has no pieces: lost
        if ((p.m[0] | p.k[0]).empty()) return -1;         // White has no pieces: Black has won
        Material const m = material_of(p);
        return int(mat_data(m, 1)[mat_index(m).index(p)]) - 1;
}

// the same, recomputed by one ply over White-to-move results
int btm_lost_from_successors(Pos const& p)
{
        if ((p.m[1] | p.k[1]).empty()) return 0;          // Black has no pieces: lost
        int lost = 0;
        for (auto const& mv : moves(p)) {                   // no move: lost now (0)
                int const v = lookup_wtm(mv.next);
                if (v == 0) return -1;
                lost = std::max(lost, goal_move(p, mv) ? 1 : 1 + v);
        }
        return lost;
}

// White to move, recomputed from the successors: the fastest goal over the moves, 0 if none wins
int wtm_from_successors(Pos const& p)
{
        int best = INF;
        for (auto const& mv : moves(p)) {
                int const j = lookup_btm_lost(mv.next);
                if (j >= 0) best = std::min(best, goal_move(p, mv) ? 1 : 1 + j);
        }
        return best < INF ? best : 0;
}

// ---------------------------------------------------------------- bit planes

struct Plane {
        std::vector<std::uint64_t> w;
        explicit Plane(std::uint64_t n = 0) : w((n + 63) / 64, 0) {}
        bool test(std::uint64_t i) const { return w[i >> 6] >> (i & 63) & 1; }
        void set(std::uint64_t i) { w[i >> 6] |= std::uint64_t(1) << (i & 63); }
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

struct Seed { int level; std::uint32_t idx; };

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

void solve_group(MatIndex const& mi, Group const& g)
{
        std::uint64_t const P = g.P;
        Plane open_w(P), open_b(P), lossable(P), exits_done(P), won(P), lost(P);
        std::vector<Seed> win_seeds, loss_decided, exit_seeds;

        // pass 0: forward over every position, both sides to move; exits are looked up in finished groups and materials
        for (std::uint64_t i = 0; i < P; ++i) {
                Pos const p = g.pos(i, 0);
                bool in = false;
                int best = INF;
                for (auto const& mv : moves(p)) {
                        if (inside(p, mv)) { in = true; continue; }
                        int const j = lookup_btm_lost(mv.next);
                        if (j >= 0) best = std::min(best, goal_move(p, mv) ? 1 : 1 + j);
                }
                if (in) open_w.set(i);
                if (best < INF) win_seeds.push_back({ best, std::uint32_t(i) });
        }
        for (std::uint64_t i = 0; i < P; ++i) {
                Pos const p = g.pos(i, 1);
                auto const ml = moves(p);
                if (ml.empty()) { loss_decided.push_back({ 0, std::uint32_t(i) }); continue; }
                bool in = false, all_lose = true;
                int slowest = 0;
                for (auto const& mv : ml) {
                        if (inside(p, mv)) { in = true; continue; }
                        int const v = lookup_wtm(mv.next);
                        if (v == 0) all_lose = false;
                        else slowest = std::max(slowest, goal_move(p, mv) ? 1 : 1 + v);
                }
                if (!in) {                                              // every move leaves the group: decided
                        if (all_lose) loss_decided.push_back({ slowest, std::uint32_t(i) });
                        continue;
                }
                open_b.set(i);
                if (!all_lose) continue;
                lossable.set(i);
                if (slowest == 0) exits_done.set(i);                    // no exits
                else exit_seeds.push_back({ slowest, std::uint32_t(i) });
        }
        auto by_level = [](Seed const& a, Seed const& b) { return a.level < b.level; };
        std::ranges::sort(win_seeds, by_level);
        std::ranges::sort(loss_decided, by_level);
        std::ranges::sort(exit_seeds, by_level);
        int max_seed = 0;
        for (auto const* v : { &win_seeds, &loss_decided, &exit_seeds })
                if (!v->empty()) max_seed = std::max(max_seed, v->back().level);

        // rounds, level d in plies: W_d from B_{d-1}, B_d from W_{d-1}
        std::size_t ws = 0, ld = 0, xs = 0;
        Plane w_last(P), b_last(P), w_new(P), b_new(P), cand(P);
        for (; ld < loss_decided.size() && loss_decided[ld].level == 0; ++ld) {
                std::uint32_t const q = loss_decided[ld].idx;
                b_last.set(q);
                lost.set(q);
                g_res[1][mi.index(g.pos(q, 1))] = 1;                    // lost in 0 plies: no move
        }
        for (int d = 1;; ++d) {
                // P2: White wins in d
                w_new.clear();
                b_last.for_each([&](std::uint64_t q) {
                        for_each_inside_predecessor(g, g.pos(q, 1), [&](std::uint64_t id) {
                                if (!g_waves) { if (open_w.test(id) && !won.test(id)) w_new.set(id); }
                                else w_new.set(id);                                     // 1996: mask later
                        });
                });
                if (g_waves) w_new.mask(open_w, false);                         // unmoves only reach open positions
                for (; ws < win_seeds.size() && win_seeds[ws].level == d; ++ws) w_new.set(win_seeds[ws].idx);
                w_new.mask(won, true);

                // P3: candidates for Black losses in d
                cand.clear();
                for (; xs < exit_seeds.size() && exit_seeds[xs].level == d; ++xs) {
                        exits_done.set(exit_seeds[xs].idx);
                        cand.set(exit_seeds[xs].idx);
                }
                w_last.for_each([&](std::uint64_t q) {
                        for_each_inside_predecessor(g, g.pos(q, 0), [&](std::uint64_t id) {
                                if (!g_waves) { if (open_b.test(id) && lossable.test(id) && !lost.test(id)) cand.set(id); }
                                else cand.set(id);                                      // 1996: mask later
                        });
                });
                if (g_waves) { cand.mask(open_b, false); cand.mask(lossable, false); cand.mask(lost, true); }

                // P4: lost in d iff the exits are lost by d and every move inside leads into W (levels below d)
                b_new.clear();
                cand.for_each([&](std::uint64_t q) {
                        if (!exits_done.test(q) || lost.test(q)) return;
                        Pos const p = g.pos(q, 1);
                        for (auto const& mv : moves(p))
                                if (inside(p, mv) && !won.test(g.index(mv.next))) return;
                        b_new.set(q);
                });
                for (; ld < loss_decided.size() && loss_decided[ld].level == d; ++ld) b_new.set(loss_decided[ld].idx);

                w_new.for_each([&](std::uint64_t q) {
                        won.set(q);
                        g_res[0][mi.index(g.pos(q, 0))] = std::uint8_t(d);
                });
                b_new.for_each([&](std::uint64_t q) {
                        lost.set(q);
                        g_res[1][mi.index(g.pos(q, 1))] = std::uint8_t(d + 1);
                });
                std::swap(w_last, w_new);
                std::swap(b_last, b_new);
                if (d >= 254) { std::cerr << "depth beyond one byte" << std::endl; std::exit(1); }
                if (w_last.empty() && b_last.empty() && d >= max_seed) break;
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

bool build_material(std::string const& dir, Material c)
{
        g_dir = dir;
        g_current = c;
        MatIndex const& mi = mat_index(c);
        for (auto& r : g_res) r.assign(mi.size, 0);
        auto const groups = groups_of(c);
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (auto const& g : groups) { solve_group(mi, g); positions += 2 * g.P; }
        for (int side : { 0, 1 }) {
                std::string const path = mat_path(dir, c, side), tmp = path + ".tmp";
                {
                        auto const& r = g_res[std::size_t(side)];
                        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
                        f.write(reinterpret_cast<char const*>(r.data()), std::streamsize(r.size()));   // one sequential write
                        if (!f) { std::cerr << "write failed " << tmp << std::endl; return false; }
                }
                std::filesystem::rename(tmp, path);
        }
        std::uint64_t wins = 0, losses = 0;
        int longest = 0, longest_loss = 0;
        for (auto v : g_res[0]) { wins += v != 0; longest = std::max(longest, int(v)); }
        for (auto v : g_res[1]) { losses += v != 0; longest_loss = std::max(longest_loss, int(v) - 1); }
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "material " << c.name() << " " << metric_name() << (g_waves ? " 1996" : " 1986") << ": " << groups.size()
                  << " groups, " << positions << " positions, " << mi.size << " slots, " << wins << " White wins, longest "
                  << longest << " plies, " << losses << " Black losses, longest " << longest_loss << " plies, " << sec << " s"
                  << std::endl;
        return true;
}

// ---------------------------------------------------------------- checks

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

// dtm: every value equals egdb_6x6's DTM, both sides to move; dtc, dtz: the wins and losses equal egdb_6x6's, and every
// White-to-move value equals the value recomputed from its successors
bool check_all(std::string const& dir, int n, int threads)
{
        g_dir = dir;
        build(n, threads, n);                                   // egdb_6x6, once for every material
        bool ok = true;
        std::uint64_t total = 0;
        for (Material const c : materials_up_to(n)) {
                MatIndex const& mi = mat_index(c);
                std::uint8_t const* res = mat_data(c, 0);
                std::uint64_t checked = 0, bad_w = 0, bad_b = 0, bad_s = 0;
                for (auto const& g : groups_of(c))
                        for (std::uint64_t i = 0; i < g.P; ++i) {
                                Pos const pw = g.pos(i, 0), pb = g.pos(i, 1);
                                std::uint16_t const v = lookup(pw), u = lookup(pb);
                                bool const win = v != DRAW && (v & 1), loss = u != DRAW && !(u & 1);
                                int const got = res[mi.index(pw)], got_b = lookup_btm_lost(pb);
                                bool const w_ok = g_metric == Metric::dtm ? got == (win ? int(v) : 0) : (got != 0) == win;
                                bool const b_ok = g_metric == Metric::dtm ? got_b == (loss ? int(u) : -1) : (got_b >= 0) == loss;
                                if (!w_ok && bad_w++ < 3) std::cerr << "W " << fen_of(pw) << " got " << got << " oracle " << show(v) << std::endl;
                                if (!b_ok && bad_b++ < 3) std::cerr << "B " << fen_of(pb) << " got " << got_b << " oracle " << show(u) << std::endl;
                                int const sv = wtm_from_successors(pw), sb = btm_lost_from_successors(pb);
                                if (sv != got && bad_s++ < 3) std::cerr << "S " << fen_of(pw) << " stored " << got << " from successors " << sv << std::endl;
                                if (sb != got_b && bad_s++ < 3) std::cerr << "S " << fen_of(pb) << " stored " << got_b << " from successors " << sb << std::endl;
                                checked += 2;
                        }
                total += checked;
                std::cout << "check " << c.name() << " " << metric_name() << ": " << checked << " positions, " << bad_w << " / "
                          << bad_b << " differences from egdb_6x6 (White / Black to move), " << bad_s
                          << " differences from the successors" << std::endl;
                ok = ok && bad_w == 0 && bad_b == 0 && bad_s == 0;
        }
        std::cout << total << " positions; " << (ok ? "all materials check out" : "DIFFERENCES") << std::endl;
        return ok;
}

// dtz <= dtc <= dtm for every White-to-move win and every Black-to-move loss, and the same wins and losses in all three
bool check_order(std::string const& dz, std::string const& dc, std::string const& dm, int n)
{
        bool ok = true;
        for (int side : { 0, 1 }) {
                std::uint64_t count = 0, bad = 0, equal_zc = 0, equal_cm = 0;
                for (Material const c : materials_up_to(n)) {
                        std::size_t const size = mat_index(c).size;
                        g_metric = Metric::dtz; auto const z = map_file(mat_path(dz, c, side), size);
                        g_metric = Metric::dtc; auto const x = map_file(mat_path(dc, c, side), size);
                        g_metric = Metric::dtm; auto const m = map_file(mat_path(dm, c, side), size);
                        for (std::size_t i = 0; i < size; ++i) {
                                if ((z[i] == 0) != (m[i] == 0) || (x[i] == 0) != (m[i] == 0)) { ++bad; continue; }
                                if (m[i] == 0) continue;
                                ++count;
                                if (!(z[i] <= x[i] && x[i] <= m[i])) ++bad;
                                equal_zc += z[i] == x[i];
                                equal_cm += x[i] == m[i];
                        }
                }
                std::cout << "order: " << count << (side == 0 ? " White-to-move wins, " : " Black-to-move losses, ") << bad
                          << " violations of dtz <= dtc <= dtm (or of the same set); dtz = dtc in " << equal_zc << ", dtc = dtm in "
                          << equal_cm << std::endl;
                ok = ok && bad == 0;
        }
        return ok;
}

} // namespace

int main(int argc, char* argv[])
{
        init_tables();
        init_cc();
        std::vector<std::string> const a(argv + 1, argv + argc);
        auto mat_at = [&](std::size_t i) {                             // wp bp wm bm
                int const wp = std::stoi(a[i]), bp = std::stoi(a[i + 1]), wm = std::stoi(a[i + 2]), bm = std::stoi(a[i + 3]);
                if (wm < 0 || bm < 0 || wm > wp || bm > bp) { std::cerr << "not a material: wp bp wm bm" << std::endl; std::exit(2); }
                return Material{ wm, wp - wm, bm, bp - bm };
        };
        if (a.size() == 8 && a[0] == "build" && parse_metric(a[6]) && (a[7] == "86" || a[7] == "96")) {
                g_waves = a[7] == "96";
                return build_material(a[1], mat_at(2)) ? 0 : 1;
        }
        if ((a.size() == 4 || a.size() == 5) && a[0] == "checkall" && parse_metric(a[3]))
                return check_all(a[1], std::stoi(a[2]), a.size() == 5 ? std::stoi(a[4]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "order") return check_order(a[1], a[2], a[3], std::stoi(a[4])) ? 0 : 1;
        if (a.size() == 5 && a[0] == "size") { std::cout << MatIndex(mat_at(1)).size << std::endl; return 0; }
        std::cerr << "usage: thompson86 build <dir> wp bp wm bm dtm|dtc|dtz 86|96 | checkall <dir> n dtm|dtc|dtz [threads]"
                     " | order <dir_dtz> <dir_dtc> <dir_dtm> n | size wp bp wm bm" << std::endl;
        return 2;
}
