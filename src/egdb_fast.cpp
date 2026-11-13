//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Fast kings-only endgame databases (DTM) for draughts on general W x H chequer boards.
//
// Same rules, output formats and DTM convention as egdb.cpp, but built for speed:
// - bitboards: xstd::bit_align<xstd::bit_fixed_set<N>> (xstd-bits): the number of playable squares rounded up
//   to whole 64-bit blocks (64, 128, ..., 512), chosen at run time, and precomputed rays per square and direction
// - compact combinatorial indexing: index = colex_rank(white) * C(N - w, b) + colex_rank(black'),
//   where black' are the black squares renumbered among the squares white does not occupy,
//   so every array slot is a legal position (no gaps)
// - compact storage: one byte per position per side to move
// - retrograde analysis: capture positions are resolved from smaller databases, after which
//   only predecessors (unmoves) of newly decided positions are visited, using move counters
// - OpenMP for the initialisation and verification passes
//
// Values (per side to move, in plies): even = loss in that many plies (0 = no moves left),
// odd = win in that many plies, DRAW = draw.

#include <tabula/grids/chequer.hpp>             // chequer
#include <tabula/lake.hpp>                      // algebraic
#include <xstd/bits/bit_set/bit_fixed_set.hpp>  // bit_fixed_set
#include <xstd/bits/bit_type_traits/bit_align.hpp> // bit_align

#include <algorithm>
#include <array>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace fast {

// Rules (set once in main)
bool flying_kings     = true;   // international: flying kings; checkers: one square
bool majority_capture = true;   // international: must capture the most pieces; checkers: free choice

constexpr int max_kings   = 4;          // per side
constexpr int max_squares = 512;        // the widest bitboard instantiated below

constexpr std::uint8_t DRAW    = 254;
constexpr std::uint8_t UNKNOWN = 255;   // only during a build

using square = std::uint16_t;

// ---------------------------------------------------------------------------
// Binomials (colexicographic ranks)
// ---------------------------------------------------------------------------

std::array<std::array<std::uint64_t, max_kings + 1>, max_squares + 1> C{};

void init_binomial()
{
        for (auto n = 0; n <= max_squares; ++n) {
                C[static_cast<std::size_t>(n)][0] = 1;
                for (auto k = 1; k <= max_kings; ++k) {
                        C[static_cast<std::size_t>(n)][static_cast<std::size_t>(k)] = n == 0 ? 0 :
                                C[static_cast<std::size_t>(n - 1)][static_cast<std::size_t>(k - 1)] + C[static_cast<std::size_t>(n - 1)][static_cast<std::size_t>(k)];
                }
        }
}

inline std::uint64_t binom(std::size_t n, int k) { return C[n][static_cast<std::size_t>(k)]; }

// ---------------------------------------------------------------------------
// Geometry: playable squares numbered 0..n-1 in PDN order, rays per direction
// ---------------------------------------------------------------------------

struct Geometry
{
        tabula::chequer grid;
        std::vector<std::pair<int, int>> lake;
        int n = 0;
        std::vector<int> pdn;                   // dense -> PDN
        std::vector<std::string> name;          // dense -> algebraic
        std::vector<int> dense;                 // PDN -> dense, -1 if not playable
        std::array<std::vector<std::vector<square>>, 4> ray;   // NE, NW, SE, SW

        Geometry(int width, int height, int parity, std::vector<std::pair<int, int>> lake_squares)
        :
                grid{width, height, parity},
                lake(std::move(lake_squares))
        {
                auto const playable = [&](int f, int r) {
                        auto const sq = std::pair{f, r};
                        return grid.is_valid(sq) && std::ranges::find(lake, sq) == lake.end();
                };
                struct Sq { int pdn, file, rank; };
                std::vector<Sq> sqs;
                for (auto r = 0; r < height; ++r) {
                        for (auto f = 0; f < width; ++f) {
                                if (playable(f, r)) {
                                        sqs.push_back({ grid.flip().index(std::pair{f, height - 1 - r}) + 1, f, r });
                                }
                        }
                }
                std::ranges::sort(sqs, {}, &Sq::pdn);
                n = static_cast<int>(sqs.size());
                if (n > max_squares) { throw std::invalid_argument(std::format("board has {} squares, at most {} supported", n, max_squares)); }
                dense.assign(static_cast<std::size_t>(grid.size() + 1), -1);
                std::map<std::pair<int, int>, int> at;
                for (auto i = 0; i < n; ++i) {
                        auto const& q = sqs[static_cast<std::size_t>(i)];
                        pdn.push_back(q.pdn);
                        name.push_back(std::format("{}{}", static_cast<char>('a' + q.file), q.rank + 1));
                        dense[static_cast<std::size_t>(q.pdn)] = i;
                        at[{ q.file, q.rank }] = i;
                }
                auto k = 0;
                for (auto [df, dr] : { std::pair{1, 1}, {-1, 1}, {1, -1}, {-1, -1} }) {
                        auto& rk = ray[static_cast<std::size_t>(k++)];
                        rk.resize(static_cast<std::size_t>(n));
                        for (auto i = 0; i < n; ++i) {
                                auto f = sqs[static_cast<std::size_t>(i)].file + df;
                                auto r = sqs[static_cast<std::size_t>(i)].rank + dr;
                                for (; playable(f, r); f += df, r += dr) {
                                        rk[static_cast<std::size_t>(i)].push_back(static_cast<square>(at[{ f, r }]));
                                }
                        }
                }
        }

        [[nodiscard]] auto is_playable_pdn(int p) const
        {
                return 1 <= p && p <= grid.size() && dense[static_cast<std::size_t>(p)] >= 0;
        }

        template<class BB>
        [[nodiscard]] auto diagram(BB const& white, BB const& black) const
        {
                auto const border = std::format("    +{}+\n", std::string(static_cast<std::size_t>(2 * grid.width + 1), '-'));
                std::string s = border;
                for (auto rank = grid.height - 1; rank >= 0; --rank) {
                        s += std::format("{:3} | ", rank + 1);
                        for (auto file = 0; file < grid.width; ++file) {
                                auto const sq = std::pair{file, rank};
                                auto c = ' ';
                                if (grid.is_valid(sq) && std::ranges::find(lake, sq) == lake.end()) {
                                        auto const p = grid.flip().index(std::pair{file, grid.height - 1 - rank}) + 1;
                                        auto const d = static_cast<std::size_t>(dense[static_cast<std::size_t>(p)]);
                                        c = white.contains(d) ? 'W' : black.contains(d) ? 'B' : '.';
                                }
                                s += c;
                                s += ' ';
                        }
                        s += "|\n";
                }
                s += border;
                s += "      ";
                for (auto file = 0; file < grid.width; ++file) {
                        s += static_cast<char>('a' + file);
                        s += ' ';
                }
                s += '\n';
                return s;
        }
};

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

