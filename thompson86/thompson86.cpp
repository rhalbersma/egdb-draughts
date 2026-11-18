//          Copyright Rein Halbersma 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// A Thompson (1986) style build for 6x6 international draughts: one byte per position with White to move, the
// number of White moves to a win (DTM, a win in i moves is 2i - 1 plies), 0 for "not a win" (draw, loss, illegal).
//
// Runs. A run is a material (WM white men, WK white kings, BM black men, BK black kings), read as capacities: the run
// holds every position with at most that many pieces of each colour and type, the captured ones parked on graveyard
// squares above the board. Promotions leave the run (a man becomes a king: another run), captures stay inside it.
// The runs form a DAG through promotions, which Snakemake builds in order (thompson86/Snakefile).
//
// Index of a run: per colour and type a colex rank over the squares where such a piece may stand (15 for men, 18 for
// kings) plus as many graveyard squares as the capacity; captured pieces fill the graveyard from its first square on,
// so the parked layers are contiguous blocks at offsets given by the rank formula:
//   rank = colex_d(squares on the board) + sum_{j=1..p} C(S + j - 1, d + j)     (d on the board, p parked)
//   index = ((rank_wm * size_bm + rank_bm) * size_wk + rank_wk) * size_bk + rank_bk
// Slots where pieces of different colours or types share a square are never written (0 = not a win).
//
// Work inside a run: units are the slices of egdb_6x6 (a placement of the men and the numbers of kings), built in
// order of pieces on the board, then distance of the men to promotion, so that every move that leaves a unit lands
// in a finished unit or a finished run.
//   pass 0   every position: moves that leave the unit (captures, man moves, promotions) are looked up forward, no
//            uncaptures anywhere; a position without king moves inside the unit is decided here (captures are
//            compulsory); the others keep their exit results as seeds at their depth
//   rounds   Thompson's P2 / P3 / P4 on bit planes: B_i -> (White king unmoves) -> W_{i+1};
//            W_{i+1} -> (Black king unmoves) -> candidates -> (all Black king moves into W, exits already lost) -> B_{i+1}
// Only White-to-move results are kept. A Black-to-move value, needed when a White move leaves a unit, is one ply of
// search over White-to-move results: lost in max(i) moves if every Black move leads to a White win in i moves.
//
// Memory: the run's result bytes (written to disk once, sequentially, at the end) and bit planes per unit.
// Disk: other runs are read through read-only memory maps (random reads); the only write is the sequential one.
//
// build: g++-14 -std=c++23 -O2 -march=native -fopenmp -Isrc -I<xstd-bits>/include -I<xstd-ints>/include
//        -I<xstd-misc>/include thompson86/thompson86.cpp -lzstd -ltbb -o thompson86
// usage: thompson86 build <dir> WM WK BM BK      build one run (the runs it promotes into must exist in dir)
//        thompson86 check <dir> WM WK BM BK      compare every position of the run with egdb_6x6's DTM (built in memory)
//        thompson86 checkall <dir> n [threads]   the same for every run of n pieces, with one oracle
//        thompson86 size WM WK BM BK             the number of slots of a run

#define main egdb_6x6_main
#include "../src/egdb_6x6.cpp"
#undef main

#include <filesystem>
#include <fstream>
#include <map>

namespace {

// ---------------------------------------------------------------- binomials up to C(32, 32)

std::array<std::array<std::uint64_t, 33>, 33> CC{};

void init_cc()
{
        for (std::size_t n = 0; n <= 32; ++n) {
                CC[n][0] = 1;
                for (std::size_t k = 1; k <= n; ++k) CC[n][k] = CC[n - 1][k - 1] + (k <= n - 1 ? CC[n - 1][k] : 0);
        }
}

// ---------------------------------------------------------------- runs and their index

struct Caps {
        int wm, wk, bm, bk;
        auto operator<=>(Caps const&) const = default;
        std::string name() const
        {
                return std::to_string(wm) + "-" + std::to_string(wk) + "-" + std::to_string(bm) + "-" + std::to_string(bk);
        }
};

constexpr bitboard DOM_WM = FULL - PROMO_WHITE;    // squares a white man may stand on (15)
constexpr bitboard DOM_BM = FULL - PROMO_BLACK;    // squares a black man may stand on (15)

// one colour and type: colex rank over S squares plus n graveyard squares, captured pieces parked in order
struct TypeIndex {
        bitboard domain;
        std::size_t S = 0, n = 0;
        std::uint64_t size = 1;
        std::array<std::uint64_t, 33> offset{};         // offset[d]: the block of d pieces on the board (n - d parked)

