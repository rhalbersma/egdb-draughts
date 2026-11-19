#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

# A CMake shell on top of Snakemake for the 6x6 endgame databases (thompson86/, syzygy/), synthesis section 7.3:
#
#   cmake -S databases -B build/syzygy-6 -DBUILDER=syzygy -DPIECES=6      configure: compile settings, the machine
#                                                                         (cores, RAM, disk), the tree of directories,
#                                                                         the disk a build needs, a Snakemake profile
#   cmake --build build/syzygy-6                                          compile the engine, build every metric
#   cmake --build build/syzygy-6 --target dtm                             one metric (dtm, dtc or dtz)
#   cmake --build build/syzygy-6 --target plan                            what would run: Snakemake's dry run
#   cmake --build build/syzygy-6 --target machine                         the machine description of the configure step
#
# The databases go to DATA_DIR (default: <build>/data), in <DATA_DIR>/<metric>/6x6/<np>/<wp>-<bp>/. Snakemake runs from
# the build directory with the profile written here (<build>/profile/config.yaml) and the values in <build>/egdb.yaml;
# `snakemake --profile <build>/profile` from anywhere does the same as the build targets.

cmake_minimum_required(VERSION 3.28)
project(egdb-6x6-databases LANGUAGES CXX)

# ---------------------------------------------------------------- what to build

set(BUILDER "syzygy" CACHE STRING "thompson86 (one-sided, half results) or syzygy (two-sided, full results)")
set_property(CACHE BUILDER PROPERTY STRINGS thompson86 syzygy)
set(PIECES 6 CACHE STRING "the number of pieces, at most")
set(METRICS "dtm;dtc;dtz" CACHE STRING "the metrics to build (dtm, dtc, dtz), each into <DATA_DIR>/<metric>")
set(GRANULARITY "material" CACHE STRING "material, tempi or configuration: how finely a material class is cut into groups")
set_property(CACHE GRANULARITY PROPERTY STRINGS material tempi configuration)
set(POLICY "seq" CACHE STRING "seq or par: every pass of a build sequential or parallel")
set_property(CACHE POLICY PROPERTY STRINGS seq par)
set(SINK "auto" CACHE STRING "auto, groups or array: where the depths of the material being built go")
set_property(CACHE SINK PROPERTY STRINGS auto groups array)
set(STYLE "86" CACHE STRING "thompson86 only: 86 (test at once) or 96 (masked late)")
set(ROUNDS "frontier" CACHE STRING "syzygy only: frontier (bit planes) or scan (rtbgen's full scans)")
set(DATA_DIR "${CMAKE_BINARY_DIR}/data" CACHE PATH "where the databases go")
if(NOT BUILDER MATCHES "^(thompson86|syzygy)$")
    message(FATAL_ERROR "BUILDER must be thompson86 or syzygy, not ${BUILDER}")
endif()
foreach(metric IN LISTS METRICS)
    if(NOT metric MATCHES "^(dtm|dtc|dtz)$")
        message(FATAL_ERROR "METRICS: ${metric} is not dtm, dtc or dtz")
    endif()
endforeach()

# ---------------------------------------------------------------- the machine

# sizes as they are sold: memory in binary units (a "16 GB" module is 16 GiB; free -h), disk in decimal ones (a "1 TB"
# disk is 10^12 bytes; df -H). Every check here compares exact MiB (2^20 bytes); only the report is scaled.
function(human_size out value step units)        # value in units[0]; one decimal from the second unit on
    set(n "${value}")
    list(GET units 0 unit)
    set(tenths "")
    list(LENGTH units count)
    math(EXPR last "${count} - 1")
    foreach(k RANGE 1 ${last})
        if(n LESS ${step})
            break()
        endif()
        if(tenths STREQUAL "")
            math(EXPR tenths "(${n} * 10 + ${step} / 2) / ${step}")
        else()
            math(EXPR tenths "(${tenths} + ${step} / 2) / ${step}")
        endif()
        math(EXPR n "${n} / ${step}")
        list(GET units ${k} unit)
    endforeach()
    if(tenths STREQUAL "")
        set(${out} "${n} ${unit}" PARENT_SCOPE)
    else()
        math(EXPR whole "${tenths} / 10")
        math(EXPR frac "${tenths} % 10")
        set(${out} "${whole}.${frac} ${unit}" PARENT_SCOPE)
    endif()
