//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// A variable-to-fixed code for value files: Re-Pair with byte codewords. For wdl files (4 values) and for depth files
// (the depths in use, each a 16-bit value in a table: no escape codes).
//
// The grammar: starting from the values in use (the terminals: symbols 0 .. T - 1), the most frequent pair of adjacent
// symbols becomes a new symbol, again and again, until there are 256 symbols (or no pair occurs twice). A symbol stands
// for at most 524,288 positions. There is one grammar per file, built by this one procedure: no catalogue, no
// permutations, no search. The file is then a sequence of one-byte codewords.
//
// The layout, for lookups straight from the mapped file (no decompression, no cache):
//   header    magic, positions, blocks, terminals; left[256], right[256] (a rule: symbol = left right), length[256]
//             (positions a symbol stands for), uniform[256] (the terminal it repeats if it is a run of one value),
//             value[256] (the value of each terminal); start[blocks + 1] (the first position of each block)
//   blocks    4 KiB each: 256 bytes of counts (for each of the 60 mini-blocks, the positions of the block before it,
//             32 bits; the 4 unused counts are 2^31 - 1), then 60 mini-blocks of 64 codewords: one cache line each
// A lookup: the block by a binary search over start; the mini-block by comparing the counts; the codeword by adding
// lengths over at most 64 bytes; the value at once if the symbol is a run of one value, else by descending the rules.

#if defined(__AVX2__)
#include <immintrin.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <functional>
#include <string>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rpvf {

constexpr std::uint64_t BLOCK = 4096, MINI = 64, HEAD = 256, MINIS = (BLOCK - HEAD) / MINI, CODES = MINIS * MINI;
constexpr char MAGIC[8] = { 'E', 'G', 'D', 'B', 'R', 'P', '2', 0 };
constexpr std::uint64_t OFF_LEFT = 40, OFF_RIGHT = OFF_LEFT + 256, OFF_LEN = OFF_RIGHT + 256, OFF_UNI = OFF_LEN + 1024, OFF_VALUE = OFF_UNI + 256,
                        OFF_START = OFF_VALUE + 512;
constexpr std::uint8_t MIXED = 0xff;                            // uniform[s]: the symbol is not a run of one value
// a symbol stands for at most MAXLEN positions: 2^19, so that a block of 3,840 codewords stays below 2^31 positions
// (the counts in a block are compared as signed 32-bit numbers)
constexpr std::uint32_t MAXLEN = std::uint32_t(1) << 19;

struct File {
        std::uint64_t positions = 0, blocks = 0;
        unsigned terminals = 0;                                 // the symbols below it are the values themselves
        std::uint8_t const* left = nullptr;
        std::uint8_t const* right = nullptr;
        std::uint32_t const* length = nullptr;
        std::uint8_t const* uniform = nullptr;                  // the terminal a symbol repeats if it is a run of one value, else MIXED
        std::uint16_t const* value = nullptr;                   // the value of each terminal (a wdl value, or a depth: no escape codes)
        std::uint64_t const* start = nullptr;
        std::uint8_t const* data = nullptr;
        explicit operator bool() const { return data != nullptr; }
};

inline File open(std::uint8_t const* p)
{
        File f;
        if (std::memcmp(p, MAGIC, 8) != 0) return f;
        std::uint64_t t;
        std::memcpy(&f.positions, p + 8, 8);
        std::memcpy(&f.blocks, p + 16, 8);
        std::memcpy(&t, p + 24, 8);
        f.terminals = unsigned(t);
        f.left = p + OFF_LEFT;
        f.right = p + OFF_RIGHT;
        f.length = reinterpret_cast<std::uint32_t const*>(p + OFF_LEN);
        f.uniform = p + OFF_UNI;
        f.value = reinterpret_cast<std::uint16_t const*>(p + OFF_VALUE);
        f.start = reinterpret_cast<std::uint64_t const*>(p + OFF_START);
        f.data = p + (OFF_START + 8 * (f.blocks + 1) + BLOCK - 1) / BLOCK * BLOCK;
        return f;
}

