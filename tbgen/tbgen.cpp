//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// tbgen: Syzygy style builds (after R. de Man's rtbgen, the "regular" chess tablebase generator) for draughts, two-sided,
// for three metrics, with the per-material index of thompson86 (src/materials_6x6.hpp). As rtbgen's letter names its
// chess variant, the first letter names the rules: itbgen for international draughts, ctbgen for English checkers
// (-DEGDB_6X6_CHECKERS=1). Built for one board (see src/egdb_6x6.cpp: EGDB_WIDTH, EGDB_HEIGHT; the CMake targets itbgen
// and ctbgen on 6x6, itbgen_10x10, ctbgen_8x8), which names the top of the tree (6x6/, 8x8/, 10x10/) and which
// `tbgen board` prints. Below, `tbgen` stands for either program. The value of
// a position, for the side to move:
//   0          draw (also an illegal slot)
//   2n         win in n plies (n >= 1)
//   2n + 1     loss in n plies (n >= 0; 0: no move)
// 16 bits during the build; stored as one byte (see to_depth; escape tokens beyond, see g_esc): dtm self-contained (plies, the class from their parity),
// dtc and dtz unsigned with the sign from the wdl file. The metric decides which moves end the count (as in thompson86):
//   dtm   none: the count runs to the end of the game
//   dtc   captures and promotions (a conversion into another material)
//   dtz   captures and man moves (the zeroing moves of the 25-move rule)
// After a goal move only the outcome of the position it leads to counts (win in 1 ply, or loss in 1 ply).
//
// Materials. Two-sided: both sides to move, wins and losses, so only one of each pair of a material and its colour
// mirror is built (the same 82 materials up to 6 pieces as strongly_solving_6x6/materials); a position in the other
// one is looked up as its mirror image. Full results; per material four files, 6x6/np/wp-bp/wm-bm-<kind>-<side to move>
// (for dtm; dtc and dtz alike):
//   dtm-wtm   one byte per slot, White to move: 1 + the plies (odd plies a win, even a loss; 0 a draw), as the cells
//             of cell8; dtc, dtz: the plies, unsigned, the sign in wdl-wtm
//   dtm-btm   one byte per slot, Black to move
//   wdl-wtm   two bits per slot (0 draw, 1 loss, 2 win), White to move
//   wdl-btm   two bits per slot, Black to move
// The wdl files are a by-product: the sign of every value, the same for all three metrics. They are all that dtc and
// dtz need from the materials that a move leads into (every move out of a material is a goal move there), so their
// builds read only the wdl files of their captures and promotions; dtm needs the depths and reads only dtm-wtm,
// dtm-btm, whose bytes carry their class.
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
//               positions decided in pass 0 and of the exit seeds: a round visits only these; the unmoves mark loss candidates in
//               a bit plane, and each candidate is checked once (rtbgen's delayed check; EGDB_FRONTIER_INLINE: at every
//               unmove)
//   cell8       one byte per position, the statuses inside, scans, reductions (solve_group_cell8)
//   scan        as rtbgen: every round scans the whole byte array for the values of the previous ply
// Within a round the losses read only wins of earlier rounds, and the wins only losses of earlier rounds, so each
// half can run in parallel (std::for_each, seq or par, over blocks of positions; every writer writes the same value).
// Rule 6: the group's arrays are in RAM; finished materials are read through read-only memory maps; each file is
// written once, sequentially.
//
// build: g++-14 -std=c++23 -O2 -march=native -fopenmp -I<xstd-bits>/include -I<xstd-ints>/include
//        -I<xstd-misc>/include -I<tabula>/include tbgen/tbgen.cpp -lzstd -ltbb -o itbgen
// usage: tbgen build <dir> wp bp wm bm <metric> [seq|par] [frontier|scan|cell8] [auto|groups|bytes|array] [compressed|raw] [stats] [material|tempi|configuration] [mem=MB]
//                                                            build one material (its captures and promotions must exist);
//                                                            the depth sink by the claim mem=MB (auto: the array if rung 0
//                                                            fits, else group files), or as given; the granularity of
//                                                            the groups (default material; below it man moves are
//                                                            exits)
//        tbgen board                                        the board and rules of this binary (e.g. "8x8 checkers")
//        tbgen verify <dir> wp bp wm bm <metric> [seq|par]  one material as egdb_6x6's matverify: every slot of both
//                                                            sides from its files against its successors, and its wdl
//                                                            files against the values
//        tbgen plan n <metric> [granularity]                per stored material with at most n pieces: slots,
//                                                            positions, groups, the largest group, MB needed (rung 0)
//        tbgen checkall <dir> n <metric> [threads]          every material of at most n pieces against egdb_6x6
//                                                            (built in memory): dtm exactly, dtc / dtz their wins and
//                                                            losses; every value against its successors; the wdl files
//                                                            against the values
//        tbgen cmp86 <dir> <dir_thompson86> n <metric>      the same half results (dmW, dmL) as thompson86, for every
//                                                            material of at most n pieces, both colours
//        tbgen probe <dir> <metric> [FEN...]                the value of each position (default: the start position)
//                                                            and of each of its moves, from the probe files if the store
//                                                            has them, else from the build files
//        tbgen convert <dir> wp bp wm bm <metric> [seq|par] [verify=N]
//                                                            one material's build files into the probe format (one side,
//                                                            capture positions and draws as don't-cares), checked through
//                                                            the prober on every N-th slot (default 16)
//        tbgen order <dir> n                                dtz <= dtc <= dtm for every win and loss, all with the class
//                                                            in the shared wdl files
// metric: dtm, dtc, dtz. <dir>: the base directory; depth files in <dir>/<metric>/6x6/..., wdl files in <dir>/wdl/6x6/...,
// written by whichever metric is built first (a later build of another metric compares its wdl with them)

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
constexpr std::uint16_t win_value(int n) { return win(n); }    // where a local named win (a window) hides win()
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
// one of each pair of a material and its colour mirror (every material on a board without the colour mirror)
bool stored(Material m) { return !HAS_MIRROR || !(std::tuple(m.bm, m.wm, m.bk, m.wk) < std::tuple(m.wm, m.bm, m.wk, m.bk)); }

// kind 0: <metric>-wtm, 1: <metric>-btm (one byte per slot); 2: wdl-wtm, 3: wdl-btm (two bits per slot); kind = side to
// move + 2 * wdl. The wdl files carry no metric: they are the same for all three
enum : int { BYTES = 0, WDL = 2 };

std::string mat_path(std::string const& dir, Material m, int kind)
{
        std::string const sub = kind >= WDL ? std::string("wdl") : std::string(metric_name());   // <dir>/wdl, shared by the metrics
        return m.path(dir + "/" + sub) + "-" + sub + (kind % 2 == 0 ? "-wtm" : "-btm");
}

std::uint64_t file_size(Material m, int kind) { return kind < WDL ? mat_index(m).size : (mat_index(m).size + 3) / 4; }

std::string g_dir;
Material g_current{ -1, -1, -1, -1 };
DepthSink<std::uint16_t>* g_sink = nullptr;             // the material being built: its values (src/materials_6x6.hpp)

// Compressed build-format files (the default; the build option `raw` keeps plain files): a material's four files are
// written as <name>.z, the same bytes in zstd blocks of 4 KiB, and every lookup into a finished material goes through a
// per-thread direct-mapped cache of decompressed blocks (EGDB_CACHE_BLOCKS per thread, default 16384: 64 MiB). Only
// pass 0 looks into other materials. Format: "EGDBZ1\0\0", the raw size, the block size, the number of blocks n, n + 1
// offsets of the blocks from the start of the data, the blocks.
bool g_compressed = true;
constexpr std::uint64_t ZBLOCK = 4096;
constexpr char ZMAGIC[8] = { 'E', 'G', 'D', 'B', 'Z', '2', 0, 0 };

// Depths beyond one byte (escape tokens, synthesis 10). A depth file holds a depth index d per position (dtm: 1 + plies,
// 0 a draw; dtc, dtz: plies). d below E is its own byte; else the byte is the token E + k with one extra byte x, and
// d = E + 240 k + x. E is 241 (EGDB_DIRECT=k forces a smaller one, to test the tokens on small builds), so 15 tokens
// reach d = 3840. A compressed file appends the extra bytes of each block after its position bytes, in position order,
// and records E in its header; a raw file keeps them in a side file <name>.x (E, then the slot and d of each).
constexpr int ZRANGE = 240;
int const g_esc = std::getenv("EGDB_DIRECT") ? std::clamp(std::atoi(std::getenv("EGDB_DIRECT")), 1, 255) : 241;
int max_depth_index(int esc) { return esc + ZRANGE * (255 - esc) + ZRANGE - 1; }

struct ZFile { std::uint64_t size = 0, nblocks = 0; int esc = 256; std::uint64_t const* off = nullptr; std::uint8_t const* data = nullptr; };

std::string zpath(std::string const& path) { return path + ".z"; }
std::string xpath(std::string const& path) { return path + ".x"; }

ZFile open_zfile(std::string const& path, std::uint64_t size)
{
        std::uintmax_t const fs = std::filesystem::exists(zpath(path)) ? std::filesystem::file_size(zpath(path)) : 0;
        if (fs < 40) { std::cerr << "missing or short " << zpath(path) << std::endl; std::exit(1); }
        std::uint8_t const* const p = map_file(zpath(path), std::size_t(fs));
        ZFile z;
        std::uint64_t h[5];
        std::memcpy(h, p, sizeof h);
        if (std::memcmp(p, ZMAGIC, 8) != 0 || h[1] != size || h[2] != ZBLOCK) { std::cerr << "bad header " << zpath(path) << std::endl; std::exit(1); }
        z.size = h[1];
        z.nblocks = h[3];
        z.esc = int(h[4]);
        z.off = reinterpret_cast<std::uint64_t const*>(p + 40);
        z.data = p + 40 + 8 * (z.nblocks + 1);
        return z;
}

using Extras = std::vector<std::pair<std::uint64_t, std::uint8_t>>;     // (slot, extra byte), sorted by slot

// zstd's levels: the build files 3 (written on the critical path, read by later builds, deleted after the conversion;
// EGDB_ZLEVEL), the probe files 19 (written once, read for good; EGDB_PROBE_ZLEVEL). Decompression barely depends on it
int const g_build_level = std::getenv("EGDB_ZLEVEL") ? std::atoi(std::getenv("EGDB_ZLEVEL")) : 3;
int const g_probe_level = std::getenv("EGDB_PROBE_ZLEVEL") ? std::atoi(std::getenv("EGDB_PROBE_ZLEVEL")) : 19;

// bytes in memory, written as <path>.z (compressed in parallel over the blocks, written once, sequentially); extras: the
// extra bytes of the escape tokens, appended to their blocks (esc: E, 256 for a file without tokens); level: zstd's
template<class ExecutionPolicy>
void compress_buffer(std::uint8_t const* raw, std::uint64_t size, std::string const& path, ExecutionPolicy const& policy,
                     Extras const* extras = nullptr, int esc = 256, int level = g_build_level)
{
        std::uint64_t const nb = (size + ZBLOCK - 1) / ZBLOCK;
        std::vector<std::vector<char>> parts(nb);
        std::vector<std::uint64_t> idx(nb);
        std::iota(idx.begin(), idx.end(), std::uint64_t(0));
        std::for_each(policy, idx.begin(), idx.end(), [&](std::uint64_t b) {
                std::uint64_t const lo = b * ZBLOCK, n = std::min(ZBLOCK, size - lo);
                std::vector<std::uint8_t> blk(raw + lo, raw + lo + n);
                if (extras) {
                        auto it = std::ranges::lower_bound(*extras, lo, {}, &Extras::value_type::first);
                        for (; it != extras->end() && it->first < lo + n; ++it) blk.push_back(it->second);
                }
                parts[b].resize(ZSTD_compressBound(blk.size()));
                std::size_t const r = ZSTD_compress(parts[b].data(), parts[b].size(), blk.data(), blk.size(), level);
                if (ZSTD_isError(r)) { std::cerr << "zstd: " << ZSTD_getErrorName(r) << std::endl; std::exit(1); }
                parts[b].resize(r);
        });
        std::vector<std::uint64_t> off(nb + 1, 0);
        for (std::uint64_t b = 0; b < nb; ++b) off[b + 1] = off[b] + parts[b].size();
        std::ofstream f(zpath(path) + ".tmp", std::ios::binary | std::ios::trunc);
        std::uint64_t const h[4] = { size, ZBLOCK, nb, std::uint64_t(esc) };
        f.write(ZMAGIC, 8);
        f.write(reinterpret_cast<char const*>(h), sizeof h);
        f.write(reinterpret_cast<char const*>(off.data()), std::streamsize(8 * off.size()));
        for (auto const& b : parts) f.write(b.data(), std::streamsize(b.size()));
        if (!f) { std::cerr << "write failed " << zpath(path) << std::endl; std::exit(1); }
        f.close();
        std::filesystem::rename(zpath(path) + ".tmp", zpath(path));
}

// a raw file, compressed into <path>.z, then removed (files without escape tokens: the wdl files, or the array sink's)
template<class ExecutionPolicy>
void compress_file(std::string const& path, std::uint64_t size, ExecutionPolicy const& policy)
{
        std::uint8_t const* const raw = map_file(path, size);
        compress_buffer(raw, size, path, policy);
        ::munmap(const_cast<std::uint8_t*>(raw), size);
        std::filesystem::remove(path);
}

