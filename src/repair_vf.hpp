//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// A variable-to-fixed code for value files with a small alphabet (the 2-bit values of a wdl file): Re-Pair with
// byte codewords.
//
// The grammar: starting from the values themselves (symbols 0..3), the most frequent pair of adjacent symbols becomes
// a new symbol, again and again, until there are 256 symbols (or no pair occurs twice). A symbol stands for at most
// 524,288 positions. There is one grammar per file, built by this one procedure: no catalogue, no permutations, no
// search. The file is then a sequence of one-byte codewords.
//
// The layout, for lookups straight from the mapped file (no decompression, no cache):
//   header    magic, positions, blocks, symbols; left[256], right[256] (a rule: symbol = left right), length[256]
//             (positions a symbol stands for), uniform[256] (its value if it is a run of one value); start[blocks + 1]
//             (the first position of each block)
//   blocks    4 KiB each: 256 bytes of counts (for each of the 60 mini-blocks, the positions of the block before it,
//             32 bits; the 4 unused counts are 2^31 - 1), then 60 mini-blocks of 64 codewords: one cache line each
// A lookup: the block by a binary search over start; the mini-block by comparing the counts; the codeword by adding
// lengths over at most 64 bytes; the value at once if the symbol is a run of one value, else by descending the rules.

#if defined(__AVX2__)
#include <immintrin.h>
#endif
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rpvf {

constexpr std::uint64_t BLOCK = 4096, MINI = 64, HEAD = 256, MINIS = (BLOCK - HEAD) / MINI, CODES = MINIS * MINI;
constexpr char MAGIC[8] = { 'E', 'G', 'D', 'B', 'R', 'P', '1', 0 };
constexpr std::uint64_t OFF_LEFT = 32, OFF_RIGHT = OFF_LEFT + 256, OFF_LEN = OFF_RIGHT + 256, OFF_UNI = OFF_LEN + 1024, OFF_START = OFF_UNI + 256;
// a symbol stands for at most MAXLEN positions: 2^19, so that a block of 3,840 codewords stays below 2^31 positions
// (the counts in a block are compared as signed 32-bit numbers)
constexpr std::uint32_t MAXLEN = std::uint32_t(1) << 19;
constexpr std::uint8_t MIXED = 0xff;                            // uniform[s]: the symbol is not a run of one value

struct File {
        std::uint64_t positions = 0, blocks = 0;
        std::uint8_t const* left = nullptr;
        std::uint8_t const* right = nullptr;
        std::uint32_t const* length = nullptr;
        std::uint8_t const* uniform = nullptr;                  // the value of a symbol that is a run of one value, else MIXED
        std::uint64_t const* start = nullptr;
        std::uint8_t const* data = nullptr;
        explicit operator bool() const { return data != nullptr; }
};

inline File open(std::uint8_t const* p)
{
        File f;
        if (std::memcmp(p, MAGIC, 8) != 0) return f;
        std::memcpy(&f.positions, p + 8, 8);
        std::memcpy(&f.blocks, p + 16, 8);
        f.left = p + OFF_LEFT;
        f.right = p + OFF_RIGHT;
        f.length = reinterpret_cast<std::uint32_t const*>(p + OFF_LEN);
        f.uniform = p + OFF_UNI;
        f.start = reinterpret_cast<std::uint64_t const*>(p + OFF_START);
        f.data = p + (OFF_START + 8 * (f.blocks + 1) + BLOCK - 1) / BLOCK * BLOCK;
        return f;
}

// the value of position p (p < positions): the plain version, as a reference for the tuned one below
inline std::uint8_t lookup_plain(File const& f, std::uint64_t p)
{
        std::uint64_t const b = std::uint64_t(std::upper_bound(f.start, f.start + f.blocks, p) - f.start) - 1;
        std::uint32_t rel = std::uint32_t(p - f.start[b]);
        std::uint8_t const* const blk = f.data + b * BLOCK;
        std::uint32_t h[MINIS];
        std::memcpy(h, blk, sizeof h);
        unsigned m = 0;
        for (unsigned k = 1; k < MINIS; ++k) m += h[k] <= rel;
        rel -= h[m];
        std::uint8_t const* const c = blk + HEAD + MINI * m;
        unsigned i = 0;
        for (std::uint32_t l; rel >= (l = f.length[c[i]]); ++i) rel -= l;
        unsigned s = c[i];
        while (s >= 4) {
                unsigned const L = f.left[s];
                std::uint32_t const ll = f.length[L];
                if (rel < ll) s = L; else { rel -= ll; s = f.right[s]; }
        }
        return std::uint8_t(s);
}

// The tuned lookup:
//   block       a binary search without branches (conditional moves)
//   mini-block  the 64 counts (60 and 4 fillers that never match) compared 8 at a time: the number not above rel
//   codeword    the lengths of 8 codewords gathered at a time, their running sums compared with rel
//   value       a symbol that is a run of one value answers at once (most positions lie in runs); else down the rules
// With AVX2; without it the same steps one element at a time.
inline std::uint8_t lookup(File const& f, std::uint64_t p)
{
        std::uint64_t const* base = f.start;
        for (std::uint64_t n = f.blocks; n > 1;) {
                std::uint64_t const half = n / 2;
                base = base[half] <= p ? base + half : base;
                n -= half;
        }
        std::uint32_t rel = std::uint32_t(p - *base);
        std::uint8_t const* const blk = f.data + std::uint64_t(base - f.start) * BLOCK;
        unsigned s;
#if defined(__AVX2__)
        __m256i const r = _mm256_set1_epi32(int(rel));
        unsigned not_above = 0;
        for (unsigned k = 0; k < 8; ++k) {
                __m256i const h = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(blk + 32 * k));
                not_above += unsigned(std::popcount(~unsigned(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(h, r)))) & 0xffu));
        }
        unsigned const m = not_above - 1;
        std::uint32_t before;
        std::memcpy(&before, blk + 4 * m, 4);
        rel -= before;
        std::uint8_t const* const c = blk + HEAD + MINI * m;
        for (unsigned g = 0;; g += 8) {
                __m256i const idx = _mm256_cvtepu8_epi32(_mm_loadl_epi64(reinterpret_cast<__m128i const*>(c + g)));
                __m256i sum = _mm256_i32gather_epi32(reinterpret_cast<int const*>(f.length), idx, 4);
                sum = _mm256_add_epi32(sum, _mm256_slli_si256(sum, 4));          // running sums inside each half
                sum = _mm256_add_epi32(sum, _mm256_slli_si256(sum, 8));
                __m128i const low_total = _mm_shuffle_epi32(_mm256_castsi256_si128(sum), 0xff);
                sum = _mm256_add_epi32(sum, _mm256_inserti128_si256(_mm256_setzero_si256(), low_total, 1));   // carried into the high half
                unsigned const above = unsigned(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(sum, _mm256_set1_epi32(int(rel))))));
                alignas(32) std::uint32_t t[8];
                _mm256_store_si256(reinterpret_cast<__m256i*>(t), sum);
                if (above != 0) {
                        unsigned const i = unsigned(std::countr_zero(above));
                        rel -= i ? t[i - 1] : 0;
                        s = c[g + i];
                        break;
                }
                rel -= t[7];
        }
