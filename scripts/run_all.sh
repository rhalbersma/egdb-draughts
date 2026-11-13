#!/bin/bash
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

# Regenerate all databases and summaries with egdb_fast (build it first: cmake -B build && cmake --build build).
# - international draughts 3-1 (with 2-1 and 1-1 as sub-databases): B = 3..12, L = B..2B+3 (up to 162 squares, the larger boards take minutes each)
# - the 15 boards with one square removed from 10x10
# - English checkers 3-2 on a range of boards
# One board at a time (each run uses all cores). Databases go to data/, the summary to data/summary.txt.
# Text files are only written for databases up to 5 million positions; binary files always.
cd "$(dirname "$0")/.."
mkdir -p data/logs
bin=./build/egdb_fast
res=data/summary.txt
: > $res

binom3() { echo $(( $1 * ($1 - 1) * ($1 - 2) / 6 )); }

run() {   # name, args...
  local name=$1; shift
  local log=data/logs/$name.txt
  local t0=$(date +%s.%N)
  stdbuf -oL $bin "$@" > $log 2>&1
  local rc=$?
  local t1=$(date +%s.%N)
  printf "%-22s %7.1fs exit=%d %s | %s\n" "$name" "$(echo "$t1 - $t0" | bc)" $rc \
    "$(grep -o '[0-9]* error(s)' $log)" "$(grep -h '^SUMMARY' $log | cut -d' ' -f2-)" >> $res
}

# 1. 3-1 grid
for h in 3 4 5 6 7 8 9 10 11 12; do
  for w in $(seq $h $((2 * h + 3))); do
    for p in $( ((w % 2 && h % 2)) && echo 0 1 || echo 1); do
      n=$(( (w * h + p) / 2 ))
      [ $n -gt 512 ] && continue
      keys=$(( $(binom3 $n) * (n - 3) ))
      text=""; [ $keys -gt 5000000 ] && text="--no-text"
      run 31_${w}x${h}_p$p --width $w --height $h --parity $p --out data/${w}x${h}_p$p $text
    done
  done
done

# 2. lake boards on 10x10 (one square removed per symmetry class)
for sq in b10 d10 f10 h10 j10 c9 e9 g9 i9 d8 f8 h8 e7 g7 f6; do
  run lake_$sq --board international --lake $sq --out data/lake_$sq
done

# 3. English checkers 3-2 (width height parity)
for b in "7 6 1" "8 7 1" "8 8 1" "9 8 1" "10 8 1" "10 9 1" "10 10 1" "11 10 1" "12 10 1" "9 7 0" "9 9 0"; do
  set -- $b
  run checkers32_${1}x${2}_p$3 --width $1 --height $2 --parity $3 --rules checkers --white 3 --black 2 --out data/checkers32_${1}x${2}_p$3
done
echo "ALL DONE" >> $res
