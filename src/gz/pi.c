#include <stddef.h>
#include <stdint.h>
#include <n64.h>
#include "pi.h"
#include "util.h"

typedef void io_func_t(uint32_t dev_addr, uint32_t ram_addr, size_t size);

static void pio_write(uint32_t dev_addr, uint32_t ram_addr, size_t size)
{
  if (size == 0)
    return;

  uint32_t dev_s = dev_addr & ~0x3;
  uint32_t dev_e = (dev_addr + size + 0x3) & ~0x3;
  uint32_t dev_p = dev_s;

  uint32_t ram_s = ram_addr;
  uint32_t ram_e = ram_s + size;
  uint32_t ram_p = ram_addr - (dev_addr - dev_s);

  while (dev_p < dev_e) {
    uint32_t w = __pi_read_raw(dev_p);
    for (int i = 0; i < 4; i++) {
      uint8_t b;
      if (ram_p >= ram_s && ram_p < ram_e)
        b = *(uint8_t *)ram_p;
      else
        b = w >> 24;
      w = (w << 8) | b;
      ram_p++;
    }
    __pi_write_raw(dev_p, w);
    dev_p += 4;
  }
}

static void pio_read(uint32_t dev_addr, uint32_t ram_addr, size_t size)
{
  if (size == 0)
    return;

  uint32_t dev_s = dev_addr & ~0x3;
  uint32_t dev_e = (dev_addr + size + 0x3) & ~0x3;
  uint32_t dev_p = dev_s;

  uint32_t ram_s = ram_addr;
  uint32_t ram_e = ram_s + size;
  uint32_t ram_p = ram_addr - (dev_addr - dev_s);

  while (dev_p < dev_e) {
    uint32_t w = __pi_read_raw(dev_p);
    for (int i = 0; i < 4; i++) {
      if (ram_p >= ram_s && ram_p < ram_e)
        *(uint8_t *)ram_p = w >> 24;
      w <<= 8;
      ram_p++;
    }
    dev_p += 4;
  }
}

static void dma_write(uint32_t dev_addr, uint32_t ram_addr, size_t size)
{
  if (size == 0)
    return;

  OSMesgQueue mq;
  OSMesg m;
  __OSEventState pi_event;

  int irqf = get_irqf();
  if (irqf) {
    osCreateMesgQueue(&mq, &m, 1);

    pi_event = __osEventStateTab[OS_EVENT_PI];
    __osEventStateTab[OS_EVENT_PI].messageQueue = &mq;
  }

  dcache_wb((void *)ram_addr, size);
  pi_regs.dram_addr = ram_addr & 0x1FFFFFFF;
  pi_regs.cart_addr = dev_addr & 0x1FFFFFFF;
  pi_regs.rd_len = size - 1;

  if (irqf) {
    osRecvMesg(&mq, NULL, OS_MESG_BLOCK);

    __osEventStateTab[OS_EVENT_PI] = pi_event;
  }
  else {
    __pi_wait();
    pi_regs.status = PI_STATUS_CLR_INTR;
  }
}

static void dma_read(uint32_t dev_addr, uint32_t ram_addr, size_t size)
{
  if (size == 0)
    return;

  OSMesgQueue mq;
  OSMesg m;
  __OSEventState pi_event;

  int irqf = get_irqf();
  if (irqf) {
    osCreateMesgQueue(&mq, &m, 1);

    pi_event = __osEventStateTab[OS_EVENT_PI];
    __osEventStateTab[OS_EVENT_PI].messageQueue = &mq;
  }

  dcache_wbinv((void *)ram_addr, size);
  pi_regs.dram_addr = ram_addr & 0x1FFFFFFF;
  pi_regs.cart_addr = dev_addr & 0x1FFFFFFF;
  pi_regs.wr_len = size - 1;

  if (irqf) {
    osRecvMesg(&mq, NULL, OS_MESG_BLOCK);

    __osEventStateTab[OS_EVENT_PI] = pi_event;
  }
  else {
    __pi_wait();
    pi_regs.status = PI_STATUS_CLR_INTR;
  }
}

static void do_transfer(uint32_t dev_addr, uint32_t ram_addr, size_t size,
                        io_func_t *pio_func, io_func_t *dma_func)
{
  if ((dev_addr ^ ram_addr) & 1) {
    /* Impossible alignment for DMA transfer,
     * we have to PIO the whole thing.
     */
    pio_func(dev_addr, ram_addr, size);
  }
  else {
    uint32_t ram_s = ram_addr;
    uint32_t ram_e = ram_addr + size;
    uint32_t ram_align_s = (ram_s + 0x7) & ~0x7;
    uint32_t dev_s = dev_addr;

    if (ram_e > ram_align_s) {
      uint32_t ram_align_e = ram_e & ~0x1;
      size_t pio_s = ram_align_s - ram_s;
      size_t pio_e = ram_e - ram_align_e;
      size_t dma = size - pio_s - pio_e;
      uint32_t dev_e = dev_addr + size;
      uint32_t dev_align_s = dev_s + pio_s;
      uint32_t dev_align_e = dev_e - pio_e;

      pio_func(dev_s, ram_s, pio_s);
      pio_func(dev_align_e, ram_align_e, pio_e);
      dma_func(dev_align_s, ram_align_s, dma);
    }
    else {
      pio_func(dev_s, ram_s, size);
    }
  }
}

void pi_write_locked(uint32_t dev_addr, const void *src, size_t size)
{
  do_transfer(dev_addr, (uint32_t)src, size, pio_write, dma_write);
}

void pi_read_locked(uint32_t dev_addr, void *dst, size_t size)
{
  do_transfer(dev_addr, (uint32_t)dst, size, pio_read, dma_read);
}

void pi_write(uint32_t dev_addr, const void *src, size_t size)
{
  __osPiGetAccess();
  pi_write_locked(dev_addr, src, size);
  __osPiRelAccess();
}

void pi_read(uint32_t dev_addr, void *dst, size_t size)
{
  __osPiGetAccess();
  pi_read_locked(dev_addr, dst, size);
  __osPiRelAccess();
}
