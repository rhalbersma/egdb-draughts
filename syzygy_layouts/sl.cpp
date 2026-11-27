//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// The layout study's tool. Syzygy WDL tables (rtbgen's output, read through Fathom) are re-stored in the layouts of
// layout.hpp, checked against Fathom's values, and timed.
//
//   sl info    <tbdir>                          a: the sub-tables and their sizes (CSV)
//   sl convert <tbdir> <outdir> <variant>       b, c0, c1, c2: his grammar; d0, d1, d2: a grammar rebuilt for
//                                               fixed-width codes, on header 0 (u16), 1 (u32), 2 (Elias-Fano)
//   sl check   <tbdir> <outdir> <variant> [random lookups per sub-table = 10000000]
//   sl trace   <tbdir> <positions> <trace>      the (sub-table, index) of every probe Fathom makes for the positions
//   sl bench   <tbdir> <outdir> <variant|a> <seq|uniform|trace:FILE|pos:FILE> <threads> <probes> [cold]

#include "tables.hpp"
#include "fathom_ext.h"
#include "grammar.hpp"
#include "../src/repair_vf.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <zstd.h>

extern "C" unsigned tb_probe_wdl_impl(std::uint64_t white, std::uint64_t black, std::uint64_t kings, std::uint64_t queens, std::uint64_t rooks,
                                      std::uint64_t bishops, std::uint64_t knights, std::uint64_t pawns, unsigned ep, bool turn);

