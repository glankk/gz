#include <stdlib.h>
#include <vector/vector.h>
#include "menu.h"
#include "settings.h"

struct item_data
{
  struct menu      *imenu;
  struct vector     members;
  struct menu_item *add_button;
};

struct member_data
{
  struct menu_item *member;
  struct menu_item *remove_button;
  struct menu_item *userwatch;
};

struct remove_button_data
{
  struct item_data *data;
  int               index;
};

static void remove_button_proc(struct menu_item *item, void *data);

static struct member_data *get_member(struct item_data *data, int index)
{
  if (index < 0 || index >= data->members.size)
    return NULL;
  struct member_data **member_data = vector_at(&data->members, index);
  return *member_data;
}

static int add_member(struct item_data *data,
                      uint32_t address, enum watch_type type, int position)
{
  if (data->members.size >= SETTINGS_WATCHES_MAX ||
      position < 0 || position > data->members.size)
    return 0;
  ++data->add_button->y;
  for (int i = position; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    ++member_data->member->y;
    struct remove_button_data
    *remove_button_data = menu_button_callback_data(member_data->remove_button);
    ++remove_button_data->index;
  }
  struct menu *imenu;
  struct menu_item *member = menu_add_imenu(data->imenu, 0, position, &imenu);
  struct remove_button_data
  *remove_button_data = malloc(sizeof(*remove_button_data));
  remove_button_data->data = data;
  remove_button_data->index = position;
  struct menu_item *remove_button = menu_add_button(imenu, 0, 0, "-",
                                                    remove_button_proc,
                                                    remove_button_data);
  remove_button->color = 0xFFA0A0;
  struct menu_item *userwatch = menu_add_userwatch(imenu, 2, 0, address, type);
  struct member_data *member_data = malloc(sizeof(*member_data));
  member_data->member = member;
  member_data->remove_button = remove_button;
  member_data->userwatch = userwatch;
  vector_insert(&data->members, position, 1, &member_data);
  return 1;
}

static int remove_member(struct item_data *data, int position)
{
  if (position < 0 || position >= data->members.size)
    return 0;
  menu_navigate_top(data->imenu, MENU_NAVIGATE_DOWN);
  --data->add_button->y;
  for (int i = position + 1; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    --member_data->member->y;
    struct remove_button_data
    *remove_button_data = menu_button_callback_data(member_data->remove_button);
    --remove_button_data->index;
  }
  struct member_data *member_data = get_member(data, position);
  free(menu_button_callback_data(member_data->remove_button));
  menu_item_remove(member_data->member);
  vector_erase(&data->members, position, 1);
  free(member_data);
  return 1;
}

static void add_button_proc(struct menu_item *item, void *data)
{
  struct item_data *item_data = data;
  uint32_t address = 0x00000000;
  enum watch_type type = WATCH_TYPE_U8;
  if (item_data->members.size > 0) {
    struct member_data *member_data = get_member(item_data,
                                                 item_data->members.size - 1);
    struct menu_item *last_watch = menu_userwatch_watch(member_data->userwatch);
    address = menu_watch_get_address(last_watch);
    type = menu_watch_get_type(last_watch);
  }
  add_member(item_data, address, type, item_data->members.size);
}

static void remove_button_proc(struct menu_item *item, void *data)
{
  struct remove_button_data *remove_button_data = data;
  remove_member(remove_button_data->data, remove_button_data->index);
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  vector_destroy(&data->members);
  return 0;
}

struct menu_item *watchlist_create(struct menu *menu, int x, int y)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  data->imenu = imenu;
  vector_init(&data->members, sizeof(struct member_data*));
  data->add_button = menu_add_button(imenu, 0, 0, "+", add_button_proc, data);
  data->add_button->color = 0xA0FFA0;
  item->data = data;
  item->destroy_proc = destroy_proc;
  return item;
}

void watchlist_store(struct menu_item *item)
{
  struct item_data *data = item->data;
  settings->no_watches = data->members.size;
  for (int i = 0; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
    settings->watch_address[i] = menu_watch_get_address(watch);
    settings->watch_type[i] = menu_watch_get_type(watch);
  }
}

void watchlist_fetch(struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = data->members.size - 1; i >= 0; --i)
    remove_member(data, i);
  for (int i = 0; i < settings->no_watches; ++i)
    add_member(data, settings->watch_address[i], settings->watch_type[i], i);
}
