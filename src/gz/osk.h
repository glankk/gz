#ifndef OSK_H
#define OSK_H
#include "menu.h"

typedef int (*osk_callback_t)(const char *str, void *data);

void menu_get_osk_string(struct menu *menu, const char *dflt,
                         osk_callback_t callback_proc, void *callback_data);

#endif