namespace {

using namespace sl;
using Clock = std::chrono::steady_clock;
double seconds_since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

int g_threads = int(std::thread::hardware_concurrency());

template<class F>
void parallel(int n, F&& f)
{
        std::atomic<int> next{ 0 };
        std::vector<std::thread> ts;
        for (int t = 0; t < g_threads; ++t) ts.emplace_back([&] { for (int i; (i = next.fetch_add(1)) < n;) f(i); });
        for (auto& t : ts) t.join();
}

Source source_of(int i, std::uint64_t* substituted = nullptr, bool with_seq = true)
{
        SlSub const& s = *sl_sub(i);
        Source S;
        S.name = s.name; S.k = s.k; S.tb_size = s.tb_size; S.is_const = s.is_const; S.const_value = s.const_value;
        if (S.is_const) return S;
        S.block_bytes = 1u << s.block_bits;
        S.g = grammar_from_sympat(s.sympat, s.nsym);
        S.h.min_len = s.min_len; S.h.max_len = s.max_len;
        for (int l = s.min_len; l <= s.max_len; ++l) { S.h.offset[l] = s.offset[l]; S.h.base[l] = s.base[l]; }
        if (with_seq) {
                S.seq.resize(sl_extract(i, nullptr, nullptr));
                sl_extract(i, S.seq.data(), substituted);
        }
        return S;
}

std::vector<std::uint8_t> values_of(Source const& S)
{
        std::vector<std::uint8_t> v;
        if (S.is_const) return v;
        std::vector<unsigned> stack;
        for (std::uint16_t s0 : S.seq) {
                stack.push_back(s0);
                while (!stack.empty()) {
                        unsigned const s = stack.back();
                        stack.pop_back();
                        if (S.g.len[s] == 1) { v.push_back(S.g.val[s]); continue; }
                        stack.push_back(S.g.right[s]); stack.push_back(S.g.left[s]);
                }
        }
        return v;
}

// d: a grammar for fixed-width codes on the sub-table's values: symbols of up to `maxlen` positions, a rule only
// while it saves more than its 3 bytes; at most `max_symbols`.
// The values are never all in memory: they are produced in chunks from his symbols. The grammar is built on a sample
// (every step-th chunk, at most about 2^29 positions; all of a smaller sub-table) and then applied to every chunk in the
// order its rules were made, which is what the builder itself does. A pair across two chunks stays unpaired.
Source rebuilt(Source const& S, unsigned max_symbols, unsigned code_bits, std::uint32_t maxlen)
{
        Source D = S;
        if (S.is_const) return D;
        constexpr std::size_t CHUNK = std::size_t(1) << 22, SAMPLE = std::size_t(1) << 29;
        std::array<int, 256> rank; rank.fill(-1);
        std::array<std::uint16_t, 256> value{};
        unsigned terminals = 0;
        { std::array<bool, 256> seen{}; for (unsigned y = 0; y < S.g.nsym; ++y) if (S.g.len[y] == 1) seen[S.g.val[y]] = true;
          for (unsigned v = 0; v < 256; ++v) if (seen[v]) { rank[v] = int(terminals); value[terminals++] = std::uint16_t(v); } }
        // the values as terminals, chunk by chunk
        auto const each_chunk = [&](auto&& f) {
                std::vector<std::uint16_t> buf;
                std::vector<unsigned> stack;
                std::uint64_t done = 0;
                std::size_t c = 0;
                auto const flush = [&] { if (!buf.empty()) { f(c++, buf); buf.clear(); } };
                for (std::uint16_t s0 : S.seq) {
                        stack.push_back(s0);
                        while (!stack.empty()) {
                                unsigned const y = stack.back();
                                stack.pop_back();
                                if (S.g.len[y] != 1) { stack.push_back(S.g.right[y]); stack.push_back(S.g.left[y]); continue; }
                                if (done == S.tb_size) continue;                        // his padding after the last position
                                buf.push_back(std::uint16_t(rank[S.g.val[y]]));
                                ++done;
                                if (buf.size() == CHUNK) flush();
                        }
                }
                flush();
        };
        std::size_t const step = std::max<std::size_t>(1, (S.tb_size + SAMPLE - 1) / SAMPLE);
        std::vector<std::uint16_t> sample;
        sample.reserve(std::min<std::size_t>(S.tb_size, SAMPLE + CHUNK));
        each_chunk([&](std::size_t c, std::vector<std::uint16_t> const& buf) { if (c % step == 0) sample.insert(sample.end(), buf.begin(), buf.end()); });
        // a rule costs 3 bytes; replacing one pair saves code_bits / 8 bytes
        std::uint32_t const min_count = std::uint32_t(3 * 8 / code_bits + 1);
        std::size_t const sample_positions = sample.size();
        std::vector<std::uint8_t> const file = rpvf::compress_wide(std::move(sample), sample_positions, terminals, value, maxlen, min_count, max_symbols);
        rpvf::Wide const w = rpvf::open_wide(file.data());
        std::vector<std::uint16_t> left(w.nsym), right(w.nsym);
        std::vector<std::uint8_t> val(w.nsym, 0xff);
        for (unsigned s = 0; s < w.nsym; ++s) { left[s] = w.left[s]; right[s] = w.right[s]; if (w.length[s] == 1) val[s] = std::uint8_t(w.value[w.uniform[s]]); }
        std::vector<std::uint8_t> const pat = sympat_of(left, right, val);
        D.g = grammar_from_sympat(pat.data(), w.nsym);
        // the rules in batches that can be applied in one pass: no symbol in two pairs, none made in the same batch
        std::vector<std::pair<unsigned, unsigned>> batches;             // [first, last) rule
        {
                std::vector<bool> used(w.nsym, false);
                unsigned first = terminals;
                for (unsigned s = terminals; s <= w.nsym; ++s) {
                        if (s == w.nsym || left[s] >= first || right[s] >= first || used[left[s]] || used[right[s]]) {
                                if (s > first) batches.push_back({ first, s });
                                for (unsigned y = first; y < s; ++y) used[left[y]] = used[right[y]] = false;
                                first = s;
                                if (s == w.nsym) break;
                        }
                        used[left[s]] = used[right[s]] = true;
                }
        }
        constexpr std::size_t W = 4096;
        std::vector<std::uint16_t> repl(W * W, 0);
        std::vector<bool> is_left(W, false);
        D.seq.clear();
        each_chunk([&](std::size_t, std::vector<std::uint16_t> const& buf) {
                std::vector<std::uint16_t> seq = buf;
                for (auto const& [first, last] : batches) {
                        for (unsigned y = first; y < last; ++y) { repl[left[y] * W + right[y]] = std::uint16_t(y); is_left[left[y]] = true; }
                        std::size_t o = 0;
                        for (std::size_t i = 0; i < seq.size(); ++i) {
                                if (i + 1 < seq.size() && is_left[seq[i]]) {
                                        std::uint16_t const r = repl[std::size_t(seq[i]) * W + seq[i + 1]];
                                        if (r != 0) { seq[o++] = r; ++i; continue; }
                                }
                                seq[o++] = seq[i];
                        }
                        seq.resize(o);
                        for (unsigned y = first; y < last; ++y) { repl[left[y] * W + right[y]] = 0; is_left[left[y]] = false; }
                }
                D.seq.insert(D.seq.end(), seq.begin(), seq.end());
        });
        return D;
}

struct Variant { std::string name; char kind; int header; };    // kind: b, c, d, e
Variant parse_variant(std::string const& v)
{
        if (v == "b") return { v, 'b', 0 };
        if (v.size() == 2 && (v[0] == 'c' || v[0] == 'd' || v[0] == 'e') && v[1] >= '0' && v[1] <= '2') return { v, v[0], v[1] - '0' };
        std::cerr << "unknown variant " << v << std::endl;
        std::exit(2);
}

// e: the sub-table as the generator's compression step received it (scripts/patch_tb.py), from $SL_DC
std::vector<std::uint8_t> read_open(Source const& S)
{
        char const* const dir = std::getenv("SL_DC");
        if (!dir) { std::cerr << "SL_DC is not set" << std::endl; std::exit(1); }
        std::string const cmd = "gzip -dc '" + std::string(dir) + "/" + S.name + "." + std::to_string(S.k) + ".dc.gz'";
        std::vector<std::uint8_t> v(S.tb_size + 1);
        FILE* const f = ::popen(cmd.c_str(), "r");
        std::size_t const n = f ? std::fread(v.data(), 1, v.size(), f) : 0;
        if (!f || ::pclose(f) != 0 || n != S.tb_size) { std::cerr << S.id() << ": no table of " << S.tb_size << " values in $SL_DC (" << n << " read)" << std::endl; std::exit(1); }
        v.resize(S.tb_size);
        return v;
}

// e: as d, but the grammar is built with the don't-care positions open
Source rebuilt_open(Source const& S, std::vector<std::uint8_t> const& filled, unsigned max_symbols, unsigned code_bits, std::uint32_t maxlen, std::uint64_t* open3 = nullptr)
{
        Source D = S;
        // SL_JOKER: every don't-care may become part of any symbol (give the table unfilled: SL_NO_PREFILL)
        Built B = std::getenv("SL_JOKER") ? build_joker(filled, maxlen, std::uint32_t(3 * 8 / code_bits + 1), max_symbols)
                                          : build_open(filled, maxlen, std::uint32_t(3 * 8 / code_bits + 1), max_symbols);
        std::vector<std::uint8_t> const pat = sympat_of(B.left, B.right, B.val);
        D.g = grammar_from_sympat(pat.data(), unsigned(B.left.size()));
        D.seq = std::move(B.seq);
        if (open3) { open3[0] = B.open_before; open3[1] = B.open_paired; open3[2] = B.open_left; }
        return D;
}

std::atomic<std::uint64_t> g_open_all{ 0 }, g_open_after_prefill{ 0 }, g_open_paired{ 0 }, g_open_default{ 0 };

Stats convert_one(Source const& S, Variant const& V, std::string const& prefix)
{
        if (V.kind == 'b') return S.block_bytes == 64 ? convert<LB64>(S, prefix, V.name) : S.block_bytes == 32 ? convert<LB32>(S, prefix, V.name) : convert<LB16>(S, prefix, V.name);
        if (V.kind == 'c') return V.header == 0 ? convert<LC0>(S, prefix, V.name) : V.header == 1 ? convert<LC1>(S, prefix, V.name) : convert<LC2>(S, prefix, V.name);
        // d: 12-bit codes with up to 4,095 symbols, or 10-bit codes with up to 1,024; the smaller total
        if (S.is_const) return convert<LC0>(S, prefix, V.name);
        // u16 line counts: a symbol of at most 65,536 / 42 positions, so that a full line of codes always fits
        // SL_MAXLEN overrides (with u16 counts a line then closes early once it would pass 65,536 positions)
        std::uint32_t const maxlen = std::getenv("SL_MAXLEN") ? std::uint32_t(std::atoi(std::getenv("SL_MAXLEN"))) : V.header == 0 ? 1560 : 65536;
        Source D12, D10;
        if (V.kind == 'e') {
                std::vector<std::uint8_t> v = read_open(S);
                std::uint64_t all = 0, open3[3];
                for (std::uint8_t x : v) all += x >= 5;
                if (!std::getenv("SL_NO_PREFILL")) prefill(v);        // SL_NO_PREFILL: every don't-care stays open for the pairing
                D12 = rebuilt_open(S, v, 4095, 12, maxlen, open3); D10 = rebuilt_open(S, v, 1024, 10, maxlen);
                g_open_all += all; g_open_after_prefill += open3[0]; g_open_paired += open3[1]; g_open_default += open3[2];
        } else { D12 = rebuilt(S, 4095, 12, maxlen); D10 = rebuilt(S, 1024, 10, maxlen); }
        auto const run = [&](Source const& D, bool ten, std::string const& p) {
                return V.header == 0 ? (ten ? convert<LC0_10>(D, p, V.name) : convert<LC0>(D, p, V.name))
                     : V.header == 1 ? (ten ? convert<LC1_10>(D, p, V.name) : convert<LC1>(D, p, V.name))
                                     : (ten ? convert<LC2_10>(D, p, V.name) : convert<LC2>(D, p, V.name));
        };
        Stats const a = run(D12, false, prefix + ".w12"), b = run(D10, true, prefix + ".w10");
        bool const ten = b.total() < a.total();
        for (char const* ext : { ".dat", ".pri", ".top", ".sub", ".gra", ".man" }) {
                std::filesystem::rename(prefix + (ten ? ".w10" : ".w12") + ext, prefix + ext);
                std::filesystem::remove(prefix + (ten ? ".w12" : ".w10") + ext);
        }
        return ten ? b : a;
}

int open_tables(char const* tbdir)
{
        if (std::getenv("SL_DTZ")) sl_set_dtz(1);              // the DTZ tables of the directory (info, zstd, bench a with index traces)
        int const n = sl_open_all(tbdir);
        if (n <= 0) { std::cerr << "no tables in " << tbdir << std::endl; std::exit(1); }
        return n;
}

std::string prefix_of(std::string const& outdir, std::string const& variant, int i)
{
        SlSub const& s = *sl_sub(i);
        return outdir + "/" + variant + "/" + s.name + "." + std::to_string(s.k);
}

int pieces_of(char const* name) { int n = 0; for (char const* c = name; *c; ++c) n += *c != 'v'; return n; }

// ---- info: variant a

int info(char const* tbdir)
{
        int const n = open_tables(tbdir);
        std::cout << "variant,table,subtable,pieces,tb_size,const,line_bytes,code_bits,symbols,codes,coded_bytes,padding_bytes,index_bytes,grammar_bytes,total_bytes,pages,header3,header4,split_symbols\n";
        for (int i = 0; i < n; ++i) {
                SlSub const& s = *sl_sub(i);
                std::uint64_t data, index, sizes, grammar, subst = 0;
                sl_sizes(i, &data, &index, &sizes, &grammar);
                std::uint64_t bits = 0, codes = 0;
                if (!s.is_const) {
                        Source const S = source_of(i, &subst);
                        codes = S.seq.size();
                        for (std::uint16_t c : S.seq) bits += std::uint64_t(S.h.length(c));     // the extracted symbols at his code lengths: close to, not exactly, his bits
                }
                std::uint64_t const coded = std::min<std::uint64_t>(data, (bits + 7) / 8);
                std::cout << "a," << s.name << ',' << s.k << ',' << pieces_of(s.name) << ',' << s.tb_size << ',' << s.is_const << ',' << (s.is_const ? 0 : 1 << s.block_bits) << ",0," << s.nsym << ','
                          << codes << ',' << coded << ',' << data - coded << ',' << index + sizes << ',' << grammar << ',' << data + index + sizes + grammar << ",0,0,0," << subst << "\n";
        }
        return 0;
}

// ---- convert

int convert_all(char const* tbdir, std::string const& outdir, std::string const& vname)
{
        Variant const V = parse_variant(vname);
        int const n = open_tables(tbdir);
        std::filesystem::create_directories(outdir + "/" + vname);
        std::vector<std::string> rows(static_cast<std::size_t>(n));
        auto const t0 = Clock::now();
        // the largest first, so that the threads end together
        std::vector<int> order(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) order[std::size_t(i)] = i;
        std::sort(order.begin(), order.end(), [](int a, int b) { return sl_sub(a)->tb_size > sl_sub(b)->tb_size; });
        parallel(n, [&](int j) {
                int const i = order[std::size_t(j)];
                SlSub const& s = *sl_sub(i);
                Source const S = source_of(i);
                Stats const st = convert_one(S, V, prefix_of(outdir, vname, i));
                Manifest const man = read_manifest(prefix_of(outdir, vname, i) + ".man");
                auto const get = [&](char const* k) { auto it = man.find(k); return it == man.end() ? std::string("0") : it->second; };
                std::ostringstream o;
                o << vname << ',' << s.name << ',' << s.k << ',' << pieces_of(s.name) << ',' << s.tb_size << ',' << s.is_const << ',' << (s.is_const ? "0" : get("line_bytes")) << ','
                  << (s.is_const ? "0" : get("code_bits")) << ',' << get("symbols") << ',' << st.symbols << ',' << (st.code_bits + 7) / 8 << ',' << (s.is_const ? 0 : st.padding()) << ','
                  << st.index() << ',' << st.grammar << ',' << st.total() << ',' << st.pages << ',' << st.hdr3 << ',' << st.hdr4 << ',' << st.split_symbols << "\n";
                rows[std::size_t(i)] = o.str();
        });
        std::ofstream csv(outdir + "/" + vname + "/sizes.csv");
        csv << "variant,table,subtable,pieces,tb_size,const,line_bytes,code_bits,symbols,codes,coded_bytes,padding_bytes,index_bytes,grammar_bytes,total_bytes,pages,header3,header4,split_symbols\n";
        for (auto const& r : rows) csv << r;
        std::cerr << "convert " << vname << ": " << n << " sub-tables, " << seconds_since(t0) << " s" << std::endl;
        if (V.kind == 'e')
                std::cout << "don't-care positions: " << g_open_all.load() << "; still open after the neighbour fill: " << g_open_after_prefill.load() << "; of those settled by a pair (12-bit build): "
                          << g_open_paired.load() << ", by default: " << g_open_default.load() << std::endl;
        return 0;
}

// ---- check: the correctness gate

int check_all(char const* tbdir, std::string const& outdir, std::string const& vname, std::uint64_t random_per_sub)
{
        int const n = open_tables(tbdir);
        std::atomic<std::uint64_t> seq_checked{ 0 }, rnd_checked{ 0 }, exh_checked{ 0 }, bad{ 0 }, ref_checked{ 0 }, dc_differ{ 0 };
        std::mutex mx;
        auto const t0 = Clock::now();
        std::vector<int> order(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) order[std::size_t(i)] = i;
        std::sort(order.begin(), order.end(), [](int a, int b) { return sl_sub(a)->tb_size > sl_sub(b)->tb_size; });
        parallel(n, [&](int j) {
                int const i = order[std::size_t(j)];
                SlSub const& s = *sl_sub(i);
                auto const report = [&](std::string const& m) { std::lock_guard const lk(mx); if (bad.fetch_add(1) < 10) std::cerr << "FAIL " << s.name << "." << s.k << ": " << m << std::endl; };
                Table T;
                std::string why;
                if (!open_table(prefix_of(outdir, vname, i), T, &why)) { report(why); return; }
                std::mt19937_64 rng(std::uint64_t(i) * 7919 + 1);
                if (s.is_const) {
                        if (!T.is_const || T.const_value != s.const_value) report("constant differs");
                        for (int k = 0; k < 1000; ++k) { std::uint64_t const x = rng() % s.tb_size; if (T.fn(T, x) != sl_ref(i, x)) { report("constant lookup"); break; } }
                        return;
                }
                Source const S = source_of(i);
                if (vname[0] == 'e') {
                        // e may differ from Fathom where the value does not matter. The generator's table says where: his
                        // values must fit it everywhere (which also shows the two are aligned), ours must fit it everywhere,
                        // and the lookups must agree with the sequential decode
                        std::vector<std::uint8_t> const open = read_open(S), his = values_of(S);
                        std::vector<std::uint8_t> got;
                        if (!decode_all(T, got, &why)) { report(why); return; }
                        if (got.size() < s.tb_size || his.size() < s.tb_size) { report("too few values"); return; }
                        std::uint64_t differ = 0;
                        for (std::uint64_t x = 0; x < s.tb_size; ++x) {
                                if (!dc_allows(his[x], open[x])) { report("his value does not fit the generator's table at " + std::to_string(x)); return; }
                                if (!dc_allows(got[x], open[x])) { report("value not allowed at " + std::to_string(x)); return; }
                                differ += got[x] != his[x];
                        }
                        for (int k = 0; k < 100000; ++k) { std::uint64_t const x = rng() % s.tb_size; if (his[x] != sl_ref(i, x)) { report("the expanded symbols differ from Fathom's lookup"); return; } }
                        ref_checked += 100000; seq_checked += s.tb_size; dc_differ += differ;
                        bool const exhaustive = pieces_of(s.name) <= 4 || s.tb_size <= random_per_sub;
                        for (std::uint64_t k = 0, m = exhaustive ? s.tb_size : random_per_sub; k < m; ++k) {
                                std::uint64_t const x = exhaustive ? k : rng() % s.tb_size;
                                if (T.fn(T, x) != got[x]) { report("lookup differs at " + std::to_string(x)); return; }
                        }
                        (exhaustive ? exh_checked : rnd_checked) += exhaustive ? s.tb_size : random_per_sub;
                        return;
                }
                if (pieces_of(s.name) > 4 && s.tb_size > random_per_sub) {
                        // large: his symbols are expanded alongside the pages (no table of all values in memory), the first
                        // value of every page is also asked of Fathom, and the lookups are compared with Fathom's directly
                        std::vector<std::uint8_t> rb;
                        std::vector<unsigned> stack;
                        std::size_t si = 0;
                        std::uint64_t asked = 0;
                        auto const sink = [&](std::uint8_t const* v, std::size_t m, std::size_t first) {
                                while (rb.size() < m && si < S.seq.size()) {
                                        stack.push_back(S.seq[si++]);
                                        while (!stack.empty()) {
                                                unsigned const y = stack.back();
                                                stack.pop_back();
                                                if (S.g.len[y] == 1) { rb.push_back(S.g.val[y]); continue; }
                                                stack.push_back(S.g.right[y]); stack.push_back(S.g.left[y]);
                                        }
                                }
                                std::size_t const c = std::min<std::size_t>(m, first < s.tb_size ? s.tb_size - first : 0);
                                if (rb.size() < c || std::memcmp(v, rb.data(), c) != 0) return false;
                                if (c != 0) { ++asked; if (v[0] != sl_ref(i, first)) return false; }
                                rb.erase(rb.begin(), rb.begin() + std::ptrdiff_t(std::min(m, rb.size())));
                                return true;
                        };
                        if (!decode_pages(T, sink, &why)) { report(why); return; }
                        seq_checked += s.tb_size; ref_checked += asked;
                        for (std::uint64_t k = 0; k < random_per_sub; ++k) { std::uint64_t const x = rng() % s.tb_size; if (T.fn(T, x) != sl_ref(i, x)) { report("lookup differs at " + std::to_string(x)); return; } }
                        rnd_checked += random_per_sub; ref_checked += random_per_sub;
                        return;
                }
                // small: Fathom's values are his symbols expanded, spot-checked against his own lookup
                std::vector<std::uint8_t> const ref = values_of(S);
                for (int k = 0; k < 100000; ++k) { std::uint64_t const x = rng() % s.tb_size; if (ref[x] != sl_ref(i, x)) { report("the expanded symbols differ from Fathom's lookup"); return; } }
                ref_checked += 100000;
                // 1. every position, read in order through the page headers
                std::vector<std::uint8_t> got;
                if (!decode_all(T, got, &why)) { report(why); return; }
                if (got.size() < s.tb_size || std::memcmp(got.data(), ref.data(), s.tb_size) != 0) { report("sequential decode differs"); return; }
                seq_checked += s.tb_size;
                // 2. the lookup: every index for tables of up to 4 pieces, else random indices
                bool const exhaustive = pieces_of(s.name) <= 4 || s.tb_size <= random_per_sub;
                if (exhaustive) {
                        for (std::uint64_t x = 0; x < s.tb_size; ++x) if (T.fn(T, x) != ref[x]) { report("lookup differs at " + std::to_string(x)); return; }
                        exh_checked += s.tb_size;
                } else {
                        for (std::uint64_t k = 0; k < random_per_sub; ++k) { std::uint64_t const x = rng() % s.tb_size; if (T.fn(T, x) != ref[x]) { report("lookup differs at " + std::to_string(x)); return; } }
                        rnd_checked += random_per_sub;
                }
        });
        std::cout << "check " << vname << ": " << n << " sub-tables; " << seq_checked.load() << " positions decoded in order, " << exh_checked.load() << " looked up exhaustively, "
                  << rnd_checked.load() << " at random; " << ref_checked.load() << " reference values confirmed by Fathom's lookup; " << bad.load() << " failures; " << seconds_since(t0) << " s" << std::endl;
        if (vname[0] == 'e') std::cout << "check " << vname << ": " << dc_differ.load() << " don't-care positions hold another value than his" << std::endl;
        return bad.load() == 0 ? 0 : 1;
}

// ---- zstd on the values, for comparison: blocks of `block` positions of his stored values in his order, one byte
// each (two where a sub-table has values above 255, as DTZ tables can). SL_DTZ=1: the DTZ tables of the directory.
// The values are expanded from his symbols block by block; no sub-table is held in memory.

int zstd_sizes(char const* tbdir, std::size_t block, int level)
{
        int const n = open_tables(tbdir);
        struct Row { std::uint64_t positions = 0, his = 0, z = 0, blocks = 0, ns = 0, timed = 0; int wide = 0, subs = 0; };
        std::map<std::string, Row> rows;
        std::mutex mx;
        parallel(n, [&](int i) {
                SlSub const& s = *sl_sub(i);
                if (s.is_const) return;
                Source const S = source_of(i);
                std::vector<std::uint16_t> value(S.g.nsym, 0);
                bool wide = false;
                for (unsigned y = 0; y < S.g.nsym; ++y)
                        if (S.g.len[y] == 1) { value[y] = std::uint16_t(s.sympat[3 * y] | (s.sympat[3 * y + 1] & 0x0f) << 8); wide |= value[y] > 255; }
                std::size_t const w = wide ? 2 : 1;
                std::vector<char> buf, z(ZSTD_compressBound(block * w)), back(block * w);
                buf.reserve(block * w);
                ZSTD_DCtx* const dx = ZSTD_createDCtx();
                Row r;
                r.wide = wide; r.subs = 1; r.positions = s.tb_size;
                std::uint64_t d, ix, sz, gr;
                sl_sizes(i, &d, &ix, &sz, &gr);
                r.his = d + ix + sz + gr;
                auto const flush = [&] {
                        if (buf.empty()) return;
                        std::size_t const c = ZSTD_compress(z.data(), z.size(), buf.data(), buf.size(), level);
                        r.z += c + 8;                           // and an 8-byte offset
                        if (r.blocks++ % 64 == 0) {             // decompression of one block, timed on every 64th
                                auto const t0 = Clock::now();
                                ZSTD_decompressDCtx(dx, back.data(), back.size(), z.data(), c);
                                r.ns += std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count());
                                ++r.timed;
                        }
                        buf.clear();
                };
                std::vector<unsigned> stack;
                std::uint64_t done = 0;
                for (std::uint16_t s0 : S.seq) {
                        stack.push_back(s0);
                        while (!stack.empty()) {
                                unsigned const y = stack.back();
                                stack.pop_back();
                                if (S.g.len[y] != 1) { stack.push_back(S.g.right[y]); stack.push_back(S.g.left[y]); continue; }
                                if (done == s.tb_size) continue;
                                buf.push_back(char(value[y]));
                                if (wide) buf.push_back(char(value[y] >> 8));
                                if (++done % block == 0) flush();
                        }
                }
                flush();
                ZSTD_freeDCtx(dx);
                std::lock_guard const lk(mx);
                Row& t = rows[s.name];
                t.positions += r.positions; t.his += r.his; t.z += r.z; t.blocks += r.blocks; t.ns += r.ns; t.timed += r.timed; t.wide += r.wide; t.subs += r.subs;
        });
        std::cout << "table,sub_tables,with_2_byte_values,positions,his_bytes,zstd_bytes,zstd_against_his,blocks,block_decompress_ns  (zstd level " << level << ", blocks of " << block << " positions)\n";
        Row all;
        auto const line = [&](std::string const& name, Row const& t) {
                std::cout << name << ',' << t.subs << ',' << t.wide << ',' << t.positions << ',' << t.his << ',' << t.z << ',' << double(t.z) / double(std::max<std::uint64_t>(1, t.his)) << ','
                          << t.blocks << ',' << double(t.ns) / double(std::max<std::uint64_t>(1, t.timed)) << "\n";
        };
        for (auto const& [name, t] : rows) {
                if (rows.size() <= 12) line(name, t);
                all.positions += t.positions; all.his += t.his; all.z += t.z; all.blocks += t.blocks; all.ns += t.ns; all.timed += t.timed; all.wide += t.wide; all.subs += t.subs;
        }
        line("all", all);
        return 0;
}

