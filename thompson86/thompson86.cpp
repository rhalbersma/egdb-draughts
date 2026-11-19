//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Thompson style builds (1986 and 1996) for 6x6 international draughts, for three metrics. One-sided: half results,
// White's wins and Black's losses. Per material four files, 6x6-wp-bp-wm-bm.<side to move>.<kind> (for dtm; dtc and dtz
// alike):
//   wtm.dtm-W   one byte: k for a White win in 2k - 1 plies, 0 for "not a win" (draw, loss, illegal slot); dtc-W, dtz-W
//               store the plies n (their wins can take an even number of plies)
//   btm.dtm-L   one byte: k for Black lost in 2(k - 1) plies, 0 for "not lost" (a loss can take 0 plies: Black has no
//               move); dtc-L, dtz-L store n + 1; as Thompson 1996 kept the Black-to-move loss distances (RES)
//   wtm.bit-W   one bit: White wins (a by-product, the half-result outcome)
//   btm.bit-L   one bit: Black is lost
// The bit files are the same for all three metrics. After a goal move only the outcome counts, so a dtc or dtz build
// reads only the bit files of the materials that its moves out lead into (every move out of a material is a goal move
// there); dtm reads dtm-W and dtm-L.
// The goal:
//   dtm   the goal is the end of the game (Black has no move)
//   dtc   the end, or a capture or promotion (a conversion: another material) that keeps the win
//   dtz   the end, or a capture or man move (a zeroing move of the 25-move rule) that keeps the win
// A goal move by either side ends the count: a White win can end with Black's forced capture (an even number of plies).
//
// Materials. A material is named wp-bp-wm-bm (white pieces, black pieces, white men, black men, two digits each, as
// in egdb_6x6's material mode); every material with a piece on each side is built once, into files of its own,
// 6x6-wp-bp-wm-bm.wtm.<metric>-W and so on. Its index: per colour and type the colex rank over the squares where such a
// piece may stand (15 for men, 18 for kings),
//   index = ((rank_wm * C(15, bm) + rank_bm) * C(18, wp - wm) + rank_wk) * C(18, bp - bm) + rank_bk
// Slots where pieces of different colours or types share a square are never written (0). A move into another
// material (a capture, a promotion) is a lookup in that material's files at its own index: after a White move in btm.<metric>-L
// (or btm.bit-L), after a Black move in wtm.<metric>-W (or wtm.bit-W); the smaller
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
// W_d goes into wtm.<metric>-W, B_d into btm.<metric>-L: both are one-sided (White's wins, Black's losses), and every lookup is one read.
//
// Memory: the material's two result arrays (each written to disk once, sequentially, at the end) and bit planes per
// group. Disk: other materials are read through read-only memory maps (random reads); the only writes are sequential.
//
// build: g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include
//        -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o thompson86
// usage: thompson86 build <dir> wp bp wm bm <metric> <style> [groups|array|auto] [seq|par] [mem=MB]
//                                                               build one material (its captures and promotions must exist);
//                                                               the depth sink: per-group files (default) or the array;
//                                                               every pass a std::for_each, seq (default) or par;
//                                                               auto: the array if rung 0 fits the claim mem=MB, else
//                                                               the group files
//        thompson86 plan n <metric> <style>                     per material with at most n pieces: slots, positions,
//                                                               groups, the largest group, and the MB needed at rung 0
//                                                               (array) and rung 1 (group files), tab-separated
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

#include "../src/materials_6x6.hpp"

#include <filesystem>
#include <fstream>

