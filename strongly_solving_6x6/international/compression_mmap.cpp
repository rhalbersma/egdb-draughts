//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Lookups through mmap: the zstd-compressed blocks (as in compression_lookup) and, for comparison, the uncompressed
// 2-bit stream, each written to a file and memory-mapped. Measured per variant:
//   * warm: every page of the file is in the page cache;
//   * cold: the file is first evicted from the page cache (posix_fadvise DONTNEED after msync, and madvise DONTNEED
//     on the mapping), so the first touch of every page reads it from disk; the time is over the first lookups.
// Every looked-up value is checked against the uncompressed stream in memory.
//
// build: g++-14 -std=c++23 -O2 -fopenmp compression_mmap.cpp -lzstd -o compression_mmap
// usage: compression_mmap data/6x6_10.dtm <dir for the test files> [sample=16] [threads=4] [lookups=2000000]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

#include <cstring>
#include <memory>
#include <omp.h>
#include <random>
#include <zstd.h>

namespace {

struct Mapped {
        std::uint8_t const* p = nullptr;
        std::size_t size = 0;
        int fd = -1;
};

Mapped map_ro(std::string const& path)
{
        Mapped m;
        m.fd = ::open(path.c_str(), O_RDONLY);
        struct stat st;
        if (m.fd < 0 || ::fstat(m.fd, &st) != 0) std::abort();
        m.size = std::size_t(st.st_size);
        void* q = ::mmap(nullptr, m.size, PROT_READ, MAP_SHARED, m.fd, 0);
        if (q == MAP_FAILED) std::abort();
        m.p = static_cast<std::uint8_t const*>(q);
        return m;
}

void evict(Mapped const& m)
{
        ::madvise(const_cast<std::uint8_t*>(m.p), m.size, MADV_DONTNEED);
        ::posix_fadvise(m.fd, 0, 0, POSIX_FADV_DONTNEED);
}

void warm(Mapped const& m)
{
        volatile std::uint8_t s = 0;
        for (std::size_t i = 0; i < m.size; i += 4096) s = s + m.p[i];
}

// resident fraction of the mapping (mincore)
double resident(Mapped const& m)
{
        std::size_t pages = (m.size + 4095) / 4096;
        std::vector<unsigned char> v(pages);
        if (::mincore(const_cast<std::uint8_t*>(m.p), m.size, v.data()) != 0) return -1;
        std::size_t r = 0;
        for (auto c : v) r += c & 1;
        return double(r) / double(pages);
}

void write_file(std::string const& path, std::vector<std::uint8_t> const& a, std::vector<std::uint8_t> const& b = {})
{
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) std::abort();
        std::fwrite(a.data(), 1, a.size(), f);
        if (!b.empty()) std::fwrite(b.data(), 1, b.size(), f);
        std::fflush(f);
        ::fsync(::fileno(f));            // clean pages, so that they can be evicted for the cold measurement
        std::fclose(f);
}

// compressed file: [u64 block][u64 nblocks][u64 offsets[nblocks + 1]][blocks]
std::vector<std::uint8_t> compress_file(std::vector<std::uint8_t> const& s, std::size_t block, int level, int threads)
{
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
        std::vector<std::uint64_t> head{ block, nb };
        std::uint64_t off = 0;
        for (auto const& p : parts) { head.push_back(off); off += p.size(); }
        head.push_back(off);
        std::vector<std::uint8_t> out(head.size() * 8);
        std::memcpy(out.data(), head.data(), out.size());
        for (auto const& p : parts) out.insert(out.end(), p.begin(), p.end());
        return out;
}

struct Reader {
        std::uint8_t const* base;
        std::size_t block, nb;
        std::uint64_t const* off;
        std::uint8_t const* data;
        ZSTD_DCtx* dx = ZSTD_createDCtx();
        std::vector<std::uint8_t> buf;
        explicit Reader(std::uint8_t const* p) : base(p)
        {
                auto const* h = reinterpret_cast<std::uint64_t const*>(p);
                block = h[0]; nb = h[1]; off = h + 2; data = p + (nb + 3) * 8;
                buf.resize(block);
        }
        ~Reader() { ZSTD_freeDCtx(dx); }
        int value(std::uint64_t pos)
        {
                std::size_t byte = pos >> 2, b = byte / block;
                std::size_t r = ZSTD_decompressDCtx(dx, buf.data(), buf.size(), data + off[b], off[b + 1] - off[b]);
                if (ZSTD_isError(r)) std::abort();
                return buf[byte - b * block] >> (2 * (pos & 3)) & 3;
        }
};

