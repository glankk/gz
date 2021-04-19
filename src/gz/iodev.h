#ifndef IODEV_H
#define IODEV_H

struct iodev
{
  int           (*probe)        (void);

  int           (*disk_init)    (void);
  int           (*disk_read)    (size_t lba, size_t n_blocks, void *dst);
  int           (*disk_write)   (size_t lba, size_t n_blocks, const void *src);

  int           (*fifo_poll)    (void);
  int           (*fifo_read)    (void *dst, size_t n_blocks);
  int           (*fifo_write)   (const void *src, size_t n_blocks);

  unsigned int  (*clock_ticks)  (void);
  unsigned int  (*clock_freq)   (void);
  void          (*cpu_reset)    (void);
};

#endif
