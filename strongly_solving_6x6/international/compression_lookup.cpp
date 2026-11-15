//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Random-access lookup speed of zstd-compressed 6x6 databases. A sample of the slices of a layer made by `extend`
// (e.g. data/6x6_10.dtm) is reduced to win / draw / loss with the capture positions filled in (as in
// compression_zstd), packed 4 positions per byte, cut into independent blocks and compressed with zstd; the blocks
// and a block index stay in memory. A lookup of a position finds its block, decompresses it and extracts the 2-bit
// value; every looked-up value is checked against the uncompressed stream.
//
// Measured per block size and zstd level: the compressed size, and the time per lookup for
//   * uniformly random positions, one thread and all threads (every lookup decompresses a block);
//   * the same with a small cache of the most recently decompressed block per thread, for positions that come in
//     runs of nearby indices (as when a slice is built: successors of nearby positions lie close together).
//
// build: g++-14 -std=c++23 -O2 -fopenmp compression_lookup.cpp -lzstd -o compression_lookup
// usage: compression_lookup data/6x6_10.dtm [sample=16] [threads=4] [lookups=2000000]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

#include <omp.h>
#include <random>
#include <zstd.h>

namespace {

struct Compressed {
        std::size_t block;                        // uncompressed bytes per block (4 positions per byte)
        std::vector<std::uint8_t> data;           // the compressed blocks, one after the other
        std::vector<std::uint64_t> offset;        // block b is data[offset[b], offset[b + 1])
        std::size_t raw_size;
};

Compressed compress(std::vector<std::uint8_t> const& s, std::size_t block, int level, int threads)
{
        Compressed c{ block, {}, {}, s.size() };
        std::size_t nb = (s.size() + block - 1) / block;
        std::vector<std::vector<std::uint8_t>> parts(nb);
        #pragma omp parallel num_threads(threads)
        {
                ZSTD_CCtx* cx = ZSTD_createCCtx();
                #pragma omp for schedule(dynamic, 64)
                for (std::size_t b = 0; b < nb; ++b) {
                        std::size_t n = std::min(block, s.size() - b * block);
                        parts[b].resize(ZSTD_compressBound(n));
                        std::size_t r = ZSTD_compressCCtx(cx, parts[b].data(), parts[b].size(), s.data() + b * block, n, level);
                        if (ZSTD_isError(r)) std::abort();
                        parts[b].resize(r);
                }
                ZSTD_freeCCtx(cx);
        }
        c.offset.push_back(0);
        for (auto const& p : parts) { c.data.insert(c.data.end(), p.begin(), p.end()); c.offset.push_back(c.data.size()); }
        return c;
}

// one lookup thread: a decompression context and the last decompressed block
struct Reader {
        Compressed const* c;
        ZSTD_DCtx* dx = ZSTD_createDCtx();
        std::vector<std::uint8_t> buf;
        std::size_t cached = ~std::size_t(0);
        bool use_cache = false;
        explicit Reader(Compressed const& cc, bool cache) : c(&cc), buf(cc.block), use_cache(cache) {}
        ~Reader() { ZSTD_freeDCtx(dx); }
        int value(std::uint64_t pos)
        {
                std::size_t byte = pos >> 2, b = byte / c->block;
                if (!use_cache || b != cached) {
                        std::size_t r = ZSTD_decompressDCtx(dx, buf.data(), buf.size(), c->data.data() + c->offset[b],
                                                            c->offset[b + 1] - c->offset[b]);
                        if (ZSTD_isError(r)) std::abort();
                        cached = b;
                }
                return buf[byte - b * c->block] >> (2 * (pos & 3)) & 3;
        }
};

// positions: uniformly random, or in runs of `run` positions within a window of +-`spread` around a random centre
std::vector<std::uint64_t> make_positions(std::uint64_t n, std::size_t count, int run, std::uint64_t spread, unsigned seed)
{
        std::mt19937_64 rng(seed);
        std::vector<std::uint64_t> v;
        v.reserve(count);
        while (v.size() < count) {
                std::uint64_t centre = rng() % n;
                for (int k = 0; k < run && v.size() < count; ++k) {
                        std::int64_t d = spread ? std::int64_t(rng() % (2 * spread + 1)) - std::int64_t(spread) : 0;
                        std::int64_t p = std::int64_t(centre) + d;
                        v.push_back(std::uint64_t(std::clamp<std::int64_t>(p, 0, std::int64_t(n) - 1)));
                }
        }
        return v;
}

// nanoseconds per lookup, with every value checked against the uncompressed stream
double time_lookups(Compressed const& c, std::vector<std::uint8_t> const& raw, std::vector<std::uint64_t> const& pos,
                    int threads, bool cache)
{
        auto t0 = std::chrono::steady_clock::now();
        std::uint64_t bad = 0;
        #pragma omp parallel num_threads(threads) reduction(+ : bad)
        {
                Reader rd(c, cache);
                #pragma omp for schedule(static)
                for (std::size_t i = 0; i < pos.size(); ++i) {
                        std::uint64_t p = pos[i];
                        int v = rd.value(p);
                        if (v != (raw[p >> 2] >> (2 * (p & 3)) & 3)) ++bad;
                }
        }
        double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count();
        if (bad) { std::cerr << bad << " wrong values\n"; std::abort(); }
        return ns / double(pos.size()) * threads;          // per lookup and thread (wall time x threads / lookups)
}

}   // namespace

