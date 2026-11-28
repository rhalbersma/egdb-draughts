//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// stress test of SharedBlockCache: many threads, a cache much smaller than the data, every value checked.
//   block_cache_test [threads] [blocks] [slots] [reads per thread]
#include "block_cache.hpp"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

constexpr std::size_t B = 8192;
static std::uint8_t value(std::uint64_t id, std::size_t off) { return std::uint8_t((id * 0x9E3779B97F4A7C15ull >> 32) + off * 131); }

int main(int argc, char** argv)
{
        int const threads = argc > 1 ? std::atoi(argv[1]) : 8;
        std::uint64_t const blocks = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 20000;
        std::uint64_t const slots = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 257;
        std::uint64_t const reads = argc > 4 ? std::strtoull(argv[4], nullptr, 10) : 2000000;
        SharedBlockCache<B> cache(slots * B);
        std::atomic<std::uint64_t> loads{ 0 }, bad{ 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t)
                pool.emplace_back([&, t] {
                        std::mt19937_64 rng(std::uint64_t(t) + 1);
                        std::uint64_t wrong = 0, mine = 0;
                        for (std::uint64_t i = 0; i < reads; ++i) {
                                // half the reads in a hot range (shared blocks), half anywhere (evictions)
                                std::uint64_t const id = (rng() & 1) ? rng() % 64 : rng() % blocks;
                                std::size_t const off = rng() % B;
                                std::uint8_t const got = cache.read(id,
                                        [&](std::uint8_t* dst) { ++mine; for (std::size_t k = 0; k < B; ++k) dst[k] = value(id, k); },
                                        [&](std::uint8_t const* blk) { return blk[off]; });
                                wrong += got != value(id, off);
                        }
                        bad += wrong; loads += mine;
                });
        for (auto& th : pool) th.join();
        std::printf("%d threads, %llu blocks in %zu slots, %llu reads each: %llu loads, %llu wrong values\n", threads,
                    static_cast<unsigned long long>(blocks), cache.slots(), static_cast<unsigned long long>(reads),
                    static_cast<unsigned long long>(loads.load()), static_cast<unsigned long long>(bad.load()));
        return bad.load() == 0 ? 0 : 1;
}
