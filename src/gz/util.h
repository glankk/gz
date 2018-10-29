#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include <mips.h>
#include <n64.h>

/* set interrupt enable bit and return previous value */
static inline _Bool set_int(_Bool enable)
{
  uint32_t sr;
  __asm__ volatile ("mfc0  %0, $12;" : "=r"(sr));
  _Bool ie = sr & MIPS_STATUS_IE;
  if (enable)
    sr |= MIPS_STATUS_IE;
  else
    sr &= ~MIPS_STATUS_IE;
  __asm__ volatile ("mtc0  %0, $12;" :: "r"(sr));
  return ie;
}

/* wait for dma and disable interrupts */
static inline _Bool enter_dma_section(void)
{
  _Bool ie;
  while (1) {
    if (pi_regs.status & PI_STATUS_DMA_BUSY)
      continue;
    ie = set_int(0);
    if (pi_regs.status & PI_STATUS_DMA_BUSY) {
      set_int(ie);
      continue;
    }
    break;
  }
  return ie;
}

/* dma cart to ram and invalidate cache */
static inline void dma_read(void *dst, uint32_t cart_addr, uint32_t len)
{
  pi_regs.dram_addr = MIPS_KSEG0_TO_PHYS(dst);
  pi_regs.cart_addr = MIPS_KSEG1_TO_PHYS(cart_addr);
  pi_regs.wr_len = len - 1;
  while (pi_regs.status & PI_STATUS_DMA_BUSY)
    ;
  pi_regs.status = PI_STATUS_CLR_INTR;
  for (uint32_t i = 0; i < len; i += 0x10)
    __asm__ volatile ("cache 0x11, 0x0000(%0);" :: "r"((uint32_t)dst + i));
}

/* flush cache and dma ram to cart */
static inline void dma_write(void *src, uint32_t cart_addr, uint32_t len)
{
  for (uint32_t i = 0; i < len; i += 0x10)
    __asm__ volatile ("cache 0x19, 0x0000(%0);" :: "r"((uint32_t)src + i));
  pi_regs.dram_addr = MIPS_KSEG0_TO_PHYS(src);
  pi_regs.cart_addr = MIPS_KSEG1_TO_PHYS(cart_addr);
  pi_regs.rd_len = len - 1;
  while (pi_regs.status & PI_STATUS_DMA_BUSY)
    ;
  pi_regs.status = PI_STATUS_CLR_INTR;
}

/* safe (non-signaling) nan check */
static inline _Bool is_nan(float f)
{
  uint32_t exp_mask = 0b01111111100000000000000000000000;
  uint32_t sig_mask = 0b00000000011111111111111111111111;
  union
  {
    uint32_t  w;
    float     f;
  } pun;
  pun.f = f;
  return (pun.w & exp_mask) == exp_mask && (pun.w & sig_mask) != 0;
}

#endif
