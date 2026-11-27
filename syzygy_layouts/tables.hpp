//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// The study's table files and their lookups. A sub-table of a Syzygy WDL table (one side to move of one pawn file)
// is stored as
//   <p>.dat   pages of 4,096 bytes (layout.hpp)
//   <p>.pri   u32 per page: its first position, relative to its subdivision
//   <p>.top   u32 per 1,024 pages: top[k] = pri[1024 k]
//   <p>.sub   u32 per subdivision: its first page; then the number of pages
//   <p>.gra   u32 symbols, then 3 bytes per symbol (two 12-bit children; a terminal: its value and 0xfff), as in the
//             Syzygy files
//   <p>.huf   Huffman codes only: shortest and longest code, then per length the first symbol and the first code
//   <p>.man   the manifest, "key value" lines
// A lookup of index x: its subdivision q = x >> SUB_BITS and r = x mod 2^SUB_BITS; the page by a search over top and
// then pri within q's pages; the line from the page header; the symbol by adding symbol lengths along the line; the
// value by walking down the symbol's rules.

#include "layout.hpp"
#include <fcntl.h>
#include <immintrin.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace sl {

// ---- grammar and codes

struct Grammar {
        unsigned nsym = 0;
        std::vector<std::uint16_t> left, right;
        std::vector<std::uint32_t> len;                         // positions
        std::vector<std::uint8_t> val;                          // a terminal's value, else 0xff
        std::vector<std::uint16_t> val12;                       // a terminal's value in full (12 bits), else 0xffff
        std::vector<std::uint8_t> sympat;                       // 3 bytes per symbol
};

inline Grammar grammar_from_sympat(std::uint8_t const* p, unsigned n)
{
        Grammar g;
        g.nsym = n;
        g.left.assign(n, 0); g.right.assign(n, 0); g.len.assign(n, 0); g.val.assign(n, 0xff); g.val12.assign(n, 0xffff);
        g.sympat.assign(p, p + 3 * std::size_t(n));
        std::vector<unsigned> stack;
        for (unsigned s0 = 0; s0 < n; ++s0) {
                stack.push_back(s0);
                while (!stack.empty()) {
                        unsigned const s = stack.back();
                        if (g.len[s]) { stack.pop_back(); continue; }
                        std::uint8_t const* w = p + 3 * std::size_t(s);
                        unsigned const s1 = unsigned((w[1] & 0xf) << 8 | w[0]), s2 = unsigned(w[2] << 4 | w[1] >> 4);
                        if (s2 == 0xfff) { g.len[s] = 1; g.val[s] = w[0]; g.val12[s] = std::uint16_t(s1); g.left[s] = g.right[s] = std::uint16_t(s); stack.pop_back(); continue; }
                        if (s1 >= n || s2 >= n) { std::fprintf(stderr, "grammar: bad symbol\n"); std::exit(1); }
                        if (!g.len[s1]) { stack.push_back(s1); continue; }
                        if (!g.len[s2]) { stack.push_back(s2); continue; }
                        g.left[s] = std::uint16_t(s1); g.right[s] = std::uint16_t(s2); g.len[s] = g.len[s1] + g.len[s2];
                        stack.pop_back();
                }
        }
        return g;
}

// the 3-byte form of a grammar given as rules (a terminal: left == right == itself, value in val)
inline std::vector<std::uint8_t> sympat_of(std::vector<std::uint16_t> const& left, std::vector<std::uint16_t> const& right, std::vector<std::uint8_t> const& val)
{
        std::vector<std::uint8_t> p(3 * left.size());
        for (std::size_t s = 0; s < left.size(); ++s) {
                unsigned const a = val[s] != 0xff ? val[s] : left[s], b = val[s] != 0xff ? 0xfffu : right[s];
                p[3 * s] = std::uint8_t(a); p[3 * s + 1] = std::uint8_t(a >> 8 | (b & 0xf) << 4); p[3 * s + 2] = std::uint8_t(b >> 4);
        }
        return p;
}

// the same with terminal values of up to 12 bits (value[s] for a terminal, 0xffff otherwise)
inline std::vector<std::uint8_t> sympat_of12(std::vector<std::uint16_t> const& left, std::vector<std::uint16_t> const& right, std::vector<std::uint16_t> const& value)
{
        std::vector<std::uint8_t> p(3 * left.size());
        for (std::size_t s = 0; s < left.size(); ++s) {
                unsigned const a = value[s] != 0xffff ? value[s] : left[s], b = value[s] != 0xffff ? 0xfffu : right[s];
                p[3 * s] = std::uint8_t(a); p[3 * s + 1] = std::uint8_t(a >> 8 | (b & 0xf) << 4); p[3 * s + 2] = std::uint8_t(b >> 4);
        }
        return p;
}

