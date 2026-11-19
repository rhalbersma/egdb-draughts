//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Syzygy style builds (after R. de Man's rtbgen) for 6x6 international draughts, two-sided, for three metrics, with
// the per-material index of thompson86 (src/materials_6x6.hpp). The value of a position, for the side to move:
//   0          draw (also an illegal slot)
//   2n         win in n plies (n >= 1)
//   2n + 1     loss in n plies (n >= 0; 0: no move)
// 16 bits during the build; stored as one unsigned byte with the sign from the wdl file (see to_byte). The metric decides which moves end the count (as in thompson86):
//   dtm   none: the count runs to the end of the game
//   dtc   captures and promotions (a conversion into another material)
//   dtz   captures and man moves (the zeroing moves of the 25-move rule)
// After a goal move only the outcome of the position it leads to counts (win in 1 ply, or loss in 1 ply).
//
// Materials. Two-sided: both sides to move, wins and losses, so only one of each pair of a material and its colour
// mirror is built (the same 82 materials up to 6 pieces as strongly_solving_6x6/materials); a position in the other
// one is looked up as its mirror image. Full results; per material four files, 6x6-wp-bp-wm-bm.<side to move>.<kind>
// (for dtm; dtc and dtz alike):
//   wtm.dtm   one byte per slot, White to move: the unsigned depth, k for a win in 2k - 1 or a loss in 2k plies
//             (dtc, dtz: n plies); the sign is in wtm.wdl
//   btm.dtm   one byte per slot, Black to move
//   wtm.wdl   two bits per slot (0 draw, 1 loss, 2 win), White to move
//   btm.wdl   two bits per slot, Black to move
// The wdl files are a by-product: the sign of every value, the same for all three metrics. They are all that dtc and
// dtz need from the materials that a move leads into (every move out of a material is a goal move there), so their
// builds read only the wdl files of their captures and promotions; dtm needs the depths and reads wtm.dtm, btm.dtm
// with the wdl files for their signs.
//
// Groups. Captures and promotions (and in dtz man moves, which end the count) are looked up forward in pass 0, in
// finished files: no uncaptures. A dtm or dtc material is one group, with king moves and man moves inside; a dtz
// material is solved one distance at a time (man moves lead to a smaller distance), with king moves inside.
//
// Passes, in the manner of rtbgen: one byte array per group, both sides to move.
//   pass 0      every position looks up its moves out of the group: without a move inside it is decided at once
//               (captures are compulsory); otherwise its fastest win and its slowest loss through an exit are kept
//               (no loss if an exit draws or wins)
//   round n     losses in n: every position won in n - 1 unmoves to its predecessors, and each undecided one whose
//               exits lose by n is checked forward (rtbgen's check_loss): lost iff every move inside leads to a win
//               in fewer than n plies (pass 0 decides positions of any depth: they are a frontier at their level);
//               also every undecided position whose exits lose in n. Then wins in n: every position lost in n - 1
//               unmoves to its undecided predecessors, and every undecided position whose exit wins in n
// Two ways to find the positions of the previous ply, with the same files:
//   frontier    (default) bit planes of the wins and losses found in the previous round, and lists by level of the
//               positions decided in pass 0 and of the exit seeds: a round visits only these
//   scan        as rtbgen: every round scans the whole byte array for the values of the previous ply
// Within a round the losses read only wins of earlier rounds, and the wins only losses of earlier rounds, so each
// half can run in parallel (std::for_each, seq or par, over blocks of positions; every writer writes the same value).
// Rule 6: the group's arrays are in RAM; finished materials are read through read-only memory maps; each file is
// written once, sequentially.
//
// build: g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include
//        -I<xstd-misc>/include syzygy/syzygy.cpp -lzstd -ltbb -o syzygy
// usage: syzygy build <dir> wp bp wm bm <metric> [seq|par] [frontier|scan] [mem=MB]
//                                                            build one material (its captures and promotions must exist);
//                                                            mem: the job's claim, checked against the estimate
//        syzygy plan n <metric>                              per stored material with at most n pieces: slots,
//                                                            positions, groups, the largest group, MB needed (rung 0)
//        syzygy checkall <dir> n <metric> [threads]          every material of at most n pieces against egdb_6x6
//                                                            (built in memory): dtm exactly, dtc / dtz their wins and
//                                                            losses; every value against its successors; the wdl files
//                                                            against the values
//        syzygy cmp86 <dir> <dir_thompson86> n <metric>      the same half results (dtm-W, dtm-L) as thompson86, for every
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

