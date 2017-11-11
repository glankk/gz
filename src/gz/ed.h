#ifndef ED_H
#define ED_H
#include <stdint.h>

#define ED_CFG_SDRAM_ON             0
#define ED_CFG_SWAP                 1
#define ED_CFG_WR_MOD               2
#define ED_CFG_WR_ADDR_MASK         3

#define ED_STATE_DMA_BUSY           0
#define ED_STATE_DMA_TOUT           1
#define ED_STATE_TXE                2
#define ED_STATE_RXF                3
#define ED_STATE_SPI                4

#define DCFG_SD_TO_RAM              1
#define DCFG_RAM_TO_SD              2
#define DCFG_FIFO_TO_RAM            3
#define DCFG_RAM_TO_FIFO            4

#define SPI_CFG_SPD0                0
#define SPI_CFG_SPD1                1
#define SPI_CFG_SS                  2
#define SPI_CFG_RD                  3
#define SPI_CFG_DAT                 4
#define SPI_CFG_1BIT                5

#define SPI_SPEED_INIT              2
#define SPI_SPEED_25                1
#define SPI_SPEED_50                0

#define SD_V2                       2
#define SD_HC                       1

#define SD_CMD_GO_IDLE_STATE        0
#define SD_CMD_ALL_SEND_CID         2
#define SD_CMD_SEND_RELATIVE_ADDR   3
#define SD_CMD_SELECT_CARD          7
#define SD_CMD_SEND_IF_COND         8
#define SD_CMD_SEND_CSD             9
#define SD_CMD_STOP_TRANSMISSION    12
#define SD_CMD_READ_MULTIPLE_BLOCK  18
#define SD_CMD_WRITE_MULTIPLE_BLOCK 25
#define SD_CMD_APP_CMD              55

#define SD_ACMD_SET_BUS_WIDTH       6
#define SD_ACMD_SD_SEND_OP_COND     41

#define SD_R1                       1
#define SD_R2                       2
#define SD_R3                       3
#define SD_R6                       6
#define SD_R7                       7

enum ed_error
{
  ED_ERROR_SUCCESS,
  ED_ERROR_SD_CMD_CRC,
  ED_ERROR_SD_CMD_TIMEOUT,
  ED_ERROR_SD_INIT_TIMEOUT,
  ED_ERROR_SD_CLOSE_TIMEOUT,
  ED_ERROR_SD_WR_TIMEOUT,
  ED_ERROR_SD_WR_CRC,
  ED_ERROR_SD_RD_TIMEOUT,
};

typedef struct
{
  uint32_t cfg;
  uint32_t status;
  uint32_t dma_len;
  uint32_t dma_ram_addr;
  uint32_t msg;
  uint32_t dma_cfg;
  uint32_t spi;
  uint32_t spi_cfg;
  uint32_t key;
  uint32_t sav_cfg;
  uint32_t sec;
  uint32_t ver;
  uint32_t unk_00_[0x0004];
  uint32_t cfg_cnt;
  uint32_t cfg_dat;
  uint32_t max_msg;
  uint32_t crc;
} ed_regs_t;

#define ed_regs                     (*(volatile ed_regs_t*)0xA8040000)

enum ed_error ed_sd_cmd(int cmd, uint32_t arg, void *resp_buf);
enum ed_error ed_sd_init(void);
void          ed_close(void);

enum ed_error ed_sd_read(uint32_t lba, uint32_t n_sectors, void *dst);
enum ed_error ed_sd_write(uint32_t lba, uint32_t n_sectors, void *src);
enum ed_error ed_sd_stop_rw(void);

void          ed_spi_ss(_Bool enable);
void          ed_spi_mode(_Bool data, _Bool write, _Bool wide);
void          ed_spi_speed(int speed);
uint8_t       ed_spi_transmit(uint8_t data);
enum ed_error ed_spi_read(void *dst, uint32_t n_sectors);
enum ed_error ed_spi_write(void *src);

#endif