// canonical Huffman codes as the Syzygy files have them: the symbols are numbered by code length, the longest first
struct Huffman {
        int min_len = 0, max_len = 0;
        std::uint16_t offset[40] = {};                          // by length: the first symbol of that length
        std::uint64_t base[40] = {};                            // by length: the first code of that length, left-aligned
        int length(unsigned s) const { for (int l = min_len; l < max_len; ++l) if (s >= offset[l]) return l; return max_len; }
        // a symbol that occurs only inside other symbols has no code: its number does not come back from decoding
        bool has_code(unsigned s) const
        {
                int const n = length(s);
                std::uint64_t const left = code(s, n) << (64 - n);
                int l = min_len;
                while (l < max_len && left < base[l]) ++l;
                return l == n && left >= base[l] && unsigned(offset[l]) + unsigned((left - base[l]) >> (64 - l)) == s;
        }
        std::uint64_t code(unsigned s, int l) const { return (base[l] >> (64 - l)) + (s - offset[l]); }    // right-aligned
};

// ---- a sub-table as taken from the Syzygy file

struct Source {
        std::string name;                                       // the table
        int k = 0;                                              // the sub-table's number in its file
        std::uint64_t tb_size = 0;
        bool is_const = false;
        int const_value = 0;
        unsigned block_bytes = 64;                              // his block size
        Grammar g;
        Huffman h;
        std::vector<std::uint16_t> seq;                         // the symbols, covering at least tb_size positions
        std::string id() const { return name + "." + std::to_string(k); }
};

// ---- files

inline std::uint32_t crc32c(std::uint8_t const* p, std::size_t n, std::uint32_t crc = 0)
{
        std::uint64_t c = ~crc & 0xffffffffu;
        std::size_t i = 0;
        for (; i + 8 <= n; i += 8) { std::uint64_t x; std::memcpy(&x, p + i, 8); c = _mm_crc32_u64(c, x); }
        for (; i < n; ++i) c = _mm_crc32_u8(std::uint32_t(c), p[i]);
        return ~std::uint32_t(c);
}

inline void write_file(std::string const& path, void const* p, std::size_t n)
{
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f.write(static_cast<char const*>(p), std::streamsize(n));
        if (!f) { std::fprintf(stderr, "cannot write %s\n", path.c_str()); std::exit(1); }
}

// a file mapped read-only, followed by one page of zeros (a line is read in pieces that may run past its end)
// random: no read-ahead, as Fathom asks for its files (a fault reads the one page)
inline std::uint8_t const* map_file(std::string const& path, std::size_t& size, bool random = false)
{
        int const fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) { std::fprintf(stderr, "cannot open %s\n", path.c_str()); std::exit(1); }
        struct stat st;
        ::fstat(fd, &st);
        size = std::size_t(st.st_size);
        std::size_t const span = (size + 4095) / 4096 * 4096 + 4096;
        void* const m = ::mmap(nullptr, span, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m == MAP_FAILED) { std::fprintf(stderr, "mmap failed\n"); std::exit(1); }
        if (size && ::mmap(m, size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) == MAP_FAILED) { std::fprintf(stderr, "mmap failed %s\n", path.c_str()); std::exit(1); }
        if (size && random) ::madvise(m, size, MADV_RANDOM);
        ::close(fd);
        return static_cast<std::uint8_t const*>(m);
}

using Manifest = std::map<std::string, std::string>;
inline Manifest read_manifest(std::string const& path)
{
        Manifest m;
        std::ifstream f(path);
        for (std::string k, v; f >> k && std::getline(f, v);) m[k] = v.empty() ? v : v.substr(1);
        return m;
}

// ---- the table as probed

struct Table {
        std::uint64_t positions = 0;
        std::uint32_t num_pages = 0, num_sub = 0;
        std::uint32_t const* sub = nullptr;
        std::uint32_t const* pri = nullptr;
        std::uint32_t const* top = nullptr;
        std::uint8_t const* dat = nullptr;
        std::uint32_t const* len = nullptr;
        std::uint16_t const* left = nullptr;
        std::uint16_t const* right = nullptr;
        std::uint8_t const* val = nullptr;
        std::uint16_t const* val12 = nullptr;
        std::uint64_t hbase[40] = {};
        std::uint16_t hoffset[40] = {};
        unsigned min_len = 0;
        int (*fn)(Table const&, std::uint64_t) = nullptr;
        bool is_const = false;
        int const_value = 0;
        unsigned C = 64, W = 0;
        Header H = Header::counts16;
        Grammar g;
        Manifest man;
};

constexpr std::size_t SUB_BITS = SL_SUB_BITS, SUB = std::size_t(1) << SUB_BITS, TOP = 1024;

// the page of index x; t: x's position within the page
inline std::uint8_t const* find_page(Table const& T, std::size_t x, std::uint32_t& t)
{
        std::size_t const q = x >> SUB_BITS;
        std::uint32_t const r = std::uint32_t(x & (SUB - 1));
        std::size_t lo = T.sub[q], hi = T.sub[q + 1];
        std::size_t const k0 = (lo + TOP - 1) / TOP, k1 = (hi - 1) / TOP;                // the top entries of pages in [lo, hi)
        if (k0 <= k1) {
                std::size_t const g = std::size_t(std::upper_bound(T.top + k0, T.top + k1 + 1, r) - T.top);
                if (g == k0) hi = k0 * TOP;
                else { lo = (g - 1) * TOP; hi = std::min(hi, g * TOP); }
        }
        std::size_t const b = std::size_t(std::upper_bound(T.pri + lo, T.pri + hi, r) - T.pri) - 1;
        t = r - T.pri[b];
        return T.dat + b * 4096;
}

