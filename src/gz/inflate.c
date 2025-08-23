#include <stdint.h>
#include <mips.h>
#include "pi.h"

struct huf
{
  uint16_t *max;
  int16_t  *idx;
  int16_t  *tbl;
};

static uint16_t   lit_max[15];
static int16_t    lit_idx[15];
static int16_t    lit_tbl[288];

static uint16_t   dst_max[15];
static int16_t    dst_idx[15];
static int16_t    dst_tbl[32];

static struct huf lit_huf =
{
  lit_max,
  lit_idx,
  lit_tbl,
};

static struct huf dst_huf =
{
  dst_max,
  dst_idx,
  dst_tbl,
};

static uint32_t   dev_addr;

#define BUF_SIZE  0x400
#define BUF_MASK  (BUF_SIZE - 1)

static uint8_t    buf[BUF_SIZE];
static uint32_t   buf_rdpos;

#define WND_SIZE  0x8000
#define WND_MASK  (WND_SIZE - 1)

static uint8_t    wnd[WND_SIZE];
static uint32_t   wnd_wrpos;
static uint32_t   wnd_rdpos;

static int        bits_have;
static uint32_t   bits_next;

static int        block_last;
static int        block_type;
static int        block_len;
static int        block_dst;
static int        block_fin;

static uint8_t get(void)
{
  if ((buf_rdpos & BUF_MASK) == 0)
    pi_read(dev_addr + buf_rdpos, buf, BUF_SIZE);
  return buf[buf_rdpos++ & BUF_MASK];
}

static int get_bits(int num)
{
  if (bits_have < num) {
    bits_next |= get() << bits_have;
    bits_have += 8;
  }

  if (bits_have < num) {
    bits_next |= get() << bits_have;
    bits_have += 8;
  }

  int ret = bits_next & ((1u << num) - 1);
  bits_next >>= num;
  bits_have -= num;

  return ret;
}

static inline void discard_bits(void)
{
  bits_have = 0;
  bits_next = 0;
}

static inline void put(int c)
{
  wnd[wnd_wrpos++ & WND_MASK] = c;
}

static inline void copy(int len, int dst)
{
  uint32_t p = wnd_wrpos - dst;
  for (int i = 0; i < len; i++)
    put(wnd[p++ & WND_MASK]);
}

static void huf_gen(struct huf *huf, int max_len, int num, int16_t *len)
{
  int val = 0;
  int idx = 0;
  int len_count[16] = {0};

  for (int i = 0; i < num; i++)
    if (len[i] != 0)
      len_count[len[i]]++;

  for (int i = 0; i < max_len; i++) {
    val = (val + len_count[i]) << 1;
    huf->max[i] = val;
    huf->idx[i] = idx - huf->max[i];
    idx += len_count[i + 1];
  }

  for (int i = 0; i < num; i++)
    if (len[i] != 0)
      huf->tbl[huf->idx[len[i] - 1] + huf->max[len[i] - 1]++] = i;
}

static int huf_get(struct huf *huf)
{
  int val = 0;
  int len = 0;

  do {
    val = (val << 1) | get_bits(1);
  } while (val >= huf->max[len++]);

  return huf->tbl[huf->idx[len - 1] + val];
}

static void gen_fix_huf(void)
{
  int16_t len[288];

  for (int i = 0; i < 144; i++)
    len[i] = 8;
  for (int i = 144; i < 256; i++)
    len[i] = 9;
  for (int i = 256; i < 280; i++)
    len[i] = 7;
  for (int i = 280; i < 288; i++)
    len[i] = 8;
  huf_gen(&lit_huf, 9, 288, len);

  for (int i = 0; i < 30; i++)
    len[i] = 5;
  huf_gen(&dst_huf, 5, 30, len);
}

