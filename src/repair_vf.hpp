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
        FlatLayout(std::uint64_t positions, std::uint64_t blocks, std::uint64_t base = OFF_START)
        {
                std::uint64_t const avg = blocks ? positions / blocks : 1;
                L = avg > 1 ? std::uint64_t(std::bit_width(avg) - 1) : 0;
                buckets = (positions >> L) + 1;
                samples = (buckets + SAMPLE - 1) / SAMPLE;
                upper_words = (blocks + buckets + 63) / 64 + 1;               // a spare word of zeros ends every scan
                low_bytes = (blocks * L + 7) / 8 + 8;                         // 8 spare bytes: a low part is read as one word
                off_sample = (base + 7) / 8 * 8;
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

// the index of the blocks' first positions, written into a file laid out by y
inline void write_index(std::uint8_t* out, FlatLayout const& y, std::vector<std::uint64_t> const& starts)
{
        std::uint64_t* const upper = reinterpret_cast<std::uint64_t*>(out + y.off_upper);
        std::uint64_t* const sample = reinterpret_cast<std::uint64_t*>(out + y.off_sample);
        std::uint8_t* const low = out + y.off_low;
        for (std::uint64_t i = 0; i < starts.size(); ++i) {
                std::uint64_t const h = starts[i] >> y.L, bit = h + i, lo = starts[i] & ((std::uint64_t(1) << y.L) - 1);
                upper[bit / 64] |= std::uint64_t(1) << (bit % 64);
                std::uint64_t w;
                std::memcpy(&w, low + i * y.L / 8, 8);
                w |= lo << (i * y.L % 8);
                std::memcpy(low + i * y.L / 8, &w, 8);
        }
        // bucket j * SAMPLE starts after zero number j * SAMPLE - 1
        std::uint64_t zeros = 0, next = 0;
        for (std::uint64_t bit = 0; next < y.samples; ++bit) {
                if (zeros == next * SAMPLE) sample[next++] = bit;
                if (!(upper[bit / 64] >> (bit % 64) & 1)) ++zeros;
        }
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
        std::vector<std::uint64_t> starts(blocks);
        std::uint64_t pos = 0;
        for (std::uint64_t i = 0; i < blocks; ++i) {
                starts[i] = pos;
                for (std::uint64_t k = 0; k < MINI && i * MINI + k < seq.size(); ++k) pos += f.length[seq[i * MINI + k]];
        }
        write_index(out.data(), y, starts);
        return out;
}

// the block that holds position p (its number in i) and its first position
template<class F>
inline std::uint64_t locate(F const& f, std::uint64_t p, std::uint64_t& i_out)
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
        i_out = i;
        return high << L | low_of(i);
}

