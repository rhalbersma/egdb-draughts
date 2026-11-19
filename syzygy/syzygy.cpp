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
// one is looked up as its mirror image. Full results; per material four files, 6x6/np/wp-bp/wm-bm-<kind>-<side to move>
// (for dtm; dtc and dtz alike):
//   dtm-wtm   one byte per slot, White to move: the unsigned depth, k for a win in 2k - 1 or a loss in 2k plies
//             (dtc, dtz: n plies); the sign is in wdl-wtm
//   dtm-btm   one byte per slot, Black to move
//   wdl-wtm   two bits per slot (0 draw, 1 loss, 2 win), White to move
//   wdl-btm   two bits per slot, Black to move
// The wdl files are a by-product: the sign of every value, the same for all three metrics. They are all that dtc and
// dtz need from the materials that a move leads into (every move out of a material is a goal move there), so their
// builds read only the wdl files of their captures and promotions; dtm needs the depths and reads dtm-wtm, dtm-btm
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
// usage: syzygy build <dir> wp bp wm bm <metric> [seq|par] [frontier|scan] [auto|groups|array] [material|tempi|configuration] [mem=MB]
//                                                            build one material (its captures and promotions must exist);
//                                                            the depth sink by the claim mem=MB (auto: the array if rung 0
//                                                            fits, else group files), or as given; the granularity of
//                                                            the groups (default material; below it man moves are
//                                                            exits)
//        syzygy plan n <metric> [granularity]                per stored material with at most n pieces: slots,
//                                                            positions, groups, the largest group, MB needed (rung 0)
//        syzygy checkall <dir> n <metric> [threads]          every material of at most n pieces against egdb_6x6
//                                                            (built in memory): dtm exactly, dtc / dtz their wins and
//                                                            losses; every value against its successors; the wdl files
//                                                            against the values
//        syzygy cmp86 <dir> <dir_thompson86> n <metric>      the same half results (dmW, dmL) as thompson86, for every
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

// kind 0: <metric>-wtm, 1: <metric>-btm (one byte per slot); 2: wdl-wtm, 3: wdl-btm (two bits per slot); kind = side to
// move + 2 * wdl. The wdl files carry no metric: they are the same for all three
enum : int { BYTES = 0, WDL = 2 };

std::string mat_path(std::string const& dir, Material m, int kind)
{
        return m.path(dir) + "-" + (kind >= WDL ? std::string("wdl") : std::string(metric_name())) + (kind % 2 == 0 ? "-wtm" : "-btm");
}

std::uint64_t file_size(Material m, int kind) { return kind < WDL ? mat_index(m).size : (mat_index(m).size + 3) / 4; }

std::string g_dir;
Material g_current{ -1, -1, -1, -1 };
DepthSink<std::uint16_t>* g_sink = nullptr;             // the material being built: its values (src/materials_6x6.hpp)
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
        if (m == g_current) return g_sink->read(p.stm, p);
        return from_byte(g_metric, mat_file(m, BYTES + p.stm)[i], wdl_bits(mat_file(m, WDL + p.stm), i));
}