        TypeIndex(bitboard dom, int cap) : domain(dom), S(dom.size()), n(std::size_t(cap))
        {
                size = CC[S + n][n];
                for (std::size_t d = 0; d <= n; ++d) {
                        std::uint64_t o = 0;
                        for (std::size_t j = 1; j <= n - d; ++j) o += CC[S + j - 1][d + j];
                        offset[d] = o;
                }
        }

        std::uint64_t rank(bitboard const& pieces) const
        {
                return rank_in(pieces, domain) + offset[pieces.size()];
        }
};

struct RunIndex {
        Caps caps;
        TypeIndex wm, bm, wk, bk;
        std::uint64_t size;

        explicit RunIndex(Caps c)
        :
                caps(c), wm(DOM_WM, c.wm), bm(DOM_BM, c.bm), wk(FULL, c.wk), bk(FULL, c.bk),
                size(wm.size * bm.size * wk.size * bk.size)
        {}

        bool fits(Pos const& p) const
        {
                return int(p.m[0].size()) <= caps.wm && int(p.k[0].size()) <= caps.wk && int(p.m[1].size()) <= caps.bm
                    && int(p.k[1].size()) <= caps.bk;
        }

        std::uint64_t index(Pos const& p) const
        {
                return ((wm.rank(p.m[0]) * bm.size + bm.rank(p.m[1])) * wk.size + wk.rank(p.k[0])) * bk.size + bk.rank(p.k[1]);
        }
};

// the run of a position reached by a move from a position of run c: a promotion moves it to the run with one man
// fewer and one king more for the side that promoted
Caps caps_after(Caps c, Pos const& before, Pos const& after)
{
        int const side = before.stm;
        if (after.k[side].size() > before.k[side].size()) {
                if (side == 0) { --c.wm; ++c.wk; } else { --c.bm; ++c.bk; }
        }
        return c;
}

std::string run_path(std::string const& dir, Caps c) { return dir + "/run-" + c.name() + ".res"; }

// ---------------------------------------------------------------- results: the run being built, and finished runs

struct Mapped {
        std::uint8_t const* p = nullptr;
        std::size_t size = 0;
};

std::string g_dir;
Caps g_current{ -1, -1, -1, -1 };
std::vector<std::uint8_t> g_res;                    // the run being built (written to disk at the end)
std::map<Caps, Mapped> g_runs;                      // finished runs, memory-mapped read-only
std::map<Caps, RunIndex> g_index;

RunIndex const& run_index(Caps c)
{
        auto it = g_index.find(c);
        if (it == g_index.end()) it = g_index.emplace(c, RunIndex(c)).first;
        return it->second;
}

std::uint8_t const* run_data(Caps c)
{
        if (c == g_current) return g_res.data();
        auto it = g_runs.find(c);
        if (it != g_runs.end()) return it->second.p;
        std::string const path = run_path(g_dir, c);
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) { std::cerr << "missing run " << path << std::endl; std::exit(1); }
        struct stat st{};
        ::fstat(fd, &st);
        std::size_t const size = std::size_t(st.st_size);
        if (size != run_index(c).size) { std::cerr << "wrong size " << path << std::endl; std::exit(1); }
        void* m = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        ::close(fd);
        if (m == MAP_FAILED) { std::cerr << "mmap failed " << path << std::endl; std::exit(1); }
        g_runs[c] = { static_cast<std::uint8_t const*>(m), size };
        return static_cast<std::uint8_t const*>(m);
}

// White to move: moves to a White win, 0 if not a win
std::uint8_t lookup_wtm(Caps c, Pos const& p)
{
        if ((p.m[0] | p.k[0]).empty()) return 0;          // White has no pieces: lost
        return run_data(c)[run_index(c).index(p)];
}

// Black to move: the number of moves in which Black is lost, -1 if Black is not lost (one ply over White results)
int lookup_btm_lost(Caps c, Pos const& p)
{
        if ((p.m[1] | p.k[1]).empty()) return 0;          // Black has no pieces: lost
        int lost = 0;
        for (auto const& mv : moves(p)) {                   // no move: lost now (0)
                std::uint8_t const v = lookup_wtm(caps_after(c, p, mv.next), mv.next);
                if (v == 0) return -1;
                lost = std::max(lost, int(v));
        }
        return lost;
}

// ---------------------------------------------------------------- bit planes

