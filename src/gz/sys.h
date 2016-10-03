#ifndef SYS_H
#define SYS_H

#ifdef __cplusplus
extern "C"
{
#endif

void set_stdout(int (*handler)(const char *ptr, int size));

#ifdef __cplusplus
}
#endif

#endif
