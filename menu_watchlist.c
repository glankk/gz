#include <stdlib.h>
#include <vector/vector.h>
#include "menu.h"

struct member
{
  struct menu_item *minus;
  struct menu_item *watch;
};

struct item_data
{
  struct vector members;
  struct menu_item *plus;
};

struct minus_data
{
  struct item_data *data;
  int index;
};


static int minus_activate_proc(struct menu *menu, struct menu_item *item)
{
  menu_navigate(menu, MENU_NAVIGATE_DOWN);
  struct minus_data *minus_data = item->data;
  struct item_data *data = minus_data->data;
  for (int i = minus_data->index + 1; i < data->members.size; ++i) {
    struct member *member = vector_at(&data->members, i);
    menu_item_move(menu, member->minus,
                   member->minus->x, member->minus->y - 1);
    struct minus_data *member_minus_data = member->minus->data;
    --member_minus_data->index;
  }
  menu_item_remove(menu, item);
  --data->plus->y;
  return 1;
}

static int minus_move_proc(struct menu *menu, struct menu_item *item,
                           int x, int y)
{
  struct minus_data *minus_data = item->data;
  struct item_data *data = minus_data->data;
  struct member *member = vector_at(&data->members, minus_data->index);
  struct menu_item *watch = member->watch;
  int x_rel = x - item->x;
  int y_rel = y - item->y;
  menu_item_move(menu, watch, watch->x + x_rel, watch->y + y_rel);
  return 0;
}

static int minus_remove_proc(struct menu *menu, struct menu_item *item)
{
  struct minus_data *minus_data = item->data;
  struct item_data *data = minus_data->data;
  struct member *member = vector_at(&data->members, minus_data->index);
  menu_item_remove(menu, member->watch);
  vector_erase(&data->members, minus_data->index, 1);
  item->text = NULL;
  return 0;
}

static int plus_activate_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->members.size >= 20)
    return 0;
  struct minus_data *minus_data = malloc(sizeof(struct minus_data));
  minus_data->data = data;
  minus_data->index = data->members.size;
  struct menu_item *minus = menu_add_item(menu, NULL);
  menu_item_init(minus, item->x, item->y, "-", 0xFFA0A0);
  minus->priority = item->priority;
  minus->data = minus_data;
  minus->activate_proc = minus_activate_proc;
  minus->move_proc = minus_move_proc;
  minus->remove_proc = minus_remove_proc;
  struct menu_item *watch = menu_add_watch(menu, item->x + 2, item->y,
                                           item->priority);
  if (data->members.size > 0) {
    struct member *last_member = vector_at(&data->members,
                                           data->members.size - 1);
    struct menu_item *last_watch = last_member->watch;
    uint32_t address = menu_address_get(menu_watch_address(last_watch));
    enum data_type type = menu_datatype_get(menu_watch_type(last_watch));
    menu_address_set(menu_watch_address(watch), address);
    menu_datatype_set(menu_watch_type(watch), type);
  }
  struct member member = {minus, watch};
  vector_push_back(&data->members, 1, &member);
  ++item->y;
  return 1;
}

static int plus_move_proc(struct menu *menu, struct menu_item *item,
                          int x, int y)
{
  struct item_data *data = item->data;
  int x_rel = x - item->x;
  int y_rel = y - item->y;
  for (int i = 0; i < data->members.size; ++i) {
    struct member *member = vector_at(&data->members, i);
    struct menu_item *minus = member->minus;
    menu_item_move(menu, minus, minus->x + x_rel, minus->y + y_rel);
  }
  return 0;
}

static int plus_remove_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = data->members.size - 1; i >= 0; --i) {
    struct member *member = vector_at(&data->members, i);
    menu_item_remove(menu, member->minus);
  }
  vector_destroy(&data->members);
  item->text = NULL;
  return 0;
}

struct menu_item *menu_add_watchlist(struct menu *menu, int x, int y,
                                     int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  vector_init(&data->members, sizeof(struct member));
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, "+", 0xA0FFA0);
  item->priority = priority;
  item->data = data;
  item->activate_proc = plus_activate_proc;
  item->move_proc = plus_move_proc;
  item->remove_proc = plus_remove_proc;
  data->plus = item;
  return item;
}
