//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// How well do the 6x6 databases compress? Measured on a layer made by `extend` (one byte of DTM per position,
// e.g. data/6x6_10.dtm with its .idx), reduced to win / draw / loss:
//   * the distribution of win / draw / loss, and the fraction of positions where the side to move can capture
//     ("don't care": their value follows from one ply of captures, so a compressor may store anything there);
//   * slices whose positions all have one value (ignoring the don't-cares), which can be stored as one number;
//   * slices with more than 6 pieces on one side (superfluous: each side starts with 6 pieces);
//   * the order-0 and order-1 entropy of the win / draw / loss stream (bits per position), with the don't-cares
//     filled in with the previous value, as a bound for an arithmetic coder;
//   * xz (preset 9) and zlib (level 9) on blocks of 1 MiB of 2-bit packed values, for a sample of the slices.
//
// build: g++-14 -std=c++23 -O2 -fopenmp compression_stats.cpp -llzma -lz -o compression_stats
// usage: compression_stats data/6x6_10.dtm [sample=16] [threads=4]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

#include <cmath>
#include <lzma.h>
#include <omp.h>
#include <zlib.h>

namespace {

struct Stats {
        std::uint64_t n[3][2]{};             // [loss, win, draw][capture]
        std::uint64_t slices = 0, single = 0, single_pos = 0, over6 = 0, over6_pos = 0;
        std::uint64_t t1[3][3]{};            // order-1 transitions of the stream with don't-cares filled in
        std::uint64_t raw_bytes = 0, xz_raw = 0, xz_dc = 0, zl_raw = 0, zl_dc = 0;
        void add(Stats const& o)
        {
                for (int a = 0; a < 3; ++a) for (int b = 0; b < 2; ++b) n[a][b] += o.n[a][b];
                for (int a = 0; a < 3; ++a) for (int b = 0; b < 3; ++b) t1[a][b] += o.t1[a][b];
                slices += o.slices; single += o.single; single_pos += o.single_pos; over6 += o.over6; over6_pos += o.over6_pos;
                raw_bytes += o.raw_bytes; xz_raw += o.xz_raw; xz_dc += o.xz_dc; zl_raw += o.zl_raw; zl_dc += o.zl_dc;
        }
};

std::size_t xz_size(std::vector<std::uint8_t> const& in)
{
        std::vector<std::uint8_t> out(lzma_stream_buffer_bound(in.size()));
        std::size_t pos = 0;
        if (lzma_easy_buffer_encode(9, LZMA_CHECK_NONE, nullptr, in.data(), in.size(), out.data(), &pos, out.size()) != LZMA_OK) std::abort();
        return pos;
}

std::size_t zlib_size(std::vector<std::uint8_t> const& in)
{
        uLongf len = compressBound(in.size());
        std::vector<std::uint8_t> out(len);
        if (compress2(out.data(), &len, in.data(), in.size(), 9) != Z_OK) std::abort();
        return len;
}

// pack 2-bit symbols, 4 per byte, into blocks of 1 MiB; compress every full block
struct Packer {
        std::vector<std::uint8_t> raw, dc;
        std::uint8_t br = 0, bd = 0;
        int k = 0;
        void put(int sraw, int sdc, Stats& st)
        {
                br |= std::uint8_t(sraw << (2 * k)); bd |= std::uint8_t(sdc << (2 * k));
                if (++k == 4) { raw.push_back(br); dc.push_back(bd); br = bd = 0; k = 0; }
                if (raw.size() == (1u << 20)) flush(st);
        }
        void flush(Stats& st)
        {
                if (raw.empty()) return;
                st.raw_bytes += raw.size();
                st.xz_raw += xz_size(raw); st.xz_dc += xz_size(dc);
                st.zl_raw += zlib_size(raw); st.zl_dc += zlib_size(dc);
                raw.clear(); dc.clear();
        }
};

double entropy0(std::uint64_t const* c, int k)
{
        double tot = 0, h = 0;
        for (int i = 0; i < k; ++i) tot += double(c[i]);
        for (int i = 0; i < k; ++i) if (c[i]) h -= double(c[i]) / tot * std::log2(double(c[i]) / tot);
        return h;
}

}   // namespace

