//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Syzygy style builds (after R. de Man's rtbgen) for 6x6 international draughts, two-sided, for three metrics, with
// the per-material index of thompson86 (src/materials_6x6.hpp). The value of a position, for the side to move:
//   0          draw (also an illegal slot)
//   2n         win in n plies (n >= 1)
//   2n + 1     loss in n plies (n >= 0; 0: no move)
// 16 bits during the build, one byte stored. The metric decides which moves end the count (as in thompson86):
//   dtm   none: the count runs to the end of the game
//   dtc   captures and promotions (a conversion into another material)
//   dtz   captures and man moves (the zeroing moves of the 25-move rule)
// After a goal move only the outcome of the position it leads to counts (win in 1 ply, or loss in 1 ply).
//
// Materials. Two-sided: both sides to move, wins and losses, so only one of each pair of a material and its colour
// mirror is built (the same 82 materials up to 6 pieces as strongly_solving_6x6/materials); a position in the other
// one is looked up as its mirror image. Per material three files, named wp-bp-wm-bm as in egdb_6x6:
//   6x6-wp-bp-wm-bm.<metric>.wtm   one byte per slot, White to move
//   6x6-wp-bp-wm-bm.<metric>.btm   one byte per slot, Black to move
//   6x6-wp-bp-wm-bm.<metric>.wdl   two bits per slot (0 draw, 1 loss, 2 win), first White to move, then Black to move
// The WDL file is a by-product: the sign of every value. It is all that dtc and dtz need from the materials that a
// move leads into (every move out of a material is a goal move there), so their builds read only the .wdl files of
// their captures and promotions; dtm needs the depths and reads .wtm and .btm.
//
// Groups. Captures and promotions (and in dtz man moves, which end the count) are looked up forward in pass 0, in
// finished files: no uncaptures. A dtm or dtc material is one group, with king moves and man moves inside; a dtz
// material is solved one distance at a time (man moves lead to a smaller distance), with king moves inside.
//
// Passes, in the manner of rtbgen: one byte array per group, both sides to move, scanned in full in every pass.
//   pass 0      every position looks up its moves out of the group: without a move inside it is decided at once
//               (captures are compulsory); otherwise its fastest win and its slowest loss through an exit are kept
//               (no loss if an exit draws or wins)
//   round n     losses in n: every position won in n - 1 unmoves to its predecessors, and each undecided one whose
//               exits lose by n is checked forward (rtbgen's check_loss): lost iff every move inside leads to a win
//               in fewer than n plies (pass 0 decides positions of any depth: they are a frontier at their level);
//               also every undecided position whose exits lose in n. Then wins in n: every position lost in n - 1
//               unmoves to its undecided predecessors, and every undecided position whose exit wins in n
// Within a round the losses read only wins of earlier rounds, and the wins only losses of earlier rounds, so each
// half can run in parallel (std::for_each, seq or par, over blocks of positions; every writer writes the same value).
// Rule 6: the group's arrays are in RAM; finished materials are read through read-only memory maps; each file is
// written once, sequentially.
//
// build: g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include
//        -I<xstd-misc>/include syzygy/syzygy.cpp -lzstd -ltbb -o syzygy
// usage: syzygy build <dir> wp bp wm bm <metric> [seq|par]   build one material (its captures and promotions must exist)
//        syzygy checkall <dir> n <metric> [threads]          every material of at most n pieces against egdb_6x6
//                                                            (built in memory): dtm exactly, dtc / dtz their wins and
//                                                            losses; every value against its successors; the .wdl file
//                                                            against the values
//        syzygy cmp86 <dir> <dir_thompson86> n <metric>      the same wins (.w) and losses (.l) as thompson86, for every
//                                                            material of at most n pieces, both colours
//        syzygy order <dir_dtz> <dir_dtc> <dir_dtm> n         dtz <= dtc <= dtm for every win and loss, the same WDL
// metric: dtm, dtc, dtz

#define main egdb_6x6_main
#include "../src/egdb_6x6.cpp"
#undef main

#include "../src/materials_6x6.hpp"

#include <atomic>
#include <execution>
#include <filesystem>
#include <fstream>
#include <tuple>