// the value of position p (p < positions): the plain version, as a reference for the tuned one below
inline std::uint16_t lookup_plain(File const& f, std::uint64_t p)
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
        while (s >= f.terminals) {
                unsigned const L = f.left[s];
                std::uint32_t const ll = f.length[L];
                if (rel < ll) s = L; else { rel -= ll; s = f.right[s]; }
        }
        return f.value[s];
}

// The tuned lookup:
//   block       a binary search without branches (conditional moves)
//   mini-block  the 64 counts (60 and 4 fillers that never match) compared 8 at a time: the number not above rel
//   codeword    the lengths of 8 codewords gathered at a time, their running sums compared with rel
//   value       a symbol that is a run of one value answers at once (most positions lie in runs); else down the rules
// With AVX2; without it the same steps one element at a time.
inline std::uint16_t lookup(File const& f, std::uint64_t p)
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
                if (u != MIXED) return f.value[u];
                unsigned const L = f.left[s];
                std::uint32_t const ll = f.length[L];
                bool const in_left = rel < ll;
                s = in_left ? L : f.right[s];
                rel -= in_left ? 0 : ll;
        }
}

// ---- the compressor
//
// The input is a sequence of terminals (0 .. terminals - 1: the ranks of the values in use; at most 255 of them, in
// practice a few dozen). It arrives through a Source, a function that feeds the sequence front to back in chunks and
// can be called again.
//   on disk    while the sequence is longer than `ram_symbols`, a round is one sequential read and one sequential
//              write: the round's new rules (a batch of the most frequent pairs with no symbol in common, so that
//              they cannot overlap) are applied and the pairs of the result counted on the way. Only the pair counts
//              (256 KB) are in memory. The first round reads the source and writes a temporary file; the later ones
//              rewrite that file in place.
//   in memory  once it fits: the exact method, the most frequent pair, one at a time.
using Feed = std::function<void(std::uint8_t const*, std::size_t)>;
using Source = std::function<void(Feed const&)>;

struct Grammar {
        unsigned terminals = 0, nsym = 0;
        std::array<std::uint8_t, 256> left{}, right{}, uniform{};
        std::array<std::uint32_t, 256> length{};
        explicit Grammar(unsigned t) : terminals(t), nsym(t)
        {
                uniform.fill(MIXED);
                for (unsigned s = 0; s < t; ++s) { left[s] = right[s] = std::uint8_t(s); length[s] = 1; uniform[s] = std::uint8_t(s); }
        }
        std::uint8_t add(unsigned a, unsigned c)
        {
                left[nsym] = std::uint8_t(a); right[nsym] = std::uint8_t(c); length[nsym] = length[a] + length[c];
                uniform[nsym] = uniform[a] == uniform[c] ? uniform[a] : MIXED;
                return std::uint8_t(nsym++);
        }
};

// counts the pairs of a sequence as it passes by (aaa holds one pair aa, not two)
struct PairCounter {
        std::vector<std::uint32_t> count = std::vector<std::uint32_t>(65536, 0);
        std::uint64_t n = 0;
        int prev = -1, prev2 = -1;
        bool skip = false;
        void put(std::uint8_t x)
        {
                if (prev >= 0) {
                        bool counted = true;
                        if (skip) { skip = false; if (prev == x && prev2 == prev) counted = false; }
                        if (counted) { ++count[std::size_t(prev) << 8 | x]; skip = prev == x; }
                }
                prev2 = prev; prev = x; ++n;
        }
};

struct Stats { int disk_rounds = 0; std::uint64_t disk_bytes = 0, ram_symbols_at_switch = 0; };

