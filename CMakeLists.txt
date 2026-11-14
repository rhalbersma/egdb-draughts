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

# 6x6 international draughts with men and kings: slice databases built with leapfrog retrograde analysis
# (used to check "Strategie der 36 velden", see strategie_der_36_velden/)
add_executable(egdb_6x6 src/egdb_6x6.cpp)
target_link_libraries(egdb_6x6 PRIVATE OpenMP::OpenMP_CXX)
target_compile_features(egdb_6x6 PRIVATE cxx_std_20)
target_compile_options(egdb_6x6 PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>
)
