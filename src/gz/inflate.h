#ifndef INFLATE_H
#define INFLATE_H

void inflate_begin(uint32_t prom_start);
char inflate_get_byte(void);
void inflate_read(void *dst, uint32_t num);
void inflate_advance(uint32_t num);
int inflate_atend(void);

#endif
