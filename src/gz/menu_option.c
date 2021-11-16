#include <stdlib.h>
#include <string.h>
#include <vector/vector.h>
#include "menu.h"

struct item_data
{
  struct vector           options;
  menu_generic_callback   callback_proc;
  void                   *callback_data;
  int                     value;
  _Bool                   active;
};

static int think_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->callback_proc) {
    int r = data->callback_proc(item, MENU_CALLBACK_THINK, data->callback_data);
    if (r)
      return r;
    if (data->active)
      r = data->callback_proc(item, MENU_CALLBACK_THINK_ACTIVE,
                              data->callback_data);
    else
      r = data->callback_proc(item, MENU_CALLBACK_THINK_INACTIVE,
                              data->callback_data);
    return r;
  }
  return 0;
}

static int navigate_proc(struct menu_item *item,
                         enum menu_navigation nav)
{
  struct item_data *data = item->data;
  if (data->callback_proc &&
      data->callback_proc(item, MENU_CALLBACK_NAV_UP + nav,
                          data->callback_data))
    return 1;
  int value = data->value;
  switch (nav) {
    case MENU_NAVIGATE_UP:    value += 1; break;
    case MENU_NAVIGATE_DOWN:  value -= 1; break;
    case MENU_NAVIGATE_LEFT:  value -= 3; break;
    case MENU_NAVIGATE_RIGHT: value += 3; break;
  }
  value %= (int)data->options.size;
  if (value < 0)
    value += (int)data->options.size;
  data->value = value;
  char **option = vector_at(&data->options, data->value);
  item->text = *option;
  if (data->callback_proc)
    data->callback_proc(item, MENU_CALLBACK_CHANGED, data->callback_data);
  return 1;
};

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->active) {
    if (data->callback_proc && data->callback_proc(item,
                                                   MENU_CALLBACK_DEACTIVATE,
                                                   data->callback_data))
      return 1;
    item->navigate_proc = NULL;
    item->animate_highlight = 0;
  }
  else {
    if (data->callback_proc && data->callback_proc(item,
                                                   MENU_CALLBACK_ACTIVATE,
                                                   data->callback_data))
      return 1;
    item->navigate_proc = navigate_proc;
    item->animate_highlight = 1;
  }
  data->active = !data->active;
  return 1;
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  item->text = NULL;
  for (size_t i = 0; i < data->options.size; ++i) {
    char **option = vector_at(&data->options, i);
    free(*option);
  }
  vector_destroy(&data->options);
  return 0;
}

struct menu_item *menu_add_option(struct menu *menu, int x, int y,
                                  const char *options,
                                  menu_generic_callback callback_proc,
                                  void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  vector_init(&data->options, sizeof(char *));
  for (const char *option = options; *option;) {
    size_t option_length = strlen(option);
    char *new_option = malloc(option_length + 1);
    strcpy(new_option, option);
    vector_push_back(&data->options, 1, &new_option);
    option += option_length + 1;
  }
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->value = 0;
  data->active = 0;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0xFFFFFF);
  char **option = vector_at(&data->options, data->value);
  item->text = *option;
  item->data = data;
  item->think_proc = think_proc;
  item->activate_proc = activate_proc;
  item->destroy_proc = destroy_proc;
  return item;
}

int menu_option_get(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->value;
}

void menu_option_set(struct menu_item *item, int value)
{
  struct item_data *data = item->data;
  data->value = value;
  char **option = vector_at(&data->options, data->value);
  item->text = *option;
}