// value for the mover of playing into a child with value c (the parity flips)
inline std::uint8_t backup(std::uint8_t c) { return c >= DRAW ? DRAW : static_cast<std::uint8_t>(c + 1); }

// ordering for the mover: fastest win > draw > slowest loss
inline int preference(std::uint8_t v) { return v >= DRAW ? 0 : (v & 1) ? 1000 - v : -1000 + v; }

[[nodiscard]] auto describe(std::uint8_t v) -> std::string
{
        if (v >= DRAW) { return "draw"; }
        auto const n = static_cast<int>(v);
        return std::format("{} in {} {} ({} {})", (v & 1) ? "win" : "loss",
                n, n == 1 ? "ply" : "plies", (n + 1) / 2, (n + 1) / 2 == 1 ? "move" : "moves");
}

[[nodiscard]] auto wdl(std::uint8_t v) -> char { return v >= DRAW ? 'D' : (v & 1) ? 'W' : 'L'; }

struct Wdl
{
        std::size_t w{}, d{}, l{};

        void add(std::uint8_t v) { (v >= DRAW ? d : (v & 1) ? w : l)++; }
        [[nodiscard]] friend auto operator==(Wdl const&, Wdl const&) -> bool = default;
};

[[nodiscard]] auto to_string(Wdl const& x) { return std::format("W {:9}  D {:9}  L {:9}", x.w, x.d, x.l); }

struct Stats
{
        std::array<Wdl, 2> all, stm_quiet, both_quiet;
        std::array<std::array<std::size_t, 256>, 2> hist{};
};

inline int signed_dtm(int v) { return (v & 1) ? v : -v; }

void print_stats(Stats const& st, int w, int b)
{
        std::cout << std::format("\n=== statistics db {}-{} ({} white kings vs {} black kings) ===\n", w, b, w, b);
        for (auto s = 0; s < 2; ++s) {
                auto const i = static_cast<std::size_t>(s);
                std::cout << std::format("{} to move\n", s == 0 ? "white" : "black");
                std::cout << std::format("  all positions        : {}\n", to_string(st.all[i]));
                std::cout << std::format("  side to move quiet   : {}\n", to_string(st.stm_quiet[i]));
                std::cout << std::format("  both sides quiet     : {}\n", to_string(st.both_quiet[i]));
                std::vector<std::pair<int, std::size_t>> h;
                for (auto v = 0; v < DRAW; ++v) {
                        if (st.hist[i][static_cast<std::size_t>(v)]) { h.push_back({ signed_dtm(v), st.hist[i][static_cast<std::size_t>(v)] }); }
                }
                if (h.empty()) { continue; }
                std::ranges::sort(h);
                auto const lo = h.front().first;
                auto const hi = h.back().first;
                std::cout << std::format("  longest win {} plies, longest loss {} plies\n", hi > 0 ? hi : 0, lo <= 0 ? -lo : 0);
                std::cout << "  DTM histogram (plies: count):";
                auto n = 0;
                for (auto const& [d, c] : h) {
                        std::cout << std::format("{}{:+}: {}", n++ % 6 == 0 ? "\n    " : "   ", d, c);
                }
                std::cout << '\n';
        }
}

void append_dtm(std::string& s, std::uint8_t v)
{
        if (v >= DRAW) { s += 'D'; return; }
        s += (v & 1) ? '+' : '-';
        s += std::to_string(v);
}

// ---------------------------------------------------------------------------
// Reference data from https://mdgsoftware.nl/draughts/stats/ (10x10 international)
// ---------------------------------------------------------------------------

struct RefStats { int w, b, stm; Wdl all, quiet; char const* page; };

