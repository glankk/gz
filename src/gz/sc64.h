#ifndef SC64_H
#define SC64_H
#include <stdint.h>

#define BUFFER_BASE     0xBFFE0000
#define BUFFER_SIZE     0x2000

#define REG_BASE        0xBFFF0000
#define REG_STAT        0
#define REG_CMD         0
#define REG_DAT0        1
#define REG_DAT1        2
#define REG_ID          3
#define REG_KEY         4
#define REGS_PTR        ((volatile uint32_t *)REG_BASE)

/* REG_STAT */
#define STAT_BUSY       0x80000000
#define STAT_ERR        0x40000000
#define STAT_PEND       0x20000000

/* REG_CMD */
#define CMD_CFG_GET     'c'
#define CMD_CFG_SET     'C'
#define CMD_SD_OP       'i'
#define CMD_SD_SECT     'I'
#define CMD_SD_RD       's'
#define CMD_SD_WR       'S'
#define CMD_USB_RD      'm'
#define CMD_USB_WR      'M'
#define CMD_USB_RSTAT   'u'
#define CMD_USB_WSTAT   'U'

/* REG_DAT0: CFG_SET */
#define CFG_ROM_WR      1

/* REG_DAT1: SD_OP */
#define SD_INIT         1
#define SD_STAT         2
#define SD_INFO         3
#define SD_BSWAP_ON     4
#define SD_BSWAP_OFF    5

/* REG_DAT0: USB_RSTAT */
#define USB_RSTAT_BUSY  0x80000000

/* REG_DAT0: USB_WSTAT */
#define USB_WSTAT_BUSY  0x80000000

/* REG_ID */
#define SC64_IDENT      0x53437632 /* SCv2 */

/* REG_KEY */
#define KEY_RST         0x00000000
#define KEY_LCK         0xFFFFFFFF
#define KEY_UNL         0x5F554E4C /* _UNL */
#define KEY_OCK         0x4F434B5F /* OCK_ */

#endif
