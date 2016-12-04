#include <stdlib.h>
#include "menu.h"

struct item_data
{
  struct menu_item       *state_item;
  menu_generic_callback   callback_proc;
  void                   *callback_data;
  _Bool                   state;
};

static void update(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->state) {
    data->state_item->text[0] = '-';
    data->state_item->color = 0xFF8080;
  }
  else {
    data->state_item->text[0] = '+';
    data->state_item->color = 0x80FF80;
  }
}

static int think_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->callback_proc)
    return data->callback_proc(item, MENU_CALLBACK_THINK, data->callback_data);
  return 0;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (!data->callback_proc ||
      !data->callback_proc(item, data->state ? MENU_CALLBACK_SWITCH_OFF :
                           MENU_CALLBACK_SWITCH_ON, data->callback_data))
  {
    data->state = !data->state;
    update(item);
    if (data->callback_proc)
      data->callback_proc(item, MENU_CALLBACK_CHANGED, data->callback_data);
  }
  return 1;
}

struct menu_item *menu_add_switch(struct menu *menu, int x, int y,
                                  const char *name,
                                  menu_generic_callback callback_proc,
                                  void *callback_data)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  menu_add_static(imenu, 2, 0, name, 0xC0C0C0);
  data->state_item = menu_item_add(imenu, 0, 0, "+", 0x80FF80);
  data->state_item->data = data;
  data->state_item->think_proc = think_proc;
  data->state_item->activate_proc = activate_proc;
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->state = 0;
  item->data = data;
  return item;
}

void menu_switch_set(struct menu_item *item, _Bool state)
{
  struct item_data *data = item->data;
  data->state = state;
  update(item);
}

_Bool menu_switch_get(struct menu_item *item)
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
