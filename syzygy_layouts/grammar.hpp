//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// A grammar for fixed-width codes built on a WDL sub-table whose don't-care positions are still open.
//
// The input is the table as the Syzygy generator hands it to its compression step: 0..4 are the values, 5..8 mark a
// position whose value does not matter, by class: 5 may become 0 or 1; 6: 0, 1 or 2; 7: 0 to 3; 8: any value.
// As in his compressor, a stretch of don't-cares first takes the value of a neighbour where that is allowed
// (prefill, his rule); what remains open is settled during the pairing: an open position counts towards every pair
// it could complete, and takes its value when a pair is replaced. Anything still open at the end takes the most
// frequent value it may have.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace sl {

constexpr bool dc_allows(unsigned value, unsigned cls)           // value 0..4; cls 0..8 (below 5: that value itself)
{
        return cls < 5 ? cls == value : value <= cls - 4;
}

// remove_wdl_worker of syzygy1/tb compress.c, on the whole table
inline void prefill(std::vector<std::uint8_t>& data)
{
        std::array<bool, 5> has{};
        for (std::uint8_t v : data) if (v < 5) has[v] = true;
        std::uint8_t to_val[4];
        to_val[0] = has[0] ? 0 : 1;
        to_val[1] = has[2] ? 2 : to_val[0];
        to_val[2] = has[2] ? 2 : (has[0] ? 0 : (has[3] ? 3 : 1));
        to_val[3] = has[4] ? 4 : to_val[2];
        std::size_t const size = data.size();
        for (std::size_t idx = 0; idx < size;) {
                while (idx < size && data[idx] < 5) ++idx;
                if (idx == size) break;
                unsigned s = data[idx];                         // the narrowest class of the stretch
                std::size_t idx2 = idx + 1;
                for (; idx2 < size && data[idx2] >= 5; ++idx2) s = std::min<unsigned>(s, data[idx2]);
                int f = 0;
                std::uint8_t t = 0;
                if (idx > 0 && dc_allows(data[idx - 1], s)) {
                        t = data[idx - 1];
                        f = 1;
                        if (idx > 1 && data[idx - 2] == t) f = 2;
                }
                if (idx2 < size && dc_allows(data[idx2], s) && f != 2) {
                        if (!f || (idx2 < size - 1 && dc_allows(data[idx2], data[idx2 + 1]))) t = data[idx2];
                        f = 1;
                }
                if (f) std::fill(data.begin() + std::ptrdiff_t(idx), data.begin() + std::ptrdiff_t(idx2), t);
                else if (idx2 - idx > 5) std::fill(data.begin() + std::ptrdiff_t(idx), data.begin() + std::ptrdiff_t(idx2), to_val[s - 5]);
                idx = idx2;
        }
}

struct Built {
        std::vector<std::uint16_t> left, right;                 // per symbol; 0..8 are the terminals
        std::vector<std::uint8_t> val;                          // a terminal's value, else 0xff
        std::vector<std::uint16_t> seq;
        std::uint64_t open_before = 0, open_paired = 0, open_left = 0;      // don't-cares after prefill; settled by a pair; by default
};

