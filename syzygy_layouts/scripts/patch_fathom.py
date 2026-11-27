#!/usr/bin/env python3
# Makes a copy of Fathom's tbprobe.c with three hooks for the layout study (the original is not modified):
#   - every sub-table (PairsData) is registered with its size and block counts as it is set up
#   - the one call of decompress_pairs in probe_table goes through sl_decompress (trace / replacement lookup)
#   - fathom_ext.c is included at the end, where it can reach the file's static functions
import sys
src, dst = sys.argv[1], sys.argv[2]
s = open(src).read()
def rep(a, b, n=1):
    global s
    assert s.count(a) == n, (s.count(a), a)
    s = s.replace(a, b)
rep('''  uint8_t constValue[2];
  uint64_t base[1];
};''', '''  uint8_t constValue[2];
  void *ext;
  int subId;
  uint64_t base[1];
};
static void sl_register(struct PairsData *d, size_t tb_size, uint32_t real, uint32_t nb, uint32_t nsym, int minLen, int maxLen);
static uint8_t *sl_decompress(struct PairsData *d, size_t idx);''')
rep('''    size[0] = size[1] = size[2] = 0;
    return d;''', '''    size[0] = size[1] = size[2] = 0;
    sl_register(d, tb_size, 0, 0, 0, 0, 0);
    return d;''')
rep('''  d->offset -= d->minLen;

  return d;''', '''  d->offset -= d->minLen;

  sl_register(d, tb_size, realNumBlocks, numBlocks, numSyms, minLen, maxLen);
  return d;''')
rep('uint8_t *w = decompress_pairs(ei->precomp, idx);', 'uint8_t *w = sl_decompress(ei->precomp, idx);')
s += '\n#include "fathom_ext.c"\n'
open(dst, 'w').write(s)
