#!/bin/bash
# Generates the WDL tables of 3 to $MAX (default 5) pieces once, into $WORK/tables, in the order of the generator's
# own driver script (a table's captures and promotions first), and compares them with the published checksums.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
WORK=${WORK:-$HERE/work}; MAX=${MAX:-5}; THREADS=${THREADS:-4}
mkdir -p "$WORK/tables" && cd "$WORK/tables"
sed -e 's/system "\(rtbgenp\?\) .*";/print "GEN \1 $tb\\n";/' -e 's/!-e \$tb\.".rtbz"/1/' "$WORK/tb/src/run.pl" > "$WORK/list.pl"
perl "$WORK/list.pl" --generate --min 3 --max $MAX | awk '/^GEN/ {print $2, $3}' > "$WORK/tables.txt"
export RTBPATH=.
while read prog tb; do
  [ -e $tb.rtbw ] && continue
  t0=$(date +%s.%N)
  "$WORK/tb/src/$prog" -w -t $THREADS $tb > $tb.log 2>&1 || { echo "FAILED $tb"; exit 1; }
  echo "$tb $(echo "$(date +%s.%N) - $t0" | bc) s $(stat -c %s $tb.rtbw) bytes"
done < "$WORK/tables.txt"
echo "generated $(ls *.rtbw | wc -l) tables, $(cat *.rtbw | wc -c) bytes"
