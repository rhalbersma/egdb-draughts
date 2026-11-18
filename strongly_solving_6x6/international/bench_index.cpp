//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// The king index of a 6x6 slice (index_of / pos_of of egdb_6x6) computed in several ways, checked for equality and
// timed. A slice fixes the men; its index ranks the white kings within the free squares and the black kings within
// the squares still free (colex ranks), as egdb_6x6 does.
//
// The bit permutation (the kings packed onto consecutive bits of the free squares, and back):
//   xstd      xstd::bit_compress / xstd::bit_expand on std::uint32_t
//   native    _pext_u32 / _pdep_u32 written by hand (only with BMI2): the reference the xstd primitives must match
//   set       bitboards as xstd::bit_align<xstd::bit_least<xstd::bit_fixed_set<18>>> (one std::uint32_t block, all 32
//             bits), with xstd::bit_compress<std::uint32_t> / xstd::bit_expand
// The colex rank and unrank of the packed kings:
//   loop      rank: one binomial per king; unrank: a linear search up the binomials per king (egdb_6x6's search)
//   table     one table of 2^18 ranks and one of the subsets in rank order (1 MB each): only for small boards
//   chunk     rank: per byte of the packed kings a table of its contribution, given the kings below it
//             (3 bytes x 19 x 256 entries); unrank: a branchless binary search in the binomials per king.
//             Both scale to 8x8 and 10x10, where 2^32 or 2^50 tables cannot exist.
//   simd      the chunked rank; unrank: per king count at once how many C(p, i) are <= r (vector compares)
// plus "loops", the code of egdb_6x6 as it is: rank and unrank by walking the free squares without any packing.
//
// xstd-ints runs bit_compress and bit_expand as PEXT and PDEP where the target makes them fast (BMI2, not AMD before
// Zen 3) and as portable kernels everywhere else (build with -mno-bmi2 to see those).
// The positions are random slices of 6x6 international draughts (men off their promotion rows, at most 6 pieces a
// side and 12 in all) with random kings on the free squares.
//
// usage: bench_index [samples=4000000] [repeats=5]
// build: cmake --build build --target bench_index_6x6   (then build/bench_index_6x6), or by hand with GCC 14 or later:
//        g++ -std=c++23 -O3 -DNDEBUG -march=native -I<xstd-bits>/include -I<xstd-ints>/include -I<xstd-misc>/include
//        bench_index.cpp -o bench_index          (add -mno-bmi2 for the portable kernels)

#include <xstd/bits/bit/bit_compress.hpp>          // bit_compress
#include <xstd/bits/bit/bit_expand.hpp>            // bit_expand
#include <xstd/bits/bit_ranges/from_blocks.hpp>    // from_blocks
#include <xstd/bits/bit_set/bit_fixed_set.hpp>     // basic_bit_fixed_set, bit_fixed_set
#include <xstd/bits/bit_type_traits/bit_align.hpp> // bit_align
#include <xstd/bits/bit_type_traits/bit_least.hpp> // bit_least
#include <xstd/ints/bit/countr_zero.hpp>           // countr_zero
#include <xstd/ints/bit/popcount.hpp>              // popcount

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
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

// binom[n][k] = C(n, k); column[k][p] = C(p, k), contiguous in p for the binary search
std::array<std::array<std::uint32_t, N + 1>, N + 1> const binom = [] {
        std::array<std::array<std::uint32_t, N + 1>, N + 1> b{};
        for (int n = 0; n <= N; ++n) {
                b[n][0] = 1;
                for (int k = 1; k <= n; ++k) b[n][k] = b[n - 1][k - 1] + (k <= n - 1 ? b[n - 1][k] : 0);
        }
        return b;
}();

std::array<std::array<std::uint32_t, N + 1>, N + 1> const column = [] {
        std::array<std::array<std::uint32_t, N + 1>, N + 1> c{};
        for (int k = 0; k <= N; ++k) for (int p = 0; p <= N; ++p) c[k][p] = binom[p][k];
        return c;
}();

// ================================================================ loops: the code of egdb_6x6 as it is

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

// ================================================================ ranks of packed kings (c: k bits within n positions)