bool g_frontier = true;                                 // rounds from frontier bit planes (false: rtbgen's full scans)

// relaxed atomic access to the value array during the parallel passes
std::uint16_t load(std::uint16_t& x) { return std::atomic_ref<std::uint16_t>(x).load(std::memory_order_relaxed); }
void store(std::uint16_t& x, std::uint16_t v) { std::atomic_ref<std::uint16_t>(x).store(v, std::memory_order_relaxed); }

// ---------------------------------------------------------------- materials and files

// one of each pair of a material and its colour mirror, as in egdb_6x6 and strongly_solving_6x6/materials
bool stored(Material m) { return !(std::tuple(m.bm, m.wm, m.bk, m.wk) < std::tuple(m.wm, m.bm, m.wk, m.bk)); }

// kind 0: wtm.<metric>, 1: btm.<metric> (one byte per slot); 2: wtm.wdl, 3: btm.wdl (two bits per slot); kind = side to
// move + 2 * wdl. The wdl files carry no metric: they are the same for all three
enum : int { BYTES = 0, WDL = 2 };

std::string mat_path(std::string const& dir, Material m, int kind)
{
        return dir + "/6x6-" + m.name() + (kind % 2 == 0 ? ".wtm." : ".btm.") + (kind >= WDL ? std::string("wdl") : std::string(metric_name()));
}

std::uint64_t file_size(Material m, int kind) { return kind < WDL ? mat_index(m).size : (mat_index(m).size + 3) / 4; }

std::string g_dir;
Material g_current{ -1, -1, -1, -1 };
std::array<std::vector<std::uint16_t>, 2> g_val;        // the material being built, 16 bits per slot and side to move
std::map<std::pair<Material, int>, std::uint8_t const*> g_files;

bool g_frozen = false;                                  // during the passes of a build: the maps are only read

std::uint8_t const* mat_file(Material m, int kind)
{
        auto it = g_files.find({ m, kind });
        if (it != g_files.end()) return it->second;
        if (g_frozen) { std::cerr << "not opened before the passes: " << mat_path(g_dir, m, kind) << std::endl; std::abort(); }
        return g_files[{ m, kind }] = map_file(mat_path(g_dir, m, kind), file_size(m, kind));
}

Material canonical(Material m) { return stored(m) ? m : mirror_of(m); }

// the stored materials that one move can lead into
std::vector<Material> successors_of(Material c)
{
        std::vector<Material> out;
        for (Material const m : successor_materials(c)) {
                Material const k = canonical(m);
                if (k != c && std::ranges::find(out, k) == out.end()) out.push_back(k);
        }
        return out;
}

// open every file a build of c can read, and fill the index memo, so that the parallel passes only read the maps
void open_successors(Material c)
{
        mat_index(c);
        for (Material const m : successors_of(c)) {
                mat_index(m);
                mat_index(mirror_of(m));
                for (int kind : g_metric == Metric::dtm ? std::vector<int>{ BYTES, BYTES + 1, WDL, WDL + 1 } : std::vector<int>{ WDL, WDL + 1 })
                        mat_file(m, kind);                      // dtm: the depths with their signs; dtc, dtz: the signs
        }
}

// a position in a stored material: itself or its mirror image (the value for the side to move is the same)
Pos stored_pos(Pos const& p) { return stored(material_of(p)) ? p : mirror(p); }

// The stored bytes: unsigned depths, the sign (win, draw or loss) from the wdl file of the same side to move. A dtm win
// always takes an odd number of plies and a dtm loss an even number, so dtm stores moves; dtc and dtz store plies
// (their wins can take an even number of plies):
//   dtm   k: a win in 2k - 1 plies, or a loss in 2k plies (0: lost in 0 plies, no move)
//   dtc, dtz   n: a win or a loss in n plies
// A draw is 0 in both files. Exact plies either way; the build works in plies (2n win, 2n + 1 loss) and only the files
// differ: up to 509 plies of dtm win (510 of loss) and 255 plies of dtc or dtz in a byte.
int to_byte(Metric mt, std::uint16_t v)
{
        if (v == 0) return 0;
        if (mt != Metric::dtm) return plies(v);
        if ((plies(v) % 2 == 1) != is_win(v)) { std::cerr << "dtm: a win in an even or a loss in an odd number of plies" << std::endl; std::exit(1); }
        return is_win(v) ? (plies(v) + 1) / 2 : plies(v) / 2;
}