inline std::uint16_t lookup(Flat const& f, std::uint64_t p)
{
        std::uint64_t i;
        std::uint32_t rel = std::uint32_t(p - locate(f, p, i));
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

// ---- 12-bit codewords in the flat layout
//
// Up to 4,096 symbols; a 64-byte block holds 42 codewords of 12 bits (codeword k at bit 12 k; 8 bits spare). The
// grammar's tables are as long as the symbols in use, so a small file carries a small dictionary, and a rule is only
// made while it saves more bytes than it costs (`min_count` occurrences). The index is the same Elias-Fano index.
constexpr char MAGIC_WIDE[8] = { 'E', 'G', 'D', 'B', 'R', 'W', '1', 0 };
constexpr std::uint64_t WCODES = 42, WOFF_VALUE = 48, WOFF_LEN = WOFF_VALUE + 512;

struct Wide {
        std::uint64_t positions = 0, blocks = 0, low_bits = 0;
        unsigned terminals = 0, nsym = 0;
        std::uint16_t const* left = nullptr;
        std::uint16_t const* right = nullptr;
        std::uint32_t const* length = nullptr;
        std::uint8_t const* uniform = nullptr;
        std::uint16_t const* value = nullptr;
        std::uint64_t const* sample = nullptr;
        std::uint64_t const* upper = nullptr;
        std::uint8_t const* low = nullptr;
        std::uint8_t const* data = nullptr;
        std::uint64_t index_bytes = 0, dict_bytes = 0;
        explicit operator bool() const { return data != nullptr; }
};

inline std::uint64_t wide_base(std::uint64_t nsym) { return WOFF_LEN + 4 * nsym + 2 * nsym + 2 * nsym + nsym; }

inline Wide open_wide(std::uint8_t const* p)
{
        Wide f;
        if (std::memcmp(p, MAGIC_WIDE, 8) != 0) return f;
        std::uint64_t t, n;
        std::memcpy(&f.positions, p + 8, 8);
        std::memcpy(&f.blocks, p + 16, 8);
        std::memcpy(&t, p + 24, 8);
        std::memcpy(&f.low_bits, p + 32, 8);
        std::memcpy(&n, p + 40, 8);
        f.terminals = unsigned(t); f.nsym = unsigned(n);
        FlatLayout const y(f.positions, f.blocks, wide_base(n));
        f.value = reinterpret_cast<std::uint16_t const*>(p + WOFF_VALUE);
        f.length = reinterpret_cast<std::uint32_t const*>(p + WOFF_LEN);
        f.left = reinterpret_cast<std::uint16_t const*>(p + WOFF_LEN + 4 * n);
        f.right = f.left + n;
        f.uniform = p + WOFF_LEN + 8 * n;
        f.sample = reinterpret_cast<std::uint64_t const*>(p + y.off_sample);
        f.upper = reinterpret_cast<std::uint64_t const*>(p + y.off_upper);
        f.low = p + y.off_low;
        f.data = p + y.off_data;
        f.index_bytes = y.off_data - y.off_sample;
        f.dict_bytes = y.off_sample;
        return f;
}

// seq: the terminals (0 .. terminals - 1), in memory. Pairing in passes: the most frequent pair, or a batch of
// frequent pairs with no symbol in common
inline std::vector<std::uint8_t> compress_wide(std::vector<std::uint16_t> seq, std::uint64_t positions, unsigned terminals, std::array<std::uint16_t, 256> const& value,
                                               std::uint32_t maxlen = MAXLEN, std::uint32_t min_count = 8, unsigned max_symbols = 4096,
                                               std::vector<std::uint16_t>* final_seq = nullptr)
{
        constexpr std::uint32_t W = 4096;
        std::vector<std::uint16_t> left(terminals), right(terminals);
        std::vector<std::uint32_t> length(terminals, 1);
        std::vector<std::uint8_t> uniform(terminals);
        for (unsigned t = 0; t < terminals; ++t) { left[t] = right[t] = std::uint16_t(t); uniform[t] = std::uint8_t(t); }
        std::vector<std::uint32_t> count(std::size_t(W) * W, 0), touched;
        std::vector<std::uint16_t> repl(std::size_t(W) * W / 1, 0);       // pair -> new symbol (0: none)
        for (int pass = 0; left.size() < max_symbols; ++pass) {
                touched.clear();
                bool skip = false;
                for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
                        if (skip) { skip = false; if (seq[i] == seq[i + 1] && seq[i - 1] == seq[i]) continue; }
                        std::uint32_t const key = std::uint32_t(seq[i]) * W + seq[i + 1];
                        if (count[key]++ == 0) touched.push_back(key);
                        skip = seq[i] == seq[i + 1];
                }
                std::vector<std::pair<std::uint32_t, std::uint32_t>> cand;
                for (std::uint32_t key : touched) {
                        if (count[key] >= min_count && length[key / W] + length[key % W] <= maxlen) cand.push_back({ count[key], key });
                        count[key] = 0;
                }
                if (cand.empty()) break;
                std::size_t const nsym = left.size();
                std::size_t const batch = std::min<std::size_t>({ cand.size(), max_symbols - nsym, pass < 16 ? std::size_t(1) : std::max<std::size_t>(1, nsym / 16) });
                std::partial_sort(cand.begin(), cand.begin() + std::ptrdiff_t(std::min(cand.size(), 4 * batch)), cand.end(), std::greater<>());
                std::vector<bool> used(W, false), is_left(W, false);
                std::vector<std::uint32_t> chosen;
                for (std::size_t k = 0; k < std::min(cand.size(), 4 * batch) && chosen.size() < batch; ++k) {
                        std::uint32_t const key = cand[k].second, a = key / W, c = key % W;
                        if (2 * cand[k].first < cand[0].first || used[a] || used[c]) continue;        // near the best only, no symbol twice
                        used[a] = used[c] = true; is_left[a] = true;
                        repl[key] = std::uint16_t(left.size());
                        chosen.push_back(key);
                        left.push_back(std::uint16_t(a)); right.push_back(std::uint16_t(c)); length.push_back(length[a] + length[c]);
                        uniform.push_back(uniform[a] == uniform[c] ? uniform[a] : MIXED);
                }
                std::size_t o = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        if (i + 1 < seq.size() && is_left[seq[i]]) {
                                std::uint16_t const r = repl[std::size_t(seq[i]) * W + seq[i + 1]];
                                if (r != 0) { seq[o++] = r; ++i; continue; }
                        }
                        seq[o++] = seq[i];
                }
                seq.resize(o);
                for (std::uint32_t key : chosen) repl[key] = 0;
        }
        std::uint64_t const n = left.size(), blocks = (seq.size() + WCODES - 1) / WCODES;
        FlatLayout const y(positions, blocks, wide_base(n));
        std::vector<std::uint8_t> out(y.off_data + blocks * MINI + 32, 0);      // 32 spare bytes: the last block is read in 16-byte pieces
        std::uint64_t const t64 = terminals;
        std::memcpy(out.data(), MAGIC_WIDE, 8);
        std::memcpy(out.data() + 8, &positions, 8);
        std::memcpy(out.data() + 16, &blocks, 8);
        std::memcpy(out.data() + 24, &t64, 8);
        std::memcpy(out.data() + 32, &y.L, 8);
        std::memcpy(out.data() + 40, &n, 8);
        std::memcpy(out.data() + WOFF_VALUE, value.data(), 512);
        std::memcpy(out.data() + WOFF_LEN, length.data(), 4 * n);
        std::memcpy(out.data() + WOFF_LEN + 4 * n, left.data(), 2 * n);
        std::memcpy(out.data() + WOFF_LEN + 6 * n, right.data(), 2 * n);
        std::memcpy(out.data() + WOFF_LEN + 8 * n, uniform.data(), n);
        std::vector<std::uint64_t> starts(blocks);
        std::uint64_t pos = 0;
        for (std::uint64_t i = 0; i < blocks; ++i) {
                starts[i] = pos;
                std::uint8_t* const blk = out.data() + y.off_data + i * MINI;
                for (std::uint64_t k = 0; k < WCODES && i * WCODES + k < seq.size(); ++k) {
                        std::uint16_t const c = seq[i * WCODES + k];
                        pos += length[c];
                        std::uint16_t w;
                        std::memcpy(&w, blk + 12 * k / 8, 2);
                        w = std::uint16_t(w | c << (12 * k % 8));
                        std::memcpy(blk + 12 * k / 8, &w, 2);
                }
        }
        write_index(out.data(), y, starts);
        if (final_seq) *final_seq = std::move(seq);
        return out;
}

