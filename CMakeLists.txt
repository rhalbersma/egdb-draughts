cmake_minimum_required(VERSION 3.28)
project(egdb-draughts LANGUAGES CXX)

# tabula (header-only board geometry). To use a local checkout instead of fetching:
#   cmake -B build -DFETCHCONTENT_SOURCE_DIR_TABULA=/path/to/tabula
include(FetchContent)
FetchContent_Declare(
    tabula
    GIT_REPOSITORY https://github.com/rhalbersma/tabula.git
    GIT_TAG main
)
FetchContent_MakeAvailable(tabula)

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

# fast version: bitboards, compact combinatorial indexing, retrograde analysis, OpenMP
find_package(OpenMP REQUIRED)
add_executable(egdb_fast src/egdb_fast.cpp)
target_link_libraries(egdb_fast PRIVATE tabula::tabula OpenMP::OpenMP_CXX)
target_compile_features(egdb_fast PRIVATE cxx_std_23)
target_compile_options(egdb_fast PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wshadow -march=native>
)
