#include <stddef.h>
#include <stdint.h>
#include <n64.h>
#include "ed64_l.h"
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

  cart_lat = pi_regs.dom2_lat;
  cart_pwd = pi_regs.dom2_pwd;
}

static void cart_lock(void)
{
  cart_lock_safe();

  pi_regs.dom2_lat = 4;
  pi_regs.dom2_pwd = 12;
}

static void cart_unlock(void)
{
  pi_regs.dom2_lat = cart_lat;
  pi_regs.dom2_pwd = cart_pwd;

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

static inline void spi_tx(uint8_t dat)
{
  reg_wr(REG_SPI, dat);

  while (reg_rd(REG_STATUS) & STATUS_SPI)
    ;
}

static inline uint8_t spi_io(uint8_t dat)
{
  spi_tx(dat);

  return reg_rd(REG_SPI);
}

static inline uint8_t spi_rx(void)
{
  return spi_io(0xFF);
}

static void sd_set_spd(int spd)
{
  spi_cfg &= ~SPI_SPEED;

  if (spd >= 50)
    spi_cfg |= SPI_SPEED_50;
  else if (spd >= 25)
    spi_cfg |= SPI_SPEED_25;
  else
    spi_cfg |= SPI_SPEED_LO;

  reg_wr(REG_SPI_CFG, spi_cfg);
}

static void sd_spi_ss(int ss)
{
  if (ss)
    spi_cfg &= ~SPI_SS;
  else
    spi_cfg |= SPI_SS;

  reg_wr(REG_SPI_CFG, spi_cfg);
}

static int sd_spi_io(int dat)
{
  return spi_io(dat);
}

static void sd_spi_rx_buf(void *buf, size_t size)
{
  uint8_t *p = buf;

  for (size_t i = 0; i < size; i++)
    *p++ = spi_rx();
}

static void sd_spi_tx_buf(const void *buf, size_t size)
{
  const uint8_t *p = buf;

  for (size_t i = 0; i < size; i++)
    spi_tx(*p++);
}

static void sd_spi_tx_clk(int dat, size_t n_clk)
{
  if (dat & 0x1)
    dat = 0xFF;
  else
    dat = 0x00;

  for (size_t i = 0; i < n_clk / 8; i++)
    spi_tx(dat);
}

static int sd_rx_mblk(void *buf, size_t blk_size, size_t n_blk)
{
  const uint32_t cart_addr = 0xB2000000;

  /* dma to cart */
  reg_wr(REG_DMA_LEN, n_blk - 1);
  reg_wr(REG_DMA_ADDR, cart_addr >> 11);
  reg_wr(REG_DMA_CFG, DMA_SD_TO_RAM);
  while (reg_rd(REG_STATUS) & STATUS_DMA_BUSY)
    ;

  /* check for dma timeout */
  if (reg_rd(REG_STATUS) & STATUS_DMA_TOUT)
    return -SD_ERR_TIMEOUT;

  /* copy to ram */
  pi_read_locked(cart_addr, buf, blk_size * n_blk);

  return 0;
}

static struct sd_host sd_host =
{
  .proto      = SD_PROTO_SPIBUS,

  .lock       = cart_lock,
  .unlock     = cart_unlock,
  .set_spd    = sd_set_spd,

  .spi_ss     = sd_spi_ss,
  .spi_io     = sd_spi_io,
  .spi_rx_buf = sd_spi_rx_buf,
  .spi_tx_buf = sd_spi_tx_buf,
  .spi_tx_clk = sd_spi_tx_clk,

  .rx_mblk    = sd_rx_mblk,
};

static int probe(void)
{
  cart_lock_safe();

  /* open registers */
  reg_wr(REG_KEY, 0x1234);

  /* check firmware version */
  uint16_t fw_ver = reg_rd(REG_VER);
  if (fw_ver < 0x0100 || fw_ver >= 0x0116)
    goto nodev;

  /* check spi device */
  /* for a v1 device we expect a write to trigger eight clocks with the MISO
   * line high */
  reg_wr(REG_SPI_CFG, SPI_SPEED_LO | SPI_SS);
  reg_wr(REG_SPI, 0x00);
  for (int i = 0; ; i++) {
    if (i > 32)
      goto nodev;

    if ((reg_rd(REG_STATUS) & STATUS_SPI) == 0)
      break;
  }
  uint16_t dat = reg_rd(REG_SPI);
  if (dat == 0xFF) {
    /* spi seems to work as expected */
    cart_unlock();
    return 0;
  }

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

struct iodev everdrive64_v1 =
{
  .probe      = probe,

  .disk_init  = disk_init,
  .disk_read  = disk_read,
  .disk_write = disk_write,
};