std::uint16_t from_byte(Metric mt, std::uint8_t s, std::uint8_t r)    // r: the wdl of the same position
{
        if (r == WDL_DRAW) return 0;
        bool const dtm = mt == Metric::dtm;
        return r == WDL_WIN ? win(dtm ? 2 * s - 1 : s) : loss(dtm ? 2 * s : s);
}

std::uint8_t wdl_bits(std::uint8_t const* w, std::uint64_t i) { return std::uint8_t(w[i >> 2] >> (2 * (i & 3)) & 3); }

// the value of a position for the side to move
std::uint16_t probe(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return loss(0);                    // no pieces, no move
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        if (m == g_current) return g_val[std::size_t(p.stm)][i];
        return from_byte(g_metric, mat_file(m, BYTES + p.stm)[i], wdl_bits(mat_file(m, WDL + p.stm), i));
}

// win, draw or loss of a position for the side to move
std::uint8_t probe_wdl(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return WDL_LOSS;
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        if (m == g_current) return wdl_of(g_val[std::size_t(p.stm)][i]);
        return wdl_bits(mat_file(m, WDL + p.stm), i);
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

        // the two steps of a round, shared by both ways of finding the frontier
        auto const check_loss = [&](std::uint64_t id, int n) {                 // rtbgen's check_loss: lost in n?
                if (!open[id] || load(v[id]) != 0 || eloss[id] == NONE || eloss[id] > n) return false;
                Pos const p = pos(id);
                for (auto const& mv : moves(p)) {                               // every move inside to a win in fewer than n plies
                        if (!inside(p, mv)) continue;
                        std::uint16_t const x = load(v[id_of(mv.next)]);
                        if (!is_win(x) || plies(x) >= n) return false;          // pass 0 decides wins of any depth
                }
                store(v[id], loss(n));
                return true;
        };
        auto const mark_win = [&](std::uint64_t id, int n) {                    // undecided: won in n
                if (!open[id] || load(v[id]) != 0) return false;
                store(v[id], win(n));
                return true;
        };
        auto const predecessors = [&](std::uint64_t q, auto&& f) {             // the other side to move
                std::uint64_t const off = (1 - q / P) * P;
                for_each_inside_predecessor(g, pos(q), [&](std::uint64_t i) { f(off + i); });
        };

        if (!g_frontier) {
                // rtbgen: every round scans the whole group for the values of the previous ply
                for (int n = 1;; ++n) {
                        std::atomic<bool> found{ false };
                        auto const note = [&](bool b) { if (b) found.store(true, std::memory_order_relaxed); };
                        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {         // losses in n
                                for (std::uint64_t q = lo; q < hi; ++q) {
                                        if (n > 1 && load(v[q]) == win(n - 1))            // win(0) would be 0
                                                predecessors(q, [&](std::uint64_t id) { note(check_loss(id, n)); });
                                        if (open[q] && eloss[q] == n) note(check_loss(q, n));
                                }
                        });
                        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {         // wins in n
                                for (std::uint64_t q = lo; q < hi; ++q) {
                                        if (load(v[q]) == loss(n - 1))
                                                predecessors(q, [&](std::uint64_t id) { note(mark_win(id, n)); });
                                        if (open[q] && ewin[q] == n) note(mark_win(q, n));
                                }
                        });
                        if (!found && n > max_seed) break;      // positions decided at max_seed are a frontier in max_seed + 1
                }
        } else {
                // frontiers: bit planes of the wins and losses found in the previous round, plus lists by level of the
                // positions decided in pass 0 and of the exits; a round visits only these
                using List = std::vector<std::uint32_t>;
                std::vector<List> won_at(std::size_t(max_seed) + 1), lost_at(won_at.size()), ewin_at(won_at.size()),
                        eloss_at(won_at.size());
                for (std::uint64_t id = 0; id < T; ++id) {
                        auto const i = std::uint32_t(id);
                        if (!open[id]) {
                                if (is_win(v[id])) won_at[std::size_t(plies(v[id]))].push_back(i);
                                else if (is_loss(v[id])) lost_at[std::size_t(plies(v[id]))].push_back(i);
                                continue;
                        }
                        if (ewin[id] != 0) ewin_at[ewin[id]].push_back(i);
                        if (eloss[id] != NONE && eloss[id] != 0) eloss_at[eloss[id]].push_back(i);
                }
                auto const at = [&](std::vector<List> const& by, int n) -> List const& {
                        static List const none;
                        return n >= 0 && n < int(by.size()) ? by[std::size_t(n)] : none;
                };
                auto const for_list = [&](List const& l, auto&& f) {
                        for_blocks(l.size(), [&](std::uint64_t lo, std::uint64_t hi) { for (auto j = lo; j < hi; ++j) f(l[j]); });
                };
                auto const for_plane = [&](Plane const& pl, auto&& f) {
                        for_blocks(pl.w.size(), [&](std::uint64_t lo, std::uint64_t hi) {
                                for (auto k = lo; k < hi; ++k)
                                        for (std::uint64_t b = pl.w[k]; b; b &= b - 1) f(k * 64 + std::uint64_t(std::countr_zero(b)));
                        });
                };
                auto const set_bit = [](Plane& pl, std::uint64_t i) {
                        std::atomic_ref<std::uint64_t>(pl.w[i >> 6]).fetch_or(std::uint64_t(1) << (i & 63), std::memory_order_relaxed);
                };
                Plane won_prev(T), lost_prev(T), won_new(T), lost_new(T);
                for (int n = 1;; ++n) {
                        auto const lose = [&](std::uint64_t id) { if (check_loss(id, n)) set_bit(lost_new, id); };
                        auto const gain = [&](std::uint64_t id) { if (mark_win(id, n)) set_bit(won_new, id); };
                        // losses in n: the predecessors of the wins in n - 1, and the exits losing in n
                        for_plane(won_prev, [&](std::uint64_t q) { predecessors(q, lose); });
                        for_list(at(won_at, n - 1), [&](std::uint64_t q) { predecessors(q, lose); });
                        for_list(at(eloss_at, n), lose);
                        // wins in n: the predecessors of the losses in n - 1, and the exits winning in n
                        for_plane(lost_prev, [&](std::uint64_t q) { predecessors(q, gain); });
                        for_list(at(lost_at, n - 1), [&](std::uint64_t q) { predecessors(q, gain); });
                        for_list(at(ewin_at, n), gain);
                        std::swap(won_prev, won_new);
                        std::swap(lost_prev, lost_new);
                        won_new.clear();
                        lost_new.clear();
                        if (won_prev.empty() && lost_prev.empty() && n > max_seed) break;
                }
        }

        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        g_val[std::size_t(p.stm)][mi.index(p)] = v[id];
                }
        });
}

