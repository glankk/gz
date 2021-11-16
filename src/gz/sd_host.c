#include <stddef.h>
#include <stdint.h>
#include "io.h"
#include "sd.h"
#include "sd_host.h"

static uint8_t crc7(void *data, int size)
{
  uint8_t *p = data;
  uint8_t crc = 0;

  while (size != 0) {
    crc = crc ^ *p++;
    for (int i = 0; i < 8; i++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x12;
      else
        crc = (crc << 1);
    }
    size--;
  }

  return crc | 1;
}

static void crc16_wide(const void *data, int size, uint16_t (*crc_buf)[4])
{
#if defined(__mips) && __mips >= 3
  int crc;
  int t0;
  int t1;

  __asm__
  (
    "  dli     %[crc], 0;"
    "  beq     %[size], $zero, 1f;"

    "  addu    %[size], %[data], %[size];"
    "0:"
    "  lbu     %[t1], 0(%[data]);"
    "  lbu     %[t0], 1(%[data]);"
    "  sll     %[t1], %[t1], 8;"
    "  or      %[t0], %[t0], %[t1];"

    "  dsrl32  %[t1], %[crc], 16;"
    "  xor     %[t1], %[t1], %[t0];"

    "  dsll    %[t0], %[t1], 28;"
    "  daddu   %[t0], %[t0], %[t1];"
    "  dsll    %[t0], %[t0], 20;"
    "  daddu   %[t0], %[t0], %[t1];"
    "  dsll    %[crc], %[crc], 16;"

    "  addiu   %[data], %[data], 2;"
    "  xor     %[crc], %[crc], %[t0];"

    "  bne     %[data], %[size], 0b;"
    "1:"

    /* big endian byte order is assumed */
    "  sh      %[crc], 6 + %[crc_buf];"
    "  dsrl    %[crc], %[crc], 16;"
    "  sh      %[crc], 4 + %[crc_buf];"
    "  dsrl    %[crc], %[crc], 16;"
    "  sh      %[crc], 2 + %[crc_buf];"
    "  dsrl    %[crc], %[crc], 16;"
    "  sh      %[crc], 0 + %[crc_buf];"

    : [data]    "+r"(data),
      [size]    "+r"(size),
      [crc_buf] "=m"(*crc_buf),
      [crc]     "=r"(crc),
      [t0]      "=r"(t0),
      [t1]      "=r"(t1)
  );
#else
  /* CRC-16 CCITT/XMODEM (0x1021) stretched to 64 bits */
  const uint64_t poly = 0x0001000000100001;
  uint64_t crc = 0;
  uint64_t x;
  const uint8_t *p = data;

  for (int i = 0; i < size; i += 2) {
    x = (p[i] << 8) | p[i + 1];
    x = (crc >> 48) ^ x;
    x = x * poly;
    crc = (crc << 16) ^ x;
  }

  /* store result in big endian byte order */
  uint8_t *crc_p = (void *)*crc_buf;
  for (int i = 0; i < 8; i++) {
    *crc_p++ = crc >> 56;
    crc = crc << 8;
  }
#endif
}

static int cs_err(uint32_t cs)
{
  if ((cs & CS_ERR_BITS) == 0)
    return 0;
  else if (cs & CS_CC_ERROR)
    return -SD_ERR_CC;
  else if (cs & CS_CARD_ECC_FAILED)
    return -SD_ERR_ECC;
  else if (cs & CS_ILLEGAL_COMMAND)
    return -SD_ERR_ILCMD;
  else if (cs & CS_COM_CRC_ERROR)
    return -SD_ERR_CRC;
  else if (cs & (CS_WP_ERASE_SKIP | CS_CSD_OVERWRITE |
                 CS_LOCK_UNLOCK_FAILED | CS_WP_VIOLATION))
    return -SD_ERR_WPVIOL;
  else if (cs & CS_ERASE_SEQ_ERROR)
    return -SD_ERR_ERSEQ;
  else if (cs & (CS_BLOCK_LEN_ERROR | CS_ERASE_PARAM))
    return -SD_ERR_PARAM;
  else if (cs & CS_ADDRESS_ERROR)
    return -SD_ERR_ADDR;
  else if (cs & CS_OUT_OF_RANGE)
    return -SD_ERR_RANGE;
  else
    return -SD_ERR_GEN;
}

