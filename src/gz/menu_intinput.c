#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "menu.h"

struct item_data
{
  int                     base;
  int                     length;
  menu_generic_callback   callback_proc;
  void                   *callback_data;
  uint32_t                value;
  _Bool                   active;
  struct menu            *imenu;
  struct menu_item       *item;
  struct menu_item      **digits;
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

static int think_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->active) {
    int r = menu_think(data->imenu);
    if (r)
      return r;
  }
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

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  if (data->active) {
    data->imenu->cxoffset = item->x;
    data->imenu->cyoffset = item->y;
    menu_draw(data->imenu);
  }
  return data->active;
}

static int navigate_proc(struct menu_item *item, enum menu_navigation nav)
{
  struct item_data *data = item->data;
  if (data->active)
    menu_navigate(data->imenu, nav);
  return data->active;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->active) {
    if (data->callback_proc && data->callback_proc(item,
                                                   MENU_CALLBACK_DEACTIVATE,
                                                   data->callback_data))
      return 1;
    uint32_t value = 0;
    uint32_t mul = 1;
    for (int i = data->length - 1; i >= 0; --i) {
      int n = data->digits[i]->text[0];
      data->item->text[i] = n;
      value += char_to_int(n) * mul;
      mul *= data->base;
    }
    data->value = value;
    if (data->callback_proc)
      data->callback_proc(item, MENU_CALLBACK_CHANGED, data->callback_data);
  }
  else {
    if (data->callback_proc && data->callback_proc(item,
                                                   MENU_CALLBACK_ACTIVATE,
                                                   data->callback_data))
      return 1;
  }
  data->active = !data->active;
  return 1;
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = 0; i < data->length; ++i)
    data->digits[i]->data = NULL;
  menu_destroy(data->imenu);
  free(data->digits);
  return 0;
}

static int digit_navigate_proc(struct menu_item *item,
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
}

struct menu_item *menu_add_intinput(struct menu *menu, int x, int y,
                                    int base, int length,
                                    menu_generic_callback callback_proc,
                                    void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->base = base;
  data->length = length;
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->value = 0;
  data->active = 0;
  data->digits = malloc(sizeof(*data->digits) * length);
  data->imenu = malloc(sizeof(*data->imenu));
  menu_init(data->imenu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  data->imenu->parent = menu;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0xFFFFFF);
  data->item = item;
  item->text = malloc(length + 1);
  item->text[length] = 0;
  item->data = data;
  item->think_proc = think_proc;
  item->draw_proc = draw_proc;
  item->navigate_proc = navigate_proc;
  item->activate_proc = activate_proc;
  item->destroy_proc = destroy_proc;
  for (int i = 0; i < length; ++i) {
    item->text[i] = '0';
    struct menu_item *digit = menu_item_add(data->imenu, i, 0, NULL,
                                            data->imenu->
                                            highlight_color_static);
    data->digits[i] = digit;
    digit->text = malloc(2);
    digit->text[0] = '0';
    digit->text[1] = 0;
    digit->animate_highlight = 1;
    digit->data = data;
    digit->navigate_proc = digit_navigate_proc;
  }
  data->imenu->selector = data->digits[length - 1];
  return item;
}

uint32_t menu_intinput_get(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->value;
}

void menu_intinput_set(struct menu_item *item, uint32_t value)
{
  struct item_data *data = item->data;
  data->value = value;
  for (int i = data->length - 1; i >= 0; --i) {
    int c = int_to_char(value % data->base);
    value /= data->base;
    data->digits[i]->text[0] = c;
    data->item->text[i] = c;
  }
}
