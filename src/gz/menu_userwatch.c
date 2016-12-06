#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "menu.h"

struct item_data
{
  struct menu_item *address;
  struct menu_item *type;
  struct menu_item *watch;
};

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
                                     uint32_t address, enum watch_type type)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  data->address = menu_add_intinput(imenu, 0, 0, 16, 8, address_proc, data);
  menu_intinput_set(data->address, address);
  data->type = menu_add_option(imenu, 9, 0,
                               "u8\0""s8\0""x8\0"
                               "u16\0""s16\0""x16\0"
                               "u32\0""s32\0""x32\0"
                               "f32\0",
                               type_proc, data);
  menu_option_set(data->type, type);
  data->watch = menu_add_watch(imenu, 13, 0, address, type);
  item->data = data;
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
