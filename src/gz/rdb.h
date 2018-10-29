#ifndef RDB_H
#define RDB_H
#include <mips.h>

#define RDB_INTR_OP     MIPS_TEQ(MIPS_R0, MIPS_R0, 0x151)
#define rdb_interrupt() ({__asm__ volatile (".word %0;" :: "i"(RDB_INTR_OP));})

void  rdb_start(void);
void  rdb_stop(void);
_Bool rdb_check(void);

#endif
