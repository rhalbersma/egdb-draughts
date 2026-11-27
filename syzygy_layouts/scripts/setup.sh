#!/bin/bash
# Fetches and builds what the study needs, into $WORK (default: ./work): the Syzygy generator and Fathom at pinned
# commits, a patched copy of Fathom's prober, and the study's tool. Neither project's files are modified in place.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
WORK=${WORK:-$HERE/work}
TB_COMMIT=0bb8aeee525f364bb750f96df312a1a7c9b54398          # syzygy1/tb
FATHOM_COMMIT=c9c6fef0dddc05d2e242c183acf5833149ab676d      # jdart1/Fathom
mkdir -p "$WORK" && cd "$WORK"
[ -d tb ] || git clone -q https://github.com/syzygy1/tb.git tb
(cd tb && git checkout -q $TB_COMMIT && cd src && make -j1 all >/dev/null)        # -j1: the link-time optimisation runs out of memory otherwise
[ -d fathom ] || git clone -q https://github.com/jdart1/Fathom.git fathom
(cd fathom && git checkout -q $FATHOM_COMMIT)
python3 "$HERE/scripts/patch_fathom.py" fathom/src/tbprobe.c tbprobe_sl.c
gcc -O3 -march=native -DNDEBUG -std=gnu11 -Ifathom/src -I"$HERE" -c tbprobe_sl.c -o tbprobe_sl.o
g++ -O3 -DNDEBUG -std=gnu++23 -march=native -Wall -Wextra -I"$HERE" -Ifathom/src "$HERE/sl.cpp" tbprobe_sl.o -o sl -lpthread
g++ -O3 -DNDEBUG -DSL_SUB_BITS=10 -std=gnu++23 -march=native -I"$HERE" -Ifathom/src "$HERE/sl.cpp" tbprobe_sl.o -o sl_sub10 -lpthread
echo "syzygy1/tb $TB_COMMIT (flags: $(grep -h '^CFLAGS\|^FLAGS' tb/src/Makefile | tr '\n' ' '))" > versions.txt
echo "jdart1/Fathom $FATHOM_COMMIT" >> versions.txt
echo "built in $WORK"
