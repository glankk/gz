#ifndef ED64_X_H
#define ED64_X_H
#include <stdint.h>

#define REG_BASE          0xBF800000
#define REG_FPG_CFG       0x0000
#define REG_USB_CFG       0x0001
#define REG_TIMER         0x0003
#define REG_BOOT_CFG      0x0004
#define REG_EDID          0x0005
#define REG_I2C_CMD       0x0006
#define REG_I2C_DAT       0x0007
#define REG_FPG_DAT       0x0080
#define REG_USB_DAT       0x0100
#define REG_SYS_CFG       0x2000
#define REG_KEY           0x2001
#define REG_DMA_STA       0x2002
#define REG_DMA_ADDR      0x2002
#define REG_DMA_LEN       0x2003
#define REG_RTC_SET       0x2004
#define REG_GAM_CFG       0x2006
#define REG_IOM_CFG       0x2007
#define REG_SD_CMD_RD     0x2008
#define REG_SD_CMD_WR     0x2009
#define REG_SD_DAT_RD     0x200A
#define REG_SD_DAT_WR     0x200B
#define REG_SD_STATUS     0x200C
#define REG_SDIO_ARD      0x2080
#define REG_IOM_DAT       0x2100
#define REG_DD_TBL        0x2200
#define REGS_PTR          ((volatile uint32_t *)REG_BASE)

#define FPG_CFG_NCFG      0x0001
#define FPG_STA_CDON      0x0001
#define FPG_STA_NSTAT     0x0002

#define USB_LE_CFG        0x8000
#define USB_LE_CTR        0x4000

#define USB_CFG_ACT       0x0200
#define USB_CFG_RD        0x0400
#define USB_CFG_WR        0x0000

#define USB_STA_ACT       0x0200
#define USB_STA_RXF       0x0400
#define USB_STA_TXE       0x0800
#define USB_STA_PWR       0x1000
#define USB_STA_BSY       0x2000

#define I2C_CMD_DAT       0x10
#define I2C_CMD_STA       0x20
#define I2C_CMD_END       0x30

#define CFG_BROM_ON       0x0001
#define CFG_REGS_OFF      0x0002
#define CFG_SWAP_ON       0x0004

#define DMA_STA_BUSY      0x0001
#define DMA_STA_ERROR     0x0002
#define DMA_STA_LOCK      0x0080

#define IOM_CFG_SS        0x0001
#define IOM_CFG_RST       0x0002
#define IOM_CFG_ACT       0x0080
#define IOM_STA_CDN       0x0001

#define SD_CFG_BITLEN     0x000F
#define SD_CFG_SPD        0x0010
#define SD_STA_BUSY       0x0080

#define SPI_SPEED_50      0x0000
#define SPI_SPEED_25      0x0001
#define SPI_SPEED_LO      0x0002
#define SPI_SPEED         0x0003
#define SPI_SS            0x0004
#define SPI_RD            0x0008
#define SPI_WR            0x0000
#define SPI_DAT           0x0010
#define SPI_CMD           0x0000
#define SPI_BIT           0x0020
#define SPI_BYTE          0x0000

#endif