// win, draw or loss of a position for the side to move
std::uint8_t probe_wdl(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return WDL_LOSS;
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        if (m == g_current) return wdl_of(g_sink->read(p.stm, p));
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

// The values go to the depth sink: frontier rounds never read them back, so they write straight into it; scan mode
// finds its frontier by scanning the values, so it keeps them in an array of the group and hands them over at the end.
void solve_group(Group const& g, DepthSink<std::uint16_t>& sink)
{
        std::uint64_t const P = g.P, T = 2 * P;                 // position id = stm * P + index in the group
        auto const pos = [&](std::uint64_t id) { return g.pos(id % P, int(id / P)); };
        auto const id_of = [&](Pos const& p) { return std::uint64_t(p.stm) * P + g.index(p); };
        std::vector<std::uint16_t> v(g_frontier ? 0 : T, 0);    // scan mode only
        auto const set_value = [&](std::uint64_t id, Pos const& p, std::uint16_t value) {
                if (g_frontier) sink.put(int(id / P), id % P, p, value);
                else v[id] = value;
        };

        // pass 0: every position looks up its moves out of the group. Decided ones get their value; the others leave
        // bits (has a move inside; every exit loses; no exits) and, by level, their fastest win and slowest loss through
        // an exit. A block of 4096 positions owns the words of its bits and has its own lists, merged by level after.
        struct Entry { std::uint32_t level, id; };
        std::uint64_t const nblocks = (T + 4095) / 4096;
        std::vector<std::vector<Entry>> won_b(nblocks), lost_b(nblocks), ewin_b(nblocks), eloss_b(nblocks);
        Plane openp(T), lossable(T), exits_done(T);
        auto const t0 = std::chrono::steady_clock::now();
        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {
                std::uint64_t const blk = lo / 4096;
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        auto const ml = moves(p);
                        auto const i = std::uint32_t(id);
                        if (ml.empty()) { set_value(id, p, loss(0)); lost_b[blk].push_back({ 0, i }); continue; }
                        bool in = false, drawn = false;
                        int w = INF, l = 0;
                        for (auto const& mv : ml) {
                                if (inside(p, mv)) { in = true; continue; }
                                std::uint16_t const x = move_value(p, mv);
                                if (is_win(x)) w = std::min(w, plies(x));
                                else if (is_loss(x)) l = std::max(l, plies(x));
                                else drawn = true;
                        }
                        if (!in) {                                      // decided
                                if (w < INF) { set_value(id, p, win(w)); won_b[blk].push_back({ std::uint32_t(w), i }); }
                                else if (!drawn) { set_value(id, p, loss(l)); lost_b[blk].push_back({ std::uint32_t(l), i }); }
                                continue;
                        }
                        openp.set(id);
                        if (w < INF) ewin_b[blk].push_back({ std::uint32_t(w), i });
                        if (drawn || w < INF) continue;                 // lost only if every exit loses
                        lossable.set(id);
                        if (l == 0) exits_done.set(id);                 // no exits
                        else eloss_b[blk].push_back({ std::uint32_t(l), i });
                }
        });
        int max_seed = 0;
        for (auto const* parts : { &won_b, &lost_b, &ewin_b, &eloss_b })
                for (auto const& v_ : *parts)
                        for (auto const& e : v_) max_seed = std::max(max_seed, int(e.level));
        g_stats.pass0 += seconds_since(t0);
        auto const t1 = std::chrono::steady_clock::now();
        using List = std::vector<std::uint32_t>;
        auto const by_level = [&](std::vector<std::vector<Entry>>& parts) {    // in block order within a level
                std::vector<List> out(std::size_t(max_seed) + 1);
                for (auto& v_ : parts) {
                        for (auto const& e : v_) out[e.level].push_back(e.id);
                        std::vector<Entry>().swap(v_);
                }
                return out;
        };
        std::vector<List> const won_at = by_level(won_b), lost_at = by_level(lost_b), ewin_at = by_level(ewin_b),
                                eloss_at = by_level(eloss_b);
        auto const at = [&](std::vector<List> const& by, int n) -> List const& {
                static List const none;
                return n >= 0 && n < int(by.size()) ? by[std::size_t(n)] : none;
        };
        auto const for_list = [&](List const& l, auto&& f) {
                for_blocks(l.size(), [&](std::uint64_t lo, std::uint64_t hi) { for (auto j = lo; j < hi; ++j) f(l[j]); });
        };
        auto const predecessors = [&](std::uint64_t q, auto&& f) {             // the other side to move
                std::uint64_t const off = (1 - q / P) * P;
                for_each_inside_predecessor(g, pos(q), [&](std::uint64_t i) { f(off + i); });
        };

        if (!g_frontier) {
                // rtbgen: every round scans the whole group for the values of the previous ply; the values are the state
                auto const check_loss = [&](std::uint64_t id, int n) {         // rtbgen's check_loss: lost in n?
                        if (!openp.test(id) || load(v[id]) != 0 || !lossable.test(id) || !exits_done.test(id)) return false;
                        Pos const p = pos(id);
                        for (auto const& mv : moves(p)) {                       // every move inside to a win in fewer than n plies
                                if (!inside(p, mv)) continue;
                                std::uint16_t const x = load(v[id_of(mv.next)]);
                                if (!is_win(x) || plies(x) >= n) return false;  // pass 0 decides wins of any depth
                        }
                        store(v[id], loss(n));
                        return true;
                };
                auto const mark_win = [&](std::uint64_t id, int n) {            // undecided: won in n
                        if (!openp.test(id) || load(v[id]) != 0) return false;
                        store(v[id], win(n));
                        return true;
                };
                for (int n = 1;; ++n) {
                        std::atomic<bool> found{ false };
                        auto const note = [&](bool b) { if (b) found.store(true, std::memory_order_relaxed); };
                        for_list(at(eloss_at, n), [&](std::uint64_t id) { exits_done.set_atomic(id); });   // exits lost by n
                        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {         // losses in n
                                for (std::uint64_t q = lo; q < hi; ++q)
                                        if (n > 1 && load(v[q]) == win(n - 1))            // win(0) would be 0
                                                predecessors(q, [&](std::uint64_t id) { note(check_loss(id, n)); });
                        });
                        for_list(at(eloss_at, n), [&](std::uint64_t id) { note(check_loss(id, n)); });
                        for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {         // wins in n
                                for (std::uint64_t q = lo; q < hi; ++q)
                                        if (load(v[q]) == loss(n - 1))
                                                predecessors(q, [&](std::uint64_t id) { note(mark_win(id, n)); });
                        });
                        for_list(at(ewin_at, n), [&](std::uint64_t id) { note(mark_win(id, n)); });
                        if (!found && n > max_seed) {   // positions decided at max_seed are a frontier in max_seed + 1
                                g_stats.levels = std::max(g_stats.levels, std::uint64_t(n));
                                break;
                        }
                }
                for_blocks(T, [&](std::uint64_t lo, std::uint64_t hi) {         // hand the values to the sink
                        for (std::uint64_t id = lo; id < hi; ++id)
                                if (v[id] != 0) sink.put(int(id / P), id % P, pos(id), v[id]);
                });
        } else {
                // frontiers: bit planes of the wins and losses found in the previous round, plus the lists by level of the
                // positions decided in pass 0 and of the exits; a round visits only these. The state the rounds test is
                // in cumulative bit planes, not in the values: won and lost hold every position decided at a level
                // below the round (pass-0 decisions join when their level comes round), so v is only written
                Plane won(T), lost(T), won_prev(T), lost_prev(T), won_new(T), lost_new(T);
                for (int n = 1;; ++n) {
                        for_list(at(eloss_at, n), [&](std::uint64_t id) { exits_done.set_atomic(id); });   // exits lost by n
                        // losses in n: lossable, exits lost by n, undecided, every move inside to a win of a level below n
                        auto const lose = [&](std::uint64_t id) {
                                if (!openp.test(id) || !lossable.test(id) || !exits_done.test(id) || won.test(id) || lost.test(id)) return;
                                Pos const p = pos(id);
                                for (auto const& mv : moves(p))
                                        if (inside(p, mv) && !won.test(id_of(mv.next))) return;
                                sink.put(int(id / P), id % P, p, loss(n));
                                lost_new.set_atomic(id);
                        };
                        // wins in n: undecided, and not lost in n
                        auto const gain = [&](std::uint64_t id) {
                                if (!openp.test(id) || won.test(id) || lost.test(id) || lost_new.test(id)) return;
                                sink.put(int(id / P), id % P, pos(id), win(n));
                                won_new.set_atomic(id);
                        };
                        // losses in n: the predecessors of the wins in n - 1, and the exits losing in n
                        won_prev.for_each_par([&](std::uint64_t q) { predecessors(q, lose); });
                        for_list(at(won_at, n - 1), [&](std::uint64_t q) { predecessors(q, lose); });
                        for_list(at(eloss_at, n), lose);
                        // wins in n: the predecessors of the losses in n - 1, and the exits winning in n
                        lost_prev.for_each_par([&](std::uint64_t q) { predecessors(q, gain); });
                        for_list(at(lost_at, n - 1), [&](std::uint64_t q) { predecessors(q, gain); });
                        for_list(at(ewin_at, n), gain);
                        // commit level n: the new wins and losses, and the pass-0 decisions of level n
                        won_new.for_each_par([&](std::uint64_t q) { won.set(q); });
                        lost_new.for_each_par([&](std::uint64_t q) { lost.set(q); });
                        for_list(at(won_at, n), [&](std::uint64_t q) { won.set_atomic(q); });
                        for_list(at(lost_at, n), [&](std::uint64_t q) { lost.set_atomic(q); });
                        std::swap(won_prev, won_new);
                        std::swap(lost_prev, lost_new);
                        won_new.clear();
                        lost_new.clear();
                        g_stats.frontier += won_prev.count() + lost_prev.count();
                        if (won_prev.empty() && lost_prev.empty() && n > max_seed) {
                                g_stats.levels = std::max(g_stats.levels, std::uint64_t(n));
                                break;
                        }
                }
        }
        g_stats.rounds += seconds_since(t1);
}

