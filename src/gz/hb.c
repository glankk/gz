#include <stdint.h>
#include <errno.h>
#include <mips.h>
#include "hb.h"

/*  Homeboy devices are currently only implemented on systems that don't
 *  emulate the CPU cache. No flushing/invalidating is done.
 */

int hb_sd_init(void)
{
  if (hb_regs.key != 0x1234)
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
  if (hb_regs.key != 0x1234)
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
  if (hb_regs.key != 0x1234)
    return -1;
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

int hb_reset(uint32_t dram_save_addr, uint32_t dram_save_len)
{
  if (hb_regs.key != 0x1234)
    return -1;
  hb_regs.dram_save_addr = dram_save_addr;
  hb_regs.dram_save_len = dram_save_len;
  hb_regs.status = HB_STATUS_RESET;
  return 0;
}