endfunction()
function(memory_size out mib)                    # binary: MiB, GiB, TiB
    human_size(h ${mib} 1024 "MiB;GiB;TiB")
    set(${out} "${h}" PARENT_SCOPE)
endfunction()
function(disk_size out mib)                      # decimal: MB, GB, TB (10^6, 10^9, 10^12 bytes)
    math(EXPR mb "(${mib} * 1048576 + 500000) / 1000000")
    human_size(h ${mb} 1000 "MB;GB;TB")
    set(${out} "${h}" PARENT_SCOPE)
endfunction()


cmake_host_system_information(RESULT LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
cmake_host_system_information(RESULT PHYSICAL_CORES QUERY NUMBER_OF_PHYSICAL_CORES)
cmake_host_system_information(RESULT TOTAL_MB QUERY TOTAL_PHYSICAL_MEMORY)
cmake_host_system_information(RESULT AVAILABLE_MB QUERY AVAILABLE_PHYSICAL_MEMORY)
math(EXPR DEFAULT_MEM_MB "${TOTAL_MB} * 3 / 4")
set(CORES "${LOGICAL_CORES}" CACHE STRING "Snakemake's --cores: the jobs side by side (default: the logical cores)")
set(MEM_MB "${DEFAULT_MEM_MB}" CACHE STRING
    "the memory budget of the jobs together, MiB (default: 75 % of physical memory, the rest for the page cache)")
if(MEM_MB GREATER TOTAL_MB)
    message(WARNING "MEM_MB=${MEM_MB} is more than the physical memory (${TOTAL_MB} MiB)")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(SNAKEMAKE_EXECUTABLE snakemake REQUIRED DOC "Snakemake (8.x)")
set(REPO "${CMAKE_CURRENT_SOURCE_DIR}/..")
file(MAKE_DIRECTORY "${DATA_DIR}")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -I "${REPO}/src/egdb_configure.py" "${BUILDER}" "${PIECES}" "${METRICS}" "${DATA_DIR}"
    OUTPUT_VARIABLE probe COMMAND_ERROR_IS_FATAL ANY)