// ---- positions, traces and timing

struct Rec { std::uint64_t w, b, k, q, r, bi, n, p; std::uint8_t turn, cap; std::uint8_t pad[6]; };
struct Probe { std::uint64_t idx; std::uint32_t sub, pad; };

template<class T>
std::vector<T> read_all(std::string const& path)
{
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) { std::cerr << "cannot read " << path << std::endl; std::exit(1); }
        std::vector<T> v(std::size_t(f.tellg()) / sizeof(T));
        f.seekg(0);
        f.read(reinterpret_cast<char*>(v.data()), std::streamsize(v.size() * sizeof(T)));
        return v;
}

int trace(char const* tbdir, std::string const& positions, std::string const& out)
{
        open_tables(tbdir);
        // Fathom resolves captures by search, so a position with a capture into a table that is not present cannot be
        // probed: such positions are dropped from the file, for every variant alike
        std::vector<Rec> recs = read_all<Rec>(positions);
        std::size_t const before = recs.size();
        std::erase_if(recs, [](Rec const& x) { return tb_probe_wdl_impl(x.w, x.b, x.k, x.q, x.r, x.bi, x.n, x.p, 0, x.turn) == 0xffffffffu; });
        if (recs.size() != before) { write_file(positions, recs.data(), recs.size() * sizeof(Rec)); std::cout << "trace: kept " << recs.size() << " of " << before << " positions" << std::endl; }
        std::vector<std::uint32_t> sub(recs.size() * 64);
        std::vector<std::uint64_t> idx(recs.size() * 64);
        sl_set_trace(sub.data(), idx.data(), sub.size());
        std::uint64_t failed = 0;
        for (Rec const& x : recs) failed += tb_probe_wdl_impl(x.w, x.b, x.k, x.q, x.r, x.bi, x.n, x.p, 0, x.turn) == 0xffffffffu;
        std::size_t const n = sl_trace_count();
        std::vector<Probe> t(n);
        for (std::size_t i = 0; i < n; ++i) t[i] = { idx[i], sub[i], 0 };
        write_file(out, t.data(), n * sizeof(Probe));
        std::cout << "trace: " << recs.size() << " positions, " << failed << " not probed, " << n << " table lookups (" << double(n) / double(recs.size()) << " per position)" << std::endl;
        return 0;
}

