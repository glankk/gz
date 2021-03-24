#include <errno.h>
#include <n64.h>
#include "ed64_io.h"
#include "hb_io.h"
#include "iodev.h"
#include "zu.h"

static unsigned int clock_ticks_dflt(void)
{
  unsigned int count;
  __asm__ ("mfc0    %0, $9;" : "=r"(count));
  return count;
}

static unsigned int clock_freq_dflt(void)
{
  return OS_CPU_COUNTER;
}

static void cpu_reset_dflt(void)
{
  return zu_reset();
}

static struct iodev *current_dev;

int io_init(void)
{
  struct iodev *devs[] = {
    &homeboy_iodev,
    &everdrive64_x,
    &everdrive64_v2,
    &everdrive64_v1,
  };

  int n_devs = sizeof(devs) / sizeof(devs[0]);
  for (int i = 0; i < n_devs; i++) {
    current_dev = devs[i];
    if (current_dev->probe() == 0)
      return 0;
  }

  current_dev = NULL;
  errno = ENODEV;
  return -1;
}

int disk_init(void)
{
  if (current_dev && current_dev->disk_init) {
    if (current_dev->disk_init()) {
      errno = ENODEV;
      return -1;
    }
    else
      return 0;
  }
  else {
    errno = ENODEV;
    return -1;
  }
}

int disk_read(size_t lba, size_t n_blocks, void *dst)
{
  if (current_dev && current_dev->disk_read) {
    if (current_dev->disk_read(lba, n_blocks, dst)) {
      errno = EIO;
      return -1;
    }
    else
      return 0;
  }
  else {
    errno = ENODEV;
    return -1;
  }
}

int disk_write(size_t lba, size_t n_blocks, const void *src)
{
  if (current_dev && current_dev->disk_write) {
    if (current_dev->disk_write(lba, n_blocks, src)) {
      errno = EIO;
      return -1;
    }
    else
      return 0;
  }
  else {
    errno = ENODEV;
    return -1;
  }
}

int fifo_poll(void)
{
  if (current_dev && current_dev->fifo_poll) {
    return current_dev->fifo_poll();
  }
  else {
    errno = ENODEV;
    return 0;
  }
}

int fifo_read(void *dst, size_t n_blocks)
{
  if (current_dev && current_dev->fifo_read) {
    if (current_dev->fifo_read(dst, n_blocks)) {
      errno = EIO;
      return -1;
    }
    else
      return 0;
  }
  else {
    errno = ENODEV;
    return -1;
  }
}

int fifo_write(const void *src, size_t n_blocks)
{
  if (current_dev && current_dev->fifo_write) {
    if (current_dev->fifo_write(src, n_blocks)) {
      errno = EIO;
      return -1;
    }
    else
      return 0;
  }
  else {
    errno = ENODEV;
    return -1;
  }
}

unsigned int clock_ticks(void)
{
  if (current_dev && current_dev->clock_ticks)
    return current_dev->clock_ticks();
  else
    return clock_ticks_dflt();
}

unsigned int clock_freq(void)
{
  if (current_dev && current_dev->clock_freq)
    return current_dev->clock_freq();
  else
    return clock_freq_dflt();
}

int cpu_reset(void)
{
  if (current_dev && current_dev->cpu_reset)
    current_dev->cpu_reset();
  else
    cpu_reset_dflt();

  return 0;
}
