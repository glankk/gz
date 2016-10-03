#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef __cplusplus
extern "C"
{
#endif

void console_init(int width, int height);
void console_set_view(int x, int y, int width, int height);
void console_scroll(int cx, int cy);
void console_print();

#ifdef __cplusplus
}
#endif

#endif