static void gen_dyn_huf(void)
{
  int16_t     len[320];
  uint16_t    len_max[7];
  int16_t     len_idx[7];
  int16_t     len_tbl[19];
  struct huf  len_huf =
  {
    len_max,
    len_idx,
    len_tbl,
  };

  int nlit = get_bits(5) + 257;
  int ndst = get_bits(5) + 1;
  int nlen = get_bits(4) + 4;

  static const char co[] =
  {
    16, 17, 18,  0,  8,  7,  9,  6,
    10,  5, 11,  4, 12,  3, 13,  2,
    14,  1, 15,
  };

  for (int i = 0; i < 19; i++) {
    int j = co[i];
    if (i < nlen)
      len[j] = get_bits(3);
    else
      len[j] = 0;
  }

  huf_gen(&len_huf, 7, 19, len);

  int num = 0;
  int pre = 0;
  while (num < nlit + ndst) {
    int sym = huf_get(&len_huf);
    if (sym < 16) {
      len[num++] = sym;
      pre = sym;
    }
    else if (sym == 16) {
      int n = 3 + get_bits(2);
      for (int j = 0; j < n; j++)
        len[num++] = pre;
    }
    else {
      int n;
      if (sym == 17)
        n = 3 + get_bits(3);
      else
        n = 11 + get_bits(7);
      for (int j = 0; j < n; j++)
        len[num++] = 0;
    }
  }

  huf_gen(&lit_huf, 15, nlit, &len[0]);
  huf_gen(&dst_huf, 15, ndst, &len[nlit]);
}

static void next_block(void)
{
  block_last = get_bits(1);
  block_type = get_bits(2);
  block_fin = 0;

  switch (block_type) {
    case 0:
      discard_bits();
      block_len = get();
      block_len |= get() << 8;
      get();
      get();
      break;
    case 1:
      block_len = 0;
      gen_fix_huf();
      break;
    case 2:
      block_len = 0;
      gen_dyn_huf();
      break;
  }
}

static void store_cont(void)
{
  int rem = block_len < WND_SIZE ? block_len : WND_SIZE;
  block_len -= rem;
  while (rem-- != 0)
    put(get());
  if (block_len == 0)
    block_fin = 1;
}

static void compr_cont(void)
{
  int rem = WND_SIZE;

  for (;;) {
    while (block_len == 0) {
      int sym = huf_get(&lit_huf);
      if (sym < 256) {
        put(sym);
        if (--rem == 0)
          return;
      }
      else if (sym < 257) {
        block_fin = 1;
        return;
      }
      else {
        if (sym < 265)
          block_len = sym - 254;
        else if (sym < 285) {
          int n = (sym - 261) >> 2;
          block_len = 3 + ((4 + ((sym - 261) & 3)) << n) + get_bits(n);
        }
        else
          block_len = 258;

        sym = huf_get(&dst_huf);
        if (sym < 4)
          block_dst = 1 + sym;
        else {
          int n = (sym >> 1) - 1;
          block_dst = 1 + ((2 + (sym & 1)) << n) + get_bits(n);
        }
      }
    }

    if (block_len < rem) {
      copy(block_len, block_dst);
      rem -= block_len;
      block_len = 0;
    }
    else {
      copy(rem, block_dst);
      block_len -= rem;
      return;
    }
  }
}

static void block_cont(void)
{
  switch (block_type) {
    case 0:
      store_cont();
    case 1:
    case 2:
      compr_cont();
      break;
  }
}

static void inflate_cont(void)
{
  if (wnd_rdpos == wnd_wrpos) {
    if (block_fin) {
      if (block_last)
        return;
      next_block();
    }
    block_cont();
  }
}

void inflate_begin(uint32_t prom_start)
{
  dev_addr = MIPS_PHYS_TO_KSEG1(0x10000000 + prom_start);
  buf_rdpos = 0;
  wnd_wrpos = 0;
  wnd_rdpos = 0;
  bits_have = 0;
  bits_next = 0;
  block_last = 0;
  block_fin = 1;
  inflate_cont();
}

char inflate_get_byte(void)
{
  uint8_t byte = wnd[wnd_rdpos++ & WND_MASK];
  inflate_cont();
  return byte;
}

void inflate_read(void *dst, uint32_t num)
{
  char *p = dst;
  while (num-- != 0)
    *p++ = inflate_get_byte();
}

void inflate_advance(uint32_t num)
{
  while (num != 0) {
    uint32_t size = wnd_wrpos - wnd_rdpos;
    if (num < size) {
      wnd_rdpos += num;
      num = 0;
    }
    else {
      wnd_rdpos += size;
      num -= size;
      inflate_cont();
    }
  }
}

int inflate_atend(void)
{
  return wnd_rdpos == wnd_wrpos;
}
