#ifndef ED64_L_H
#define ED64_L_H
#include <stdint.h>

/*
 *  FW < 1.16:
 *    SPI: 8-bit shift register. Shifts right-to-left, lsb in and msb out.
 *
 *    REG_SPI (R/W)
 *      W: Store 8 bits in SPI. Send 8 clocks on SCLK, shifting bits out of SPI
 *         on MOSI, and in to SPI on MISO.
 *      R: Read SPI.
 *
 *    REG_SPI_CFG (R/W)
 *      [1:0] SPI_SPEED
 *        0: 50MHz
 *        1: 25MHz
 *        2: < 400KHz
 *      [2] SPI_SS
 *        0: SS line low (on)
 *        1: SS line high (off)
 *
 *  FW >= 1.16:
 *    SPI: 8-bit shift register. Shifts right-to-left, lsb in and msb out.
 *
 *    REG_SPI (R/W)
 *      W: Store 8 bits in SPI.
 *        If ~SPI_DAT and ~SPI_1BIT:
 *          Send 8 clocks, shifting 1 bit per clock on CMD.
 *        If ~SPI_DAT and SPI_1BIT:
 *          Send 1 clock, shifting 1 bit per clock on CMD.
 *        If SPI_DAT and ~SPI_1BIT:
 *          Send 2 clocks, shifting 4 bits per clock on DAT0 - DAT3.
 *        If SPI_DAT and SPI_1BIT:
 *          Send 1 clock, shifting 4 bits per clock on DAT0 - DAT3.
 *        If ~SPI_RD:
 *          Shift bits out of SPI.
 *        If SPI_RD:
 *          Shift bits in to SPI.
 *      R: Read SPI.
 *
 *    REG_SPI_CFG (R/W)
 *      [1:0] SPI_SPEED
 *        0: 50MHz
 *        1: 25MHz
 *        2: < 400KHz
 *      [3] SPI_RD
 *        0: Shift bits out of SPI
 *        1: Shift bits in to SPI
 *      [4] SPI_DAT
 *        0: Shift bits on CMD
 *        1: Shift bits on DAT0 - DAT3
 *      [5] SPI_1BIT
 *        0: Send 8 clocks if ~SPI_DAT or 2 clocks if SPI_DAT
 *        1: Send 1 clock
 */

#define REG_BASE          0xA8040000
#define REG_CFG           0
#define REG_STATUS        1
#define REG_DMA_LEN       2
#define REG_DMA_ADDR      3
#define REG_MSG           4
#define REG_DMA_CFG       5
#define REG_SPI           6
#define REG_SPI_CFG       7
#define REG_KEY           8
#define REG_SAV_CFG       9
#define REG_SEC           10
#define REG_VER           11
#define REG_CFG_CNT       16
#define REG_CFG_DAT       17
#define REG_MAX_MSG       18
#define REG_CRC           19
#define REGS_PTR          ((volatile uint32_t *)REG_BASE)

#define CFG_SDRAM_ON      0x0001
#define CFG_SWAP          0x0002
#define CFG_WR_MOD        0x0004
#define CFG_WR_ADDR_MASK  0x0008

#define STATUS_DMA_BUSY   0x0001
#define STATUS_DMA_TOUT   0x0002
#define STATUS_TXE        0x0004
#define STATUS_RXF        0x0008
#define STATUS_SPI        0x0010

#define DMA_SD_TO_RAM     0x0001
#define DMA_RAM_TO_SD     0x0002
#define DMA_FIFO_TO_RAM   0x0003
#define DMA_RAM_TO_FIFO   0x0004

#define SPI_SPEED_50      0x0000
#define SPI_SPEED_25      0x0001
#define SPI_SPEED_LO      0x0002
#define SPI_SPEED         0x0003
#define SPI_SS            0x0004
#define SPI_RD            0x0008
#define SPI_WR            0x0000
#define SPI_DAT           0x0010
#define SPI_CMD           0x0000
#define SPI_1CLK          0x0020
#define SPI_BYTE          0x0000

#endif
