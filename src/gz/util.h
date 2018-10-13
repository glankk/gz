#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include <mips.h>

/* set interrupt enable bit and return previous value */
static inline _Bool set_int(_Bool enable)
{
  uint32_t sr;
  __asm__ volatile ("mfc0 %0, $12" : "=r"(sr));
  _Bool ie = sr & MIPS_STATUS_IE;
  if (enable)
    sr |= MIPS_STATUS_IE;
  else
    sr &= ~MIPS_STATUS_IE;
  __asm__ volatile ("mtc0 %0, $12" :: "r"(sr));
  return ie;
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
