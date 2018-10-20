#include <stdint.h>
#include <mips.h>
#include "util.h"

struct yaz0
{
  char              magic[4];                 /* 0x0000 */
  uint32_t          size;                     /* 0x0004 */
  char              padding[8];               /* 0x0008 */
  uint8_t           code[];                   /* 0x0010 */
};

static struct yaz0 *yaz0;
static uint32_t     yaz0_cp;
static uint32_t     yaz0_op;
static uint8_t      yaz0_group_header;
static uint32_t     yaz0_group_pos;
static uint32_t     yaz0_chunk_end;
_Alignas(0x10)
static uint8_t      yaz0_code_buf[0x400];
static uint8_t      yaz0_buf[0x1000];


static uint8_t yaz0_get_code(void)
{
  if ((yaz0_cp & 0x3FF) == 0) {
    _Bool ie = enter_dma_section();
    dma_read(yaz0_code_buf, (uint32_t)&yaz0->code[yaz0_cp], 0x400);
    set_int(ie);
  }
  return yaz0_code_buf[yaz0_cp++ & 0x3FF];
}

static void yaz0_continue(void)
{
  if (yaz0_op == yaz0_chunk_end) {
    ++yaz0_group_pos;
    if (yaz0_group_pos == 8) {
      yaz0_group_pos = 0;
      yaz0_group_header = yaz0_get_code();
    }
    if (yaz0_group_header & 0x80) {
      yaz0_buf[yaz0_op & 0xFFF] = yaz0_get_code();
      yaz0_chunk_end = yaz0_op + 1;
    }
    else {
      uint32_t chunk_dist;
      uint32_t chunk_size;
      uint8_t c = yaz0_get_code();
      if (c & 0xF0) {
        chunk_size = (c >> 4) + 0x02;
        chunk_dist = ((c & 0x0F) << 8);
        chunk_dist |= yaz0_get_code();
      }
      else {
        chunk_dist = (c << 8);
        chunk_dist |= yaz0_get_code();
        chunk_size = yaz0_get_code() + 0x12;
      }
      uint32_t src = yaz0_op - 1 - chunk_dist;
      uint32_t dst = yaz0_op;
      for (uint32_t i = 0; i < chunk_size; ++i)
        yaz0_buf[dst++ & 0xFFF] = yaz0_buf[src++ & 0xFFF];
      yaz0_chunk_end = yaz0_op + chunk_size;
    }
    yaz0_group_header <<= 1;
  }
}

void yaz0_begin(uint32_t prom_start)
{
  yaz0 = (void*)MIPS_PHYS_TO_KSEG1(0x10000000 + prom_start);
  yaz0_cp = 0;
  yaz0_op = 0;
  yaz0_group_pos = 7;
  yaz0_chunk_end = 0;
  yaz0_continue();
}

char yaz0_get_byte(void)
{
  uint8_t byte = yaz0_buf[yaz0_op++ & 0xFFF];
  yaz0_continue();
  return byte;
}

void yaz0_advance(uint32_t n_bytes)
{
  while (n_bytes > 0) {
    uint32_t chunk_bytes_left = yaz0_chunk_end - yaz0_op;
    if (n_bytes >= chunk_bytes_left) {
      yaz0_op = yaz0_chunk_end;
      n_bytes -= chunk_bytes_left;
      yaz0_continue();
    }
    else {
      yaz0_op += n_bytes;
      n_bytes = 0;
    }
  }
}