inline std::vector<std::uint8_t> compress(Source const& source, std::uint64_t positions, unsigned terminals, std::array<std::uint16_t, 256> const& value,
                                          std::uint64_t ram_symbols, std::string const& tmp, Stats* stats = nullptr, std::uint32_t maxlen = MAXLEN)
{
        Grammar G(terminals);
        Stats st;
        PairCounter pc;
        source([&](std::uint8_t const* d, std::size_t n) { for (std::size_t i = 0; i < n; ++i) pc.put(d[i]); });
        // One temporary file. The first round writes it from the source; every later round rewrites it in place: a
        // round never lengthens the sequence, so the write position never passes the read position
        int fd = -1;
        std::uint64_t flen = 0;                                 // symbols in the file
        auto const read_current = [&](Feed const& feed) {
                if (fd < 0) { source(feed); return; }
                std::vector<std::uint8_t> buf(1 << 20);
                for (std::uint64_t r = 0; r < flen;) {
                        ssize_t const n = ::pread(fd, buf.data(), std::size_t(std::min<std::uint64_t>(buf.size(), flen - r)), off_t(r));
                        if (n <= 0) break;
                        r += std::uint64_t(n);
                        feed(buf.data(), std::size_t(n));
                }
        };
        while (pc.n > ram_symbols && G.nsym < 256) {            // a round on disk
                // the batch: by count, pairs of symbols no chosen pair has used yet (at most 8)
                std::vector<std::pair<std::uint32_t, unsigned>> cand;
                for (unsigned a = 0; a < G.nsym; ++a)
                        for (unsigned c = 0; c < G.nsym; ++c)
                                if (pc.count[a << 8 | c] >= 2 && G.length[a] + G.length[c] <= maxlen) cand.push_back({ pc.count[a << 8 | c], a << 8 | c });
                std::sort(cand.begin(), cand.end(), std::greater<>());
                std::vector<std::uint16_t> repl(65536, 0);
                std::array<bool, 256> used{};
                int chosen = 0;
                for (auto const& [cnt, key] : cand) {
                        unsigned const a = key >> 8, c = key & 255;
                        if (used[a] || used[c] || G.nsym >= 256 || chosen >= 8) continue;
                        used[a] = used[c] = true;
                        repl[key] = G.add(a, c);
                        ++chosen;
                }
                if (chosen == 0) break;
                bool const first = fd < 0;
                int const out = first ? ::open(tmp.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644) : fd;
                std::uint64_t woff = 0;
                std::vector<std::uint8_t> obuf; obuf.reserve(1 << 20);
                PairCounter next;
                auto const flush = [&] {
                        if (::pwrite(out, obuf.data(), obuf.size(), off_t(woff)) != ssize_t(obuf.size())) { std::perror("repair: write"); std::exit(1); }
                        woff += obuf.size(); st.disk_bytes += obuf.size(); obuf.clear();
                };
                auto const emit = [&](std::uint8_t x) {         // everything emitted so far was read before: woff stays behind the reader
                        next.put(x);
                        obuf.push_back(x);
                        if (obuf.size() == (1 << 20)) flush();
                };
                int held = -1;                                  // one symbol of look-ahead across the chunks
                read_current([&](std::uint8_t const* d, std::size_t n) {
                        for (std::size_t i = 0; i < n; ++i) {
                                if (held < 0) { held = d[i]; continue; }
                                std::uint16_t const r = repl[std::size_t(held) << 8 | d[i]];
                                if (r != 0) { emit(std::uint8_t(r)); held = -1; }
                                else { emit(std::uint8_t(held)); held = d[i]; }
                        }
                });
                if (held >= 0) emit(std::uint8_t(held));
                flush();
                fd = out;
                flen = woff;
                if (::ftruncate(fd, off_t(flen)) != 0) { std::perror("repair: truncate"); std::exit(1); }
                pc = std::move(next);
                ++st.disk_rounds;
        }
        // in memory: the rest, one pair at a time
        std::vector<std::uint8_t> seq;
        seq.reserve(pc.n);
        read_current([&](std::uint8_t const* d, std::size_t n) { seq.insert(seq.end(), d, d + n); });
        if (fd >= 0) { ::close(fd); std::remove(tmp.c_str()); }
        st.ram_symbols_at_switch = seq.size();
        std::vector<std::uint32_t> count(65536);
        while (G.nsym < 256) {
                std::fill(count.begin(), count.end(), 0);
                bool skip = false;
                for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
                        if (skip) { skip = false; if (seq[i] == seq[i + 1] && seq[i - 1] == seq[i]) continue; }
                        ++count[std::size_t(seq[i]) << 8 | seq[i + 1]];
                        skip = seq[i] == seq[i + 1];
                }
                std::uint32_t best = 1; unsigned key = 0;
                for (unsigned a = 0; a < G.nsym; ++a)
                        for (unsigned c = 0; c < G.nsym; ++c)
                                if (count[a << 8 | c] > best && G.length[a] + G.length[c] <= maxlen) { best = count[a << 8 | c]; key = a << 8 | c; }
                if (best < 2) break;
                std::uint8_t const a = std::uint8_t(key >> 8), c = std::uint8_t(key & 255), n = G.add(a, c);
                std::size_t o = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        if (i + 1 < seq.size() && seq[i] == a && seq[i + 1] == c) { seq[o++] = n; ++i; }
                        else seq[o++] = seq[i];
                }
                seq.resize(o);
        }
        // the file
        std::uint64_t const blocks = (seq.size() + CODES - 1) / CODES;
        std::uint64_t const data_at = (OFF_START + 8 * (blocks + 1) + BLOCK - 1) / BLOCK * BLOCK;
        std::vector<std::uint8_t> out(data_at + blocks * BLOCK, 0);
        std::uint64_t const t64 = terminals;
        std::memcpy(out.data(), MAGIC, 8);
        std::memcpy(out.data() + 8, &positions, 8);
        std::memcpy(out.data() + 16, &blocks, 8);
        std::memcpy(out.data() + 24, &t64, 8);
        std::memcpy(out.data() + OFF_LEFT, G.left.data(), 256);
        std::memcpy(out.data() + OFF_RIGHT, G.right.data(), 256);
        std::memcpy(out.data() + OFF_LEN, G.length.data(), 1024);
        std::memcpy(out.data() + OFF_UNI, G.uniform.data(), 256);
        std::memcpy(out.data() + OFF_VALUE, value.data(), 512);
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
                                in_block += G.length[seq[i]];
                        }
                }
                pos += in_block;
        }
        std::memcpy(out.data() + OFF_START + 8 * blocks, &pos, 8);
        if (stats) *stats = st;
        return out;
}

