#include <stddef.h>
#include <stdint.h>
#include <n64.h>
#include "ed64_x.h"
#include "iodev.h"
#include "pi.h"
#include "sd_host.h"
#include "util.h"

static int      cart_irqf;
static uint32_t cart_lat;
static uint32_t cart_pwd;
static uint16_t spi_cfg;

static void cart_lock_safe(void)
{
  __osPiGetAccess();

  cart_irqf = set_irqf(0);

  cart_lat = pi_regs.dom1_lat;
  cart_pwd = pi_regs.dom1_pwd;
}

static void cart_lock(void)
{
  cart_lock_safe();

  pi_regs.dom1_lat = 4;
  pi_regs.dom1_pwd = 12;
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

static inline void spi_nclk(int bitlen)
{
  spi_cfg &= ~SD_CFG_BITLEN;
  spi_cfg |= bitlen;

  reg_wr(REG_SD_STATUS, spi_cfg);
}

static inline void cmd_tx(uint8_t dat)
{
  reg_wr(REG_SD_CMD_WR, dat);

  while (reg_rd(REG_SD_STATUS) & SD_STA_BUSY)
    ;
}

static inline uint8_t cmd_rx(void)
{
  reg_wr(REG_SD_CMD_RD, 0xFF);

  while (reg_rd(REG_SD_STATUS) & SD_STA_BUSY)
    ;

  return reg_rd(REG_SD_CMD_RD);
}

static inline void dat_tx(uint16_t dat)
{
  reg_wr(REG_SD_DAT_WR, dat);

  while (reg_rd(REG_SD_STATUS) & SD_STA_BUSY)
    ;
}

static inline uint16_t dat_rx(void)
{
  reg_wr(REG_SD_DAT_RD, 0xFFFF);

  while (reg_rd(REG_SD_STATUS) & SD_STA_BUSY)
    ;

  return reg_rd(REG_SD_DAT_RD);
}

static void sd_set_spd(int spd)
{
  /* The ED64-X IO don't seem to support Default Speed (25MHz), so I guess
   * we'd better hope that the card supports High Speed (50MHz).
   */
  if (spd >= 25)
    spi_cfg |= SD_CFG_SPD;
  else
    spi_cfg &= ~SD_CFG_SPD;

  reg_wr(REG_SD_STATUS, spi_cfg);
}

static int sd_cmd_rx(void)
{
  spi_nclk(1);

  return cmd_rx() & 0x1;
}

static int sd_dat_rx(void)
{
  spi_nclk(1);

  return dat_rx() & 0xF;
}

static void sd_dat_tx(int dat)
{
  spi_nclk(1);

  dat_tx((dat << 12) | 0x0FFF);
}

static void sd_cmd_rx_buf(void *buf, size_t size)
{
  uint8_t *p = buf;

  spi_nclk(8);

  for (size_t i = 0; i < size; i++)
    *p++ = cmd_rx();
}

static void sd_cmd_tx_buf(const void *buf, size_t size)
{
  const uint8_t *p = buf;

  spi_nclk(8);

  for (size_t i = 0; i < size; i++)
    cmd_tx(*p++);
}

static void sd_dat_rx_buf(void *buf, size_t size)
{
  uint8_t *p = buf;

  spi_nclk(4);

  for (size_t i = 0; i < size / 2; i++) {
    uint16_t dat = dat_rx();
    *p++ = dat >> 8;
    *p++ = dat >> 0;
  }
}

static void sd_dat_tx_buf(const void *buf, size_t size)
{
  const uint8_t *p = buf;

  spi_nclk(4);

  for (size_t i = 0; i < size / 2; i++) {
    uint16_t dat = 0;
    dat = (dat << 8) | *p++;
    dat = (dat << 8) | *p++;
    dat_tx(dat);
  }
}

static void sd_dat_tx_clk(int dat, size_t n_clk)
{
  dat = dat & 0xF;
  dat = (dat << 4) | dat;
  dat = (dat << 8) | dat;

  spi_nclk(4);

  for (size_t i = 0; i < n_clk / 4; i++)
    dat_tx(dat);
}

static int sd_rx_mblk(void *buf, size_t blk_size, size_t n_blk)
{
  const uint32_t cart_addr = 0xB2000000;

  /* dma to cart */
  reg_wr(REG_DMA_ADDR, cart_addr);
  reg_wr(REG_DMA_LEN, n_blk);
  while (reg_rd(REG_DMA_STA) & DMA_STA_BUSY)
    ;

  /* check for dma timeout */
  if (reg_rd(REG_DMA_STA) & DMA_STA_ERROR)
    return -SD_ERR_TIMEOUT;

  /* copy to ram */
  pi_read_locked(cart_addr, buf, blk_size * n_blk);

  return 0;
}

static struct sd_host sd_host =
{
  .proto      = SD_PROTO_SDBUS,

  .lock       = cart_lock,
  .unlock     = cart_unlock,
  .set_spd    = sd_set_spd,

  .cmd_rx     = sd_cmd_rx,
  .dat_rx     = sd_dat_rx,
  .dat_tx     = sd_dat_tx,
  .cmd_rx_buf = sd_cmd_rx_buf,
  .cmd_tx_buf = sd_cmd_tx_buf,
  .dat_rx_buf = sd_dat_rx_buf,
  .dat_tx_buf = sd_dat_tx_buf,
  .dat_tx_clk = sd_dat_tx_clk,

  .rx_mblk    = sd_rx_mblk,
};

static int probe(void)
{
  cart_lock_safe();

  /* open registers */
  reg_wr(REG_KEY, 0xAA55);

  /* check magic number */
  if ((reg_rd(REG_EDID) >> 16) != 0xED64)
    goto nodev;

  cart_unlock();
  return 0;

nodev:
  reg_wr(REG_KEY, 0);
  cart_unlock();
  return -1;
}

static int disk_init(void)
{
  return sd_init(&sd_host);
}

static int disk_read(size_t lba, size_t n_blocks, void *dst)
{
  return sd_read(&sd_host, lba, dst, n_blocks);
}

static int disk_write(size_t lba, size_t n_blocks, const void *src)
{
  return sd_write(&sd_host, lba, src, n_blocks);
}

static int fifo_poll(void)
{
  int ret;

  cart_lock();
  if ((reg_rd(REG_USB_CFG) & (USB_STA_PWR | USB_STA_RXF)) == USB_STA_PWR)
    ret = 1;
  else
    ret = 0;
  cart_unlock();

  return ret;
}

static int fifo_read(void *dst, size_t n_blocks)
{
  const size_t blk_size = 512;

  cart_lock();

  char *p = dst;
  while (n_blocks != 0) {
    /* wait for power on and rx buffer full (PWR high, RXF low) */
    while ((reg_rd(REG_USB_CFG) & (USB_STA_PWR | USB_STA_RXF)) != USB_STA_PWR)
      ;

    /* receive */
    reg_wr(REG_USB_CFG, USB_LE_CFG | USB_LE_CTR | USB_CFG_RD | USB_CFG_ACT);
    while (reg_rd(REG_USB_CFG) & USB_STA_ACT)
      ;

    /* copy from rx buffer */
    reg_wr(REG_USB_CFG, USB_LE_CFG | USB_LE_CTR | USB_CFG_RD);
    pi_read_locked((uint32_t)&REGS_PTR[REG_USB_DAT], p, blk_size);

    p += blk_size;
    n_blocks--;
  }

  cart_unlock();
  return 0;
}

static int fifo_write(const void *src, size_t n_blocks)
{
  const size_t blk_size = 512;

  cart_lock();

  const char *p = src;
  while (n_blocks != 0) {
    /* wait for power on and tx buffer empty (PWR high, TXE low) */
    while ((reg_rd(REG_USB_CFG) & (USB_STA_PWR | USB_STA_TXE)) != USB_STA_PWR)
      ;

    /* copy to tx buffer */
    reg_wr(REG_USB_CFG, USB_LE_CFG | USB_LE_CTR | USB_CFG_WR);
    pi_write_locked((uint32_t)&REGS_PTR[REG_USB_DAT], p, blk_size);

    /* transmit */
    reg_wr(REG_USB_CFG, USB_LE_CFG | USB_LE_CTR | USB_CFG_WR | USB_CFG_ACT);
    while (reg_rd(REG_USB_CFG) & USB_STA_ACT)
      ;

    p += blk_size;
    n_blocks--;
  }

  cart_unlock();
  return 0;
}

struct iodev everdrive64_x =
{
  .probe      = probe,

  .disk_init  = disk_init,
  .disk_read  = disk_read,
  .disk_write = disk_write,

  .fifo_poll  = fifo_poll,
  .fifo_read  = fifo_read,
  .fifo_write = fifo_write,
};
