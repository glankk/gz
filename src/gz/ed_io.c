#include <stddef.h>
#include <stdint.h>
#include "ed.h"

static uint32_t card_type;
static uint32_t spi_cfg;

static uint16_t *crc16_table()
{
  static uint16_t crc_table[256];
  static _Bool generate_table = 1;
  if (generate_table) {
    const uint16_t p = 0x1021;
    for (int i = 0; i < 256; ++i) {
      uint16_t crc = 0;
      uint16_t c = i;
      for (int j = 0; j < 8; ++j) {
        if ((crc ^ (c << 1)) & 0x0100)
          crc = (crc << 1) ^ p;
        else
          crc <<= 1;
        c <<= 1;
      }
      crc_table[i] = crc;
    }
    generate_table = 0;
  }
  return crc_table;
}

static void crc16_wide(void *data, int size, uint16_t *crc)
{
  uint16_t *crc_table = crc16_table();
  uint8_t *p = data;
  uint16_t crc_buf[4] = {0, 0, 0, 0};
  while (size > 0) {
    uint8_t dat[4] = {0, 0, 0, 0};
    /* deserialize lines */
    for (int i = 3; i >= 0; --i) {
      uint8_t d = size > i ? p[i] : 0;
      for (int j = 0; j < 8; ++j) {
        dat[j % 4] >>= 1;
        dat[j % 4] |= (d & 1) << 7;
        d >>= 1;
      }
    }
    for (int i = 0; i < 4; ++i) {
      uint16_t c = crc_buf[i];
      crc_buf[i] = crc_table[(c >> 8) ^ dat[i]] ^ (c << 8);
    }
    p += 4;
    size -= 4;
  }
  /* serialize lines */
  for (int i = 0; i < 4; i++)
    crc[i] = 0;
  for (int i = 0; i < 4 * 16; ++i) {
    crc[3 - i / 16] >>= 1;
    crc[3 - i / 16] |= (crc_buf[i % 4] & 1) << 15;
    crc_buf[i % 4] >>= 1;
  }
}

static uint8_t crc7(void *data, int size)
{
  uint8_t *p = data;
  uint8_t crc = 0;
  while (size-- > 0) {
    crc ^= *p++;
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x12;
      else
        crc = (crc << 1);
    }
  }
  return crc & 0xFE;
}

static int sd_cmd_resp_type(int cmd)
{
  switch (cmd) {
    case SD_CMD_ALL_SEND_CID:
    case SD_CMD_SEND_CSD:           return SD_R2;
    case SD_ACMD_SD_SEND_OP_COND:   return SD_R3;
    case SD_CMD_SEND_RELATIVE_ADDR: return SD_R6;
    case SD_CMD_SEND_IF_COND:       return SD_R7;
    default:                        return SD_R1;
  }
}

enum ed_error ed_sd_cmd(int cmd, uint32_t arg, void *resp_buf)
{
  int resp_type = sd_cmd_resp_type(cmd);
  int resp_size = resp_type == SD_R2 ? 17 : 6;
  /* fill command buffer */
  uint8_t buf[17];
  buf[0] = 0x40 | (cmd & 0x3F);
  buf[1] = (arg >> 24) & 0xFF;
  buf[2] = (arg >> 16) & 0xFF;
  buf[3] = (arg >> 8)  & 0xFF;
  buf[4] = (arg >> 0)  & 0xFF;
  buf[5] = crc7(buf, 5) | 1;
  /* send command */
  ed_spi_mode(0, 1, 1);
  ed_spi_transmit(0xFF);
  for (int i = 0; i < 6; ++i)
    ed_spi_transmit(buf[i]);
  /* wait for response */
  int n = 0;
  uint8_t resp = 0xFF;
  ed_spi_mode(0, 0, 0);
  do {
    if (n++ >= 0x400)
      return ED_ERROR_SD_CMD_TIMEOUT;
    resp = ed_spi_transmit(resp);
  } while (resp & 0xC0);
  /* receive response */
 uint8_t *resp_buf_u8 = resp_buf;
 if (!resp_buf_u8)
    resp_buf_u8 = buf;
  resp_buf_u8[0] = resp;
  ed_spi_mode(0, 0, 1);
  for (int i = 1; i < resp_size; i++)
    resp_buf_u8[i] = ed_spi_transmit(0xFF);
  /* verify crc if applicable */
  if (resp_type != SD_R3) {
    uint8_t crc;
    if (resp_type == SD_R2)
      crc = crc7(&resp_buf_u8[1], resp_size - 2);
    else
      crc = crc7(&resp_buf_u8[0], resp_size - 1);
    if ((resp_buf_u8[resp_size - 1] & ~1) != crc)
      return ED_ERROR_SD_CMD_CRC;
  }
  return ED_ERROR_SUCCESS;
}

