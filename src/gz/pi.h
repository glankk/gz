#ifndef PI_H
#define PI_H
#include <stddef.h>
#include <stdint.h>

void pi_write_locked(uint32_t dev_addr, const void *src, size_t size);
void pi_read_locked(uint32_t dev_addr, void *dst, size_t size);
void pi_write(uint32_t dev_addr, const void *src, size_t size);
void pi_read(uint32_t dev_addr, void *dst, size_t size);

#endif
