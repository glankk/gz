#include <stdlib.h>
#include "menu.h"

struct item_data
{
  menu_action_callback  callback_proc;
  void                 *callback_data;
  int                   anim_state;
};

static int enter_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  data->anim_state = 0;
  return 0;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  data->callback_proc(item, data->callback_data);
  data->anim_state = 1;
  return 1;
}

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  if (data->anim_state > 0) {
    ++draw_params->x;
    ++draw_params->y;
    data->anim_state = (data->anim_state + 1) % 3;
  }
  return 0;
}

struct menu_item *menu_add_button(struct menu *menu, int x, int y,
                                  const char *name,
                                  menu_action_callback callback_proc,
                                  void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->anim_state = 0;
  struct menu_item *item = menu_item_add(menu, x, y, name, 0xFFFFFF);
  item->data = data;
  item->enter_proc = enter_proc;
  item->draw_proc = draw_proc;
  item->activate_proc = activate_proc;
  return item;
}

void *menu_button_callback_data(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->callback_data;
}
