#ifndef SD_H
#define SD_H
#include <stdint.h>

/* Common commands */
#define GO_IDLE_STATE           0
#define SWITCH_FUNC             6
#define SEND_IF_COND            8
#define SEND_CSD                9
#define SEND_CID                10
#define STOP_TRANSMISSION       12
#define SEND_STATUS             13
#define SET_BLOCKLEN            16
#define READ_SINGLE_BLOCK       17
#define READ_MULTIPLE_BLOCK     18
#define ADDRESS_EXTENSION       22
#define SET_BLOCK_COUNT         23
#define WRITE_BLOCK             24
#define WRITE_MULTIPLE_BLOCK    25
#define PROGRAM_CSD             27
#define SET_WRITE_PROT          28
#define CLR_WRITE_PROT          29
#define SEND_WRITE_PROT         30
#define ERASE_WR_BLK_START      32
#define ERASE_WR_BLK_END        33
#define ERASE                   38
#define LOCK_UNLOCK             42
#define APP_CMD                 55
#define GEN_CMD                 56

/* Application specific commands */
#define SD_STATUS               13
#define SEND_NUM_WR_BLOCKS      22
#define SET_WR_BLK_ERASE_COUNT  23
#define SD_SEND_OP_COND         41
#define SET_CLR_CARD_DETECT     42
#define SEND_SCR                51

/* Response types */
#define R1                      1
#define R2                      2
#define R3                      3
#define R6                      6
#define R7                      7

/* Data response tokens */
#define DAT_RESP                0x1F
#define DAT_RESP_OK             0x05
#define DAT_RESP_CRC_ERR        0x0B
#define DAT_RESP_WR_ERR         0x0D

/* OCR bits */
#define OCR_2V7_2V8             0x00008000
#define OCR_2V8_2V9             0x00010000
#define OCR_2V9_3V0             0x00020000
#define OCR_3V0_3V1             0x00040000
#define OCR_3V1_3V2             0x00080000
#define OCR_3V2_3V3             0x00100000
#define OCR_3V3_3V4             0x00200000
#define OCR_3V4_3V5             0x00400000
#define OCR_3V5_3V6             0x00800000
#define OCR_S18A                0x01000000
#define OCR_CO2T                0x08000000
#define OCR_UHS2                0x20000000
#define OCR_CCS                 0x40000000
#define OCR_BUSY                0x80000000

/* Card Status bits */
#define CS_AKE_SEQ_ERROR        0x00000008
#define CS_APP_CMD              0x00000020
#define CS_FX_EVENT             0x00000040
#define CS_READY_FOR_DATA       0x00000100
#define CS_CURRENT_STATE        0x00001E00
#define CS_ERASE_RESET          0x00002000
#define CS_CARD_ECC_DISABLED    0x00004000
#define CS_WP_ERASE_SKIP        0x00008000
#define CS_CSD_OVERWRITE        0x00010000
#define CS_ERROR                0x00080000
#define CS_CC_ERROR             0x00100000
#define CS_CARD_ECC_FAILED      0x00200000
#define CS_ILLEGAL_COMMAND      0x00400000
#define CS_COM_CRC_ERROR        0x00800000
#define CS_LOCK_UNLOCK_FAILED   0x01000000
#define CS_CARD_IS_LOCKED       0x02000000
#define CS_WP_VIOLATION         0x04000000
#define CS_ERASE_PARAM          0x08000000
#define CS_ERASE_SEQ_ERROR      0x10000000
#define CS_BLOCK_LEN_ERROR      0x20000000
#define CS_ADDRESS_ERROR        0x40000000
#define CS_OUT_OF_RANGE         0x80000000
#define CS_ERR_BITS             0xFDF98008

/* Interface conditions */
#define IF_COND_VHS_VDD1        0x00000100
#define IF_COND_PCIE            0x00001000
#define IF_COND_PCIE_VDD3       0x00002000

/* Response fields */
static inline uint32_t sd_r1_cs(const void *resp)
{
  uint32_t cs = 0;

  const uint8_t *p = resp;
  p++;
  cs = (cs << 8) | *p++;
  cs = (cs << 8) | *p++;
  cs = (cs << 8) | *p++;
  cs = (cs << 8) | *p++;

  return cs;
}

static inline uint32_t r3_ocr(const void *resp)
{
  uint32_t ocr = 0;

  const uint8_t *p = resp;
  p++;
  ocr = (ocr << 8) | *p++;
  ocr = (ocr << 8) | *p++;
  ocr = (ocr << 8) | *p++;
  ocr = (ocr << 8) | *p++;

  return ocr;
}

static inline uint32_t r6_rca(const void *resp)
{
  uint32_t rca = 0;

  const uint8_t *p = resp;
  p++;
  rca = (rca << 8) | *p++;
  rca = (rca << 8) | *p++;

  return rca;
}