// ---------------------------------------------------------------- one material

std::uint64_t g_mem_mb = 0;                                     // the job's memory claim in MB (0: none)

// the memory a build of c needs: the material's 16-bit values (both sides to move) and the largest group's value,
// seed and flag arrays, frontier planes and level lists (a bound); only rung 0 exists, so rung 1 is the same
MemoryNeed memory_need(Material c)
{
        g_waves = true;
        MemoryNeed m;
        MatIndex const& mi = mat_index(c);
        auto const groups = groups_of(c);
        m.slots = mi.size;
        m.groups = groups.size();
        for (auto const& g : groups) {
                m.positions += 2 * g.P;
                m.max_group = std::max(m.max_group, g.P);
                m.slices += g.units.size();
        }
        std::uint64_t const T = 2 * m.max_group;
        // calibrated on 6 pieces (peak RSS against these): the level lists measured at up to about 4 bytes per position
        // and side to move; a group about 480 bytes, a slice 160
        std::uint64_t const transient = 7 * T + 4 * ((T + 63) / 64) * 8 + 4 * T;       // v, ewin, eloss, open; planes; lists
        std::uint64_t const meta = 160 * m.slices + 480 * m.groups;
        std::uint64_t const writing = mi.size + (mi.size + 3) / 4;                     // one side's bytes and wdl at a time
        m.rung0 = m.rung1 = FIXED_BYTES + meta + 4 * mi.size + std::max(transient, writing);
        return m;
}

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
        if (g_mem_mb != 0) {
                std::uint64_t const r0 = to_mb(memory_need(c).rung0);
                std::cout << "memory: claim " << g_mem_mb << " MB, rung 0 " << r0 << " MB"
                          << (r0 > g_mem_mb ? " (over the claim: needs a lower rung, not implemented yet)" : "") << std::endl;
        }
        g_dir = dir;
        g_current = c;
        g_waves = true;                                 // king moves and man moves inside (dtz: king moves only)
        MatIndex const& mi = mat_index(c);
        for (auto& r : g_val) r.assign(mi.size, 0);
        open_successors(c);
        g_frozen = true;
        auto const groups = groups_of(c);
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (auto const& g : groups) { solve_group(mi, g); positions += 2 * g.P; }

        std::uint64_t wins[2]{}, losses[2]{};
        int longest = 0;
        for (int s : { 0, 1 }) {
                std::vector<std::uint8_t> bytes(mi.size), wdl((mi.size + 3) / 4, 0);
                for (std::uint64_t i = 0; i < mi.size; ++i) {
                        std::uint16_t const v = g_val[std::size_t(s)][i];
                        int const b = to_byte(g_metric, v);
                        if (b > 255) { std::cerr << "value beyond one byte" << std::endl; return false; }
                        bytes[i] = std::uint8_t(b);
                        wdl[i >> 2] |= std::uint8_t(wdl_of(v) << (2 * (i & 3)));
                        wins[s] += is_win(v);
                        losses[s] += is_loss(v);
                        longest = std::max(longest, plies(v));
                }
                if (!write_file(mat_path(dir, c, BYTES + s), bytes) || !write_file(mat_path(dir, c, WDL + s), wdl)) return false;
        }
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "material " << c.name() << " " << metric_name() << ": " << groups.size() << " groups, " << positions
                  << " positions, " << mi.size << " slots, wins " << wins[0] << " / " << wins[1] << ", losses " << losses[0]
                  << " / " << losses[1] << " (White / Black to move), longest " << longest << " plies, peak RSS " << peak_rss_mb()
                  << " MB, " << sec << " s"
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
// recomputation from the successors, and the wdl files the sign of the value
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
                          << " differences from egdb_6x6, " << bad_s << " from the successors, " << bad_w << " in the wdl files"
                          << std::endl;
                ok = ok && bad_o == 0 && bad_s == 0 && bad_w == 0;
        }
        std::cout << total << " positions; " << (ok ? "all materials check out" : "DIFFERENCES") << std::endl;
        return ok;
}

