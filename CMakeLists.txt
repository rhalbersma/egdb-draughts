#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.30)
project(egdb-draughts LANGUAGES CXX)

# tabula (header-only board geometry). To use local checkouts instead of fetching:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_TABULA=/path/to/tabula -DFETCHCONTENT_SOURCE_DIR_XSTD-BITS=/path/to/xstd-bits
include(FetchContent)
FetchContent_Declare(
    tabula
    GIT_REPOSITORY https://github.com/rhalbersma/tabula.git
    GIT_TAG main
)
# xstd-bits (header-only bitsets, used as bitboards by egdb_fast); it pulls in xstd-ints and xstd-misc
# itself and needs the Boost.Container headers (find_package(boost_container CONFIG)).
FetchContent_Declare(
    xstd-bits
    GIT_REPOSITORY https://github.com/rhalbersma/xstd-bits.git
    GIT_TAG main
)
FetchContent_MakeAvailable(tabula xstd-bits)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# simple version: std::map per material balance, forward iteration per ply
add_executable(egdb src/egdb.cpp)
target_link_libraries(egdb PRIVATE tabula::tabula)
target_compile_features(egdb PRIVATE cxx_std_23)
target_compile_options(egdb PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow>
)

# fast version: xstd::bit_fixed_set bitboards, compact combinatorial indexing, retrograde analysis, OpenMP
find_package(OpenMP REQUIRED)
add_executable(egdb_fast src/egdb_fast.cpp)
target_link_libraries(egdb_fast PRIVATE tabula::tabula xstd::bits OpenMP::OpenMP_CXX)
target_compile_features(egdb_fast PRIVATE cxx_std_23)
target_compile_options(egdb_fast PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wshadow -march=native>
)

# the king index of a 6x6 slice: egdb_6x6's loops against xstd::bit_compress / bit_expand (PEXT / PDEP) and tables
add_executable(bench_index_6x6 strongly_solving_6x6/international/bench_index.cpp)
target_link_libraries(bench_index_6x6 PRIVATE xstd::bits)
target_compile_features(bench_index_6x6 PRIVATE cxx_std_23)
target_compile_options(bench_index_6x6 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)

# The programs with men and kings (egdb_6x6, thompson86, tbgen) are built for one board and one rule set each. The board
# is any tabula draughts board (bitboards in blocks of 64 squares): width, height, the colour of a1 (PARITY 1: dark, the
# default; 0: light) and a LAKE of removed squares, or a tabula preset (BOARD tabula::draughts::mertens_cut_j10); the
# rules by -DEGDB_6X6_CHECKERS (0: international draughts, 1: English checkers). See src/egdb_6x6.cpp.
#   egdb_board(<target> <source> <width> <height> <checkers> [PARITY <0|1>] [LAKE <"h8"_sq,...>] [BOARD <type>])
find_library(ZSTD_LIBRARY zstd REQUIRED)
# std::execution::par runs on Intel TBB with GCC's standard library (without it, it silently runs sequentially)
find_package(TBB REQUIRED)
function(egdb_board target source width height checkers)
    cmake_parse_arguments(PARSE_ARGV 5 arg "" "PARITY;LAKE;BOARD" "")
    add_executable(${target} ${source})
    target_compile_definitions(${target} PRIVATE EGDB_WIDTH=${width} EGDB_HEIGHT=${height} EGDB_6X6_CHECKERS=${checkers})
    if(DEFINED arg_PARITY)
        target_compile_definitions(${target} PRIVATE EGDB_PARITY=${arg_PARITY})
    endif()
    if(DEFINED arg_LAKE)
        target_compile_definitions(${target} PRIVATE "EGDB_LAKE=${arg_LAKE}")
    endif()
    if(DEFINED arg_BOARD)
        target_compile_definitions(${target} PRIVATE EGDB_BOARD=${arg_BOARD})
    endif()
    target_link_libraries(${target} PRIVATE tabula::tabula xstd::bits OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
    target_compile_features(${target} PRIVATE cxx_std_23)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
    )
endfunction()

# 6x6 international draughts with men and kings: slice databases built with leapfrog retrograde analysis
# (used to check "Strategie der 36 velden", see strategie_der_36_velden/), and the same engine with the rules of English
# checkers on the same 6x6 board
egdb_board(egdb_6x6 src/egdb_6x6.cpp 6 6 0)
egdb_board(egdb_6x6_checkers src/egdb_6x6.cpp 6 6 1)

# Thompson (1986, 1996) style builds on top of egdb_6x6's rules: one-sided files per material (White-to-move wins,
# Black-to-move losses) for dtm, dtc and dtz, bit planes per group (see thompson86/)
egdb_board(thompson86 thompson86/thompson86.cpp 6 6 0)

# a Syzygy (rtbgen) style build on top of egdb_6x6's rules: two-sided files per material for dtm, dtc and dtz, with
# WDL as a by-product, the per-material index of thompson86 (see tbgen/): itbgen for international draughts, ctbgen with the rules of English
# checkers (the first letter names the rules, as the r of rtbgen names regular chess)
egdb_board(itbgen tbgen/tbgen.cpp 6 6 0)
egdb_board(ctbgen tbgen/tbgen.cpp 6 6 1)

# the same programs on the full boards: English checkers on 8x8 and international draughts on 10x10 (egdb_* for its
# perft and the slice solver that tbgen's checkall compares with)
egdb_board(egdb_8x8_checkers src/egdb_6x6.cpp 8 8 1)
egdb_board(egdb_10x10 src/egdb_6x6.cpp 10 10 0)
egdb_board(ctbgen_8x8 tbgen/tbgen.cpp 8 8 1)
egdb_board(itbgen_10x10 tbgen/tbgen.cpp 10 10 0)

# a few boards of the kings-only sites, now with men: 8x8 with a1 light (the colouring of Italian and Spanish draughts,
# here with the checkers rules), 10x10 without j10 (Mertens; no colour mirror: both colours of every material), and a
# board with an odd side (6 wide, 5 high: the colour mirror is the reflection in the middle row)
egdb_board(ctbgen_8x8_p0 tbgen/tbgen.cpp 8 8 1 PARITY 0)
egdb_board(itbgen_10x10_j10 tbgen/tbgen.cpp 10 10 0 BOARD tabula::draughts::mertens_cut_j10)
egdb_board(itbgen_6x5 tbgen/tbgen.cpp 6 5 0)
# more than 64 squares (bitboards of two blocks): Canadian draughts' 12x12 board with the international rules
egdb_board(itbgen_12x12 tbgen/tbgen.cpp 12 12 0 BOARD tabula::draughts::canadian)
