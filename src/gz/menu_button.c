#include <stdlib.h>
#include "menu.h"

struct item_data
{
  menu_action_callback  callback_proc;
  void                 *callback_data;
};

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  data->callback_proc(item, data->callback_data);
  return 1;
}

struct menu_item *menu_add_button(struct menu *menu, int x, int y,
                                  const char *name,
                                  menu_action_callback callback_proc,
                                  void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  struct menu_item *item = menu_item_add(menu, x, y, name, 0xFFFFFF);
  item->data = data;
  item->activate_proc = activate_proc;
  return item;
}

void *menu_button_callback_data(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->callback_data;
}