// one decompressed block (its position bytes, then the extra bytes of its tokens), through the calling thread's cache
std::uint8_t const* zblock(ZFile const& z, std::uint64_t b)
{
        struct Tag { ZFile const* f = nullptr; std::uint64_t block = ~std::uint64_t(0); };
        static std::size_t const nslots = std::getenv("EGDB_CACHE_BLOCKS") ? std::size_t(std::atoll(std::getenv("EGDB_CACHE_BLOCKS"))) : 8192;
        thread_local std::vector<Tag> tags(nslots);                                    // the data uninitialised: pages are
        thread_local std::unique_ptr<std::uint8_t[]> blocks(new std::uint8_t[nslots * 2 * ZBLOCK]);   // touched only when used
        thread_local std::unique_ptr<ZSTD_DCtx, std::size_t (*)(ZSTD_DCtx*)> dx(ZSTD_createDCtx(), ZSTD_freeDCtx);
        std::size_t const h = std::size_t((std::uintptr_t(&z) >> 4) * 0x9E3779B97F4A7C15ull ^ b * 0xC2B2AE3D27D4EB4Full) % nslots;
        Tag& t = tags[h];
        std::uint8_t* const data = blocks.get() + h * 2 * ZBLOCK;
        if (t.f != &z || t.block != b) {
                std::size_t const r = ZSTD_decompressDCtx(dx.get(), data, 2 * ZBLOCK, z.data + z.off[b], z.off[b + 1] - z.off[b]);
                if (ZSTD_isError(r)) { std::cerr << "zstd: bad block " << b << std::endl; std::exit(1); }
                t.f = &z;
                t.block = b;
        }
        return data;
}

std::uint8_t zbyte(ZFile const& z, std::uint64_t pos) { return zblock(z, pos / ZBLOCK)[pos % ZBLOCK]; }

// the depth index of a position in a decompressed block of n positions: a token's extra byte follows the block's
// position bytes, as many places in as there are tokens before it
int block_depth(std::uint8_t const* blk, std::uint64_t n, std::uint64_t j, int esc)
{
        std::uint8_t const c = blk[j];
        if (c < esc) return c;
        std::uint64_t k = 0;
        for (std::uint64_t q = 0; q < j; ++q) k += blk[q] >= esc;
        return esc + ZRANGE * (c - esc) + blk[n + k];
}

int zdepth(ZFile const& z, std::uint64_t pos)
{
        std::uint64_t const b = pos / ZBLOCK, lo = b * ZBLOCK;
        return block_depth(zblock(z, b), std::min(ZBLOCK, z.size - lo), pos - lo, z.esc);
}

bool g_frozen = false;                                  // during the passes of a build: the maps are only read

// a raw file's escape tokens, from its side file <name>.x (none if it does not exist)
struct RawExtras { int esc = 256; std::unordered_map<std::uint64_t, std::uint16_t> d; };
RawExtras load_xfile(std::string const& path)
{
        RawExtras x;
        std::ifstream f(xpath(path), std::ios::binary);
        if (!f) return x;
        std::uint64_t h[2];
        f.read(reinterpret_cast<char*>(h), sizeof h);
        x.esc = int(h[0]);
        for (std::uint64_t i = 0; i < h[1]; ++i) {
                std::uint64_t slot;
                std::uint16_t d;
                f.read(reinterpret_cast<char*>(&slot), 8);
                f.read(reinterpret_cast<char*>(&d), 2);
                x.d[slot] = d;
        }
        return x;
}

// a file of a finished material, opened once: compressed if <name>.z exists, else raw (so either kind of store is read)
struct Input { std::uint8_t const* raw = nullptr; ZFile z; RawExtras x; };
std::map<std::pair<Material, int>, Input> g_inputs;

Input const& open_input(Material m, int kind)
{
        auto it = g_inputs.find({ m, kind });
        if (it != g_inputs.end()) return it->second;
        if (g_frozen) { std::cerr << "not opened before the passes: " << mat_path(g_dir, m, kind) << std::endl; std::abort(); }
        std::string const path = mat_path(g_dir, m, kind);
        Input in;
        if (std::filesystem::exists(zpath(path))) in.z = open_zfile(path, file_size(m, kind));
        else {
                in.raw = map_file(path, file_size(m, kind));
                if (kind < WDL) in.x = load_xfile(path);
        }
        return g_inputs[{ m, kind }] = std::move(in);
}

// one byte of a wdl file of a finished material
std::uint8_t input_byte(Material m, int kind, std::uint64_t pos)
{
        Input const& in = open_input(m, kind);
        return in.raw ? in.raw[pos] : zbyte(in.z, pos);
}

// the depth index of a position in a depth file of a finished material
int input_depth(Material m, int kind, std::uint64_t pos)
{
        Input const& in = open_input(m, kind);
        if (!in.raw) return zdepth(in.z, pos);
        std::uint8_t const c = in.raw[pos];
        return c < in.x.esc ? c : in.x.d.at(pos);
}

// a wdl file already written by the build of another metric (raw or compressed)
bool wdl_exists(std::string const& path) { return std::filesystem::exists(path) || std::filesystem::exists(zpath(path)); }

// a whole wdl file of a finished material, for the checks and comparisons (no tokens)
std::vector<std::uint8_t> load_input(std::string const& path, std::uint64_t size)
{
        std::vector<std::uint8_t> out(size);
        if (std::filesystem::exists(zpath(path))) {            // every block decompressed in place (not through the cache:
                ZFile const z = open_zfile(path, size);         // a local ZFile has no stable address to key it by)
                std::vector<std::uint8_t> blk(2 * ZBLOCK);
                for (std::uint64_t b = 0; b < z.nblocks; ++b) {
                        std::uint64_t const lo = b * ZBLOCK, n = std::min(ZBLOCK, size - lo);
                        std::size_t const r = ZSTD_decompress(blk.data(), blk.size(), z.data + z.off[b], z.off[b + 1] - z.off[b]);
                        if (ZSTD_isError(r) || r < n) { std::cerr << "zstd: bad block " << b << " of " << zpath(path) << std::endl; std::exit(1); }
                        std::copy(blk.begin(), blk.begin() + std::ptrdiff_t(n), out.begin() + std::ptrdiff_t(lo));
                }
        } else {
                std::uint8_t const* const p = map_file(path, size);
                std::copy(p, p + size, out.begin());
        }
        return out;
}

// a whole depth file of a finished material: the depth index of every slot, tokens resolved
std::vector<std::uint16_t> load_depths(std::string const& path, std::uint64_t size)
{
        std::vector<std::uint16_t> out(size);
        if (std::filesystem::exists(zpath(path))) {
                ZFile const z = open_zfile(path, size);
                std::vector<std::uint8_t> blk(2 * ZBLOCK);
                for (std::uint64_t b = 0; b < z.nblocks; ++b) {
                        std::uint64_t const lo = b * ZBLOCK, n = std::min(ZBLOCK, size - lo);
                        std::size_t const r = ZSTD_decompress(blk.data(), blk.size(), z.data + z.off[b], z.off[b + 1] - z.off[b]);
                        if (ZSTD_isError(r) || r < n) { std::cerr << "zstd: bad block " << b << " of " << zpath(path) << std::endl; std::exit(1); }
                        std::uint64_t k = 0;                    // tokens so far in the block
                        for (std::uint64_t j = 0; j < n; ++j) {
                                std::uint8_t const c = blk[j];
                                out[lo + j] = std::uint16_t(c < z.esc ? c : z.esc + ZRANGE * (c - z.esc) + blk[n + k++]);
                        }
                }
        } else {
                std::uint8_t const* const p = map_file(path, size);
                RawExtras const x = load_xfile(path);
                for (std::uint64_t i = 0; i < size; ++i) out[i] = p[i] < x.esc ? p[i] : x.d.at(i);
        }
        return out;
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
                for (int kind : g_metric == Metric::dtm ? std::vector<int>{ BYTES, BYTES + 1 } : std::vector<int>{ WDL, WDL + 1 })
                        open_input(m, kind);                    // dtm: the depths with their signs; dtc, dtz: the signs
        }
}

// a position in a stored material: itself or its mirror image (the value for the side to move is the same)
Pos stored_pos(Pos const& p) { return stored(material_of(p)) ? p : mirror(p); }

// The stored depth index d of a value, exact plies either way (the build works in plies: 2n win, 2n + 1 loss):
//   dtm        1 + n for a win or a loss in n plies, 0 a draw. A dtm win always takes an odd number of plies and a loss an
//              even number, so d carries its class: a dtm build reads no wdl file (as the cells of cell8, synthesis 7.5).
//   dtc, dtz   n for a win or a loss in n plies, 0 a draw; the sign from the wdl file of the same side to move (their wins
//              and losses can take the same number of plies).
// d below E is one byte; above, an escape token and an extra byte (see g_esc).
int to_depth(Metric mt, std::uint16_t v)
{
        if (v == 0) return 0;
        if (mt != Metric::dtm) return plies(v);
        if ((plies(v) % 2 == 1) != is_win(v)) { std::cerr << "dtm: a win in an even or a loss in an odd number of plies" << std::endl; std::exit(1); }
        return 1 + plies(v);
}

std::uint16_t from_depth(Metric mt, int d, std::uint8_t r)    // r: the wdl of the same position (dtc, dtz)
{
        if (mt == Metric::dtm) return d == 0 ? std::uint16_t(0) : (d - 1) % 2 == 1 ? win(d - 1) : loss(d - 1);
        if (r == WDL_DRAW) return 0;
        return r == WDL_WIN ? win(d) : loss(d);
}

std::uint8_t wdl_bits(std::uint8_t const* w, std::uint64_t i) { return std::uint8_t(w[i >> 2] >> (2 * (i & 3)) & 3); }

// The 8-bit sink: the four output files in their stored form (<metric>-wtm, <metric>-btm, wdl-wtm, wdl-btm), held in
// anonymous memory for the whole build of the material (zero pages until touched) and written once, sequentially, at the
// end: compressed (<name>.z) or raw. No random writes ever reach the disk. A decided value goes straight into its stored
// form, the depth byte (an escape token for a depth beyond the direct range, its extra byte kept aside) and two wdl
// bits, at its slot of the material; a lookup into an earlier group decodes it again. 2.5 bytes per slot for both sides
// to move, and no conversion pass at the end. Concurrent writers: the depth bytes are distinct per position; four slots
// share a wdl byte, so the wdl bits are set with an atomic or; the rare extra bytes go into a map under a mutex.
class ByteSink final : public DepthSink<std::uint16_t>
{
        MatIndex const& mi_;
        std::string path_[4];
        std::uint8_t* map_[4]{};
        std::size_t size_[4]{};
        std::atomic<bool> beyond_{ false };
        mutable std::mutex mx_;
        std::unordered_map<std::uint64_t, std::uint8_t> extra_[2];