namespace {

// kind 0: wtm.<metric>-W, 1: btm.<metric>-L, one byte per slot; 2: wtm.bit-W, 3: btm.bit-L, one bit per slot (won,
// lost); kind = side to move + 2 * bit. The bit files carry no metric: they are the same for all three
enum : int { BYTES = 0, BITS = 2 };

std::string mat_path(std::string const& dir, Material m, int kind)
{
        std::string const stm = kind % 2 == 0 ? ".wtm." : ".btm.", half = kind % 2 == 0 ? "-W" : "-L";
        return dir + "/6x6-" + m.name() + stm + (kind >= BITS ? std::string("bit") : std::string(metric_name())) + half;
}

std::uint64_t file_size(Material m, int kind) { return kind < BITS ? mat_index(m).size : (mat_index(m).size + 7) / 8; }

// ---------------------------------------------------------------- results: the material being built, and finished ones

std::string g_dir;
Material g_current{ -1, -1, -1, -1 };
std::map<std::pair<Material, int>, std::uint8_t const*> g_files;    // finished materials, memory-mapped read-only

bool g_frozen = false;                                  // during the passes of a build: the maps are only read

std::uint8_t const* mat_file(Material m, int kind)
{
        auto it = g_files.find({ m, kind });
        if (it != g_files.end()) return it->second;
        if (g_frozen) { std::cerr << "not opened before the passes: " << mat_path(g_dir, m, kind) << std::endl; std::abort(); }
        return g_files[{ m, kind }] = map_file(mat_path(g_dir, m, kind), file_size(m, kind));
}

// The stored bytes. A dtm win always takes an odd number of plies and a dtm loss an even number (no move ends the count
// early), so dtm stores moves; dtc and dtz store plies (a goal move by the loser can end a win on an even ply):
//   dtm-W   k: win in 2k - 1 plies        dtc-W, dtz-W   n: win in n plies
//   dtm-L   k: loss in 2(k - 1) plies     dtc-L, dtz-L   n + 1: loss in n plies
// 0: not a win, not lost. Exact plies either way; the build works in plies, and only the files are in these units.
int encode(Metric mt, int side, int plies)
{
        if (mt == Metric::dtm) return side == 0 ? (plies + 1) / 2 : plies / 2 + 1;
        return side == 0 ? plies : plies + 1;
}

int decode(Metric mt, int side, std::uint8_t s)         // the plies, -1 for 0 (not a win, not lost)
{
        if (s == 0) return -1;
        if (mt == Metric::dtm) return side == 0 ? 2 * s - 1 : 2 * (s - 1);
        return side == 0 ? s : s - 1;
}

std::uint8_t store_byte(int side, int plies)
{
        if (g_metric == Metric::dtm && (plies % 2 == 1) != (side == 0)) {
                std::cerr << "dtm: a win in an even or a loss in an odd number of plies" << std::endl;
                std::exit(1);
        }
        int const s = encode(g_metric, side, plies);
        if (s > 255) { std::cerr << "depth beyond one byte" << std::endl; std::exit(1); }
        return std::uint8_t(s);
}

DepthSink<std::uint8_t>* g_sink = nullptr;                                    // of the material being built

std::uint8_t mat_byte(Material m, int side, Pos const& p)       // the stored byte (dtm-W, dtm-L)
{
        if (m == g_current) return g_sink->read(side, p);
        return mat_file(m, BYTES + side)[mat_index(m).index(p)];
}

// side 0: White to move wins; side 1: Black to move is lost (one bit; the material being built: its bytes)
bool mat_bit(Material m, int side, Pos const& p)
{
        if (m == g_current) return g_sink->read(side, p) != 0;
        std::uint64_t const i = mat_index(m).index(p);
        return mat_file(m, BITS + side)[i >> 3] >> (i & 7) & 1;
}

// White to move: plies to the goal of a White win, 0 if not a win
int lookup_wtm(Pos const& p)
{
        if ((p.m[0] | p.k[0]).empty() || (p.m[1] | p.k[1]).empty()) return 0;   // a side without pieces: not a White win to look up
        Material const m = material_of(p);
        return std::max(0, decode(g_metric, 0, mat_byte(m, 0, p)));
}

// Black to move: plies to the goal of Black's loss, -1 if Black is not lost
int lookup_btm_lost(Pos const& p)
{
        if ((p.m[1] | p.k[1]).empty()) return 0;          // Black has no pieces: lost
        if ((p.m[0] | p.k[0]).empty()) return -1;         // White has no pieces: Black has won
        Material const m = material_of(p);
        return decode(g_metric, 1, mat_byte(m, 1, p));
}

// White to move: does White win (one bit)
bool wtm_won(Pos const& p)
{
        if ((p.m[0] | p.k[0]).empty() || (p.m[1] | p.k[1]).empty()) return false;
        Material const m = material_of(p);
        return mat_bit(m, 0, p);
}

// Black to move: is Black lost (one bit)
bool btm_lost(Pos const& p)
{
        if ((p.m[1] | p.k[1]).empty()) return true;
        if ((p.m[0] | p.k[0]).empty()) return false;
        Material const m = material_of(p);
        return mat_bit(m, 1, p);
}

// a White move: the plies to the goal of the win through it, -1 if it does not win; after a goal move only the outcome
// counts, so dtc and dtz read only the bit files of the materials that their moves out lead into
int white_move(Pos const& p, Move const& mv)
{
        if (goal_move(p, mv)) return btm_lost(mv.next) ? 1 : -1;
        int const j = lookup_btm_lost(mv.next);
        return j >= 0 ? 1 + j : -1;
}

// a Black move: the plies to the goal of Black's loss through it, 0 if White does not win after it
int black_move(Pos const& p, Move const& mv)
{
        if (goal_move(p, mv)) return wtm_won(mv.next) ? 1 : 0;
        int const v = lookup_wtm(mv.next);
        return v != 0 ? 1 + v : 0;
}

// the same, recomputed by one ply over White-to-move results
int btm_lost_from_successors(Pos const& p)
{
        if ((p.m[1] | p.k[1]).empty()) return 0;          // Black has no pieces: lost
        int lost = 0;
        for (auto const& mv : moves(p)) {                   // no move: lost now (0)
                int const v = black_move(p, mv);
                if (v == 0) return -1;
                lost = std::max(lost, v);
        }
        return lost;
}

// White to move, recomputed from the successors: the fastest goal over the moves, 0 if none wins
int wtm_from_successors(Pos const& p)
{
        int best = INF;
        for (auto const& mv : moves(p)) {
                int const j = white_move(p, mv);
                if (j >= 0) best = std::min(best, j);
        }
        return best < INF ? best : 0;
}

struct Seed { int level; std::uint32_t idx; };

// Every pass is one std::for_each over blocks (for_blocks, seq or par). Writes are race-free: in pass 0 a block owns
// the words of its positions; the pushes (P2, P3) and the seeds set bits with an atomic OR (idempotent); verification
// (P4) and the commit iterate whole words of the planes they write; the sink is written at distinct positions.
void solve_group(Group const& g, DepthSink<std::uint8_t>& sink)
{
        std::uint64_t const P = g.P;
        Plane open_w(P), open_b(P), lossable(P), exits_done(P), won(P), lost(P);
        std::uint64_t const nblocks = (P + 4095) / 4096;
        std::vector<std::vector<Seed>> win_b(nblocks), loss_b(nblocks), exit_b(nblocks);    // per block, merged after

        // pass 0: forward over every position, both sides to move; exits are looked up in finished groups and materials
        for_blocks(P, [&](std::uint64_t lo, std::uint64_t hi) {
                std::uint64_t const blk = lo / 4096;
                for (std::uint64_t i = lo; i < hi; ++i) {
                        Pos const p = g.pos(i, 0);
                        bool in = false;
                        int best = INF;
                        for (auto const& mv : moves(p)) {
                                if (inside(p, mv)) { in = true; continue; }
                                int const j = white_move(p, mv);
                                if (j >= 0) best = std::min(best, j);
                        }
                        if (in) open_w.set(i);
                        if (best < INF) win_b[blk].push_back({ best, std::uint32_t(i) });
                }
                for (std::uint64_t i = lo; i < hi; ++i) {
                        Pos const p = g.pos(i, 1);
                        auto const ml = moves(p);
                        if (ml.empty()) { loss_b[blk].push_back({ 0, std::uint32_t(i) }); continue; }
                        bool in = false, all_lose = true;
                        int slowest = 0;
                        for (auto const& mv : ml) {
                                if (inside(p, mv)) { in = true; continue; }
                                int const v = black_move(p, mv);
                                if (v == 0) all_lose = false;
                                else slowest = std::max(slowest, v);
                        }
                        if (!in) {                                      // every move leaves the group: decided
                                if (all_lose) loss_b[blk].push_back({ slowest, std::uint32_t(i) });
                                continue;
                        }
                        open_b.set(i);
                        if (!all_lose) continue;
                        lossable.set(i);
                        if (slowest == 0) exits_done.set(i);            // no exits
                        else exit_b[blk].push_back({ slowest, std::uint32_t(i) });
                }
        });
        auto const merge = [](std::vector<std::vector<Seed>>& parts) {  // in block order, then stably by level
                std::vector<Seed> out;
                for (auto& v : parts) { out.insert(out.end(), v.begin(), v.end()); std::vector<Seed>().swap(v); }
                std::ranges::stable_sort(out, {}, &Seed::level);
                return out;
        };
        std::vector<Seed> const win_seeds = merge(win_b), loss_decided = merge(loss_b), exit_seeds = merge(exit_b);
        int max_seed = 0;
        for (auto const* v : { &win_seeds, &loss_decided, &exit_seeds })
                if (!v->empty()) max_seed = std::max(max_seed, v->back().level);

        // the seeds of one level, as a std::for_each over their range
        auto const level_range = [](std::vector<Seed> const& v, std::size_t& pos, int level) {
                std::size_t const lo = pos;
                while (pos < v.size() && v[pos].level == level) ++pos;
                return std::pair{ lo, pos };
        };
        auto const for_seeds = [](std::vector<Seed> const& v, std::pair<std::size_t, std::size_t> r, auto&& f) {
                for_blocks(r.second - r.first, [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t j = lo; j < hi; ++j) f(v[r.first + j].idx);
                });
        };

