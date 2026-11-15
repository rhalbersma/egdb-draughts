//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// An independent check of the compressed units against the slice databases of the earlier storage: all slices with
// at most n pieces are built in memory as before (build, one byte of DTM per position, indexed by slice), and every
// stored position of every unit with at most n pieces is looked up in both: through the compressed unit (with the
// captures resolved) and in its slice of the in-memory database. The two storages share the solver, but not the
// indexing, the order of building, the mirroring, the compression or the resolution of captures.
//
// build: g++-14 -std=c++23 -O2 -fopenmp units_crosscheck.cpp -lzstd -o units_crosscheck
// usage: units_crosscheck data/units [n=8] [threads=4]

#define main egdb_6x6_main
#include "../../src/egdb_6x6.cpp"
#undef main

int main(int argc, char** argv)
{
        if (argc < 2) { std::cerr << "usage: units_crosscheck <dir> [n] [threads]\n"; return 1; }
        std::string dir = argv[1];
        int nmax = argc > 2 ? std::atoi(argv[2]) : 8;
        int threads = argc > 3 ? std::atoi(argv[3]) : 4;
        init_tables();
        build(nmax, threads, 99);                         // the earlier storage, in memory
        if (!store_open(dir, nmax + 1, 0, -1)) { std::cerr << "cannot open the units\n"; return 1; }
        std::uint64_t total = 0, errors = 0;
        for (int n = 2; n <= nmax; ++n)
                for (int m = 0; m <= n; ++m) {
                        auto const lay = unit_layout(n, m);
                        if (lay.empty()) continue;
                        std::vector<std::uint64_t> c(2 * std::size_t(threads), 0);
                        for_each_stored(lay, threads, "crosscheck", [&](Cursor const& cur, std::uint64_t) {
                                Pos const q = cur.pos();
                                std::uint64_t* t = &c[2 * std::size_t(omp_get_thread_num())];
                                ++t[0];
                                Pos const p = canonical_key(key_of(q)) ? q : mirror(q);
                                Meta const s = meta_of(key_of(p));
                                SliceRef r;
                                if (!find_prebuilt(key_of(p).pack(), r)) { std::cerr << "no slice\n"; std::abort(); }
                                std::uint16_t const a = ref_value(r, std::uint64_t(p.stm) * s.P + index_of(s, p)), b = store_lookup(q);
                                if (a != b && ++t[1] <= 10) {
                                        #pragma omp critical
                                        std::cerr << fen_of(q) << ": slices " << show(a) << ", units " << show(b) << std::endl;
                                }
                        });
                        std::uint64_t u = 0, e = 0;
                        for (int i = 0; i < threads; ++i) { u += c[2 * i]; e += c[2 * i + 1]; }
                        std::printf("unit %2d/%-2d %12llu positions, %llu differences\n", n, m, (unsigned long long)u, (unsigned long long)e);
                        std::fflush(stdout);
                        total += u; errors += e;
                }
        std::printf("total %llu positions, %llu differences\n", (unsigned long long)total, (unsigned long long)errors);
        return errors != 0;
}
