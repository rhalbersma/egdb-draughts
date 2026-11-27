#!/bin/bash
# The whole study from generated tables: convert, check, measure. Results go to $WORK/results.
#   TB (default $WORK/tables)   MATERIALS (e.g. "KRPvKRP,KRBvKRN": games start in these) REACHED (materials they may capture into)   VARIANTS (default "b c0 c1 c2")   PROBES per thread (default 2000000)   COLD_PROBES (default 200000)
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
WORK=${WORK:-$HERE/work}; TB=${TB:-$WORK/tables}; OUT=$WORK/out; RES=$WORK/results; SL=$WORK/sl
VARIANTS=${VARIANTS:-"b c0 c1 c2"}; PROBES=${PROBES:-2000000}; COLD_PROBES=${COLD_PROBES:-200000}; STEPS=${STEPS:-"convert check positions bench"}
mkdir -p "$OUT" "$RES"
has() { [[ " $STEPS " == *" $1 "* ]]; }
if has convert; then
  $SL info $TB > $RES/sizes_a.csv
  for v in $VARIANTS; do $SL convert $TB $OUT $v; cp $OUT/$v/sizes.csv $RES/sizes_$v.csv; done
fi
if has check; then
  for v in $VARIANTS; do $SL check $TB $OUT $v | tee -a $RES/check.txt; $WORK/sl_sub17 convert $TB $OUT.sub17 $v 2>/dev/null; echo "subdivisions of 2^17: $($WORK/sl_sub17 check $TB $OUT.sub17 $v 100000)" | tee -a $RES/check.txt; rm -rf "${OUT:?}.sub17"; done
fi
if has positions; then
  for s in 1 2 3 4; do python3 $HERE/scripts/positions.py $WORK/pos.$s ${POSITIONS:-1000000} 5 $s $MATERIALS $REACHED & done; wait
  cat $WORK/pos.1 $WORK/pos.2 $WORK/pos.3 $WORK/pos.4 > $WORK/positions.bin; rm $WORK/pos.[1-4]
  $SL trace $TB $WORK/positions.bin $WORK/trace.bin | tee $RES/trace.txt
fi
if has bench; then
  [ -e $RES/bench.csv ] || echo "variant,trace,kind,cache,threads,probes,probes_per_s,mean_ns,p50_ns,p90_ns,p99_ns,checksum" > $RES/bench.csv
  for v in a $VARIANTS; do
    for m in seq uniform trace:$WORK/trace.bin pos:$WORK/positions.bin; do
      for t in 1 4; do
        $SL bench $TB $OUT $v $m $t $PROBES >> $RES/bench.csv
        sync; echo 3 > /proc/sys/vm/drop_caches
        $SL bench $TB $OUT $v $m $t $COLD_PROBES cold >> $RES/bench.csv
      done
    done
  done
fi
