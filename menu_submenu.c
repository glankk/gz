#include "menu.h"


static int activate_proc(struct menu *menu, struct menu_item *item)
{
  if (item->data)
    menu_enter(menu, item->data);
  else
    menu_return(menu);
  return 1;
}

struct menu_item *menu_add_submenu(struct menu *menu, int x, int y,
                                   struct menu *submenu, const char *name,
                                   int priority)
{
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, name, 0xFFFFFF);
  item->priority = priority;
  item->data = submenu;
  item->activate_proc = activate_proc;
  return item;
}
