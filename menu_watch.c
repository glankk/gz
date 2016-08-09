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
};

static const char *datatype_options =
  "u8\0"
  "s8\0"
  "x8\0"
  "u16\0"
  "s16\0"
  "x16\0"
  "u32\0"
  "s32\0"
  "x32\0"
  "f32\0"
;


static int think_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  uint32_t address = menu_intinput_get(data->address);
  enum data_type type = menu_option_get(data->type);
  if (address == 0)
    return 1;
  switch (type) {
  case DATA_TYPE_U8:
    snprintf(item->text, 17, "%"   PRIu8,  *(uint8_t*) address); break;
  case DATA_TYPE_S8:
    snprintf(item->text, 17, "%"   PRIi8,  *(int8_t*)  address); break;
  case DATA_TYPE_X8:
    snprintf(item->text, 17, "0x%" PRIx8,  *(uint8_t*) address); break;
  case DATA_TYPE_U16:
    snprintf(item->text, 17, "%"   PRIu16, *(uint16_t*)address); break;
  case DATA_TYPE_S16:
    snprintf(item->text, 17, "%"   PRIi16, *(int16_t*) address); break;
  case DATA_TYPE_X16:
    snprintf(item->text, 17, "0x%" PRIx16, *(uint16_t*)address); break;
  case DATA_TYPE_U32:
    snprintf(item->text, 17, "%"   PRIu32, *(uint32_t*)address); break;
  case DATA_TYPE_S32:
    snprintf(item->text, 17, "%"   PRIi32, *(int32_t*) address); break;
  case DATA_TYPE_X32:
    snprintf(item->text, 17, "0x%" PRIx32, *(uint32_t*)address); break;
  case DATA_TYPE_F32:
    snprintf(item->text, 17, "%f",         *(float*)   address); break;
  default:
    return 1;
  }
  return 0;
}

static int move_proc(struct menu *menu, struct menu_item *item, int x, int y)
{
  struct item_data *data = item->data;
  int x_rel = x - item->x;
  int y_rel = y - item->y;
  menu_item_move(menu, data->address,
                 data->address->x + x_rel, data->address->y + y_rel);
  menu_item_move(menu, data->type,
                 data->type->x + x_rel, data->type->y + y_rel);
  return 0;
}

static int remove_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  menu_item_remove(menu, data->address);
  menu_item_remove(menu, data->type);
  return 0;
}

struct menu_item *menu_add_watch(struct menu *menu, int x, int y, int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->address = menu_add_intinput(menu, x, y, 16, 8, NULL, NULL, priority);
  data->type = menu_add_option(menu, x + 9, y, datatype_options, NULL, NULL,
                               priority);
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x + 13, y, NULL, 0xA0A0A0);
  item->text = malloc(17);
  item->priority = INT_MIN;
  item->data = data;
  item->think_proc = think_proc;
  item->move_proc = move_proc;
  item->remove_proc = remove_proc;
  return item;
}

struct menu_item *menu_watch_address(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->address;
}

struct menu_item *menu_watch_type(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->type;
}
