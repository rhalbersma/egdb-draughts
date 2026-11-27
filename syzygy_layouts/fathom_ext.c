// Layout study: access to Fathom's sub-tables. Included at the end of a patched copy of tbprobe.c
// (scripts/patch_fathom.py), so it can use that file's static functions.
#include <dirent.h>
#include "fathom_ext.h"

static SlSub sl_subs[SL_MAX_SUBS];
static struct PairsData *sl_pairs[SL_MAX_SUBS];
static int sl_nsubs;
static char sl_cur_name[16];
static int sl_cur_k;
static sl_hook_fn sl_hook;
static uint32_t *sl_trace_sub; static uint64_t *sl_trace_idx; static size_t sl_trace_cap, sl_trace_n;
static const uint8_t sl_values[8][2] = { {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0} };

static void sl_register(struct PairsData *d, size_t tb_size, uint32_t real, uint32_t nb, uint32_t nsym, int minLen, int maxLen)
{
  if (sl_nsubs >= SL_MAX_SUBS) { fprintf(stderr, "too many sub-tables\n"); exit(1); }
  SlSub *s = &sl_subs[sl_nsubs];
  memset(s, 0, sizeof *s);
  strcpy(s->name, sl_cur_name);
  s->k = sl_cur_k++;
  s->tb_size = tb_size;
  s->is_const = d->idxBits == 0;
  s->const_value = d->constValue[0];
  s->real_blocks = real; s->num_blocks = nb; s->nsym = nsym; s->min_len = minLen; s->max_len = maxLen;
  d->ext = NULL;
  d->subId = sl_nsubs;
  sl_pairs[sl_nsubs++] = d;
}

// the pointers of a sub-table are set after setup_pairs returns: filled in once its table is open
static void sl_finish(int i)
{
  SlSub *s = &sl_subs[i]; struct PairsData *d = sl_pairs[i];
  if (s->is_const) return;
  s->block_bits = d->blockSize; s->idx_bits = d->idxBits;
  s->sympat = d->symPat; s->symlen = d->symLen; s->size_table = d->sizeTable; s->index = d->indexTable; s->data = d->data;
  for (int l = s->min_len; l <= s->max_len; l++) { s->offset[l] = from_le_u16(d->offset[l]); s->base[l] = d->base[l - s->min_len]; }
}

static int sl_dtz;
void sl_set_dtz(int on) { sl_dtz = on; }       // open the DTZ tables (.rtbz) of the directory instead; their WDL files must be there too

int sl_open_all(const char *dir)
{
  if (!tb_init(dir)) return -1;
  // the table names in a fixed order
  char names[1024][16]; int n = 0;
  DIR *D = opendir(dir); struct dirent *e;
  while ((e = readdir(D))) {
    char *dot = strstr(e->d_name, sl_dtz ? ".rtbz" : ".rtbw");
    if (!dot || dot[5] != 0 || dot - e->d_name > 12) continue;
    memcpy(names[n], e->d_name, (size_t)(dot - e->d_name)); names[n][dot - e->d_name] = 0; ++n;
  }
  closedir(D);
  qsort(names, (size_t)n, 16, (int (*)(const void *, const void *))strcmp);
  for (int t = 0; t < n; t++) {
    int pcs[16] = {0}, color = 0;
    for (char *c = names[t]; *c; c++) if (*c == 'v') color = 8; else { int pt = char_to_piece_type(*c); if (pt) pcs[pt | color]++; }
    uint64_t key = calc_key_from_pcs(pcs, false);
    struct BaseEntry *be = NULL;
    for (int i = 0; i < tbNumPiece; i++) if (pieceEntry[i].be.key == key) be = &pieceEntry[i].be;
    for (int i = 0; i < tbNumPawn; i++) if (pawnEntry[i].be.key == key) be = &pawnEntry[i].be;
    if (!be) { fprintf(stderr, "no entry for %s\n", names[t]); return -1; }
    strcpy(sl_cur_name, names[t]); sl_cur_k = 0;
    int first = sl_nsubs;
    if (!init_table(be, names[t], sl_dtz ? DTZ : WDL)) { fprintf(stderr, "cannot open %s\n", names[t]); return -1; }
    atomic_store_explicit(&be->ready[sl_dtz ? DTZ : WDL], true, memory_order_release);
    for (int i = first; i < sl_nsubs; i++) sl_finish(i);
  }
  return sl_nsubs;
}

