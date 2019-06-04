#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <inttypes.h>
#include "menu.h"
#include "util.h"

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

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  uint32_t address = data->address;
  if (address < 0x80000000 || address >= 0x80800000 ||
      data->type < 0 || data->type >= WATCH_TYPE_MAX)
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
  case WATCH_TYPE_F32: {
    float v = *(float*)address;
    if (is_nan(v))
      strcpy(item->text, "nan");
    else if (v == INFINITY)
      strcpy(item->text, "inf");
    else if (v == -INFINITY)
      strcpy(item->text, "-inf");
    else {
      if (!isnormal(v))
        v = 0.f;
      snprintf(item->text, 17, "%g", v);
    }
    break;
  }
  default:
    break;
  }
  return 0;
}

struct menu_item *menu_add_watch(struct menu *menu, int x, int y,
                                 uint32_t address, enum watch_type type)
{
  struct item_data *data = malloc(sizeof(*data));
  data->address = address;
  data->type = type;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0xC0C0C0);
  item->text = malloc(17);
  item->selectable = 0;
  item->data = data;
  item->draw_proc = draw_proc;
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