struct Plane {
        std::vector<std::uint64_t> w;
        explicit Plane(std::uint64_t n = 0) : w((n + 63) / 64, 0) {}
        bool test(std::uint64_t i) const { return w[i >> 6] >> (i & 63) & 1; }
        void set(std::uint64_t i) { w[i >> 6] |= std::uint64_t(1) << (i & 63); }
        void clear() { std::ranges::fill(w, 0); }
        bool empty() const { return std::ranges::all_of(w, [](std::uint64_t x) { return x == 0; }); }
        template<class F> void for_each(F&& f) const
        {
                for (std::size_t k = 0; k < w.size(); ++k)
                        for (std::uint64_t b = w[k]; b; b &= b - 1) f(std::uint64_t(k) * 64 + std::uint64_t(std::countr_zero(b)));
        }
};

struct Seed { int level; std::uint32_t idx; };

// ---------------------------------------------------------------- one unit (a slice of egdb_6x6)

void solve_unit(Caps caps, RunIndex const& ri, Meta const& s)
{
        std::uint64_t const P = s.P;
        Plane open_w(P), open_b(P), lossable(P), exits_done(P), won(P), lost(P);
        std::vector<Seed> win_seeds, loss_decided, exit_seeds;

        // pass 0: forward over every position, both sides to move
        for (std::uint64_t i = 0; i < P; ++i) {
                Pos const p = pos_of(s, i, 0);
                int internal = 0, best = INF;
                for (auto const& mv : moves(p)) {
                        if (internal_move(p, mv)) { ++internal; continue; }
                        int const j = lookup_btm_lost(caps_after(caps, p, mv.next), mv.next);
                        if (j >= 0) best = std::min(best, j + 1);
                }
                if (internal > 0) open_w.set(i);
                if (best < INF) win_seeds.push_back({ best, std::uint32_t(i) });
        }
        for (std::uint64_t i = 0; i < P; ++i) {
                Pos const p = pos_of(s, i, 1);
                auto const ml = moves(p);
                if (ml.empty()) { loss_decided.push_back({ 0, std::uint32_t(i) }); continue; }
                int internal = 0, slowest = 0;
                bool all_lose = true;
                for (auto const& mv : ml) {
                        if (internal_move(p, mv)) { ++internal; continue; }
                        std::uint8_t const v = lookup_wtm(caps_after(caps, p, mv.next), mv.next);
                        if (v == 0) all_lose = false;
                        else slowest = std::max(slowest, int(v));
                }
                if (internal == 0) {                                    // every move leaves the unit: decided
                        if (all_lose) loss_decided.push_back({ slowest, std::uint32_t(i) });
                        continue;
                }
                open_b.set(i);
                if (!all_lose) continue;
                lossable.set(i);
                if (slowest == 0) exits_done.set(i);                    // no exits
                else exit_seeds.push_back({ slowest, std::uint32_t(i) });
        }
        auto by_level = [](Seed const& a, Seed const& b) { return a.level < b.level; };
        std::ranges::sort(win_seeds, by_level);
        std::ranges::sort(loss_decided, by_level);
        std::ranges::sort(exit_seeds, by_level);
        int max_seed = 0;
        for (auto const* v : { &win_seeds, &loss_decided, &exit_seeds })
                if (!v->empty()) max_seed = std::max(max_seed, v->back().level);

        // rounds: B_i -> W_{i+1} -> B_{i+1}
        std::size_t ws = 0, ld = 0, xs = 0;
        Plane b_front(P), w_new(P), b_new(P), cand(P);
        while (ld < loss_decided.size() && loss_decided[ld].level == 0) { b_front.set(loss_decided[ld].idx); lost.set(loss_decided[ld].idx); ++ld; }
        for (int i = 0;; ++i) {
                // P2: White wins in i + 1: White king unmoves from Black losses in i, and exits that win in i + 1
                w_new.clear();
                b_front.for_each([&](std::uint64_t q) {
                        for_each_predecessor(s, pos_of(s, q, 1), [&](std::uint64_t id) {
                                if (open_w.test(id) && !won.test(id)) w_new.set(id);
                        });
                });
                for (; ws < win_seeds.size() && win_seeds[ws].level == i + 1; ++ws)
                        if (!won.test(win_seeds[ws].idx)) w_new.set(win_seeds[ws].idx);
                w_new.for_each([&](std::uint64_t q) {
                        won.set(q);
                        g_res[ri.index(pos_of(s, q, 0))] = std::uint8_t(i + 1);
                });

                // P3: candidates: Black king unmoves from White wins in i + 1, and positions whose exits all lose by i + 1
                cand.clear();
                for (; xs < exit_seeds.size() && exit_seeds[xs].level == i + 1; ++xs) {
                        exits_done.set(exit_seeds[xs].idx);
                        cand.set(exit_seeds[xs].idx);
                }
                w_new.for_each([&](std::uint64_t q) {
                        for_each_predecessor(s, pos_of(s, q, 0), [&](std::uint64_t id) {
                                std::uint64_t const j = id - P;
                                if (open_b.test(j) && lossable.test(j) && !lost.test(j)) cand.set(j);
                        });
                });
                // P4: lost in i + 1 iff every Black king move leads into W (the exits are lost already)
                b_new.clear();
                cand.for_each([&](std::uint64_t q) {
                        if (!exits_done.test(q) || lost.test(q)) return;
                        Pos const p = pos_of(s, q, 1);
                        for (auto const& mv : moves(p))
                                if (internal_move(p, mv) && !won.test(index_of(s, mv.next))) return;
                        b_new.set(q);
                });
                for (; ld < loss_decided.size() && loss_decided[ld].level == i + 1; ++ld) b_new.set(loss_decided[ld].idx);
                b_new.for_each([&](std::uint64_t q) { lost.set(q); });

                std::swap(b_front, b_new);
                if (w_new.empty() && b_front.empty() && i + 1 >= max_seed) break;
        }
}

