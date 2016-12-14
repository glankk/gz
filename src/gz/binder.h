#ifndef BINDER_H
#define BINDER_H
#include "menu.h"

struct menu_item *binder_create(struct menu *menu, int x, int y,
                                int command_index, _Bool *override_input);

#endif
