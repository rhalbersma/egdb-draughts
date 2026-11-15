//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// How well does depth-to-mate compress? A sample of the slices of a layer made by `extend` (e.g. data/6x6_10.dtm,
// one byte of DTM per position: odd = win, even = loss, DRAW8 = draw) is taken as a byte stream, once as is and once
// with the positions where the side to move can capture filled in with the previous byte (their value follows from
// one ply of captures). The streams are cut into independent blocks and compressed with zstd and xz.
//
// build: g++-14 -std=c++23 -O2 -fopenmp compression_dtm.cpp -lzstd -llzma -o compression_dtm
// usage: compression_dtm data/6x6_10.dtm [sample=16] [threads=4]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

#include <cmath>
#include <lzma.h>
#include <omp.h>
#include <zstd.h>

namespace {

std::size_t zstd_total(std::vector<std::uint8_t> const& s, std::size_t block, int level, int threads)
{
        std::size_t nb = (s.size() + block - 1) / block, total = 0;
        #pragma omp parallel num_threads(threads) reduction(+ : total)
        {
                ZSTD_CCtx* cx = ZSTD_createCCtx();
                std::vector<std::uint8_t> out(ZSTD_compressBound(block));
                #pragma omp for schedule(dynamic, 64)
                for (std::size_t b = 0; b < nb; ++b) {
                        std::size_t n = std::min(block, s.size() - b * block);
                        std::size_t r = ZSTD_compressCCtx(cx, out.data(), out.size(), s.data() + b * block, n, level);
                        if (ZSTD_isError(r)) std::abort();
                        total += r;
                }
                ZSTD_freeCCtx(cx);
        }
        return total;
}

std::size_t xz_total(std::vector<std::uint8_t> const& s, std::size_t block, int threads)
{
        std::size_t nb = (s.size() + block - 1) / block, total = 0;
        #pragma omp parallel for schedule(dynamic, 4) num_threads(threads) reduction(+ : total)
        for (std::size_t b = 0; b < nb; ++b) {
                std::size_t n = std::min(block, s.size() - b * block);
                std::vector<std::uint8_t> out(lzma_stream_buffer_bound(n));
                std::size_t pos = 0;
                if (lzma_easy_buffer_encode(9, LZMA_CHECK_NONE, nullptr, s.data() + b * block, n, out.data(), &pos, out.size()) != LZMA_OK) std::abort();
                total += pos;
        }
        return total;
}

double entropy(std::vector<std::uint8_t> const& s)
{
        std::vector<double> c(256);
        for (auto x : s) ++c[x];
        double h = 0, n = double(s.size());
        for (double x : c) if (x > 0) h -= x / n * std::log2(x / n);
        return h;
}

}   // namespace

int main(int argc, char** argv)
{
        if (argc < 2) { std::cerr << "usage: compression_dtm <layer> [sample] [threads]\n"; return 1; }
        std::string path = argv[1];
        int sample = argc > 2 ? std::atoi(argv[2]) : 16;
        int threads = argc > 3 ? std::atoi(argv[3]) : 4;
        init_tables();
        std::FILE* f = std::fopen((path + ".idx").c_str(), "rb");
        std::uint64_t hdr[4];
        if (!f || std::fread(hdr, sizeof hdr, 1, f) != 1) return 1;
        std::vector<std::pair<std::uint64_t, std::uint64_t>> layer(hdr[2]);
        if (std::fread(layer.data(), sizeof layer[0], layer.size(), f) != layer.size()) return 1;
        std::fclose(f);
        if (!map_file(path, hdr[3], 0)) return 1;

        std::vector<std::size_t> pick;
        for (std::size_t j = 0; j < layer.size(); j += std::size_t(sample)) pick.push_back(j);
        std::vector<std::vector<std::uint8_t>> pr(pick.size()), pd(pick.size());
        #pragma omp parallel for schedule(dynamic, 64) num_threads(threads)
        for (std::size_t q = 0; q < pick.size(); ++q) {
                Meta m = meta_of(Key::unpack(layer[pick[q]].first));
                std::uint8_t const* v = db2 + (layer[pick[q]].second & ~(WDL_FLAG | DB2_FLAG));
                std::uint64_t T = 2 * m.P;
                pr[q].assign(v, v + T);
                pd[q].resize(T);
                int prev = -1;
                for (std::uint64_t i = 0; i < T; ++i) {
                        int b = has_capture(pos_of(m, i % m.P, int(i / m.P))) && prev >= 0 ? prev : v[i];
                        pd[q][i] = std::uint8_t(b);
                        prev = b;
                }
        }
        std::vector<std::uint8_t> raw, dc;
        for (auto const& x : pr) raw.insert(raw.end(), x.begin(), x.end());
        for (auto const& x : pd) dc.insert(dc.end(), x.begin(), x.end());
        pr.clear(); pd.clear();
        double n = double(raw.size());
        std::printf("sample: every %d-th slice, %.0f positions (one byte of DTM each = 8 bits)\n", sample, n);
        std::printf("byte entropy: as is %.3f bits, captures filled %.3f bits per position\n\n", entropy(raw), entropy(dc));
        std::printf("%-30s %10s %10s %10s   (bits per position)\n", "", "4 KiB", "64 KiB", "1 MiB");
        auto row = [&](char const* name, auto&& fn) {
                std::printf("%-30s", name);
                for (std::size_t b : { std::size_t(4096), std::size_t(65536), std::size_t(1) << 20 }) {
                        std::printf(" %10.3f", 8.0 * double(fn(b)) / n);
                        std::fflush(stdout);
                }
                std::printf("\n");
        };
        row("zstd -3, as is", [&](std::size_t b) { return zstd_total(raw, b, 3, threads); });
        row("zstd -3, captures filled", [&](std::size_t b) { return zstd_total(dc, b, 3, threads); });
        row("zstd -19, captures filled", [&](std::size_t b) { return zstd_total(dc, b, 19, threads); });
        row("xz -9, captures filled", [&](std::size_t b) { return xz_total(dc, b, threads); });
        return 0;
}