        std::uint16_t value(int side, std::uint64_t i) const
        {
                std::uint8_t const b = std::atomic_ref<std::uint8_t>(map_[side][i]).load(std::memory_order_relaxed);
                std::uint8_t const w = std::atomic_ref<std::uint8_t>(map_[WDL + side][i >> 2]).load(std::memory_order_relaxed);
                int d = b;
                if (b >= g_esc) {
                        std::lock_guard lock(mx_);
                        d = g_esc + ZRANGE * (b - g_esc) + extra_[side].at(i);
                }
                return from_depth(g_metric, d, std::uint8_t(w >> (2 * (i & 3)) & 3));
        }

public:
        ByteSink(std::string const& dir, Material c, MatIndex const& mi) : mi_(mi)
        {
                for (int kind = 0; kind < 4; ++kind) {
                        path_[kind] = mat_path(dir, c, kind);
                        size_[kind] = file_size(c, kind);
                        void* const p = ::mmap(nullptr, size_[kind], PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                        if (p == MAP_FAILED) { std::cerr << "bytes sink: cannot allocate " << size_[kind] << " bytes" << std::endl; std::exit(1); }
                        map_[kind] = static_cast<std::uint8_t*>(p);
                }
        }
        ~ByteSink() override
        {
                for (int kind = 0; kind < 4; ++kind)
                        if (map_[kind]) ::munmap(map_[kind], size_[kind]);
        }
        void begin_group(std::size_t, Group const&) override {}
        void end_group() override {}
        void put(int side, std::uint64_t, Pos const& p, std::uint16_t v) override
        {
                std::uint64_t const i = mi_.index(p);
                int const d = to_depth(g_metric, v);
                std::uint8_t b = std::uint8_t(d);
                if (d >= g_esc) {                                               // an escape token and its extra byte
                        if (d > max_depth_index(g_esc)) { beyond_.store(true, std::memory_order_relaxed); return; }
                        b = std::uint8_t(g_esc + (d - g_esc) / ZRANGE);
                        std::lock_guard lock(mx_);
                        extra_[side][i] = std::uint8_t((d - g_esc) % ZRANGE);
                }
                std::atomic_ref<std::uint8_t>(map_[side][i]).store(b, std::memory_order_relaxed);
                std::atomic_ref<std::uint8_t>(map_[WDL + side][i >> 2]).fetch_or(std::uint8_t(wdl_of(v) << (2 * (i & 3))), std::memory_order_relaxed);
        }
        std::uint16_t read(int side, Pos const& p) const override { return value(side, mi_.index(p)); }
        std::uint16_t at(int side, std::uint64_t slot) const override { return value(side, slot); }
        bool beyond() const { return beyond_.load(); }
        std::uint64_t tokens() const { return extra_[0].size() + extra_[1].size(); }
        template<class ExecutionPolicy>
        void finish(ExecutionPolicy const& policy)      // the four files, each written once, sequentially
        {
                for (int kind = 0; kind < 4; ++kind) {
                        if (kind >= WDL && wdl_exists(path_[kind])) {          // built by another metric: compare, keep
                                if (load_input(path_[kind], size_[kind]) != std::vector<std::uint8_t>(map_[kind], map_[kind] + size_[kind])) {
                                        std::cerr << "wdl differs from the existing " << path_[kind] << std::endl; std::exit(1);
                                }
                        } else {
                                Extras ex;
                                if (kind < WDL) {
                                        ex.assign(extra_[kind].begin(), extra_[kind].end());
                                        std::ranges::sort(ex);
                                }
                                if (g_compressed) compress_buffer(map_[kind], size_[kind], path_[kind], policy, kind < WDL ? &ex : nullptr, kind < WDL ? g_esc : 256);
                                else {
                                        std::ofstream f(path_[kind] + ".tmp", std::ios::binary | std::ios::trunc);
                                        f.write(reinterpret_cast<char const*>(map_[kind]), std::streamsize(size_[kind]));
                                        if (!f) { std::cerr << "write failed " << path_[kind] << std::endl; std::exit(1); }
                                        f.close();
                                        std::filesystem::rename(path_[kind] + ".tmp", path_[kind]);
                                        if (!ex.empty()) {                      // the side file of the tokens: E, n, (slot, d)
                                                std::ofstream x(xpath(path_[kind]), std::ios::binary | std::ios::trunc);
                                                std::uint64_t const h[2] = { std::uint64_t(g_esc), ex.size() };
                                                x.write(reinterpret_cast<char const*>(h), sizeof h);
                                                for (auto const& [slot, e] : ex) {
                                                        std::uint16_t const d = std::uint16_t(g_esc + ZRANGE * (map_[kind][slot] - g_esc) + e);
                                                        x.write(reinterpret_cast<char const*>(&slot), 8);
                                                        x.write(reinterpret_cast<char const*>(&d), 2);
                                                }
                                        }
                                }
                        }
                        ::munmap(map_[kind], size_[kind]);
                        map_[kind] = nullptr;
                }
        }
};

// the value of a position for the side to move
std::uint16_t probe(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return loss(0);                    // no pieces, no move
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        if (m == g_current) return g_sink->read(p.stm, p);
        if (g_metric == Metric::dtm) return from_depth(Metric::dtm, input_depth(m, BYTES + p.stm, i), 0);          // one file
        return from_depth(g_metric, input_depth(m, BYTES + p.stm, i), std::uint8_t(input_byte(m, WDL + p.stm, i >> 2) >> (2 * (i & 3)) & 3));
}

// win, draw or loss of a position for the side to move
std::uint8_t probe_wdl(Pos const& p0)
{
        if ((p0.m[p0.stm] | p0.k[p0.stm]).empty()) return WDL_LOSS;
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        if (m == g_current) return wdl_of(g_sink->read(p.stm, p));
        return std::uint8_t(input_byte(m, WDL + p.stm, i >> 2) >> (2 * (i & 3)) & 3);
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
        std::uint16_t best = loss(0);
        bool first = true;
        for_each_move(pad_pos(p), [&](MoveRef const& m) {
                std::uint16_t const x = move_value(p, to_move(p, m));
                best = first ? x : better(best, x);
                first = false;
                return true;
        });
        return best;
}

// ---------------------------------------------------------------- one group in the 8-bit cell (synthesis 7.5)
//
// One byte per position and side to move, the statuses inside, rounds by scanning (rtbgen's scan mode), and
// value-preserving reductions: when the plies run past the window, the window's depth codes are written unchanged to a
// zstd snapshot (0 elsewhere) and collapse to WON or LOST, the last ply to FRONT_WIN or FRONT_LOSS (the previous ply of
// the next round). At the end the snapshots and the last window are overlaid into the sink. Codes:
//   0          undecided and lossable, its exits (if any) all lost by now; a draw at the end
//   1 .. 240   a depth in the current window: dtm one code per ply (the class is the parity), dtc and dtz two
//              (win, loss), so a window holds 240 or 120 plies (EGDB_CELL8_WINDOW=k plies forces a smaller window,
//              to test the reductions on small builds)
//   C_WON, C_LOST            decided in an earlier window
//   C_FRONT_WIN, C_FRONT_LOSS the last ply of the previous window
//   C_NOLOSE   undecided, cannot lose: an exit draws or wins
//   C_PENDING  undecided, lossable once its slowest losing exit is reached (its level in the eloss list)
//   C_DEFER    decided in pass 0 at a ply beyond the current window (dtm); recomputed when its window comes
//   C_CHANGED  a loss candidate of the current round (rtbgen's CHANGED): undecided and lossable, a predecessor of a win
//              of the previous ply; checked once, then a loss or back to 0
//   C_DRAWN    decided in pass 0 as a draw. Needed: the unmoves do not test compulsory capture, so a position whose
//              captures leave the group (no move inside) still turns up as a predecessor, and must not look undecided

bool g_cell8 = false;
constexpr std::uint8_t C_WON = 241, C_LOST = 242, C_FRONT_WIN = 243, C_FRONT_LOSS = 244, C_NOLOSE = 245,
                       C_PENDING = 246, C_DEFER = 247, C_DRAWN = 248, C_CHANGED = 249;

struct Snapshot { int window; std::vector<char> z; };

template<class ExecutionPolicy>
void solve_group_cell8(Group const& g, DepthSink<std::uint16_t>& sink, ExecutionPolicy const& policy)
{
        std::uint64_t const P = g.P, T = 2 * P;
        auto const pos = [&](std::uint64_t id) { return g.pos(id % P, int(id / P)); };
        bool const dtm = g_metric == Metric::dtm;
        int const cpp = dtm ? 1 : 2;                                    // codes per ply
        int WP = 240 / cpp;                                             // plies per window
        if (char const* e = std::getenv("EGDB_CELL8_WINDOW")) WP = std::clamp(std::atoi(e), 2, 240 / cpp);
        int w = 0;                                                      // the current window
        auto const encode = [&](std::uint16_t v, int win) {            // a value of window win -> its code
                int const o = plies(v) - WP * win;
                return std::uint8_t(dtm ? 1 + o : 1 + 2 * o + (is_loss(v) ? 1 : 0));
        };
        auto const decode = [&](std::uint8_t c, int win) -> std::uint16_t {
                int const o = dtm ? c - 1 : (c - 1) / 2, p = WP * win + o;
                bool const lost = dtm ? p % 2 == 0 : ((c - 1) & 1) != 0;
                return lost ? loss(p) : win_value(p);
        };
        std::vector<std::uint8_t> cell(T, 0);
        // fused rounds (EGDB_CELL8_FUSE=1, as rtbgen's iter): a loss found by a check marks its predecessors won at once,
        // so the next ply needs no sweep for it; the losses that no check finds (decided in pass 0, or re-encoded from
        // C_DEFER by a reduction) are kept in a bit plane, and only they are visited for the wins of the next ply
        static bool const fuse = std::getenv("EGDB_CELL8_FUSE") && std::string(std::getenv("EGDB_CELL8_FUSE")) != "0";
        Plane p0loss(fuse ? T : 0);
        auto const ld = [&](std::uint64_t id) { return std::atomic_ref<std::uint8_t>(cell[id]).load(std::memory_order_relaxed); };
        auto const st = [&](std::uint64_t id, std::uint8_t c) { std::atomic_ref<std::uint8_t>(cell[id]).store(c, std::memory_order_relaxed); };
        // f(q) for every cell q in [lo, hi) holding code c, 8 cells at a time: one relaxed atomic load of a 64-bit word
        // (GCC's __atomic builtin on a may_alias word: the cells are bytes), its bytes equal to c found at once (the
        // exact zero-byte test on word ^ c c c c c c c c), the rest of the block byte by byte. A word is read before
        // f runs on its cells; f writes only cells that do not hold c before or after (a candidate it checks, other
        // codes elsewhere), so the word is never stale for the test. The blocks of for_blocks start on whole words.
        using alias_word = std::uint64_t __attribute__((may_alias));
        auto const scan_eq = [&](std::uint64_t lo, std::uint64_t hi, std::uint8_t c, auto&& f) {
                constexpr std::uint64_t ones = 0x0101010101010101ull, low7 = 0x7f7f7f7f7f7f7f7full;
                std::uint64_t const pattern = ones * c;
                std::uint64_t q = lo;
                for (; q < hi && q % 8 != 0; ++q) if (ld(q) == c) f(q);
                for (; q + 8 <= hi; q += 8) {
                        std::uint64_t const x = __atomic_load_n(reinterpret_cast<alias_word const*>(cell.data() + q), __ATOMIC_RELAXED) ^ pattern;
                        std::uint64_t m = ~(((x & low7) + low7) | x | low7);    // the high bit of every zero byte of x
                        for (; m != 0; m &= m - 1) f(q + std::uint64_t(std::countr_zero(m)) / 8);
                }
                for (; q < hi; ++q) if (ld(q) == c) f(q);
        };

        // the value of a position whose moves all leave the group (pass 0, and again for a deferred one), or 0
        auto const exit_value = [&](Pos const& p, auto const& ml) -> std::uint16_t {
                int wn = INF, l = 0;
                bool drawn = false;
                for (auto const& mv : ml) {
                        std::uint16_t const x = move_value(p, mv);
                        if (is_win(x)) wn = std::min(wn, plies(x));
                        else if (is_loss(x)) l = std::max(l, plies(x));
                        else drawn = true;
                }
                return wn < INF ? win_value(wn) : drawn ? std::uint16_t(0) : loss(l);
        };

        // pass 0: decided positions get their code (or C_DEFER), the others a status; the exits by level in lists
        struct Entry { std::uint32_t level, id; };
        std::uint64_t const nblocks = num_blocks(T);
        std::vector<std::vector<Entry>> ewin_b(nblocks), eloss_b(nblocks);
        std::atomic<int> max_seed{ 0 };
        auto const t0 = std::chrono::steady_clock::now();
        for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                std::uint64_t const blk = lo / block_size;
                int ms = 0;
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        auto const i = std::uint32_t(id);
                        bool any = false, in = false, drawn = false;
                        int wn = INF, l = 0;
                        for_each_move(pad_pos(p), [&](MoveRef const& m) {      // a move inside needs no successor
                                any = true;
                                if (inside_ref(m)) { in = true; return true; }
                                std::uint16_t const x = move_value(p, to_move(p, m));
                                if (is_win(x)) wn = std::min(wn, plies(x));
                                else if (is_loss(x)) l = std::max(l, plies(x));
                                else drawn = true;
                                return true;
                        });
                        if (!any) { cell[id] = encode(loss(0), 0); if (fuse) p0loss.set_atomic(id); continue; }
                        if (!in) {                                      // decided
                                std::uint16_t const v = wn < INF ? win_value(wn) : drawn ? std::uint16_t(0) : loss(l);
                                if (v == 0) { cell[id] = C_DRAWN; continue; }
                                ms = std::max(ms, plies(v));
                                cell[id] = plies(v) < WP ? encode(v, 0) : C_DEFER;
                                if (fuse && is_loss(v)) p0loss.set_atomic(id);
                                continue;
                        }
                        if (wn < INF) { ewin_b[blk].push_back({ std::uint32_t(wn), i }); ms = std::max(ms, wn); }
                        if (drawn || wn < INF) { cell[id] = C_NOLOSE; continue; }
                        if (l > 0) { cell[id] = C_PENDING; eloss_b[blk].push_back({ std::uint32_t(l), i }); ms = std::max(ms, l); }
                }
                int cur = max_seed.load();
                while (ms > cur && !max_seed.compare_exchange_weak(cur, ms)) {}
        });
        g_stats.pass0 += seconds_since(t0);
        auto const t1 = std::chrono::steady_clock::now();
        using List = std::vector<std::uint32_t>;
        auto const by_level = [&](std::vector<std::vector<Entry>>& parts) {
                std::vector<List> out(std::size_t(max_seed.load()) + 1);
                for (auto& v_ : parts) {
                        for (auto const& e : v_) out[e.level].push_back(e.id);
                        std::vector<Entry>().swap(v_);
                }
                return out;
        };
        std::vector<List> const ewin_at = by_level(ewin_b), eloss_at = by_level(eloss_b);
        auto const at = [&](std::vector<List> const& by, int n) -> List const& {
                static List const none;
                return n >= 0 && n < int(by.size()) ? by[std::size_t(n)] : none;
        };
        auto const for_list = [&](List const& l, auto&& f) {
                for_blocks(policy, l.size(), [&](std::uint64_t lo, std::uint64_t hi) { for (auto j = lo; j < hi; ++j) f(l[j]); });
        };
        auto const predecessors = [&](std::uint64_t q, auto&& f) {
                std::uint64_t const off = (1 - q / P) * P;
                for_each_inside_predecessor(g, pos(q), q % P, [&](std::uint64_t i) { f(off + i); });
        };

        // reduce: snapshot window w - 1 (its depth codes unchanged, 0 elsewhere), collapse it, enter window w
        std::vector<Snapshot> snaps;
        auto const reduce = [&]() {
                Snapshot s{ w - 1, {} };
                ZSTD_CCtx* cx = ZSTD_createCCtx();
                ZSTD_CCtx_setParameter(cx, ZSTD_c_compressionLevel, 1);
                ZSTD_CCtx_setParameter(cx, ZSTD_c_nbWorkers, is_parallel_policy<ExecutionPolicy> ? 4 : 0);
                constexpr std::size_t B = std::size_t(1) << 20;
                std::vector<std::uint8_t> stage(B);
                std::vector<char> out(ZSTD_CStreamOutSize());
                for (std::uint64_t lo = 0; lo < T; lo += B) {
                        std::size_t const n = std::size_t(std::min<std::uint64_t>(B, T - lo));
                        for (std::size_t j = 0; j < n; ++j) { std::uint8_t const c = cell[lo + j]; stage[j] = c >= 1 && c <= 240 ? c : 0; }
                        ZSTD_inBuffer in{ stage.data(), n, 0 };
                        bool const last = lo + n == T;
                        for (;;) {
                                ZSTD_outBuffer o{ out.data(), out.size(), 0 };
                                std::size_t const r = ZSTD_compressStream2(cx, &o, &in, last ? ZSTD_e_end : ZSTD_e_continue);
                                if (ZSTD_isError(r)) { std::cerr << "zstd: " << ZSTD_getErrorName(r) << std::endl; std::abort(); }
                                s.z.insert(s.z.end(), out.data(), out.data() + o.pos);
                                if (last ? r == 0 : in.pos == in.size) break;
                        }
                }
                ZSTD_freeCCtx(cx);
                snaps.push_back(std::move(s));
                int const last_ply = WP * w - 1;
                for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t id = lo; id < hi; ++id) {
                                std::uint8_t const c = cell[id];
                                if (c >= 1 && c <= 240) {
                                        std::uint16_t const v = decode(c, w - 1);
                                        cell[id] = plies(v) == last_ply ? (is_win(v) ? C_FRONT_WIN : C_FRONT_LOSS) : (is_win(v) ? C_WON : C_LOST);
                                } else if (c == C_FRONT_WIN) cell[id] = C_WON;
                                else if (c == C_FRONT_LOSS) cell[id] = C_LOST;
                                else if (c == C_DEFER) {                       // decided in pass 0: does it fall in window w?
                                        Pos const p = pos(id);
                                        std::uint16_t const v = exit_value(p, moves(p));
                                        if (plies(v) < WP * (w + 1)) { cell[id] = encode(v, w); if (fuse && is_loss(v)) p0loss.set_atomic(id); }
                                }
                        }
                });
        };

        // a code that is a win in fewer than n plies (WON and FRONT_WIN are wins of earlier windows)
        auto const win_below = [&](std::uint8_t c, int n) {
                if (c == C_WON || c == C_FRONT_WIN) return true;
                if (c < 1 || c > 240) return false;
                std::uint16_t const v = decode(c, w);
                return is_win(v) && plies(v) < n;
        };
        // rtbgen's CHANGED mark (default): the predecessors of the previous ply's wins are marked, then each candidate
        // is checked once, against all its successors (EGDB_CELL8_INLINE: check at every unmove instead, for comparison)
        static bool const mark_changed = std::getenv("EGDB_CELL8_INLINE") == nullptr;
        static bool const count_checks = std::getenv("EGDB_CELL8_COUNT") != nullptr;
        std::atomic<std::uint64_t> checks{ 0 };
        auto const mark_win = [&](std::uint64_t id, int n) {
                std::uint8_t const c = ld(id);
                if (c != 0 && c != C_NOLOSE && c != C_PENDING) return false;
                st(id, encode(win_value(n), w));
                return true;
        };
        std::atomic<bool> wins_ahead{ false };                          // fused: wins of the next ply already marked
        auto const check_loss = [&](std::uint64_t id, int n) {            // rtbgen's check_loss: lost in n?
                if (ld(id) != 0) return false;
                if (count_checks) checks.fetch_add(1, std::memory_order_relaxed);
                Pos const p = pos(id);
                std::uint64_t const gp = id % P, other = (1 - id / P) * P;
                bool const lost = for_each_move(pad_pos(p), [&](MoveRef const& m) {    // stops at the first move inside that does not lose
                        return !inside_ref(m) || win_below(ld(other + successor_index(g, p, gp, m)), n);
                });
                if (!lost) return false;
                st(id, encode(loss(n), w));
                if (fuse && (n + 1) % WP != 0) {                        // rtbgen's RETRO(mark_wins): the predecessors won in n + 1
                        // a predecessor may be a candidate of this ply not checked yet: it is won (it has a move to this
                        // loss), so it takes the win; the check sweep claims its candidates by a compare-and-swap and
                        // skips one that became a win meanwhile
                        bool any = false;
                        for_each_inside_predecessor(g, p, gp, [&](std::uint64_t i) {
                                auto cell_ref = std::atomic_ref<std::uint8_t>(cell[other + i]);
                                std::uint8_t c = cell_ref.load(std::memory_order_relaxed);
                                std::uint8_t const won = encode(win_value(n + 1), w);
                                while (c == 0 || c == C_NOLOSE || c == C_PENDING || c == C_CHANGED)
                                        if (cell_ref.compare_exchange_weak(c, won, std::memory_order_relaxed)) { any = true; break; }
                        });
                        if (any) wins_ahead.store(true, std::memory_order_relaxed);
                }
                return true;
        };
        // the time of each step of the rounds (summed over the plies), reported with the group
        double t_reduce = 0, t_unmove = 0, t_check = 0, t_win = 0;
        auto const timed = [](double& acc, auto&& f) { auto const s0 = std::chrono::steady_clock::now(); f(); acc += seconds_since(s0); };
        for (int n = 1;; ++n) {
                if (n % WP == 0) { ++w; timed(t_reduce, reduce); }
                std::atomic<bool> found{ false };
                auto const note = [&](bool b) { if (b) found.store(true, std::memory_order_relaxed); };
                note(wins_ahead.exchange(false));                       // fused: this ply's wins, marked by the last ply's checks
                bool const fused_prev = fuse && n % WP != 0;            // the last ply's checks marked this ply's wins
                // the codes of the previous ply n - 1 (dtm: a win only at an odd ply, a loss only at an even one)
                bool const front = (n - 1) / WP != w;
                bool const has_win = n > 1 && (!dtm || (n - 1) % 2 == 1), has_loss = !dtm || (n - 1) % 2 == 0;
                std::uint8_t const cw = has_win ? (front ? C_FRONT_WIN : encode(win_value(n - 1), w)) : 0;
                std::uint8_t const cl = has_loss ? (front ? C_FRONT_LOSS : encode(loss(n - 1), w)) : 0;
                // the wins in n (from the losses in n - 1, and the exits). Fused, they come first: a check of ply n marks
                // wins in n + 1 at once, and must not take a cell that is a win in n; and a position won in n is never
                // lost in n, nor do the checks of ply n need the wins of n
                auto const wins_in_n = [&] {
                        if (has_loss && fused_prev)                     // only the losses no check found
                                timed(t_win, [&] { p0loss.for_each_par(policy, [&](std::uint64_t q) {
                                        if (ld(q) == cl) predecessors(q, [&](std::uint64_t id) { note(mark_win(id, n)); });
                                }); });
                        else if (has_loss)
                                timed(t_win, [&] { for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                                        scan_eq(lo, hi, cl, [&](std::uint64_t q) { predecessors(q, [&](std::uint64_t id) { note(mark_win(id, n)); }); });
                                }); });
                        timed(t_win, [&] { for_list(at(ewin_at, n), [&](std::uint64_t id) { note(mark_win(id, n)); }); });
                };
                if (fuse) wins_in_n();
                for_list(at(eloss_at, n), [&](std::uint64_t id) { if (cell[id] == C_PENDING) cell[id] = 0; });
                if (has_win && mark_changed) {                                  // losses in n: mark, then check once
                        timed(t_unmove, [&] { for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                                scan_eq(lo, hi, cw, [&](std::uint64_t q) {
                                        predecessors(q, [&](std::uint64_t id) {
                                                std::uint8_t z = 0;
                                                std::atomic_ref<std::uint8_t>(cell[id]).compare_exchange_strong(z, C_CHANGED, std::memory_order_relaxed);
                                        });
                                });
                        }); });
                        timed(t_check, [&] { for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                                scan_eq(lo, hi, C_CHANGED, [&](std::uint64_t q) {
                                        if (fuse) {                     // claim it: a fused win may have taken it meanwhile
                                                std::uint8_t c = C_CHANGED;
                                                if (!std::atomic_ref<std::uint8_t>(cell[q]).compare_exchange_strong(c, 0, std::memory_order_relaxed)) return;
                                        } else st(q, 0);
                                        note(check_loss(q, n));
                                });
                        }); });
                } else if (has_win)
                        timed(t_check, [&] { for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {         // losses in n
                                scan_eq(lo, hi, cw, [&](std::uint64_t q) { predecessors(q, [&](std::uint64_t id) { note(check_loss(id, n)); }); });
                        }); });
                timed(t_check, [&] { for_list(at(eloss_at, n), [&](std::uint64_t id) { note(check_loss(id, n)); }); });
                if (!fuse) wins_in_n();
                if (!found && n > max_seed.load()) {
                        g_stats.levels = std::max(g_stats.levels, std::uint64_t(n));
                        break;
                }
        }

        auto const t_merge0 = std::chrono::steady_clock::now();
        // merge: all windows side by side, chunk by chunk (every snapshot stream-decompressed, the last window read from
        // the cells). A position's value is a transform-reduce over the windows: transform its byte in each window into
        // that window's value (0 where it is NOT_MINE, or not a depth code), reduce by keeping the non-zero one (exactly
        // one window decided it; 0 is the identity, and no decided value is 0)
        struct Window { std::uint8_t const* bytes; int window; };
        struct Stream { ZSTD_DCtx* dx; ZSTD_inBuffer in; std::vector<std::uint8_t> buf; };
        constexpr std::size_t B = std::size_t(1) << 20;
        std::uint64_t zbytes = 0;
        std::vector<Stream> streams;
        for (auto const& s : snaps) {
                zbytes += s.z.size();
                streams.push_back({ ZSTD_createDCtx(), { s.z.data(), s.z.size(), 0 }, std::vector<std::uint8_t>(B) });
        }
        std::vector<Window> windows(snaps.size() + 1);
        std::atomic<bool> deferred{ false };
        for (std::uint64_t base = 0; base < T; base += B) {
                std::size_t const n = std::size_t(std::min<std::uint64_t>(B, T - base));
                std::vector<std::size_t> ks(streams.size());                   // the snapshots' chunks, side by side
                std::iota(ks.begin(), ks.end(), std::size_t(0));
                auto const unpack = [&](std::size_t k) {
                        ZSTD_outBuffer o{ streams[k].buf.data(), n, 0 };
                        while (o.pos < o.size) {
                                std::size_t const r = ZSTD_decompressStream(streams[k].dx, &o, &streams[k].in);
                                if (ZSTD_isError(r)) { std::cerr << "zstd: " << ZSTD_getErrorName(r) << std::endl; std::abort(); }
                                if (r == 0 && o.pos < o.size) { std::cerr << "zstd: snapshot too short" << std::endl; std::abort(); }
                        }
                        windows[k] = { streams[k].buf.data(), snaps[k].window };
                };
                std::for_each(policy, ks.begin(), ks.end(), unpack);
                windows.back() = { cell.data() + base, w };                    // the last window: the cells themselves
                for_blocks(policy, n, [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t j = lo; j < hi; ++j) {
                                std::uint16_t const v = std::transform_reduce(windows.begin(), windows.end(), std::uint16_t(0),
                                        [](std::uint16_t a, std::uint16_t b) { return a != 0 ? a : b; },
                                        [&](Window const& x) -> std::uint16_t {
                                                std::uint8_t const c = x.bytes[j];
                                                return c >= 1 && c <= 240 ? decode(c, x.window) : std::uint16_t(0);
                                        });
                                std::uint64_t const id = base + j;
                                if (v != 0) sink.put(int(id / P), id % P, pos(id), v);
                                else if (cell[id] == C_DEFER) deferred = true;
                        }
                });
        }
        for (auto& st : streams) ZSTD_freeDCtx(st.dx);
        if (deferred) { std::cerr << "cell8: a deferred position was never reached" << std::endl; std::abort(); }
        g_stats.frontier += zbytes;                             // cell8: the bytes of the snapshots
        if (count_checks) std::cout << "cell8: " << checks.load() << " loss checks for " << T << " positions" << std::endl;
        g_stats.rounds += seconds_since(t1);
        std::cout << "cell8 steps: unmoves " << t_unmove << " s, loss checks " << t_check << " s, win marking " << t_win
                  << " s, reductions " << t_reduce << " s, merge " << seconds_since(t_merge0) << " s (dual index " << g_dual_index << ", fused " << fuse << ")" << std::endl;
        if (!snaps.empty())
                std::cout << "cell8: group of " << T << " positions, " << snaps.size() << " reductions, snapshots " << zbytes << " bytes" << std::endl;
}