int main(int argc, char** argv)
{
        if (argc < 2) { std::cerr << "usage: compression_stats <layer> [sample] [threads]\n"; return 1; }
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
        std::cerr << layer.size() << " slices, " << hdr[3] << " positions (" << hdr[1] << " pieces)\n";

        std::vector<Stats> per(threads);
        std::vector<Packer> pk(threads);
        #pragma omp parallel for schedule(dynamic, 256) num_threads(threads)
        for (std::size_t j = 0; j < layer.size(); ++j) {
                int t = omp_get_thread_num();
                Stats& st = per[t];
                Key key = Key::unpack(layer[j].first);
                Meta m = meta_of(key);
                std::uint8_t const* v = db2 + (layer[j].second & ~(WDL_FLAG | DB2_FLAG));
                std::uint64_t T = 2 * m.P;
                bool sampled = (j % std::size_t(sample)) == 0;
                int first = -1, prev = -1;
                bool one = true;
                for (std::uint64_t i = 0; i < T; ++i) {
                        int s = v[i] == DRAW8 ? 2 : (v[i] & 1);
                        bool dc = has_capture(pos_of(m, i % m.P, int(i / m.P)));
                        ++st.n[s][dc];
                        int fill = s;
                        if (dc) fill = prev >= 0 ? prev : s;
                        else {
                                if (first < 0) first = s;
                                else if (s != first) one = false;
                        }
                        if (prev >= 0) ++st.t1[prev][fill];
                        prev = fill;
                        if (sampled) pk[t].put(s, fill, st);
                }
                ++st.slices;
                if (one) { ++st.single; st.single_pos += T; }
                if (__builtin_popcount(key.wm) + key.nwk > 6 || __builtin_popcount(key.bm) + key.nbk > 6) { ++st.over6; st.over6_pos += T; }
        }
        Stats tot;
        for (int t = 0; t < threads; ++t) { pk[t].flush(per[t]); tot.add(per[t]); }

        std::uint64_t all = 0, dcs = 0, sym[3]{};
        for (int a = 0; a < 3; ++a) { sym[a] = tot.n[a][0] + tot.n[a][1]; all += sym[a]; dcs += tot.n[a][1]; }
        auto pct = [&](std::uint64_t x, std::uint64_t y) { return 100.0 * double(x) / double(y); };
        std::printf("positions          %llu (both sides to move, canonical slices)\n", (unsigned long long)all);
        std::printf("loss / win / draw  %.2f%% / %.2f%% / %.2f%%\n", pct(sym[0], all), pct(sym[1], all), pct(sym[2], all));
        std::printf("capture (don't care) %.2f%% of all positions\n", pct(dcs, all));
        std::printf("slices             %llu, of which single-valued %llu (%.2f%% of the slices, %.2f%% of the positions)\n",
                    (unsigned long long)tot.slices, (unsigned long long)tot.single, pct(tot.single, tot.slices), pct(tot.single_pos, all));
        std::printf("more than 6 pieces on one side: %llu slices, %.2f%% of the positions\n", (unsigned long long)tot.over6, pct(tot.over6_pos, all));
        double h0 = entropy0(sym, 3);
        double h1 = 0, n1 = 0;
        for (int a = 0; a < 3; ++a) { double r = 0; for (int b = 0; b < 3; ++b) r += double(tot.t1[a][b]); n1 += r; h1 += r * entropy0(tot.t1[a], 3); }
        h1 /= n1;
        std::printf("entropy            order-0 %.3f bits, order-1 (don't cares filled) %.3f bits per position\n", h0, h1);
        double ps = double(tot.raw_bytes) * 4;
        std::printf("sample (every %d-th slice): %.0f positions\n", sample, ps);
        std::printf("  xz -9   raw %.3f bits, don't cares filled %.3f bits per position\n", 8 * double(tot.xz_raw) / ps, 8 * double(tot.xz_dc) / ps);
        std::printf("  zlib -9 raw %.3f bits, don't cares filled %.3f bits per position\n", 8 * double(tot.zl_raw) / ps, 8 * double(tot.zl_dc) / ps);
        return 0;
}