        // rounds, level d in plies: W_d from B_{d-1}, B_d from W_{d-1}
        std::size_t ws = 0, ld = 0, xs = 0;
        Plane w_last(P), b_last(P), w_new(P), b_new(P), cand(P);
        for_seeds(loss_decided, level_range(loss_decided, ld, 0), [&](std::uint32_t q) {
                b_last.set_atomic(q);
                lost.set_atomic(q);
                sink.put(1, q, g.pos(q, 1), store_byte(1, 0));          // lost in 0 plies: no move
        });
        for (int d = 1;; ++d) {
                // P2: White wins in d
                w_new.clear();
                b_last.for_each_par([&](std::uint64_t q) {
                        for_each_inside_predecessor(g, g.pos(q, 1), [&](std::uint64_t id) {
                                if (!g_waves) { if (open_w.test(id) && !won.test(id)) w_new.set_atomic(id); }
                                else w_new.set_atomic(id);                              // 1996: mask later
                        });
                });
                if (g_waves) w_new.mask_par(open_w, false);                     // unmoves only reach open positions
                for_seeds(win_seeds, level_range(win_seeds, ws, d), [&](std::uint32_t q) { w_new.set_atomic(q); });
                w_new.mask_par(won, true);

                // P3: candidates for Black losses in d
                cand.clear();
                for_seeds(exit_seeds, level_range(exit_seeds, xs, d), [&](std::uint32_t q) {
                        exits_done.set_atomic(q);
                        cand.set_atomic(q);
                });
                w_last.for_each_par([&](std::uint64_t q) {
                        for_each_inside_predecessor(g, g.pos(q, 0), [&](std::uint64_t id) {
                                if (!g_waves) { if (open_b.test(id) && lossable.test(id) && !lost.test(id)) cand.set_atomic(id); }
                                else cand.set_atomic(id);                               // 1996: mask later
                        });
                });
                if (g_waves) { cand.mask_par(open_b, false); cand.mask_par(lossable, false); cand.mask_par(lost, true); }

                // P4: lost in d iff the exits are lost by d and every move inside leads into W (levels below d); each
                // candidate writes only its own bit, in a word its block owns
                b_new.clear();
                cand.for_each_par([&](std::uint64_t q) {
                        if (!exits_done.test(q) || lost.test(q)) return;
                        Pos const p = g.pos(q, 1);
                        for (auto const& mv : moves(p))
                                if (inside(p, mv) && !won.test(g.index(mv.next))) return;
                        b_new.set(q);
                });
                for_seeds(loss_decided, level_range(loss_decided, ld, d), [&](std::uint32_t q) { b_new.set_atomic(q); });

                // commit: whole words of won and lost, the sink at distinct positions
                w_new.for_each_par([&](std::uint64_t q) {
                        won.set(q);
                        sink.put(0, q, g.pos(q, 0), store_byte(0, d));
                });
                b_new.for_each_par([&](std::uint64_t q) {
                        lost.set(q);
                        sink.put(1, q, g.pos(q, 1), store_byte(1, d));
                });
                std::swap(w_last, w_new);
                std::swap(b_last, b_new);
                if (w_last.empty() && b_last.empty() && d >= max_seed) break;
        }
}