// ---- the layout of the Syzygy tables (de Man), for comparison
//
// The same kind of grammar (at most 4,095 symbols of at most 256 positions each), then: Huffman codes on the symbols
// (canonical, at most 32 bits); blocks of 2^b bytes that hold whole codewords and at most 65,536 positions (b at
// most max_block_bits; the b with the smallest file); per block its positions less one in 16 bits; a 6-byte index
// entry (block, offset) for the middle of every 2^idxbits positions, idxbits = bits of the average block + 4. The
// lookup follows his decoder: index entry, a walk over the size table, the block's codewords bit by bit, down the rules.
constexpr char MAGIC_SYZ[8] = { 'E', 'G', 'D', 'B', 'R', 'S', '1', 0 };
constexpr std::uint64_t SOFF_BASE = 64, SOFF_OFFSET = SOFF_BASE + 8 * 65, SOFF_SYMLEN = SOFF_OFFSET + 2 * 66 + 4;

struct Syz {
        std::uint64_t positions = 0, blocks = 0, block_bits = 0, idxbits = 0, nsym = 0, min_len = 0, num_indices = 0;
        std::uint64_t const* base = nullptr;                    // by code length: the first code of that length, left-aligned
        std::uint16_t const* offset = nullptr;                  // by code length: the first symbol of that length
        std::uint8_t const* symlen = nullptr;                   // positions of a symbol, less one
        std::uint16_t const* left = nullptr;                    // a terminal (symlen 0): its value
        std::uint16_t const* right = nullptr;
        std::uint8_t const* index = nullptr;
        std::uint16_t const* sizes = nullptr;
        std::uint8_t const* data = nullptr;
        std::uint64_t table_bytes = 0, dict_bytes = 0, data_bytes = 0;
        explicit operator bool() const { return data != nullptr; }
};

struct SyzLayout {
        std::uint64_t off_left, off_index, off_sizes, off_data;
        SyzLayout(std::uint64_t nsym, std::uint64_t num_indices, std::uint64_t blocks)
        {
                off_left = (SOFF_SYMLEN + nsym + 1) / 2 * 2;
                off_index = off_left + 4 * nsym;
                off_sizes = (off_index + 6 * num_indices + 1) / 2 * 2;
                off_data = (off_sizes + 2 * blocks + 63) / 64 * 64;
        }
};

