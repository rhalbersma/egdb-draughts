// Kings-only endgame databases (DTM) for draughts on general W x H chequer boards.
//
// Deliberately simple ("banaal"):
// - one std::map per material balance, keyed by (sorted white squares, sorted black squares)
// - every value stores the DTM for both sides to move, plus which colors can capture
// - forward iteration per ply instead of retrograde unmoves
// - a mailbox board scanned square by square, embedded in a padded tabula::chequer
//
// DTM convention (per side to move, in plies):
//   +n  side to move wins in n plies (n odd)
//   -n  side to move loses in n plies (n even; -0 = no legal moves / no pieces left)
//   std::nullopt = draw (game-theoretic, no move-count rules)
//
// Rules: flying kings, mandatory capture, majority capture (most pieces),
// Turkish strike (captured pieces stay on the board until the capture is complete
// and cannot be jumped twice), the capturing king's origin square counts as empty.

#include <tabula/grids/chequer.hpp>     // chequer
#include <tabula/lake.hpp>              // algebraic
#include <tabula/padding.hpp>           // padding

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace egdb {

// ---------------------------------------------------------------------------
// Board: a runtime tabula::chequer, embedded in a padded chequer so that the
// four diagonal directions are uniform strides and edges/lakes are ghosts.
// Squares are identified by their PDN number 1..N (tabula's sequential1).
// ---------------------------------------------------------------------------

enum Content : char { ghost = '#', empty = '.', mine = 'M', theirs = 'T', taken = 'x' };

// Rules (set once in main):
//   international: flying kings, majority capture (must capture the most pieces)
//   checkers:      short kings (one square), free choice among captures
bool flying_kings     = true;
bool majority_capture = true;

struct Board
{
        static constexpr auto padding = tabula::pad_boxed(1, 1);

        tabula::chequer grid;
        std::vector<std::pair<int, int>> lake;  // (file, rank) of removed squares
        tabula::chequer padded;
        std::array<int, 4> strides{};           // NE, NW, SE, SW
        std::vector<int> pdn_to_emb;            // PDN 1..grid.size() -> embedded index, -1 for lakes
        std::vector<int> emb_to_pdn;            // embedded index -> PDN, 0 for ghosts
        std::vector<int> squares;               // playable PDN numbers, ascending
        std::vector<std::string> names;         // PDN -> algebraic name (e.g. "c3")
        std::vector<char> empty_board;          // embedded array: ghost or empty

        Board(int width, int height, int parity, std::vector<std::pair<int, int>> lake_squares)
        :
                grid{width, height, parity},
                lake(std::move(lake_squares)),
                padded(grid.pad(padding))
        {
                auto const wp = padded.width;
                assert(wp % 2 == 1);    // odd padded width => uniform diagonal strides
                auto k = 0;
                for (auto [df, dr] : { std::pair{1, 1}, {-1, 1}, {1, -1}, {-1, -1} }) {
                        strides[static_cast<std::size_t>(k++)] = (df + dr * wp) / 2;
                }

                empty_board.assign(static_cast<std::size_t>(padded.width * padded.height / 2 + 2), ghost);
                emb_to_pdn.assign(empty_board.size(), 0);
                pdn_to_emb.assign(static_cast<std::size_t>(grid.size() + 1), -1);
                names.assign(static_cast<std::size_t>(grid.size() + 1), "");

                for (auto rank = 0; rank < height; ++rank) {
                        for (auto file = 0; file < width; ++file) {
                                auto const sq = std::pair{file, rank};
                                if (!grid.is_valid(sq) || std::ranges::find(lake, sq) != lake.end()) {
                                        continue;
                                }
                                auto const pdn = grid.flip().index(std::pair{file, height - 1 - rank}) + 1;
                                auto const emb = padded.index(std::pair{file + padding.left, rank + padding.bottom});
                                assert(0 <= emb && emb < std::ssize(empty_board));
                                empty_board[static_cast<std::size_t>(emb)] = empty;
                                emb_to_pdn[static_cast<std::size_t>(emb)] = pdn;
                                pdn_to_emb[static_cast<std::size_t>(pdn)] = emb;
                                squares.push_back(pdn);
                                names[static_cast<std::size_t>(pdn)] = std::format("{}{}", static_cast<char>('a' + file), rank + 1);
                        }
                }
                std::ranges::sort(squares);
        }

        [[nodiscard]] auto emb(int pdn) const { return pdn_to_emb[static_cast<std::size_t>(pdn)]; }
        [[nodiscard]] auto pdn(int emb) const { return emb_to_pdn[static_cast<std::size_t>(emb)]; }

        [[nodiscard]] auto is_playable(int pdn) const
        {
                return 1 <= pdn && pdn <= grid.size() && emb(pdn) >= 0;
        }

        [[nodiscard]] auto diagram(std::span<int const> white, std::span<int const> black) const
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
                                        c = std::ranges::find(white, p) != white.end() ? 'W'
                                          : std::ranges::find(black, p) != black.end() ? 'B' : '.';
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
// Move generation (relative: own pieces vs opponent pieces, all kings)
// ---------------------------------------------------------------------------

struct Move
{
        int from;                       // PDN
        int to;                         // PDN
        std::vector<int> captured;      // PDN, sorted
};

// Numeric (PDN) notation, e.g. "10-23" or "5x13"
[[nodiscard]] auto notation(Move const& m)
{
        return std::format("{}{}{}", m.from, m.captured.empty() ? '-' : 'x', m.to);
}

// Algebraic notation, e.g. "c3-d4" or "d4xa1"
[[nodiscard]] auto notation(Board const& bd, Move const& m)
{
        auto const& n = bd.names;
        return std::format("{}{}{}", n[static_cast<std::size_t>(m.from)], m.captured.empty() ? '-' : 'x', n[static_cast<std::size_t>(m.to)]);
}

namespace detail {

void capture(Board const& bd, std::vector<char>& b, int from, int sq,
             std::vector<int>& path, std::size_t& best, std::vector<Move>& moves)
{
        auto extended = false;
        for (auto const d : bd.strides) {
                auto t = sq + d;
                if (flying_kings) {
                        while (b[static_cast<std::size_t>(t)] == empty) { t += d; }
                }
                if (b[static_cast<std::size_t>(t)] != theirs) { continue; }     // ghost, own, or already taken
                auto u = t + d;
                if (b[static_cast<std::size_t>(u)] != empty) { continue; }
                b[static_cast<std::size_t>(t)] = taken;                         // stays on the board as a blocker
                path.push_back(t);
                for (; b[static_cast<std::size_t>(u)] == empty; u += d) {
                        extended = true;
                        capture(bd, b, from, u, path, best, moves);
                        if (!flying_kings) { break; }                           // short king lands right behind
                }
                path.pop_back();
                b[static_cast<std::size_t>(t)] = theirs;
        }
        // a capture must be completed; with majority capture only the longest ones count
        if (!extended && !path.empty() && (!majority_capture || path.size() >= best)) {
                if (majority_capture && path.size() > best) {
                        moves.clear();
                }
                best = std::max(best, path.size());
                auto m = Move{ bd.pdn(from), bd.pdn(sq), {} };
                for (auto const t : path) { m.captured.push_back(bd.pdn(t)); }
                std::ranges::sort(m.captured);
                moves.push_back(std::move(m));
        }
}

} // namespace detail

[[nodiscard]] auto generate(Board const& bd, std::span<int const> own, std::span<int const> opp)
{
        auto b = bd.empty_board;
        for (auto const s : own) { b[static_cast<std::size_t>(bd.emb(s))] = mine;   }
        for (auto const s : opp) { b[static_cast<std::size_t>(bd.emb(s))] = theirs; }

        std::vector<Move> moves;
        std::vector<int> path;
        auto best = std::size_t{0};
        for (auto const s : own) {
                auto const from = bd.emb(s);
                b[static_cast<std::size_t>(from)] = empty;      // origin counts as empty during a capture
                detail::capture(bd, b, from, from, path, best, moves);
                b[static_cast<std::size_t>(from)] = mine;
        }
        if (best > 0) {
                // different capture orders with the same origin, destination and captured set are the same move
                auto const key = [](Move const& m) { return std::tie(m.from, m.to, m.captured); };
                std::ranges::sort(moves, {}, key);
                auto const [first, last] = std::ranges::unique(moves, {}, key);
                moves.erase(first, last);
                return moves;
        }
        for (auto const s : own) {
                auto const from = bd.emb(s);
                for (auto const d : bd.strides) {
                        for (auto t = from + d; b[static_cast<std::size_t>(t)] == empty; t += d) {
                                moves.push_back({ s, bd.pdn(t), {} });
                                if (!flying_kings) { break; }
                        }
                }
        }
        return moves;
}

[[nodiscard]] auto can_capture(Board const& bd, std::span<int const> own, std::span<int const> opp)
{
        auto const moves = generate(bd, own, opp);
        return !moves.empty() && !moves.front().captured.empty();
}

// ---------------------------------------------------------------------------
// Positions, keys and databases
// ---------------------------------------------------------------------------

enum class Side { white, black };

[[nodiscard]] constexpr auto operator!(Side s) { return s == Side::white ? Side::black : Side::white; }

struct Position
{
        std::vector<int> white;
        std::vector<int> black;
        Side stm;

        [[nodiscard]] auto own() const -> std::vector<int> const& { return stm == Side::white ? white : black; }
        [[nodiscard]] auto opp() const -> std::vector<int> const& { return stm == Side::white ? black : white; }
};

[[nodiscard]] auto play(Position p, Move const& m)
{
        auto& own = p.stm == Side::white ? p.white : p.black;
        auto& opp = p.stm == Side::white ? p.black : p.white;
        *std::ranges::find(own, m.from) = m.to;
        std::erase_if(opp, [&](int s) { return std::ranges::binary_search(m.captured, s); });
        p.stm = !p.stm;
        return p;
}

template<int N>
using Squares = std::array<int, static_cast<std::size_t>(N)>;      // sorted PDN numbers

template<int W, int B>
using Key = std::tuple<Squares<W>, Squares<B>>;

struct Value
{
        std::optional<int> dtm_wtm;
        std::optional<int> dtm_btm;
        bool white_can_capture{};
        bool black_can_capture{};

        [[nodiscard]] auto dtm(Side s)       -> std::optional<int>&       { return s == Side::white ? dtm_wtm : dtm_btm; }
        [[nodiscard]] auto dtm(Side s) const -> std::optional<int> const& { return s == Side::white ? dtm_wtm : dtm_btm; }
        [[nodiscard]] auto can_capture(Side s) const { return s == Side::white ? white_can_capture : black_can_capture; }
};

template<int W, int B>
using Map = std::map<Key<W, B>, Value>;

constexpr auto max_white = 3;
constexpr auto max_black = 3;

template<int W, int B>
Map<W, B> db;   // one map per material balance (W white kings vs B black kings)

template<int W, int B>
[[nodiscard]] auto make_key(std::span<int const> white, std::span<int const> black)
{
        assert(std::ssize(white) == W && std::ssize(black) == B);
        auto k = Key<W, B>{};
        std::ranges::copy(white, std::get<0>(k).begin());
        std::ranges::copy(black, std::get<1>(k).begin());
        std::ranges::sort(std::get<0>(k));
        std::ranges::sort(std::get<1>(k));
        return k;
}

template<int W, int B>
[[nodiscard]] auto position(Key<W, B> const& k, Side stm)
{
        auto const& [w, b] = k;
        return Position{ { w.begin(), w.end() }, { b.begin(), b.end() }, stm };
}

// Runtime (w, b) -> compile-time <W, B>
template<class R, int W = 1, int B = 1, class F>
R dispatch(int w, int b, F&& f)
{
        if constexpr (W > max_white) {
                throw std::out_of_range(std::format("material {}-{} not supported", w, b));
        } else if constexpr (B > max_black) {
                return dispatch<R, W + 1, 1>(w, b, std::forward<F>(f));
        } else {
                if (w == W && b == B) {
                        return f.template operator()<W, B>();
                }
                return dispatch<R, W, B + 1>(w, b, std::forward<F>(f));
        }
}

// DTM of a position from the perspective of its side to move.
[[nodiscard]] auto probe(Position const& p) -> std::optional<int>
{
        if (p.own().empty()) {
                return 0;       // side to move has no pieces left: lost
        }
        return dispatch<std::optional<int>>(static_cast<int>(p.white.size()), static_cast<int>(p.black.size()),
                [&]<int W, int B>() {
                        return db<W, B>.at(make_key<W, B>(p.white, p.black)).dtm(p.stm);
                }
        );
}

// The result for the mover of playing into a child with DTM c (child's side to move).
[[nodiscard]] auto backup(std::optional<int> c) -> std::optional<int>
{
        if (!c) { return std::nullopt; }
        return *c <= 0 ? 1 - *c : -(*c + 1);
}

[[nodiscard]] auto describe(std::optional<int> d) -> std::string
{
        if (!d) { return "draw"; }
        auto const n = std::abs(*d);
        return std::format("{} in {} {} ({} {})", *d > 0 ? "win" : "loss",
                n, n == 1 ? "ply" : "plies", (n + 1) / 2, (n + 1) / 2 == 1 ? "move" : "moves");
}

[[nodiscard]] auto wdl(std::optional<int> d) -> char
{
        return !d ? 'D' : *d > 0 ? 'W' : 'L';
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

int max_dtm_built = 0;  // max |DTM| over all databases built so far

template<int K, class F>
void for_each_combination(std::vector<int> const& items, F&& f)
{
        auto combo = Squares<K>{};
        auto rec = [&](auto&& self, std::size_t start, std::size_t depth) -> void {
                if (depth == K) { f(combo); return; }
                for (auto i = start; i < items.size(); ++i) {
                        combo[depth] = items[i];
                        self(self, i + 1, depth + 1);
                }
        };
        rec(rec, 0, 0);
}

// Value of a position at iteration `ply`, using only DTMs strictly smaller than ply.
[[nodiscard]] auto evaluate(Board const& bd, Position const& p, int ply) -> std::optional<int>
{
        auto const moves = generate(bd, p.own(), p.opp());
        if (moves.empty()) {
                return ply == 0 ? std::optional{0} : std::nullopt;
        }
        if (ply == 0) {
                return std::nullopt;
        }
        auto all_win = true;
        auto max_win = 0;
        for (auto const& m : moves) {
                auto const c = probe(play(p, m));
                if (!c || std::abs(*c) >= ply) {
                        all_win = false;
                        continue;
                }
                if (*c <= 0) {
                        assert(-*c == ply - 1);
                        return ply;
                }
                max_win = std::max(max_win, *c);
        }
        if (all_win) {
                assert(max_win == ply - 1);
                return -ply;
        }
        return std::nullopt;
}

template<int W, int B>
void build(Board const& bd)
{
        using clock = std::chrono::steady_clock;
        auto const t0 = clock::now();
        auto& m = db<W, B>;
        m.clear();

        for_each_combination<W>(bd.squares, [&](Squares<W> const& w) {
                for_each_combination<B>(bd.squares, [&](Squares<B> const& b) {
                        for (auto const s : b) {
                                if (std::ranges::find(w, s) != w.end()) { return; }
                        }
                        auto v = Value{};
                        v.white_can_capture = can_capture(bd, w, b);
                        v.black_can_capture = can_capture(bd, b, w);
                        m.emplace_hint(m.end(), Key<W, B>{ w, b }, v);
                });
        });
        std::cout << std::format("db {}-{}: {} keys\n", W, B, m.size());

        auto const max_sub = max_dtm_built;
        for (auto ply = 0;; ++ply) {
                auto changed = std::size_t{0};
                for (auto& [k, v] : m) {
                        for (auto const s : { Side::white, Side::black }) {
                                if (v.dtm(s)) { continue; }
                                if (auto const d = evaluate(bd, position<W, B>(k, s), ply)) {
                                        v.dtm(s) = d;
                                        ++changed;
                                }
                        }
                }
                if (changed) {
                        std::cout << std::format("  ply {:3}: {:8} positions resolved\n", ply, changed);
                        max_dtm_built = std::max(max_dtm_built, ply);
                }
                if (ply > 0 && changed == 0 && ply > max_sub) {
                        break;
                }
        }
        std::cout << std::format("  built in {:.1f}s\n", std::chrono::duration<double>(clock::now() - t0).count());
}

// ---------------------------------------------------------------------------
// Verification: recompute every value from its children (Bellman consistency).
// Together with well-founded DTMs this proves the database is the fixpoint.
// ---------------------------------------------------------------------------

[[nodiscard]] auto recompute(Board const& bd, Position const& p) -> std::optional<int>
{
        auto const moves = generate(bd, p.own(), p.opp());
        if (moves.empty()) { return 0; }
        std::optional<int> best;
        for (auto const& m : moves) {
                auto const r = backup(probe(play(p, m)));
                // order: win (fastest first) > draw > loss (slowest first)
                auto const better = [](std::optional<int> a, std::optional<int> b) {
                        auto const rank = [](std::optional<int> x) { return !x ? 0 : *x > 0 ? 1'000'000 - *x : -1'000'000 - *x; };
                        return rank(a) > rank(b);
                };
                if (&m == &moves.front() || better(r, best)) { best = r; }
        }
        return best;
}

template<int W, int B>
auto verify(Board const& bd)
{
        auto errors = std::size_t{0};
        for (auto const& [k, v] : db<W, B>) {
                for (auto const s : { Side::white, Side::black }) {
                        auto const p = position<W, B>(k, s);
                        if (recompute(bd, p) != v.dtm(s)) {
                                if (errors++ < 10) {
                                        std::cout << std::format("  MISMATCH {}-{} stm={} stored={} recomputed={}\n",
                                                W, B, s == Side::white ? 'W' : 'B', describe(v.dtm(s)), describe(recompute(bd, p)));
                                }
                        }
                        if (v.can_capture(Side::white) != can_capture(bd, p.white, p.black) ||
                            v.can_capture(Side::black) != can_capture(bd, p.black, p.white)) {
                                ++errors;
                        }
                }
        }
        std::cout << std::format("verify db {}-{}: {}\n", W, B, errors == 0 ? "OK" : std::format("{} ERRORS", errors));
        return errors;
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

struct Wdl
{
        std::size_t w{}, d{}, l{};

        void add(std::optional<int> x) { (!x ? d : *x > 0 ? w : l)++; }
        [[nodiscard]] friend auto operator==(Wdl const&, Wdl const&) -> bool = default;
};

[[nodiscard]] auto to_string(Wdl const& x)
{
        return std::format("W {:9}  D {:9}  L {:9}", x.w, x.d, x.l);
}

template<int W, int B>
struct Stats
{
        std::array<Wdl, 2> all, stm_quiet, both_quiet;  // per side to move
        std::array<std::map<int, std::size_t>, 2> hist; // per side to move: DTM -> count
};

template<int W, int B>
auto stats()
{
        auto st = Stats<W, B>{};
        for (auto const& [k, v] : db<W, B>) {
                for (auto const s : { Side::white, Side::black }) {
                        auto const i = static_cast<std::size_t>(s);
                        auto const d = v.dtm(s);
                        st.all[i].add(d);
                        if (!v.can_capture(s)) { st.stm_quiet[i].add(d); }
                        if (!v.white_can_capture && !v.black_can_capture) { st.both_quiet[i].add(d); }
                        if (d) { ++st.hist[i][*d]; }
                }
        }
        return st;
}

template<int W, int B>
void print_stats(Stats<W, B> const& st)
{
        std::cout << std::format("\n=== statistics db {}-{} ({} white kings vs {} black kings) ===\n", W, B, W, B);
        for (auto const s : { Side::white, Side::black }) {
                auto const i = static_cast<std::size_t>(s);
                std::cout << std::format("{} to move\n", s == Side::white ? "white" : "black");
                std::cout << std::format("  all positions        : {}\n", to_string(st.all[i]));
                std::cout << std::format("  side to move quiet   : {}\n", to_string(st.stm_quiet[i]));
                std::cout << std::format("  both sides quiet     : {}\n", to_string(st.both_quiet[i]));
                if (!st.hist[i].empty()) {
                        auto const [lo, hi] = std::ranges::minmax(st.hist[i] | std::views::keys);
                        std::cout << std::format("  longest win {} plies, longest loss {} plies\n",
                                hi > 0 ? hi : 0, lo <= 0 ? -lo : 0);
                        std::cout << "  DTM histogram (plies: count):";
                        auto n = 0;
                        for (auto const& [d, c] : st.hist[i]) {
                                std::cout << std::format("{}{:+}: {}", n++ % 6 == 0 ? "\n    " : "   ", d, c);
                        }
                        std::cout << '\n';
                }
        }
}

// ---------------------------------------------------------------------------
// Text output
// ---------------------------------------------------------------------------

[[nodiscard]] auto format_dtm(std::optional<int> d) -> std::string
{
        if (!d) { return "D"; }
        return *d > 0 ? std::format("+{}", *d) : std::format("-{}", -*d);
}

template<int W, int B>
void write_text(Board const& bd, std::filesystem::path const& dir)
{
        std::filesystem::create_directories(dir);
        auto const path = dir / std::format("db_{}_{}.txt", W, B);
        std::ofstream out(path);
        out << std::format("# kings-only EGDB: {} white kings vs {} black kings\n", W, B);
        out << std::format("# board: width={} height={} parity={} lake={}\n", bd.grid.width, bd.grid.height, bd.grid.parity, bd.lake.size());
        out << std::format("# rules: {}\n", flying_kings ? "international (flying kings, majority capture)" : "checkers (short kings, free choice of capture)");
        out << "# DTM in plies for the side to move: +n win, -n loss, D draw\n";
        out << "# white\tblack\tdtm_wtm\tdtm_btm\twhite_can_capture\tblack_can_capture\n";
        auto const join = [](auto const& a) {
                std::string s;
                for (auto const x : a) { s += (s.empty() ? "" : ",") + std::to_string(x); }
                return s;
        };
        for (auto const& [k, v] : db<W, B>) {
                out << join(std::get<0>(k)) << '\t' << join(std::get<1>(k)) << '\t'
                    << format_dtm(v.dtm_wtm) << '\t' << format_dtm(v.dtm_btm) << '\t'
                    << v.white_can_capture << '\t' << v.black_can_capture << '\n';
        }
        std::cout << std::format("wrote {}\n", path.string());
}

// ---------------------------------------------------------------------------
// FEN and queries
// ---------------------------------------------------------------------------

// Parses e.g. "W:WK2,K10,K45:BK50". Only kings are supported.
[[nodiscard]] auto parse_fen(Board const& bd, std::string const& fen) -> Position
{
        auto p = Position{};
        std::stringstream ss(fen);
        std::string field;
        auto n = 0;
        while (std::getline(ss, field, ':')) {
                if (field.empty()) { continue; }
                if (n++ == 0) {
                        if (field != "W" && field != "B") { throw std::invalid_argument("FEN: bad side to move: " + field); }
                        p.stm = field == "W" ? Side::white : Side::black;
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
                        if (!bd.is_playable(sq)) { throw std::invalid_argument("FEN: square not on board: " + tok); }
                        pieces.push_back(sq);
                }
        }
        return p;
}

[[nodiscard]] auto to_fen(Position const& p)
{
        auto s = std::string(p.stm == Side::white ? "W" : "B");
        for (auto const c : { Side::white, Side::black }) {
                auto const& v = c == Side::white ? p.white : p.black;
                s += c == Side::white ? ":W" : ":B";
                for (auto i = 0UZ; i < v.size(); ++i) { s += std::format("{}K{}", i ? "," : "", v[i]); }
        }
        return s;
}

// Swaps colors so that white is the side with at least as many kings (kings are color-symmetric).
[[nodiscard]] auto normalize(Position p) -> std::pair<Position, bool>
{
        if (p.white.size() >= p.black.size()) { return { p, false }; }
        std::swap(p.white, p.black);
        p.stm = !p.stm;
        return { p, true };
}

struct MoveResult
{
        Move move;
        std::optional<int> value;       // for the mover
};

[[nodiscard]] auto analyse(Board const& bd, Position const& p)
{
        std::vector<MoveResult> r;
        for (auto const& m : generate(bd, p.own(), p.opp())) {
                r.push_back({ m, backup(probe(play(p, m))) });
        }
        return r;
}

void query(Board const& bd, std::string const& fen)
{
        auto const [p, swapped] = normalize(parse_fen(bd, fen));
        std::cout << std::format("\nFEN {}{}\n", fen, swapped ? std::format("  (colors swapped: {})", to_fen(p)) : "");
        std::cout << bd.diagram(p.white, p.black);
        auto const v = probe(p);
        std::cout << std::format("value for side to move: {}\n", describe(v));
        auto const moves = analyse(bd, p);
        for (auto const& mr : moves) {
                std::cout << std::format("  {:8} {:8} {}\n", notation(bd, mr.move), notation(mr.move), describe(mr.value));
        }
        // principal variation, numbered with the colors of the original FEN
        std::cout << "PV:";
        auto q = p;
        for (auto i = 0, n = 1; i < 40; ++i) {
                auto const ms = analyse(bd, q);
                if (ms.empty() || q.opp().empty()) { break; }
                auto const qv = probe(q);
                auto const it = std::ranges::find_if(ms, [&](auto const& mr) { return mr.value == qv; });
                assert(it != ms.end());
                auto const white_moves = (q.stm == Side::white) != swapped;
                if (white_moves) {
                        std::cout << std::format(" {}.", n);
                } else if (i == 0) {
                        std::cout << std::format(" {}...", n);
                }
                if (!white_moves) { ++n; }
                std::cout << ' ' << notation(bd, it->move);
                q = play(q, it->move);
                if (!qv && i >= 9) { std::cout << " ..."; break; }
        }
        std::cout << '\n';
}

// For every quiet win with white to move (and DTM >= min_dtm): follow the PV and
// record how it ends: the first capture by black (number of pieces, landing square).
// Prints one example position with its PV per category.
template<int W, int B>
void motif(Board const& bd, int min_dtm)
{
        auto const is_corner = [&](int pdn) {
                auto const& n = bd.names[static_cast<std::size_t>(pdn)];
                auto const file = n[0] - 'a';
                auto const rank = std::stoi(n.substr(1)) - 1;
                return (file == 0 || file == bd.grid.width - 1) && (rank == 0 || rank == bd.grid.height - 1);
        };
        std::map<std::string, std::pair<std::size_t, Position>> hist;
        auto total = std::size_t{0};
        for (auto const& [k, v] : db<W, B>) {
                if (v.white_can_capture || v.black_can_capture || !v.dtm_wtm || *v.dtm_wtm < std::max(1, min_dtm)) { continue; }
                ++total;
                auto const start = position<W, B>(k, Side::white);
                auto q = start;
                std::string label = "no capture by black";
                while (!q.opp().empty()) {
                        auto const ms = analyse(bd, q);
                        if (ms.empty()) { break; }
                        auto const qv = probe(q);
                        auto const it = std::ranges::find_if(ms, [&](auto const& mr) { return mr.value == qv; });
                        if (q.stm == Side::black && !it->move.captured.empty()) {
                                label = std::format("black captures {} landing on {}{}", it->move.captured.size(),
                                        bd.names[static_cast<std::size_t>(it->move.to)], is_corner(it->move.to) ? " (corner)" : "");
                                break;
                        }
                        q = play(q, it->move);
                }
                auto& [n, example] = hist[label];
                if (n++ == 0) { example = start; }
        }
        std::cout << std::format("\nmotif db {}-{}: {} quiet wins with white to move and DTM >= {}\n", W, B, total, min_dtm);
        for (auto const& [label, entry] : hist) {
                std::cout << std::format("  {:44} {:7}   e.g. {}\n", label, entry.first, to_fen(entry.second));
        }
        for (auto const& [label, entry] : hist) {
                std::cout << "\nexample: " << label;
                query(bd, to_fen(entry.second));
        }
}

// ---------------------------------------------------------------------------
// Reference checks against https://mdgsoftware.nl/draughts/stats/ (10x10 international)
// ---------------------------------------------------------------------------

struct RefStats
{
        int w, b;       // material in our database (white = strong side)
        Side stm;
        Wdl all;
        Wdl quiet;      // "non capture positions"
        char const* page;
};

constexpr auto ref_stats = std::array{
        RefStats{ 1, 1, Side::white, {    424,   2'024,      2 }, {     16,   2'024,      2 }, "0101" },
        RefStats{ 2, 1, Side::white, { 19'398,  39'402,      0 }, {  2'330,  39'402,      0 }, "0201" },
        RefStats{ 2, 1, Side::black, {  3'240,  54'988,    572 }, {      0,  40'920,    236 }, "0102" },
        RefStats{ 3, 1, Side::white, {486'444, 434'756,      0 }, {134'804, 434'756,      0 }, "0301" },
        RefStats{ 3, 1, Side::black, { 16'812, 863'802, 40'586 }, {      0, 528'990, 17'210 }, "0103" },
};

struct RefExample
{
        char const* fen;
        char value;                     // W/D/L for the side to move
        char const* moves;              // "from-to:R ..." for all legal moves
        char const* page;
};

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

template<int W, int B>
auto ref_stats_for(Side s) -> RefStats const*
{
        for (auto const& r : ref_stats) {
                if (r.w == W && r.b == B && r.stm == s) { return &r; }
        }
        return nullptr;
}

template<int W, int B>
auto check_ref_stats(Stats<W, B> const& st)
{
        auto errors = 0;
        for (auto const s : { Side::white, Side::black }) {
                auto const* r = ref_stats_for<W, B>(s);
                if (!r) { continue; }
                auto const i = static_cast<std::size_t>(s);
                auto const ok_all   = st.all[i] == r->all;
                auto const ok_stm   = st.stm_quiet[i] == r->quiet;
                auto const ok_both  = st.both_quiet[i] == r->quiet;
                std::cout << std::format("ref {} (db {}-{}, {} to move): all {}, non-capture {}\n",
                        r->page, W, B, s == Side::white ? "white" : "black",
                        ok_all ? "OK" : "MISMATCH",
                        ok_stm ? "OK (= side to move cannot capture)" : ok_both ? "OK (= neither side can capture)" : "MISMATCH");
                if (!ok_all) { std::cout << "    expected " << to_string(r->all) << "\n    got      " << to_string(st.all[i]) << '\n'; }
                if (!ok_stm && !ok_both) {
                        std::cout << "    expected " << to_string(r->quiet) << "\n    got (stm quiet)  " << to_string(st.stm_quiet[i])
                                  << "\n    got (both quiet) " << to_string(st.both_quiet[i]) << '\n';
                }
                errors += !ok_all + (!ok_stm && !ok_both);
        }
        return errors;
}

auto check_ref_examples(Board const& bd, int max_w, int max_b)
{
        auto errors = 0;
        auto checked = 0;
        for (auto const& ex : ref_examples) {
                auto const [p, swapped] = normalize(parse_fen(bd, ex.fen));
                if (std::ssize(p.white) > max_w || std::ssize(p.black) > max_b) { continue; }
                ++checked;
                auto ok = wdl(probe(p)) == ex.value;
                std::map<std::string, char> expected;
                std::stringstream ss(ex.moves);
                for (std::string tok; ss >> tok;) {
                        expected[tok.substr(0, tok.find(':'))] = tok.back();
                }
                std::map<std::string, char> got;
                for (auto const& mr : analyse(bd, p)) {
                        got[notation(mr.move)] = wdl(mr.value);
                }
                ok = ok && got == expected;
                errors += !ok;
                if (!ok) {
                        std::cout << std::format("ref example {} {}: MISMATCH (value {}, expected {})\n",
                                ex.page, ex.fen, wdl(probe(p)), ex.value);
                        for (auto const& [mv, r] : got) {
                                auto const e = expected.contains(mv) ? expected[mv] : '?';
                                if (e != r) { std::cout << std::format("    {} got {} expected {}\n", mv, r, e); }
                        }
                }
        }
        std::cout << std::format("ref examples: {}/{} OK\n", checked - errors, checked);
        return errors;
}

} // namespace egdb

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
                "usage: egdb [options]\n"
                "  --board NAME        preset: international (default), spantsiretti, mertens_cut_j10,\n"
                "                      mertens_add_k9, canadian, checkers\n"
                "  --width W --height H --parity P   custom chequer board (tabula convention)\n"
                "  --lake SQ           remove a square, algebraic (e.g. j10); repeatable\n"
                "  --white N --black M material of the target database (default 3 1)\n"
                "  --out DIR           write db_<w>_<b>.txt files to DIR\n"
                "  --query FEN         analyse a position (e.g. W:WK2,K10,K45:BK50); repeatable\n"
                "  --rules R           international (default) or checkers (short kings, free capture choice)\n"
                "  --no-verify         skip the verification pass\n"
                "  --motif             classify how quiet wins end (where black accepts a sacrifice)\n"
                "  --motif-min N       same, only for wins of at least N plies\n";
}

auto parse_square(std::string const& s)
{
        if (s.size() < 2 || s[0] < 'a' || s[0] > 'z') { throw std::invalid_argument("bad square: " + s); }
        return tabula::algebraic(s[0], std::stoi(s.substr(1)));
}

} // namespace

int main(int argc, char* argv[])
try {
        using namespace egdb;

        auto board_name = std::string("international");
        std::optional<int> width, height, parity;
        std::vector<std::string> lake_args;
        auto target_w = 3;
        auto target_b = 1;
        std::optional<std::filesystem::path> out_dir;
        std::vector<std::string> queries;
        auto do_verify = true;
        auto do_motif = false;
        auto motif_min = 1;

        for (auto i = 1; i < argc; ++i) {
                auto const arg = std::string(argv[i]);
                auto const next = [&]() -> std::string {
                        if (i + 1 >= argc) { throw std::invalid_argument("missing value for " + arg); }
                        return argv[++i];
                };
                if      (arg == "--board")     { board_name = next(); }
                else if (arg == "--width")     { width  = std::stoi(next()); }
                else if (arg == "--height")    { height = std::stoi(next()); }
                else if (arg == "--parity")    { parity = std::stoi(next()); }
                else if (arg == "--lake")      { lake_args.push_back(next()); }
                else if (arg == "--white")     { target_w = std::stoi(next()); }
                else if (arg == "--black")     { target_b = std::stoi(next()); }
                else if (arg == "--out")       { out_dir = next(); }
                else if (arg == "--query")     { queries.push_back(next()); }
                else if (arg == "--no-verify") { do_verify = false; }
                else if (arg == "--motif")     { do_motif = true; }
                else if (arg == "--rules")     {
                        auto const r = next();
                        if (r == "checkers")           { flying_kings = false; majority_capture = false; }
                        else if (r != "international") { throw std::invalid_argument("unknown rules " + r); }
                }
                else if (arg == "--motif-min") { do_motif = true; motif_min = std::stoi(next()); }
                else if (arg == "--help" || arg == "-h") { usage(); return 0; }
                else { usage(); throw std::invalid_argument("unknown option " + arg); }
        }

        auto const preset = std::ranges::find(presets, board_name, &Preset::name);
        if (preset == presets.end()) { throw std::invalid_argument("unknown board " + board_name); }
        auto const custom = width || height || parity;
        if (custom) { board_name = "custom"; }
        std::vector<std::pair<int, int>> lake;
        if (!custom) {
                for (auto const* sq : preset->lake) { lake.push_back(parse_square(sq)); }
        }
        for (auto const& sq : lake_args) { lake.push_back(parse_square(sq)); }
        auto const bd = Board(
                width.value_or(custom ? 10 : preset->width),
                height.value_or(custom ? 10 : preset->height),
                parity.value_or(custom ? 1 : preset->parity),
                lake
        );
        if (target_w < 1 || target_w > max_white || target_b < 1 || target_b > max_black) {
                throw std::invalid_argument("material must be between 1-1 and 3-3");
        }

        std::cout << std::format("board {}: {}x{} parity {}, {} playable squares, {} lake squares\n",
                board_name, bd.grid.width, bd.grid.height, bd.grid.parity, bd.squares.size(), lake.size());
        std::cout << bd.diagram({}, {});

        auto const t0 = std::chrono::steady_clock::now();
        auto errors = 0;
        auto const is_international = board_name == "international" && lake.empty() && flying_kings;
        for (auto w = 1; w <= target_w; ++w) {
                for (auto b = 1; b <= target_b; ++b) {
                        dispatch<void>(w, b, [&]<int W, int B>() {
                                build<W, B>(bd);
                                if (do_verify) { errors += static_cast<int>(verify<W, B>(bd)); }
                                auto const st = stats<W, B>();
                                print_stats(st);
                                if (is_international) { errors += check_ref_stats(st); }
                                if (out_dir) { write_text<W, B>(bd, *out_dir); }
                        });
                }
        }
        if (is_international) { errors += check_ref_examples(bd, target_w, target_b); }

        // one-line summary for board sweeps: draws among positions where neither side can capture
        dispatch<void>(target_w, target_b, [&]<int W, int B>() {
                auto const st = stats<W, B>();
                auto max_win = 0;
                for (auto const& h : st.hist) {
                        if (!h.empty()) { max_win = std::max(max_win, h.rbegin()->first); }
                }
                std::cout << std::format("SUMMARY {}-{} {}x{} parity {} squares {} quiet-draws wtm {} btm {} max-win {} rules {}\n",
                        W, B, bd.grid.width, bd.grid.height, bd.grid.parity, bd.squares.size(),
                        st.both_quiet[0].d, st.both_quiet[1].d, max_win, flying_kings ? "international" : "checkers");
        });
        std::cout << std::format("\ntotal time {:.1f}s, {} error(s)\n",
                std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(), errors);

        if (do_motif) { dispatch<void>(target_w, target_b, [&]<int W, int B>() { motif<W, B>(bd, motif_min); }); }
        for (auto const& q : queries) { query(bd, q); }
        return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
} catch (std::exception const& e) {
        std::cerr << "error: " << e.what() << '\n';
        return EXIT_FAILURE;
}