bool g_group_files = true;                                      // the depth sink: per-group files (false: the material array)
bool g_sink_auto = false;                                       // choose the sink by the memory claim
std::uint64_t g_mem_mb = 0;                                     // the job's memory claim in MB (0: none)

// the memory a build of c needs (the style and metric as set): rung 0 keeps the material's depth bytes in RAM (the array
// sink), rung 1 only the current group's (the group sink); both hold the largest group's bit planes and seed lists
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
        std::uint64_t const P = m.max_group;
        // calibrated on 6 pieces (peak RSS against these): the seed lists, with their copies while merging, measured
        // from about 4 to 21 bytes per position (8-byte entries, at most two per position, one list copied at a time:
        // 24 at most), counted at that bound; a group costs about 480 bytes and a slice 160 (Group, its offsets and
        // hash map); the group sink's slice map about 64 bytes per slice
        std::uint64_t const transient = 11 * ((P + 63) / 64) * 8 + 24 * P;     // 11 bit planes, seed lists
        std::uint64_t const meta = 160 * m.slices + 480 * m.groups;
        std::uint64_t const writing = std::uint64_t(5) << 18;                  // the final pass's chunk buffers
        m.rung0 = FIXED_BYTES + meta + 2 * mi.size + std::max(transient, writing);
        m.rung1 = FIXED_BYTES + meta + 64 * m.slices + 2 * P + std::max(transient, writing);
        return m;
}