int main(int argc, char** argv)
{
        if (argc < 2) { std::cerr << "usage: compression_lookup <layer> [sample] [threads] [lookups]\n"; return 1; }
        std::string path = argv[1];
        int sample = argc > 2 ? std::atoi(argv[2]) : 16;
        int threads = argc > 3 ? std::atoi(argv[3]) : 4;
        std::size_t lookups = argc > 4 ? std::size_t(std::atoll(argv[4])) : 2'000'000;
        init_tables();
        std::FILE* f = std::fopen((path + ".idx").c_str(), "rb");
        std::uint64_t hdr[4];
        if (!f || std::fread(hdr, sizeof hdr, 1, f) != 1) { std::cerr << "cannot read index\n"; return 1; }
        std::vector<std::pair<std::uint64_t, std::uint64_t>> layer(hdr[2]);
        if (std::fread(layer.data(), sizeof layer[0], layer.size(), f) != layer.size()) return 1;
        std::fclose(f);
        if (!map_file(path, hdr[3], 0)) { std::cerr << "cannot map " << path << "\n"; return 1; }

        // the sampled slices as one packed stream with the capture positions filled in
        std::vector<std::size_t> pick;
        for (std::size_t j = 0; j < layer.size(); j += std::size_t(sample)) pick.push_back(j);
        std::vector<std::vector<std::uint8_t>> sym(pick.size());
        #pragma omp parallel for schedule(dynamic, 64) num_threads(threads)
        for (std::size_t q = 0; q < pick.size(); ++q) {
                Meta m = meta_of(Key::unpack(layer[pick[q]].first));
                std::uint8_t const* v = db2 + (layer[pick[q]].second & ~(WDL_FLAG | DB2_FLAG));
                std::uint64_t T = 2 * m.P;
                int prev = -1;
                sym[q].resize(T);
                for (std::uint64_t i = 0; i < T; ++i) {
                        int s = v[i] == DRAW8 ? 2 : (v[i] & 1);
                        int fill = has_capture(pos_of(m, i % m.P, int(i / m.P))) && prev >= 0 ? prev : s;
                        sym[q][i] = std::uint8_t(fill);
                        prev = fill;
                }
        }
        std::vector<std::uint8_t> raw;
        { std::uint8_t b = 0; int k = 0;
          for (auto const& v : sym) for (auto x : v) { b |= std::uint8_t(x << (2 * k)); if (++k == 4) { raw.push_back(b); b = 0; k = 0; } }
          if (k) raw.push_back(b); }
        sym.clear(); sym.shrink_to_fit();
        std::uint64_t npos = std::uint64_t(raw.size()) * 4;
        std::printf("sample: every %d-th slice, %llu positions, %zu lookups per measurement, %d threads\n", sample,
                    (unsigned long long)npos, lookups, threads);
        auto uni = make_positions(npos, lookups, 1, 0, 1);
        auto runs = make_positions(npos, lookups, 64, 2000, 2);

        std::printf("\n%-8s %-6s %12s %16s %16s %18s\n", "block", "level", "bits/pos", "random, 1 thr", "random, all thr",
                    "runs + cache, 1 thr");
        for (std::size_t block : { std::size_t(1024), std::size_t(4096), std::size_t(16384), std::size_t(65536) }) {
                for (int level : { 3, 19 }) {
                        Compressed c = compress(raw, block, level, threads);
                        double bits = 8.0 * double(c.data.size()) / double(npos);
                        double t1 = time_lookups(c, raw, uni, 1, false);
                        double tn = time_lookups(c, raw, uni, threads, false);
                        double tc = time_lookups(c, raw, runs, 1, true);
                        std::printf("%-8s %-6d %12.3f %13.2f us %13.2f us %15.2f us\n",
                                    (std::to_string(block / 1024) + " KiB").c_str(), level, bits, t1 / 1000, tn / 1000, tc / 1000);
                        std::fflush(stdout);
                }
        }
        std::printf("\n(time per lookup; \"all thr\" = wall time x threads / lookups, i.e. per lookup and thread)\n");
        return 0;
}
