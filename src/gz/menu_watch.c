#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include "menu.h"

struct item_data
{
  uint32_t        address;
  enum watch_type type;
};


static int watch_type_size[] =
{
  1, 1, 1,
  2, 2, 2,
  4, 4, 4,
  4,
};

static int think_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  uint32_t address = data->address;
  if (address == 0)
    return 1;
  address -= address % watch_type_size[data->type];
  switch (data->type) {
  case WATCH_TYPE_U8:
    snprintf(item->text, 17, "%"   PRIu8,  *(uint8_t*) address); break;
  case WATCH_TYPE_S8:
    snprintf(item->text, 17, "%"   PRIi8,  *(int8_t*)  address); break;
  case WATCH_TYPE_X8:
    snprintf(item->text, 17, "0x%" PRIx8,  *(uint8_t*) address); break;
  case WATCH_TYPE_U16:
    snprintf(item->text, 17, "%"   PRIu16, *(uint16_t*)address); break;
  case WATCH_TYPE_S16:
    snprintf(item->text, 17, "%"   PRIi16, *(int16_t*) address); break;
  case WATCH_TYPE_X16:
    snprintf(item->text, 17, "0x%" PRIx16, *(uint16_t*)address); break;
  case WATCH_TYPE_U32:
    snprintf(item->text, 17, "%"   PRIu32, *(uint32_t*)address); break;
  case WATCH_TYPE_S32:
    snprintf(item->text, 17, "%"   PRIi32, *(int32_t*) address); break;
  case WATCH_TYPE_X32:
    snprintf(item->text, 17, "0x%" PRIx32, *(uint32_t*)address); break;
  case WATCH_TYPE_F32:
    snprintf(item->text, 17, "%f",         *(float*)   address); break;
  default:
    return 1;
  }
  return 0;
}

struct menu_item *menu_add_watch(struct menu *menu, int x, int y,
                                 uint32_t address, enum watch_type type)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->address = address;
  data->type = type;
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, NULL, 0xA0A0A0);
  item->text = malloc(17);
  item->priority = INT_MIN;
  item->data = data;
  item->think_proc = think_proc;
  return item;
}

uint32_t menu_watch_get_address(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->address;
}
void menu_watch_set_address(struct menu_item *item, uint32_t address)
{
  struct item_data *data = item->data;
  data->address = address;
}

enum watch_type menu_watch_get_type(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->type;
}
void menu_watch_set_type(struct menu_item *item, enum watch_type type)
{
  struct item_data *data = item->data;
  data->type = type;
}
