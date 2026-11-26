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
// 65,535 positions. There is one grammar per file, built by this one procedure: no catalogue, no permutations, no
// search. The file is then a sequence of one-byte codewords.
//
// The layout, for lookups straight from the mapped file (no decompression, no cache):
//   header    magic, positions, blocks, symbols; left[256], right[256] (a rule: symbol = left right), length[256]
//             (positions a symbol stands for); start[blocks + 1] (the first position of each block)
//   blocks    4 KiB each: 256 bytes of counts (for each of the 60 mini-blocks, the positions of the block before it,
//             32 bits; 16 bytes unused), then 60 mini-blocks of 64 codewords: a mini-block is one cache line
// A lookup: the block by a binary search over start; the mini-block by comparing the 60 counts; the codeword by
// adding lengths over at most 64 bytes; the value by descending the rules. Three or four cache lines in all.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rpvf {

constexpr std::uint64_t BLOCK = 4096, MINI = 64, HEAD = 256, MINIS = (BLOCK - HEAD) / MINI, CODES = MINIS * MINI;
constexpr char MAGIC[8] = { 'E', 'G', 'D', 'B', 'R', 'P', '1', 0 };
constexpr std::uint64_t OFF_LEFT = 32, OFF_RIGHT = OFF_LEFT + 256, OFF_LEN = OFF_RIGHT + 256, OFF_START = OFF_LEN + 512;

struct File {
        std::uint64_t positions = 0, blocks = 0;
        std::uint8_t const* left = nullptr;
        std::uint8_t const* right = nullptr;
        std::uint16_t const* length = nullptr;
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
        f.length = reinterpret_cast<std::uint16_t const*>(p + OFF_LEN);
        f.start = reinterpret_cast<std::uint64_t const*>(p + OFF_START);
        f.data = p + (OFF_START + 8 * (f.blocks + 1) + BLOCK - 1) / BLOCK * BLOCK;
        return f;
}

// the value of position p (p < positions)
inline std::uint8_t lookup(File const& f, std::uint64_t p)
{
        std::uint64_t const b = std::uint64_t(std::upper_bound(f.start, f.start + f.blocks, p) - f.start) - 1;
        std::uint32_t rel = std::uint32_t(p - f.start[b]);
        std::uint8_t const* const blk = f.data + b * BLOCK;
        std::uint32_t h[MINIS];
        std::memcpy(h, blk, sizeof h);
        unsigned m = 0;
        for (unsigned k = 1; k < MINIS; ++k) m += h[k] <= rel;         // the counts never decrease: the last one not above rel
        rel -= h[m];
        std::uint8_t const* const c = blk + HEAD + MINI * m;
        unsigned i = 0;
        for (std::uint32_t l; rel >= (l = f.length[c[i]]); ++i) rel -= l;
        unsigned s = c[i];
        while (s >= 4) {                                                // descend the rules to the one value
                unsigned const L = f.left[s];
                std::uint32_t const ll = f.length[L];
                if (rel < ll) s = L; else { rel -= ll; s = f.right[s]; }
        }
        return std::uint8_t(s);
}

// values: one 2-bit value per element (0..3). Returns the file's bytes.
inline std::vector<std::uint8_t> compress(std::vector<std::uint8_t> seq)
{
        std::uint64_t const positions = seq.size();
        std::array<std::uint8_t, 256> left{}, right{};
        std::array<std::uint16_t, 256> length{};
        for (unsigned s = 0; s < 4; ++s) { left[s] = right[s] = std::uint8_t(s); length[s] = 1; }
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
                                if (count[a << 8 | c] > best && unsigned(length[a]) + length[c] <= 65535) { best = count[a << 8 | c]; key = a << 8 | c; }
                if (best < 2) break;
                std::uint8_t const a = std::uint8_t(key >> 8), c = std::uint8_t(key & 255);
                left[nsym] = a; right[nsym] = c; length[nsym] = std::uint16_t(length[a] + length[c]);
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
        std::memcpy(out.data() + OFF_LEN, length.data(), 512);
        std::uint64_t pos = 0;
        for (std::uint64_t b = 0; b < blocks; ++b) {
                std::memcpy(out.data() + OFF_START + 8 * b, &pos, 8);
                std::uint8_t* const blk = out.data() + data_at + b * BLOCK;
                std::uint32_t in_block = 0;
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