// ---------------------------------------------------------------- one material

std::uint64_t g_mem_mb = 0;                                     // the job's memory claim in MB (0: none)

bool g_group_files = true;                                      // the depth sink: group files (false: the material array)
bool g_sink_auto = true;                                        // choose the sink by the memory claim

// the memory a build of c needs. Rung 0 keeps the material's 16-bit values in RAM (the array sink, both sides to
// move), rung 1 only the current group's (the group sink); both hold the largest group's bit planes and level lists,
// and in scan mode also its value array
MemoryNeed memory_need(Material c)
{
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
        // 9 bit planes (has a move inside, every exit loses, exits lost by now, won, lost, and four frontiers); the level
        // lists with pass 0's per-block entries while they are merged (8 + 4 bytes per entry, at most one per position
        // and side to move counted); the scan mode's value array; a group about 480 bytes, a slice 160
        std::uint64_t const transient = 9 * ((T + 63) / 64) * 8 + 12 * T + (g_frontier ? 0 : 2 * T);
        std::uint64_t const meta = 160 * m.slices + 480 * m.groups;
        std::uint64_t const writing = std::uint64_t(5) << 18;                          // the final pass's chunk buffers
        // the memory-mapped files count in the resident set as their pages are touched: the inputs (the materials the
        // moves lead into: the bytes and wdl files for dtm, the wdl files for dtc and dtz) and, at rung 1, the
        // group-order files of the material itself (read by the lookups into earlier groups and by the final pass)
        std::uint64_t inputs = 0;
        for (Material const s : successors_of(c)) {
                std::uint64_t const n = mat_index(s).size;
                inputs += 2 * ((n + 3) / 4) + (g_metric == Metric::dtm ? 2 * n : 0);
        }
        m.rung0 = FIXED_BYTES + meta + inputs + 4 * mi.size + std::max(transient, writing);
        m.rung1 = FIXED_BYTES + meta + inputs + 64 * m.slices + 2 * T + 2 * m.positions + std::max(transient, writing);
        return m;
}

