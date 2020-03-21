#ifndef START_H
#define START_H

void _start();
void *gz_leave();

__attribute__((section(".data")))
extern void *gz_leave_func;

#endif
