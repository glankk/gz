#include <stdlib.h>
#include "menu.h"

struct item_data
{
  menu_button_callback  callback_proc;
  void                 *callback_data;
};


static int activate_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  data->callback_proc(item, data->callback_data);
  return 1;
}

struct menu_item *menu_add_button(struct menu *menu, int x, int y,
                                  const char *name,
                                  menu_button_callback callback_proc,
                                  void *callback_data, int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, name, 0xFFFFFF);
  item->priority = priority;
  item->data = data;
  item->activate_proc = activate_proc;
  return item;
}
