#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "menu.h"
#include "input.h"

struct item_data
{
  struct menu_item *address;
  struct menu_item *type;
  struct menu_item *watch;
  struct menu_item *pointer;
  _Bool             edit_offset;
};

static int address_proc(struct menu_item *item,
                        enum menu_callback_reason reason,
                        void *data)
{
  struct item_data *item_data = data;
  if (reason == MENU_CALLBACK_CHANGED) {
    if (!item_data->edit_offset) {
      menu_watch_set_address(item_data->watch, menu_intinput_get(item));
    } else {
      menu_watch_set_offset(item_data->watch, menu_intinput_get(item));
    }
  }
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

static int pointer_proc(struct menu_item *item,
                        enum menu_callback_reason reason,
                        void *data)
{
  struct item_data *item_data = data;
  if (reason == MENU_CALLBACK_SWITCH_OFF) {
    if (input_pad() & BUTTON_Z) {
      if (!item_data->edit_offset) {
        menu_intinput_set(item_data->address,
                          menu_watch_get_offset(item_data->watch));
        item_data->edit_offset = 1;
      } else {
        menu_intinput_set(item_data->address,
                          menu_watch_get_address(item_data->watch));
        item_data->edit_offset = 0;
      }
      return 1;
    } else {
      menu_watch_set_pointer(item_data->watch, 0);
      menu_intinput_set(item_data->address,
                        menu_watch_get_address(item_data->watch));
      item_data->edit_offset = 0;
    }
  }
  else if (reason == MENU_CALLBACK_SWITCH_ON) {
    menu_watch_set_pointer(item_data->watch, 1);
  }
  return 0;
}

struct menu_item *menu_add_userwatch(struct menu *menu, int x, int y,
                                     uint32_t address, enum watch_type type,
                                     _Bool pointer, int offset)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  data->pointer = menu_add_checkbox(imenu, 0, 0, pointer_proc, data);
  data->edit_offset = 0;
  menu_checkbox_set(data->pointer, pointer);
  menu_checkbox_set_type(data->pointer, MENU_CHECKBOX_POINTER);
  data->address = menu_add_intinput(imenu, 2, 0, 16, 8, address_proc, data);
  menu_intinput_set(data->address, address);
  data->type = menu_add_option(imenu, 11, 0,
                               "u8\0""s8\0""x8\0"
                               "u16\0""s16\0""x16\0"
                               "u32\0""s32\0""x32\0"
                               "f32\0",
                               type_proc, data);
  menu_option_set(data->type, type);
  data->watch = menu_add_watch(imenu, 15, 0, address, type, pointer, offset);
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

struct menu_item *menu_userwatch_pointer(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->pointer;
}