//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// One file per material, as the Syzygy tables have it: every sub-table of the material (wdl with white to move, wdl
// with black to move, the depth of one side) in one file, the sections grouped by kind:
//
//   header        64 bytes: magic, the number of sub-tables, the size of the front part and its CRC32C, the file size
//   directory     one entry per sub-table: what it is, how it is coded, where its sections are
//   grammars      per sub-table: the symbols (and the Huffman code table)
//   page indexes  per sub-table: top, pri (tables.hpp)
//   small data    the pages of every sub-table of at most one page, 64-byte aligned
//   -- the front part ends here: it is small and is read once when the file is opened --
//   data          the pages of every larger sub-table, each sub-table starting on a 4 KiB boundary of the file
//   64 zero bytes (a lookup reads a few bytes past the line it decodes)
//
// A sub-table's last page stops after its last line, so a small sub-table takes its header and lines and no more.
// A sub-table that is a single value has a directory entry and nothing else. The file is written once, front to back.

#include "tables.hpp"
#include <string>
#include <vector>

namespace sl {

constexpr char PACK_MAGIC[8] = { 'E', 'G', 'D', 'B', 'P', 'K', '2', 0 };

struct PackHeader {
        char magic[8];
        std::uint32_t subtables, front_crc;                     // CRC32C of the front part after this header
        std::uint64_t front_bytes, file_bytes;
        std::uint8_t unused[32];
};
static_assert(sizeof(PackHeader) == 64);

enum : std::uint32_t { SEC_GRA, SEC_HUF, SEC_TOP, SEC_PRI, SEC_DAT, SECTIONS };

struct PackEntry {
        std::uint32_t kind, side;                               // the caller's: e.g. 0 wdl, 1 depth; the side to move
        std::uint32_t is_const, const_value;
        std::uint32_t line_bytes, code_bits, header, pages;
        std::uint64_t positions;
        std::uint64_t off[SECTIONS], len[SECTIONS];             // from the start of the file, in bytes
};
static_assert(sizeof(PackEntry) == 40 + 16 * SECTIONS);

struct PackItem { std::uint32_t kind, side; Packed const* p; };

struct PackSizes { std::uint64_t file = 0, front = 0, directory = 0, grammar = 0, index = 0, small_data = 0, data = 0, alignment = 0; };

// the file's bytes
inline std::vector<std::uint8_t> pack(std::vector<PackItem> const& items, PackSizes* sizes = nullptr)
{
        std::vector<std::uint8_t> out(sizeof(PackHeader) + items.size() * sizeof(PackEntry), 0);
        std::vector<PackEntry> dir(items.size());
        PackSizes z;
        z.directory = out.size();
        auto const align = [&](std::size_t a) { std::size_t const n = (out.size() + a - 1) / a * a; z.alignment += n - out.size(); out.resize(n, 0); };
        auto const put = [&](PackEntry& e, std::uint32_t sec, void const* p, std::size_t n, std::size_t a) {
                align(a);
                e.off[sec] = out.size(); e.len[sec] = n;
                out.insert(out.end(), static_cast<std::uint8_t const*>(p), static_cast<std::uint8_t const*>(p) + n);
        };
        auto const dat_bytes = [](Packed const& p) { return p.dat.size() - 4096 + p.last_page_bytes; };
        for (std::size_t i = 0; i < items.size(); ++i) {
                Packed const& p = *items[i].p;
                PackEntry& e = dir[i];
                e = PackEntry{};
                e.kind = items[i].kind; e.side = items[i].side;
                e.is_const = p.is_const; e.const_value = std::uint32_t(p.const_value);
                e.line_bytes = p.C; e.code_bits = p.W; e.header = std::uint32_t(p.H); e.pages = std::uint32_t(p.pri.size()); e.positions = p.positions;
        }
        std::size_t mark = out.size();
        for (std::size_t i = 0; i < items.size(); ++i) {
                Packed const& p = *items[i].p;
                if (p.is_const) continue;
                put(dir[i], SEC_GRA, p.gra.data(), p.gra.size(), 8);
                if (p.W == 0) put(dir[i], SEC_HUF, p.huf.data(), p.huf.size(), 8);
        }
        z.grammar = out.size() - mark; mark = out.size();
        for (std::size_t i = 0; i < items.size(); ++i) {
                Packed const& p = *items[i].p;
                if (p.is_const) continue;
                put(dir[i], SEC_TOP, p.top.data(), 8 * p.top.size(), 8);
                put(dir[i], SEC_PRI, p.pri.data(), 8 * p.pri.size(), 8);
        }
        z.index = out.size() - mark; mark = out.size();
        for (std::size_t i = 0; i < items.size(); ++i) {
                Packed const& p = *items[i].p;
                if (!p.is_const && p.pri.size() == 1) put(dir[i], SEC_DAT, p.dat.data(), dat_bytes(p), 64);
        }
        z.small_data = out.size() - mark;
        z.front = out.size();
        mark = out.size();
        for (std::size_t i = 0; i < items.size(); ++i) {
                Packed const& p = *items[i].p;
                if (!p.is_const && p.pri.size() > 1) put(dir[i], SEC_DAT, p.dat.data(), dat_bytes(p), 4096);
        }
        z.data = out.size() - mark;
        out.resize(out.size() + 64, 0);
        z.file = out.size();
        std::memcpy(out.data() + sizeof(PackHeader), dir.data(), dir.size() * sizeof(PackEntry));
        PackHeader h{};
        std::memcpy(h.magic, PACK_MAGIC, 8);
        h.subtables = std::uint32_t(items.size());
        h.front_bytes = z.front; h.file_bytes = z.file;
        h.front_crc = crc32c(out.data() + sizeof(PackHeader), z.front - sizeof(PackHeader));
        std::memcpy(out.data(), &h, sizeof h);
        if (sizes) *sizes = z;
        return out;
}

struct Package {
        std::uint8_t const* base = nullptr;
        std::size_t size = 0;
        std::vector<PackEntry> dir;
        std::vector<Table> tables;                              // one per directory entry
};

// a mapped package: the header, the front part's CRC and every section's bounds are checked, then each sub-table as
// tables.hpp checks it
inline bool open_package(std::uint8_t const* base, std::size_t size, Package& K, std::string const& name, std::string* why = nullptr, bool validate = true)
{
        auto const fail = [&](char const* m) { if (why) *why = name + ": " + m; return false; };
        PackHeader h;
        if (size < sizeof h) return fail("too short");
        std::memcpy(&h, base, sizeof h);
        if (std::memcmp(h.magic, PACK_MAGIC, 8) != 0) return fail("not a package");
        if (h.file_bytes != size || h.front_bytes > size || h.front_bytes < sizeof h + std::size_t(h.subtables) * sizeof(PackEntry)) return fail("sizes in the header");
        if (crc32c(base + sizeof h, h.front_bytes - sizeof h) != h.front_crc) return fail("CRC of the front part");
        K.base = base; K.size = size;
        K.dir.resize(h.subtables);
        std::memcpy(K.dir.data(), base + sizeof h, h.subtables * sizeof(PackEntry));
        K.tables.resize(h.subtables);
        for (std::size_t i = 0; i < K.dir.size(); ++i) {
                PackEntry const& e = K.dir[i];
                Parts P;
                P.is_const = e.is_const; P.const_value = int(e.const_value);
                if (!P.is_const) {
                        for (std::uint32_t s = 0; s < SECTIONS; ++s) if (e.off[s] > size || e.len[s] > size - e.off[s]) return fail("a section lies outside the file");
                        if (e.pages > 1 && e.off[SEC_DAT] % 4096) return fail("data not on a page boundary");
                        P.C = e.line_bytes; P.W = e.code_bits; P.H = Header(e.header); P.positions = e.positions;
                        P.dat = base + e.off[SEC_DAT]; P.n_dat = e.len[SEC_DAT];
                        P.pri = reinterpret_cast<std::uint64_t const*>(base + e.off[SEC_PRI]); P.n_pri = e.len[SEC_PRI];
                        P.top = reinterpret_cast<std::uint64_t const*>(base + e.off[SEC_TOP]); P.n_top = e.len[SEC_TOP];
                        P.gra = base + e.off[SEC_GRA]; P.n_gra = e.len[SEC_GRA];
                        P.huf = base + e.off[SEC_HUF];
                        if (P.W == 0 && e.len[SEC_HUF] < 2) return fail("no code table");
                }
                if (!open_parts(P, K.tables[i], name + " sub-table " + std::to_string(i), why, validate)) return false;
        }
        return true;
}

// a package file, mapped: the data after the front part without read-ahead
inline bool open_package(std::string const& path, Package& K, std::string* why = nullptr, bool validate = true)
{
        std::size_t size;
        std::uint8_t const* const base = map_file(path, size);
        if (!open_package(base, size, K, path, why, validate)) return false;
        PackHeader h;
        std::memcpy(&h, base, sizeof h);
        std::size_t const from = (h.front_bytes + 4095) / 4096 * 4096;
        if (from < size) ::madvise(const_cast<std::uint8_t*>(base) + from, size - from, MADV_RANDOM);
        return true;
}

}       // namespace sl