// Pairing in passes, as rpvf::compress_wide: the most frequent pair, later a batch of frequent pairs with no symbol in
// common. A symbol covers at most maxlen positions; a pair needs min_count occurrences; at most max_symbols symbols.
inline Built build_open(std::vector<std::uint8_t> const& data, std::uint32_t maxlen, std::uint32_t min_count, unsigned max_symbols)
{
        constexpr std::size_t W = 4096;
        constexpr unsigned T = 9;
        Built B;
        B.left.resize(T); B.right.resize(T);
        std::vector<std::uint32_t> length(T, 1);
        for (unsigned t = 0; t < T; ++t) B.left[t] = B.right[t] = std::uint16_t(t);
        std::vector<std::uint16_t>& seq = B.seq;
        seq.assign(data.begin(), data.end());
        auto const open = [](unsigned x) { return x - 5u < 4u; };
        for (std::uint16_t x : seq) B.open_before += open(x);
        std::vector<std::uint32_t> count(W * W, 0), touched;
        std::vector<std::uint16_t> partner(W, 0), made(W, 0);
        std::vector<bool> is_left(W, false);
        for (int pass = 0; B.left.size() < max_symbols; ++pass) {
                touched.clear();
                auto const add = [&](unsigned a, unsigned c) { std::uint32_t const key = std::uint32_t(a * W + c); if (count[key]++ == 0) touched.push_back(key); };
                bool skip = false;
                for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
                        unsigned const x = seq[i], y = seq[i + 1];
                        if (!open(x) && !open(y)) {
                                if (skip) { skip = false; if (x == y && seq[i - 1] == x) continue; }       // a a a: one pair
                                add(x, y);
                                skip = x == y;
                                continue;
                        }
                        skip = false;
                        for (unsigned a = open(x) ? 0 : x; a < (open(x) ? 5u : x + 1); ++a) {
                                if (open(x) && !dc_allows(a, x)) continue;
                                for (unsigned c = open(y) ? 0 : y; c < (open(y) ? 5u : y + 1); ++c)
                                        if (!open(y) || dc_allows(c, y)) add(a, c);
                        }
                }
                std::vector<std::pair<std::uint32_t, std::uint32_t>> cand;
                for (std::uint32_t key : touched) {
                        if (count[key] >= min_count && length[key / W] + length[key % W] <= maxlen) cand.push_back({ count[key], key });
                        count[key] = 0;
                }
                if (cand.empty()) break;
                std::size_t const nsym = B.left.size();
                std::size_t const batch = std::min<std::size_t>({ cand.size(), max_symbols - nsym, pass < 16 ? std::size_t(1) : std::max<std::size_t>(1, nsym / 16) });
                std::size_t const look = std::min(cand.size(), 4 * batch);
                std::partial_sort(cand.begin(), cand.begin() + std::ptrdiff_t(look), cand.end(), std::greater<>());
                std::vector<bool> used(W, false);
                std::vector<unsigned> lefts;
                for (std::size_t k = 0; k < look && lefts.size() < batch; ++k) {
                        unsigned const a = cand[k].second / W, c = cand[k].second % W;
                        if (2 * cand[k].first < cand[0].first || used[a] || used[c]) continue;        // near the best only, no symbol twice
                        used[a] = used[c] = true;
                        is_left[a] = true; partner[a] = std::uint16_t(c); made[a] = std::uint16_t(B.left.size());
                        lefts.push_back(a);
                        B.left.push_back(std::uint16_t(a)); B.right.push_back(std::uint16_t(c)); length.push_back(length[a] + length[c]);
                }
                // replace, left to right; an open position becomes what the pair needs
                auto const fits = [&](unsigned want, unsigned have) { return have == want || (open(have) && want < 5 && dc_allows(want, have)); };
                std::size_t o = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        unsigned const x = seq[i];
                        if (i + 1 < seq.size()) {
                                unsigned const y = seq[i + 1];
                                unsigned a = W;
                                if (!open(x)) { if (is_left[x] && fits(partner[x], y)) a = x; }
                                else for (unsigned v = 0; v < 5; ++v) if (dc_allows(v, x) && is_left[v] && fits(partner[v], y)) { a = v; break; }
                                if (a != W) { B.open_paired += open(x) + open(y); seq[o++] = made[a]; ++i; continue; }
                        }
                        seq[o++] = std::uint16_t(x);
                }
                seq.resize(o);
                for (unsigned a : lefts) is_left[a] = false;
        }
        // what is still open: the most frequent value it may have
        std::array<std::uint64_t, 5> freq{};
        for (std::uint16_t x : seq) if (x < 5) ++freq[x];
        std::array<std::uint8_t, 9> to{};
        for (unsigned c = 5; c < 9; ++c) { unsigned best = 0; for (unsigned v = 1; v < 5; ++v) if (dc_allows(v, c) && freq[v] > freq[best]) best = v; to[c] = std::uint8_t(best); }
        for (std::uint16_t& x : seq) if (open(x)) { ++B.open_left; x = to[x]; }
        B.val.assign(B.left.size(), 0xff);
        for (unsigned t = 0; t < T; ++t) B.val[t] = std::uint8_t(t < 5 ? t : to[t]);
        return B;
}

