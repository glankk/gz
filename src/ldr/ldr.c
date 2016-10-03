#include <stdint.h>
#include <n64.h>

#define S(x) S_(x)
#define S_(x) #x

enum
{
  DMA_BUSY  = 0x00000001,
  DMA_DONE  = 0x00000002,
  DMA_ERROR = 0x00000008,
};

struct
{
  uint32_t ram_address;
  uint32_t rom_address;
  uint32_t size_ramrom;
  uint32_t size_romram;
  uint32_t status;
}
static volatile *dma_regs = (void*)0xA4600000;


ENTRY __attribute__((noreturn)) void _start()
{
  dma_regs->ram_address = DMA_RAM & 0x00FFFFFF;
  dma_regs->rom_address = DMA_ROM | 0x10000000;
  dma_regs->size_romram = DMA_SIZE - 1;
  dma_regs->status = DMA_DONE;
  while(dma_regs->status & DMA_BUSY)
    ;
  __asm__ volatile(
                   "la $t0, " S(DMA_RAM) " \n"
                   "jr $t0                 \n"
                  );

  __builtin_unreachable();
}