bool build_material(std::string const& dir, Material c)
{
        if (!stored(c)) { std::cerr << c.name() << " is the mirror of a stored material" << std::endl; return false; }
        std::filesystem::create_directories(dir + "/" + c.tree());
        g_dir = dir;
        g_current = c;
        MatIndex const& mi = mat_index(c);
        auto const groups = groups_of(c);
        if (g_mem_mb != 0 || g_sink_auto) {             // the claim against the estimates, and the sink
                MemoryNeed const need = memory_need(c);
                std::uint64_t const r0 = to_mb(need.rung0), r1 = to_mb(need.rung1);
                if (g_sink_auto) g_group_files = !(g_mem_mb == 0 || r0 <= g_mem_mb);
                std::uint64_t const used = g_group_files ? r1 : r0;
                std::cout << "memory: claim " << g_mem_mb << " MB, rung 0 (array) " << r0 << " MB, rung 1 (group files) " << r1
                          << " MB: " << (g_group_files ? "group files" : "array");
                if (g_mem_mb != 0 && used > g_mem_mb) std::cout << " (over the claim: needs a lower rung, not implemented yet)";
                std::cout << std::endl;
        }
        open_successors(c);
        g_frozen = true;
        std::unique_ptr<DepthSink<std::uint16_t>> sink;
        if (g_group_files) sink = std::make_unique<GroupSink<std::uint16_t>>(dir, c, mi, groups);
        else sink = std::make_unique<ArraySink<std::uint16_t>>(mi);
        g_sink = sink.get();
        g_stats = {};
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (std::size_t k = 0; k < groups.size(); ++k) {
                sink->begin_group(k, groups[k]);
                solve_group(groups[k], *sink);
                sink->end_group();
                positions += 2 * groups[k].P;
        }

        // the four files, each written once, sequentially, in index order: the values gathered from the sink slot by
        // slot, as unsigned bytes and wdl bits
        std::uint64_t wins[2]{}, losses[2]{};
        int longest = 0;
        auto const tw = std::chrono::steady_clock::now();
        constexpr std::uint64_t chunk = std::uint64_t(1) << 20;         // slots per write, a multiple of 4
        for (int side : { 0, 1 }) {
                std::string const pb = mat_path(dir, c, BYTES + side), pw = mat_path(dir, c, WDL + side);
                std::ofstream fb(pb + ".tmp", std::ios::binary | std::ios::trunc), fw(pw + ".tmp", std::ios::binary | std::ios::trunc);
                std::vector<std::uint8_t> bytes, wdl;
                for (std::uint64_t lo = 0; lo < mi.size; lo += chunk) {
                        std::uint64_t const hi = std::min(mi.size, lo + chunk);
                        bytes.assign(hi - lo, 0);
                        wdl.assign((hi - lo + 3) / 4, 0);
                        for (std::uint64_t i = lo; i < hi; ++i) {
                                std::uint16_t const v = sink->at(side, i);
                                int const b = to_byte(g_metric, v);
                                if (b > 255) { std::cerr << "value beyond one byte" << std::endl; return false; }
                                bytes[i - lo] = std::uint8_t(b);
                                wdl[(i - lo) >> 2] |= std::uint8_t(wdl_of(v) << (2 * ((i - lo) & 3)));
                                wins[side] += is_win(v);
                                losses[side] += is_loss(v);
                                longest = std::max(longest, plies(v));
                        }
                        fb.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));
                        fw.write(reinterpret_cast<char const*>(wdl.data()), std::streamsize(wdl.size()));
                }
                if (!fb || !fw) { std::cerr << "write failed " << pb << std::endl; return false; }
                fb.close();
                fw.close();
                std::filesystem::rename(pb + ".tmp", pb);
                std::filesystem::rename(pw + ".tmp", pw);
        }
        g_sink = nullptr;
        g_frozen = false;
        g_stats.write = seconds_since(tw);
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "material " << c.name() << " " << metric_name() << (g_group_files ? " group files" : " array") << ": "
                  << groups.size() << " groups, " << positions << " positions, " << mi.size << " slots, wins " << wins[0] << " / "
                  << wins[1] << ", losses " << losses[0] << " / " << losses[1] << " (White / Black to move), longest " << longest
                  << " plies, peak RSS " << peak_rss_mb() << " MB, " << sec << " s" << std::endl;
        print_stats(c, positions, groups.size(), sec);
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

