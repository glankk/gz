#include <stdlib.h>
#include <vector/vector.h>
#include "menu.h"

struct item_data
{
  struct menu      *imenu;
  struct vector     members;
  struct menu_item *add_item;
};

struct member_data
{
  struct menu_item *member;
  struct menu_item *remove_item;
  struct menu_item *userwatch;
};

struct remove_item_data
{
  struct item_data *data;
  int               index;
};

static void remove_item_proc(struct menu_item *item, void *data)
{
  struct remove_item_data *remove_item_data = data;
  struct item_data *item_data = remove_item_data->data;
  int index = remove_item_data->index;
  menu_navigate_top(item_data->imenu, MENU_NAVIGATE_DOWN);
  for (int i = index + 1; i < item_data->members.size; ++i) {
    struct member_data **member_data = vector_at(&item_data->members, i);
    --(*member_data)->member->y;
    struct remove_item_data *remove_item_data =
      menu_button_callback_data((*member_data)->remove_item);
    --remove_item_data->index;
  }
  --item_data->add_item->y;
  struct member_data **member_data = vector_at(&item_data->members, index);
  menu_item_remove((*member_data)->member);
  free(remove_item_data);
  free(*member_data);
  vector_erase(&item_data->members, index, 1);
}

static void add_item_proc(struct menu_item *item, void *data)
{
  struct item_data *item_data = data;
  if (item_data->members.size >= 20)
    return;
  struct menu *imenu;
  struct menu_item *member = menu_add_imenu(item_data->imenu, 0,
                                            item_data->members.size, &imenu);
  struct remove_item_data *remove_item_data = malloc(sizeof(*remove_item_data));
  remove_item_data->data = item_data;
  remove_item_data->index = item_data->members.size;
  struct menu_item *remove_item = menu_add_button(imenu, 0, 0, "-",
                                                  remove_item_proc,
                                                  remove_item_data);
  remove_item->color = 0xFFA0A0;
  uint32_t address = 0x00000000;
  enum watch_type type = WATCH_TYPE_U8;
  if (item_data->members.size > 0) {
    struct member_data **last_member = vector_at(&item_data->members,
                                                 item_data->members.size - 1);
    struct menu_item *last_watch = menu_userwatch_watch((*last_member)->
                                                        userwatch);
    address = menu_watch_get_address(last_watch);
    type = menu_watch_get_type(last_watch);
  }
  struct menu_item *userwatch = menu_add_userwatch(imenu, 2, 0, address, type);
  struct member_data *member_data = malloc(sizeof(*member_data));
  member_data->member = member;
  member_data->remove_item = remove_item;
  member_data->userwatch = userwatch;
  vector_push_back(&item_data->members, 1, &member_data);
  ++item_data->add_item->y;
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  vector_destroy(&data->members);
  return 0;
}

struct menu_item *menu_add_watchlist(struct menu *menu, int x, int y)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  data->imenu = imenu;
  vector_init(&data->members, sizeof(struct member_data*));
  data->add_item = menu_add_button(imenu, 0, 0, "+", add_item_proc, data);
  data->add_item->color = 0xA0FFA0;
  item->data = data;
  item->destroy_proc = destroy_proc;
  return item;
}
