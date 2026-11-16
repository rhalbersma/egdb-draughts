//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// The king index of a 6x6 slice (index_of / pos_of of egdb_6x6) computed three ways, checked for equality and timed:
//
//   loops     the code of egdb_6x6: colex rank and unrank by walking the free squares (rank_in, unrank_in)
//   ints      the same ranks over the packed squares, with xstd::bit_compress / xstd::bit_expand on std::uint32_t
//   fixed_set the same with the bitboards as xstd::bit_align<xstd::bit_least<xstd::bit_fixed_set<18>>>: one
//             std::uint32_t block, all 32 bits; bit_compress<std::uint32_t> and bit_expand
//   tables    ints with the colex rank and unrank of the packed squares looked up in two tables of 2^18 entries
//   set+tab   fixed_set with the same tables
//
// xstd-ints runs bit_compress and bit_expand as PEXT and PDEP where the target makes them fast (BMI2, not AMD before
// Zen 3) and as a portable kernel everywhere else; with BMI2 they are also checked against _pext_u32 / _pdep_u32.
// The positions are random slices of 6x6 international draughts (men off their promotion rows, at most 6 pieces a
// side and 12 in all) with random kings on the free squares, as egdb_6x6 indexes them.
//
// usage: bench_index [samples=4000000] [repeats=5]
// build: cmake --build build --target bench_index_6x6   (then build/bench_index_6x6), or by hand with GCC 14 or later:
//        g++ -std=c++23 -O3 -DNDEBUG -march=native -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include
//        bench_index.cpp -o bench_index          (add -mno-bmi2 for the portable kernels)

#include <xstd/bits/bit/bit_compress.hpp>       // bit_compress
#include <xstd/bits/bit/bit_expand.hpp>         // bit_expand
#include <xstd/bits/bit_ranges/from_blocks.hpp> // from_blocks
#include <xstd/bits/bit_set/bit_fixed_set.hpp>  // bit_fixed_set
#include <xstd/bits/bit_type_traits/bit_align.hpp> // bit_align
#include <xstd/bits/bit_type_traits/bit_least.hpp> // bit_least

#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __BMI2__
#include <immintrin.h>
#endif