// thompson86 (one-sided, every material): d<m|c|z>W-wtm = the plies of a White-to-move win, d<m|c|z>L-btm = 1 + the
// plies of a Black-to-move loss; here both from the two-sided values, a material's mirror read as its mirror image
bool compare_thompson86(std::string const& dir, std::string const& dir86, int n)
{
        g_dir = dir;
        std::uint64_t checked = 0, bad = 0;
        for (Material const c : materials_up_to(n)) {
                MatIndex const& mi = mat_index(c);
                std::string const base = c.path(dir86) + "-d" + metric_name()[2];
                std::uint8_t const* w = map_file(base + "W-wtm", mi.size);
                std::uint8_t const* l = map_file(base + "L-btm", mi.size);
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
        g_granularity = Granularity::material;             // syzygy's default: the whole material class (dtz: by total tempo)
        std::vector<std::string> const a(argv + 1, argv + argc);
        auto mat_at = [&](std::size_t i) {                             // wp bp wm bm
                int const wp = std::stoi(a[i]), bp = std::stoi(a[i + 1]), wm = std::stoi(a[i + 2]), bm = std::stoi(a[i + 3]);
                if (wm < 0 || bm < 0 || wm > wp || bm > bp) { std::cerr << "not a material: wp bp wm bm" << std::endl; std::exit(2); }
                return Material{ wm, wp - wm, bm, bp - bm };
        };
        if ((a.size() == 2 || a.size() == 3) && a[0] == "bench") {   // what the rules cost per position
                run_bench(std::stoi(a[1]), a.size() == 3 ? std::stoi(a[2]) : 2000);
                return 0;
        }
        if ((a.size() == 3 || a.size() == 4) && a[0] == "plan" && parse_metric(a[2])) {
                if (a.size() == 4 && !parse_granularity(a[3])) { std::cerr << "granularity: material, tempi or configuration" << std::endl; return 2; }
                print_plan_header();
                for (Material const c : stored_materials_up_to(std::stoi(a[1]))) print_plan_row(c, memory_need(c));
                return 0;
        }
        if (a.size() >= 7 && a.size() <= 12 && a[0] == "build" && parse_metric(a[6])) {
                for (std::size_t i = 7; i < a.size(); ++i) {
                        if (a[i] == "seq" || a[i] == "par") g_par = a[i] == "par";
                        else if (a[i] == "scan" || a[i] == "frontier") g_frontier = a[i] == "frontier";
                        else if (a[i].starts_with("mem=")) g_mem_mb = std::stoull(a[i].substr(4));
                        else if (a[i] == "groups" || a[i] == "array") { g_group_files = a[i] == "groups"; g_sink_auto = false; }
                        else if (a[i] == "auto") g_sink_auto = true;
                        else if (parse_granularity(a[i])) {}
                        else { std::cerr << "unknown option " << a[i] << std::endl; return 2; }
                }
                return build_material(a[1], mat_at(2)) ? 0 : 1;
        }
        if ((a.size() == 4 || a.size() == 5) && a[0] == "checkall" && parse_metric(a[3]))
                return check_all(a[1], std::stoi(a[2]), a.size() == 5 ? std::stoi(a[4]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "cmp86" && parse_metric(a[4])) return compare_thompson86(a[1], a[2], std::stoi(a[3])) ? 0 : 1;
        if (a.size() == 5 && a[0] == "order") return check_order(a[1], a[2], a[3], std::stoi(a[4])) ? 0 : 1;
        std::cerr << "usage: syzygy build <dir> wp bp wm bm dtm|dtc|dtz [seq|par] [frontier|scan] [auto|groups|array] [material|tempi|configuration] [mem=MB] | bench n [samples] | plan n dtm|dtc|dtz [granularity] | checkall <dir> n dtm|dtc|dtz [threads]"
                     " | cmp86 <dir> <dir_thompson86> n dtm|dtc|dtz | order <dir_dtz> <dir_dtc> <dir_dtm> n" << std::endl;
        return 2;
}