namespace {

// ---------------------------------------------------------------- values

constexpr std::uint16_t win(int n) { return std::uint16_t(2 * n); }
constexpr std::uint16_t loss(int n) { return std::uint16_t(2 * n + 1); }
constexpr bool is_win(std::uint16_t v) { return v != 0 && (v & 1) == 0; }
constexpr bool is_loss(std::uint16_t v) { return (v & 1) != 0; }
constexpr int plies(std::uint16_t v) { return v >> 1; }

enum : std::uint8_t { WDL_DRAW = 0, WDL_LOSS = 1, WDL_WIN = 2 };
constexpr std::uint8_t wdl_of(std::uint16_t v) { return v == 0 ? WDL_DRAW : is_loss(v) ? WDL_LOSS : WDL_WIN; }

constexpr std::uint16_t NONE = 0xFFFF;                  // eloss: an exit draws or wins

// the better of two values for the side to move: the fastest win, else a draw, else the slowest loss
std::uint16_t better(std::uint16_t a, std::uint16_t b)
{
        auto const key = [](std::uint16_t v) { return is_win(v) ? 1000 - plies(v) : v == 0 ? 0 : -1000 + plies(v); };
        return key(a) >= key(b) ? a : b;
}

// ---------------------------------------------------------------- seq or par, over blocks of positions

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

std::uint16_t load(std::uint16_t& x) { return std::atomic_ref<std::uint16_t>(x).load(std::memory_order_relaxed); }
void store(std::uint16_t& x, std::uint16_t v) { std::atomic_ref<std::uint16_t>(x).store(v, std::memory_order_relaxed); }

// ---------------------------------------------------------------- materials and files

// one of each pair of a material and its colour mirror, as in egdb_6x6 and strongly_solving_6x6/materials
bool stored(Material m) { return !(std::tuple(m.bm, m.wm, m.bk, m.wk) < std::tuple(m.wm, m.bm, m.wk, m.bk)); }

// kind 0: .wtm, 1: .btm (one byte per slot), 2: .wdl (two bits per slot and side to move)
std::string mat_path(std::string const& dir, Material m, int kind)
{
        return dir + "/6x6-" + m.name() + "." + metric_name() + (kind == 0 ? ".wtm" : kind == 1 ? ".btm" : ".wdl");
}

std::uint64_t file_size(Material m, int kind) { return kind < 2 ? mat_index(m).size : (2 * mat_index(m).size + 3) / 4; }

std::string g_dir;
Material g_current{ -1, -1, -1, -1 };
std::array<std::vector<std::uint16_t>, 2> g_val;        // the material being built, 16 bits per slot and side to move
std::map<std::pair<Material, int>, std::uint8_t const*> g_files;

std::uint8_t const* mat_file(Material m, int kind)
{
        auto it = g_files.find({ m, kind });
        if (it != g_files.end()) return it->second;
        return g_files[{ m, kind }] = map_file(mat_path(g_dir, m, kind), file_size(m, kind));
}

// a position in a stored material: itself or its mirror image (the value for the side to move is the same)
Pos stored_pos(Pos const& p) { return stored(material_of(p)) ? p : mirror(p); }

// the value of a position for the side to move
std::uint16_t probe(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return loss(0);                    // no pieces, no move
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        return m == g_current ? g_val[std::size_t(p.stm)][i] : mat_file(m, p.stm)[i];
}

// win, draw or loss of a position for the side to move
std::uint8_t probe_wdl(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return WDL_LOSS;
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        if (m == g_current) return wdl_of(g_val[std::size_t(p.stm)][i]);
        std::uint64_t const bit = 2 * (std::uint64_t(p.stm) * mat_index(m).size + i);
        return std::uint8_t(mat_file(m, 2)[bit >> 3] >> (bit & 7) & 3);
}

// the value of a move for the side making it: after a goal move only the outcome counts
std::uint16_t move_value(Pos const& p, Move const& mv)
{
        if (goal_move(p, mv)) {
                std::uint8_t const r = probe_wdl(mv.next);
                return r == WDL_LOSS ? win(1) : r == WDL_WIN ? loss(1) : 0;
        }
        std::uint16_t const v = probe(mv.next);
        return is_loss(v) ? win(plies(v) + 1) : is_win(v) ? loss(plies(v) + 1) : 0;
}

// the value recomputed from the successors (for the checks)
std::uint16_t value_from_successors(Pos const& p)
{
        auto const ml = moves(p);
        if (ml.empty()) return loss(0);
        std::uint16_t best = loss(0);
        bool first = true;
        for (auto const& mv : ml) {
                std::uint16_t const x = move_value(p, mv);
                best = first ? x : better(best, x);
                first = false;
        }
        return best;
}

// ---------------------------------------------------------------- one group: pass 0 and the rounds

void solve_group(MatIndex const& mi, Group const& g)
{
        std::uint64_t const P = g.P, T = 2 * P;                 // position id = stm * P + index in the group
        auto const pos = [&](std::uint64_t id) { return g.pos(id % P, int(id / P)); };
        auto const id_of = [&](Pos const& p) { return std::uint64_t(p.stm) * P + g.index(p); };
        std::vector<std::uint16_t> v(T, 0), ewin(T, 0), eloss(T, NONE);
        std::vector<std::uint8_t> open(T, 0);                   // undecided after pass 0: has a move inside

        // pass 0
        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        auto const ml = moves(p);
                        if (ml.empty()) { v[id] = loss(0); continue; }
                        bool in = false, drawn = false;
                        int w = INF, l = 0;
                        for (auto const& mv : ml) {
                                if (inside(p, mv)) { in = true; continue; }
                                std::uint16_t const x = move_value(p, mv);
                                if (is_win(x)) w = std::min(w, plies(x));
                                else if (is_loss(x)) l = std::max(l, plies(x));
                                else drawn = true;
                        }
                        if (!in) { v[id] = w < INF ? win(w) : drawn ? 0 : loss(l); continue; }     // decided
                        open[id] = 1;
                        ewin[id] = w < INF ? std::uint16_t(w) : 0;
                        eloss[id] = drawn || w < INF ? NONE : std::uint16_t(l);    // lost only if every exit loses
                }
        });
        int max_seed = 0;
        for (std::uint64_t id = 0; id < T; ++id) {
                max_seed = std::max({ max_seed, plies(v[id]), int(ewin[id]) });
                if (open[id] && eloss[id] != NONE) max_seed = std::max(max_seed, int(eloss[id]));
        }

        for (int n = 1;; ++n) {
                std::atomic<bool> found{ false };

                // losses in n: rtbgen's check_loss on the predecessors of the wins in n - 1, and on the exits losing in n
                auto const check_loss = [&](std::uint64_t id) {
                        if (!open[id] || load(v[id]) != 0 || eloss[id] == NONE || eloss[id] > n) return;
                        Pos const p = pos(id);
                        for (auto const& mv : moves(p)) {               // every move inside to a win in fewer than n plies
                                if (!inside(p, mv)) continue;
                                std::uint16_t const x = load(v[id_of(mv.next)]);
                                if (!is_win(x) || plies(x) >= n) return;        // pass 0 decides wins of any depth
                        }
                        store(v[id], loss(n));
                        found.store(true, std::memory_order_relaxed);
                };
                for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t q = lo; q < hi; ++q) {
                                if (n > 1 && load(v[q]) == win(n - 1)) {               // win(0) would be 0
                                        std::uint64_t const off = (1 - q / P) * P;     // the predecessors: other side to move
                                        for_each_inside_predecessor(g, pos(q), [&](std::uint64_t i) { check_loss(off + i); });
                                }
                                if (open[q] && eloss[q] == n) check_loss(q);
                        }
                });

                // wins in n: the undecided predecessors of the losses in n - 1, and the exits winning in n
                for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t q = lo; q < hi; ++q) {
                                if (load(v[q]) == loss(n - 1)) {
                                        std::uint64_t const off = (1 - q / P) * P;
                                        for_each_inside_predecessor(g, pos(q), [&](std::uint64_t i) {
                                                if (open[off + i] && load(v[off + i]) == 0) {
                                                        store(v[off + i], win(n));
                                                        found.store(true, std::memory_order_relaxed);
                                                }
                                        });
                                }
                                if (open[q] && ewin[q] == n && load(v[q]) == 0) {
                                        store(v[q], win(n));
                                        found.store(true, std::memory_order_relaxed);
                                }
                        }
                });
                if (!found && n > max_seed) break;              // positions decided at max_seed are a frontier in max_seed + 1
        }

        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        g_val[std::size_t(p.stm)][mi.index(p)] = v[id];
                }
        });
}