int hook(void* ext, std::uint64_t idx) { Table const& T = *static_cast<Table const*>(ext); return T.fn(T, idx); }

int bench(char const* tbdir, std::string const& outdir, std::string const& vname, std::string const& mode, int threads, std::uint64_t probes, bool cold)
{
        int const n = open_tables(tbdir);
        bool const ref = vname == "a";
        std::vector<Table> tables(ref ? 0 : std::size_t(n));
        if (!ref)
                for (int i = 0; i < n; ++i) {
                        std::string why;
                        if (!open_table(prefix_of(outdir, vname, i), tables[std::size_t(i)], &why, !cold)) { std::cerr << why << std::endl; return 1; }
                }
        auto const lookup = [&](std::uint32_t s, std::uint64_t x) { return ref ? sl_ref(int(s), x) : tables[s].fn(tables[s], x); };
        // the probes
        std::vector<Probe> ps;
        std::vector<Rec> recs;
        bool const positions = mode.rfind("pos:", 0) == 0;
        if (positions) {
                recs = read_all<Rec>(mode.substr(4));
                if (!ref) { for (int i = 0; i < n; ++i) sl_set_ext(i, &tables[std::size_t(i)]); sl_set_hook(&hook); }
        } else if (mode.rfind("trace:", 0) == 0) {
                ps = read_all<Probe>(mode.substr(6));
        } else {
                std::vector<std::uint64_t> cum;
                std::uint64_t tot = 0;
                for (int i = 0; i < n; ++i) { tot += sl_sub(i)->is_const ? 0 : sl_sub(i)->tb_size; cum.push_back(tot); }
                std::mt19937_64 rng(11);
                ps.resize(probes * std::uint64_t(threads));
                if (mode == "uniform")
                        for (auto& p : ps) { std::uint64_t const a = rng() % tot; p.sub = std::uint32_t(std::upper_bound(cum.begin(), cum.end(), a) - cum.begin()); p.idx = rng() % sl_sub(int(p.sub))->tb_size; p.pad = 0; }
                else                                            // seq: runs of 4,096 consecutive indices from random starts
                        for (std::size_t k = 0; k < ps.size(); k += 4096) {
                                std::uint64_t const a = rng() % tot;
                                std::uint32_t const s = std::uint32_t(std::upper_bound(cum.begin(), cum.end(), a) - cum.begin());
                                std::uint64_t const size = sl_sub(int(s))->tb_size, start = rng() % size;
                                for (std::size_t j = 0; j < 4096 && k + j < ps.size(); ++j) ps[k + j] = { (start + j) % size, s, 0 };
                        }
        }
        std::size_t const total = positions ? recs.size() : ps.size();
        std::size_t const per = std::min<std::size_t>(total / std::size_t(threads), probes);
        if (per == 0) { std::cerr << "no probes" << std::endl; return 1; }
        auto const one = [&](std::size_t k) -> std::uint64_t {
                if (positions) { Rec const& x = recs[k]; return tb_probe_wdl_impl(x.w, x.b, x.k, x.q, x.r, x.bi, x.n, x.p, 0, x.turn); }
                return std::uint64_t(lookup(ps[k].sub, ps[k].idx));
        };
        // throughput: untimed probes in each thread; latency: each probe timed, in a second pass (cold: the first pass, timed)
        std::vector<std::uint64_t> sums(static_cast<std::size_t>(threads), 0);
        std::vector<std::vector<std::uint32_t>> lat(static_cast<std::size_t>(threads));
        double wall = 0;
        auto const pass = [&](bool timed) {
                std::vector<std::thread> ts;
                auto const t0 = Clock::now();
                for (int t = 0; t < threads; ++t)
                        ts.emplace_back([&, t] {
                                std::size_t const lo = std::size_t(t) * per;
                                std::uint64_t acc = 0;
                                if (timed) {
                                        auto& v = lat[std::size_t(t)];
                                        v.resize(per);
                                        for (std::size_t k = 0; k < per; ++k) {
                                                auto const a = Clock::now();
                                                acc += one(lo + k);
                                                v[k] = std::uint32_t(std::min<std::int64_t>(0xffffffff, std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - a).count()));
                                        }
                                } else
                                        for (std::size_t k = 0; k < per; ++k) acc += one(lo + k);
                                sums[std::size_t(t)] = acc;
                        });
                for (auto& t : ts) t.join();
                wall = seconds_since(t0);
        };
        double mean_ns = 0, rate = 0;
        if (cold) {
                pass(true);
                rate = double(per) * threads / wall;
        } else {
                pass(false);                                    // warm-up
                pass(false);
                rate = double(per) * threads / wall;
                mean_ns = wall * 1e9 / double(per);
                pass(true);
        }
        std::vector<std::uint32_t> all;
        for (auto const& v : lat) all.insert(all.end(), v.begin(), v.end());
        std::sort(all.begin(), all.end());
        auto const pct = [&](double q) { return all[std::min(all.size() - 1, std::size_t(q * double(all.size())))]; };
        double timed_mean = 0;
        for (std::uint32_t x : all) timed_mean += x;
        timed_mean /= double(all.size());
        if (cold) mean_ns = timed_mean;
        std::uint64_t sum = 0;
        for (std::uint64_t s : sums) sum += s;
        // variant,trace,kind,cache,threads,probes,probes_per_s,mean_ns,p50,p90,p99,checksum
        std::string const tname = positions ? "game" : mode.rfind("trace:", 0) == 0 ? "game" : mode;
        std::cout << vname << ',' << tname << ',' << (positions ? "position" : "index") << ',' << (cold ? "cold" : "warm") << ',' << threads << ',' << per * std::size_t(threads) << ','
                  << std::uint64_t(rate) << ',' << mean_ns << ',' << pct(0.5) << ',' << pct(0.9) << ',' << pct(0.99) << ',' << sum << std::endl;
        return 0;
}

}       // namespace

