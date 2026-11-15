//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// zstd (and xz for comparison) on the 6x6 databases, by block size: how much does random access cost?
// A sample of the slices of a layer made by `extend` (e.g. data/6x6_10.dtm) is reduced to win / draw / loss, packed
// 4 positions per byte, once as is and once with the positions where the side to move can capture ("don't care")
// filled in with the previous value (which minimises the number of value changes). The packed streams are then cut
// into independent blocks of 4 KiB ... 1 MiB and compressed with zstd at several levels, with and without a
// dictionary trained on the blocks, and with xz -9.
//
// build: g++-14 -std=c++23 -O2 -fopenmp compression_zstd.cpp -lzstd -llzma -o compression_zstd
// usage: compression_zstd data/6x6_10.dtm [sample=16] [threads=4]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

#include <lzma.h>
#include <omp.h>
#include <zdict.h>
#include <zstd.h>

namespace {

std::size_t xz_size(std::uint8_t const* in, std::size_t n)
{
        std::vector<std::uint8_t> out(lzma_stream_buffer_bound(n));
        std::size_t pos = 0;
        if (lzma_easy_buffer_encode(9, LZMA_CHECK_NONE, nullptr, in, n, out.data(), &pos, out.size()) != LZMA_OK) std::abort();
        return pos;
}

// total compressed size of the stream cut into blocks of `block` bytes, with zstd at `level` (and a dictionary)
std::size_t zstd_total(std::vector<std::uint8_t> const& s, std::size_t block, int level, std::vector<std::uint8_t> const* dict,
                       int threads)
{
        std::size_t nb = (s.size() + block - 1) / block, total = 0;
        ZSTD_CDict* cd = dict ? ZSTD_createCDict(dict->data(), dict->size(), level) : nullptr;
        #pragma omp parallel num_threads(threads) reduction(+ : total)
        {
                ZSTD_CCtx* cx = ZSTD_createCCtx();
                std::vector<std::uint8_t> out(ZSTD_compressBound(block));
                #pragma omp for schedule(dynamic, 64)
                for (std::size_t b = 0; b < nb; ++b) {
                        std::size_t n = std::min(block, s.size() - b * block);
                        std::size_t r = cd ? ZSTD_compress_usingCDict(cx, out.data(), out.size(), s.data() + b * block, n, cd)
                                           : ZSTD_compressCCtx(cx, out.data(), out.size(), s.data() + b * block, n, level);
                        if (ZSTD_isError(r)) std::abort();
                        total += r;
                }
                ZSTD_freeCCtx(cx);
        }
        if (cd) ZSTD_freeCDict(cd);
        return total;
}

std::size_t xz_total(std::vector<std::uint8_t> const& s, std::size_t block, int threads)
{
        std::size_t nb = (s.size() + block - 1) / block, total = 0;
        #pragma omp parallel for schedule(dynamic, 16) num_threads(threads) reduction(+ : total)
        for (std::size_t b = 0; b < nb; ++b) total += xz_size(s.data() + b * block, std::min(block, s.size() - b * block));
        return total;
}

std::vector<std::uint8_t> train(std::vector<std::uint8_t> const& s, std::size_t block, std::size_t dict_size)
{
        // train on every k-th block, at most 100 MB of samples
        std::size_t nb = s.size() / block, take = std::min(nb, std::size_t(100'000'000) / block), step = std::max<std::size_t>(1, nb / take);
        std::vector<std::uint8_t> samples;
        std::vector<std::size_t> sizes;
        for (std::size_t b = 0; b < nb && sizes.size() < take; b += step) {
                samples.insert(samples.end(), s.begin() + b * block, s.begin() + (b + 1) * block);
                sizes.push_back(block);
        }
        std::vector<std::uint8_t> dict(dict_size);
        std::size_t r = ZDICT_trainFromBuffer(dict.data(), dict.size(), samples.data(), sizes.data(), unsigned(sizes.size()));
        if (ZDICT_isError(r)) { std::cerr << "dictionary: " << ZDICT_getErrorName(r) << "\n"; return {}; }
        dict.resize(r);
        return dict;
}

}   // namespace