static int wr_tok_err(int tok)
{
  if (tok == DAT_RESP_OK)
    return 0;
  else if (tok == DAT_RESP_CRC_ERR)
    return -SD_ERR_CRC;
  else if (tok == DAT_RESP_WR_ERR)
    return -SD_ERR_CC;
  else
    return -SD_ERR_GEN;
}

static int spi_r1_err(int r1)
{
  if ((r1 & SPI_R1_ERR_BITS) == 0)
    return 0;
  else if (r1 & SPI_R1_ILLEGAL_CMD)
    return -SD_ERR_ILCMD;
  else if (r1 & SPI_R1_CRC_ERR)
    return -SD_ERR_CRC;
  else if (r1 & SPI_R1_ERASE_SEQ_ERR)
    return -SD_ERR_ERSEQ;
  else if (r1 & SPI_R1_ADDR_ERR)
    return -SD_ERR_ADDR;
  else if (r1 & SPI_R1_PARAM_ERR)
    return -SD_ERR_PARAM;
  else
    return -SD_ERR_GEN;
}

static int spi_rd_tok_err(int tok)
{
  if (tok & SPI_BLK_RANGE_ERR)
    return -SD_ERR_RANGE;
  else if (tok & SPI_BLK_ECC_ERR)
    return -SD_ERR_ECC;
  else if (tok & SPI_BLK_CC_ERR)
    return -SD_ERR_CC;
  else
    return -SD_ERR_GEN;
}

static void set_spd(struct sd_host *host, int spd)
{
  /* provide an 8 clock period before switching */
  if (host->proto == SD_PROTO_SDBUS)
    host->dat_tx_clk(0xF, 8);
  else /* host->proto == SD_PROTO_SPIBUS */
    host->spi_io(0xFF);

  host->set_spd(spd);
}

static int card_cmd_sd(struct sd_host *host, int cmd, const uint8_t *tx_buf,
                       uint8_t *rx_buf)
{
  int resp_type = sd_resp_type(cmd);
  int resp_size = sd_resp_size(resp_type);

  /* send command */
  host->cmd_tx_buf(tx_buf, 7);

  if (resp_size != 0) {
    /* wait for response */
    rx_buf[0] = 0xFF;
    for (int i = 0; ; i++) {
      if (i > 64)
        return -SD_ERR_TIMEOUT;

      rx_buf[0] = (rx_buf[0] << 1) | host->cmd_rx();

      /* start bit, direction bit card -> host */
      if ((rx_buf[0] & 0xC0) == 0x00)
        break;
    }

    /* receive response */
    host->cmd_rx_buf(&rx_buf[1], resp_size - 1);
  }

  if (resp_type != 0 && resp_type != R3) {
    /* verify response crc */
    uint8_t crc;
    if (resp_type == R2)
      crc = crc7(&rx_buf[1], resp_size - 2);
    else
      crc = crc7(&rx_buf[0], resp_size - 1);

    if (rx_buf[resp_size - 1] != crc)
      return -SD_ERR_CRC;
  }

  /* check respone status */
  if (resp_type == R1)
    return cs_err(sd_r1_cs(rx_buf));
  else
    return 0;
}

static int card_cmd_spi(struct sd_host *host, int cmd, const uint8_t *tx_buf,
                        uint8_t *rx_buf)
{
  int resp_type = spi_resp_type(cmd);
  int resp_size = spi_resp_size(resp_type);

  /* send command */
  host->spi_tx_buf(tx_buf, 7);

  if (cmd == STOP_TRANSMISSION) {
    /* send 8 clocks to allow the transmission to end */
    host->spi_io(0xFF);
  }

  if (resp_size != 0) {
    /* wait for response */
    for (int i = 0; ; i++) {
      if (i > 8)
        return -SD_ERR_TIMEOUT;

      rx_buf[0] = host->spi_io(0xFF);
      if ((rx_buf[0] & SPI_R1_ZERO) == 0)
        break;
    }

    /* receive response */
    host->spi_rx_buf(&rx_buf[1], resp_size - 1);
  }

  /* check respone status */
  if (resp_type != 0)
    return spi_r1_err(rx_buf[0]);
  else
    return 0;
}