// counts16: the line of position t of the page; t becomes the position within the line
inline std::size_t line_counts16(std::uint8_t const* page, std::uint32_t& t)
{
        std::size_t g = 0;
        for (;; g += 8) {
                __m256i const v = _mm256_add_epi32(_mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<__m128i const*>(page + 2 * g))), _mm256_set1_epi32(1));
                __m128i s = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
                s = _mm_add_epi32(s, _mm_shuffle_epi32(s, 0x4e));
                s = _mm_add_epi32(s, _mm_shuffle_epi32(s, 0xb1));
                std::uint32_t const sum = std::uint32_t(_mm_cvtsi128_si32(s));
                if (t < sum) break;
                t -= sum;
        }
        for (;; ++g) {
                std::uint16_t c;
                std::memcpy(&c, page + 2 * g, 2);
                if (t <= c) return g;
                t -= std::uint32_t(c) + 1;
        }
}

// totals32: 60 running totals and 4 meta slots
inline std::size_t line_totals32(std::uint8_t const* page, std::uint32_t& t)
{
        __m256i const r = _mm256_set1_epi32(int(t));
        unsigned not_above = 0;
        for (unsigned k = 0; k < 8; ++k) {
                __m256i const h = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(page + 32 * k));
                unsigned m = unsigned(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(h, r))));
                if (k == 7) m |= 0xf0;                          // the meta slots
                not_above += unsigned(std::popcount(~m & 0xffu));
        }
        std::size_t const j = not_above - 1;
        std::uint32_t before;
        std::memcpy(&before, page + 4 * j, 4);
        t -= before;
        return j;
}

// eliasfano: byte 0 the header lines, byte 1 the low bits L, byte 2 the lines n; n low parts of L bits; the upper bits
inline std::size_t line_eliasfano(std::uint8_t const* page, std::uint32_t& t, std::size_t& hdr_lines)
{
        hdr_lines = page[0];
        unsigned const L = page[1], n = page[2];
        std::uint8_t const* const low = page + 3;
        std::uint8_t const* const up = low + (std::size_t(n) * L + 7) / 8;
        std::uint32_t const hi = t >> L, mask = (std::uint32_t(1) << L) - 1, lo = t & mask;
        auto const word = [&](std::size_t w) { std::uint64_t x; std::memcpy(&x, up + 8 * w, 8); return x; };
        auto const low_of = [&](std::size_t k) { std::uint64_t x; std::memcpy(&x, low + k * L / 8, 8); return std::uint32_t(x >> (k * L % 8)) & mask; };
        std::size_t start = 0;                                  // the bit at which bucket hi starts: after zero number hi - 1
        if (hi != 0) {
                std::size_t w = 0;
                std::uint64_t z = ~word(0);
                std::uint32_t r = hi;
                for (std::uint32_t c; (c = std::uint32_t(std::popcount(z))) < r; z = ~word(++w)) r -= c;
                start = w * 64 + std::size_t(_tzcnt_u64(_pdep_u64(std::uint64_t(1) << (r - 1), z))) + 1;
        }
        std::size_t i = start - hi, bit = start;                // the lines before the bucket
        std::uint32_t high = hi;
        bool found = false;
        while (i < n && (word(bit / 64) >> (bit % 64) & 1) && low_of(i) <= lo) { found = true; ++i; ++bit; }
        if (!found) {                                           // the last line before the bucket: back over the zeros
                std::size_t const e = start - 1;
                std::size_t w = e / 64;
                std::uint64_t ones = word(w) & ((std::uint64_t(1) << (e % 64)) - 1);
                while (ones == 0) ones = word(--w);
                high = std::uint32_t(w * 64 + 63 - std::size_t(std::countl_zero(ones)) - (i - 1));
        }
        --i;
        t -= high << L | low_of(i);
        return i;
}