inline Syz open_syz(std::uint8_t const* p)
{
        Syz f;
        if (std::memcmp(p, MAGIC_SYZ, 8) != 0) return f;
        std::memcpy(&f.positions, p + 8, 8); std::memcpy(&f.blocks, p + 16, 8); std::memcpy(&f.block_bits, p + 24, 8);
        std::memcpy(&f.idxbits, p + 32, 8); std::memcpy(&f.nsym, p + 40, 8); std::memcpy(&f.min_len, p + 48, 8); std::memcpy(&f.num_indices, p + 56, 8);
        SyzLayout const y(f.nsym, f.num_indices, f.blocks);
        f.base = reinterpret_cast<std::uint64_t const*>(p + SOFF_BASE);
        f.offset = reinterpret_cast<std::uint16_t const*>(p + SOFF_OFFSET);
        f.symlen = p + SOFF_SYMLEN;
        f.left = reinterpret_cast<std::uint16_t const*>(p + y.off_left);
        f.right = f.left + f.nsym;
        f.index = p + y.off_index;
        f.sizes = reinterpret_cast<std::uint16_t const*>(p + y.off_sizes);
        f.data = p + y.off_data;
        f.table_bytes = 6 * f.num_indices + 2 * f.blocks;
        f.dict_bytes = y.off_index;
        return f;
}

// canonical Huffman codes for the symbols of a sequence (at most 32 bits; the longest codes are the smallest numbers),
// and the symbols renumbered by code length (shortest first, then those without a code)
struct Canon {
        std::vector<int> len;
        std::vector<unsigned> order;
        std::vector<std::uint16_t> renum;
        std::vector<std::uint64_t> codeword;                    // right-aligned
        std::array<std::uint64_t, 65> base{};
        std::array<std::uint16_t, 66> offset{};
        int min_len = 64, max_len = 1;
};
inline Canon canonical(Wide const& g, std::vector<std::uint16_t> const& seq)
{
        Canon C;
        std::vector<int>& len = C.len; std::vector<unsigned>& order = C.order; std::vector<std::uint16_t>& renum = C.renum;
        std::vector<std::uint64_t>& codeword = C.codeword; auto& base = C.base; auto& offset = C.offset; int& min_len = C.min_len; int& max_len = C.max_len;
        std::vector<std::uint64_t> freq(g.nsym, 0);
        for (std::uint16_t c : seq) ++freq[c];
        // Huffman code lengths (the two rarest merged, again and again); the rarest are lifted until no code is over 32 bits
        len.assign(g.nsym, 0);
        for (std::uint64_t floor = 1;; floor *= 4) {
                struct Node { std::uint64_t w; int a, b; };
                std::vector<Node> nodes;
                std::vector<std::pair<std::uint64_t, int>> heap;
                for (unsigned s = 0; s < g.nsym; ++s) if (freq[s]) { nodes.push_back({ std::max(freq[s], floor), -1, int(s) }); heap.push_back({ nodes.back().w, int(nodes.size()) - 1 }); }
                auto const cmp = std::greater<>();
                std::make_heap(heap.begin(), heap.end(), cmp);
                while (heap.size() > 1) {
                        std::pop_heap(heap.begin(), heap.end(), cmp); auto const x = heap.back(); heap.pop_back();
                        std::pop_heap(heap.begin(), heap.end(), cmp); auto const z = heap.back(); heap.pop_back();
                        nodes.push_back({ x.first + z.first, x.second, z.second });
                        heap.push_back({ x.first + z.first, int(nodes.size()) - 1 }); std::push_heap(heap.begin(), heap.end(), cmp);
                }
                std::vector<int> depth(nodes.size(), 0);
                int deepest = 1;
                for (std::size_t k = nodes.size(); k-- > 0;) {
                        if (nodes[k].a < 0) { len[std::size_t(nodes[k].b)] = std::max(1, depth[k]); deepest = std::max(deepest, depth[k]); continue; }
                        depth[std::size_t(nodes[k].a)] = depth[std::size_t(nodes[k].b)] = depth[k] + 1;
                }
                if (deepest <= 32) break;
        }
        // the symbols renumbered: by code length (shortest first), then those without a code
        for (unsigned s = 0; s < g.nsym; ++s) if (len[s]) order.push_back(s);
        std::stable_sort(order.begin(), order.end(), [&](unsigned x, unsigned z) { return len[x] < len[z]; });
        std::size_t const coded = order.size();
        for (unsigned s = 0; s < g.nsym; ++s) if (!len[s]) order.push_back(s);
        renum.resize(g.nsym);
        for (std::size_t k = 0; k < order.size(); ++k) renum[order[k]] = std::uint16_t(k);
        // canonical codes: the longest codes are the smallest numbers
        std::array<std::uint64_t, 65> first{}, count{};
        for (std::size_t k = 0; k < coded; ++k) { int const l = len[order[k]]; ++count[std::size_t(l)]; min_len = std::min(min_len, l); max_len = std::max(max_len, l); }
        { std::uint16_t at = 0; for (int l = 1; l <= 64; ++l) { offset[std::size_t(l)] = at; at = std::uint16_t(at + count[std::size_t(l)]); } }
        first[std::size_t(max_len)] = 0;
        for (int l = max_len; l > 1; --l) first[std::size_t(l - 1)] = (first[std::size_t(l)] + count[std::size_t(l)]) >> 1;
        for (int l = 1; l <= 64; ++l) base[std::size_t(l)] = l <= max_len ? first[std::size_t(l)] << (64 - l) : 0;
        for (int l = 1; l < min_len; ++l) base[std::size_t(l)] = ~std::uint64_t(0);
        codeword.assign(g.nsym, 0);
        { std::array<std::uint64_t, 65> next = first; for (std::size_t k = 0; k < coded; ++k) { int const l = len[order[k]]; codeword[order[k]] = next[std::size_t(l)]++; } }
        return C;
}