static int card_cmd(struct sd_host *host, int cmd, uint32_t arg, void *resp)
{
  uint8_t tx_buf[17];
  uint8_t *rx_buf;

  if (resp != NULL)
    rx_buf = resp;
  else
    rx_buf = tx_buf;

  /* 8 clocks to allow the previous operation to complete */
  tx_buf[0] = 0xFF;
  /* start bit, direction bit host -> card, cmd */
  tx_buf[1] = 0x40 | cmd;
  /* argument */
  tx_buf[2] = arg >> 24;
  tx_buf[3] = arg >> 16;
  tx_buf[4] = arg >> 8;
  tx_buf[5] = arg >> 0;
  /* crc, end bit (tacked on by crc7) */
  tx_buf[6] = crc7(&tx_buf[1], 5);

  if (host->proto == SD_PROTO_SDBUS)
    return card_cmd_sd(host, cmd, tx_buf, rx_buf);
  else /* host->proto == SD_PROTO_SPIBUS */
    return card_cmd_spi(host, cmd, tx_buf, rx_buf);
}

static int rx_blk_sd(struct sd_host *host, void *buf, size_t blk_size)
{
  /* wait for start bit on DAT0-DAT3 */
  unsigned timeout = msec_from_now(100);
  while (host->dat_rx() != 0x0) {
    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  /* receive data block */
  host->dat_rx_buf(buf, blk_size);

  /* receive crc */
  uint16_t rx_crc[4];
  host->dat_rx_buf(rx_crc, sizeof(rx_crc));

  /* compute crc */
  uint16_t crc[4];
  crc16_wide(buf, blk_size, &crc);

  /* verify crc */
  for (int i = 0; i < 4; i++) {
    if (rx_crc[i] != crc[i])
      return -SD_ERR_CRC;
  }

  return 0;
}

static int rx_blk_spi(struct sd_host *host, void *buf, size_t blk_size)
{
  /* wait for data block token */
  unsigned timeout = msec_from_now(100);
  for (;;) {
    int tok = host->spi_io(0xFF);

    if (tok == SPI_BLK_START)
      break;
    else if ((tok & 0xF0) == 0x00)
      return spi_rd_tok_err(tok);

    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  /* receive data block */
  host->spi_rx_buf(buf, blk_size);

  /* receive crc */
  uint16_t rx_crc;
  host->spi_rx_buf(&rx_crc, sizeof(rx_crc));

  /* skip crc verification for SPI Bus */
  return 0;
}

static int rx_blk(struct sd_host *host, void *buf, size_t blk_size)
{
  if (host->proto == SD_PROTO_SDBUS)
    return rx_blk_sd(host, buf, blk_size);
  else /* host->proto == SD_PROTO_SPIBUS */
    return rx_blk_spi(host, buf, blk_size);
}

static int tx_blk_sd(struct sd_host *host, const void *buf, size_t blk_size)
{
  /* compute crc */
  uint16_t crc[4];
  if (buf != NULL)
    crc16_wide(buf, blk_size, &crc);
  else
    crc[0] = crc[1] = crc[2] = crc[3] = 0;

  /* wait for busy signal on DAT0-DAT3 to be released */
  unsigned timeout = msec_from_now(250);
  while (host->dat_rx() != 0xF) {
    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  /* transmit start bit on DAT0-DAT3 */
  host->dat_tx(0xF);
  host->dat_tx(0x0);
  /* at least one card seems to need the start and end tokens to be on
   * byte boundaries, hence the two clocks */

  /* transmit data */
  if (buf != NULL)
    host->dat_tx_buf(buf, blk_size);
  else
    host->dat_tx_clk(0x0, blk_size * 2);

  /* transmit crc */
  host->dat_tx_buf(crc, sizeof(crc));

  /* transmit end bit on DAT0-DAT3 */
  host->dat_tx(0xF);
  host->dat_tx(0xF);

  /* wait for start bit on DAT0 */
  for (int i = 0; ; i++) {
    if (i > 64)
      return -SD_ERR_TIMEOUT;

    if ((host->dat_rx() & 1) == 0)
      break;
  }

  /* receive response token on DAT0 */
  int tok = 0;
  for (int i = 0; i < 4; i++)
    tok = (tok << 1) | (host->dat_rx() & 1);

  /* check response token */
  return wr_tok_err(tok);
}

static int tx_blk_spi(struct sd_host *host, const void *buf, size_t blk_size)
{
  int tok;

  /* wait for free receive buffer */
  unsigned timeout = msec_from_now(250);
  while (host->spi_io(0xFF) != 0xFF) {
    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  /* transmit start block token */
  host->spi_io(SPI_MBW_START);

  /* transmit data */
  if (buf != NULL)
    host->spi_tx_buf(buf, blk_size);
  else
    host->spi_tx_clk(0, blk_size * 8);

  /* transmit bogus crc (ignored by card) */
  host->spi_io(0xFF);
  host->spi_io(0xFF);

  /* receive and check data response token */
  for (int i = 0; ; i++) {
    if (i > 8)
      return -SD_ERR_TIMEOUT;

    tok = host->spi_io(0xFF);

    if ((tok & 0x11) == 0x01)
      return wr_tok_err(tok & 0x1F);
  }
}

static int tx_blk(struct sd_host *host, const void *buf, size_t blk_size)
{
  if (host->proto == SD_PROTO_SDBUS)
    return tx_blk_sd(host, buf, blk_size);
  else /* host->proto == SD_PROTO_SPIBUS */
    return tx_blk_spi(host, buf, blk_size);
}

static int stop_rd(struct sd_host *host)
{
  return card_cmd(host, STOP_TRANSMISSION, 0, NULL);
}

static int stop_wr_sd(struct sd_host *host)
{
  int ret = card_cmd(host, STOP_TRANSMISSION, 0, NULL);
  if (ret != 0)
    return ret;

  /* wait for busy signal on DAT0-DAT3 to be released */
  unsigned timeout = msec_from_now(500);
  while (host->dat_rx() != 0xF) {
    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  return 0;
}

static int stop_wr_spi(struct sd_host *host)
{
  unsigned timeout;

  /* wait for busy signal to be released */
  timeout = msec_from_now(250);
  while (host->spi_io(0xFF) != 0xFF) {
    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  /* send stop transmission token */
  host->spi_io(SPI_MBW_STOP);

  /* send 8 clocks to allow the transmission to end */
  host->spi_io(0xFF);

  /* and wait again */
  timeout = msec_from_now(500);
  while (host->spi_io(0xFF) != 0xFF) {
    if (clock_after(timeout))
      return -SD_ERR_TIMEOUT;
  }

  return 0;
}

static int stop_wr(struct sd_host *host)
{
  if (host->proto == SD_PROTO_SDBUS)
    return stop_wr_sd(host);
  else /* host->proto == SD_PROTO_SPIBUS */
    return stop_wr_spi(host);
}

static int rx_mblk(struct sd_host *host, void *buf, size_t blk_size,
                   size_t n_blk)
{
  int ret;

  if (host->rx_mblk) {
    ret = host->rx_mblk(buf, blk_size, n_blk);
  }
  else {
    char *p = buf;

    for (size_t i = 0; i < n_blk; i++) {
      ret = rx_blk(host, p, blk_size);
      if (ret != 0)
        break;

      if (p != NULL)
        p += blk_size;
    }
  }

  return ret;
}

static int tx_mblk(struct sd_host *host, const void *buf, size_t blk_size,
                   size_t n_blk)
{
  int ret;

  const char *p = buf;

  for (size_t i = 0; i < n_blk; i++) {
    ret = tx_blk(host, p, blk_size);
    if (ret != 0)
      break;

    if (p != NULL)
      p += blk_size;
  }

  return ret;
}

int sd_init(struct sd_host *host)
{
  int ret;
  uint8_t dat[64];

  /* acquire the host device */
  host->lock();

  /* reset card */
  host->card_type = 0;
  set_spd(host, 0);
  /* provide 80 clocks for card initialization */
  if (host->proto == SD_PROTO_SDBUS) {
    host->dat_tx_clk(0xF, 80);
  }
  else { /* host->proto == SD_PROTO_SPIBUS */
    host->spi_ss(0);
    host->spi_tx_clk(0x1, 80);
    host->spi_ss(1);
  }
  ret = card_cmd(host, GO_IDLE_STATE, 0, NULL);
  if (ret != 0)
    goto exit;

  /* 2.7V - 3.6V, check pattern 0b10101010 */
  ret = card_cmd(host, SEND_IF_COND, IF_COND_VHS_VDD1 | 0xAA, NULL);
  if (ret == 0)
    host->card_type |= SD_CARD_V2;
  else if (ret != -SD_ERR_ILCMD)
    goto exit;

  unsigned timeout = msec_from_now(1000);
  for (;;) {
    ret = card_cmd(host, APP_CMD, 0, NULL);
    if (ret != 0)
      goto exit;

    /* 3.2V - 3.4V */
    uint32_t op_cond = OCR_3V2_3V3 | OCR_3V3_3V4;
    if (host->card_type & SD_CARD_V2) {
      /* Host Capacity Support */
      op_cond = op_cond | OCR_CCS;
    }
    ret = card_cmd(host, SD_SEND_OP_COND, op_cond, dat);
    if (ret != 0)
      goto exit;

    if (host->proto == SD_PROTO_SDBUS) {
      uint32_t ocr = r3_ocr(dat);
      /* check card power up status bit */
      if (ocr & OCR_BUSY) {
        if (host->card_type & SD_CARD_V2) {
          /* check Card Capacity Status */
          if (ocr & OCR_CCS)
            host->card_type |= SD_CARD_HC;
        }
        break;
      }
    }
    else { /* host->proto == SD_PROTO_SPIBUS */
      /* check in_idle_state */
      if ((dat[0] & SPI_R1_IN_IDLE_STATE) == 0)
        break;
    }

    if (clock_after(timeout)) {
      ret = -SD_ERR_TIMEOUT;
      goto exit;
    }
  }

  /* switch to Default Speed (25MHz) */
  set_spd(host, 25);

  if (host->proto == SD_PROTO_SDBUS) {
    /* get rca and select card */
    ret = card_cmd(host, ALL_SEND_CID, 0, NULL);
    if (ret != 0)
      goto exit;
    ret = card_cmd(host, SEND_RELATIVE_ADDR, 0, dat);
    if (ret != 0)
      goto exit;
    uint32_t rca = r6_rca(dat);
    ret = card_cmd(host, SELECT_CARD, rca << 16, NULL);
    if (ret != 0)
      goto exit;

    /* select wide (4 bit) data bus */
    ret = card_cmd(host, APP_CMD, rca << 16, NULL);
    if (ret != 0)
      goto exit;
    ret = card_cmd(host, SET_BUS_WIDTH, 2, NULL);
    if (ret != 0)
      goto exit;
  }
  else { /* host->proto == SD_PROTO_SPIBUS */
    if (host->card_type & SD_CARD_V2) {
      /* check Card Capacity Status */
      ret = card_cmd(host, READ_OCR, 0, dat);
      if (ret != 0)
        goto exit;

      if (r3_ocr(dat) & OCR_CCS)
        host->card_type |= SD_CARD_HC;
    }
  }

  /* try to switch to High Speed (50MHz) */
  ret = card_cmd(host, SWITCH_FUNC, SWFN_SET | 0xFFFFF1, NULL);
  if (ret == 0) {
    ret = rx_blk(host, dat, SWFN_DAT_SIZE);
    if (ret != 0)
      goto exit;

    if (swfn_sel(dat, 1) == 1)
      set_spd(host, 50);
  }
  else if (ret != -SD_ERR_ILCMD)
    goto exit;

  ret = 0;

exit:
  /* release the host device */
  host->unlock();

  return ret;
}

int sd_read(struct sd_host *host, size_t lba, void *dst, size_t n_blk)
{
  int ret;
  const size_t blk_size = 512;

  /* acquire the host device */
  host->lock();

  /* send read command */
  if (host->card_type & SD_CARD_HC)
    ret = card_cmd(host, READ_MULTIPLE_BLOCK, lba, NULL);
  else
    ret = card_cmd(host, READ_MULTIPLE_BLOCK, lba * blk_size, NULL);
  if (ret != 0)
    goto exit;

  /* receive blocks */
  ret = rx_mblk(host, dst, blk_size, n_blk);

  /* stop transmission */
  if (ret == 0)
    ret = stop_rd(host);
  else if (ret != -SD_ERR_TIMEOUT)
    stop_rd(host);

exit:
  /* release the host device */
  host->unlock();

  return ret;
}

int sd_write(struct sd_host *host, size_t lba, const void *src, size_t n_blk)
{
  int ret;
  const size_t blk_size = 512;

  /* acquire the host device */
  host->lock();

  /* send write command */
  if (host->card_type & SD_CARD_HC)
    ret = card_cmd(host, WRITE_MULTIPLE_BLOCK, lba, NULL);
  else
    ret = card_cmd(host, WRITE_MULTIPLE_BLOCK, lba * blk_size, NULL);
  if (ret != 0)
    goto exit;

  /* transmit blocks */
  ret = tx_mblk(host, src, blk_size, n_blk);

  /* stop transmission */
  if (ret == 0)
    ret = stop_wr(host);
  else if (ret != -SD_ERR_TIMEOUT)
    stop_wr(host);

exit:
  /* release the host device */
  host->unlock();

  return ret;
}