// loop: the i-th lowest king at position p adds C(p, i); unrank by a linear search per king
struct Loop {
        static std::uint32_t rank(std::uint32_t c)
        {
                std::uint32_t r = 0;
                for (int i = 1; c; c &= c - 1, ++i) r += binom[xstd::countr_zero(c)][i];
                return r;
        }
        static std::uint32_t unrank(std::uint32_t r, int k, int n)
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
};

// table: the colex rank of a packed subset does not depend on n, so 2^18 ranks serve every mask, and per number of
// kings one table of the subsets in rank order (2^18 entries over all k)
std::vector<std::uint32_t> const rank_table = [] {
        std::vector<std::uint32_t> t(std::size_t{1} << N);
        for (std::uint32_t c = 0; c < t.size(); ++c) t[c] = Loop::rank(c);
        return t;
}();

std::array<std::uint32_t, N + 2> const unrank_offset = [] {
        std::array<std::uint32_t, N + 2> o{};
        for (int k = 0; k <= N; ++k) o[k + 1] = o[k] + binom[N][k];
        return o;
}();

std::vector<std::uint32_t> const unrank_table = [] {
        std::vector<std::uint32_t> t(std::size_t{1} << N);
        for (std::uint32_t c = 0; c < t.size(); ++c) t[unrank_offset[xstd::popcount(c)] + rank_table[c]] = c;
        return t;
}();

struct Table {
        static std::uint32_t rank(std::uint32_t c) { return rank_table[c]; }
        static std::uint32_t unrank(std::uint32_t r, int k, int /* n */) { return unrank_table[unrank_offset[k] + r]; }
};

// chunk: the rank is a sum over the kings, so per byte of c its share is a table entry, given how many kings lie
// below that byte; the unrank finds per king the highest p with C(p, i) <= r by a branchless binary search
constexpr int CHUNK = 8;
constexpr int CHUNKS = (N + CHUNK - 1) / CHUNK;

using chunk_table_t = std::array<std::array<std::array<std::uint32_t, 1 << CHUNK>, N + 1>, CHUNKS>;
chunk_table_t const chunk_table = [] {
        chunk_table_t t{};
        for (int j = 0; j < CHUNKS; ++j)
                for (int below = 0; below <= N; ++below)
                        for (std::uint32_t byte = 0; byte < (1u << CHUNK); ++byte) {
                                std::uint32_t r = 0;
                                int i = below;
                                for (std::uint32_t b = byte; b; b &= b - 1) {
                                        int const p = j * CHUNK + xstd::countr_zero(b);
                                        ++i;
                                        if (p <= N && i <= N) r += binom[p][i];
                                }
                                t[j][below][byte] = r;
                        }
        return t;
}();

struct Chunk {
        static std::uint32_t rank(std::uint32_t c)
        {
                std::uint32_t r = 0;
                int below = 0;
                for (int j = 0; j < CHUNKS; ++j) {      // unrolled: CHUNKS is a constant
                        std::uint32_t const byte = c >> (j * CHUNK) & ((1u << CHUNK) - 1);
                        r += chunk_table[j][below][byte];
                        below += xstd::popcount(byte);
                }
                return r;
        }
        static std::uint32_t unrank(std::uint32_t r, int k, int n)
        {
                std::uint32_t c = 0;
                for (int i = k; i >= 1; --i) {
                        // the highest p in [i - 1, n - 1] with C(p, i) <= r: C(i - 1, i) = 0 <= r always holds
                        std::uint32_t const* base = column[i].data() + (i - 1);
                        int len = n - (i - 1);
                        while (len > 1) {
                                int const half = len / 2;
                                base += (base[half] <= r) ? half : 0;
                                len -= half;
                        }
                        int const pos = static_cast<int>(base - column[i].data());
                        r -= *base;
                        c |= 1u << pos;
                        n = pos;
                }
                return c;
        }
};

// simd: the chunked rank, and per king the position found by counting at once how many C(p, i) are <= r. The row is
// nondecreasing in p (C(p, i) = 0 for p < i) and r < C(n, i), so that count minus one is the position: no search,
// no branch, 3 vector comparisons of 8 lanes for 18 squares (7 for 50). GCC vector extensions, AVX2 with -march.
constexpr int LANES = 8;
constexpr int VECS = (N + 1 + LANES - 1) / LANES;
using v8u = std::uint32_t __attribute__((vector_size(LANES * sizeof(std::uint32_t))));

