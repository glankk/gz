#ifndef SD_HOST_H
#define SD_HOST_H
#include <stddef.h>
#include <stdint.h>

#define SD_PROTO_SDBUS  0
#define SD_PROTO_SPIBUS 1

#define SD_CARD_V2      1
#define SD_CARD_HC      2

#define SD_ERR_TIMEOUT  1
#define SD_ERR_CRC      2
#define SD_ERR_ILCMD    3
#define SD_ERR_ADDR     4
#define SD_ERR_PARAM    5
#define SD_ERR_ERSEQ    6
#define SD_ERR_WPVIOL   7
#define SD_ERR_ECC      8
#define SD_ERR_CC       9
#define SD_ERR_RANGE    10
#define SD_ERR_GEN      11

struct sd_host
{
  int     proto;

  void  (*lock)       (void);
  void  (*unlock)     (void);
  void  (*set_spd)    (int spd);
  union
  {
    struct
    {
      void  (*spi_ss)     (int ss);
      int   (*spi_io)     (int dat);
      void  (*spi_rx_buf) (void *buf, size_t size);
      void  (*spi_tx_buf) (const void *buf, size_t size);
      void  (*spi_tx_clk) (int dat, size_t n_clk);
    };
    struct
    {
      int   (*cmd_rx)     (void);
      int   (*dat_rx)     (void);
      void  (*dat_tx)     (int dat);
      void  (*cmd_rx_buf) (void *buf, size_t size);
      void  (*cmd_tx_buf) (const void *buf, size_t size);
      void  (*dat_rx_buf) (void *buf, size_t size);
      void  (*dat_tx_buf) (const void *buf, size_t size);
      void  (*dat_tx_clk) (int dat, size_t n_clk);
    };
  };
  int   (*rx_mblk)    (void *buf, size_t blk_size, size_t n_blk);
  int   (*tx_mblk)    (const void *buf, size_t blk_size, size_t n_blk);

  int     card_type;
};

int sd_init (struct sd_host *host);
int sd_read (struct sd_host *host, size_t lba, void *dst, size_t n_blk);
int sd_write(struct sd_host *host, size_t lba, const void *src, size_t n_blk);

#endif