// ---- the flat layout: 64-byte blocks of codewords and an Elias-Fano index of their first positions
//
// The same grammar and the same codeword sequence, cut into blocks of 64 codewords (one cache line) with nothing in
// between. Which block holds a position is answered by an index beside the blocks: the first position of every block,
// a rising sequence, stored after Elias and Fano:
//   low      the low L bits of each first position, L bits each (L = log2 of the average positions per block)
//   upper    the high parts in unary: for block i with high part h, bit h + i is set. Bucket h (the blocks with high
//            part h) is the run of ones before zero number h
//   sample   for every 128th bucket, the bit at which it starts
// About L + 2 bits per block (2 to 2.5 % of the data) and no limit on the positions of a block. A lookup: the bucket
// of the position's high part (sample, then a count of zeros over a few words); among its blocks the last whose low
// part is not above the position's; if there is none, the last block before the bucket (a scan back over the zeros:
// short unless blocks stand for very many positions, so a cap on the symbols' length keeps it short).
constexpr char MAGIC_FLAT[8] = { 'E', 'G', 'D', 'B', 'R', 'F', '1', 0 };
constexpr std::uint64_t SAMPLE = 128;

struct Flat {
        std::uint64_t positions = 0, blocks = 0, low_bits = 0;
        unsigned terminals = 0;
        std::uint8_t const* left = nullptr;
        std::uint8_t const* right = nullptr;
        std::uint32_t const* length = nullptr;
        std::uint8_t const* uniform = nullptr;
        std::uint16_t const* value = nullptr;
        std::uint64_t const* sample = nullptr;
        std::uint64_t const* upper = nullptr;
        std::uint8_t const* low = nullptr;
        std::uint8_t const* data = nullptr;
        std::uint64_t index_bytes = 0;
        explicit operator bool() const { return data != nullptr; }
};