struct alignas(32) Rows { std::array<std::array<std::uint32_t, VECS * LANES>, N + 1> row; };
Rows const simd_rows = [] {
        Rows r{};
        for (int k = 0; k <= N; ++k)
                for (int p = 0; p < VECS * LANES; ++p) r.row[k][p] = p <= N ? binom[p][k] : ~std::uint32_t{0};
        return r;
}();

struct Simd {
        static std::uint32_t rank(std::uint32_t c) { return Chunk::rank(c); }
        static std::uint32_t unrank(std::uint32_t r, int k, int /* n */)
        {
                std::uint32_t c = 0;
                for (int i = k; i >= 1; --i) {
                        v8u const rv = v8u{} + r;
                        v8u count{};
                        for (int v = 0; v < VECS; ++v) {
                                v8u row;
                                __builtin_memcpy(&row, simd_rows.row[i].data() + v * LANES, sizeof row);
                                count -= (row <= rv);   // a true lane is all ones: -1
                        }
                        std::uint32_t lanes = 0;
                        for (int l = 0; l < LANES; ++l) lanes += count[l];
                        int const pos = static_cast<int>(lanes) - 1;
                        r -= binom[pos][i];
                        c |= 1u << pos;
                }
                return c;
        }
};

// ================================================================ the bit permutations

struct Xstd {
        static std::uint32_t compress(std::uint32_t x, std::uint32_t m) { return xstd::bit_compress(x, m); }
        static std::uint32_t expand(std::uint32_t x, std::uint32_t m) { return xstd::bit_expand(x, m); }
};

#ifdef __BMI2__
struct Native {
        static std::uint32_t compress(std::uint32_t x, std::uint32_t m) { return _pext_u32(x, m); }
        static std::uint32_t expand(std::uint32_t x, std::uint32_t m) { return _pdep_u32(x, m); }
};
#endif

template<class P, class R>
std::uint32_t rank_u(std::uint32_t x, std::uint32_t mask) { return R::rank(P::compress(x, mask)); }

template<class P, class R>
std::uint32_t unrank_u(std::uint32_t r, int k, std::uint32_t mask) { return P::expand(R::unrank(r, k, xstd::popcount(mask)), mask); }

// bitboards as sets: 18 squares in one std::uint32_t block (bit_least), widened to its 32 bits (bit_align)
using set = xstd::bit_align<xstd::bit_least<xstd::bit_fixed_set<N>>>;
static_assert(std::is_same_v<set, xstd::basic_bit_fixed_set<std::size_t, std::uint32_t, 32>>);

set to_set(std::uint32_t x) { return set(xstd::from_blocks, x); }

std::uint32_t to_u32(set const& x)
{
        static set const full = to_set(FULL);
        return xstd::bit_compress<std::uint32_t>(x, full);
}

set minus(set a, set const& b)
{
        a -= b;
        return a;
}

template<class R>
std::uint32_t rank_s(set const& x, set const& mask) { return R::rank(xstd::bit_compress<std::uint32_t>(x, mask)); }

template<class R>
set unrank_s(std::uint32_t r, int k, set const& mask) { return xstd::bit_expand(R::unrank(r, k, static_cast<int>(mask.size())), mask); }

// ================================================================ the positions

struct Sample {
        std::uint32_t free, wk, bk;   // free squares, white kings, black kings
        int nwk, nbk;
        std::uint32_t nb;             // C(free - nwk, nbk): the black kings' share of the index
};

std::uint32_t random_subset(std::mt19937_64& g, std::uint32_t mask, int k)
{
        std::uint32_t x = 0;
        for (int i = 0; i < k; ++i) {
                int const left = xstd::popcount(mask & ~x);
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
                int const nw = 1 + static_cast<int>(g() % 6), nb = 1 + static_cast<int>(g() % 6);
                if (nw + nb > 12) continue;
                int const nwm = static_cast<int>(g() % static_cast<std::uint64_t>(nw + 1));
                int const nbm = static_cast<int>(g() % static_cast<std::uint64_t>(nb + 1));
                std::uint32_t const wm = random_subset(g, FULL & ~PROMO_WHITE, nwm);
                if (xstd::popcount(FULL & ~PROMO_BLACK & ~wm) < nbm) continue;
                std::uint32_t const bm = random_subset(g, FULL & ~PROMO_BLACK & ~wm, nbm);
                std::uint32_t const free = FULL & ~(wm | bm);
                int const nwk = nw - nwm, nbk = nb - nbm;
                std::uint32_t const wk = random_subset(g, free, nwk);
                std::uint32_t const bk = random_subset(g, free & ~wk, nbk);
                s.push_back({ free, wk, bk, nwk, nbk, binom[xstd::popcount(free) - nwk][nbk] });
        }
        return s;
}