// open every file a build of c can read, and fill the index memo, so that the parallel passes only read the maps: the
// bytes for dtm, the bits for dtc and dtz (every move out of a material is a goal move there)
void open_successors(Material c)
{
        mat_index(c);
        for (Material const m : successor_materials(c)) {
                if (m.wm + m.wk == 0 || m.bm + m.bk == 0) continue;    // a side without pieces: no file
                mat_index(m);
                for (int side : { 0, 1 }) mat_file(m, (g_metric == Metric::dtm ? BYTES : BITS) + side);
        }
}

bool build_material(std::string const& dir, Material c)
{
        g_dir = dir;
        g_current = c;
        MatIndex const& mi = mat_index(c);
        auto const groups = groups_of(c);
        if (g_mem_mb != 0 || g_sink_auto) {                     // the claim against the estimates, and the sink
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
        std::unique_ptr<DepthSink<std::uint8_t>> sink;
        if (g_group_files) sink = std::make_unique<GroupSink<std::uint8_t>>(dir, c, mi, groups);
        else sink = std::make_unique<ArraySink<std::uint8_t>>(mi);
        g_sink = sink.get();
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (std::size_t k = 0; k < groups.size(); ++k) {
                sink->begin_group(k, groups[k]);
                solve_group(groups[k], *sink);
                sink->end_group();
                positions += 2 * groups[k].P;
        }

        // the four files of the material, each written once, sequentially, in index order: the bytes gathered from the
        // sink slot by slot, the bits from the bytes
        std::uint64_t wins = 0, losses = 0;
        int longest = 0, longest_loss = 0;
        constexpr std::uint64_t chunk = std::uint64_t(1) << 20;         // slots per write, a multiple of 8
        for (int side : { 0, 1 }) {
                std::string const pb = mat_path(dir, c, BYTES + side), pt = mat_path(dir, c, BITS + side);
                std::ofstream fb(pb + ".tmp", std::ios::binary | std::ios::trunc), ft(pt + ".tmp", std::ios::binary | std::ios::trunc);
                std::vector<std::uint8_t> bytes, bits;
                for (std::uint64_t lo = 0; lo < mi.size; lo += chunk) {
                        std::uint64_t const hi = std::min(mi.size, lo + chunk);
                        bytes.assign(hi - lo, 0);
                        bits.assign((hi - lo + 7) / 8, 0);
                        for (std::uint64_t i = lo; i < hi; ++i) {
                                std::uint8_t const v = sink->at(side, i);
                                bytes[i - lo] = v;
                                bits[(i - lo) >> 3] |= std::uint8_t((v != 0) << ((i - lo) & 7));
                                if (v == 0) continue;
                                int const d = decode(g_metric, side, v);
                                if (side == 0) { ++wins; longest = std::max(longest, d); }
                                else { ++losses; longest_loss = std::max(longest_loss, d); }
                        }
                        fb.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));
                        ft.write(reinterpret_cast<char const*>(bits.data()), std::streamsize(bits.size()));
                }
                if (!fb || !ft) { std::cerr << "write failed " << pb << std::endl; return false; }
                fb.close();
                ft.close();
                std::filesystem::rename(pb + ".tmp", pb);
                std::filesystem::rename(pt + ".tmp", pt);
        }
        g_sink = nullptr;
        g_frozen = false;
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "material " << c.name() << " " << metric_name() << (g_waves ? " 1996" : " 1986")
                  << (g_group_files ? " group files" : " array") << ": " << groups.size() << " groups, " << positions
                  << " positions, " << mi.size << " slots, " << wins << " White wins, longest " << longest << " plies, "
                  << losses << " Black losses, longest " << longest_loss << " plies, peak RSS " << peak_rss_mb() << " MB, "
                  << sec << " s" << std::endl;
        return true;
}