struct FlatLayout {
        std::uint64_t L, buckets, samples, upper_words, low_bytes, off_sample, off_upper, off_low, off_data;
        FlatLayout(std::uint64_t positions, std::uint64_t blocks)
        {
                std::uint64_t const avg = blocks ? positions / blocks : 1;
                L = avg > 1 ? std::uint64_t(std::bit_width(avg) - 1) : 0;
                buckets = (positions >> L) + 1;
                samples = (buckets + SAMPLE - 1) / SAMPLE;
                upper_words = (blocks + buckets + 63) / 64 + 1;               // a spare word of zeros ends every scan
                low_bytes = (blocks * L + 7) / 8 + 8;                         // 8 spare bytes: a low part is read as one word
                off_sample = (OFF_START + 7) / 8 * 8;
                off_upper = off_sample + 8 * samples;
                off_low = off_upper + 8 * upper_words;
                off_data = (off_low + low_bytes + 63) / 64 * 64;
        }
};

inline Flat open_flat(std::uint8_t const* p)
{
        Flat f;
        if (std::memcmp(p, MAGIC_FLAT, 8) != 0) return f;
        std::uint64_t t;
        std::memcpy(&f.positions, p + 8, 8);
        std::memcpy(&f.blocks, p + 16, 8);
        std::memcpy(&t, p + 24, 8);
        std::memcpy(&f.low_bits, p + 32, 8);
        f.terminals = unsigned(t);
        FlatLayout const y(f.positions, f.blocks);
        f.left = p + OFF_LEFT; f.right = p + OFF_RIGHT;
        f.length = reinterpret_cast<std::uint32_t const*>(p + OFF_LEN);
        f.uniform = p + OFF_UNI;
        f.value = reinterpret_cast<std::uint16_t const*>(p + OFF_VALUE);
        f.sample = reinterpret_cast<std::uint64_t const*>(p + y.off_sample);
        f.upper = reinterpret_cast<std::uint64_t const*>(p + y.off_upper);
        f.low = p + y.off_low;
        f.data = p + y.off_data;
        f.index_bytes = y.off_data - y.off_sample;
        return f;
}

// the blocked file's codewords as a flat file
inline std::vector<std::uint8_t> to_flat(File const& f, std::uint8_t const* header)
{
        std::vector<std::uint8_t> seq;
        std::uint64_t done = 0;
        for (std::uint64_t b = 0; b < f.blocks && done < f.positions; ++b)
                for (std::uint64_t k = 0; k < CODES && done < f.positions; ++k) {
                        std::uint8_t const c = f.data[b * BLOCK + HEAD + k];
                        seq.push_back(c); done += f.length[c];
                }
        std::uint64_t const blocks = (seq.size() + MINI - 1) / MINI;
        FlatLayout const y(f.positions, blocks);
        std::vector<std::uint8_t> out(y.off_data + blocks * MINI, 0);
        std::memcpy(out.data(), header, OFF_START);                     // the counts and the grammar's tables as they are
        std::memcpy(out.data(), MAGIC_FLAT, 8);
        std::memcpy(out.data() + 16, &blocks, 8);
        std::memcpy(out.data() + 32, &y.L, 8);
        std::memcpy(out.data() + y.off_data, seq.data(), seq.size());
        std::uint64_t* const upper = reinterpret_cast<std::uint64_t*>(out.data() + y.off_upper);
        std::uint64_t* const sample = reinterpret_cast<std::uint64_t*>(out.data() + y.off_sample);
        std::uint8_t* const low = out.data() + y.off_low;
        std::uint64_t pos = 0;
        for (std::uint64_t i = 0; i < blocks; ++i) {
                std::uint64_t const h = pos >> y.L, bit = h + i, lo = pos & ((std::uint64_t(1) << y.L) - 1);
                upper[bit / 64] |= std::uint64_t(1) << (bit % 64);
                std::uint64_t w;
                std::memcpy(&w, low + i * y.L / 8, 8);
                w |= lo << (i * y.L % 8);
                std::memcpy(low + i * y.L / 8, &w, 8);
                for (std::uint64_t k = 0; k < MINI && i * MINI + k < seq.size(); ++k) pos += f.length[seq[i * MINI + k]];
        }
        // bucket j * SAMPLE starts after zero number j * SAMPLE - 1
        std::uint64_t zeros = 0, next = 0;
        for (std::uint64_t bit = 0; next < y.samples; ++bit) {
                if (zeros == next * SAMPLE) sample[next++] = bit;
                if (!(upper[bit / 64] >> (bit % 64) & 1)) ++zeros;
        }
        return out;
}