namespace {

constexpr int N = 18;
constexpr std::uint32_t FULL = (1u << N) - 1;
constexpr std::uint32_t PROMO_WHITE = 0b111;          // squares 1-3: no white man there
constexpr std::uint32_t PROMO_BLACK = 0b111u << 15;   // squares 16-18: no black man there

std::array<std::array<std::uint64_t, N + 1>, N + 1> const binom = [] {
        std::array<std::array<std::uint64_t, N + 1>, N + 1> b{};
        for (int n = 0; n <= N; ++n) {
                b[n][0] = 1;
                for (int k = 1; k <= n; ++k) b[n][k] = b[n - 1][k - 1] + (k <= n - 1 ? b[n - 1][k] : 0);
        }
        return b;
}();

// ---------------------------------------------------------------- loops: the code of egdb_6x6

std::uint64_t rank_in(std::uint32_t x, std::uint32_t mask)
{
        std::uint64_t r = 0;
        int pos = 0, i = 0;
        for (std::uint32_t b = mask; b; b &= b - 1, ++pos) {
                int sq = __builtin_ctz(b);
                if (x >> sq & 1) { ++i; r += binom[pos][i]; }
        }
        return r;
}

std::uint32_t unrank_in(std::uint64_t r, int k, std::uint32_t mask)
{
        int sq[N]; int n = 0;
        for (std::uint32_t b = mask; b; b &= b - 1) sq[n++] = __builtin_ctz(b);
        std::uint32_t x = 0;
        for (int i = k; i >= 1; --i) {
                int pos = i - 1;
                while (pos + 1 < n && binom[pos + 1][i] <= r) ++pos;
                r -= binom[pos][i];
                x |= 1u << sq[pos];
                n = pos;
        }
        return x;
}

// ---------------------------------------------------------------- the ranks over packed squares (shared)

// colex rank of a packed subset: the i-th lowest bit at position p adds C(p, i)
std::uint64_t rank_packed(std::uint32_t c)
{
        std::uint64_t r = 0;
        for (int i = 1; c; c &= c - 1, ++i) r += binom[std::countr_zero(c)][i];
        return r;
}

// the packed subset of k positions out of n with colex rank r
std::uint32_t unrank_packed(std::uint64_t r, int k, int n)
{
        std::uint32_t c = 0;
        for (int i = k; i >= 1; --i) {
                int pos = i - 1;
                while (pos + 1 < n && binom[pos + 1][i] <= r) ++pos;
                r -= binom[pos][i];
                c |= 1u << pos;
                n = pos;
        }
        return c;
}

// ---------------------------------------------------------------- tables over the packed squares
// The colex rank of a packed subset does not depend on how many squares there are, so one table of 2^18 ranks serves
// every mask, and per number of kings k one table of the subsets in rank order (2^18 entries over all k): 1 MB each.

std::vector<std::uint32_t> const rank_table = [] {
        std::vector<std::uint32_t> t(std::size_t{1} << N);
        for (std::uint32_t c = 0; c < t.size(); ++c) t[c] = static_cast<std::uint32_t>(rank_packed(c));
        return t;
}();

std::array<std::size_t, N + 2> const unrank_offset = [] {
        std::array<std::size_t, N + 2> o{};
        for (int k = 0; k <= N; ++k) o[k + 1] = o[k] + binom[N][k];
        return o;
}();

std::vector<std::uint32_t> const unrank_table = [] {
        std::vector<std::uint32_t> t(std::size_t{1} << N);
        for (std::uint32_t c = 0; c < t.size(); ++c) t[unrank_offset[std::popcount(c)] + rank_table[c]] = c;
        return t;
}();

// ---------------------------------------------------------------- ints: xstd::bit_compress / bit_expand on uint32_t

std::uint64_t rank_ints(std::uint32_t x, std::uint32_t mask)
{
        return rank_packed(xstd::bit_compress(x, mask));
}

std::uint32_t unrank_ints(std::uint64_t r, int k, std::uint32_t mask)
{
        return xstd::bit_expand(unrank_packed(r, k, std::popcount(mask)), mask);
}

std::uint64_t rank_tab(std::uint32_t x, std::uint32_t mask)
{
        return rank_table[xstd::bit_compress(x, mask)];
}

std::uint32_t unrank_tab(std::uint64_t r, int k, std::uint32_t mask)
{
        return xstd::bit_expand(unrank_table[unrank_offset[k] + r], mask);
}

// ---------------------------------------------------------------- fixed_set: bitboards as xstd::bit_fixed_set

// 18 squares in one std::uint32_t block (bit_least), widened to its 32 bits (bit_align): no tail to mask off
using set = xstd::bit_align<xstd::bit_least<xstd::bit_fixed_set<N>>>;
static_assert(std::is_same_v<set, xstd::basic_bit_fixed_set<std::size_t, std::uint32_t, 32>>);

set to_set(std::uint32_t x)
{
        return set(xstd::from_blocks, x);
}

set minus(set a, set const& b)
{
        a -= b;
        return a;
}

std::uint32_t to_u32(set const& x)
{
        static set const full = to_set(FULL);
        return xstd::bit_compress<std::uint32_t>(x, full);
}

std::uint64_t rank_set(set const& x, set const& mask)
{
        return rank_packed(xstd::bit_compress<std::uint32_t>(x, mask));
}

set unrank_set(std::uint64_t r, int k, set const& mask)
{
        return xstd::bit_expand(unrank_packed(r, k, static_cast<int>(mask.size())), mask);
}

std::uint64_t rank_set_tab(set const& x, set const& mask)
{
        return rank_table[xstd::bit_compress<std::uint32_t>(x, mask)];
}

set unrank_set_tab(std::uint64_t r, int k, set const& mask)
{
        return xstd::bit_expand(unrank_table[unrank_offset[k] + r], mask);
}

// ---------------------------------------------------------------- the positions

struct Sample {
        std::uint32_t free, wk, bk;   // free squares, white kings, black kings
        int nwk, nbk;
        std::uint64_t nb;             // C(free - nwk, nbk): the black kings' share of the index
};

std::uint32_t random_subset(std::mt19937_64& g, std::uint32_t mask, int k)
{
        std::uint32_t x = 0;
        for (int i = 0; i < k; ++i) {
                int left = std::popcount(mask & ~x);
                int j = static_cast<int>(g() % static_cast<std::uint64_t>(left));
                std::uint32_t b = mask & ~x;
                while (j--) b &= b - 1;
                x |= b & -b;
        }
        return x;
}

std::vector<Sample> make_samples(std::size_t count)
{
        std::mt19937_64 g(20261116);
        std::vector<Sample> s;
        s.reserve(count);
        while (s.size() < count) {
                int nw = 1 + static_cast<int>(g() % 6), nb = 1 + static_cast<int>(g() % 6);
                if (nw + nb > 12) continue;
                int nwm = static_cast<int>(g() % static_cast<std::uint64_t>(nw + 1));
                int nbm = static_cast<int>(g() % static_cast<std::uint64_t>(nb + 1));
                std::uint32_t wm = random_subset(g, FULL & ~PROMO_WHITE, nwm);
                if (std::popcount(FULL & ~PROMO_BLACK & ~wm) < nbm) continue;
                std::uint32_t bm = random_subset(g, FULL & ~PROMO_BLACK & ~wm, nbm);
                std::uint32_t free = FULL & ~(wm | bm);
                int nwk = nw - nwm, nbk = nb - nbm;
                std::uint32_t wk = random_subset(g, free, nwk);
                std::uint32_t bk = random_subset(g, free & ~wk, nbk);
                int f = std::popcount(free);
                s.push_back({ free, wk, bk, nwk, nbk, binom[f - nwk][nbk] });
        }
        return s;
}

// ---------------------------------------------------------------- checks

// a plain deposit for the exhaustive check, independent of the code under test
std::uint32_t deposit(std::uint32_t c, std::uint32_t mask)
{
        std::uint32_t x = 0;
        for (std::uint32_t b = mask; b; b &= b - 1, c >>= 1) if (c & 1) x |= b & -b;
        return x;
}

int check(std::vector<Sample> const& s)
{
        long bad = 0;
        for (auto const& x : s) {
                std::uint32_t const rest = x.free & ~x.wk;
                // ranks
                std::uint64_t const rw = rank_in(x.wk, x.free), rb = rank_in(x.bk, rest);
                bad += rank_ints(x.wk, x.free) != rw || rank_ints(x.bk, rest) != rb;
                bad += rank_set(to_set(x.wk), to_set(x.free)) != rw || rank_set(to_set(x.bk), to_set(rest)) != rb;
                // unranks: back to the same kings
                bad += unrank_in(rw, x.nwk, x.free) != x.wk || unrank_in(rb, x.nbk, rest) != x.bk;
                bad += unrank_ints(rw, x.nwk, x.free) != x.wk || unrank_ints(rb, x.nbk, rest) != x.bk;
                bad += to_u32(unrank_set(rw, x.nwk, to_set(x.free))) != x.wk || to_u32(unrank_set(rb, x.nbk, to_set(rest))) != x.bk;
                bad += rank_tab(x.wk, x.free) != rw || rank_tab(x.bk, rest) != rb;
                bad += rank_set_tab(to_set(x.wk), to_set(x.free)) != rw || rank_set_tab(to_set(x.bk), to_set(rest)) != rb;
                bad += to_u32(unrank_set_tab(rw, x.nwk, to_set(x.free))) != x.wk || to_u32(unrank_set_tab(rb, x.nbk, to_set(rest))) != x.bk;
                bad += unrank_tab(rw, x.nwk, x.free) != x.wk || unrank_tab(rb, x.nbk, rest) != x.bk;
#ifdef __BMI2__
                // xstd's bit_compress / bit_expand are the instructions themselves
                bad += xstd::bit_compress(x.wk, x.free) != _pext_u32(x.wk, x.free);
                bad += xstd::bit_expand(xstd::bit_compress(x.bk, rest), rest) != _pdep_u32(_pext_u32(x.bk, rest), rest);
#endif
        }
        // exhaustively: every subset of every size of a few masks, rank and unrank against the loops
        long exhaustive = 0;
        for (std::uint32_t mask : { FULL, FULL & ~PROMO_WHITE, 0b101101011011011010u, 0b000111111000111000u }) {
                int const f = std::popcount(mask);
                for (std::uint32_t c = 0; c < (1u << f); ++c) {
                        std::uint32_t const x = deposit(c, mask);
                        int const k = std::popcount(c);
                        std::uint64_t const r = rank_in(x, mask);
                        exhaustive += 1;
                        bad += rank_ints(x, mask) != r || unrank_ints(r, k, mask) != x;
                        bad += rank_set(to_set(x), to_set(mask)) != r || to_u32(unrank_set(r, k, to_set(mask))) != x;
                        bad += rank_tab(x, mask) != r || unrank_tab(r, k, mask) != x;
                        bad += rank_set_tab(to_set(x), to_set(mask)) != r || to_u32(unrank_set_tab(r, k, to_set(mask))) != x;
                }
        }
        std::printf("check: %zu random slices, %ld exhaustive subsets: %s\n", s.size(), exhaustive, bad ? "MISMATCH" : "all identical");
        return bad ? 1 : 0;
}

// ---------------------------------------------------------------- timing

template<class F>
double time_ns(std::size_t ops, int repeats, F&& f)
{
        double best = 1e300;
        for (int r = 0; r < repeats; ++r) {
                auto const t0 = std::chrono::steady_clock::now();
                f();
                double const ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count();
                best = std::min(best, ns / static_cast<double>(ops));
        }
        return best;
}

volatile std::uint64_t sink;

}   // namespace