// ---------------------------------------------------------------- one group in rtbgen's sweeps (DTM, one window)
//
// rtbgen's iteration (Ronald de Man's DTZ50 generator) carried over to DTM, with rtbgen's names: one byte per position
// in each colour (table_w, table_b), one sweep per ply over one colour (white at even plies, black at odd), the loss
// depth computed from the successors' codes, and rtbgen's tbl dispatch on the code. No lists: an exit win is written
// in pass 0 as a provisional win, and the loss check looks the exits up again for the slowest losing one. Codes:
//   DRAWN            decided in pass 0 as a draw (all its moves leave the group)
//   MATE - k         a loss in 2k plies (MATE: no moves)
//   WIN_IN_ONE + k   a win in 2k + 1 plies
//   EXIT_DRAW        undecided, cannot lose: an exit draws (rtbgen's CAPT_DRAW)
//   CHANGED          a loss candidate (rtbgen's CHANGED)
//   UNKNOWN          undecided; a draw at the end (0, so the table starts zeroed)
//
// Ply p sweeps colour p mod 2, and each of its cells is read once:
//   CHANGED          rtbgen's check_loss: every move inside to an opened win (one of at most p - 1 plies); the depth e
//                    is 1 + the slowest of them, or the slowest losing exit. A loss in time (e <= p) marks its
//                    predecessors won in e + 1 at once (rtbgen's RETRO(mark_wins)); a deeper one (a slow losing exit)
//                    waits for its own ply, as the losses of pass 0 do
//   a loss of the sweep's even depth (p or p - 1), written by pass 0 or by an early check: its predecessors won
//                    (rtbgen's tbl[MATE] = 3 and mark_win_in_1, at any depth)
//   a win of the sweep's odd depth (p or p - 1): its predecessors UNKNOWN -> CHANGED (rtbgen's mark_changed)
// So every loss marks its predecessors exactly once, at its ply, and a win written then is final: a faster one comes
// from a shallower loss, marked at an earlier ply. Only a provisional exit win (a move out of the group wins, and moves
// inside remain) can be replaced by a faster win. The unmoves do not test compulsory capture, so a position decided in
// pass 0 (its moves all leave the group: it has a capture) can turn up as a predecessor; it holds a decided code, which
// no marking touches: a win replaces only UNKNOWN, CHANGED, EXIT_DRAW, or a slower win of a position without a capture.
// The sweeps end past the deepest value written.
namespace rtb {
constexpr std::uint8_t UNKNOWN = 0, DRAWN = 1, MATE = 121, WIN_IN_ONE = 122, EXIT_DRAW = 254, CHANGED = 255;
constexpr int KMAX = 120;                                       // codes per class: wins to 239 plies, losses to 238
constexpr int MAX_DEPTH = 2 * KMAX - 1;
constexpr bool is_win_code(std::uint8_t c) { return c >= WIN_IN_ONE && c < WIN_IN_ONE + KMAX; }
constexpr bool is_loss_code(std::uint8_t c) { return c > MATE - KMAX && c <= MATE; }
constexpr int win_depth(std::uint8_t c) { return 2 * (c - WIN_IN_ONE) + 1; }
constexpr int loss_depth(std::uint8_t c) { return 2 * (MATE - c); }
constexpr std::uint8_t win_code(int d) { return std::uint8_t(WIN_IN_ONE + (d - 1) / 2); }
constexpr std::uint8_t loss_code(int d) { return std::uint8_t(MATE - d / 2); }
static_assert(loss_code(MAX_DEPTH - 1) > DRAWN && win_code(MAX_DEPTH) < EXIT_DRAW);
}

