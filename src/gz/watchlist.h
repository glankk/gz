#ifndef WATCHLIST_H
#define WATCHLIST_H
#include "menu.h"

struct menu_item *watchlist_create(struct menu *menu,
                                   struct menu *menu_release,
                                   int x, int y);
void              watchlist_store(struct menu_item *item);
void              watchlist_fetch(struct menu_item *item);
void              watchlist_show(struct menu_item *item);
void              watchlist_hide(struct menu_item *item);
int               watchlist_add(struct menu_item *item, uint32_t address,
                                enum watch_type type);

#endif