// g: a grammar made by compress_wide with symbols of at most 256 positions; seq: its final sequence
inline std::vector<std::uint8_t> syzygy_build(Wide const& g, std::vector<std::uint16_t> const& seq, int max_block_bits)
{
        Canon const C = canonical(g, seq);
        std::vector<int> const& len = C.len; std::vector<std::uint16_t> const& renum = C.renum; std::vector<std::uint64_t> const& codeword = C.codeword;
        auto const& base = C.base; auto const& offset = C.offset; int const min_len = C.min_len;
        // the block size with the smallest file
        std::uint64_t best_total = 0; int best_b = max_block_bits;
        auto const cut = [&](int b, std::vector<std::uint16_t>* sizes, std::vector<std::uint64_t>* firsts) {
                std::uint64_t const maxbits = std::uint64_t(8) << b;
                std::uint64_t blocks = 0, bits = 0, numpos = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        std::uint16_t const c = seq[i];
                        if (bits + std::uint64_t(len[c]) > maxbits || numpos + g.length[c] > 65536) { if (sizes) sizes->push_back(std::uint16_t(numpos - 1)); ++blocks; bits = 0; numpos = 0; }
                        if (bits == 0 && firsts) firsts->push_back(i);
                        bits += std::uint64_t(len[c]); numpos += g.length[c];
                }
                if (numpos > 0) { if (sizes) sizes->push_back(std::uint16_t(numpos - 1)); ++blocks; }
                return blocks;
        };
        auto const idxbits_of = [&](std::uint64_t blocks) {
                int ib = std::bit_width(blocks ? g.positions / blocks : 1) + 4;
                while (ib > 1 && (std::uint64_t(1) << (ib - 1)) > g.positions) --ib;
                return ib;
        };
        for (int b = max_block_bits; b >= 4; --b) {
                std::uint64_t const blocks = cut(b, nullptr, nullptr);
                int const ib = idxbits_of(blocks);
                std::uint64_t const total = (blocks << b) + 2 * blocks + 6 * ((g.positions + (std::uint64_t(1) << ib) - 1) >> ib);
                if (best_total == 0 || total < best_total) { best_total = total; best_b = b; }
        }
        std::vector<std::uint16_t> sizes;
        std::vector<std::uint64_t> firsts;
        std::uint64_t const real_blocks = cut(best_b, &sizes, &firsts);
        firsts.push_back(seq.size());
        std::uint64_t const idxbits = std::uint64_t(idxbits_of(real_blocks));
        std::uint64_t const num_indices = (g.positions + (std::uint64_t(1) << idxbits) - 1) >> idxbits;
        // the index: for the middle of every interval, its block and the offset in it (dummy blocks of 65,536 at the end, as his)
        std::vector<std::uint8_t> index(6 * num_indices);
        {
                std::uint64_t idx2 = std::uint64_t(1) << (idxbits - 1), block = 0, idx = 0;
                for (std::uint64_t i = 0; i < num_indices; ++i) {
                        while (block < sizes.size() && idx + sizes[block] < idx2) idx += std::uint64_t(sizes[block++]) + 1;
                        if (block == sizes.size()) {
                                sizes.push_back(65535);
                                while (idx + sizes[block] < idx2) { idx += std::uint64_t(sizes[block++]) + 1; sizes.push_back(65535); }
                        }
                        std::uint32_t const b32 = std::uint32_t(block);
                        std::uint16_t const o16 = std::uint16_t(idx2 - idx);
                        std::memcpy(index.data() + 6 * i, &b32, 4);
                        std::memcpy(index.data() + 6 * i + 4, &o16, 2);
                        idx2 += std::uint64_t(1) << idxbits;
                }
        }
        std::uint64_t const blocks = sizes.size(), n = g.nsym;
        SyzLayout const y(n, num_indices, blocks);
        std::vector<std::uint8_t> out(y.off_data + (real_blocks << best_b) + 32, 0);
        std::uint64_t const h[7] = { g.positions, blocks, std::uint64_t(best_b), idxbits, n, std::uint64_t(min_len), num_indices };
        std::memcpy(out.data(), MAGIC_SYZ, 8);
        std::memcpy(out.data() + 8, h, sizeof h);
        std::memcpy(out.data() + SOFF_BASE, base.data(), 8 * 65);
        std::memcpy(out.data() + SOFF_OFFSET, offset.data(), 2 * 66);
        for (unsigned s = 0; s < n; ++s) {
                unsigned const r = renum[s];
                out[SOFF_SYMLEN + r] = std::uint8_t(g.length[s] - 1);
                std::uint16_t const l = s < g.terminals ? g.value[s] : renum[g.left[s]], rr = s < g.terminals ? std::uint16_t(0) : renum[g.right[s]];
                std::memcpy(out.data() + y.off_left + 2 * r, &l, 2);
                std::memcpy(out.data() + y.off_left + 2 * n + 2 * r, &rr, 2);
        }
        std::memcpy(out.data() + y.off_index, index.data(), index.size());
        std::memcpy(out.data() + y.off_sizes, sizes.data(), 2 * sizes.size());
        for (std::uint64_t b = 0; b < real_blocks; ++b) {              // the codewords, most significant bit first
                std::uint8_t* const blk = out.data() + y.off_data + (b << best_b);
                std::uint64_t bit = 0;
                for (std::uint64_t i = firsts[b]; i < firsts[b + 1]; ++i) {
                        int const l = len[seq[i]];
                        std::uint64_t const cw = codeword[seq[i]];
                        for (int k = l - 1; k >= 0; --k, ++bit) if (cw >> k & 1) blk[bit / 8] |= std::uint8_t(0x80 >> (bit % 8));
                }
        }
        return out;
}