// the symbol holding position rel of a line of W-bit codes (code k at bit W k); rel becomes the position in the symbol
template<int W, std::size_t CODES>
inline unsigned code_in_line(std::uint32_t const* len, std::uint8_t const* c, std::uint32_t& rel)
{
        // 8 codes are W bytes: 4 per half, each taken from the two bytes it lies in and shifted down
        __m256i const pick = W == 12 ? _mm256_setr_epi8(0, 1, -1, -1, 1, 2, -1, -1, 3, 4, -1, -1, 4, 5, -1, -1, 0, 1, -1, -1, 1, 2, -1, -1, 3, 4, -1, -1, 4, 5, -1, -1)
                                     : _mm256_setr_epi8(0, 1, -1, -1, 1, 2, -1, -1, 2, 3, -1, -1, 3, 4, -1, -1, 0, 1, -1, -1, 1, 2, -1, -1, 2, 3, -1, -1, 3, 4, -1, -1);
        __m256i const shift = W == 12 ? _mm256_setr_epi32(0, 4, 0, 4, 0, 4, 0, 4) : _mm256_setr_epi32(0, 2, 4, 6, 0, 2, 4, 6);
        __m256i const mask = _mm256_set1_epi32((1 << W) - 1);
        __m256i const lane = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
        for (unsigned g = 0;; g += 8) {
                std::uint8_t const* const q = c + std::size_t(W) * (g / 8);
                __m256i const raw = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm_loadu_si128(reinterpret_cast<__m128i const*>(q))),
                                                            _mm_loadu_si128(reinterpret_cast<__m128i const*>(q + W / 2)), 1);
                __m256i idx = _mm256_and_si256(_mm256_srlv_epi32(_mm256_shuffle_epi8(raw, pick), shift), mask);
                if (g + 8 > CODES) idx = _mm256_and_si256(idx, _mm256_cmpgt_epi32(_mm256_set1_epi32(int(CODES - g)), lane));   // past the line's codes
                __m256i sum = _mm256_i32gather_epi32(reinterpret_cast<int const*>(len), idx, 4);
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
                        return x[k];
                }
                rel -= t[7];
        }
}

// the same for a line of Huffman codes (most significant bit first), decoded as Fathom decodes a block
inline unsigned huff_in_line(Table const& T, std::uint8_t const* ptr, std::uint32_t& rel)
{
        std::uint64_t code;
        std::memcpy(&code, ptr, 8);
        code = __builtin_bswap64(code);
        ptr += 8;
        unsigned bit_cnt = 0;
        for (;;) {
                unsigned l = T.min_len;
                while (code < T.hbase[l]) ++l;
                unsigned const sym = unsigned(T.hoffset[l]) + unsigned((code - T.hbase[l]) >> (64 - l));
                std::uint32_t const n = T.len[sym];
                if (rel < n) return sym;
                rel -= n;
                code <<= l;
                bit_cnt += l;
                if (bit_cnt >= 32) {
                        bit_cnt -= 32;
                        std::uint32_t x;
                        std::memcpy(&x, ptr, 4);
                        ptr += 4;
                        code |= std::uint64_t(__builtin_bswap32(x)) << bit_cnt;
                }
        }
}

inline int descend(Table const& T, unsigned s, std::uint32_t rel)
{
        while (T.len[s] != 1) {
                unsigned const a = T.left[s];
                std::uint32_t const n = T.len[a];
                if (rel < n) s = a;
                else { rel -= n; s = T.right[s]; }
        }
        return T.val12[s];
}

template<class L>
int probe(Table const& T, std::uint64_t x)
{
        std::uint32_t t;
        std::uint8_t const* const page = find_page(T, x, t);
        std::size_t hdr = L::HDR_LINES;
        std::size_t const j = L::H == Header::counts16 ? line_counts16(page, t) : L::H == Header::totals32 ? line_totals32(page, t) : line_eliasfano(page, t, hdr);
        std::uint8_t const* const line = page + (hdr + j) * L::C;
        unsigned const s = L::W ? code_in_line<L::W ? L::W : 12, L::W ? L::CODES : 42>(T.len, line, t) : huff_in_line(T, line, t);
        return descend(T, s, t);
}

inline int probe_const(Table const& T, std::uint64_t) { return T.const_value; }

// ---- the converter

struct Stats {
        std::uint64_t code_bits = 0, symbols = 0, lines = 0, pages = 0, subdivisions = 0, split_symbols = 0;
        std::uint64_t dat = 0, header = 0, pri = 0, top = 0, sub = 0, grammar = 0, hdr3 = 0, hdr4 = 0, positions = 0;
        std::uint64_t total() const { return dat + pri + top + sub + grammar; }
        std::uint64_t index() const { return header + pri + top + sub; }
        std::uint64_t padding() const { return dat - header - (code_bits + 7) / 8; }
};

struct Line { std::array<std::uint8_t, 64> b{}; std::uint32_t pos = 0, n = 0, bits = 0; bool end_sub = false; };

// The Elias-Fano header of n lines with the given starts, into `out` (hdr_lines * 64 bytes); false if it does not fit.
// The upper bits have a bucket for every high part up to that of the page's last position (not only of the last
// line's start), so that a lookup anywhere in the last line finds its bucket.
inline bool ef_header(std::vector<std::uint32_t> const& starts, std::uint32_t total, std::size_t hdr_lines, std::uint8_t* out)
{
        std::size_t const n = starts.size(), room = hdr_lines * 64 - 7;
        std::uint32_t const last = total - 1;
        unsigned bestL = 0;
        std::size_t best = ~std::size_t(0);
        for (unsigned L = 0; L <= 31; ++L) {
                std::size_t const bytes = (n * L + 7) / 8 + (n + (last >> L) + 1 + 7) / 8;
                if (bytes < best) { best = bytes; bestL = L; }
        }
        if (best > room) return false;
        unsigned const L = bestL;
        out[0] = std::uint8_t(hdr_lines); out[1] = std::uint8_t(L); out[2] = std::uint8_t(n);
        std::uint8_t* const low = out + 3;
        std::uint8_t* const up = low + (n * L + 7) / 8;
        for (std::size_t i = 0; i < n; ++i) {
                std::uint64_t const lo = starts[i] & ((std::uint32_t(1) << L) - 1);
                for (unsigned k = 0; k < L; ++k) if (lo >> k & 1) low[(i * L + k) / 8] |= std::uint8_t(1u << ((i * L + k) % 8));
                std::size_t const bit = (starts[i] >> L) + i;
                up[bit / 8] |= std::uint8_t(1u << (bit % 8));
        }
        return true;
}

