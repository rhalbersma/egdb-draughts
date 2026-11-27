// Layout study: the interface to Fathom's sub-tables (see fathom_ext.c)
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { SL_MAX_SUBS = 4096 };
typedef struct {
  char name[16];                // the table, e.g. KRPvKR
  int k;                        // the sub-table's number within its table file
  uint64_t tb_size;             // index values
  int is_const, const_value;    // a single value, no data
  int block_bits, idx_bits, min_len, max_len;
  uint32_t real_blocks, num_blocks, nsym;
  const uint8_t *sympat;        // 3 bytes per symbol
  const uint8_t *symlen;        // positions less one
  uint16_t offset[40];          // by code length: the first symbol with that length
  uint64_t base[40];            // by code length: the first code of that length, left-aligned in 64 bits
  const uint16_t *size_table;
  const uint8_t *index, *data;
} SlSub;
typedef int (*sl_hook_fn)(void *ext, uint64_t idx);     // a replacement lookup: the stored value of index idx
int sl_open_all(const char *dir);                       // opens every WDL table of dir; the number of sub-tables or -1
const SlSub *sl_sub(int i);
int sl_ref(int i, uint64_t idx);                        // Fathom's own lookup
void sl_set_ext(int i, void *ext);                      // probes of sub-table i go to the hook with this pointer
void sl_set_hook(sl_hook_fn f);
void sl_set_trace(uint32_t *sub, uint64_t *idx, size_t cap);   // log (sub-table, index) of every probe
size_t sl_trace_count(void);
uint64_t sl_extract(int i, uint16_t *out, uint64_t *substituted);
void sl_sizes(int i, uint64_t *data, uint64_t *index, uint64_t *sizes, uint64_t *grammar);
#ifdef __cplusplus
}
#endif
