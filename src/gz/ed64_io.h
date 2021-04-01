#ifndef ED64_IO_H
#define ED64_IO_H

void ed64_get_access(void);
void ed64_rel_access(void);

struct iodev;

extern struct iodev everdrive64_v1;
extern struct iodev everdrive64_v2;
extern struct iodev everdrive64_x;

#endif