template<class L>
Stats convert(Source const& S, std::string const& prefix, std::string const& variant)
{
        Stats st;
        Manifest man;
        man["variant"] = variant; man["table"] = S.name; man["subtable"] = std::to_string(S.k); man["tb_size"] = std::to_string(S.tb_size);
        man["line_bytes"] = std::to_string(L::C); man["code_bits"] = std::to_string(L::W);
        man["header"] = L::H == Header::counts16 ? "counts16" : L::H == Header::totals32 ? "totals32" : "eliasfano";
        man["sub_bits"] = std::to_string(L::SUB_BITS);
        auto const write_manifest = [&] {
                std::ostringstream o;
                for (auto const& [k, v] : man) o << k << ' ' << v << '\n';
                write_file(prefix + ".man", o.str().data(), o.str().size());
        };
        if (S.is_const) {
                man["const"] = std::to_string(S.const_value);
                st.grammar = 2;
                write_manifest();
                return st;
        }
        Grammar const& g = S.g;
        // 1. lines, cut at the subdivisions
        std::vector<Line> lines;
        Line cur;
        std::size_t pos_in_sub = 0;
        auto const close = [&](bool end_sub) {
                if (cur.n == 0) { if (end_sub && !lines.empty()) lines.back().end_sub = true; return; }
                if (L::W) cur.b[L::C - 1] = std::uint8_t(cur.n);
                cur.end_sub = end_sub;
                st.code_bits += cur.bits;
                lines.push_back(cur);
                cur = Line{};
        };
        auto const put = [&](unsigned s) {
                std::uint32_t const bits = L::W ? std::uint32_t(L::W) : std::uint32_t(S.h.length(s));
                bool const full = L::W ? cur.n == L::CODES : cur.bits + bits > 8 * L::C;
                if (cur.n != 0 && (full || std::size_t(cur.pos) + g.len[s] > L::LINE_CAP)) close(false);
                if (g.len[s] > L::LINE_CAP) { std::fprintf(stderr, "a symbol of %u positions does not fit a line\n", g.len[s]); std::exit(1); }
                if (L::W) {
                        std::size_t const at = std::size_t(L::W) * cur.n;
                        std::uint16_t w;
                        std::memcpy(&w, cur.b.data() + at / 8, 2);
                        w = std::uint16_t(w | s << (at % 8));
                        std::memcpy(cur.b.data() + at / 8, &w, 2);
                } else {
                        std::uint64_t const cw = S.h.code(s, int(bits));
                        for (std::uint32_t k = 0; k < bits; ++k)
                                if (cw >> (bits - 1 - k) & 1) cur.b[(cur.bits + k) / 8] |= std::uint8_t(0x80u >> ((cur.bits + k) % 8));
                }
                cur.bits += bits; cur.pos += g.len[s]; ++cur.n;
                ++st.symbols;
        };
        std::vector<unsigned> stack;
        for (std::uint16_t s0 : S.seq) {
                stack.push_back(s0);
                while (!stack.empty()) {
                        unsigned const s = stack.back();
                        stack.pop_back();
                        bool const coded = L::W != 0 || S.h.has_code(s);       // b: a piece of a cut symbol may have no Huffman code
                        if (!coded && g.len[s] == 1) { std::fprintf(stderr, "%s: a value without a Huffman code\n", S.id().c_str()); std::exit(1); }
                        if (coded && g.len[s] <= L::SUB - pos_in_sub) {
                                put(s);
                                pos_in_sub += g.len[s];
                                st.positions += g.len[s];
                                if (pos_in_sub == L::SUB) { close(true); pos_in_sub = 0; }
                        } else {                                // straddles the subdivision's end, or has no code: its children instead
                                ++st.split_symbols;
                                stack.push_back(g.right[s]);
                                stack.push_back(g.left[s]);
                        }
                }
        }
        close(true);
        if (st.positions < S.tb_size) { std::fprintf(stderr, "%s: the symbols cover %llu of %llu positions\n", S.id().c_str(), (unsigned long long)st.positions, (unsigned long long)S.tb_size); std::exit(1); }
        st.lines = lines.size();
        // 2. pages
        std::vector<std::uint8_t> dat;
        std::vector<std::uint32_t> pri, sub;
        std::size_t page_start = 0;                             // position within the subdivision
        bool new_sub = true;
        for (std::size_t i = 0; i < lines.size();) {
                if (new_sub) { sub.push_back(std::uint32_t(pri.size())); page_start = 0; new_sub = false; }
                std::size_t avail = 0;                          // lines up to the subdivision's end
                while (i + avail < lines.size() && (avail == 0 || !lines[i + avail - 1].end_sub)) ++avail;
                std::array<std::uint8_t, 4096> page{};
                std::size_t n = 0, hdr = L::HDR_LINES;
                if (L::H == Header::eliasfano) {
                        for (;; ++hdr) {
                                if (hdr > L::MAX_HDR_LINES) { std::fprintf(stderr, "eliasfano: a page header does not fit\n"); std::exit(1); }
                                n = std::min(avail, L::LINES - hdr);
                                std::vector<std::uint32_t> starts(n);
                                std::uint32_t p = 0;
                                for (std::size_t j = 0; j < n; ++j) { starts[j] = p; p += lines[i + j].pos; }
                                std::fill(page.begin(), page.begin() + 256, std::uint8_t(0));
                                if (ef_header(starts, p, hdr, page.data())) break;
                        }
                        st.hdr3 += hdr == 3; st.hdr4 += hdr == 4;
                } else {
                        n = std::min(avail, L::DATA_LINES);
                        std::uint32_t p = 0;
                        for (std::size_t j = 0; j < L::DATA_LINES; ++j) {
                                if (L::H == Header::counts16) {
                                        std::uint16_t const c = j < n ? std::uint16_t(lines[i + j].pos - 1) : std::uint16_t(0);
                                        std::memcpy(page.data() + 2 * j, &c, 2);
                                } else {
                                        std::uint32_t const c = j < n ? p : 0x7fffffffu;
                                        std::memcpy(page.data() + 4 * j, &c, 4);
                                }
                                if (j < n) p += lines[i + j].pos;
                        }
                }
                std::size_t page_positions = 0;
                for (std::size_t j = 0; j < n; ++j) {
                        std::memcpy(page.data() + (hdr + j) * L::C, lines[i + j].b.data(), L::C);
                        page_positions += lines[i + j].pos;
                }
                if (page_positions >= (std::size_t(1) << 31) || page_start >= (std::size_t(1) << 31)) { std::fprintf(stderr, "a stored number is not below 2^31\n"); std::exit(1); }
                std::uint32_t const crc = crc32c(page.data(), 4096);            // with the 4 CRC bytes zero
                std::memcpy(page.data() + hdr * L::C - 4, &crc, 4);
                dat.insert(dat.end(), page.begin(), page.end());
                pri.push_back(std::uint32_t(page_start));
                st.header += hdr * L::C;
                page_start += page_positions;
                new_sub = lines[i + n - 1].end_sub;
                i += n;
        }
        std::uint32_t const num_pages = std::uint32_t(pri.size());
        sub.push_back(num_pages);
        std::vector<std::uint32_t> top((pri.size() + TOP - 1) / TOP);
        for (std::size_t k = 0; k < top.size(); ++k) top[k] = pri[k * TOP];
        // 3. files
        write_file(prefix + ".dat", dat.data(), dat.size());
        write_file(prefix + ".pri", pri.data(), 4 * pri.size());
        write_file(prefix + ".top", top.data(), 4 * top.size());
        write_file(prefix + ".sub", sub.data(), 4 * sub.size());
        {
                std::vector<std::uint8_t> gra(4 + g.sympat.size());
                std::uint32_t const n = g.nsym;
                std::memcpy(gra.data(), &n, 4);
                std::memcpy(gra.data() + 4, g.sympat.data(), g.sympat.size());
                write_file(prefix + ".gra", gra.data(), gra.size());
                st.grammar = gra.size();
        }
        if (L::W == 0) {
                std::vector<std::uint8_t> huf(2);
                huf[0] = std::uint8_t(S.h.min_len); huf[1] = std::uint8_t(S.h.max_len);
                for (int l = S.h.min_len; l <= S.h.max_len; ++l) {
                        std::size_t const at = huf.size();
                        huf.resize(at + 10);
                        std::memcpy(huf.data() + at, &S.h.offset[l], 2);
                        std::memcpy(huf.data() + at + 2, &S.h.base[l], 8);
                }
                write_file(prefix + ".huf", huf.data(), huf.size());
                st.grammar += huf.size();
        }
        st.pages = num_pages; st.subdivisions = sub.size() - 1;
        st.dat = dat.size(); st.pri = 4 * pri.size(); st.top = 4 * top.size(); st.sub = 4 * sub.size();
        man["positions"] = std::to_string(st.positions); man["pages"] = std::to_string(num_pages); man["subdivisions"] = std::to_string(sub.size() - 1);
        man["symbols"] = std::to_string(g.nsym); man["codes"] = std::to_string(st.symbols); man["code_bits_total"] = std::to_string(st.code_bits);
        man["dat_bytes"] = std::to_string(st.dat); man["pri_bytes"] = std::to_string(st.pri); man["top_bytes"] = std::to_string(st.top);
        man["sub_bytes"] = std::to_string(st.sub); man["grammar_bytes"] = std::to_string(st.grammar); man["header_bytes"] = std::to_string(st.header);
        man["dat_crc32c"] = std::to_string(crc32c(dat.data(), dat.size()));
        write_manifest();
        return st;
}

