//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// The page layouts of the study, as compile-time parameters.
//
// A sub-table is a sequence of pages of B bytes. A page is a header followed by data lines of C bytes; a line holds
// whole codes (Huffman codes, or codes of W bits) and is one cache line when C = 64. The header says where in the
// page's positions each line starts, in one of three forms:
//   counts16    one u16 per line: its positions less one (at most 65,536 positions per line)
//   totals32    one u32 per line: the positions of the page before it (unused lines: 0x7fffffff)
//   eliasfano   the same starts as an Elias-Fano list: low parts, then the high parts in unary. 2 header lines, or 3
//               or 4 when a page covers too many positions for that
// Every header ends with the CRC32C of the page (computed with those 4 bytes zero).
//
// Positions are cut into subdivisions of 2^SUB_BITS for the index (tables.hpp), so that a page's start is a 16-bit
// number within its subdivision. A page may cover many subdivisions. Every stored 16-bit number is below 2^15 and
// every stored 32-bit number below 2^31.

#include <cstddef>
#include <cstdint>

namespace sl {

static_assert(sizeof(std::size_t) == 8, "the arithmetic is in size_t and needs 64 bits");

enum class Header { counts16, totals32, eliasfano };

#ifndef SL_SUB_BITS
#define SL_SUB_BITS 15
#endif

// the header lines of a page with `lines` lines of c bytes and slots of `slot` bytes: one slot per data line and
// `meta` more at the end
constexpr std::size_t header_lines(std::size_t lines, std::size_t c, std::size_t slot, std::size_t meta)
{
        std::size_t h = 1;
        while (h * c / slot < (lines - h) + meta) ++h;
        return h;
}

template<std::size_t C_, int W_, Header H_>
struct Layout {
        static constexpr std::size_t B = 4096;                  // a page
        static constexpr std::size_t C = C_;                    // a line
        static constexpr int W = W_;                            // bits per code; 0: Huffman codes
        static constexpr Header H = H_;
        static constexpr int SUB_BITS = SL_SUB_BITS;
        static constexpr std::size_t SUB = std::size_t(1) << SUB_BITS;
        static constexpr std::size_t LINES = B / C;
        // codes of W bits in a line: the last byte of the line is their number
        static constexpr std::size_t CODES = W ? (8 * C - 8) / std::size_t(W) : 0;
        // header lines (eliasfano: the fewest) and the data lines that leaves
        static constexpr std::size_t HDR_LINES = H == Header::counts16 ? header_lines(LINES, C, 2, 2)
                                               : H == Header::totals32 ? header_lines(LINES, C, 4, 4) : 2;
        static constexpr std::size_t MAX_HDR_LINES = H == Header::eliasfano ? 4 : HDR_LINES;
        static constexpr std::size_t DATA_LINES = LINES - HDR_LINES;
        static constexpr std::size_t LINE_CAP = H == Header::counts16 ? 65536 : (std::size_t(1) << 31) - 1;   // positions of a line

        static_assert(B % C == 0 && C % 16 == 0 && C <= 64);
        static_assert(W == 0 || W == 10 || W == 12);
        static_assert(SUB_BITS >= 8 && SUB_BITS <= 15, "positions relative to a subdivision are stored in 16 bits, below 2^15");
        static_assert(HDR_LINES < LINES);
};

// the variants
using LB64 = Layout<64, 0, Header::counts16>;           // b: his Huffman codes, his block size
using LB32 = Layout<32, 0, Header::counts16>;
using LB16 = Layout<16, 0, Header::counts16>;
using LC0 = Layout<64, 12, Header::counts16>;            // c0
using LC1 = Layout<64, 12, Header::totals32>;            // c1
using LC2 = Layout<64, 12, Header::eliasfano>;           // c2
using LC0_10 = Layout<64, 10, Header::counts16>;         // d with 10-bit codes, on each header
using LC1_10 = Layout<64, 10, Header::totals32>;
using LC2_10 = Layout<64, 10, Header::eliasfano>;

static_assert(LB64::HDR_LINES == 2 && LB64::DATA_LINES == 62);
static_assert(LB32::HDR_LINES == 8 && LB32::DATA_LINES == 120);
static_assert(LC0::CODES == 42 && LC0::HDR_LINES == 2 && LC0::DATA_LINES == 62);
static_assert(LC1::HDR_LINES == 4 && LC1::DATA_LINES == 60);
static_assert(LC0_10::CODES == 50);

}       // namespace sl