// ---------------------------------------------------------------- the units of a run, in dependency order

std::vector<Meta> units_of(Caps c)
{
        std::vector<std::pair<std::tuple<int, int, std::uint64_t>, Meta>> units;
        for (std::uint32_t w = u32(DOM_WM);; w = (w - 1) & u32(DOM_WM)) {
                bitboard const wm = bb_of(w);
                if (int(wm.size()) <= c.wm) {
                        std::uint32_t const bdom = u32(DOM_BM - wm);
                        for (std::uint32_t b = bdom;; b = (b - 1) & bdom) {
                                bitboard const bm = bb_of(b);
                                if (int(bm.size()) <= c.bm)
                                        for (int nwk = 0; nwk <= c.wk; ++nwk)
                                                for (int nbk = 0; nbk <= c.bk; ++nbk) {
                                                        if (int(wm.size()) + nwk == 0 || int(bm.size()) + nbk == 0) continue;
                                                        Key const k{ wm, bm, nwk, nbk };
                                                        Meta const m = meta_of(k);
                                                        if (m.P == 0) continue;
                                                        units.push_back({ { pieces(k), distance(k), k.pack() }, m });
                                                }
                                if (b == 0) break;
                        }
                }
                if (w == 0) break;
        }
        std::ranges::sort(units, {}, [](auto const& u) { return u.first; });
        std::vector<Meta> out;
        for (auto const& u : units) out.push_back(u.second);
        return out;
}

bool build_run(std::string const& dir, Caps c)
{
        g_dir = dir;
        g_current = c;
        RunIndex const& ri = run_index(c);
        g_res.assign(ri.size, 0);
        auto const units = units_of(c);
        auto const t0 = std::chrono::steady_clock::now();
        std::uint64_t positions = 0;
        for (auto const& s : units) { solve_unit(c, ri, s); positions += 2 * s.P; }
        std::string const path = run_path(dir, c), tmp = path + ".tmp";
        {
                std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
                f.write(reinterpret_cast<char const*>(g_res.data()), std::streamsize(g_res.size()));   // one sequential write
                if (!f) { std::cerr << "write failed " << tmp << std::endl; return false; }
        }
        std::filesystem::rename(tmp, path);
        std::uint64_t wins = 0;
        int longest = 0;
        for (auto v : g_res) { wins += v != 0; longest = std::max(longest, int(v)); }
        double const sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "run " << c.name() << ": " << units.size() << " units, " << positions << " positions, " << ri.size
                  << " slots, " << wins << " White wins, longest " << longest << " moves, " << sec << " s" << std::endl;
        return true;
}

// ---------------------------------------------------------------- check against egdb_6x6