// The same pairing with every don't-care kept as a joker that may become part of any symbol of the grammar, not only
// a single value (no neighbour fill beforehand). When a pair (a, c) is replaced, a stretch of jokers after an a may
// become the c, and a stretch before a c may become the a, provided every position of the stretch allows every value
// of that symbol. A symbol next to a long enough stretch counts towards doubling itself. Jokers left at the end are
// covered by the longest symbols that fit.
inline Built build_joker(std::vector<std::uint8_t> const& data, std::uint32_t maxlen, std::uint32_t min_count, unsigned max_symbols)
{
        constexpr std::size_t W = 4096;
        constexpr unsigned T = 9;
        Built B;
        B.left.resize(T); B.right.resize(T);
        std::vector<std::uint32_t> length(T, 1);
        std::vector<std::uint8_t> maxval(T, 0);                 // the largest value in a symbol
        for (unsigned t = 0; t < T; ++t) { B.left[t] = B.right[t] = std::uint16_t(t); maxval[t] = std::uint8_t(t < 5 ? t : 0); }
        std::vector<std::uint16_t>& seq = B.seq;
        seq.assign(data.begin(), data.end());
        auto const open = [](unsigned x) { return x - 5u < 4u; };
        auto const lim = [](unsigned x) { return x - 4u; };     // the largest value a joker of class x may take
        for (std::uint16_t x : seq) B.open_before += open(x);
        // n jokers from j on, each allowing the value mv
        auto const run_fwd = [&](std::size_t j, std::size_t n, unsigned mv) {
                if (j + n > seq.size()) return false;
                for (std::size_t k = 0; k < n; ++k) if (!open(seq[j + k]) || lim(seq[j + k]) < mv) return false;
                return true;
        };
        auto const run_back = [&](std::size_t end, std::size_t n, unsigned mv) {     // the n items before `end`
                if (end < n) return false;
                for (std::size_t k = 1; k <= n; ++k) if (!open(seq[end - k]) || lim(seq[end - k]) < mv) return false;
                return true;
        };
        std::vector<std::uint32_t> count(W * W, 0), touched;
        std::vector<std::uint16_t> partner(W, 0), lpartner(W, 0), made(W, 0);
        std::vector<std::uint8_t> is_left(W, 0), is_right(W, 0);
        for (int pass = 0; B.left.size() < max_symbols; ++pass) {
                touched.clear();
                auto const add = [&](unsigned a, unsigned c) { std::uint32_t const key = std::uint32_t(a * W + c); if (count[key]++ == 0) touched.push_back(key); };
                bool skip = false;
                for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
                        unsigned const x = seq[i], y = seq[i + 1];
                        bool const ox = open(x), oy = open(y);
                        if (!ox && !oy) {
                                if (skip) { skip = false; if (x == y && seq[i - 1] == x) continue; }
                                add(x, y);
                                skip = x == y;
                                continue;
                        }
                        skip = false;
                        if (ox && oy) continue;
                        if (oy) {
                                for (unsigned c = 0; c <= lim(y); ++c) add(x, c);
                                if (x >= T && maxval[x] <= lim(y) && run_fwd(i + 1, length[x], maxval[x])) add(x, x);
                        } else {
                                for (unsigned a = 0; a <= lim(x); ++a) add(a, y);
                                if (y >= T && maxval[y] <= lim(x) && run_back(i + 1, length[y], maxval[y])) add(y, y);
                        }
                }
                std::vector<std::pair<std::uint32_t, std::uint32_t>> cand;
                for (std::uint32_t key : touched) {
                        if (count[key] >= min_count && length[key / W] + length[key % W] <= maxlen) cand.push_back({ count[key], key });
                        count[key] = 0;
                }
                if (cand.empty()) break;
                std::size_t const nsym = B.left.size();
                std::size_t const batch = std::min<std::size_t>({ cand.size(), max_symbols - nsym, pass < 16 ? std::size_t(1) : std::max<std::size_t>(1, nsym / 16) });
                std::size_t const look = std::min(cand.size(), 4 * batch);
                std::partial_sort(cand.begin(), cand.begin() + std::ptrdiff_t(look), cand.end(), std::greater<>());
                std::vector<bool> used(W, false);
                std::vector<std::pair<unsigned, unsigned>> chosen;
                for (std::size_t k = 0; k < look && chosen.size() < batch; ++k) {
                        unsigned const a = cand[k].second / W, c = cand[k].second % W;
                        if (2 * cand[k].first < cand[0].first || used[a] || used[c]) continue;
                        used[a] = used[c] = true;
                        is_left[a] = 1; partner[a] = std::uint16_t(c); made[a] = std::uint16_t(B.left.size());
                        is_right[c] = 1; lpartner[c] = std::uint16_t(a);
                        chosen.push_back({ a, c });
                        B.left.push_back(std::uint16_t(a)); B.right.push_back(std::uint16_t(c)); length.push_back(length[a] + length[c]);
                        maxval.push_back(std::max(maxval[a], maxval[c]));
                }
                // replace, left to right, compacting in place (o <= i: what lies ahead is still the input)
                std::size_t o = 0;
                for (std::size_t i = 0; i < seq.size(); ++i) {
                        unsigned const x = seq[i];
                        if (open(x)) { seq[o++] = std::uint16_t(x); continue; }
                        if (is_left[x] && i + 1 < seq.size()) {
                                unsigned const c = partner[x], y = seq[i + 1];
                                if (y == c) { seq[o++] = made[x]; ++i; continue; }
                                if (open(y) && run_fwd(i + 1, length[c], maxval[c])) { B.open_paired += length[c]; seq[o++] = made[x]; i += length[c]; continue; }
                        }
                        if (is_right[x]) {
                                unsigned const a = lpartner[x];
                                std::size_t const la = length[a];
                                bool ok = o >= la;
                                for (std::size_t k = 1; ok && k <= la; ++k) ok = open(seq[o - k]) && lim(seq[o - k]) >= maxval[a];
                                if (ok) { B.open_paired += la; o -= la; seq[o++] = made[a]; continue; }
                        }
                        seq[o++] = std::uint16_t(x);
                }
                seq.resize(o);
                for (auto const& [a, c] : chosen) { is_left[a] = 0; is_right[c] = 0; }
        }
        // the jokers that are left: the longest symbols that fit, per largest value allowed
        std::array<std::vector<unsigned>, 5> by_len;
        for (unsigned m = 0; m < 5; ++m) {
                for (unsigned s = 0; s < B.left.size(); ++s) if (!open(s) && maxval[s] <= m) by_len[m].push_back(s);
                std::stable_sort(by_len[m].begin(), by_len[m].end(), [&](unsigned p, unsigned q) { return length[p] > length[q]; });
        }
        std::vector<std::uint16_t> out;
        out.reserve(seq.size());
        for (std::size_t i = 0; i < seq.size();) {
                if (!open(seq[i])) { out.push_back(seq[i++]); continue; }
                std::size_t j = i;
                unsigned m = 4;
                for (; j < seq.size() && open(seq[j]); ++j) m = std::min(m, lim(seq[j]));
                std::size_t n = j - i;
                B.open_left += n;
                auto const& list = by_len[m];
                auto it = std::partition_point(list.begin(), list.end(), [&](unsigned s) { return length[s] > n; });
                for (; n != 0; ++it) while (length[*it] <= n) { out.push_back(std::uint16_t(*it)); n -= length[*it]; }
                i = j;
        }
        seq = std::move(out);
        B.val.assign(B.left.size(), 0xff);
        for (unsigned t = 0; t < T; ++t) B.val[t] = std::uint8_t(t < 5 ? t : 0);
        return B;
}

}       // namespace sl