// ================================================================ checks

// a plain deposit for the exhaustive check, independent of the code under test
std::uint32_t deposit(std::uint32_t c, std::uint32_t mask)
{
        std::uint32_t x = 0;
        for (std::uint32_t b = mask; b; b &= b - 1, c >>= 1) if (c & 1) x |= b & -b;
        return x;
}

// every variant: rank equals the loops' rank, and unrank gives the kings back
template<class F>
void for_each_variant(F&& f)
{
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Xstd, Loop>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Xstd, Loop>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Xstd, Table>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Xstd, Table>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Xstd, Chunk>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Xstd, Chunk>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Xstd, Simd>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Xstd, Simd>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_s<Simd>(to_set(x), to_set(m)); }, [](std::uint32_t r, int k, std::uint32_t m) { return to_u32(unrank_s<Simd>(r, k, to_set(m))); });
#ifdef __BMI2__
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Native, Loop>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Native, Loop>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Native, Table>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Native, Table>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Native, Chunk>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Native, Chunk>(r, k, m); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_u<Native, Simd>(x, m); }, [](std::uint32_t r, int k, std::uint32_t m) { return unrank_u<Native, Simd>(r, k, m); });
#endif
        f([](std::uint32_t x, std::uint32_t m) { return rank_s<Loop>(to_set(x), to_set(m)); }, [](std::uint32_t r, int k, std::uint32_t m) { return to_u32(unrank_s<Loop>(r, k, to_set(m))); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_s<Table>(to_set(x), to_set(m)); }, [](std::uint32_t r, int k, std::uint32_t m) { return to_u32(unrank_s<Table>(r, k, to_set(m))); });
        f([](std::uint32_t x, std::uint32_t m) { return rank_s<Chunk>(to_set(x), to_set(m)); }, [](std::uint32_t r, int k, std::uint32_t m) { return to_u32(unrank_s<Chunk>(r, k, to_set(m))); });
}

int check(std::vector<Sample> const& s)
{
        long bad = 0, exhaustive = 0;
        for (auto const& x : s) {
                std::uint32_t const rest = x.free & ~x.wk;
                std::uint64_t const rw = rank_in(x.wk, x.free), rb = rank_in(x.bk, rest);
                bad += unrank_in(rw, x.nwk, x.free) != x.wk || unrank_in(rb, x.nbk, rest) != x.bk;
                for_each_variant([&](auto rank, auto unrank) {
                        bad += rank(x.wk, x.free) != rw || rank(x.bk, rest) != rb;
                        bad += unrank(static_cast<std::uint32_t>(rw), x.nwk, x.free) != x.wk || unrank(static_cast<std::uint32_t>(rb), x.nbk, rest) != x.bk;
                });
#ifdef __BMI2__
                // the xstd primitives are the instructions themselves
                bad += xstd::bit_compress(x.wk, x.free) != _pext_u32(x.wk, x.free);
                bad += xstd::bit_expand(xstd::bit_compress(x.bk, rest), rest) != _pdep_u32(_pext_u32(x.bk, rest), rest);
#endif
        }
        // exhaustively: every subset of every size of a few masks
        for (std::uint32_t mask : { FULL, FULL & ~PROMO_WHITE, 0b101101011011011010u, 0b000111111000111000u }) {
                int const f = xstd::popcount(mask);
                for (std::uint32_t c = 0; c < (1u << f); ++c) {
                        std::uint32_t const x = deposit(c, mask);
                        int const k = xstd::popcount(c);
                        std::uint64_t const r = rank_in(x, mask);
                        ++exhaustive;
                        bad += unrank_in(r, k, mask) != x;
                        for_each_variant([&](auto rank, auto unrank) {
                                bad += rank(x, mask) != r || unrank(static_cast<std::uint32_t>(r), k, mask) != x;
                        });
                }
        }
        std::printf("check: %zu random slices, %ld exhaustive subsets, all variants: %s\n", s.size(), exhaustive, bad ? "MISMATCH" : "all identical");
        return bad ? 1 : 0;
}

