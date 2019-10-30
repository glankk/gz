#ifndef HB_H
#include <stdint.h>

#define hb_regs               (*(volatile hb_regs_t*)0xA8050000)

#define HB_STATUS_RESET       (0b1    << 9)
#define HB_STATUS_ERROR       (0b1111 << 5)
#define HB_STATUS_SD_INIT     (0b1    << 4)
#define HB_STATUS_SD_HC       (0b1    << 3)
#define HB_STATUS_SD_INSERTED (0b1    << 2)
#define HB_STATUS_SD_BUSY     (0b1    << 1)
#define HB_STATUS_SD_READY    (0b1    << 0)

#define HB_ERROR_SUCCESS    0
#define HB_ERROR_INVAL      1
#define HB_ERROR_QUEUEFULL  2
#define HB_ERROR_NOMEM      3
#define HB_ERROR_NOBUFFER   4
#define HB_ERROR_OTHER      5

typedef struct
{
  uint32_t key;
  uint32_t sd_dram_addr;
  uint32_t sd_write_lba;
  uint32_t sd_read_lba;
  uint32_t sd_n_blocks;
  uint32_t status;
  uint32_t dram_save_addr;
  uint32_t dram_save_len;
  uint32_t dram_save_key;
} hb_regs_t;

int hb_sd_init(void);
int hb_sd_read(uint32_t lba, uint32_t n_blocks, void *dst);
int hb_sd_write(uint32_t lba, uint32_t n_blocks, void *src);
int hb_reset(uint32_t dram_save_addr, uint32_t dram_save_len);

#endif