enum ed_error ed_sd_init(void)
{
  enum ed_error e;
  static uint8_t resp[17];
  /* initialize spi */
  spi_cfg = (0 << SPI_CFG_SPD0) | (1 << SPI_CFG_SPD1) | (1 << SPI_CFG_SS);
  ed_regs.cfg;
  ed_regs.key = 0x1234;
  ed_regs.cfg;
  ed_regs.spi_cfg = spi_cfg;
  ed_spi_ss(0);
  ed_spi_mode(0, 1, 1);
  ed_spi_speed(SPI_SPEED_INIT);
  /* reset card */
  card_type = 0;
  for (int i = 0; i < 40; ++i)
    ed_spi_transmit(0xFF);
  e = ed_sd_cmd(SD_CMD_GO_IDLE_STATE, 0, NULL);
  for (int i = 0; i < 40; ++i)
    ed_spi_transmit(0xFF);
  /* 2.7-3.6V, check pattern 0b10101010 */
  e = ed_sd_cmd(SD_CMD_SEND_IF_COND, 0x1AA, NULL);
  if (e == ED_ERROR_SD_CMD_CRC)
    return e;
  else if (e == ED_ERROR_SUCCESS)
    card_type |= SD_V2;
  /* else timeout, PLS v2.00 not supported */
  /* do initialization */
  _Bool timeout = 1;
  if (card_type == SD_V2) {
    for (int i = 0; i < 0x400; i++) {
      e = ed_sd_cmd(SD_CMD_APP_CMD, 0, resp);
      if (e)
        return e;
      /* check READY_FOR_DATA (why?) */
      if (!(resp[3] & 1))
        continue;
      /* Host Capacity Support, 3.2-3.3V, 3.3-3.4V */
      e = ed_sd_cmd(SD_ACMD_SD_SEND_OP_COND, 0x40300000, resp);
      if (e)
        return e;
      /* check busy bit */
      if (!(resp[1] & 128))
        continue;
      /* check Card Capacity Status */
      if (resp[1] & 64)
        card_type |= SD_HC;
      timeout = 0;
      break;
    }
  }
  else {
    for (int i = 0; i < 0x400; i++) {
      e = ed_sd_cmd(SD_CMD_APP_CMD, 0, NULL);
      if (e)
        return e;
      /* 3.2-3.3V, 3.3-3.4V */
      e = ed_sd_cmd(SD_ACMD_SD_SEND_OP_COND, 0x00300000, resp);
      if (e)
        return e;
      /* check busy bit */
      if (!(resp[1] & 128))
        continue;
      timeout = 0;
      break;
    }
  }
  if (timeout)
    return ED_ERROR_SD_INIT_TIMEOUT;
  /* get rca and select card */
  e = ed_sd_cmd(SD_CMD_ALL_SEND_CID, 0, NULL);
  if (e)
    return e;
  e = ed_sd_cmd(SD_CMD_SEND_RELATIVE_ADDR, 0, resp);
  if (e)
    return e;
  uint32_t rca = (resp[1] << 24) | (resp[2] << 16) |
                 (resp[3] << 8) | (resp[4] << 0);
  ed_sd_cmd(SD_CMD_SELECT_CARD, 0, NULL);
  e = ed_sd_cmd(SD_CMD_SEND_CSD, rca, NULL);
  if (e)
    return e;
  e = ed_sd_cmd(SD_CMD_SELECT_CARD, rca, NULL);
  if (e)
    return e;
  /* select wide mode */
  e = ed_sd_cmd(SD_CMD_APP_CMD, rca, NULL);
  if (e)
    return e;
  e = ed_sd_cmd(SD_ACMD_SET_BUS_WIDTH, 2, NULL);
  if (e)
    return e;
  ed_spi_speed(SPI_SPEED_25);
  return ED_ERROR_SUCCESS;
}

void ed_close(void)
{
  ed_regs.cfg;
  ed_regs.key = 0;
}

enum ed_error ed_sd_read(uint32_t sector_index, uint32_t n_sectors, void *dst)
{
  enum ed_error e = ED_ERROR_SUCCESS;
  if (!(card_type & SD_HC))
    sector_index *= 512;
  e = ed_sd_cmd(SD_CMD_READ_MULTIPLE_BLOCK, sector_index, NULL);
  if (e)
    return e;
  e = ed_spi_read(dst, n_sectors);
  if (e)
    return e;
  e = ed_sd_stop_rw();
  return e;
}

