#include <startup.h>
#include <mips.h>
#include <n64.h>

ENTRY _Noreturn void _start()
{
  pi_regs.dram_addr     = MIPS_KSEG0_TO_PHYS(DMA_RAM);
  pi_regs.cart_addr     = (DMA_ROM) | 0x10000000;
  pi_regs.wr_len        = (DMA_SIZE) - 1;
  while (pi_regs.status & PI_STATUS_DMA_BUSY)
    ;
  pi_regs.status        = PI_STATUS_CLR_INTR;
  ((void (*)())(DMA_RAM))();

  __builtin_unreachable();
}