// ---- opening a table, with the checks of the prompt

template<class L> int (*probe_fn())(Table const&, std::uint64_t) { return &probe<L>; }

// validate = false skips the checks that read all of pri (a cold-cache timing run must not warm the index by opening)
inline bool open_table(std::string const& prefix, Table& T, std::string* why = nullptr, bool validate = true)
{
        auto const fail = [&](char const* m) { if (why) *why = prefix + ": " + m; return false; };
        T.man = read_manifest(prefix + ".man");
        if (T.man.empty()) return fail("no manifest");
        if (T.man.count("const")) { T.is_const = true; T.const_value = std::stoi(T.man["const"]); T.fn = &probe_const; return true; }
        T.C = unsigned(std::stoul(T.man["line_bytes"])); T.W = unsigned(std::stoul(T.man["code_bits"]));
        T.H = T.man["header"] == "counts16" ? Header::counts16 : T.man["header"] == "totals32" ? Header::totals32 : Header::eliasfano;
        if (std::stoul(T.man["sub_bits"]) != SUB_BITS) return fail("made with another subdivision size");
        T.positions = std::stoull(T.man["positions"]);
        std::size_t n_dat, n_pri, n_top, n_sub, n_gra;
        T.dat = map_file(prefix + ".dat", n_dat, true);
        T.pri = reinterpret_cast<std::uint32_t const*>(map_file(prefix + ".pri", n_pri));
        T.top = reinterpret_cast<std::uint32_t const*>(map_file(prefix + ".top", n_top));
        T.sub = reinterpret_cast<std::uint32_t const*>(map_file(prefix + ".sub", n_sub));
        std::uint8_t const* const gra = map_file(prefix + ".gra", n_gra);
        T.num_pages = std::uint32_t(n_dat / 4096);
        T.num_sub = std::uint32_t(n_sub / 4 - 1);
        if (n_dat % 4096 || n_pri != 4 * std::size_t(T.num_pages) || n_top != 4 * ((std::size_t(T.num_pages) + TOP - 1) / TOP)) return fail("file sizes disagree");
        if (T.num_sub != (T.positions + SUB - 1) / SUB) return fail("subdivision count");
        if (T.sub[0] != 0 || T.sub[T.num_sub] != T.num_pages) return fail("sub does not span the pages");
        for (std::uint32_t q = 0; validate && q < T.num_sub; ++q) {
                if (T.sub[q] >= T.sub[q + 1] || T.pri[T.sub[q]] != 0) return fail("a subdivision does not start at 0");
                for (std::uint32_t b = T.sub[q] + 1; b < T.sub[q + 1]; ++b) if (T.pri[b] <= T.pri[b - 1] || T.pri[b] >= (std::uint32_t(1) << 31)) return fail("pri not increasing");
        }
        for (std::size_t k = 0; validate && k < n_top / 4; ++k) if (T.top[k] != T.pri[k * TOP]) return fail("top disagrees with pri");
        std::uint32_t nsym;
        std::memcpy(&nsym, gra, 4);
        if (n_gra != 4 + 3 * std::size_t(nsym)) return fail("grammar size");
        T.g = grammar_from_sympat(gra + 4, nsym);
        T.len = T.g.len.data(); T.left = T.g.left.data(); T.right = T.g.right.data(); T.val = T.g.val.data(); T.val12 = T.g.val12.data();
        if (T.W == 0) {
                std::size_t n_huf;
                std::uint8_t const* const huf = map_file(prefix + ".huf", n_huf);
                T.min_len = huf[0];
                for (unsigned l = huf[0], at = 2; l <= huf[1]; ++l, at += 10) { std::memcpy(&T.hoffset[l], huf + at, 2); std::memcpy(&T.hbase[l], huf + at + 2, 8); }
                T.fn = T.C == 64 ? probe_fn<LB64>() : T.C == 32 ? probe_fn<LB32>() : T.C == 16 ? probe_fn<LB16>() : nullptr;
        } else if (T.W == 12)
                T.fn = T.H == Header::counts16 ? probe_fn<LC0>() : T.H == Header::totals32 ? probe_fn<LC1>() : probe_fn<LC2>();
        else if (T.W == 10)
                T.fn = T.H == Header::counts16 ? probe_fn<LC0_10>() : T.H == Header::totals32 ? probe_fn<LC1_10>() : probe_fn<LC2_10>();
        if (!T.fn) return fail("no lookup for this layout");
        return true;
}