#else
        std::uint32_t h[MINIS];
        std::memcpy(h, blk, sizeof h);
        unsigned m = 0;
        for (unsigned k = 1; k < MINIS; ++k) m += h[k] <= rel;
        rel -= h[m];
        std::uint8_t const* const c = blk + HEAD + MINI * m;
        unsigned i = 0;
        for (std::uint32_t l; rel >= (l = f.length[c[i]]); ++i) rel -= l;
        s = c[i];
#endif
        for (;;) {
                std::uint8_t const u = f.uniform[s];
                if (u != MIXED) return u;
                unsigned const L = f.left[s];
                std::uint32_t const ll = f.length[L];
                bool const in_left = rel < ll;
                s = in_left ? L : f.right[s];
                rel -= in_left ? 0 : ll;
        }
}

// values: one 2-bit value per element (0..3). Returns the file's bytes.
inline std::vector<std::uint8_t> compress(std::vector<std::uint8_t> seq)
{
        std::uint64_t const positions = seq.size();
        std::array<std::uint8_t, 256> left{}, right{};
        std::array<std::uint32_t, 256> length{};
        std::array<std::uint8_t, 256> uniform;
        uniform.fill(MIXED);
        for (unsigned s = 0; s < 4; ++s) { left[s] = right[s] = std::uint8_t(s); length[s] = 1; uniform[s] = std::uint8_t(s); }
        std::vector<std::uint32_t> count(65536);
        for (unsigned nsym = 4; nsym < 256; ++nsym) {
                std::fill(count.begin(), count.end(), 0);
                bool skip = false;                                      // aaa holds one pair aa, not two
                for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
                        if (skip) { skip = false; if (seq[i] == seq[i + 1] && seq[i - 1] == seq[i]) continue; }
                        ++count[std::size_t(seq[i]) << 8 | seq[i + 1]];
                        skip = seq[i] == seq[i + 1];
                }
                std::uint32_t best = 1; unsigned key = 0;
                for (unsigned a = 0; a < nsym; ++a)
                        for (unsigned c = 0; c < nsym; ++c)
                                if (count[a << 8 | c] > best && length[a] + length[c] <= MAXLEN) { best = count[a << 8 | c]; key = a << 8 | c; }
                if (best < 2) break;
                std::uint8_t const a = std::uint8_t(key >> 8), c = std::uint8_t(key & 255);
                left[nsym] = a; right[nsym] = c; length[nsym] = length[a] + length[c];
                uniform[nsym] = uniform[a] == uniform[c] ? uniform[a] : MIXED;
                std::size_t o = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        if (i + 1 < seq.size() && seq[i] == a && seq[i + 1] == c) { seq[o++] = std::uint8_t(nsym); ++i; }
                        else seq[o++] = seq[i];
                }
                seq.resize(o);
        }
        std::uint64_t const blocks = (seq.size() + CODES - 1) / CODES;
        std::uint64_t const data_at = (OFF_START + 8 * (blocks + 1) + BLOCK - 1) / BLOCK * BLOCK;
        std::vector<std::uint8_t> out(data_at + blocks * BLOCK, 0);
        std::memcpy(out.data(), MAGIC, 8);
        std::memcpy(out.data() + 8, &positions, 8);
        std::memcpy(out.data() + 16, &blocks, 8);
        std::memcpy(out.data() + OFF_LEFT, left.data(), 256);
        std::memcpy(out.data() + OFF_RIGHT, right.data(), 256);
        std::memcpy(out.data() + OFF_LEN, length.data(), 1024);
        std::memcpy(out.data() + OFF_UNI, uniform.data(), 256);
        std::uint64_t pos = 0;
        for (std::uint64_t b = 0; b < blocks; ++b) {
                std::memcpy(out.data() + OFF_START + 8 * b, &pos, 8);
                std::uint8_t* const blk = out.data() + data_at + b * BLOCK;
                std::uint32_t in_block = 0;
                std::uint32_t const never = 0x7fffffff;                 // the 4 unused counts: above every position
                for (std::uint64_t m = MINIS; m < MINIS + 4; ++m) std::memcpy(blk + 4 * m, &never, 4);
                for (std::uint64_t m = 0; m < MINIS; ++m) {
                        std::memcpy(blk + 4 * m, &in_block, 4);
                        for (std::uint64_t k = 0; k < MINI; ++k) {
                                std::uint64_t const i = b * CODES + m * MINI + k;
                                if (i >= seq.size()) continue;         // the unused tail stays 0 and is never reached
                                blk[HEAD + m * MINI + k] = seq[i];
                                in_block += length[seq[i]];
                        }
                }
                pos += in_block;
        }
        std::memcpy(out.data() + OFF_START + 8 * blocks, &pos, 8);
        return out;
}

}       // namespace rpvf
