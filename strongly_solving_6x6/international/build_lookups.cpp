//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// How many lookups does building a slice take, and how well would a block cache serve them? A sample of the slices of
// a layer (e.g. the 10-piece slices, with the layers below loaded and the layer itself attached) is solved again with
// a copy of the engine's solve_slice in which every lookup outside the slice (captures, man moves, promotions) is
// counted: phase 1 is the first pass over all positions, phase 2 the checks of the loss candidates. Each lookup is
// mapped to its 4 KiB block of 2-bit values (16,384 positions) in the stored layout, and fed to LRU caches of
// 1 ... 4096 blocks per slice (a cache that starts empty at every slice, as a pessimistic case). The solved values
// are checked against the stored ones.
// With compressed storage the positions where the side to move can capture are stored as "don't care": a lookup that
// hits such a position resolves it by playing its captures, recursively until positions without a capture. That
// variant is counted too (all captures resolved, no cut-off: an upper bound), with its own LRU caches.
//
// build: g++-14 -std=c++23 -O2 -fopenmp build_lookups.cpp -o build_lookups
// usage: build_lookups data/6x6_le9.db data/6x6_10.dtm [sample=200] [threads=4]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

#include <list>
#include <omp.h>

namespace {

constexpr std::uint64_t BLOCK = 16384;               // positions per 4 KiB block of 2-bit values
constexpr int NC = 7;
constexpr std::size_t CACHES[NC] = { 1, 8, 64, 256, 512, 1024, 4096 };

struct Lru {
        std::size_t cap;
        std::list<std::uint64_t> order;
        std::unordered_map<std::uint64_t, std::list<std::uint64_t>::iterator> where;
        bool touch(std::uint64_t b)
        {
                auto it = where.find(b);
                if (it != where.end()) { order.splice(order.begin(), order, it->second); return true; }
                order.push_front(b);
                where[b] = order.begin();
                if (order.size() > cap) { where.erase(order.back()); order.pop_back(); }
                return false;
        }
};

struct Counts {
        std::uint64_t positions = 0, slices = 0, look[3]{}, hits[NC]{}, distinct = 0;
        std::uint64_t capture_targets = 0, resolved = 0, rhits[NC]{};
        void add(Counts const& o)
        {
                positions += o.positions; slices += o.slices; distinct += o.distinct;
                capture_targets += o.capture_targets; resolved += o.resolved;
                for (int i = 0; i < 3; ++i) look[i] += o.look[i];
                for (int i = 0; i < NC; ++i) { hits[i] += o.hits[i]; rhits[i] += o.rhits[i]; }
        }
};

thread_local Counts* cnt = nullptr;
thread_local std::vector<Lru>* lrus = nullptr;
thread_local std::vector<Lru>* rlrus = nullptr;
thread_local std::unordered_map<std::uint64_t, char>* seen = nullptr;

// the global block of a position in the stored layout: (storage, offset / BLOCK)
std::uint64_t block_of(Pos const& p0)
{
        Pos const p = canonical_key(key_of(p0)) ? p0 : mirror(p0);
        Key const key = key_of(p);
        Meta const m = meta_of(key);
        SliceRef r;
        if (!find_prebuilt(key.pack(), r)) return ~0ull;
        std::uint64_t tag, base;
        if (r.dtm && r.dtm >= db.data() && r.dtm < db.data() + db.size()) { tag = 0; base = std::uint64_t(r.dtm - db.data()); }
        else if (r.dtm) { tag = 1; base = std::uint64_t(r.dtm - db2); }
        else { tag = 2; base = r.wdl; }
        return tag << 60 | (base + std::uint64_t(p.stm) * m.P + index_of(m, p)) / BLOCK;
}

// the stored lookups needed for p when capture positions are don't-cares: resolve captures until quiet positions
void resolve(Pos const& p)
{
        if (!has_capture(p)) {
                ++cnt->resolved;
                std::uint64_t b = block_of(p);
                for (int i = 0; i < NC; ++i) cnt->rhits[i] += (*rlrus)[i].touch(b);
                return;
        }
        for (auto const& mv : moves(p)) resolve(mv.next);       // captures are compulsory: every move is a capture
}

std::uint16_t counted(Pos const& p, int phase)
{
        ++cnt->look[phase];
        std::uint64_t b = block_of(p);
        if (seen->emplace(b, 1).second) ++cnt->distinct;
        for (int i = 0; i < NC; ++i) cnt->hits[i] += (*lrus)[i].touch(b);
        if (has_capture(p)) ++cnt->capture_targets;
        resolve(p);
        return lookup(p);
}

// ---------------------------------------------------------------- a copy of solve_slice with counted lookups

void solve_slice_counted(Meta const& s, std::uint16_t* out)
{
        std::uint64_t T = 2 * s.P;
        std::vector<std::uint16_t> ewin(T, NONE16);
        int maxd = 0;
        for (std::uint64_t i = 0; i < T; ++i) {
                Pos p = pos_of(s, i % s.P, int(i / s.P));
                int c = 0, ew = INF, el = 0;
                bool ed = false;
                for (auto const& mv : moves(p)) {
                        if (internal_move(p, mv)) { ++c; continue; }
                        std::uint16_t v = counted(mv.next, 1);
                        if (v == DRAW) ed = true;
                        else if (v & 1) el = std::max(el, v + 1);
                        else ew = std::min(ew, v + 1);
                }
                if (c == 0) {
                        out[i] = ew < INF ? std::uint16_t(ew) : ed ? DRAW : std::uint16_t(el);
                        if (out[i] != DRAW) maxd = std::max(maxd, int(out[i]));
                } else {
                        out[i] = UNKNOWN16;
                        if (ew < INF) { ewin[i] = std::uint16_t(ew); maxd = std::max(maxd, ew); }
                }
        }
        if (maxd >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
        std::vector<std::uint64_t> cand((T + 63) / 64);
        for (int n = 1; n <= maxd + 1; ++n) {
                std::uint16_t const prev = std::uint16_t(n - 1);
                bool const wins = n & 1;
                for (std::uint64_t i = 0; i < T; ++i) {
                        if (out[i] != prev) continue;
                        Pos q = pos_of(s, i % s.P, int(i / s.P));
                        for_each_predecessor(s, q, [&](std::uint64_t j) {
                                if (out[j] != UNKNOWN16) return;
                                if (wins) { out[j] = std::uint16_t(n); maxd = std::max(maxd, n); }
                                else cand[j / 64] |= 1ull << (j % 64);
                        });
                }
                if (wins) {
                        for (std::uint64_t i = 0; i < T; ++i)
                                if (ewin[i] == n && out[i] == UNKNOWN16) { out[i] = std::uint16_t(n); maxd = std::max(maxd, n); }
                        continue;
                }
                for (std::uint64_t w = 0; w < cand.size(); ++w) {
                        for (std::uint64_t b = cand[w]; b; b &= b - 1) {
                                std::uint64_t j = w * 64 + std::uint64_t(__builtin_ctzll(b));
                                if (out[j] != UNKNOWN16) continue;
                                Pos p = pos_of(s, j % s.P, int(j / s.P));
                                int worst = 0;
                                bool lost = true;
                                for (auto const& mv : moves(p)) {
                                        std::uint16_t v = internal_move(p, mv)
                                                ? out[std::uint64_t(1 - p.stm) * s.P + index_of(s, mv.next)]
                                                : counted(mv.next, 2);
                                        if (v >= UNKNOWN16 || !(v & 1)) { lost = false; break; }
                                        worst = std::max(worst, int(v));
                                }
                                if (lost) {
                                        out[j] = std::uint16_t(worst + 1);
                                        maxd = std::max(maxd, worst + 1);
                                }
                        }
                        cand[w] = 0;
                }
                if (maxd >= 0xFF00) { std::cerr << "DTM overflow" << std::endl; std::abort(); }
        }
        for (std::uint64_t i = 0; i < T; ++i)
                if (out[i] == UNKNOWN16) out[i] = DRAW;
}

}   // namespace

int main(int argc, char** argv)
{
        if (argc < 3) { std::cerr << "usage: build_lookups <db> <layer> [sample] [threads]\n"; return 1; }
        int sample = argc > 3 ? std::atoi(argv[3]) : 200;
        int threads = argc > 4 ? std::atoi(argv[4]) : 4;
        init_tables();
        if (!load(argv[1]) || !attach(argv[2])) { std::cerr << "cannot load the databases\n"; return 1; }
        allow_on_demand = false;
        std::FILE* f = std::fopen((std::string(argv[2]) + ".idx").c_str(), "rb");
        std::uint64_t hdr[4];
        if (!f || std::fread(hdr, sizeof hdr, 1, f) != 1) return 1;
        std::vector<std::pair<std::uint64_t, std::uint64_t>> layer(hdr[2]);
        if (std::fread(layer.data(), sizeof layer[0], layer.size(), f) != layer.size()) return 1;
        std::fclose(f);
        std::cerr << "loaded; " << layer.size() << " slices in the layer, sampling every " << sample << "-th\n";

        std::vector<Counts> per(threads);
        #pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
        for (std::size_t j = 0; j < layer.size(); j += std::size_t(sample)) {
                int t = omp_get_thread_num();
                std::vector<Lru> l, rl;
                for (auto c : CACHES) { l.push_back(Lru{ c, {}, {} }); rl.push_back(Lru{ c, {}, {} }); }
                rlrus = &rl;
                std::unordered_map<std::uint64_t, char> sn;
                cnt = &per[t]; lrus = &l; seen = &sn;
                Meta m = meta_of(Key::unpack(layer[j].first));
                std::vector<std::uint16_t> out(2 * m.P);
                solve_slice_counted(m, out.data());
                std::uint8_t const* v = db2 + (layer[j].second & ~(WDL_FLAG | DB2_FLAG));
                for (std::uint64_t i = 0; i < 2 * m.P; ++i)
                        if ((out[i] == DRAW ? DRAW8 : std::uint8_t(out[i])) != v[i]) { std::cerr << "mismatch\n"; std::abort(); }
                per[t].positions += 2 * m.P;
                ++per[t].slices;
        }
        Counts c;
        for (auto const& x : per) c.add(x);
        double n = double(c.positions), L = double(c.look[1] + c.look[2]);
        std::printf("slices %llu, positions %llu (every %d-th slice of the layer; values checked against the stored ones)\n",
                    (unsigned long long)c.slices, (unsigned long long)c.positions, sample);
        std::printf("lookups outside the slice per position: phase 1 %.2f, phase 2 %.2f, total %.2f\n",
                    double(c.look[1]) / n, double(c.look[2]) / n, L / n);
        std::printf("distinct 4 KiB blocks per slice: %.1f (%.3f per lookup)\n", double(c.distinct) / double(c.slices), double(c.distinct) / L);
        std::printf("\nLRU cache per slice (blocks of 4 KiB): hit rate, and estimated time per position at 6 us per miss, 0.05 us per hit\n");
        for (int i = 0; i < NC; ++i) {
                double h = double(c.hits[i]) / L, miss = L - double(c.hits[i]);
                std::printf("  %5zu blocks (%6.1f MiB decoded): hit %.4f, %.2f us per position\n", CACHES[i],
                            double(CACHES[i]) * 4 / 1024, h, (miss * 6 + double(c.hits[i]) * 0.05) / n);
        }
        double R = double(c.resolved);
        std::printf("\nwith capture positions stored as don't care: %.1f%% of the lookups hit a capture position;\n"
                    "resolving captures until quiet positions takes %.2f stored lookups per lookup, %.2f per position\n",
                    100 * double(c.capture_targets) / L, R / L, R / n);
        for (int i = 0; i < NC; ++i) {
                double h = double(c.rhits[i]) / R, miss = R - double(c.rhits[i]);
                std::printf("  %5zu blocks: hit %.4f, %.2f us per position\n", CACHES[i], h, (miss * 6 + double(c.rhits[i]) * 0.05) / n);
        }
        return 0;
}