// ---------------------------------------------------------------- one material

bool write_file(std::string const& path, std::vector<std::uint8_t> const& bytes)
{
        std::string const tmp = path + ".tmp";
        {
                std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
                f.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));   // one sequential write
                if (!f) { std::cerr << "write failed " << tmp << std::endl; return false; }
        }
        std::filesystem::rename(tmp, path);
        return true;
}

bool build_material(std::string const& dir, Material c)
{
        if (!stored(c)) { std::cerr << c.name() << " is the mirror of a stored material" << std::endl; return false; }
        g_dir = dir;
        g_current = c;
        g_waves = true;                                 // king moves and man moves inside (dtz: king moves only)
        MatIndex const& mi = mat_index(c);
        for (auto& r : g_val) r.assign(mi.size, 0);
        auto const groups = groups_of(c);
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (auto const& g : groups) { solve_group(mi, g); positions += 2 * g.P; }

        std::uint64_t wins[2]{}, losses[2]{};
        int longest = 0;
        std::vector<std::uint8_t> wdl((2 * mi.size + 3) / 4, 0);
        for (int s : { 0, 1 }) {
                std::vector<std::uint8_t> bytes(mi.size);
                for (std::uint64_t i = 0; i < mi.size; ++i) {
                        std::uint16_t const v = g_val[std::size_t(s)][i];
                        if (v > 255) { std::cerr << "value beyond one byte" << std::endl; return false; }
                        bytes[i] = std::uint8_t(v);
                        std::uint64_t const bit = 2 * (std::uint64_t(s) * mi.size + i);
                        wdl[bit >> 3] |= std::uint8_t(wdl_of(v) << (bit & 7));
                        wins[s] += is_win(v);
                        losses[s] += is_loss(v);
                        longest = std::max(longest, plies(v));
                }
                if (!write_file(mat_path(dir, c, s), bytes)) return false;
        }
        if (!write_file(mat_path(dir, c, 2), wdl)) return false;
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "material " << c.name() << " " << metric_name() << ": " << groups.size() << " groups, " << positions
                  << " positions, " << mi.size << " slots, wins " << wins[0] << " / " << wins[1] << ", losses " << losses[0]
                  << " / " << losses[1] << " (White / Black to move), longest " << longest << " plies, " << sec << " s"
                  << std::endl;
        return true;
}