auto const ref_stats = std::array{
        RefStats{ 1, 1, 0, {    424,   2'024,      2 }, {     16,   2'024,      2 }, "0101" },
        RefStats{ 2, 1, 0, { 19'398,  39'402,      0 }, {  2'330,  39'402,      0 }, "0201" },
        RefStats{ 2, 1, 1, {  3'240,  54'988,    572 }, {      0,  40'920,    236 }, "0102" },
        RefStats{ 3, 1, 0, {486'444, 434'756,      0 }, {134'804, 434'756,      0 }, "0301" },
        RefStats{ 3, 1, 1, { 16'812, 863'802, 40'586 }, {      0, 528'990, 17'210 }, "0103" },
};

struct RefExample { char const* fen; char value; char const* moves; char const* page; };

constexpr auto ref_examples = std::array{
        RefExample{ "W:WK6:BK50", 'D', "6-1:D 6-11:L 6-17:L 6-22:L 6-28:L 6-33:L 6-39:L 6-44:L", "0101" },
        RefExample{ "W:WK46:BK5", 'L', "46-41:L 46-37:L 46-32:L 46-28:L 46-23:L 46-19:L 46-14:L 46-10:L", "0101" },
        RefExample{ "W:WK1,K12:BK50", 'W', "1-6:W 1-7:D 12-7:D 12-8:D 12-3:D 12-17:D 12-21:D 12-26:D 12-18:D 12-23:D 12-29:D 12-34:D 12-40:D 12-45:D", "0201" },
        RefExample{ "B:WK2,K6:BK50", 'D', "50-44:L 50-39:L 50-33:L 50-28:L 50-22:L 50-17:L 50-11:L 50-45:D", "0201" },
        RefExample{ "B:WK1,K3:BK50", 'D', "50-44:D 50-39:D 50-33:D 50-28:D 50-22:D 50-17:L 50-11:D 50-6:D 50-45:L", "0201" },
        RefExample{ "W:WK16,K20:BK49", 'W', "16-11:D 16-7:D 16-2:D 16-21:D 16-27:D 16-32:D 16-38:D 16-43:L 20-14:D 20-9:D 20-3:D 20-15:D 20-24:D 20-29:D 20-33:D 20-38:W 20-42:D 20-47:D 20-25:D", "0201" },
        RefExample{ "B:WK1:BK17,K50", 'W', "17-11:D 17-6:D 17-12:D 17-8:D 17-3:D 17-21:D 17-26:D 17-22:D 17-28:D 17-33:D 17-39:D 17-44:D 50-44:D 50-39:D 50-33:D 50-28:D 50-22:D 50-45:W", "0102" },
        RefExample{ "W:WK6:BK15,K50", 'D', "6-1:D 6-11:L 6-17:L 6-22:L 6-28:L 6-33:L 6-39:L 6-44:L", "0102" },
        RefExample{ "W:WK2:BK49,K50", 'D', "2-7:D 2-11:L 2-16:D 2-8:D 2-13:D 2-19:D 2-24:D 2-30:D 2-35:D", "0102" },
        RefExample{ "B:WK16:BK2,K50", 'W', "2-7:D 2-11:D 2-8:D 2-13:D 2-19:D 2-24:D 2-30:D 2-35:D 50-44:D 50-39:D 50-33:D 50-28:D 50-22:D 50-17:D 50-11:W 50-6:D 50-45:D", "0102" },
        RefExample{ "W:WK2,K10,K45:BK50", 'W', "2-7:D 2-11:D 2-16:D 2-8:D 2-13:D 2-19:D 2-24:D 2-30:D 2-35:D 10-4:D 10-5:D 10-14:D 10-19:D 10-23:W 10-28:D 10-32:D 10-37:D 10-41:D 10-46:D 10-15:D 45-40:D 45-34:D 45-29:D 45-23:D 45-18:D 45-12:D 45-7:D 45-1:D", "0301" },
        RefExample{ "B:WK2,K4,K6:BK50", 'D', "50-44:L 50-39:L 50-33:L 50-28:L 50-22:L 50-17:L 50-11:L 50-45:D", "0301" },
        RefExample{ "B:WK1,K36,K43:BK50", 'D', "50-44:D 50-39:L 50-33:D 50-28:D 50-22:L 50-17:D 50-11:D 50-6:D 50-45:L", "0301" },
        RefExample{ "W:WK3,K12,K14:BK50", 'W', "3-8:D 3-9:D 12-7:D 12-1:D 12-8:D 12-17:D 12-21:D 12-26:D 12-18:D 12-23:D 12-29:D 12-34:D 12-40:D 12-45:D 14-9:D 14-10:D 14-5:D 14-19:D 14-23:D 14-28:W 14-32:D 14-37:D 14-41:D 14-46:D 14-20:D 14-25:D", "0301" },
        RefExample{ "B:WK26:BK17,K21,K48", 'W', "17-11:D 17-6:D 17-12:D 17-8:D 17-3:W 17-22:D 17-28:D 17-33:D 17-39:D 17-44:D 17-50:D 21-16:D 21-27:D 21-32:D 21-38:D 21-43:D 21-49:D 48-42:D 48-37:D 48-31:D 48-43:D 48-39:D 48-34:D 48-30:D 48-25:D", "0103" },
        RefExample{ "W:WK6:BK5,K49,K50", 'D', "6-1:D 6-11:L 6-17:L 6-22:L 6-28:L 6-33:L 6-39:L 6-44:L", "0103" },
        RefExample{ "W:WK15:BK14,K49,K50", 'D', "15-10:L 15-4:D 15-20:L 15-24:D 15-29:D 15-33:L 15-38:L 15-42:D 15-47:D", "0103" },
        RefExample{ "B:WK38:BK35,K48,K50", 'W', "35-30:D 35-24:D 35-19:D 35-13:D 35-8:D 35-2:D 35-40:D 35-44:D 35-49:D 48-42:D 48-37:D 48-31:D 48-26:D 48-43:W 48-39:D 48-34:D 48-30:D 48-25:D 50-44:D 50-39:D 50-33:D 50-28:D 50-22:D 50-17:D 50-11:D 50-6:D 50-45:D", "0103" },
};

struct Options
{
        std::string board_name;
        int target_w = 3;
        int target_b = 1;
        std::optional<std::filesystem::path> out_dir;
        std::vector<std::string> queries;
        bool do_verify = true;
        bool do_text = true;
};

// ---------------------------------------------------------------------------
// The engine, parameterized on the bitboard type (a set of dense square numbers)
// ---------------------------------------------------------------------------

template<class BB>
class Engine
{
public:
        struct Move
        {
                square from;
                square to;
                BB captured;
        };

        struct MoveList
        {
                int n = 0;
                std::array<Move, 1024> m;
        };

        struct Pos
        {
                BB white;
                BB black;
                int stm;        // 0 = white, 1 = black

                [[nodiscard]] auto own() const -> BB const& { return stm == 0 ? white : black; }
                [[nodiscard]] auto opp() const -> BB const& { return stm == 0 ? black : white; }
        };

        struct Slice
        {
                int w = -1;
                int b = -1;
                std::uint64_t nb = 0;                           // black configurations per white one
                std::uint64_t size = 0;
                std::array<std::vector<std::uint8_t>, 2> val;   // per side to move
                std::vector<std::uint8_t> flags;                // bit 0: white can capture, bit 1: black can capture
        };

private:
        Geometry const& g;
        std::size_t N;                                          // playable squares
        std::array<std::vector<BB>, max_kings + 1> combos;      // combos[k][r]: the k-subset with colex rank r
        std::array<std::array<Slice, max_kings + 1>, max_kings + 1> slices;

        // ---- set helpers --------------------------------------------------

        static BB one(std::size_t s)
        {
                BB b;
                b.insert(s);
                return b;
        }

        static std::size_t count(BB const& b) { return static_cast<std::size_t>(b.size()); }

        // colex rank: sum over the i-th smallest element s_i of C(s_i, i)
        static std::uint64_t rank(BB const& set)
        {
                auto r = std::uint64_t{0};
                auto i = 1;
                for (std::size_t s : set) { r += binom(s, i++); }
                return r;
        }

        // renumber the squares of `set` among the squares not in `skip`, and back
        static BB compress(BB const& set, BB const& skip)
        {
                BB out;
                for (std::size_t s : set) {
                        auto below = std::size_t{0};
                        for (std::size_t k : skip) {
                                if (k >= s) { break; }
                                ++below;
                        }
                        out.insert(s - below);
                }
                return out;
        }

        static BB expand(BB const& set, BB const& skip)
        {
                BB out;
                for (std::size_t s : set) {
                        for (std::size_t k : skip) {
                                if (k <= s) { ++s; }
                        }
                        out.insert(s);
                }
                return out;
        }

        void init_combos(int kmax)
        {
                for (auto k = 0; k <= kmax; ++k) {
                        auto& t = combos[static_cast<std::size_t>(k)];
                        t.assign(binom(N, k), BB{});
                        BB set;
                        auto rec = [&](auto&& self, std::size_t start, int depth) -> void {
                                if (depth == k) { t[rank(set)] = set; return; }
                                for (auto s = start; s < N; ++s) {
                                        set.insert(s);
                                        self(self, s + 1, depth + 1);
                                        set.erase(s);
                                }
                        };
                        rec(rec, 0, 0);
                }
        }

        [[nodiscard]] std::uint64_t encode(BB const& white, BB const& black, std::uint64_t nb) const
        {
                return rank(white) * nb + rank(compress(black, white));
        }

        void decode(Slice const& sl, std::uint64_t idx, BB& white, BB& black) const
        {
                white = combos[static_cast<std::size_t>(sl.w)][idx / sl.nb];
                black = expand(combos[static_cast<std::size_t>(sl.b)][idx % sl.nb], white);
        }

        // ---- move generation (own kings vs opponent kings), no allocations ----

        void capture_dfs(std::size_t from, std::size_t sq, BB const& occ, BB const& opp, BB& taken, int ntaken, int& best, MoveList& ml) const
        {
                auto extended = false;
                for (auto const& rk : g.ray) {
                        auto const& r = rk[sq];
                        auto i = std::size_t{0};
                        auto const e = r.size();
                        if (flying_kings) {
                                while (i < e && !occ.contains(r[i])) { ++i; }
                        }
                        if (i >= e) { continue; }
                        auto const t = r[i];
                        if (!opp.contains(t) || taken.contains(t)) { continue; }   // empty (short king), own piece, or already taken
                        for (auto j = i + 1; j < e && !occ.contains(r[j]); ++j) {
                                extended = true;
                                taken.insert(t);
                                capture_dfs(from, r[j], occ, opp, taken, ntaken + 1, best, ml);
                                taken.erase(t);
                                if (!flying_kings) { break; }
                        }
                }
                // a capture must be completed; with majority capture only the longest ones count
                if (!extended && ntaken > 0) {
                        if (majority_capture) {
                                if (ntaken < best) { return; }
                                if (ntaken > best) { best = ntaken; ml.n = 0; }
                        } else {
                                best = std::max(best, ntaken);
                        }
                        if (ml.n >= static_cast<int>(ml.m.size())) { throw std::runtime_error("move list overflow"); }
                        ml.m[static_cast<std::size_t>(ml.n++)] = { static_cast<square>(from), static_cast<square>(sq), taken };
                }
        }

public:
        // Fills ml with the legal moves; returns > 0 if they are captures.
        int generate(BB const& own, BB const& opp, MoveList& ml) const
        {
                ml.n = 0;
                auto best = 0;
                auto occ = own | opp;
                BB taken;
                for (std::size_t s : own) {
                        occ.erase(s);                           // origin counts as empty
                        capture_dfs(s, s, occ, opp, taken, 0, best, ml);
                        occ.insert(s);
                }
                if (best > 0) {
                        // different capture orders with the same origin, destination and captured set are the same move
                        auto const first = ml.m.begin();
                        auto const last = first + ml.n;
                        auto const key = [](Move const& a) { return std::tie(a.from, a.to, a.captured); };
                        std::sort(first, last, [&](Move const& a, Move const& b) { return key(a) < key(b); });
                        ml.n = static_cast<int>(std::unique(first, last, [&](Move const& a, Move const& b) { return key(a) == key(b); }) - first);
                        return best;
                }
                for (std::size_t s : own) {
                        for (auto const& rk : g.ray) {
                                for (auto const x : rk[s]) {
                                        if (occ.contains(x)) { break; }
                                        if (ml.n >= static_cast<int>(ml.m.size())) { throw std::runtime_error("move list overflow"); }
                                        ml.m[static_cast<std::size_t>(ml.n++)] = { static_cast<square>(s), x, BB{} };
                                        if (!flying_kings) { break; }
                                }
                        }
                }
                return 0;
        }

        static Pos play(Pos p, Move const& m)
        {
                auto& own = p.stm == 0 ? p.white : p.black;
                auto& opp = p.stm == 0 ? p.black : p.white;
                own.erase(m.from);
                own.insert(m.to);
                opp -= m.captured;
                p.stm ^= 1;
                return p;
        }

        // DTM of a position for its side to move
        [[nodiscard]] std::uint8_t probe(Pos const& p) const
        {
                if (p.own().empty()) { return 0; }      // side to move has no pieces left: lost
                auto const& sl = slices[count(p.white)][count(p.black)];
                auto const v = sl.val[static_cast<std::size_t>(p.stm)][encode(p.white, p.black, sl.nb)];
                return v == UNKNOWN ? DRAW : v;
        }

        explicit Engine(Geometry const& geometry, int kmax)
        :
                g(geometry),
                N(static_cast<std::size_t>(geometry.n))
        {
                init_combos(kmax);
        }

        // ---- building one slice: initialisation + retrograde propagation ----

        void build(int w, int b)
        {
                using clock = std::chrono::steady_clock;
                auto const t0 = clock::now();
                auto& sl = slices[static_cast<std::size_t>(w)][static_cast<std::size_t>(b)];
                sl.w = w;
                sl.b = b;
                sl.nb = binom(N - static_cast<std::size_t>(w), b);
                auto const nw = binom(N, w);
                if (sl.nb != 0 && nw > (std::uint64_t{1} << 40) / sl.nb) { throw std::runtime_error("database too large"); }
                sl.size = nw * sl.nb;
                for (auto& v : sl.val) { v.assign(sl.size, UNKNOWN); }
                sl.flags.assign(sl.size, 0);
                std::array<std::vector<std::uint16_t>, 2> cnt;
                for (auto& c : cnt) { c.assign(sl.size, 0); }
                std::cout << std::format("db {}-{}: {} keys\n", w, b, sl.size) << std::flush;

                // initialisation: no moves -> lost; captures -> resolved from smaller databases; else count moves
                auto const size = static_cast<std::int64_t>(sl.size);
#pragma omp parallel
                {
                        MoveList ml;
#pragma omp for schedule(dynamic, 4096)
                        for (std::int64_t ii = 0; ii < size; ++ii) {
                                auto const i = static_cast<std::uint64_t>(ii);
                                BB white, black;
                                decode(sl, i, white, black);
                                for (auto s = 0; s < 2; ++s) {
                                        auto const p = Pos{ white, black, s };
                                        auto const caps = generate(p.own(), p.opp(), ml);
                                        if (caps) { sl.flags[i] |= static_cast<std::uint8_t>(1 << s); }
                                        if (ml.n == 0) {
                                                sl.val[static_cast<std::size_t>(s)][i] = 0;
                                        } else if (caps) {
                                                auto best = DRAW;
                                                for (auto k = 0; k < ml.n; ++k) {
                                                        auto const v = backup(probe(play(p, ml.m[static_cast<std::size_t>(k)])));
                                                        if (k == 0 || preference(v) > preference(best)) { best = v; }
                                                }
                                                sl.val[static_cast<std::size_t>(s)][i] = best;
                                        } else {
                                                if (ml.n > 65535) { throw std::runtime_error("too many moves"); }
                                                cnt[static_cast<std::size_t>(s)][i] = static_cast<std::uint16_t>(ml.n);
                                        }
                                }
                        }
                }

                // retrograde propagation, level by level (level = DTM in plies)
                std::vector<std::vector<std::uint64_t>> bucket(256);
                for (auto s = 0; s < 2; ++s) {
                        for (auto i = std::uint64_t{0}; i < sl.size; ++i) {
                                auto const v = sl.val[static_cast<std::size_t>(s)][i];
                                if (v < DRAW) { bucket[v].push_back(i << 1 | static_cast<std::uint64_t>(s)); }
                        }
                }
                for (auto lv = 0; lv < DRAW; ++lv) {
                        auto& q = bucket[static_cast<std::size_t>(lv)];
                        if (q.empty()) { continue; }
                        if (lv + 1 >= DRAW) { throw std::runtime_error("DTM too large for one byte"); }
                        auto const n_at_level = q.size();
                        for (auto qi = std::size_t{0}; qi < q.size(); ++qi) {
                                auto const i = q[qi] >> 1;
                                auto const s = static_cast<int>(q[qi] & 1);
                                BB white, black;
                                decode(sl, i, white, black);
                                auto const m = 1 - s;                   // the side that made the last move
                                auto const& mover = m == 0 ? white : black;
                                auto const occ = white | black;
                                for (std::size_t y : mover) {
                                        for (auto const& rk : g.ray) {
                                                for (auto const x : rk[y]) {
                                                        if (occ.contains(x)) { break; }
                                                        auto moved = mover;
                                                        moved.erase(y);
                                                        moved.insert(x);
                                                        auto const pi = m == 0 ? encode(moved, black, sl.nb) : encode(white, moved, sl.nb);
                                                        auto& pv = sl.val[static_cast<std::size_t>(m)][pi];
                                                        if (pv == UNKNOWN) {
                                                                // a parent that is still unknown has no captures, so x-y was a legal quiet move
                                                                if (lv % 2 == 0 || --cnt[static_cast<std::size_t>(m)][pi] == 0) {
                                                                        pv = static_cast<std::uint8_t>(lv + 1);
                                                                        bucket[static_cast<std::size_t>(lv + 1)].push_back(pi << 1 | static_cast<std::uint64_t>(m));
                                                                }
                                                        }
                                                        if (!flying_kings) { break; }
                                                }
                                        }
                                }
                        }
                        std::cout << std::format("  ply {:3}: {:8} positions resolved\n", lv, n_at_level) << std::flush;
                        std::vector<std::uint64_t>().swap(q);
                }
                for (auto& v : sl.val) {
                        std::ranges::replace(v, UNKNOWN, DRAW);
                }
                std::cout << std::format("  built in {:.1f}s\n", std::chrono::duration<double>(clock::now() - t0).count());
        }

        // ---- verification: recompute every value from its children ----------

        std::size_t verify(int w, int b) const
        {
                auto const& sl = slices[static_cast<std::size_t>(w)][static_cast<std::size_t>(b)];
                auto errors = std::size_t{0};
                auto const size = static_cast<std::int64_t>(sl.size);
#pragma omp parallel reduction(+ : errors)
                {
                        MoveList ml;
#pragma omp for schedule(dynamic, 4096)
                        for (std::int64_t ii = 0; ii < size; ++ii) {
                                auto const i = static_cast<std::uint64_t>(ii);
                                BB white, black;
                                decode(sl, i, white, black);
                                if (encode(white, black, sl.nb) != i || count(white) != static_cast<std::size_t>(w) ||
                                    count(black) != static_cast<std::size_t>(b) || !(white & black).empty()) { ++errors; continue; }
                                for (auto s = 0; s < 2; ++s) {
                                        auto const p = Pos{ white, black, s };
                                        auto const caps = generate(p.own(), p.opp(), ml);
                                        auto v = std::uint8_t{0};
                                        if (ml.n > 0) {
                                                v = DRAW;
                                                for (auto k = 0; k < ml.n; ++k) {
                                                        auto const c = backup(probe(play(p, ml.m[static_cast<std::size_t>(k)])));
                                                        if (k == 0 || preference(c) > preference(v)) { v = c; }
                                                }
                                        }
                                        errors += v != sl.val[static_cast<std::size_t>(s)][i];
                                        errors += (caps > 0) != ((sl.flags[i] >> s) & 1);
                                }
                        }
                }
                std::cout << std::format("verify db {}-{}: {}\n", w, b, errors == 0 ? "OK" : std::format("{} ERRORS", errors));
                return errors;
        }

        // ---- statistics and output -------------------------------------------

        Stats stats(int w, int b) const
        {
                auto const& sl = slices[static_cast<std::size_t>(w)][static_cast<std::size_t>(b)];
                Stats st;
                for (auto i = std::uint64_t{0}; i < sl.size; ++i) {
                        auto const f = sl.flags[i];
                        for (auto s = 0; s < 2; ++s) {
                                auto const v = sl.val[static_cast<std::size_t>(s)][i];
                                st.all[static_cast<std::size_t>(s)].add(v);
                                if (!((f >> s) & 1)) { st.stm_quiet[static_cast<std::size_t>(s)].add(v); }
                                if (f == 0) { st.both_quiet[static_cast<std::size_t>(s)].add(v); }
                                ++st.hist[static_cast<std::size_t>(s)][v];
                        }
                }
                return st;
        }

        std::vector<BB> lex_combos(int k) const
        {
                std::vector<BB> out;
                BB set;
                auto rec = [&](auto&& self, std::size_t start, int depth) -> void {
                        if (depth == k) { out.push_back(set); return; }
                        for (auto s = start; s < N; ++s) {
                                set.insert(s);
                                self(self, s + 1, depth + 1);
                                set.erase(s);
                        }
                };
                rec(rec, 0, 0);
                return out;
        }

        void write_text(int w, int b, std::filesystem::path const& dir) const
        {
                auto const& sl = slices[static_cast<std::size_t>(w)][static_cast<std::size_t>(b)];
                std::filesystem::create_directories(dir);
                auto const path = dir / std::format("db_{}_{}.txt", w, b);
                std::ofstream out(path, std::ios::binary);
                out << std::format("# kings-only EGDB: {} white kings vs {} black kings\n", w, b);
                out << std::format("# board: width={} height={} parity={} lake={}\n", g.grid.width, g.grid.height, g.grid.parity, g.lake.size());
                out << std::format("# rules: {}\n", flying_kings ? "international (flying kings, majority capture)" : "checkers (short kings, free choice of capture)");
                out << "# DTM in plies for the side to move: +n win, -n loss, D draw\n";
                out << "# white\tblack\tdtm_wtm\tdtm_btm\twhite_can_capture\tblack_can_capture\n";
                auto const join = [&](BB const& set) {
                        std::string s;
                        for (std::size_t x : set) {
                                if (!s.empty()) { s += ','; }
                                s += std::to_string(g.pdn[x]);
                        }
                        return s;
                };
                auto const bl = lex_combos(b);
                std::vector<std::string> bnames;
                for (auto const& set : bl) { bnames.push_back(join(set)); }
                std::string buf;
                for (auto const& wset : lex_combos(w)) {
                        auto const wname = join(wset);
                        auto const wr = rank(wset) * sl.nb;
                        for (auto j = std::size_t{0}; j < bl.size(); ++j) {
                                if (!(bl[j] & wset).empty()) { continue; }
                                auto const i = wr + rank(compress(bl[j], wset));
                                buf += wname; buf += '\t'; buf += bnames[j]; buf += '\t';
                                append_dtm(buf, sl.val[0][i]); buf += '\t';
                                append_dtm(buf, sl.val[1][i]); buf += '\t';
                                buf += (sl.flags[i] & 1) ? '1' : '0'; buf += '\t';
                                buf += (sl.flags[i] & 2) ? '1' : '0'; buf += '\n';
                        }
                        if (buf.size() > (1u << 20)) { out << buf; buf.clear(); }
                }
                out << buf;
                std::cout << std::format("wrote {}\n", path.string());
        }

        // Binary format: 64-byte header, then val[wtm][size], val[btm][size], flags[size].
        // Header: "EGDBFAST" magic, then little-endian int32: version, width, height, parity, lake count,
        // squares, white kings, black kings, rules (0 international, 1 checkers), then uint64 size.
        void write_binary(int w, int b, std::filesystem::path const& dir) const
        {
                auto const& sl = slices[static_cast<std::size_t>(w)][static_cast<std::size_t>(b)];
                std::filesystem::create_directories(dir);
                auto const path = dir / std::format("db_{}_{}.bin", w, b);
                std::ofstream out(path, std::ios::binary);
                std::array<char, 64> header{};
                std::ranges::copy(std::string_view("EGDBFAST"), header.begin());
                auto const fields = std::array<std::int32_t, 9>{ 1, g.grid.width, g.grid.height, g.grid.parity,
                        static_cast<std::int32_t>(g.lake.size()), g.n, w, b, flying_kings ? 0 : 1 };
                std::memcpy(header.data() + 8, fields.data(), sizeof(fields));
                std::memcpy(header.data() + 8 + sizeof(fields), &sl.size, sizeof(sl.size));
                out.write(header.data(), header.size());
                for (auto const& v : sl.val) { out.write(reinterpret_cast<char const*>(v.data()), static_cast<std::streamsize>(v.size())); }
                out.write(reinterpret_cast<char const*>(sl.flags.data()), static_cast<std::streamsize>(sl.flags.size()));
                std::cout << std::format("wrote {}\n", path.string());
        }

        // ---- FEN and queries ---------------------------------------------------

        [[nodiscard]] Pos parse_fen(std::string const& fen) const
        {
                auto p = Pos{ BB{}, BB{}, 0 };
                std::stringstream ss(fen);
                std::string field;
                auto n = 0;
                while (std::getline(ss, field, ':')) {
                        if (field.empty()) { continue; }
                        if (n++ == 0) {
                                if (field != "W" && field != "B") { throw std::invalid_argument("FEN: bad side to move: " + field); }
                                p.stm = field == "W" ? 0 : 1;
                                continue;
                        }
                        auto& pieces = field[0] == 'W' ? p.white : field[0] == 'B' ? p.black
                                     : throw std::invalid_argument("FEN: bad color: " + field);
                        std::stringstream fs(field.substr(1));
                        std::string tok;
                        while (std::getline(fs, tok, ',')) {
                                if (tok.empty()) { continue; }
                                if (tok[0] != 'K') { throw std::invalid_argument("FEN: only kings supported: " + tok); }
                                auto const sq = std::stoi(tok.substr(1));
                                if (!g.is_playable_pdn(sq)) { throw std::invalid_argument("FEN: square not on board: " + tok); }
                                pieces.insert(static_cast<std::size_t>(g.dense[static_cast<std::size_t>(sq)]));
                        }
                }
                return p;
        }

        [[nodiscard]] std::string to_fen(Pos const& p) const
        {
                auto s = std::string(p.stm == 0 ? "W" : "B");
                for (auto c = 0; c < 2; ++c) {
                        s += c == 0 ? ":W" : ":B";
                        auto first = true;
                        for (std::size_t x : (c == 0 ? p.white : p.black)) {
                                s += std::format("{}K{}", first ? "" : ",", g.pdn[x]);
                                first = false;
                        }
                }
                return s;
        }

        // kings are color-symmetric: let white be the side with at least as many kings
        [[nodiscard]] static std::pair<Pos, bool> normalize(Pos p)
        {
                if (count(p.white) >= count(p.black)) { return { p, false }; }
                std::swap(p.white, p.black);
                p.stm ^= 1;
                return { p, true };
        }

        [[nodiscard]] std::string notation(Move const& m, bool algebraic) const
        {
                auto const sep = m.captured.empty() ? '-' : 'x';
                return algebraic ? std::format("{}{}{}", g.name[m.from], sep, g.name[m.to])
                                 : std::format("{}{}{}", g.pdn[m.from], sep, g.pdn[m.to]);
        }

        struct MoveResult
        {
                Move move;
                std::uint8_t value;     // for the mover
        };

        [[nodiscard]] std::vector<MoveResult> analyse(Pos const& p) const
        {
                auto ml = std::make_unique<MoveList>();
                generate(p.own(), p.opp(), *ml);
                std::vector<MoveResult> r;
                for (auto k = 0; k < ml->n; ++k) {
                        r.push_back({ ml->m[static_cast<std::size_t>(k)], backup(probe(play(p, ml->m[static_cast<std::size_t>(k)]))) });
                }
                return r;
        }

        void query(std::string const& fen) const
        {
                auto const [p, swapped] = normalize(parse_fen(fen));
                std::cout << std::format("\nFEN {}{}\n", fen, swapped ? std::format("  (colors swapped: {})", to_fen(p)) : "");
                std::cout << g.diagram(p.white, p.black);
                auto const v = probe(p);
                std::cout << std::format("value for side to move: {}\n", describe(v));
                for (auto const& mr : analyse(p)) {
                        std::cout << std::format("  {:8} {:8} {}\n", notation(mr.move, true), notation(mr.move, false), describe(mr.value));
                }
                std::cout << "PV:";
                auto q = p;
                for (auto i = 0, n = 1; i < 40; ++i) {
                        auto const ms = analyse(q);
                        if (ms.empty() || q.opp().empty()) { break; }
                        auto const qv = probe(q);
                        auto const it = std::ranges::find_if(ms, [&](auto const& mr) { return mr.value == qv; });
                        if (it == ms.end()) { break; }
                        auto const white_moves = (q.stm == 0) != swapped;
                        if (white_moves) {
                                std::cout << std::format(" {}.", n);
                        } else if (i == 0) {
                                std::cout << std::format(" {}...", n);
                        }
                        if (!white_moves) { ++n; }
                        std::cout << ' ' << notation(it->move, true);
                        q = play(q, it->move);
                        if (qv >= DRAW && i >= 9) { std::cout << " ..."; break; }
                }
                std::cout << '\n';
        }

        // ---- reference checks (10x10 international) -----------------------------

        int check_ref_stats(Stats const& st, int w, int b) const
        {
                auto errors = 0;
                for (auto const& r : ref_stats) {
                        if (r.w != w || r.b != b) { continue; }
                        auto const i = static_cast<std::size_t>(r.stm);
                        auto const ok_all = st.all[i] == r.all;
                        auto const ok_quiet = st.stm_quiet[i] == r.quiet;
                        std::cout << std::format("ref {} (db {}-{}, {} to move): all {}, non-capture {}\n", r.page, w, b,
                                r.stm == 0 ? "white" : "black", ok_all ? "OK" : "MISMATCH", ok_quiet ? "OK (= side to move cannot capture)" : "MISMATCH");
                        errors += !ok_all + !ok_quiet;
                }
                return errors;
        }

        int check_ref_examples(int max_w, int max_b) const
        {
                auto errors = 0;
                auto checked = 0;
                for (auto const& ex : ref_examples) {
                        auto const [p, swapped] = normalize(parse_fen(ex.fen));
                        if (count(p.white) > static_cast<std::size_t>(max_w) || count(p.black) > static_cast<std::size_t>(max_b)) { continue; }
                        ++checked;
                        std::map<std::string, char> expected, got;
                        std::stringstream ss(ex.moves);
                        for (std::string tok; ss >> tok;) { expected[tok.substr(0, tok.find(':'))] = tok.back(); }
                        for (auto const& mr : analyse(p)) { got[notation(mr.move, false)] = wdl(mr.value); }
                        auto const ok = wdl(probe(p)) == ex.value && got == expected;
                        errors += !ok;
                        if (!ok) { std::cout << std::format("ref example {} {}: MISMATCH\n", ex.page, ex.fen); }
                }
                std::cout << std::format("ref examples: {}/{} OK\n", checked - errors, checked);
                return errors;
        }

        // ---- the whole run ---------------------------------------------------------

        int run(Options const& opt)
        {
                auto const t0 = std::chrono::steady_clock::now();
                auto errors = 0;
                auto const is_international = opt.board_name == "international" && g.lake.empty() && flying_kings;
                for (auto w = 1; w <= opt.target_w; ++w) {
                        for (auto b = 1; b <= opt.target_b; ++b) {
                                build(w, b);
                                if (opt.do_verify) { errors += static_cast<int>(verify(w, b)); }
                                auto const st = stats(w, b);
                                print_stats(st, w, b);
                                if (is_international) { errors += check_ref_stats(st, w, b); }
                                if (opt.out_dir) {
                                        if (opt.do_text) { write_text(w, b, *opt.out_dir); }
                                        write_binary(w, b, *opt.out_dir);
                                }
                        }
                }
                if (is_international) { errors += check_ref_examples(opt.target_w, opt.target_b); }

                // one-line summary for board sweeps: draws among positions where neither side can capture
                auto const st = stats(opt.target_w, opt.target_b);
                auto max_win = 0;
                for (auto const& h : st.hist) {
                        for (auto v = 1; v < DRAW; v += 2) {
                                if (h[static_cast<std::size_t>(v)]) { max_win = std::max(max_win, v); }
                        }
                }
                std::cout << std::format("SUMMARY {}-{} {}x{} parity {} squares {} quiet-draws wtm {} btm {} max-win {} rules {}\n",
                        opt.target_w, opt.target_b, g.grid.width, g.grid.height, g.grid.parity, g.n,
                        st.both_quiet[0].d, st.both_quiet[1].d, max_win, flying_kings ? "international" : "checkers");
                std::cout << std::format("\ntotal time {:.1f}s, {} error(s)\n",
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), errors);

                for (auto const& q : opt.queries) { query(q); }
                return errors;
        }
};

// The bitboard for boards that need K blocks of 64 bits: bit_align rounds the width of the smallest
// such board, (K - 1) * 64 + 1 squares, up to whole blocks, so that no block carries an unused tail.
template<std::size_t K>
using bitboard = xstd::bit_align<xstd::bit_fixed_set<(K - 1) * 64 + 1>>;

constexpr std::size_t max_blocks = max_squares / 64;

template<std::size_t... Ks>
int dispatch(Geometry const& g, Options const& opt, std::index_sequence<Ks...>)
{
        auto const blocks = (static_cast<std::size_t>(g.n) + 63) / 64;
        auto const kmax = std::max(opt.target_w, opt.target_b);
        auto result = -1;
        auto const try_one = [&]<std::size_t K>() {
                if (K != blocks) { return; }
                using BB = bitboard<K>;
                static_assert(BB::max_size() == K * 64);
                std::cout << std::format("bitboard: xstd::bit_align<xstd::bit_fixed_set<{}>> = {} squares\n", (K - 1) * 64 + 1, BB::max_size());
                result = std::make_unique<Engine<BB>>(g, kmax)->run(opt);
        };
        (try_one.template operator()<Ks + 1>(), ...);
        if (result < 0) { throw std::invalid_argument("board too large"); }
        return result;
}

} // namespace fast

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

namespace {

struct Preset
{
        char const* name;
        int width, height, parity;
        std::vector<char const*> lake;
};

// Mirrors tabula/games/draughts.hpp
auto const presets = std::vector<Preset>{
        { "international",   10, 10, 1, {} },
        { "spantsiretti",    10,  8, 1, {} },
        { "mertens_cut_j10", 10, 10, 1, { "j10" } },
        { "mertens_add_k9",  11, 10, 1, { "k7", "k5", "k3", "k1" } },
        { "canadian",        12, 12, 1, {} },
        { "checkers",         8,  8, 1, {} },
};

void usage()
{
        std::cout <<
                "usage: egdb_fast [options]\n"
                "  --board NAME        preset: international (default), spantsiretti, mertens_cut_j10,\n"
                "                      mertens_add_k9, canadian, checkers\n"
                "  --width W --height H --parity P   custom chequer board (tabula convention), up to 512 squares\n"
                "  --lake SQ           remove a square, algebraic (e.g. j10); repeatable\n"
                "  --white N --black M material of the target database (default 3 1, at most 4 each)\n"
                "  --rules R           international (default) or checkers (short kings, free capture choice)\n"
                "  --out DIR           write db_<w>_<b>.txt and db_<w>_<b>.bin files to DIR\n"
                "  --no-text           with --out: only the compact binary files\n"
                "  --query FEN         analyse a position (e.g. W:WK2,K10,K45:BK50); repeatable\n"
                "  --no-verify         skip the verification pass\n";
}

auto parse_square(std::string const& s)
{
        if (s.size() < 2 || s[0] < 'a' || s[0] > 'z') { throw std::invalid_argument("bad square: " + s); }
        return tabula::algebraic(s[0], std::stoi(s.substr(1)));
}

} // namespace

int main(int argc, char* argv[])
try {
        using namespace fast;

        auto opt = Options{};
        opt.board_name = "international";
        std::optional<int> width, height, parity;
        std::vector<std::string> lake_args;

        for (auto i = 1; i < argc; ++i) {
                auto const arg = std::string(argv[i]);
                auto const next = [&]() -> std::string {
                        if (i + 1 >= argc) { throw std::invalid_argument("missing value for " + arg); }
                        return argv[++i];
                };
                if      (arg == "--board")     { opt.board_name = next(); }
                else if (arg == "--width")     { width  = std::stoi(next()); }
                else if (arg == "--height")    { height = std::stoi(next()); }
                else if (arg == "--parity")    { parity = std::stoi(next()); }
                else if (arg == "--lake")      { lake_args.push_back(next()); }
                else if (arg == "--white")     { opt.target_w = std::stoi(next()); }
                else if (arg == "--black")     { opt.target_b = std::stoi(next()); }
                else if (arg == "--out")       { opt.out_dir = next(); }
                else if (arg == "--no-text")   { opt.do_text = false; }
                else if (arg == "--query")     { opt.queries.push_back(next()); }
                else if (arg == "--no-verify") { opt.do_verify = false; }
                else if (arg == "--rules")     {
                        auto const r = next();
                        if (r == "checkers")           { flying_kings = false; majority_capture = false; }
                        else if (r != "international") { throw std::invalid_argument("unknown rules " + r); }
                }
                else if (arg == "--help" || arg == "-h") { usage(); return 0; }
                else { usage(); throw std::invalid_argument("unknown option " + arg); }
        }

        auto const preset = std::ranges::find(presets, opt.board_name, &Preset::name);
        if (preset == presets.end()) { throw std::invalid_argument("unknown board " + opt.board_name); }
        auto const custom = width || height || parity;
        if (custom) { opt.board_name = "custom"; }
        std::vector<std::pair<int, int>> lake;
        if (!custom) {
                for (auto const* sq : preset->lake) { lake.push_back(parse_square(sq)); }
        }
        for (auto const& sq : lake_args) { lake.push_back(parse_square(sq)); }
        auto const g = Geometry(
                width.value_or(custom ? 10 : preset->width),
                height.value_or(custom ? 10 : preset->height),
                parity.value_or(custom ? 1 : preset->parity),
                lake
        );
        if (opt.target_w < 1 || opt.target_w > max_kings || opt.target_b < 1 || opt.target_b > max_kings) {
                throw std::invalid_argument("material must be between 1-1 and 4-4");
        }
        init_binomial();

        std::cout << std::format("board {}: {}x{} parity {}, {} playable squares, {} lake squares\n",
                opt.board_name, g.grid.width, g.grid.height, g.grid.parity, g.n, lake.size());
        std::cout << g.diagram(xstd::bit_fixed_set<max_squares>{}, xstd::bit_fixed_set<max_squares>{});

        auto const errors = dispatch(g, opt, std::make_index_sequence<max_blocks>{});
        return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
} catch (std::exception const& e) {
        std::cerr << "error: " << e.what() << '\n';
        return EXIT_FAILURE;
}