inline std::uint16_t lookup(Syz const& f, std::uint64_t p)
{
        std::uint64_t const main_idx = p >> f.idxbits;
        std::int64_t lit = std::int64_t(p & ((std::uint64_t(1) << f.idxbits) - 1)) - (std::int64_t(1) << (f.idxbits - 1));
        std::uint32_t block; std::uint16_t off;
        std::memcpy(&block, f.index + 6 * main_idx, 4);
        std::memcpy(&off, f.index + 6 * main_idx + 4, 2);
        lit += off;
        if (lit < 0) while (lit < 0) lit += std::int64_t(f.sizes[--block]) + 1;
        else while (lit > std::int64_t(f.sizes[block])) lit -= std::int64_t(f.sizes[block++]) + 1;
        std::uint8_t const* ptr = f.data + (std::uint64_t(block) << f.block_bits);
        std::uint64_t code; std::memcpy(&code, ptr, 8); code = __builtin_bswap64(code); ptr += 8;
        std::uint64_t const* const base = f.base;
        unsigned sym, bit_cnt = 0;
        for (;;) {
                unsigned l = unsigned(f.min_len);
                while (code < base[l]) ++l;
                sym = unsigned(f.offset[l]) + unsigned((code - base[l]) >> (64 - l));
                if (lit < std::int64_t(f.symlen[sym]) + 1) break;
                lit -= std::int64_t(f.symlen[sym]) + 1;
                code <<= l;
                bit_cnt += l;
                if (bit_cnt >= 32) {
                        bit_cnt -= 32;
                        std::uint32_t t; std::memcpy(&t, ptr, 4); ptr += 4;
                        code |= std::uint64_t(__builtin_bswap32(t)) << bit_cnt;
                }
        }
        while (f.symlen[sym] != 0) {
                unsigned const s1 = f.left[sym];
                if (lit < std::int64_t(f.symlen[s1]) + 1) sym = s1;
                else { lit -= std::int64_t(f.symlen[s1]) + 1; sym = f.right[sym]; }
        }
        return f.left[sym];
}

// ---- Huffman codes on the symbols, 64-byte blocks, the Elias-Fano index
//
// Any grammar of compress_wide (256 symbols: Huffman on top of the byte codewords; up to 4,096: as the Syzygy layout
// but with this index, so no limit on the positions of a symbol or of a block). A block holds whole codewords, most
// significant bit first. A lookup: the block from the index, then its codewords one by one, adding up positions.
constexpr char MAGIC_HUFF[8] = { 'E', 'G', 'D', 'B', 'R', 'H', '1', 0 };
constexpr std::uint64_t HOFF_LEN = SOFF_OFFSET + 2 * 66 + 4;
constexpr std::uint16_t NOVALUE = 0xffff;