// ---- every value in order, read through the page headers (the sequential check); false on an inconsistency

// sink(values, n, first index) is called per page and returns false to stop
template<class Sink>
bool decode_pages(Table const& T, Sink sink, std::string* why = nullptr)
{
        auto const fail = [&](std::string const& m) { if (why) *why = m; return false; };
        std::vector<std::uint8_t> out;
        std::size_t done = 0;
        std::size_t const lines = 4096 / T.C;
        std::size_t const fixed_hdr = T.H == Header::counts16 ? header_lines(lines, T.C, 2, 2) : header_lines(lines, T.C, 4, 4);
        std::vector<unsigned> stack;
        auto const expand = [&](unsigned s0) {
                stack.push_back(s0);
                while (!stack.empty()) {
                        unsigned const s = stack.back();
                        stack.pop_back();
                        if (T.len[s] == 1) { out.push_back(T.val[s]); continue; }
                        stack.push_back(T.right[s]); stack.push_back(T.left[s]);
                }
        };
        for (std::uint32_t q = 0; q < T.num_sub; ++q) {
                std::size_t const sub_positions = q + 1 < T.num_sub ? SUB : T.positions - std::size_t(q) * SUB;
                for (std::uint32_t b = T.sub[q]; b < T.sub[q + 1]; ++b) {
                        std::uint8_t const* const page = T.dat + std::size_t(b) * 4096;
                        std::size_t const total = (b + 1 < T.sub[q + 1] ? T.pri[b + 1] : sub_positions) - T.pri[b];
                        if (done != std::size_t(q) * SUB + T.pri[b]) return fail("pri disagrees with the pages before it");
                        std::size_t hdr = fixed_hdr;
                        std::vector<std::size_t> starts;                // of each line, then the total
                        if (T.H == Header::counts16) {
                                for (std::size_t j = 0, p = 0; p < total; ++j) { std::uint16_t c; std::memcpy(&c, page + 2 * j, 2); starts.push_back(p); p += std::size_t(c) + 1; }
                        } else if (T.H == Header::totals32) {
                                for (std::size_t j = 0; j < 60; ++j) { std::uint32_t c; std::memcpy(&c, page + 4 * j, 4); if (c == 0x7fffffffu) break; starts.push_back(c); }
                        } else {
                                hdr = page[0];
                                unsigned const Lb = page[1], n = page[2];
                                std::uint8_t const* const low = page + 3;
                                std::uint8_t const* const up = low + (std::size_t(n) * Lb + 7) / 8;
                                for (std::size_t i = 0, bit = 0; i < n; ++bit) {
                                        if (!(up[bit / 8] >> (bit % 8) & 1)) continue;
                                        std::uint64_t x; std::memcpy(&x, low + i * Lb / 8, 8);
                                        starts.push_back(((bit - i) << Lb) | (std::uint32_t(x >> (i * Lb % 8)) & ((std::uint32_t(1) << Lb) - 1)));
                                        ++i;
                                }
                        }
                        starts.push_back(total);
                        std::uint32_t stored, zero = 0;
                        std::memcpy(&stored, page + hdr * T.C - 4, 4);
                        std::array<std::uint8_t, 4096> copy;
                        std::memcpy(copy.data(), page, 4096);
                        std::memcpy(copy.data() + hdr * T.C - 4, &zero, 4);
                        if (crc32c(copy.data(), 4096) != stored) return fail("page CRC");
                        for (std::size_t j = 0; j + 1 < starts.size(); ++j) {
                                if (starts[j + 1] <= starts[j]) return fail("line starts not increasing");
                                std::uint8_t const* const line = page + (hdr + j) * T.C;
                                std::size_t const want = out.size() + (starts[j + 1] - starts[j]);
                                if (T.W) {
                                        unsigned const n = line[T.C - 1];
                                        for (unsigned k = 0; k < n; ++k) {
                                                std::uint16_t w; std::memcpy(&w, line + std::size_t(T.W) * k / 8, 2);
                                                expand(unsigned(w >> (T.W * k % 8)) & ((1u << T.W) - 1));
                                        }
                                } else {
                                        std::size_t bit = 0;
                                        while (out.size() < want) {
                                                std::uint64_t code = 0;
                                                for (unsigned k = 0; k < 64; ++k) { std::size_t const a = bit + k; code = code << 1 | (a < 8 * T.C ? (line[a / 8] >> (7 - a % 8) & 1) : 0); }
                                                unsigned l = T.min_len;
                                                while (code < T.hbase[l]) ++l;
                                                expand(unsigned(T.hoffset[l]) + unsigned((code - T.hbase[l]) >> (64 - l)));
                                                bit += l;
                                        }
                                }
                                if (out.size() != want) return fail("a line does not hold the positions its header says");
                        }
                        if (!sink(out.data(), out.size(), done)) return fail("the values differ");
                        done += out.size();
                        out.clear();
                }
        }
        if (done != T.positions) return fail("positions");
        return true;
}

inline bool decode_all(Table const& T, std::vector<std::uint8_t>& all, std::string* why = nullptr)
{
        all.clear();
        all.reserve(T.positions);
        return decode_pages(T, [&](std::uint8_t const* v, std::size_t n, std::size_t) { all.insert(all.end(), v, v + n); return true; }, why);
}

}       // namespace sl
