#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include "menu.h"

struct item_data
{
  struct menu_item *address;
  struct menu_item *type;
  struct menu_item *watch;
};


static int move_proc(struct menu *menu, struct menu_item *item, int x, int y)
{
  struct item_data *data = item->data;
  int x_rel = x - item->x;
  int y_rel = y - item->y;
  menu_item_move(menu, data->address,
                 data->address->x + x_rel, data->address->y + y_rel);
  menu_item_move(menu, data->type,
                 data->type->x + x_rel, data->type->y + y_rel);
  menu_item_move(menu, data->watch,
                 data->watch->x + x_rel, data->watch->y + y_rel);
  return 0;
}

static int remove_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  menu_item_remove(menu, data->address);
  menu_item_remove(menu, data->type);
  menu_item_remove(menu, data->watch);
  return 0;
}

static int address_proc(struct menu_item *item,
                        enum menu_callback_reason reason,
                        void *data)
{
  struct item_data *item_data = data;
  if (reason == MENU_CALLBACK_CHANGED)
    menu_watch_set_address(item_data->watch, menu_intinput_get(item));
  return 0;
}

static int type_proc(struct menu_item *item,
                     enum menu_callback_reason reason,
                     void *data)
{
  struct item_data *item_data = data;
  if (reason == MENU_CALLBACK_CHANGED)
    menu_watch_set_type(item_data->watch, menu_option_get(item));
  return 0;
}

struct menu_item *menu_add_userwatch(struct menu *menu, int x, int y,
                                     uint32_t address, enum watch_type type,
                                     int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->address = menu_add_intinput(menu, x, y, 16, 8, address_proc, data,
                                    priority);
  menu_intinput_set(data->address, address);
  data->type = menu_add_option(menu, x + 9, y,
                               "u8\0""s8\0""x8\0"
                               "u16\0""s16\0""x16\0"
                               "u32\0""s32\0""x32\0"
                               "f32\0",
                               type_proc, data, priority);
  menu_option_set(data->type, type);
  data->watch = menu_add_watch(menu, x + 13, y, address, type);
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, NULL, 0xFFFFFF);
  item->priority = INT_MIN;
  item->data = data;
  item->move_proc = move_proc;
  item->remove_proc = remove_proc;
  return item;
}

struct menu_item *menu_userwatch_address(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->address;
}

struct menu_item *menu_userwatch_type(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->type;
}

struct menu_item *menu_userwatch_watch(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->watch;
}
