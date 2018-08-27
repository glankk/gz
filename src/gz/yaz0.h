#ifndef YAZ0_H
#define YAZ0_H
#include <stdint.h>

void yaz0_begin(uint32_t prom_start);
char yaz0_get_byte(void);
void yaz0_advance(uint32_t n_bytes);

#endif
