#!/usr/bin/env python3
# Patches a copy of syzygy1/tb so that the generator also writes each WDL sub-table as its compression step receives
# it: one byte per index in the file's order, 0..4 the values and 5..8 the don't-care classes, before any don't-care
# is given a value. Written through gzip to $SL_DUMP/<table>.<sub-table>.dc.gz; nothing else changes.
# usage: patch_tb.py <copy of tb/src>
import sys
p = sys.argv[1] + '/compress_tmpl.c'
s = open(p).read()
a = '''  if (F->wdl)
    c = construct_pairs_wdl((uint8_t *)data, size, minfreq, 0);
'''
b = '''  if (F->wdl && getenv("SL_DUMP")) {
    char cmd[512];
    snprintf(cmd, sizeof cmd, "gzip -1 > '%s/%s.%d.dc.gz'", getenv("SL_DUMP"), F->name, num);
    FILE *D = popen(cmd, "w");
    if (!D || fwrite(data, 1, size, D) != size || pclose(D) != 0) { fprintf(stderr, "dump failed\\n"); exit(1); }
  }
''' + a
assert s.count(a) == 1
open(p, 'w').write(s.replace(a, b))
