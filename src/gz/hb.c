#include <stdint.h>
#include <errno.h>
#include <mips.h>
#include "hb.h"

/*  Homeboy devices are currently only implemented on systems that don't
 *  emulate the CPU cache. No flushing/invalidating is done.
 */

int hb_check(void)
{
  if (hb_regs.key == 0x1234)
    return 0;
  else
    return -1;
}

int hb_sd_init(void)
{
  if (hb_check() == -1)
    return -1;
  hb_regs.status = HB_STATUS_SD_INIT;
  while (hb_regs.status & HB_STATUS_SD_BUSY)
    ;
  uint32_t status = hb_regs.status;
  if ((status & HB_STATUS_SD_READY) && (status & HB_STATUS_SD_INSERTED))
    return 0;
  else
    return -1;
}

int hb_sd_read(uint32_t lba, uint32_t n_blocks, void *dst)
{
  if (hb_check() == -1)
    return -1;
  hb_regs.sd_dram_addr = MIPS_KSEG0_TO_PHYS(dst);
  hb_regs.sd_n_blocks = n_blocks;
  hb_regs.sd_read_lba = lba;
  while (hb_regs.status & HB_STATUS_SD_BUSY)
    ;
  if (hb_regs.status & HB_STATUS_ERROR)
    return -1;
  else
    return 0;
}

int hb_sd_write(uint32_t lba, uint32_t n_blocks, void *src)
{
  if (hb_check() == -1)
    return -1;
  if (src) {
    hb_regs.sd_dram_addr = MIPS_KSEG0_TO_PHYS(src);
    hb_regs.sd_n_blocks = n_blocks;
    hb_regs.sd_write_lba = lba;
    while (hb_regs.status & HB_STATUS_SD_BUSY)
      ;
    if (hb_regs.status & HB_STATUS_ERROR)
      return -1;
    else
      return 0;
  }
  else {
    char data[0x200] = {0};
    while (n_blocks--)
      if (hb_sd_write(lba++, 1, data))
        return -1;
    return 0;
  }
}

int hb_reset(uint32_t dram_save_addr, uint32_t dram_save_len)
{
  if (hb_check() == -1)
    return -1;
  hb_regs.dram_save_addr = dram_save_addr;
  hb_regs.dram_save_len = dram_save_len;
  hb_regs.status = HB_STATUS_RESET;
  return 0;
}

int hb_get_timebase(uint32_t *hi, uint32_t *lo)
{
  if (hb_check() == -1)
    return -1;
  if (hi)
    *hi = hb_regs.timebase_hi;
  if (lo)
    *lo = hb_regs.timebase_lo;
  return 0;
}

int hb_get_timebase64(uint64_t *tb)
{
  uint32_t hi;
  uint32_t lo;
  if (hb_get_timebase(&hi, &lo) == -1)
    return -1;
  if (tb)
    *tb = (((uint64_t)hi << 32) | (uint64_t)lo);
  return 0;
}