static inline uint32_t r6_status(const void *resp)
{
  uint32_t st = 0;

  const uint8_t *p = resp;
  p += 3;
  st = (st << 8) | *p++;
  st = (st << 8) | *p++;

  return st;
}

/* Switch function fields */
#define SWFN_CHECK              0x00000000
#define SWFN_SET                0x80000000
#define SWFN_DAT_SIZE           64

static inline uint32_t swfn_pow(const void *dat)
{
  uint32_t pow = 0;

  const uint8_t *p = dat;
  pow = (pow << 8) | *p++;
  pow = (pow << 8) | *p++;

  return pow;
}

static inline uint32_t swfn_sup(const void *dat, int group)
{
  uint32_t sup = 0;

  const uint8_t *p = dat;
  p += 14 - (group << 1);
  sup = (sup << 8) | *p++;
  sup = (sup << 8) | *p++;

  return sup;
}

static inline uint32_t swfn_sel(const void *dat, int group)
{
  const uint8_t *p = dat;
  p += 17 - ((group + 1) >> 1);
  if (group & 1)
    return *p & 0xF;
  else
    return *p >> 4;
}

static inline uint32_t swfn_ver(const void *dat)
{
  const uint8_t *p = dat;
  return p[17];
}

static inline uint32_t swfn_bsy(const void *dat, int group)
{
  uint32_t bsy = 0;

  const uint8_t *p = dat;
  p += 30 - (group << 1);
  bsy = (bsy << 8) | *p++;
  bsy = (bsy << 8) | *p++;

  return bsy;
}

/* SD Bus commands */
#define ALL_SEND_CID            2
#define SEND_RELATIVE_ADDR      3
#define SET_DSR                 4
#define SELECT_CARD             7
#define VOLTAGE_SWITCH          11
#define GO_INACTIVE_STATE       15
#define SEND_TUNING_BLOCK       19
#define SPEED_CLASS_CONTROL     20
#define Q_MANAGEMENT            43
#define Q_TASK_INFO_A           44
#define Q_TASK_INFO_B           45
#define Q_RD_TASK               46
#define Q_WR_TASK               47
#define READ_EXTR_SINGLE        48
#define WRITE_EXTR_SINGLE       49
#define READ_EXTR_MULTI         58
#define WRITE_EXTR_MULTI        59

/* SD Bus application specific commands */
#define SET_BUS_WIDTH           6

static inline int sd_resp_type(int cmd)
{
  switch (cmd) {
    case GO_IDLE_STATE      :
    case SET_DSR            :
    case GO_INACTIVE_STATE  : return 0;
    case ALL_SEND_CID       :
    case SEND_CSD           :
    case SEND_CID           : return R2;
    case SD_SEND_OP_COND    : return R3;
    case SEND_RELATIVE_ADDR : return R6;
    case SEND_IF_COND       : return R7;
    default                 : return R1;
  }
}

static inline int sd_resp_size(int resp_type)
{
  switch (resp_type) {
    case R1 :
    case R3 :
    case R6 :
    case R7 :   return 6;
    case R2 :   return 17;
    default :   return 0;
  }
}

/* SPI Bus commands */
#define SEND_OP_COND            1
#define READ_OCR                58
#define CRC_ON_OFF              59

/* SPI R1 bits */
#define SPI_R1_IN_IDLE_STATE    0x01
#define SPI_R1_ERASE_RESET      0x02
#define SPI_R1_ILLEGAL_CMD      0x04
#define SPI_R1_CRC_ERR          0x08
#define SPI_R1_ERASE_SEQ_ERR    0x10
#define SPI_R1_ADDR_ERR         0x20
#define SPI_R1_PARAM_ERR        0x40
#define SPI_R1_ZERO             0x80
#define SPI_R1_ERR_BITS         0x7C

/* SPI data block tokens */
#define SPI_BLK_START           0xFE
#define SPI_BLK_ERR             0x01
#define SPI_BLK_CC_ERR          0x02
#define SPI_BLK_ECC_ERR         0x04
#define SPI_BLK_RANGE_ERR       0x08

/* SPI multiple block write tokens */
#define SPI_MBW_START           0xFC
#define SPI_MBW_STOP            0xFD

static inline int spi_resp_type(int cmd)
{
  switch (cmd) {
    case SEND_STATUS  : return R2;
    case READ_OCR     : return R3;
    case SEND_IF_COND : return R7;
    default           : return R1;
  }
}

static inline int spi_resp_size(int resp_type)
{
  switch (resp_type) {
    case R1 :   return 1;
    case R2 :   return 2;
    case R3 :
    case R7 :   return 5;
    default :   return 0;
  }
}

#endif