struct Huff {
        std::uint64_t positions = 0, blocks = 0, low_bits = 0, nsym = 0, min_len = 0;
        std::uint64_t const* base = nullptr;
        std::uint16_t const* offset = nullptr;
        std::uint32_t const* length = nullptr;
        std::uint16_t const* left = nullptr;
        std::uint16_t const* right = nullptr;
        std::uint16_t const* uvalue = nullptr;                  // the value of a symbol that is a run of one value, else NOVALUE
        std::uint64_t const* sample = nullptr;
        std::uint64_t const* upper = nullptr;
        std::uint8_t const* low = nullptr;
        std::uint8_t const* data = nullptr;
        std::uint64_t index_bytes = 0, dict_bytes = 0;
        explicit operator bool() const { return data != nullptr; }
};

inline std::uint64_t huff_base(std::uint64_t nsym) { return HOFF_LEN + 4 * nsym + 6 * nsym; }

inline Huff open_huff(std::uint8_t const* p)
{
        Huff f;
        if (std::memcmp(p, MAGIC_HUFF, 8) != 0) return f;
        std::memcpy(&f.positions, p + 8, 8); std::memcpy(&f.blocks, p + 16, 8); std::memcpy(&f.low_bits, p + 24, 8);
        std::memcpy(&f.nsym, p + 32, 8); std::memcpy(&f.min_len, p + 40, 8);
        FlatLayout const y(f.positions, f.blocks, huff_base(f.nsym));
        f.base = reinterpret_cast<std::uint64_t const*>(p + SOFF_BASE);
        f.offset = reinterpret_cast<std::uint16_t const*>(p + SOFF_OFFSET);
        f.length = reinterpret_cast<std::uint32_t const*>(p + HOFF_LEN);
        f.left = reinterpret_cast<std::uint16_t const*>(p + HOFF_LEN + 4 * f.nsym);
        f.right = f.left + f.nsym;
        f.uvalue = f.right + f.nsym;
        f.sample = reinterpret_cast<std::uint64_t const*>(p + y.off_sample);
        f.upper = reinterpret_cast<std::uint64_t const*>(p + y.off_upper);
        f.low = p + y.off_low;
        f.data = p + y.off_data;
        f.index_bytes = y.off_data - y.off_sample;
        f.dict_bytes = y.off_sample;
        return f;
}

inline std::vector<std::uint8_t> huff_build(Wide const& g, std::vector<std::uint16_t> const& seq)
{
        Canon const C = canonical(g, seq);
        std::vector<std::uint64_t> firsts, starts;
        {
                std::uint64_t bits = 0, pos = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        std::uint64_t const l = std::uint64_t(C.len[seq[i]]);
                        if (i == 0 || bits + l > 8 * MINI) { firsts.push_back(i); starts.push_back(pos); bits = 0; }
                        bits += l; pos += g.length[seq[i]];
                }
        }
        std::uint64_t const blocks = starts.size(), n = g.nsym;
        firsts.push_back(seq.size());
        FlatLayout const y(g.positions, blocks, huff_base(n));
        std::vector<std::uint8_t> out(y.off_data + blocks * MINI + 32, 0);
        std::uint64_t const h[5] = { g.positions, blocks, y.L, n, std::uint64_t(C.min_len) };
        std::memcpy(out.data(), MAGIC_HUFF, 8);
        std::memcpy(out.data() + 8, h, sizeof h);
        std::memcpy(out.data() + SOFF_BASE, C.base.data(), 8 * 65);
        std::memcpy(out.data() + SOFF_OFFSET, C.offset.data(), 2 * 66);
        for (unsigned s = 0; s < n; ++s) {
                std::uint64_t const r = C.renum[s];
                std::uint32_t const ln = g.length[s];
                std::uint16_t const l = C.renum[g.left[s]], rr = C.renum[g.right[s]], uv = g.uniform[s] != MIXED ? g.value[g.uniform[s]] : NOVALUE;
                std::memcpy(out.data() + HOFF_LEN + 4 * r, &ln, 4);
                std::memcpy(out.data() + HOFF_LEN + 4 * n + 2 * r, &l, 2);
                std::memcpy(out.data() + HOFF_LEN + 6 * n + 2 * r, &rr, 2);
                std::memcpy(out.data() + HOFF_LEN + 8 * n + 2 * r, &uv, 2);
        }
        for (std::uint64_t b = 0; b < blocks; ++b) {
                std::uint8_t* const blk = out.data() + y.off_data + b * MINI;
                std::uint64_t bit = 0;
                for (std::uint64_t i = firsts[b]; i < firsts[b + 1]; ++i) {
                        std::uint64_t const cw = C.codeword[seq[i]];
                        for (int k = C.len[seq[i]] - 1; k >= 0; --k, ++bit) if (cw >> k & 1) blk[bit / 8] |= std::uint8_t(0x80 >> (bit % 8));
                }
        }
        write_index(out.data(), y, starts);
        return out;
}

