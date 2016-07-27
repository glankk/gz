#include <stdlib.h>
#include <limits.h>
#include "menu.h"

struct item_data
{
  struct menu_item     *name;
  menu_switch_callback  callback_proc;
  void                 *callback_data;
  int                   state;
};


static void update(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->state) {
    item->text[0] = '-';
    item->color = 0xFFA0A0;
  }
  else {
    item->text[0] = '+';
    item->color = 0xA0FFA0;
  }
}

static int activate_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  if (!data->callback_proc || !data->callback_proc(item,
                                                  data->state ?
                                                  MENU_CALLBACK_SWITCH_OFF :
                                                  MENU_CALLBACK_SWITCH_ON,
                                                  data->callback_data))
  {
    data->state = !data->state;
    update(item);
  }
  return 1;
}

static int move_proc(struct menu *menu, struct menu_item *item, int x, int y)
{
  struct item_data *data = item->data;
  int x_rel = x - item->x;
  int y_rel = y - item->y;
  menu_item_move(menu, data->name,
                 data->name->x + x_rel, data->name->y + y_rel);
  return 0;
}

static int remove_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  menu_item_remove(menu, data->name);
  return 0;
}

static int think_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->callback_proc)
    data->callback_proc(item, MENU_CALLBACK_THINK, data->callback_data);
  return 0;
}

struct menu_item *menu_add_switch(struct menu *menu, int x, int y,
                                  const char *name,
                                  menu_switch_callback callback_proc,
                                  void *callback_data, int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->name = menu_add_item(menu, NULL);
  menu_item_init(data->name, x + 2, y, name, 0xFFFFFF);
  data->name->priority = INT_MIN;
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->state = 0;
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, "+", 0xA0FFA0);
  item->priority = priority;
  item->data = data;
  item->activate_proc = activate_proc;
  item->think_proc = think_proc;
  item->move_proc = move_proc;
  item->remove_proc = remove_proc;
  return item;
}

void menu_switch_set(struct menu_item *item, int state)
{
  struct item_data *data = item->data;
  data->state = state;
  update(item);
}

int menu_switch_get(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->state;
}

void menu_switch_toggle(struct menu_item *item)
{
  struct item_data *data = item->data;
  data->state = !data->state;
  update(item);
}
