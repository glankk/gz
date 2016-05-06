#include <stdlib.h>
#include "menu.h"

static char *const data_type_name[DATA_TYPE_MAX] =
{
  "u8", "s8", "x8",
  "u16", "s16", "x16",
  "u32", "s32", "x32", "f32",
};

struct item_data
{
  enum data_type  type;
  int             active;
};


static int navigate_proc(struct menu* menu, struct menu_item *item,
                         enum menu_navigation nav)
{
  struct item_data *data = item->data;
  int value = data->type;
  value += (1 - nav / 2 * 3) * (1 - nav % 2 * 2);
  value = value % DATA_TYPE_MAX;
  if (value < 0)
    value += DATA_TYPE_MAX;
  data->type = value;
  item->text = data_type_name[value];
  return 1;
};

static int activate_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->active) {
    item->navigate_proc = NULL;
    menu->animate_highlight = 0;
  }
  else {
    item->navigate_proc = navigate_proc;
    menu->animate_highlight = 1;
  }
  data->active = !data->active;
  return 1;
}

static int remove_proc(struct menu *menu, struct menu_item *item)
{
  item->text = NULL;
  return 0;
}

struct menu_item *menu_add_datatype(struct menu *menu, int x, int y,
                                    int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->type = DATA_TYPE_U8;
  data->active = 0;
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, NULL, 0xFFFFFF);
  item->text = data_type_name[data->type];
  item->priority = priority;
  item->data = data;
  item->activate_proc = activate_proc;
  item->remove_proc = remove_proc;
  return item;
}

enum data_type menu_datatype_get(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->type;
}

void menu_datatype_set(struct menu_item *item, enum data_type type)
{
  struct item_data *data = item->data;
  data->type = type;
  item->text = data_type_name[data->type];
}
