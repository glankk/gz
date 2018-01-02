#ifndef ITEM_OPTION_H
#define ITEM_OPTION_H
#include "menu.h"

struct menu_item *item_option_create(struct menu *menu, int x, int y,
                                     int n_items, const int8_t *item_list,
                                     int8_t *value,
                                     struct gfx_texture *texture,
                                     struct gfx_texture *bg_texture,
                                     int bg_texture_tile,
                                     struct gfx_texture *empty_texture,
                                     int empty_texture_tile,
                                     uint32_t color, float scale,
                                     float bg_scale, float empty_scale,
                                     menu_generic_callback callback_proc,
                                     void *callback_data);

#endif
