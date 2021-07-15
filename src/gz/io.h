#ifndef IO_H
#define IO_H

int io_init(void);

int disk_init(void);
int disk_read(size_t lba, size_t n_blocks, void *dst);
int disk_write(size_t lba, size_t n_blocks, const void *dst);

int fifo_poll(void);
int fifo_read(void *dst, size_t n_blocks);
int fifo_write(const void *src, size_t n_blocks);

unsigned clock_ticks(void);
unsigned clock_freq(void);

void cpu_reset(void);

static inline unsigned msec_from_now(unsigned msec)
{
  return clock_ticks() + clock_freq() / 1000 * msec;
}

static inline int clock_after(unsigned ticks)
{
  return (int)clock_ticks() - (int)ticks > 0;
}

#endif