// ---------------------------------------------------------------- checks

// dtm: every value equals egdb_6x6's DTM, both sides to move; dtc, dtz: the wins and losses equal egdb_6x6's, and every
// White-to-move value equals the value recomputed from its successors
bool check_all(std::string const& dir, int n, int threads)
{
        g_dir = dir;
        build(n, threads, n);                                   // egdb_6x6, once for every material
        bool ok = true;
        std::uint64_t total = 0;
        for (Material const c : materials_up_to(n)) {
                std::uint64_t checked = 0, bad_w = 0, bad_b = 0, bad_s = 0, bad_bit = 0;
                for (auto const& g : groups_of(c))
                        for (std::uint64_t i = 0; i < g.P; ++i) {
                                Pos const pw = g.pos(i, 0), pb = g.pos(i, 1);
                                std::uint16_t const v = lookup(pw), u = lookup(pb);
                                bool const win = v != DRAW && (v & 1), loss = u != DRAW && !(u & 1);
                                int const got = lookup_wtm(pw), got_b = lookup_btm_lost(pb);
                                bool const w_ok = g_metric == Metric::dtm ? got == (win ? int(v) : 0) : (got != 0) == win;
                                bool const b_ok = g_metric == Metric::dtm ? got_b == (loss ? int(u) : -1) : (got_b >= 0) == loss;
                                if (!w_ok && bad_w++ < 3) std::cerr << "W " << fen_of(pw) << " got " << got << " oracle " << show(v) << std::endl;
                                if (!b_ok && bad_b++ < 3) std::cerr << "B " << fen_of(pb) << " got " << got_b << " oracle " << show(u) << std::endl;
                                if (wtm_won(pw) != (got != 0) || btm_lost(pb) != (got_b >= 0)) ++bad_bit;
                                int const sv = wtm_from_successors(pw), sb = btm_lost_from_successors(pb);
                                if (sv != got && bad_s++ < 3) std::cerr << "S " << fen_of(pw) << " stored " << got << " from successors " << sv << std::endl;
                                if (sb != got_b && bad_s++ < 3) std::cerr << "S " << fen_of(pb) << " stored " << got_b << " from successors " << sb << std::endl;
                                checked += 2;
                        }
                total += checked;
                std::cout << "check " << c.name() << " " << metric_name() << ": " << checked << " positions, " << bad_w << " / "
                          << bad_b << " differences from egdb_6x6 (White / Black to move), " << bad_s
                          << " differences from the successors, " << bad_bit << " in the bit files" << std::endl;
                ok = ok && bad_w == 0 && bad_b == 0 && bad_s == 0 && bad_bit == 0;
        }
        std::cout << total << " positions; " << (ok ? "all materials check out" : "DIFFERENCES") << std::endl;
        return ok;
}