int main(int argc, char** argv)
{
        std::vector<std::string> a(argv + 1, argv + argc);
        if (char const* t = std::getenv("SL_THREADS")) g_threads = std::max(1, std::atoi(t));
        if (a.size() == 2 && a[0] == "info") return info(a[1].c_str());
        if (a.size() == 4 && a[0] == "zstd") return zstd_sizes(a[1].c_str(), std::stoull(a[2]), std::stoi(a[3]));
        if (a.size() == 4 && a[0] == "convert") return convert_all(a[1].c_str(), a[2], a[3]);
        if ((a.size() == 4 || a.size() == 5) && a[0] == "check") return check_all(a[1].c_str(), a[2], a[3], a.size() == 5 ? std::stoull(a[4]) : 10000000);
        if (a.size() == 4 && a[0] == "trace") return trace(a[1].c_str(), a[2], a[3]);
        if ((a.size() == 7 || a.size() == 8) && a[0] == "bench") return bench(a[1].c_str(), a[2], a[3], a[4], std::stoi(a[5]), std::stoull(a[6]), a.size() == 8 && a[7] == "cold");
        std::cerr << "usage: sl info <tbdir> | convert <tbdir> <outdir> <variant> | check <tbdir> <outdir> <variant> [random] | trace <tbdir> <positions> <trace> | "
                     "bench <tbdir> <outdir> <variant|a> <seq|uniform|trace:FILE|pos:FILE> <threads> <probes> [cold]" << std::endl;
        return 2;
}