int main(int argc, char** argv)
{
        std::size_t const samples = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 4000000;
        int const repeats = argc > 2 ? std::atoi(argv[2]) : 5;
        auto const s = make_samples(samples);
#ifdef XSTD_INTS_HAS_FAST_BMI2
        std::printf("xstd-ints: bit_compress / bit_expand as PEXT / PDEP\n");
#else
        std::printf("xstd-ints: bit_compress / bit_expand as the portable kernels\n");
#endif
        if (int const rc = check(s)) return rc;

        // the inputs of each variant, prepared outside the timed loops
        std::vector<std::uint64_t> idx(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) idx[i] = rank_in(s[i].wk, s[i].free) * s[i].nb + rank_in(s[i].bk, s[i].free & ~s[i].wk);
        struct SetSample { set free, wk, bk; };
        std::vector<SetSample> ss(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) ss[i] = { to_set(s[i].free), to_set(s[i].wk), to_set(s[i].bk) };

        std::printf("\n%-10s %14s %14s %14s   (ns per position, best of %d over %zu positions)\n", "", "index_of", "pos_of", "round trip", repeats, s.size());
        auto report = [&](char const* name, auto&& index_of, auto&& pos_of) {
                double const a = time_ns(s.size(), repeats, [&] { std::uint64_t t = 0; for (std::size_t i = 0; i < s.size(); ++i) t += index_of(i); sink = t; });
                double const b = time_ns(s.size(), repeats, [&] { std::uint64_t t = 0; for (std::size_t i = 0; i < s.size(); ++i) t += pos_of(i); sink = t; });
                std::printf("%-10s %14.2f %14.2f %14.2f\n", name, a, b, a + b);
        };
        report("loops",
               [&](std::size_t i) { auto const& x = s[i]; return rank_in(x.wk, x.free) * x.nb + rank_in(x.bk, x.free & ~x.wk); },
               [&](std::size_t i) { auto const& x = s[i]; std::uint32_t const wk = unrank_in(idx[i] / x.nb, x.nwk, x.free);
                                    return std::uint64_t{wk} ^ unrank_in(idx[i] % x.nb, x.nbk, x.free & ~wk); });
        report("ints",
               [&](std::size_t i) { auto const& x = s[i]; return rank_ints(x.wk, x.free) * x.nb + rank_ints(x.bk, x.free & ~x.wk); },
               [&](std::size_t i) { auto const& x = s[i]; std::uint32_t const wk = unrank_ints(idx[i] / x.nb, x.nwk, x.free);
                                    return std::uint64_t{wk} ^ unrank_ints(idx[i] % x.nb, x.nbk, x.free & ~wk); });
        report("fixed_set",
               [&](std::size_t i) { auto const& x = ss[i]; return rank_set(x.wk, x.free) * s[i].nb + rank_set(x.bk, minus(x.free, x.wk)); },
               [&](std::size_t i) { auto const& x = ss[i]; set const wk = unrank_set(idx[i] / s[i].nb, s[i].nwk, x.free);
                                    return std::uint64_t{wk.size()} ^ unrank_set(idx[i] % s[i].nb, s[i].nbk, minus(x.free, wk)).size(); });
        report("tables",
               [&](std::size_t i) { auto const& x = s[i]; return rank_tab(x.wk, x.free) * x.nb + rank_tab(x.bk, x.free & ~x.wk); },
               [&](std::size_t i) { auto const& x = s[i]; std::uint32_t const wk = unrank_tab(idx[i] / x.nb, x.nwk, x.free);
                                    return std::uint64_t{wk} ^ unrank_tab(idx[i] % x.nb, x.nbk, x.free & ~wk); });
        report("set+tab",
               [&](std::size_t i) { auto const& x = ss[i]; return rank_set_tab(x.wk, x.free) * s[i].nb + rank_set_tab(x.bk, minus(x.free, x.wk)); },
               [&](std::size_t i) { auto const& x = ss[i]; set const wk = unrank_set_tab(idx[i] / s[i].nb, s[i].nwk, x.free);
                                    return std::uint64_t{wk.size()} ^ unrank_set_tab(idx[i] % s[i].nb, s[i].nbk, minus(x.free, wk)).size(); });
        return 0;
}