// ================================================================ timing

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
        std::vector<std::uint32_t> idx(s.size());
        for (std::size_t i = 0; i < s.size(); ++i)
                idx[i] = static_cast<std::uint32_t>(rank_in(s[i].wk, s[i].free) * s[i].nb + rank_in(s[i].bk, s[i].free & ~s[i].wk));
        struct SetSample { set free, wk, bk; };
        std::vector<SetSample> ss(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) ss[i] = { to_set(s[i].free), to_set(s[i].wk), to_set(s[i].bk) };

        std::printf("\n%-14s %10s %10s %12s   (ns per position, best of %d over %zu positions)\n", "", "index_of", "pos_of", "round trip", repeats, s.size());
        auto report = [&](char const* name, auto&& index_of, auto&& pos_of) {
                double const a = time_ns(s.size(), repeats, [&] { std::uint64_t t = 0; for (std::size_t i = 0; i < s.size(); ++i) t += index_of(i); sink = t; });
                double const b = time_ns(s.size(), repeats, [&] { std::uint64_t t = 0; for (std::size_t i = 0; i < s.size(); ++i) t += pos_of(i); sink = t; });
                std::printf("%-14s %10.2f %10.2f %12.2f\n", name, a, b, a + b);
        };
        auto report_u = [&]<class P, class R>(char const* name) {
                report(name,
                       [&](std::size_t i) { auto const& x = s[i]; return std::uint64_t{rank_u<P, R>(x.wk, x.free)} * x.nb + rank_u<P, R>(x.bk, x.free & ~x.wk); },
                       [&](std::size_t i) { auto const& x = s[i]; std::uint32_t const wk = unrank_u<P, R>(idx[i] / x.nb, x.nwk, x.free);
                                            return std::uint64_t{wk} ^ unrank_u<P, R>(idx[i] % x.nb, x.nbk, x.free & ~wk); });
        };
        auto report_s = [&]<class R>(char const* name) {
                report(name,
                       [&](std::size_t i) { auto const& x = ss[i]; return std::uint64_t{rank_s<R>(x.wk, x.free)} * s[i].nb + rank_s<R>(x.bk, minus(x.free, x.wk)); },
                       [&](std::size_t i) { auto const& x = ss[i]; set const wk = unrank_s<R>(idx[i] / s[i].nb, s[i].nwk, x.free);
                                            return std::uint64_t{wk.size()} ^ unrank_s<R>(idx[i] % s[i].nb, s[i].nbk, minus(x.free, wk)).size(); });
        };
        report("loops",
               [&](std::size_t i) { auto const& x = s[i]; return rank_in(x.wk, x.free) * x.nb + rank_in(x.bk, x.free & ~x.wk); },
               [&](std::size_t i) { auto const& x = s[i]; std::uint32_t const wk = unrank_in(idx[i] / x.nb, x.nwk, x.free);
                                    return std::uint64_t{wk} ^ unrank_in(idx[i] % x.nb, x.nbk, x.free & ~wk); });
        report_u.template operator()<Xstd, Loop>("xstd   + loop");
#ifdef __BMI2__
        report_u.template operator()<Native, Loop>("native + loop");
#endif
        report_s.template operator()<Loop>("set    + loop");
        report_u.template operator()<Xstd, Table>("xstd   + table");
#ifdef __BMI2__
        report_u.template operator()<Native, Table>("native + table");
#endif
        report_s.template operator()<Table>("set    + table");
        report_u.template operator()<Xstd, Chunk>("xstd   + chunk");
#ifdef __BMI2__
        report_u.template operator()<Native, Chunk>("native + chunk");
#endif
        report_s.template operator()<Chunk>("set    + chunk");
        report_u.template operator()<Xstd, Simd>("xstd   + simd");
#ifdef __BMI2__
        report_u.template operator()<Native, Simd>("native + simd");
#endif
        report_s.template operator()<Simd>("set    + simd");
        return 0;
}