string(REGEX REPLACE "\n$" "" probe "${probe}")
string(REPLACE "\n" ";" probe "${probe}")
foreach(line IN LISTS probe)                                    # materials, files_mb, need_mb, free_mb, rotational
    string(REGEX MATCH "^([a-z_]+)=(.*)$" _ "${line}")
    set(DISK_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
endforeach()
if(DISK_rotational STREQUAL "1")
    set(DISK_KIND "rotational (HDD): random reads into finished files are slow once they fall out of the page cache")
elseif(DISK_rotational STREQUAL "0")
    set(DISK_KIND "not rotational (SSD, NVMe or RAM)")
else()
    set(DISK_KIND "unknown")
endif()

foreach(v IN ITEMS TOTAL_MB AVAILABLE_MB MEM_MB)
    memory_size(${v}_h ${${v}})
endforeach()
foreach(v IN ITEMS DISK_free_mb DISK_need_mb DISK_files_mb)
    disk_size(${v}_h ${${v}})
endforeach()
message(STATUS "egdb 6x6: ${BUILDER}, ${PIECES} pieces, ${DISK_materials} materials, metrics ${METRICS}, granularity ${GRANULARITY}")
message(STATUS "  cores: ${PHYSICAL_CORES} physical, ${LOGICAL_CORES} logical; jobs side by side: ${CORES}, policy ${POLICY}")
message(STATUS "  memory: ${TOTAL_MB_h}, ${AVAILABLE_MB_h} available; budget ${MEM_MB_h} (sink ${SINK})")
message(STATUS "  disk: ${DATA_DIR}")
message(STATUS "    ${DISK_free_mb_h} free, ${DISK_KIND}")
message(STATUS "    needs ${DISK_need_mb_h} (${DISK_files_mb_h} of files, the rest while the largest material is written)")
if(DISK_need_mb GREATER DISK_free_mb)
    message(FATAL_ERROR "not enough disk: ${DISK_need_mb_h} needed, ${DISK_free_mb_h} free in ${DATA_DIR} "
                        "(choose another -DDATA_DIR, fewer -DMETRICS or -DPIECES)")
endif()
execute_process(                                                # the checks passed: the tree of subdivisions
    COMMAND "${Python3_EXECUTABLE}" -I "${REPO}/src/egdb_configure.py" "${BUILDER}" "${PIECES}" "${METRICS}" "${DATA_DIR}" tree
    COMMAND_ERROR_IS_FATAL ANY)
file(WRITE "${CMAKE_BINARY_DIR}/machine.yaml"
"# written by the configure step of databases/CMakeLists.txt (synthesis 7.3)
physical_cores: ${PHYSICAL_CORES}
logical_cores: ${LOGICAL_CORES}
memory_mb: ${TOTAL_MB}
memory_available_mb: ${AVAILABLE_MB}
data_dir: ${DATA_DIR}
disk_free_mb: ${DISK_free_mb}
disk_rotational: ${DISK_rotational}
disk_need_mb: ${DISK_need_mb}
")

# ---------------------------------------------------------------- the engine

include(FetchContent)
foreach(dep IN ITEMS bits ints misc)                            # header-only; their CMakeLists are not needed
    FetchContent_Declare(xstd-${dep} GIT_REPOSITORY https://github.com/rhalbersma/xstd-${dep}.git GIT_TAG main
                         SOURCE_SUBDIR no-cmake)
endforeach()
FetchContent_MakeAvailable(xstd-bits xstd-ints xstd-misc)

find_package(OpenMP REQUIRED)
find_package(TBB REQUIRED)            # std::execution::par runs on TBB with GCC's standard library
find_library(ZSTD_LIBRARY zstd REQUIRED)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
add_executable(${BUILDER} "${REPO}/${BUILDER}/${BUILDER}.cpp")
target_include_directories(${BUILDER} SYSTEM PRIVATE
    "${xstd-bits_SOURCE_DIR}/include" "${xstd-ints_SOURCE_DIR}/include" "${xstd-misc_SOURCE_DIR}/include")
target_link_libraries(${BUILDER} PRIVATE OpenMP::OpenMP_CXX ${ZSTD_LIBRARY} TBB::tbb)
target_compile_features(${BUILDER} PRIVATE cxx_std_23)
target_compile_options(${BUILDER} PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -march=native>)

# ---------------------------------------------------------------- the Snakemake profile and the build targets

set(ENGINE "${CMAKE_BINARY_DIR}/${BUILDER}")
file(WRITE "${CMAKE_BINARY_DIR}/egdb.yaml"
"# the values the Snakefile reads (written by the configure step; the metric comes with each target)
engine: ${ENGINE}
dir: ${DATA_DIR}
pieces: ${PIECES}
metric: dtm
granularity: ${GRANULARITY}
policy: ${POLICY}
sink: ${SINK}
style: '${STYLE}'
rounds: ${ROUNDS}
threads: ${CORES}
")
file(WRITE "${CMAKE_BINARY_DIR}/profile/config.yaml"
"# Snakemake's options (written by the configure step of databases/CMakeLists.txt)
snakefile: ${REPO}/${BUILDER}/Snakefile
directory: ${CMAKE_BINARY_DIR}
configfile: ${CMAKE_BINARY_DIR}/egdb.yaml
cores: ${CORES}
resources:
  - mem_mb=${MEM_MB}
printshellcmds: false
")

set(SNAKEMAKE "${SNAKEMAKE_EXECUTABLE}" --profile "${CMAKE_BINARY_DIR}/profile")
set(all_metrics)
foreach(metric IN LISTS METRICS)
    add_custom_target(${metric}
        COMMAND ${SNAKEMAKE} --config metric=${metric}
        DEPENDS ${BUILDER} USES_TERMINAL COMMENT "${BUILDER}: ${metric} up to ${PIECES} pieces")
    list(APPEND all_metrics COMMAND ${SNAKEMAKE} --config metric=${metric})
endforeach()
add_custom_target(databases ALL ${all_metrics}                  # one metric after another, each on the whole machine
    DEPENDS ${BUILDER} USES_TERMINAL COMMENT "${BUILDER}: ${METRICS} up to ${PIECES} pieces")
set(dry)
foreach(metric IN LISTS METRICS)
    list(APPEND dry COMMAND ${SNAKEMAKE} --config metric=${metric} --dry-run --quiet rules)
endforeach()
add_custom_target(plan ${dry} DEPENDS ${BUILDER} USES_TERMINAL COMMENT "what would run")
add_custom_target(machine COMMAND ${CMAKE_COMMAND} -E cat "${CMAKE_BINARY_DIR}/machine.yaml" USES_TERMINAL)
