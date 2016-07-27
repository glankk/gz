#include <stdlib.h>
#include <string.h>
#include <vector/vector.h>
#include "menu.h"

struct item_data
{
  struct vector options;
  int           value;
  int           active;
};


static int navigate_proc(struct menu* menu, struct menu_item *item,
                         enum menu_navigation nav)
{
  struct item_data *data = item->data;
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
                                  const char *options, int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  vector_init(&data->options, sizeof(char*));
  for (const char *option = options; *option;) {
    size_t option_length = strlen(option);
    char *new_option = malloc(option_length + 1);
    strcpy(new_option, option);
    vector_push_back(&data->options, 1, &new_option);
    option += option_length + 1;
  }
  data->value = 0;
  data->active = 0;
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, NULL, 0xFFFFFF);
  char **option = vector_at(&data->options, data->value);
  item->text = *option;
  item->priority = priority;
  item->data = data;
  item->activate_proc = activate_proc;
  item->remove_proc = remove_proc;
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