// microseconds per lookup (wall time x threads / lookups), every value checked
template <class F>
double timed(std::vector<std::uint64_t> const& pos, std::size_t count, int threads, std::vector<std::uint8_t> const& raw, F make)
{
        auto t0 = std::chrono::steady_clock::now();
        std::uint64_t bad = 0;
        #pragma omp parallel num_threads(threads) reduction(+ : bad)
        {
                auto get = make();
                #pragma omp for schedule(static)
                for (std::size_t i = 0; i < count; ++i)
                        if (get(pos[i]) != (raw[pos[i] >> 2] >> (2 * (pos[i] & 3)) & 3)) ++bad;
        }
        double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
        if (bad) { std::cerr << bad << " wrong values\n"; std::abort(); }
        return us / double(count) * threads;
}

}   // namespace

int main(int argc, char** argv)
{
        if (argc < 3) { std::cerr << "usage: compression_mmap <layer> <dir> [sample] [threads] [lookups]\n"; return 1; }
        std::string path = argv[1], dir = argv[2];
        int sample = argc > 3 ? std::atoi(argv[3]) : 16;
        int threads = argc > 4 ? std::atoi(argv[4]) : 4;
        std::size_t lookups = argc > 5 ? std::size_t(std::atoll(argv[5])) : 2'000'000;

        // the packed stream with the capture positions filled in (cached in dir/stream.bin)
        std::vector<std::uint8_t> raw;
        std::string cache = dir + "/stream.bin";
        if (std::FILE* f = std::fopen(cache.c_str(), "rb")) {
                std::fseek(f, 0, SEEK_END); raw.resize(std::size_t(std::ftell(f))); std::fseek(f, 0, SEEK_SET);
                if (std::fread(raw.data(), 1, raw.size(), f) != raw.size()) std::abort();
                std::fclose(f);
        } else {
                init_tables();
                std::FILE* fi = std::fopen((path + ".idx").c_str(), "rb");
                std::uint64_t hdr[4];
                if (!fi || std::fread(hdr, sizeof hdr, 1, fi) != 1) return 1;
                std::vector<std::pair<std::uint64_t, std::uint64_t>> layer(hdr[2]);
                if (std::fread(layer.data(), sizeof layer[0], layer.size(), fi) != layer.size()) return 1;
                std::fclose(fi);
                if (!map_file(path, hdr[3], 0)) return 1;
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
                std::uint8_t b = 0; int k = 0;
                for (auto const& v : sym) for (auto x : v) { b |= std::uint8_t(x << (2 * k)); if (++k == 4) { raw.push_back(b); b = 0; k = 0; } }
                if (k) raw.push_back(b);
                write_file(cache, raw);
        }
        std::uint64_t npos = std::uint64_t(raw.size()) * 4;
        std::mt19937_64 rng(3);
        std::vector<std::uint64_t> pos(lookups);
        for (auto& p : pos) p = rng() % npos;
        std::printf("%llu positions, uncompressed file %.0f MB, %zu lookups (cold: the first %zu), %d threads\n",
                    (unsigned long long)npos, double(raw.size()) / 1e6, lookups, lookups / 20, threads);
        std::size_t cold_n = lookups / 20;

        // the uncompressed 2-bit file
        std::string fr = dir + "/raw.bin";
        write_file(fr, raw);
        Mapped mr = map_ro(fr);
        auto raw_get = [&] { return [&](std::uint64_t p) { return mr.p[p >> 2] >> (2 * (p & 3)) & 3; }; };
        std::printf("\n%-24s %10s %14s %14s %14s %14s\n", "", "file MB", "warm, 1 thr", "warm, all thr", "cold, 1 thr", "cold, all thr");
        auto line = [&](char const* name, Mapped const& m, auto make) {
                warm(m);
                double w1 = timed(pos, lookups, 1, raw, make), wn = timed(pos, lookups, threads, raw, make);
                evict(m); double r0 = resident(m);
                double c1 = timed(pos, cold_n, 1, raw, make);
                evict(m);
                double cn = timed(pos, cold_n, threads, raw, make);
                std::printf("%-24s %10.1f %11.3f us %11.3f us %11.2f us %11.2f us   (resident after evict: %.0f%%)\n", name,
                            double(m.size) / 1e6, w1, wn, c1, cn, 100 * r0);
                std::fflush(stdout);
        };
        line("uncompressed, 2 bits", mr, raw_get);
        for (std::size_t block : { std::size_t(4096), std::size_t(16384) }) {
                std::string fc = dir + "/zstd_" + std::to_string(block) + ".bin";
                write_file(fc, compress_file(raw, block, 19, threads));
                Mapped mc = map_ro(fc);
                std::string name = "zstd -19, " + std::to_string(block / 1024) + " KiB blocks";
                line(name.c_str(), mc, [&] { return [rd = std::make_shared<Reader>(mc.p)](std::uint64_t p) { return rd->value(p); }; });
        }
        std::printf("\n(time per lookup = wall time x threads / lookups; cold over the first lookups after eviction)\n");
        return 0;
}