const SlSub *sl_sub(int i) { return &sl_subs[i]; }
int sl_ref(int i, uint64_t idx) { return decompress_pairs(sl_pairs[i], idx)[0]; }
void sl_set_ext(int i, void *ext) { sl_pairs[i]->ext = ext; }
void sl_set_hook(sl_hook_fn f) { sl_hook = f; }
void sl_set_trace(uint32_t *sub, uint64_t *idx, size_t cap) { sl_trace_sub = sub; sl_trace_idx = idx; sl_trace_cap = cap; sl_trace_n = 0; }
size_t sl_trace_count(void) { return sl_trace_n; }

static uint8_t *sl_decompress(struct PairsData *d, size_t idx)
{
  if (sl_trace_sub && sl_trace_n < sl_trace_cap) { sl_trace_sub[sl_trace_n] = (uint32_t)d->subId; sl_trace_idx[sl_trace_n++] = idx; }
  if (sl_hook && d->ext) return (uint8_t *)sl_values[sl_hook(d->ext, idx)];
  return decompress_pairs(d, idx);
}

// the first `need` positions of a symbol, as symbols
static uint64_t sl_prefix(const SlSub *s, uint32_t sym, int need, uint16_t *out, uint64_t n)
{
  while (need > 0) {
    if ((int)s->symlen[sym] + 1 <= need) { if (out) out[n] = (uint16_t)sym; n++; break; }
    const uint8_t *w = s->sympat + 3 * sym;
    uint32_t s1 = ((w[1] & 0xf) << 8) | w[0], s2 = (w[2] << 4) | (w[1] >> 4);
    if ((int)s->symlen[s1] + 1 >= need) { sym = s1; continue; }
    if (out) out[n] = (uint16_t)s1;
    n++; need -= (int)s->symlen[s1] + 1; sym = s2;
  }
  return n;
}

// The symbols of a sub-table, block by block, each block cut to exactly the positions the size table gives it (the
// writer may end a block on a longer symbol with a shorter code: only its first positions count). out == NULL: count.
// *substituted: the blocks that ended on such a symbol.
uint64_t sl_extract(int i, uint16_t *out, uint64_t *substituted)
{
  const SlSub *s = &sl_subs[i]; struct PairsData *d = sl_pairs[i];
  uint64_t n = 0, subst = 0;
  for (uint32_t b = 0; b < s->real_blocks; b++) {
    int left = (int)d->sizeTable[b] + 1;
    uint32_t *ptr = (uint32_t *)(d->data + ((size_t)b << d->blockSize));
    int m = d->minLen; uint16_t *offset = d->offset; uint64_t *base = d->base - m; uint8_t *symLen = d->symLen;
    uint64_t code = from_be_u64(*(uint64_t *)ptr); ptr += 2; uint32_t bitCnt = 0;
    while (left > 0) {
      int l = m; while (code < base[l]) l++;
      uint32_t sym = from_le_u16(offset[l]); sym += (uint32_t)((code - base[l]) >> (64 - l));
      if ((int)symLen[sym] + 1 <= left) { if (out) out[n] = (uint16_t)sym; n++; }
      else { n = sl_prefix(s, sym, left, out, n); subst++; }
      left -= (int)symLen[sym] + 1;
      code <<= l; bitCnt += l;
      if (bitCnt >= 32) { bitCnt -= 32; uint32_t tmp = from_be_u32(*ptr++); code |= (uint64_t)tmp << bitCnt; }
    }
  }
  if (substituted) *substituted = subst;
  return n;
}

// the bytes of the table file a sub-table accounts for, as Fathom lays them out
void sl_sizes(int i, uint64_t *data, uint64_t *index, uint64_t *sizes, uint64_t *grammar)
{
  const SlSub *s = &sl_subs[i];
  if (s->is_const) { *data = *index = *sizes = 0; *grammar = 2; return; }
  *data = (uint64_t)s->real_blocks << s->block_bits;
  *index = 6 * ((s->tb_size + ((uint64_t)1 << s->idx_bits) - 1) >> s->idx_bits);
  *sizes = 2 * (uint64_t)s->num_blocks;
  *grammar = 12 + 2 * (uint64_t)(s->max_len - s->min_len + 1) + 3 * (uint64_t)s->nsym + (s->nsym & 1);
}
