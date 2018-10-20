#ifndef ED_H
#define ED_H
#include <stdint.h>

#define ed_regs                     (*(volatile ed_regs_t*)0xA8040000)

#define ED_CFG_SDRAM_ON             ((uint32_t)1 << 0)
#define ED_CFG_SWAP                 ((uint32_t)1 << 1)
#define ED_CFG_WR_MOD               ((uint32_t)1 << 2)
#define ED_CFG_WR_ADDR_MASK         ((uint32_t)1 << 3)

#define ED_STATE_DMA_BUSY           ((uint32_t)1 << 0)
#define ED_STATE_DMA_TOUT           ((uint32_t)1 << 1)
#define ED_STATE_TXE                ((uint32_t)1 << 2)
#define ED_STATE_RXF                ((uint32_t)1 << 3)
#define ED_STATE_SPI                ((uint32_t)1 << 4)

#define ED_DMA_SD_TO_RAM            1
#define ED_DMA_RAM_TO_SD            2
#define ED_DMA_FIFO_TO_RAM          3
#define ED_DMA_RAM_TO_FIFO          4

#define ED_SPI_CFG_SPDMASK          ((uint32_t)0x00000003)
#define ED_SPI_CFG_SPDSHIFT         0
#define ED_SPI_CFG_SS               ((uint32_t)1 << 2)
#define ED_SPI_CFG_RD               ((uint32_t)1 << 3)
#define ED_SPI_CFG_DAT              ((uint32_t)1 << 4)
#define ED_SPI_CFG_1BIT             ((uint32_t)1 << 5)

#define ED_SPI_SPEED_50             0
#define ED_SPI_SPEED_25             1
#define ED_SPI_SPEED_INIT           2

#define SD_HC                       ((uint32_t)1 << 0)
#define SD_V2                       ((uint32_t)1 << 1)

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
  ED_ERROR_SD_RD_CRC,
  ED_ERROR_FIFO_WR_TIMEOUT,
  ED_ERROR_FIFO_RD_TIMEOUT,
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

void          ed_open(void);
void          ed_close(void);

enum ed_error ed_sd_cmd_r(int cmd, uint32_t arg, void *resp_buf);
enum ed_error ed_sd_cmd(int cmd, uint32_t arg, void *resp_buf);
enum ed_error ed_sd_init(void);

enum ed_error ed_sd_read_r(uint32_t lba, uint32_t n_blocks,
                           void *dst, uint32_t *n_read);
enum ed_error ed_sd_write_r(uint32_t lba, uint32_t n_blocks,
                            void *src, uint32_t *n_write);
enum ed_error ed_sd_read(uint32_t lba, uint32_t n_blocks, void *dst);
enum ed_error ed_sd_write(uint32_t lba, uint32_t n_blocks, void *src);
enum ed_error ed_sd_read_dma(uint32_t lba, uint32_t n_blocks, void *dst);
enum ed_error ed_sd_stop_rw(void);

enum ed_error ed_fifo_read(void *dst, uint32_t n_blocks);
enum ed_error ed_fifo_write(void *src, uint32_t n_blocks);

void          ed_spi_ss(_Bool enable);
void          ed_spi_mode(_Bool data, _Bool write, _Bool wide);
void          ed_spi_speed(int speed);
uint8_t       ed_spi_transmit(uint8_t data);

#endif