// thompson86 (one-sided, every material): wtm.<metric>-W = the plies of a White-to-move win, btm.<metric>-L = 1 + the
// plies of a Black-to-move loss; here both from the two-sided values, a material's mirror read as its mirror image
bool compare_thompson86(std::string const& dir, std::string const& dir86, int n)
{
        g_dir = dir;
        g_waves = true;
        std::uint64_t checked = 0, bad = 0;
        for (Material const c : materials_up_to(n)) {
                MatIndex const& mi = mat_index(c);
                std::string const base = dir86 + "/6x6-" + c.name();
                std::uint8_t const* w = map_file(base + ".wtm." + metric_name() + "-W", mi.size);
                std::uint8_t const* l = map_file(base + ".btm." + metric_name() + "-L", mi.size);
                for (auto const& g : groups_of(c))
                        for (std::uint64_t i = 0; i < g.P; ++i) {
                                Pos const pw = g.pos(i, 0), pb = g.pos(i, 1);
                                std::uint16_t const vw = probe(pw), vb = probe(pb);
                                bool const dtm = g_metric == Metric::dtm;              // thompson86's bytes (dtm in moves)
                                int const ew = !is_win(vw) ? 0 : dtm ? (plies(vw) + 1) / 2 : plies(vw);
                                int const el = !is_loss(vb) ? 0 : dtm ? plies(vb) / 2 + 1 : plies(vb) + 1;
                                std::uint64_t const k = mi.index(pw);
                                if (w[k] != ew && bad++ < 3) std::cerr << "w " << fen_of(pw) << " thompson86 " << int(w[k]) << " syzygy " << vw << std::endl;
                                if (l[k] != el && bad++ < 3) std::cerr << "l " << fen_of(pb) << " thompson86 " << int(l[k]) << " syzygy " << vb << std::endl;
                                checked += 2;
                        }
        }
        std::cout << "thompson86: " << checked << " positions, " << bad << " differences" << std::endl;
        return bad == 0;
}

