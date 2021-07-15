#ifndef PI_H
#define PI_H
#include <stddef.h>
#include <stdint.h>
#include <n64.h>

void pi_write_locked(uint32_t dev_addr, const void *src, size_t size);
void pi_read_locked(uint32_t dev_addr, void *dst, size_t size);
void pi_write(uint32_t dev_addr, const void *src, size_t size);
void pi_read(uint32_t dev_addr, void *dst, size_t size);

static inline void __pi_wait(void)
{
  while (pi_regs.status & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY))
    ;
}

static inline uint32_t __pi_read_raw(uint32_t dev_addr)
{
  __pi_wait();
  return *(volatile uint32_t *)dev_addr;
}

static inline void __pi_write_raw(uint32_t dev_addr, uint32_t value)
{
  __pi_wait();
  *(volatile uint32_t *)dev_addr = value;
}

#endif