bool g_rtb = false;

template<class ExecutionPolicy>
void solve_group_rtb(Group const& g, DepthSink<std::uint16_t>& sink, ExecutionPolicy const& policy)
{
        using namespace rtb;
        if (g_metric != Metric::dtm) { std::cerr << "rtb: dtm only" << std::endl; std::abort(); }
        std::uint64_t const P = g.P, T = 2 * P;                 // table_w = [0, P), table_b = [P, 2P)
        auto const pos = [&](std::uint64_t id) { return g.pos(id % P, int(id / P)); };
        std::vector<std::uint8_t> table(T, UNKNOWN);
        auto const ld = [&](std::uint64_t id) { return std::atomic_ref<std::uint8_t>(table[id]).load(std::memory_order_relaxed); };
        std::atomic<int> max_depth{ 0 };                        // the deepest value written, provisional ones included
        std::atomic<bool> provisional{ false };                 // pass 0 wrote a provisional exit win
        auto const note_depth = [&](int d) {
                int cur = max_depth.load(std::memory_order_relaxed);
                while (d > cur && !max_depth.compare_exchange_weak(cur, d, std::memory_order_relaxed)) {}
        };
        auto const fits = [](int d) {
                if (d > MAX_DEPTH) { std::cerr << "rtb: depth " << d << " beyond one window" << std::endl; std::abort(); }
        };
        std::atomic<std::uint64_t> replaced{ 0 }, refused{ 0 };
        auto const set_win_value = [&](std::uint64_t id, std::uint8_t v) {     // rtbgen's SET_WIN_VALUE, see above
                std::atomic_ref<std::uint8_t> a(table[id]);
                std::uint8_t c = a.load(std::memory_order_relaxed);
                for (;;) {
                        if (c == UNKNOWN || c == CHANGED || c == EXIT_DRAW) {
                                if (a.compare_exchange_weak(c, v, std::memory_order_relaxed)) return;
                                continue;
                        }
                        if (!is_win_code(c) || c <= v || !provisional.load(std::memory_order_relaxed)) return;
                        if (has_capture(pos(id))) { refused.fetch_add(1, std::memory_order_relaxed); return; }  // decided in pass 0
                        if (a.compare_exchange_weak(c, v, std::memory_order_relaxed)) { replaced.fetch_add(1, std::memory_order_relaxed); return; }
                }
        };
        auto const set_changed = [&](std::uint64_t id) {                        // rtbgen's SET_CHANGED
                std::uint8_t c = UNKNOWN;
                std::atomic_ref<std::uint8_t>(table[id]).compare_exchange_strong(c, CHANGED, std::memory_order_relaxed);
        };
        auto const retro = [&](std::uint64_t id, Pos const& p, auto&& f) {     // rtbgen's RETRO: the predecessors
                std::uint64_t const other = (1 - id / P) * P;
                for_each_inside_predecessor(g, p, id % P, [&](std::uint64_t i) { f(other + i); });
        };
        auto const mark_wins = [&](std::uint64_t id, Pos const& p, int e) {    // a loss in e: its predecessors won
                fits(e + 1);
                note_depth(e + 1);
                retro(id, p, [&](std::uint64_t u) { set_win_value(u, win_code(e + 1)); });
        };

        // pass 0
        auto const t0 = std::chrono::steady_clock::now();
        for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                int md = 0;
                bool prov = false;
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        bool any = false, in = false, drawn = false;
                        int wn = INF, l = 0;
                        for_each_move(pad_pos(p), [&](MoveRef const& m) {
                                any = true;
                                if (inside_ref(m)) { in = true; return true; }
                                std::uint16_t const x = move_value(p, to_move(p, m));
                                if (is_win(x)) wn = std::min(wn, plies(x));
                                else if (is_loss(x)) l = std::max(l, plies(x));
                                else drawn = true;
                                return true;
                        });
                        if (!any) { table[id] = MATE; continue; }
                        if (wn < INF) {                                 // final if all moves leave, else provisional
                                fits(wn);
                                table[id] = win_code(wn);
                                md = std::max(md, wn);
                                prov |= in;
                                continue;
                        }
                        if (!in) {
                                if (drawn) table[id] = DRAWN;
                                else { fits(l); table[id] = loss_code(l); md = std::max(md, l + 1); }
                                continue;
                        }
                        if (drawn) table[id] = EXIT_DRAW;               // else UNKNOWN: its exits (if any) all lose
                }
                note_depth(md);
                if (prov) provisional = true;
        });
        g_stats.pass0 += seconds_since(t0);
        auto const t1 = std::chrono::steady_clock::now();

        // rtbgen's check_loss: every move inside to an opened win; the depth from the slowest of them and of the exits
        auto const check_loss = [&](std::uint64_t q, int ply) {
                Pos const p = pos(q);
                std::uint64_t const gp = q % P, other = (1 - q / P) * P;
                int worst = 0;
                bool const all = for_each_move(pad_pos(p), [&](MoveRef const& m) {
                        if (!inside_ref(m)) return true;
                        std::uint8_t const c = ld(other + successor_index(g, p, gp, m));
                        if (!is_win_code(c) || win_depth(c) > ply - 1) return false;
                        worst = std::max(worst, win_depth(c));
                        return true;
                });
                if (!all) { table[q] = UNKNOWN; return; }
                int e = worst + 1;
                for_each_move(pad_pos(p), [&](MoveRef const& m) {      // the exits all lose (else pass 0 wrote a win or EXIT_DRAW)
                        if (!inside_ref(m)) e = std::max(e, plies(move_value(p, to_move(p, m))));
                        return true;
                });
                fits(e);
                table[q] = loss_code(e);
                if (e <= ply) mark_wins(q, p, e);                       // in time; a deeper loss waits for its ply
                else note_depth(e + 1);
        };

        // the sweeps
        int ply = 0;
        for (; ply <= max_depth.load() + 2; ++ply) {
                std::uint64_t const base = std::uint64_t(ply % 2) * P;
                int const de = ply % 2 ? ply - 1 : ply, dw = ply % 2 ? ply : ply - 1;  // this sweep's own losses and wins
                std::uint8_t const cl = de <= MAX_DEPTH - 1 ? loss_code(de) : CHANGED;
                std::uint8_t const cw = dw >= 1 && dw <= MAX_DEPTH ? win_code(dw) : CHANGED;
                for_blocks(policy, P, [&](std::uint64_t lo, std::uint64_t hi) {
                        for (std::uint64_t q = base + lo; q < base + hi; ++q) {
                                std::uint8_t const c = table[q];        // only this cell's own loop writes this colour
                                if (c == CHANGED) check_loss(q, ply);
                                else if (c == cl) mark_wins(q, pos(q), de);
                                else if (c == cw) retro(q, pos(q), set_changed);
                        }
                });
        }
        double const t_sweeps = seconds_since(t1);

        auto const t3 = std::chrono::steady_clock::now();
        for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                for (std::uint64_t id = lo; id < hi; ++id) {
                        std::uint8_t const c = table[id];
                        std::uint16_t const v = is_win_code(c) ? win(win_depth(c)) : is_loss_code(c) ? loss(loss_depth(c)) : 0;
                        if (v != 0) sink.put(int(id / P), id % P, pos(id), v);
                }
        });
        g_stats.rounds += seconds_since(t1);
        std::cout << "rtb steps: sweeps " << t_sweeps << " s (" << ply << " plies), output " << seconds_since(t3)
                  << " s; provisional wins replaced " << replaced.load() << ", refused (capture) " << refused.load() << std::endl;
}

// ---------------------------------------------------------------- one group: pass 0 and the rounds