// dtz <= dtc <= dtm for every win and every loss, with the same WDL in all three, and the same wdl files
bool check_order(std::string const& dz, std::string const& dc, std::string const& dm, int n)
{
        std::uint64_t wins = 0, losses = 0, bad = 0, bad_wdl = 0;
        for (Material const c : stored_materials_up_to(n)) {
                std::uint64_t const size = mat_index(c).size;
                for (int s : { 0, 1 }) {
                        std::uint64_t const ws = file_size(c, WDL + s);
                        g_metric = Metric::dtz; auto const z = map_file(mat_path(dz, c, s), size), zw = map_file(mat_path(dz, c, WDL + s), ws);
                        g_metric = Metric::dtc; auto const x = map_file(mat_path(dc, c, s), size), xw = map_file(mat_path(dc, c, WDL + s), ws);
                        g_metric = Metric::dtm; auto const m = map_file(mat_path(dm, c, s), size), mw = map_file(mat_path(dm, c, WDL + s), ws);
                        for (std::uint64_t i = 0; i < size; ++i) {
                                std::uint16_t const zv = from_byte(Metric::dtz, z[i], wdl_bits(zw, i)),
                                                    xv = from_byte(Metric::dtc, x[i], wdl_bits(xw, i)),
                                                    mv = from_byte(Metric::dtm, m[i], wdl_bits(mw, i));
                                if (wdl_of(zv) != wdl_of(mv) || wdl_of(xv) != wdl_of(mv)) { ++bad; continue; }
                                if (mv == 0) continue;
                                (is_win(mv) ? wins : losses) += 1;
                                if (!(plies(zv) <= plies(xv) && plies(xv) <= plies(mv))) ++bad;
                        }
                }
                for (int s : { 0, 1 }) {
                        std::uint64_t const ws = file_size(c, WDL + s);
                        g_metric = Metric::dtz; auto const z = map_file(mat_path(dz, c, WDL + s), ws);
                        g_metric = Metric::dtc; auto const x = map_file(mat_path(dc, c, WDL + s), ws);
                        g_metric = Metric::dtm; auto const m = map_file(mat_path(dm, c, WDL + s), ws);
                        bad_wdl += !std::equal(z, z + ws, m) || !std::equal(x, x + ws, m);
                }
        }
        std::cout << "order: " << wins << " wins, " << losses << " losses, " << bad
                  << " violations of dtz <= dtc <= dtm (or of the same WDL); " << bad_wdl << " wdl files that differ"
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
        if (a.size() == 3 && a[0] == "plan" && parse_metric(a[2])) {
                print_plan_header();
                for (Material const c : stored_materials_up_to(std::stoi(a[1]))) print_plan_row(c, memory_need(c));
                return 0;
        }
        if (a.size() >= 7 && a.size() <= 10 && a[0] == "build" && parse_metric(a[6])) {
                for (std::size_t i = 7; i < a.size(); ++i) {
                        if (a[i] == "seq" || a[i] == "par") g_par = a[i] == "par";
                        else if (a[i] == "scan" || a[i] == "frontier") g_frontier = a[i] == "frontier";
                        else if (a[i].starts_with("mem=")) g_mem_mb = std::stoull(a[i].substr(4));
                        else { std::cerr << "unknown option " << a[i] << std::endl; return 2; }
                }
                return build_material(a[1], mat_at(2)) ? 0 : 1;
        }
        if ((a.size() == 4 || a.size() == 5) && a[0] == "checkall" && parse_metric(a[3]))
                return check_all(a[1], std::stoi(a[2]), a.size() == 5 ? std::stoi(a[4]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "cmp86" && parse_metric(a[4])) return compare_thompson86(a[1], a[2], std::stoi(a[3])) ? 0 : 1;
        if (a.size() == 5 && a[0] == "order") return check_order(a[1], a[2], a[3], std::stoi(a[4])) ? 0 : 1;
        std::cerr << "usage: syzygy build <dir> wp bp wm bm dtm|dtc|dtz [seq|par] [frontier|scan] [mem=MB] | plan n dtm|dtc|dtz | checkall <dir> n dtm|dtc|dtz [threads]"
                     " | cmp86 <dir> <dir_thompson86> n dtm|dtc|dtz | order <dir_dtz> <dir_dtc> <dir_dtm> n" << std::endl;
        return 2;
}