// ---------------------------------------------------------------- checks

std::vector<Material> stored_materials_up_to(int n)
{
        std::vector<Material> out;
        for (Material const c : materials_up_to(n))
                if (stored(c)) out.push_back(c);
        return out;
}

// dtm: every value equals egdb_6x6's DTM; dtc, dtz: the same wins and losses as egdb_6x6; all: every value equals its
// recomputation from the successors, and the .wdl file the sign of the value
bool check_all(std::string const& dir, int n, int threads)
{
        g_dir = dir;
        g_waves = true;
        build(n, threads, n);                                   // egdb_6x6, once for every material
        bool ok = true;
        std::uint64_t total = 0;
        for (Material const c : stored_materials_up_to(n)) {
                std::uint64_t checked = 0, bad_o = 0, bad_s = 0, bad_w = 0;
                for (auto const& g : groups_of(c))
                        for (std::uint64_t i = 0; i < g.P; ++i)
                                for (int s : { 0, 1 }) {
                                        Pos const p = g.pos(i, s);
                                        std::uint16_t const got = probe(p), o = lookup(p);
                                        std::uint16_t const expect = o == DRAW ? 0 : (o & 1) ? win(o) : loss(o);
                                        bool const o_ok = g_metric == Metric::dtm ? got == expect : wdl_of(got) == wdl_of(expect);
                                        if (!o_ok && bad_o++ < 3) std::cerr << "O " << fen_of(p) << " got " << got << " oracle " << show(o) << std::endl;
                                        std::uint16_t const sv = value_from_successors(p);
                                        if (sv != got && bad_s++ < 3) std::cerr << "S " << fen_of(p) << " stored " << got << " from successors " << sv << std::endl;
                                        if (probe_wdl(p) != wdl_of(got) && bad_w++ < 3) std::cerr << "W " << fen_of(p) << std::endl;
                                        ++checked;
                                }
                total += checked;
                std::cout << "check " << c.name() << " " << metric_name() << ": " << checked << " positions, " << bad_o
                          << " differences from egdb_6x6, " << bad_s << " from the successors, " << bad_w << " in the wdl file"
                          << std::endl;
                ok = ok && bad_o == 0 && bad_s == 0 && bad_w == 0;
        }
        std::cout << total << " positions; " << (ok ? "all materials check out" : "DIFFERENCES") << std::endl;
        return ok;
}