inline std::uint16_t lookup(Huff const& f, std::uint64_t p)
{
        std::uint64_t i;
        std::uint32_t rel = std::uint32_t(p - locate(f, p, i));
        std::uint8_t const* ptr = f.data + i * MINI;
        std::uint64_t code; std::memcpy(&code, ptr, 8); code = __builtin_bswap64(code); ptr += 8;
        unsigned sym, bit_cnt = 0;
        for (;;) {
                unsigned l = unsigned(f.min_len);
                while (code < f.base[l]) ++l;
                sym = unsigned(f.offset[l]) + unsigned((code - f.base[l]) >> (64 - l));
                std::uint32_t const n = f.length[sym];
                if (rel < n) break;
                rel -= n;
                code <<= l;
                bit_cnt += l;
                if (bit_cnt >= 32) {
                        bit_cnt -= 32;
                        std::uint32_t t; std::memcpy(&t, ptr, 4); ptr += 4;
                        code |= std::uint64_t(__builtin_bswap32(t)) << bit_cnt;
                }
        }
        for (;;) {
                std::uint16_t const u = f.uvalue[sym];
                if (u != NOVALUE) return u;
                unsigned const Lf = f.left[sym];
                std::uint32_t const ll = f.length[Lf];
                bool const in_left = rel < ll;
                sym = in_left ? Lf : f.right[sym];
                rel -= in_left ? 0 : ll;
        }
}

inline std::uint16_t lookup(Wide const& f, std::uint64_t p)
{
        std::uint64_t i;
        std::uint32_t rel = std::uint32_t(p - locate(f, p, i));
        std::uint8_t const* const c = f.data + i * MINI;
        unsigned s;
#if defined(__AVX2__)
        // 8 codewords are 12 bytes: 4 per half, each from the two bytes it lies in, shifted by 0 or 4 bits
        __m256i const pick = _mm256_setr_epi8(0, 1, -1, -1, 1, 2, -1, -1, 3, 4, -1, -1, 4, 5, -1, -1, 0, 1, -1, -1, 1, 2, -1, -1, 3, 4, -1, -1, 4, 5, -1, -1);
        __m256i const shift = _mm256_setr_epi32(0, 4, 0, 4, 0, 4, 0, 4);
        for (unsigned g = 0;; g += 8) {
                std::uint8_t const* const q = c + 12 * (g / 8);
                __m256i const raw = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128(reinterpret_cast<__m128i const*>(q))),
                                                            _mm_loadu_si128(reinterpret_cast<__m128i const*>(q + 6)), 1);
                __m256i idx = _mm256_and_si256(_mm256_srlv_epi32(_mm256_shuffle_epi8(raw, pick), shift), _mm256_set1_epi32(0xfff));
                if (g == 40) idx = _mm256_and_si256(idx, _mm256_setr_epi32(-1, -1, 0, 0, 0, 0, 0, 0));          // the block's last two codewords
                __m256i sum = _mm256_i32gather_epi32(reinterpret_cast<int const*>(f.length), idx, 4);
                sum = _mm256_add_epi32(sum, _mm256_slli_si256(sum, 4));
                sum = _mm256_add_epi32(sum, _mm256_slli_si256(sum, 8));
                __m128i const low_total = _mm_shuffle_epi32(_mm256_castsi256_si128(sum), 0xff);
                sum = _mm256_add_epi32(sum, _mm256_inserti128_si256(_mm256_setzero_si256(), low_total, 1));
                unsigned const above = unsigned(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(sum, _mm256_set1_epi32(int(rel))))));
                alignas(32) std::uint32_t t[8], x[8];
                _mm256_store_si256(reinterpret_cast<__m256i*>(t), sum);
                if (above != 0) {
                        unsigned const k = unsigned(std::countr_zero(above));
                        _mm256_store_si256(reinterpret_cast<__m256i*>(x), idx);
                        rel -= k ? t[k - 1] : 0;
                        s = x[k];
                        break;
                }
                rel -= t[7];
        }
#else
        for (unsigned k = 0;; ++k) {
                std::uint16_t w;
                std::memcpy(&w, c + 12 * k / 8, 2);
                s = unsigned(w >> (12 * k % 8)) & 0xfff;
                std::uint32_t const l = f.length[s];
                if (rel < l) break;
                rel -= l;
        }
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
