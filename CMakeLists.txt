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

# 6x6 international draughts with men and kings: slice databases built with leapfrog retrograde analysis
# (used to check "Strategie der 36 velden", see strategie_der_36_velden/)
add_executable(egdb_6x6 src/egdb_6x6.cpp)
find_library(ZSTD_LIBRARY zstd REQUIRED)
# std::execution::par runs on Intel TBB with GCC's standard library (without it, it silently runs sequentially)
find_package(TBB REQUIRED)
target_link_libraries(egdb_6x6 PRIVATE xstd::bits OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
target_compile_features(egdb_6x6 PRIVATE cxx_std_23)
target_compile_options(egdb_6x6 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)

# the same engine with the rules of English checkers on the same 6x6 board
add_executable(egdb_6x6_checkers src/egdb_6x6.cpp)
target_compile_definitions(egdb_6x6_checkers PRIVATE EGDB_6X6_CHECKERS=1)
target_link_libraries(egdb_6x6_checkers PRIVATE xstd::bits OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
target_compile_features(egdb_6x6_checkers PRIVATE cxx_std_23)
target_compile_options(egdb_6x6_checkers PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)

# Thompson (1986, 1996) style builds on top of egdb_6x6's rules: one-sided files per material (White-to-move wins,
# Black-to-move losses) for dtm, dtc and dtz, bit planes per group (see thompson86/)
add_executable(thompson86 thompson86/thompson86.cpp)
target_link_libraries(thompson86 PRIVATE xstd::bits OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
target_compile_features(thompson86 PRIVATE cxx_std_23)
target_compile_options(thompson86 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)

# a Syzygy (rtbgen) style build on top of egdb_6x6's rules: two-sided files per material for dtm, dtc and dtz, with
# WDL as a by-product, the per-material index of thompson86 (see syzygy/)
add_executable(syzygy syzygy/syzygy.cpp)
target_link_libraries(syzygy PRIVATE xstd::bits OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
target_compile_features(syzygy PRIVATE cxx_std_23)
target_compile_options(syzygy PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)

# the same builder with the rules of English checkers on the same 6x6 board
add_executable(syzygy_checkers syzygy/syzygy.cpp)
target_compile_definitions(syzygy_checkers PRIVATE EGDB_6X6_CHECKERS=1)
target_link_libraries(syzygy_checkers PRIVATE xstd::bits OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
target_compile_features(syzygy_checkers PRIVATE cxx_std_23)
target_compile_options(syzygy_checkers PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)