inline std::uint16_t lookup(Flat const& f, std::uint64_t p)
{
        std::uint64_t const L = f.low_bits, h = p >> L, mask = (std::uint64_t(1) << L) - 1, lo = p & mask;
        // the start of bucket h: its sample, then r zeros further
        std::uint64_t start = f.sample[h / SAMPLE];
        std::uint64_t r = h % SAMPLE;
        if (r != 0) {
                std::uint64_t w = start / 64;
                std::uint64_t z = ~f.upper[w] & (~std::uint64_t(0) << (start % 64));
                for (std::uint64_t c; (c = std::uint64_t(std::popcount(z))) < r; z = ~f.upper[++w]) r -= c;
#if defined(__BMI2__)
                start = w * 64 + std::uint64_t(_tzcnt_u64(_pdep_u64(std::uint64_t(1) << (r - 1), z))) + 1;
#else
                for (; r > 1; --r) z &= z - 1;
                start = w * 64 + std::uint64_t(std::countr_zero(z)) + 1;
#endif
        }
        std::uint64_t i = start - h;                                    // the blocks before bucket h
        auto const low_of = [&](std::uint64_t k) { std::uint64_t w; std::memcpy(&w, f.low + k * L / 8, 8); return w >> (k * L % 8) & mask; };
        // the bucket's blocks (the ones from `start`) whose low part is not above lo
        std::uint64_t bit = start, high = h;
        bool found = false;
        while ((f.upper[bit / 64] >> (bit % 64) & 1) && low_of(i) <= lo) { found = true; ++i; ++bit; }
        if (!found) {                                                   // the last block before the bucket: back over the zeros
                std::uint64_t e = start - 1;                            // the zero that ends bucket h - 1 (block 0 starts at 0: never for h = 0)
                std::uint64_t w = e / 64;
                std::uint64_t ones = f.upper[w] & ((std::uint64_t(1) << (e % 64)) - 1);
                while (ones == 0) ones = f.upper[--w];
                std::uint64_t const at = w * 64 + 63 - std::uint64_t(std::countl_zero(ones));
                high = at - (i - 1);
        }
        --i;
        std::uint32_t rel = std::uint32_t(p - (high << L | low_of(i)));
        std::uint8_t const* const c = f.data + i * MINI;
        unsigned s;
#if defined(__AVX2__)
        for (unsigned g = 0;; g += 8) {
                __m256i const idx = _mm256_cvtepu8_epi32(_mm_loadl_epi64(reinterpret_cast<__m128i const*>(c + g)));
                __m256i sum = _mm256_i32gather_epi32(reinterpret_cast<int const*>(f.length), idx, 4);
                sum = _mm256_add_epi32(sum, _mm256_slli_si256(sum, 4));
                sum = _mm256_add_epi32(sum, _mm256_slli_si256(sum, 8));
                __m128i const low_total = _mm_shuffle_epi32(_mm256_castsi256_si128(sum), 0xff);
                sum = _mm256_add_epi32(sum, _mm256_inserti128_si256(_mm256_setzero_si256(), low_total, 1));
                unsigned const above = unsigned(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(sum, _mm256_set1_epi32(int(rel))))));
                alignas(32) std::uint32_t t[8];
                _mm256_store_si256(reinterpret_cast<__m256i*>(t), sum);
                if (above != 0) {
                        unsigned const k = unsigned(std::countr_zero(above));
                        rel -= k ? t[k - 1] : 0;
                        s = c[g + k];
                        break;
                }
                rel -= t[7];
        }
#else
        unsigned k = 0;
        for (std::uint32_t l; rel >= (l = f.length[c[k]]); ++k) rel -= l;
        s = c[k];
#endif
        for (;;) {
                std::uint8_t const u = f.uniform[s];
                if (u != MIXED) return f.value[u];
                unsigned const Lf = f.left[s];
                std::uint32_t const ll = f.length[Lf];
                bool const in_left = rel < ll;
                s = in_left ? Lf : f.right[s];
                rel -= in_left ? 0 : ll;
        }
}

}       // namespace rpvf
