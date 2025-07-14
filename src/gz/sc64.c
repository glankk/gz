#include <stddef.h>
#include <stdint.h>
#include <n64.h>
#include "sc64.h"
#include "iodev.h"
#include "pi.h"
#include "util.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static int      cart_irqf;
static uint32_t cart_lat;
static uint32_t cart_pwd;

static void cart_lock(void)
{
  __osPiGetAccess();

  cart_irqf = set_irqf(0);

  cart_lat = pi_regs.dom1_lat;
  cart_pwd = pi_regs.dom1_pwd;
}

static void cart_unlock(void)
{
  pi_regs.dom1_lat = cart_lat;
  pi_regs.dom1_pwd = cart_pwd;

  __osPiRelAccess();

  set_irqf(cart_irqf);
}

static inline uint32_t reg_rd(int reg)
{
  return __pi_read_raw((uint32_t)&REGS_PTR[reg]);
}

static inline void reg_wr(int reg, uint32_t dat)
{
  return __pi_write_raw((uint32_t)&REGS_PTR[reg], dat);
}

static int cmd_exec(int cmd, uint32_t arg0, uint32_t arg1)
{
  while (reg_rd(REG_STAT) & STAT_BUSY)
    ;

  /* supply arguments */
  reg_wr(REG_DAT0, arg0);
  reg_wr(REG_DAT1, arg1);

  /* execute command */
  reg_wr(REG_CMD, cmd);

  /* wait for completion */
  while (reg_rd(REG_STAT) & STAT_BUSY)
    ;

  if (reg_rd(REG_STAT) & STAT_ERR)
    return -1;
  else
    return 0;
}

static inline unsigned cmd_result(int num)
{
  /* command responses, valid until next command is executed */
  return reg_rd(num ? REG_DAT1 : REG_DAT0);
}

static int probe(void)
{
  int ret;

  cart_lock();

  /* unlock registers */
  reg_wr(REG_KEY, KEY_RST);
  reg_wr(REG_KEY, KEY_UNL);
  reg_wr(REG_KEY, KEY_OCK);

  /* check magic number, if it doesn't match assume we don't have an sc64 */
  if (reg_rd(REG_ID) == SC64_IDENT)
    ret = 0;
  else
    ret = -1;

  cart_unlock();
  return ret;
}

static int disk_init(void)
{
  int ret;

  cart_lock();

  /* init SD card */
  ret = cmd_exec(CMD_SD_OP, 0, SD_INIT);

  cart_unlock();
  return ret;
}

static int disk_read(size_t lba, size_t n_blocks, void *dst)
{
  int ret = 0;
  char *addr = dst;

  cart_lock();

  while (n_blocks != 0) {
    /* transfer as many blocks as possible in one iteration */
    size_t n = MIN(n_blocks, BUFFER_SIZE / 512);

    /* set source lba */
    ret = cmd_exec(CMD_SD_SECT, lba, 0);
    if (ret)
      break;

    /* read SD to SDRAM buffer */
    ret = cmd_exec(CMD_SD_RD, BUFFER_BASE, n);
    if (ret)
      break;

    /* read SDRAM to RDRAM */
    pi_read_locked(BUFFER_BASE, addr, 512 * n);

    addr += 512 * n;
    lba += n;
    n_blocks -= n;
  }

  cart_unlock();
  return ret;
}

static int disk_write(size_t lba, size_t n_blocks, const void *src)
{
  int ret = 0;
  const char *addr = src;

  cart_lock();

  while (n_blocks != 0) {
    /* transfer as many blocks as possible in one iteration */
    size_t n = MIN(n_blocks, BUFFER_SIZE / 512);

    /* write RDRAM to SDRAM */
    pi_write_locked(BUFFER_BASE, addr, 512 * n);

    /* set destination lba */
    ret = cmd_exec(CMD_SD_SECT, lba, 0);
    if (ret)
      break;

    /* write SDRAM buffer to SD */
    ret = cmd_exec(CMD_SD_WR, BUFFER_BASE, n);
    if (ret)
      break;

    addr += 512 * n;
    lba += n;
    n_blocks -= n;
  }

  cart_unlock();
  return ret;
}

/* TODO: usb fifo needs more thorough testing */

static int fifo_poll(void)
{
  int ret = -1;

  cart_lock();

  /* check if amount of waiting data is non-zero */
  if (cmd_exec(CMD_USB_RSTAT, 0, 0) == 0 && cmd_result(1) != 0)
    ret = 0;

  cart_unlock();
  return ret;
}

static int fifo_read(void *dst, size_t n_blocks)
{
  int ret = 0;
  char *addr = dst;

  cart_lock();

  while (n_blocks != 0) {
    /* transfer as many blocks as possible in one iteration */
    size_t n = MIN(n_blocks, BUFFER_SIZE / 512);

    /* read block into SDRAM */
    ret = cmd_exec(CMD_USB_RD, BUFFER_BASE, 512 * n);
    if (ret)
      break;

    /* wait for completion */
    do {
      ret = cmd_exec(CMD_USB_RSTAT, 0, 0);
      if (ret)
        goto err;
    } while (cmd_result(0) & USB_RSTAT_BUSY);

    /* read to RDRAM */
    pi_read_locked(BUFFER_BASE, addr, 512 * n);

    addr += 512 * n;
    n_blocks -= n;
  }

err:
  cart_unlock();
  return ret;
}

static int fifo_write(const void *src, size_t n_blocks)
{
  int ret = 0;
  const char *addr = src;

  cart_lock();

  while (n_blocks != 0) {
    /* transfer as many blocks as possible in one iteration */
    size_t n = MIN(n_blocks, BUFFER_SIZE / 512);

    /* write to SDRAM */
    pi_write_locked(BUFFER_BASE, addr, 512 * n);

    /* transmit block from SDRAM */
    ret = cmd_exec(CMD_USB_WR, BUFFER_BASE, 512 * n);
    if (ret)
      break;

    /* wait for completion */
    do {
      ret = cmd_exec(CMD_USB_WSTAT, 0, 0);
      if (ret)
        goto err;
    } while (cmd_result(0) & USB_WSTAT_BUSY);

    addr += 512 * n;
    n_blocks -= n;
  }

err:
  cart_unlock();
  return ret;
}

struct iodev sc64 =
{
  .probe      = probe,

  .disk_init  = disk_init,
  .disk_read  = disk_read,
  .disk_write = disk_write,

  .fifo_poll  = fifo_poll,
  .fifo_read  = fifo_read,
  .fifo_write = fifo_write,
};
