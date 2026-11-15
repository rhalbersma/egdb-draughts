#!/usr/bin/env bash
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

# The parallel build must not depend on the number of threads: build units with 1 and with N threads, every slice
# that is large enough solved by all threads together (EGDB_BIG_SLICE), and compare the raw files by CRC32C.
# The compressed units before them are taken (symlinked) from the store.
#
# usage: parallel_check.sh path/to/egdb_6x6 path/to/crc32c data/units workdir N "n/m n/m ..."   e.g. 4 "5/0 5/1 8/0"
set -euo pipefail
engine=$(realpath "$1"); crc=$(realpath "$2"); store=$(realpath "$3"); work=$4; threads=$5; units=$6
mkdir -p "$work"; cd "$work"
ln -sf "$store"/*.cdb .
status=0
for u in $units; do
  n=${u%/*}; m=${u#*/}
  for t in 1 "$threads"; do
    rm -f "6x6_${n}_${m}.raw" "6x6_${n}_${m}.raw.progress"
    echo "unitbuild . $n $m $t" | EGDB_BIG_SLICE=${EGDB_BIG_SLICE:-1} "$engine" > /dev/null 2>&1
    mv "6x6_${n}_${m}.raw" "t${t}_${n}_${m}.raw"
  done
  a=$("$crc" "t1_${n}_${m}.raw" | cut -d' ' -f1); b=$("$crc" "t${threads}_${n}_${m}.raw" | cut -d' ' -f1)
  echo "$u: 1 thread $a, $threads threads $b $([ "$a" = "$b" ] && echo identical || echo DIFFERENT)"
  [ "$a" = "$b" ] || status=1
done
exit $status