// dtz <= dtc <= dtm for every White-to-move win and every Black-to-move loss, the same wins and losses in all three, and
// the same bit files
bool check_order(std::string const& dz, std::string const& dc, std::string const& dm, int n)
{
        bool ok = true;
        for (int side : { 0, 1 }) {
                std::uint64_t count = 0, bad = 0, equal_zc = 0, equal_cm = 0, bad_bits = 0;
                for (Material const c : materials_up_to(n)) {
                        std::size_t const size = mat_index(c).size;
                        g_metric = Metric::dtz; auto const z = map_file(mat_path(dz, c, side), size);
                        g_metric = Metric::dtc; auto const x = map_file(mat_path(dc, c, side), size);
                        g_metric = Metric::dtm; auto const m = map_file(mat_path(dm, c, side), size);
                        for (std::size_t i = 0; i < size; ++i) {
                                int const zp = decode(Metric::dtz, side, z[i]), xp = decode(Metric::dtc, side, x[i]),
                                          mp = decode(Metric::dtm, side, m[i]);       // plies, -1 for none
                                if ((zp < 0) != (mp < 0) || (xp < 0) != (mp < 0)) { ++bad; continue; }
                                if (mp < 0) continue;
                                ++count;
                                if (!(zp <= xp && xp <= mp)) ++bad;
                                equal_zc += zp == xp;
                                equal_cm += xp == mp;
                        }
                        std::uint64_t const bs = file_size(c, BITS + side);
                        g_metric = Metric::dtz; auto const zb = map_file(mat_path(dz, c, BITS + side), bs);
                        g_metric = Metric::dtc; auto const xb = map_file(mat_path(dc, c, BITS + side), bs);
                        g_metric = Metric::dtm; auto const mb = map_file(mat_path(dm, c, BITS + side), bs);
                        bad_bits += !std::equal(zb, zb + bs, mb) || !std::equal(xb, xb + bs, mb);
                }
                std::cout << "order: " << count << (side == 0 ? " White-to-move wins, " : " Black-to-move losses, ") << bad
                          << " violations of dtz <= dtc <= dtm (or of the same set); dtz = dtc in " << equal_zc << ", dtc = dtm in "
                          << equal_cm << "; " << bad_bits << " bit files that differ" << std::endl;
                ok = ok && bad == 0 && bad_bits == 0;
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
        if (a.size() == 4 && a[0] == "plan" && parse_metric(a[2]) && (a[3] == "86" || a[3] == "96")) {
                g_waves = a[3] == "96";
                print_plan_header();
                for (Material const c : materials_up_to(std::stoi(a[1]))) print_plan_row(c, memory_need(c));
                return 0;
        }
        if (a.size() >= 8 && a.size() <= 11 && a[0] == "build" && parse_metric(a[6]) && (a[7] == "86" || a[7] == "96")) {
                g_waves = a[7] == "96";
                for (std::size_t i = 8; i < a.size(); ++i) {
                        if (a[i] == "groups" || a[i] == "array") { g_group_files = a[i] == "groups"; g_sink_auto = false; }
                        else if (a[i] == "auto") g_sink_auto = true;
                        else if (a[i] == "seq" || a[i] == "par") g_par = a[i] == "par";
                        else if (a[i].starts_with("mem=")) g_mem_mb = std::stoull(a[i].substr(4));
                        else { std::cerr << "unknown option " << a[i] << std::endl; return 2; }
                }
                return build_material(a[1], mat_at(2)) ? 0 : 1;
        }
        if ((a.size() == 4 || a.size() == 5) && a[0] == "checkall" && parse_metric(a[3]))
                return check_all(a[1], std::stoi(a[2]), a.size() == 5 ? std::stoi(a[4]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "order") return check_order(a[1], a[2], a[3], std::stoi(a[4])) ? 0 : 1;
        if (a.size() == 5 && a[0] == "size") { std::cout << MatIndex(mat_at(1)).size << std::endl; return 0; }
        std::cerr << "usage: thompson86 build <dir> wp bp wm bm dtm|dtc|dtz 86|96 [groups|array|auto] [seq|par] [mem=MB] | plan n dtm|dtc|dtz 86|96 | checkall <dir> n dtm|dtc|dtz [threads]"
                     " | order <dir_dtz> <dir_dtc> <dir_dtm> n | size wp bp wm bm" << std::endl;
        return 2;
}