enum ed_error ed_sd_write(uint32_t sector_index, uint32_t n_sectors, void *src)
{
  enum ed_error e;
  uint8_t *p = src;
  /* send write command */
  if (!(card_type & SD_HC))
    sector_index *= 512;
  e = ed_sd_cmd(SD_CMD_WRITE_MULTIPLE_BLOCK, sector_index, NULL);
  if (e)
    return e;
  /* write blocks */
  while (n_sectors-- > 0) {
    /* compute crc for block, one for each data line */
    uint16_t crc[4];
    crc16_wide(p, 512, crc);
    /* transmit a low nibble on the 4 data lines to start data block */
    ed_spi_mode(1, 1, 1);
    ed_spi_transmit(0xFF);
    ed_spi_transmit(0xF0);
    /* transmit data */
    ed_spi_write(p);
    p += 512;
    /* transmit crc */
    for (int i = 0; i < 4; i++) {
      ed_spi_transmit((crc[i] >> 8) & 0xFF);
      ed_spi_transmit((crc[i] >> 0) & 0xFF);
    }
    /* transmit end bit */
    ed_spi_mode(1, 1, 0);
    ed_spi_transmit(0xFF);
    /* wait for crc status */
    _Bool timeout = 1;
    ed_spi_mode(1, 0, 0);
    for (int i = 0; i < 1024; ++i)
      if (!(ed_spi_transmit(0xFF) & 1)) {
        timeout = 0;
        break;
      }
    if (timeout)
      return ED_ERROR_SD_WR_TIMEOUT;
    /* receive and check crc status */
    uint8_t resp = 0;
    for (int i = 0; i < 3; i++) {
      resp <<= 1;
      resp |= ed_spi_transmit(0xFF) & 1;
    }
    if (resp != 0b010)
      return ED_ERROR_SD_WR_CRC;
    /* wait for free data receive buffer */
    timeout = 1;
    ed_spi_mode(1, 0, 1);
    ed_spi_transmit(0xFF);
    for (int i = 0; i < 0x20000; ++i)
      if (ed_spi_transmit(0xFF) == 0xFF) {
        timeout = 0;
        break;
      }
    if (timeout)
      return ED_ERROR_SD_WR_TIMEOUT;
  }
  /* stop data transmission */
  e = ed_sd_stop_rw();
  if (e)
    return e;
  return ED_ERROR_SUCCESS;
}

enum ed_error ed_sd_stop_rw(void)
{
  enum ed_error e = ed_sd_cmd(SD_CMD_STOP_TRANSMISSION, 0, NULL);
  if (e)
    return e;
  ed_spi_mode(1, 0, 1);
  ed_spi_transmit(0xFF);
  _Bool timeout = 1;
  for (int i = 0; i < 0x40000; ++i)
    if (ed_spi_transmit(0xFF) == 0xFF) {
      timeout = 0;
      break;
    }
  if (timeout)
    return ED_ERROR_SD_CLOSE_TIMEOUT;
  return ED_ERROR_SUCCESS;
}

void ed_spi_ss(_Bool enable)
{
  spi_cfg &= ~(1 << SPI_CFG_SS);
  spi_cfg |= (!enable << SPI_CFG_SS);
  ed_regs.cfg;
  ed_regs.spi_cfg = spi_cfg;
}

void ed_spi_mode(_Bool data, _Bool write, _Bool wide)
{
  spi_cfg &= ~((1 << SPI_CFG_DAT) | (1 << SPI_CFG_RD) | (1 << SPI_CFG_1BIT));
  spi_cfg |= (data << SPI_CFG_DAT);
  spi_cfg |= (!write << SPI_CFG_RD);
  spi_cfg |= (!wide << SPI_CFG_1BIT);
  ed_regs.cfg;
  ed_regs.spi_cfg = spi_cfg;
}

void ed_spi_speed(int speed)
{
  spi_cfg &= ~3;
  spi_cfg |= speed & 3;
  ed_regs.cfg;
  ed_regs.spi_cfg = spi_cfg;
}

uint8_t ed_spi_transmit(uint8_t data)
{
  ed_regs.cfg;
  ed_regs.spi = data;
  do {
    ed_regs.status;
  } while ((ed_regs.status >> ED_STATE_SPI) & 1);
  return ed_regs.spi;
}

enum ed_error ed_spi_read(void *dst, uint32_t n_sectors)
{
  uint8_t *p = dst;
  while (n_sectors-- > 0) {
    /* wait for start bit */
    ed_spi_mode(1, 0, 0);
    _Bool timeout = 1;
    for (int i = 0; i < 65535; i++)
      if ((ed_spi_transmit(0xFF) & 0xF1) == 0xF0) {
        timeout = 0;
        break;
      }
    if (timeout)
      return ED_ERROR_SD_RD_TIMEOUT;
    /* read data */
    ed_spi_mode(1, 0, 1);
    for (int i = 0; i < 512; ++i)
      *p++ = ed_spi_transmit(0xFF);
    /* FIXME: ignore crc */
    for (int i = 0; i < 8; ++i)
      ed_spi_transmit(0xFF);
  }
  return ED_ERROR_SUCCESS;
}

enum ed_error ed_spi_write(void *src)
{
  uint8_t *p = src;
  for (int i = 0; i < 512; i++)
    ed_spi_transmit(*p++);
  return ED_ERROR_SUCCESS;
}