// The values go to the depth sink: frontier rounds never read them back, so they write straight into it; scan mode
// finds its frontier by scanning the values, so it keeps them in an array of the group and hands them over at the end.
template<class ExecutionPolicy>
void solve_group(Group const& g, DepthSink<std::uint16_t>& sink, ExecutionPolicy const& policy)
{
        if (g_cell8 && g_rtb) { solve_group_rtb(g, sink, policy); return; }
        if (g_cell8) { solve_group_cell8(g, sink, policy); return; }
        std::uint64_t const P = g.P, T = 2 * P;                 // position id = stm * P + index in the group
        auto const pos = [&](std::uint64_t id) { return g.pos(id % P, int(id / P)); };
        std::vector<std::uint16_t> v(g_frontier ? 0 : T, 0);    // scan mode only
        auto const set_value = [&](std::uint64_t id, Pos const& p, std::uint16_t value) {
                if (g_frontier) sink.put(int(id / P), id % P, p, value);
                else v[id] = value;
        };

        // pass 0: every position looks up its moves out of the group. Decided ones get their value; the others leave
        // bits (has a move inside; every exit loses; no exits) and, by level, their fastest win and slowest loss through
        // an exit. A block (block_size positions) owns the words of its bits and has its own lists, merged by level after.
        struct Entry { std::uint32_t level, id; };
        std::uint64_t const nblocks = num_blocks(T);
        std::vector<std::vector<Entry>> won_b(nblocks), lost_b(nblocks), ewin_b(nblocks), eloss_b(nblocks);
        Plane openp(T), lossable(T), exits_done(T);
        auto const t0 = std::chrono::steady_clock::now();
        for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {
                std::uint64_t const blk = lo / block_size;
                for (std::uint64_t id = lo; id < hi; ++id) {
                        Pos const p = pos(id);
                        auto const i = std::uint32_t(id);
                        bool any = false, in = false, drawn = false;
                        int w = INF, l = 0;
                        for_each_move(pad_pos(p), [&](MoveRef const& m) {      // a move inside needs no successor
                                any = true;
                                if (inside_ref(m)) { in = true; return true; }
                                std::uint16_t const x = move_value(p, to_move(p, m));
                                if (is_win(x)) w = std::min(w, plies(x));
                                else if (is_loss(x)) l = std::max(l, plies(x));
                                else drawn = true;
                                return true;
                        });
                        if (!any) { set_value(id, p, loss(0)); lost_b[blk].push_back({ 0, i }); continue; }
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
                for_blocks(policy, l.size(), [&](std::uint64_t lo, std::uint64_t hi) { for (auto j = lo; j < hi; ++j) f(l[j]); });
        };
        auto const predecessors = [&](std::uint64_t q, auto&& f) {             // the other side to move
                std::uint64_t const off = (1 - q / P) * P;
                for_each_inside_predecessor(g, pos(q), q % P, [&](std::uint64_t i) { f(off + i); });
        };

        if (!g_frontier) {
                // rtbgen: every round scans the whole group for the values of the previous ply; the values are the state
                auto const check_loss = [&](std::uint64_t id, int n) {         // rtbgen's check_loss: lost in n?
                        if (!openp.test(id) || load(v[id]) != 0 || !lossable.test(id) || !exits_done.test(id)) return false;
                        Pos const p = pos(id);
                        std::uint64_t const gp = id % P, other = (1 - id / P) * P;
                        bool const lost = for_each_move(pad_pos(p), [&](MoveRef const& m) {    // every move inside to a win in fewer than n plies
                                if (!inside_ref(m)) return true;
                                std::uint16_t const x = load(v[other + successor_index(g, p, gp, m)]);
                                return is_win(x) && plies(x) < n;               // pass 0 decides wins of any depth
                        });
                        if (!lost) return false;
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
                        for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {         // losses in n
                                for (std::uint64_t q = lo; q < hi; ++q)
                                        if (n > 1 && load(v[q]) == win(n - 1))            // win(0) would be 0
                                                predecessors(q, [&](std::uint64_t id) { note(check_loss(id, n)); });
                        });
                        for_list(at(eloss_at, n), [&](std::uint64_t id) { note(check_loss(id, n)); });
                        for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {         // wins in n
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
                for_blocks(policy, T, [&](std::uint64_t lo, std::uint64_t hi) {         // hand the values to the sink
                        for (std::uint64_t id = lo; id < hi; ++id)
                                if (v[id] != 0) sink.put(int(id / P), id % P, pos(id), v[id]);
                });
        } else {
                // frontiers: bit planes of the wins and losses found in the previous round, plus the lists by level of the
                // positions decided in pass 0 and of the exits; a round visits only these. The state the rounds test is
                // in cumulative bit planes, not in the values: won and lost hold every position decided at a level
                // below the round (pass-0 decisions join when their level comes round), so v is only written
                Plane won(T), lost(T), won_prev(T), lost_prev(T), won_new(T), lost_new(T);
                // rtbgen's delayed check (default): the unmoves only mark loss candidates in a bit plane, then each
                // candidate is checked once, against all its successors (EGDB_FRONTIER_INLINE: check at every unmove)
                static bool const delayed = std::getenv("EGDB_FRONTIER_INLINE") == nullptr;
                Plane cand(delayed ? T : 0);
                for (int n = 1;; ++n) {
                        for_list(at(eloss_at, n), [&](std::uint64_t id) { exits_done.set_atomic(id); });   // exits lost by n
                        // losses in n: lossable, exits lost by n, undecided, every move inside to a win of a level below n
                        auto const lose = [&](std::uint64_t id) {
                                if (!openp.test(id) || !lossable.test(id) || !exits_done.test(id) || won.test(id) || lost.test(id)) return;
                                Pos const p = pos(id);
                                std::uint64_t const gp = id % P, other = (1 - id / P) * P;
                                if (!for_each_move(pad_pos(p), [&](MoveRef const& m) { return !inside_ref(m) || won.test(other + successor_index(g, p, gp, m)); })) return;
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
                        if (delayed) {
                                auto const mark = [&](std::uint64_t id) {
                                        if (openp.test(id) && lossable.test(id) && exits_done.test(id) && !won.test(id) && !lost.test(id))
                                                cand.set_atomic(id);
                                };
                                won_prev.for_each_par(policy, [&](std::uint64_t q) { predecessors(q, mark); });
                                for_list(at(won_at, n - 1), [&](std::uint64_t q) { predecessors(q, mark); });
                                for_list(at(eloss_at, n), mark);
                                cand.for_each_par(policy, lose);
                                for_blocks(policy, cand.w.size(), [&](std::uint64_t lo, std::uint64_t hi) { std::fill(cand.w.begin() + std::ptrdiff_t(lo), cand.w.begin() + std::ptrdiff_t(hi), 0); });
                        } else {
                                won_prev.for_each_par(policy, [&](std::uint64_t q) { predecessors(q, lose); });
                                for_list(at(won_at, n - 1), [&](std::uint64_t q) { predecessors(q, lose); });
                                for_list(at(eloss_at, n), lose);
                        }
                        // wins in n: the predecessors of the losses in n - 1, and the exits winning in n
                        lost_prev.for_each_par(policy, [&](std::uint64_t q) { predecessors(q, gain); });
                        for_list(at(lost_at, n - 1), [&](std::uint64_t q) { predecessors(q, gain); });
                        for_list(at(ewin_at, n), gain);
                        // commit level n: the new wins and losses, and the pass-0 decisions of level n
                        won_new.for_each_par(policy, [&](std::uint64_t q) { won.set(q); });
                        lost_new.for_each_par(policy, [&](std::uint64_t q) { lost.set(q); });
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

bool g_group_files = true;                                      // the depth sink: group files (false: the material in RAM)
bool g_byte_sink = true;                                        // the material in RAM: the 8-bit sink (false: the 16-bit array)
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
        for (auto const& g : groups) m.add(g);
        std::uint64_t const T = 2 * m.max_group;
        // 10 bit planes (has a move inside, every exit loses, exits lost by now, won, lost, four frontiers, candidates); the level
        // lists with pass 0's per-block entries while they are merged (8 + 4 bytes per entry, at most one per position
        // and side to move counted); the scan mode's value array; the groups' own memory (MemoryNeed::meta)
        std::uint64_t const transient = g_cell8 ? T + 12 * T : 10 * ((T + 63) / 64) * 8 + 12 * T + (g_frontier ? 0 : 2 * T);
        std::uint64_t const writing = std::uint64_t(5) << 18;                          // the final pass's chunk buffers
        // the memory-mapped files count in the resident set as their pages are touched: the inputs (the materials the
        // moves lead into: the bytes and wdl files for dtm, the wdl files for dtc and dtz) and, at rung 1, the
        // group-order files of the material itself (read by the lookups into earlier groups and by the final pass)
        std::uint64_t inputs = 0;
        for (Material const s : successors_of(c)) {
                std::uint64_t const n = mat_index(s).size;
                inputs += 2 * ((n + 3) / 4) + (g_metric == Metric::dtm ? 2 * n : 0);
        }
        std::uint64_t const sink_bytes = g_byte_sink ? 2 * mi.size + 2 * ((mi.size + 3) / 4) : 4 * mi.size;   // both sides
        m.rung0 = FIXED_BYTES + m.meta(false) + inputs + sink_bytes + std::max(transient, writing);
        m.rung1 = FIXED_BYTES + m.meta(true) + inputs + 2 * T + 2 * m.positions + std::max(transient, writing);
        return m;
}

bool g_write_stats = false;                                     // the build option `stats`: a stats file per material

// The stats file (build option stats), <material>-<metric>.stats next to the material's files: per side to move, the
// legal positions won and lost in every number of plies (exact), the draws, and the longest win and loss with a
// position (as rtbgen --stats). One pass over the sink in parallel blocks, before the files are closed.
template<class ExecutionPolicy>
void write_stats(std::string const& dir, Material c, MatIndex const& mi, DepthSink<std::uint16_t> const& sink, ExecutionPolicy const& policy)
{
        constexpr int MAXP = 4096;
        struct Side {
                std::vector<std::atomic<std::uint64_t>> win, loss;
                std::atomic<std::uint64_t> draws{ 0 };
                std::mutex mx;
                int longest_win = -1, longest_loss = -1;
                std::uint64_t slot_win = 0, slot_loss = 0;
                Side() : win(MAXP), loss(MAXP) {}
        };
        std::array<Side, 2> st;
        for (int side : { 0, 1 }) {
                Side& S = st[std::size_t(side)];
                for_blocks(policy, mi.size, [&](std::uint64_t a, std::uint64_t b) {
                        std::vector<std::uint32_t> w, l;
                        std::uint64_t d = 0, sw = 0, sl = 0;
                        int lw = -1, ll = -1;
                        for (std::uint64_t j = a; j < b; ++j) {
                                std::uint16_t const v = sink.at(side, j);
                                if (v == 0) { Pos p; d += mi.pos(j, side, p); continue; }      // a legal slot: a draw
                                int const n = std::min(plies(v), MAXP - 1);
                                auto& h = is_win(v) ? w : l;
                                if (int(h.size()) <= n) h.resize(std::size_t(n) + 1, 0);
                                ++h[std::size_t(n)];
                                if (is_win(v) && plies(v) > lw) { lw = plies(v); sw = j; }
                                if (is_loss(v) && plies(v) > ll) { ll = plies(v); sl = j; }
                        }
                        for (std::size_t n = 0; n < w.size(); ++n) if (w[n]) S.win[n].fetch_add(w[n], std::memory_order_relaxed);
                        for (std::size_t n = 0; n < l.size(); ++n) if (l[n]) S.loss[n].fetch_add(l[n], std::memory_order_relaxed);
                        S.draws.fetch_add(d, std::memory_order_relaxed);
                        std::lock_guard lock(S.mx);
                        if (lw > S.longest_win || (lw == S.longest_win && sw < S.slot_win)) { S.longest_win = lw; S.slot_win = sw; }
                        if (ll > S.longest_loss || (ll == S.longest_loss && sl < S.slot_loss)) { S.longest_loss = ll; S.slot_loss = sl; }
                });
        }
        std::ofstream f(c.path(dir + "/" + metric_name()) + "-" + metric_name() + ".stats");
        f << "# " << c.name() << " " << metric_name() << ": legal positions per side to move; plies exact\n";
        for (int side : { 0, 1 }) {
                Side& S = st[std::size_t(side)];
                char const* const tm = side == 0 ? "wtm" : "btm";
                std::uint64_t nw = 0, nl = 0;
                for (int n = 0; n < MAXP; ++n) { nw += S.win[std::size_t(n)]; nl += S.loss[std::size_t(n)]; }
                f << tm << "\twins\t" << nw << "\n" << tm << "\tdraws\t" << S.draws.load() << "\n" << tm << "\tlosses\t" << nl << "\n";
                for (int n = 0; n < MAXP; ++n) if (S.win[std::size_t(n)]) f << tm << "\twin\t" << n << "\t" << S.win[std::size_t(n)].load() << "\n";
                for (int n = 0; n < MAXP; ++n) if (S.loss[std::size_t(n)]) f << tm << "\tloss\t" << n << "\t" << S.loss[std::size_t(n)].load() << "\n";
                Pos p;
                if (S.longest_win >= 0 && mi.pos(S.slot_win, side, p)) f << tm << "\tlongest win\t" << S.longest_win << "\t" << fen_of(p) << "\n";
                if (S.longest_loss >= 0 && mi.pos(S.slot_loss, side, p)) f << tm << "\tlongest loss\t" << S.longest_loss << "\t" << fen_of(p) << "\n";
        }
}

template<class ExecutionPolicy>
bool build_material(std::string const& dir, Material c, ExecutionPolicy const& policy)
{
        if (!stored(c)) { std::cerr << c.name() << " is the mirror of a stored material" << std::endl; return false; }
        std::filesystem::create_directories(dir + "/" + metric_name() + "/" + c.tree());
        std::filesystem::create_directories(dir + "/wdl/" + c.tree());
        g_dir = dir;
        g_current = c;
        MatIndex const& mi = mat_index(c);
        auto groups = groups_of(c);
        if (g_mem_mb != 0 || g_sink_auto) {             // the claim against the estimates, and the sink
                MemoryNeed const need = memory_need(c);
                std::uint64_t const r0 = to_mb(need.rung0), r1 = to_mb(need.rung1);
                if (g_sink_auto) g_group_files = !(g_mem_mb == 0 || r0 <= g_mem_mb);
                std::uint64_t const used = g_group_files ? r1 : r0;
                std::cout << "memory: claim " << g_mem_mb << " MB, rung 0 (" << (g_byte_sink ? "bytes" : "array") << ") " << r0 << " MB, rung 1 (group files) " << r1
                          << " MB: " << (g_group_files ? "group files" : g_byte_sink ? "bytes" : "array");
                if (g_mem_mb != 0 && used > g_mem_mb) std::cout << " (over the claim: needs a lower rung, not implemented yet)";
                std::cout << std::endl;
        }
        open_successors(c);
        g_frozen = true;
        std::unique_ptr<DepthSink<std::uint16_t>> sink;
        if (g_group_files) sink = std::make_unique<GroupSink<std::uint16_t>>(dir + "/" + metric_name(), c, mi, groups);     // next to the depth files
        else if (g_byte_sink) sink = std::make_unique<ByteSink>(dir, c, mi);
        else sink = std::make_unique<ArraySink<std::uint16_t>>(mi);
        g_sink = sink.get();
        g_stats = {};
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (std::size_t k = 0; k < groups.size(); ++k) {
                groups[k].load();                               // its list of configurations, if it has one
                sink->begin_group(k, groups[k]);
                solve_group(groups[k], *sink, policy);
                sink->end_group();
                positions += 2 * groups[k].P;
                if (!sink->keeps_groups()) groups[k].release();
        }

        // the four files, each written once, sequentially, in index order: the values gathered from the sink slot by
        // slot, as unsigned bytes and wdl bits, a chunk at a time in parallel (blocks of block_size slots own whole wdl bytes).
        // The 8-bit sink holds the files already: only the counts are gathered, and the files renamed
        std::atomic<std::uint64_t> wins[2]{}, losses[2]{};
        std::atomic<int> longest{ 0 };
        std::atomic<bool> beyond{ false };
        auto const tw = std::chrono::steady_clock::now();
        constexpr std::uint64_t chunk = std::uint64_t(1) << 24;         // slots per write, a multiple of 4
        auto* const bytes_sink = dynamic_cast<ByteSink*>(sink.get());
        if (bytes_sink) {
                if (bytes_sink->beyond()) { std::cerr << "a depth beyond the last escape token" << std::endl; return false; }
                if (bytes_sink->tokens()) std::cout << "escape tokens: " << bytes_sink->tokens() << " positions beyond the direct range" << std::endl;
                for (int side : { 0, 1 })
                        for_blocks(policy, mi.size, [&](std::uint64_t a, std::uint64_t b) {
                                std::uint64_t nw = 0, nl = 0;
                                int lg = 0;
                                for (std::uint64_t j = a; j < b; ++j) {
                                        std::uint16_t const v = bytes_sink->at(side, j);
                                        nw += is_win(v);
                                        nl += is_loss(v);
                                        lg = std::max(lg, plies(v));
                                }
                                wins[side].fetch_add(nw, std::memory_order_relaxed);
                                losses[side].fetch_add(nl, std::memory_order_relaxed);
                                for (int cur = longest.load(); lg > cur && !longest.compare_exchange_weak(cur, lg);) {}
                        });
        }
        for (int side : { 0, 1 }) {
                if (bytes_sink) break;
                std::string const pb = mat_path(dir, c, BYTES + side), pw = mat_path(dir, c, WDL + side);
                std::ofstream fb(pb + ".tmp", std::ios::binary | std::ios::trunc), fw(pw + ".tmp", std::ios::binary | std::ios::trunc);
                std::vector<std::uint8_t> bytes, wdl;
                for (std::uint64_t lo = 0; lo < mi.size; lo += chunk) {
                        std::uint64_t const hi = std::min(mi.size, lo + chunk);
                        bytes.assign(hi - lo, 0);
                        wdl.assign((hi - lo + 3) / 4, 0);
                        for_blocks(policy, hi - lo, [&](std::uint64_t a, std::uint64_t b) {
                                std::uint64_t nw = 0, nl = 0;
                                int lg = 0;
                                for (std::uint64_t j = a; j < b; ++j) {
                                        std::uint16_t const v = sink->at(side, lo + j);
                                        int const by = to_depth(g_metric, v);
                                        if (by >= g_esc) { beyond.store(true, std::memory_order_relaxed); continue; }   // tokens: the bytes sink only
                                        bytes[j] = std::uint8_t(by);
                                        wdl[j >> 2] |= std::uint8_t(wdl_of(v) << (2 * (j & 3)));
                                        nw += is_win(v);
                                        nl += is_loss(v);
                                        lg = std::max(lg, plies(v));
                                }
                                wins[side].fetch_add(nw, std::memory_order_relaxed);
                                losses[side].fetch_add(nl, std::memory_order_relaxed);
                                for (int cur = longest.load(); lg > cur && !longest.compare_exchange_weak(cur, lg);) {}
                        });
                        if (beyond) { std::cerr << "a depth beyond the direct range: needs escape tokens (sink=bytes)" << std::endl; return false; }
                        fb.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));
                        fw.write(reinterpret_cast<char const*>(wdl.data()), std::streamsize(wdl.size()));
                }
                if (!fb || !fw) { std::cerr << "write failed " << pb << std::endl; return false; }
                fb.close();
                fw.close();
                std::filesystem::rename(pb + ".tmp", pb);
                if (wdl_exists(pw)) {                          // built by another metric: compare, keep
                        std::uint64_t const ws = file_size(c, WDL + side);
                        if (load_input(pw + ".tmp", ws) != load_input(pw, ws)) { std::cerr << "wdl differs from the existing " << pw << std::endl; return false; }
                        std::filesystem::remove(pw + ".tmp");
                } else std::filesystem::rename(pw + ".tmp", pw);
        }
        if (g_write_stats) write_stats(dir, c, mi, *sink, policy);
        if (bytes_sink) bytes_sink->finish(policy);     // the four files from memory, compressed or raw
        else if (g_compressed)                          // the build format, compressed: <name>.z for each of the four files
                for (int kind = 0; kind < 4; ++kind)
                        if (std::filesystem::exists(mat_path(dir, c, kind)))   // (a wdl file kept from another metric is done)
                                compress_file(mat_path(dir, c, kind), file_size(c, kind), policy);
        g_sink = nullptr;
        g_frozen = false;
        g_stats.write = seconds_since(tw);
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "material " << c.name() << " " << metric_name() << (g_group_files ? " group files" : g_byte_sink ? " bytes" : " array") << ": "
                  << groups.size() << " groups, " << positions << " positions, " << mi.size << " slots, wins " << wins[0].load() << " / "
                  << wins[1].load() << ", losses " << losses[0].load() << " / " << losses[1].load() << " (White / Black to move), longest " << longest.load()
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
                for (auto const& g : loaded_groups_of(c))
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

// one material as egdb_6x6's matverify: every slot of both sides read from its files and recomputed from its successors
// (and its wdl files against the values), in parallel with the policy
template<class ExecutionPolicy>
bool verify_material(std::string const& dir, Material c, ExecutionPolicy const& policy)
{
        if (!stored(c)) { std::cerr << c.name() << " is the mirror of a stored material" << std::endl; return false; }
        g_dir = dir;
        g_current = Material{ -1, -1, -1, -1 };                 // the material itself is read from its files
        MatIndex const& mi = mat_index(c);
        mat_index(mirror_of(c));
        open_successors(c);
        for (int kind : { int(BYTES), int(BYTES) + 1, int(WDL), int(WDL) + 1 }) open_input(c, kind);
        g_frozen = true;
        auto const t0 = std::chrono::steady_clock::now();
        std::atomic<std::uint64_t> checked{ 0 }, bad_s{ 0 }, bad_w{ 0 };
        for (int side : { 0, 1 })
                for_blocks(policy, mi.size, [&](std::uint64_t lo, std::uint64_t hi) {
                        std::uint64_t n = 0, bs = 0, bw = 0;
                        for (std::uint64_t slot = lo; slot < hi; ++slot) {
                                Pos p;
                                if (!mi.pos(slot, side, p)) continue;
                                std::uint16_t const got = probe(p), sv = value_from_successors(p);
                                if (sv != got && bs++ < 3) std::cerr << "S " << fen_of(p) << " stored " << got << " from successors " << sv << std::endl;
                                if (probe_wdl(p) != wdl_of(got) && bw++ < 3) std::cerr << "W " << fen_of(p) << std::endl;
                                ++n;
                        }
                        checked += n; bad_s += bs; bad_w += bw;
                });
        g_frozen = false;
        bool const ok = bad_s == 0 && bad_w == 0;
        std::cout << "verify " << c.name() << " " << metric_name() << ": " << checked.load() << " positions, " << bad_s.load()
                  << " differ from their successors, " << bad_w.load() << " from the wdl files, " << seconds_since(t0) << " s"
                  << (ok ? ", ok" : ", DIFFERENCES") << std::endl;
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
                for (auto const& g : loaded_groups_of(c))
                        for (std::uint64_t i = 0; i < g.P; ++i) {
                                Pos const pw = g.pos(i, 0), pb = g.pos(i, 1);
                                std::uint16_t const vw = probe(pw), vb = probe(pb);
                                bool const dtm = g_metric == Metric::dtm;              // thompson86's bytes (dtm in moves)
                                int const ew = !is_win(vw) ? 0 : dtm ? (plies(vw) + 1) / 2 : plies(vw);
                                int const el = !is_loss(vb) ? 0 : dtm ? plies(vb) / 2 + 1 : plies(vb) + 1;
                                std::uint64_t const k = mi.index(pw);
                                if (w[k] != ew && bad++ < 3) std::cerr << "w " << fen_of(pw) << " thompson86 " << int(w[k]) << " tbgen " << vw << std::endl;
                                if (l[k] != el && bad++ < 3) std::cerr << "l " << fen_of(pb) << " thompson86 " << int(l[k]) << " tbgen " << vb << std::endl;
                                checked += 2;
                        }
        }
        std::cout << "thompson86: " << checked << " positions, " << bad << " differences" << std::endl;
        return bad == 0;
}

// dtz <= dtc <= dtm for every win and every loss, all three with the class of the shared wdl file (<dir>/wdl)
bool check_order(std::string const& dir, int n)
{
        std::uint64_t wins = 0, losses = 0, bad = 0;
        for (Material const c : stored_materials_up_to(n)) {
                std::uint64_t const size = mat_index(c).size;
                for (int s : { 0, 1 }) {
                        std::uint64_t const ws = file_size(c, WDL + s);
                        auto const w = load_input(mat_path(dir, c, WDL + s), ws);
                        g_metric = Metric::dtz; auto const z = load_depths(mat_path(dir, c, s), size);
                        g_metric = Metric::dtc; auto const x = load_depths(mat_path(dir, c, s), size);
                        g_metric = Metric::dtm; auto const m = load_depths(mat_path(dir, c, s), size);
                        for (std::uint64_t i = 0; i < size; ++i) {
                                std::uint8_t const r = wdl_bits(w.data(), i);
                                std::uint16_t const zv = from_depth(Metric::dtz, z[i], r), xv = from_depth(Metric::dtc, x[i], r),
                                                    mv = from_depth(Metric::dtm, m[i], r);
                                if (wdl_of(mv) != r) { ++bad; continue; }               // the dtm byte's own class
                                if (mv == 0) continue;
                                (is_win(mv) ? wins : losses) += 1;
                                if (!(plies(zv) <= plies(xv) && plies(xv) <= plies(mv))) ++bad;
                        }
                }
        }
        std::cout << "order: " << wins << " wins, " << losses << " losses, " << bad
                  << " violations of dtz <= dtc <= dtm (or of the class in the wdl file)" << std::endl;
        return bad == 0;
}

// a value in words
std::string show_value(std::uint16_t v)
{
        if (v == 0) return "draw";
        return std::string(is_win(v) ? "win" : "loss") + " in " + std::to_string(plies(v)) + " plies";
}

// ---------------------------------------------------------------- the probe format (synthesis 10)
//
// The final files (as Syzygy's): per material the wdl for both sides to move, <dir>/probe/wdl/6x6/.../<wm>-<bm>-wdl-<wtm|btm>.z,
// and the depth for one side, the one that compresses better, <dir>/probe/<metric>/...-<metric>-<wtm|btm>.z, in the
// container of the build files (zstd blocks of 4 KiB) at zstd's level 19 (EGDB_PROBE_ZLEVEL). Don't-cares, filled with
// the previous stored value (runs extend): illegal slots; positions where the side to move must capture (the prober
// searches the captures, into smaller materials); pending captures: the side to move has no capture and every move of
// it leaves the opponent one (the prober searches its moves, one ply, into capture positions: never into another
// pending capture, so the search ends); and in the depth file the draws (the prober reads wdl first). The depth
// carries no class (it comes from wdl): dtm in moves (a win in 2c + 1 plies, a loss in 2c), dtc and dtz in plies; codes
// 0 .. 239 direct, 240 .. 255 escape tokens (no draw code). The side without a depth file is a one-ply search into the other.
constexpr int PESC = 240;

std::string probe_path(std::string const& dir, Material m, std::string const& kind, int side)
{
        return m.path(dir + "/probe/" + kind) + "-" + kind + (side == 0 ? "-wtm" : "-btm");
}

struct ProbeFiles { ZFile wdl[2]; int side = -1; ZFile depth; };    // side: the depth file's side to move (-1: none)
std::map<Material, ProbeFiles> g_probe;                 // opened before any parallel use, then only read

// open the probe files of the given stored materials, those that have both wdl files (once; then only read)
void open_probe(std::string const& dir, std::vector<Material> const& ms)
{
        g_probe.clear();
        for (Material const m : ms) {
                std::uint64_t const size = mat_index(m).size;
                if (!std::filesystem::exists(zpath(probe_path(dir, m, "wdl", 0))) || !std::filesystem::exists(zpath(probe_path(dir, m, "wdl", 1)))) continue;
                ProbeFiles& f = g_probe[m];
                for (int side : { 0, 1 }) {
                        f.wdl[side] = open_zfile(probe_path(dir, m, "wdl", side), (size + 3) / 4);
                        std::string const d = probe_path(dir, m, metric_name(), side);
                        if (std::filesystem::exists(zpath(d))) { f.depth = open_zfile(d, size); f.side = side; }
                }
        }
}

// every stored material of at most n pieces (for queries of a finished store)
void open_probe_all(std::string const& dir, int n) { open_probe(dir, stored_materials_up_to(n)); }

ProbeFiles const& probe_files(Material m)
{
        auto it = g_probe.find(m);
        if (it == g_probe.end()) { std::cerr << "no probe files for " << m.name() << std::endl; std::exit(1); }
        return it->second;
}

std::uint8_t mover_wdl(std::uint8_t child) { return child == WDL_LOSS ? WDL_WIN : child == WDL_WIN ? WDL_LOSS : WDL_DRAW; }

// a pending capture: the side to move has no capture, and every move of it leaves the opponent a capture
bool pending_capture(MoveList const& ml)
{
        if (ml.empty() || is_capture(*ml.begin())) return false;
        for (auto const& mv : ml) if (!has_capture(mv.next)) return false;
        return true;
}

// the don't-care kinds of a position, from its moves: capture (the side to move must capture) or pending capture
bool dont_care(MoveList const& ml) { return !ml.empty() && (is_capture(*ml.begin()) || pending_capture(ml)); }

// win, draw or loss for the side to move, from the probe files only
std::uint8_t pwdl(Pos const& p0)
{
        auto const ml = moves(p0);
        if (ml.empty()) return WDL_LOSS;
        if (dont_care(ml)) {                                            // a one-ply search
                std::uint8_t b = WDL_LOSS;
                for (auto const& mv : ml) {
                        std::uint8_t const r = mover_wdl(pwdl(mv.next));
                        if (r == WDL_WIN) return r;
                        if (r == WDL_DRAW) b = r;
                }
                return b;
        }
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        std::uint64_t const i = mat_index(m).index(p);
        return std::uint8_t(zbyte(probe_files(m).wdl[p.stm], i >> 2) >> (2 * (i & 3)) & 3);
}

std::uint16_t pvalue(Pos const& p0);

// the value of a move for the side making it, from the probe files only
std::uint16_t pmove_value(Pos const& p, Move const& mv)
{
        if (goal_move(p, mv)) {
                std::uint8_t const r = pwdl(mv.next);
                return r == WDL_LOSS ? win(1) : r == WDL_WIN ? loss(1) : 0;
        }
        std::uint16_t const v = pvalue(mv.next);
        return is_loss(v) ? win(plies(v) + 1) : is_win(v) ? loss(plies(v) + 1) : 0;
}

// the value of a position for the side to move, from the probe files only
std::uint16_t pvalue(Pos const& p0)
{
        auto const ml = moves(p0);
        if (ml.empty()) return loss(0);
        auto const search = [&] {
                std::uint16_t best = 0;
                bool first = true;
                for (auto const& mv : ml) {
                        std::uint16_t const x = pmove_value(p0, mv);
                        best = first ? x : better(best, x);
                        first = false;
                }
                return best;
        };
        if (dont_care(ml)) return search();
        Pos const p = stored_pos(p0);
        Material const m = material_of(p);
        ProbeFiles const& f = probe_files(m);
        std::uint64_t const i = mat_index(m).index(p);
        std::uint8_t const r = std::uint8_t(zbyte(f.wdl[p.stm], i >> 2) >> (2 * (i & 3)) & 3);
        if (r == WDL_DRAW) return 0;
        if (f.side < 0) { std::cerr << "no " << metric_name() << " probe file for " << m.name() << std::endl; std::exit(1); }
        if (p.stm != f.side) return search();                           // the side without a depth file
        int const c = zdepth(f.depth, i);
        bool const dtm = g_metric == Metric::dtm;
        return r == WDL_WIN ? win(dtm ? 2 * c + 1 : c) : loss(dtm ? 2 * c : c);
}

// convert <dir> wp bp wm bm <metric>: the material's build files into probe files (the wdl files only if no metric wrote
// them yet), then a check through the prober of every stride-th slot of both sides against the build values. The
// materials its moves lead into must have their probe files already.
template<class ExecutionPolicy>
bool convert_material(std::string const& dir, Material c, int stride, ExecutionPolicy const& policy)
{
        if (!stored(c)) { std::cerr << c.name() << " is the mirror of a stored material" << std::endl; return false; }
        g_dir = dir;
        auto const t0 = std::chrono::steady_clock::now();
        MatIndex const& mi = mat_index(c);
        std::uint64_t const size = mi.size, wsize = (size + 3) / 4;
        bool const dtm = g_metric == Metric::dtm;
        std::array<std::vector<std::uint16_t>, 2> dep;
        std::array<std::vector<std::uint8_t>, 2> wdl;
        for (int s : { 0, 1 }) { dep[std::size_t(s)] = load_depths(mat_path(dir, c, s), size); wdl[std::size_t(s)] = load_input(mat_path(dir, c, WDL + s), wsize); }
        auto const value = [&](int s, std::uint64_t i) { return from_depth(g_metric, dep[std::size_t(s)][i], wdl_bits(wdl[std::size_t(s)].data(), i)); };
        // don't-cares: bit 0 the wdl (illegal, must capture, pending capture), bit 1 the depth (also a draw)
        std::array<std::vector<std::uint8_t>, 2> dc{ std::vector<std::uint8_t>(size), std::vector<std::uint8_t>(size) };
        std::atomic<std::uint64_t> n_capture{ 0 }, n_pending{ 0 }, n_draw{ 0 };
        for (int s : { 0, 1 })
                for_blocks(policy, size, [&](std::uint64_t a, std::uint64_t b) {
                        std::uint64_t nc = 0, np = 0, nd = 0;
                        for (std::uint64_t i = a; i < b; ++i) {
                                Pos p;
                                std::uint8_t f = 0;
                                if (!mi.pos(i, s, p)) f = 3;
                                else {
                                        auto const ml = moves(p);
                                        if (!ml.empty() && is_capture(*ml.begin())) { f = 3; ++nc; }
                                        else if (pending_capture(ml)) { f = 3; ++np; }
                                        else if (value(s, i) == 0) { f = 2; ++nd; }
                                }
                                dc[std::size_t(s)][i] = f;
                        }
                        n_capture += nc; n_pending += np; n_draw += nd;
                });
        auto const encode_wdl = [&](int s, std::vector<std::uint8_t>& w) {
                w.assign(wsize, 0);
                std::uint8_t pw = WDL_DRAW;
                for (std::uint64_t i = 0; i < size; ++i) {
                        std::uint8_t const r = dc[std::size_t(s)][i] & 1 ? pw : wdl_of(value(s, i));
                        w[i >> 2] |= std::uint8_t(r << (2 * (i & 3)));
                        pw = r;
                }
        };
        auto const encode_depth = [&](int s, std::vector<std::uint8_t>& d, Extras& ex) {     // with the escape tokens (extras)
                d.assign(size, 0);
                ex.clear();
                std::uint8_t pd = 0;
                for (std::uint64_t i = 0; i < size; ++i) {
                        if (dc[std::size_t(s)][i] & 2) { d[i] = pd; continue; }        // a don't-care: the previous direct code
                        std::uint16_t const v = value(s, i);
                        int const cd = dtm ? plies(v) / 2 : plies(v);
                        if (cd < PESC) { d[i] = std::uint8_t(cd); pd = d[i]; continue; }
                        if (cd > PESC + ZRANGE * (255 - PESC) + ZRANGE - 1) { std::cerr << "depth beyond the last escape token" << std::endl; std::exit(1); }
                        d[i] = std::uint8_t(PESC + (cd - PESC) / ZRANGE);
                        ex.emplace_back(i, std::uint8_t((cd - PESC) % ZRANGE));
                }
        };
        std::filesystem::create_directories(std::filesystem::path(probe_path(dir, c, "wdl", 0)).parent_path());
        std::filesystem::create_directories(std::filesystem::path(probe_path(dir, c, metric_name(), 0)).parent_path());
        bool const have_wdl = std::filesystem::exists(zpath(probe_path(dir, c, "wdl", 0))) && std::filesystem::exists(zpath(probe_path(dir, c, "wdl", 1)));
        std::uint64_t zw = 0, zd[2]{};
        for (int s : { 0, 1 }) {
                if (!have_wdl) {                                        // both sides
                        std::vector<std::uint8_t> w;
                        encode_wdl(s, w);
                        compress_buffer(w.data(), wsize, probe_path(dir, c, "wdl", s), policy, nullptr, 256, g_probe_level);
                }
                zw += std::filesystem::file_size(zpath(probe_path(dir, c, "wdl", s)));
                std::vector<std::uint8_t> d;
                Extras ex;
                encode_depth(s, d, ex);
                compress_buffer(d.data(), size, probe_path(dir, c, metric_name(), s), policy, &ex, PESC, g_probe_level);
                zd[s] = std::filesystem::file_size(zpath(probe_path(dir, c, metric_name(), s)));
        }
        int const side = zd[1] < zd[0] ? 1 : 0;                         // the depth file that compresses better
        std::filesystem::remove(zpath(probe_path(dir, c, metric_name(), 1 - side)));
        double const t_write = seconds_since(t0);
        // check through the prober: every stride-th slot of both sides; the probe files of the material and of those
        // reachable from it (not of others, which other conversions may be writing)
        std::vector<Material> need{ c };                                // c and every material reachable from it: the
        for (std::size_t k = 0; k < need.size(); ++k)                   // prober's capture searches recurse
                for (Material const m : successors_of(need[k]))
                        if (std::ranges::find(need, m) == need.end()) need.push_back(m);
        open_probe(dir, need);
        auto const t1 = std::chrono::steady_clock::now();
        std::atomic<std::uint64_t> checked{ 0 }, bad{ 0 };
        std::uint64_t const n = (size + std::uint64_t(stride) - 1) / std::uint64_t(stride);
        for (int s : { 0, 1 })
                for_blocks(policy, n, [&](std::uint64_t a, std::uint64_t b) {
                        std::uint64_t k = 0, e = 0;
                        for (std::uint64_t j = a; j < b; ++j) {
                                std::uint64_t const i = j * std::uint64_t(stride);
                                Pos p;
                                if (!mi.pos(i, s, p)) continue;
                                std::uint16_t const want = value(s, i), got = pvalue(p);
                                std::uint8_t const gw = pwdl(p);
                                ++k;
                                if (got != want || gw != wdl_of(want)) {
                                        if (bad.load() + e < 3) std::cerr << "probe " << fen_of(p) << " build " << show_value(want) << " probe " << show_value(got) << std::endl;
                                        ++e;
                                }
                        }
                        checked += k;
                        bad += e;
                });
        std::cout << "convert " << c.name() << " " << metric_name() << ": wdl both sides " << zw << " bytes, " << metric_name()
                  << " side " << (side == 0 ? "wtm" : "btm") << " " << zd[side] << " bytes (" << size << " slots; don't-cares: "
                  << n_capture.load() << " captures, " << n_pending.load() << " pending captures, " << n_draw.load() << " draws), "
                  << t_write << " s; checked " << checked.load() << " positions (every " << stride << "th slot), " << bad.load()
                  << " differences, " << seconds_since(t1) << " s" << std::endl;
        return bad.load() == 0;
}


// probe <dir> <metric> [FEN...]: the value of each position (default: the start position) and of each of its moves,
// from the finished files under <dir>
bool probe_positions(std::string const& dir, std::vector<std::string> fens)
{
        g_dir = dir;
        if (fens.empty()) fens.push_back(start_fen());
        if (std::filesystem::exists(dir + "/probe/wdl")) {             // the probe format, if the store has it
                open_probe_all(dir, 12);
                for (std::string const& fen : fens) {
                        Pos p;
                        if (!parse_fen(fen, p)) { std::cerr << "bad FEN " << fen << std::endl; return false; }
                        std::cout << fen << "\t" << metric_name() << "\t" << show_value(pvalue(p)) << " (probe files)" << std::endl;
                        for (auto const& mv : moves(p))
                                std::cout << "  " << fen_of(mv.next) << "\t" << show_value(pmove_value(p, mv)) << " (for the side to move)" << std::endl;
                }
                return true;
        }
        for (std::string const& fen : fens) {
                Pos p;
                if (!parse_fen(fen, p)) { std::cerr << "bad FEN " << fen << std::endl; return false; }
                std::cout << fen << "\t" << metric_name() << "\t" << show_value(value_from_successors(p)) << std::endl;
                for (auto const& mv : moves(p))
                        std::cout << "  " << fen_of(mv.next) << "\t" << show_value(move_value(p, mv)) << " (for the side to move)" << std::endl;
        }
        return true;
}

} // namespace

int main(int argc, char* argv[])
{
        init_tables();
        init_cc();
        g_granularity = Granularity::material;             // tbgen's default: the whole material class (dtz: by total tempo)
        std::vector<std::string> const a(argv + 1, argv + argc);
        auto mat_at = [&](std::size_t i) {                             // wp bp wm bm
                int const wp = std::stoi(a[i]), bp = std::stoi(a[i + 1]), wm = std::stoi(a[i + 2]), bm = std::stoi(a[i + 3]);
                if (wm < 0 || bm < 0 || wm > wp || bm > bp) { std::cerr << "not a material: wp bp wm bm" << std::endl; std::exit(2); }
                return Material{ wm, wp - wm, bm, bp - bm };
        };
        if (a.size() == 1 && a[0] == "board") {                      // the board this binary was built for (its tree's name)
                std::cout << board_name() << (CHECKERS ? " checkers" : " international") << " mirror=" << mirror_name() << " men=" << start_men() << std::endl;
                return 0;
        }
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
        if (a.size() >= 7 && a.size() <= 14 && a[0] == "build" && parse_metric(a[6])) {
                bool par = false;                                       // the execution policy: par or seq
                for (std::size_t i = 7; i < a.size(); ++i) {
                        if (a[i] == "seq" || a[i] == "par") par = a[i] == "par";
                        else if (a[i] == "scan" || a[i] == "frontier") g_frontier = a[i] == "frontier";
                        else if (a[i] == "cell8") { g_cell8 = true; g_frontier = false; }
                        else if (a[i] == "rtb") { g_cell8 = g_rtb = true; g_frontier = false; }
                        else if (a[i] == "raw" || a[i] == "compressed") g_compressed = a[i] == "compressed";
                        else if (a[i] == "stats") g_write_stats = true;
                        else if (a[i].starts_with("mem=")) g_mem_mb = std::stoull(a[i].substr(4));
                        else if (a[i] == "groups" || a[i] == "array" || a[i] == "bytes") { g_group_files = a[i] == "groups"; g_byte_sink = a[i] != "array"; g_sink_auto = false; }
                        else if (a[i] == "auto") g_sink_auto = true;
                        else if (parse_granularity(a[i])) {}
                        else { std::cerr << "unknown option " << a[i] << std::endl; return 2; }
                }
                bool const ok = par ? build_material(a[1], mat_at(2), std::execution::par) : build_material(a[1], mat_at(2), std::execution::seq);
                return ok ? 0 : 1;
        }
        if ((a.size() == 7 || a.size() == 8) && a[0] == "verify" && parse_metric(a[6])) {     // verify <dir> wp bp wm bm <metric> [seq|par]
                Material const c = mat_at(2);
                if (a.size() == 8 && a[7] == "par") return verify_material(a[1], c, std::execution::par) ? 0 : 1;
                return verify_material(a[1], c, std::execution::seq) ? 0 : 1;
        }
        if ((a.size() == 4 || a.size() == 5) && a[0] == "checkall" && parse_metric(a[3]))
                return check_all(a[1], std::stoi(a[2]), a.size() == 5 ? std::stoi(a[4]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "cmp86" && parse_metric(a[4])) return compare_thompson86(a[1], a[2], std::stoi(a[3])) ? 0 : 1;
        if (a.size() == 3 && a[0] == "order") return check_order(a[1], std::stoi(a[2])) ? 0 : 1;
        if (a.size() >= 3 && a[0] == "probe" && parse_metric(a[2])) return probe_positions(a[1], std::vector<std::string>(a.begin() + 3, a.end())) ? 0 : 1;
        if (a.size() >= 7 && a.size() <= 9 && a[0] == "convert" && parse_metric(a[6])) {
                bool par = false;
                int stride = 16;
                for (std::size_t i = 7; i < a.size(); ++i) {
                        if (a[i] == "seq" || a[i] == "par") par = a[i] == "par";
                        else if (a[i].starts_with("verify=")) stride = std::max(1, std::stoi(a[i].substr(7)));
                        else { std::cerr << "unknown option " << a[i] << std::endl; return 2; }
                }
                bool const ok = par ? convert_material(a[1], mat_at(2), stride, std::execution::par) : convert_material(a[1], mat_at(2), stride, std::execution::seq);
                return ok ? 0 : 1;
        }
        std::cerr << "usage: " << (CHECKERS ? "ctbgen" : "itbgen") << " build <dir> wp bp wm bm dtm|dtc|dtz [seq|par] [frontier|scan|cell8] [auto|groups|bytes|array] [compressed|raw] [stats] [material|tempi|configuration] [mem=MB] | bench n [samples] | plan n dtm|dtc|dtz [granularity] | checkall <dir> n dtm|dtc|dtz [threads]"
                     " | cmp86 <dir> <dir_thompson86> n dtm|dtc|dtz | order <dir> n | probe <dir> dtm|dtc|dtz [FEN...] | convert <dir> wp bp wm bm dtm|dtc|dtz [seq|par] [verify=N]" << std::endl;
        return 2;
}
