#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "menu.h"

struct item_data
{
  int                 base;
  int                 length;
  uint32_t            value;
  int                 active;
  struct menu_item   *parent;
  struct menu_item  **children;
  struct menu_item   *selector;
};


static inline int char_to_int(int x)
{
  if (x >= '0' && x <= '9')
    return x - ('0' - 0x0);
  else if (x >= 'a' && x <= 'f')
    return x - ('a' - 0xA);
  return -1;
}

static inline int int_to_char(int x)
{
  if (x >= 0x0 && x <= 0x9)
    return x + ('0' - 0x0);
  else if (x >= 0xA && x <= 0xF)
    return x + ('a' - 0xA);
  return -1;
}

static int activate_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->active) {
    data->selector = menu->selector;
    menu->selector = data->parent;
    menu->animate_highlight = 0;
    uint32_t value = 0;
    uint32_t mul = 1;
    for (int i = data->length - 1; i >= 0; --i) {
      int n = data->children[i]->text[0];
      data->parent->text[i] = n;
      value += char_to_int(n) * mul;
      data->children[i]->priority = INT_MIN;
      mul *= data->base;
    }
    data->value = value;
  }
  else {
    menu->selector = data->selector;
    menu->animate_highlight = 1;
    for (int i = 0; i < data->length; ++i)
      data->children[i]->priority = data->parent->priority + 1;
  }
  data->active = !data->active;
  return 1;
}

static int move_proc(struct menu *menu, struct menu_item *item, int x, int y)
{
  struct item_data *data = item->data;
  int x_rel = x - item->x;
  int y_rel = y - item->y;
  data->parent->x += x_rel;
  data->parent->y += y_rel;
  for (int i = 0; i < data->length; ++i) {
    data->children[i]->x += x_rel;
    data->children[i]->y += y_rel;
  }
  return 1;
}

static int remove_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = 0; i < data->length; ++i) {
    data->children[i]->remove_proc = NULL;
    data->children[i]->data = NULL;
    menu_item_remove(menu, data->children[i]);
  }
  free(data->children);
  data->parent->remove_proc = NULL;
  menu_item_remove(menu, data->parent);
  return 1;
}

static int parent_think_proc(struct menu *menu, struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->active;
}

static int child_think_proc(struct menu *menu, struct menu_item *item)
{
  item->color = menu->highlight_color_static;
  struct item_data *data = item->data;
  return !data->active;
}

static int child_navigate_proc(struct menu *menu, struct menu_item *item,
                               enum menu_navigation nav)
{
  struct item_data *data = item->data;
  int value = char_to_int(item->text[0]);
  if (nav == MENU_NAVIGATE_UP)
    ++value;
  else if (nav == MENU_NAVIGATE_DOWN)
    --value;
  else
    return 0;
  value = value % data->base;
  if (value < 0)
    value += data->base;
  item->text[0] = int_to_char(value);
  return 1;
};

struct menu_item *menu_add_intinput(struct menu *menu, int x, int y,
                                    int base, int length, int priority)
{
  struct item_data *data = malloc(sizeof(struct item_data));
  data->base = base;
  data->length = length;
  data->value = 0;
  data->active = 0;
  data->children = malloc(sizeof(struct menu_item) * length);
  struct menu_item *parent = data->parent = menu_add_item(menu, NULL);
  menu_item_init(parent, x, y, NULL, 0xFFFFFF);
  parent->text = malloc(length + 1);
  parent->text[length] = 0;
  parent->priority = priority;
  parent->data = data;
  parent->think_proc = parent_think_proc;
  parent->activate_proc = activate_proc;
  parent->move_proc = move_proc;
  parent->remove_proc = remove_proc;
  for (int i = 0; i < length; ++i) {
    parent->text[i] = '0';
    struct menu_item *child = data->children[i] = menu_add_item(menu, NULL);
    menu_item_init(child, x + i, y, NULL, 0xA0A0FF);
    child->text = malloc(2);
    child->text[0] = '0';
    child->text[1] = 0;
    child->priority = INT_MIN;
    child->data = data;
    child->think_proc = child_think_proc;
    child->navigate_proc = child_navigate_proc;
    child->activate_proc = activate_proc;
    child->move_proc = move_proc;
    child->remove_proc = remove_proc;
  }
  data->selector = data->children[length - 1];
  return parent;
}

uint32_t menu_intinput_get(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->value;
}

void menu_intinput_set(struct menu_item *item, uint32_t value)
{
  struct item_data *data = item->data;
  for (int i = data->length - 1; i >= 0; --i) {
    int c = int_to_char(value % data->base);
    value /= data->base;
    data->children[i]->text[0] = c;
    data->parent->text[i] = c;
  }
  data->value = value;
}