bool check_run(std::string const& dir, Caps c, int threads)
{
        g_dir = dir;
        build(c.wm + c.wk + c.bm + c.bk, threads, c.wm + c.wk + c.bm + c.bk);   // egdb_6x6: every slice, exact DTM
        RunIndex const& ri = run_index(c);
        std::uint8_t const* res = run_data(c);
        std::uint64_t checked = 0, bad_w = 0, bad_b = 0;
        for (auto const& s : units_of(c)) {
                for (std::uint64_t i = 0; i < s.P; ++i) {
                        Pos const pw = pos_of(s, i, 0);
                        std::uint16_t const v = lookup(pw);
                        int const want = v != DRAW && (v & 1) ? (v + 1) / 2 : 0;
                        if (res[ri.index(pw)] != want) {
                                if (bad_w++ < 5) std::cerr << "W " << fen_of(pw) << " got " << int(res[ri.index(pw)]) << " want " << want << std::endl;
                        }
                        Pos const pb = pos_of(s, i, 1);
                        std::uint16_t const u = lookup(pb);
                        int const want_b = u != DRAW && !(u & 1) ? u / 2 : -1;
                        int const got_b = lookup_btm_lost(c, pb);
                        if (got_b != want_b) {
                                if (bad_b++ < 5) std::cerr << "B " << fen_of(pb) << " got " << got_b << " want " << want_b << std::endl;
                        }
                        checked += 2;
                }
        }
        std::cout << "check " << c.name() << ": " << checked << " positions, " << bad_w << " White-to-move and " << bad_b
                  << " Black-to-move differences" << std::endl;
        return bad_w == 0 && bad_b == 0;
}

bool check_all(std::string const& dir, int n, int threads)
{
        g_dir = dir;
        build(n, threads, n);                                   // egdb_6x6 once, for every run
        bool ok = true;
        for (int wm = 0; wm <= n; ++wm)
                for (int wk = 0; wm + wk <= n; ++wk)
                        for (int bm = 0; wm + wk + bm <= n; ++bm) {
                                int const bk = n - wm - wk - bm;
                                if (wm + wk == 0 || bm + bk == 0) continue;
                                Caps const c{ wm, wk, bm, bk };
                                RunIndex const& ri = run_index(c);
                                std::uint8_t const* res = run_data(c);
                                std::uint64_t checked = 0, bad_w = 0, bad_b = 0;
                                for (auto const& s : units_of(c))
                                        for (std::uint64_t i = 0; i < s.P; ++i) {
                                                Pos const pw = pos_of(s, i, 0), pb = pos_of(s, i, 1);
                                                std::uint16_t const v = lookup(pw), u = lookup(pb);
                                                int const want = v != DRAW && (v & 1) ? (v + 1) / 2 : 0;
                                                int const want_b = u != DRAW && !(u & 1) ? u / 2 : -1;
                                                if (res[ri.index(pw)] != want && bad_w++ < 3)
                                                        std::cerr << "W " << fen_of(pw) << " got " << int(res[ri.index(pw)]) << " want " << want << std::endl;
                                                int const got_b = lookup_btm_lost(c, pb);
                                                if (got_b != want_b && bad_b++ < 3)
                                                        std::cerr << "B " << fen_of(pb) << " got " << got_b << " want " << want_b << std::endl;
                                                checked += 2;
                                        }
                                std::cout << "check " << c.name() << ": " << checked << " positions, " << bad_w << " / " << bad_b
                                          << " differences (White / Black to move)" << std::endl;
                                ok = ok && bad_w == 0 && bad_b == 0;
                        }
        std::cout << (ok ? "all runs agree with egdb_6x6" : "DIFFERENCES") << std::endl;
        return ok;
}

} // namespace

int main(int argc, char* argv[])
{
        init_tables();
        init_cc();
        std::vector<std::string> const a(argv + 1, argv + argc);
        auto caps_at = [&](std::size_t i) { return Caps{ std::stoi(a[i]), std::stoi(a[i + 1]), std::stoi(a[i + 2]), std::stoi(a[i + 3]) }; };
        if (a.size() == 6 && a[0] == "build") return build_run(a[1], caps_at(2)) ? 0 : 1;
        if ((a.size() == 6 || a.size() == 7) && a[0] == "check") return check_run(a[1], caps_at(2), a.size() == 7 ? std::stoi(a[6]) : 4) ? 0 : 1;
        if ((a.size() == 3 || a.size() == 4) && a[0] == "checkall") return check_all(a[1], std::stoi(a[2]), a.size() == 4 ? std::stoi(a[3]) : 4) ? 0 : 1;
        if (a.size() == 5 && a[0] == "size") { std::cout << RunIndex(caps_at(1)).size << std::endl; return 0; }
        std::cerr << "usage: thompson86 build|check <dir> WM WK BM BK [threads] | checkall <dir> n [threads] | size WM WK BM BK" << std::endl;
        return 2;
}