int main(int argc, char** argv)
{
        if (argc < 2) { std::cerr << "usage: compression_zstd <layer> [sample] [threads]\n"; return 1; }
        std::string path = argv[1];
        int sample = argc > 2 ? std::atoi(argv[2]) : 16;
        int threads = argc > 3 ? std::atoi(argv[3]) : 4;
        init_tables();
        std::FILE* f = std::fopen((path + ".idx").c_str(), "rb");
        std::uint64_t hdr[4];
        if (!f || std::fread(hdr, sizeof hdr, 1, f) != 1) { std::cerr << "cannot read index\n"; return 1; }
        std::vector<std::pair<std::uint64_t, std::uint64_t>> layer(hdr[2]);
        if (std::fread(layer.data(), sizeof layer[0], layer.size(), f) != layer.size()) return 1;
        std::fclose(f);
        if (!map_file(path, hdr[3], 0)) { std::cerr << "cannot map " << path << "\n"; return 1; }

        // the sampled slices, in index order, as two packed streams
        std::vector<std::size_t> pick;
        for (std::size_t j = 0; j < layer.size(); j += std::size_t(sample)) pick.push_back(j);
        std::vector<std::vector<std::uint8_t>> sym_raw(pick.size()), sym_dc(pick.size());
        #pragma omp parallel for schedule(dynamic, 64) num_threads(threads)
        for (std::size_t q = 0; q < pick.size(); ++q) {
                std::size_t j = pick[q];
                Meta m = meta_of(Key::unpack(layer[j].first));
                std::uint8_t const* v = db2 + (layer[j].second & ~(WDL_FLAG | DB2_FLAG));
                std::uint64_t T = 2 * m.P;
                int prev = -1;
                sym_raw[q].resize(T); sym_dc[q].resize(T);
                for (std::uint64_t i = 0; i < T; ++i) {
                        int s = v[i] == DRAW8 ? 2 : (v[i] & 1);
                        bool dc = has_capture(pos_of(m, i % m.P, int(i / m.P)));
                        int fill = dc && prev >= 0 ? prev : s;
                        sym_raw[q][i] = std::uint8_t(s); sym_dc[q][i] = std::uint8_t(fill);
                        prev = fill;
                }
        }
        auto pack = [](std::vector<std::vector<std::uint8_t>> const& sy) {
                std::vector<std::uint8_t> out;
                std::uint8_t b = 0; int k = 0;
                for (auto const& v : sy) for (auto x : v) { b |= std::uint8_t(x << (2 * k)); if (++k == 4) { out.push_back(b); b = 0; k = 0; } }
                if (k) out.push_back(b);
                return out;
        };
        std::vector<std::uint8_t> raw = pack(sym_raw), dcs = pack(sym_dc);
        sym_raw.clear(); sym_dc.clear();
        double npos = double(dcs.size()) * 4;
        std::printf("sample: every %d-th slice, %zu slices, %.0f positions, %zu bytes packed\n", sample, pick.size(), npos, dcs.size());
        auto bits = [&](std::size_t bytes) { return 8.0 * double(bytes) / npos; };

        std::printf("\n%-34s %10s %10s %10s %10s   (bits per position; 2 bits = uncompressed)\n", "", "4 KiB", "16 KiB", "64 KiB", "1 MiB");
        std::size_t blocks[4] = { 4096, 16384, 65536, 1 << 20 };
        auto row = [&](char const* name, auto&& fn) {
                std::printf("%-34s", name);
                for (auto b : blocks) { std::printf(" %10.3f", bits(fn(b))); std::fflush(stdout); }
                std::printf("\n");
        };
        row("zstd -3, as is", [&](std::size_t b) { return zstd_total(raw, b, 3, nullptr, threads); });
        row("zstd -3, captures filled", [&](std::size_t b) { return zstd_total(dcs, b, 3, nullptr, threads); });
        row("zstd -9, captures filled", [&](std::size_t b) { return zstd_total(dcs, b, 9, nullptr, threads); });
        row("zstd -19, captures filled", [&](std::size_t b) { return zstd_total(dcs, b, 19, nullptr, threads); });
        for (int level : { 9, 19 }) {
                std::string name = "zstd -" + std::to_string(level) + " + 100 KB dictionary";
                row(name.c_str(), [&](std::size_t b) {
                        auto d = train(dcs, b, 100'000);
                        return zstd_total(dcs, b, level, d.empty() ? nullptr : &d, threads) + d.size();
                });
        }
        row("xz -9, captures filled", [&](std::size_t b) { return xz_total(dcs, b, threads); });
        return 0;
}