// thompson86 (one-sided, every material): .w = the plies of a White-to-move win, .l = 1 + the plies of a Black-to-move
// loss; here both from the two-sided values, a material's mirror read as its mirror image
bool compare_thompson86(std::string const& dir, std::string const& dir86, int n)
{
        g_dir = dir;
        g_waves = true;
        std::uint64_t checked = 0, bad = 0;
        for (Material const c : materials_up_to(n)) {
                MatIndex const& mi = mat_index(c);
                std::string const base = dir86 + "/6x6-" + c.name() + "." + metric_name();
                std::uint8_t const* w = map_file(base + ".w", mi.size);
                std::uint8_t const* l = map_file(base + ".l", mi.size);
                for (auto const& g : groups_of(c))
                        for (std::uint64_t i = 0; i < g.P; ++i) {
                                Pos const pw = g.pos(i, 0), pb = g.pos(i, 1);
                                std::uint16_t const vw = probe(pw), vb = probe(pb);
                                int const ew = is_win(vw) ? plies(vw) : 0, el = is_loss(vb) ? plies(vb) + 1 : 0;
                                std::uint64_t const k = mi.index(pw);
                                if (w[k] != ew && bad++ < 3) std::cerr << "w " << fen_of(pw) << " thompson86 " << int(w[k]) << " syzygy " << vw << std::endl;
                                if (l[k] != el && bad++ < 3) std::cerr << "l " << fen_of(pb) << " thompson86 " << int(l[k]) << " syzygy " << vb << std::endl;
                                checked += 2;
                        }
        }
        std::cout << "thompson86: " << checked << " positions, " << bad << " differences" << std::endl;
        return bad == 0;
}

// dtz <= dtc <= dtm for every win and every loss, with the same WDL in all three, and the same .wdl files
bool check_order(std::string const& dz, std::string const& dc, std::string const& dm, int n)
{
        std::uint64_t wins = 0, losses = 0, bad = 0, bad_wdl = 0;
        for (Material const c : stored_materials_up_to(n)) {
                std::uint64_t const size = mat_index(c).size;
                for (int s : { 0, 1 }) {
                        g_metric = Metric::dtz; auto const z = map_file(mat_path(dz, c, s), size);
                        g_metric = Metric::dtc; auto const x = map_file(mat_path(dc, c, s), size);
                        g_metric = Metric::dtm; auto const m = map_file(mat_path(dm, c, s), size);
                        for (std::uint64_t i = 0; i < size; ++i) {
                                if (wdl_of(z[i]) != wdl_of(m[i]) || wdl_of(x[i]) != wdl_of(m[i])) { ++bad; continue; }
                                if (m[i] == 0) continue;
                                (is_win(m[i]) ? wins : losses) += 1;
                                if (!(plies(z[i]) <= plies(x[i]) && plies(x[i]) <= plies(m[i]))) ++bad;
                        }
                }
                std::uint64_t const ws = file_size(c, 2);
                g_metric = Metric::dtz; auto const z = map_file(mat_path(dz, c, 2), ws);
                g_metric = Metric::dtc; auto const x = map_file(mat_path(dc, c, 2), ws);
                g_metric = Metric::dtm; auto const m = map_file(mat_path(dm, c, 2), ws);
                bad_wdl += !std::equal(z, z + ws, m) || !std::equal(x, x + ws, m);
        }
        std::cout << "order: " << wins << " wins, " << losses << " losses, " << bad
                  << " violations of dtz <= dtc <= dtm (or of the same WDL); " << bad_wdl << " materials with different .wdl files"
                  << std::endl;
        return bad == 0 && bad_wdl == 0;
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
        if ((a.size() == 7 || a.size() == 8) && a[0] == "build" && parse_metric(a[6])) {
                g_par = a.size() == 8 && a[7] == "par";
                return build_material(a[1], mat_at(2)) ? 0 : 1;
        }
        if ((a.size() == 4 || a.size() == 5) && a[0] == "checkall" && parse_metric(a[3]))
                return check_all(a[1], std::stoi(a[2]), a.size() == 5 ? std::stoi(a[4]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "cmp86" && parse_metric(a[4])) return compare_thompson86(a[1], a[2], std::stoi(a[3])) ? 0 : 1;
        if (a.size() == 5 && a[0] == "order") return check_order(a[1], a[2], a[3], std::stoi(a[4])) ? 0 : 1;
        std::cerr << "usage: syzygy build <dir> wp bp wm bm dtm|dtc|dtz [seq|par] | checkall <dir> n dtm|dtc|dtz [threads]"
                     " | cmp86 <dir> <dir_thompson86> n dtm|dtc|dtz | order <dir_dtz> <dir_dtc> <dir_dtm> n" << std::endl;
        return 2;
}
