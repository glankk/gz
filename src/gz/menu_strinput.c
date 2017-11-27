#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "menu.h"

struct item_data
{
  int                     length;
  menu_generic_callback   callback_proc;
  void                   *callback_data;
  _Bool                   active;
  struct menu            *imenu;
  struct menu_item       *item;
  struct menu_item      **chars;
};

static const char         charset[] = "_abcdefghijklmnopqrstuvwxyz"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "0123456789-.";
static int                charset_size = sizeof(charset) - 1;

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
    {
      return 1;
    }
    int max = 0;
    for (int i = 0; i < data->length; ++i) {
      char c = data->chars[i]->text[0];
      if (c == '_')
        c = ' ';
      else
        max = i + 1;
      data->item->text[i] = c;
    }
    data->item->text[max] = 0;
    if (data->callback_proc)
      data->callback_proc(item, MENU_CALLBACK_CHANGED, data->callback_data);
  }
  else {
    if (data->callback_proc && data->callback_proc(item,
                                                   MENU_CALLBACK_ACTIVATE,
                                                   data->callback_data))
    {
      return 1;
    }
  }
  data->active = !data->active;
  return 1;
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = 0; i < data->length; ++i)
    data->chars[i]->data = NULL;
  menu_destroy(data->imenu);
  free(data->chars);
  return 0;
}

static int char_navigate_proc(struct menu_item *item,
                              enum menu_navigation nav)
{
  int n = strchr(charset, item->text[0]) - charset;
  int d = (input_pad() & BUTTON_Z) ? 3 : 1;
  if (nav == MENU_NAVIGATE_UP)
    n += d;
  else if (nav == MENU_NAVIGATE_DOWN)
    n -= d;
  else
    return 0;
  n = (n + charset_size) % charset_size;
  item->text[0] = charset[n];
  return 1;
}

struct menu_item *menu_add_strinput(struct menu *menu, int x, int y,
                                    int length,
                                    menu_generic_callback callback_proc,
                                    void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->length = length;
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->active = 0;
  data->chars = malloc(sizeof(*data->chars) * length);
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
    struct menu_item *c = menu_item_add(data->imenu, i, 0, NULL,
                                        data->imenu->highlight_color_static);
    data->chars[i] = c;
    c->text = malloc(2);
    c->text[0] = '_';
    c->text[1] = 0;
    item->text[i] = ' ';
    c->navigate_proc = char_navigate_proc;
    c->animate_highlight = 1;
    c->data = data;
  }
  data->imenu->selector = data->chars[0];
  return item;
}

void menu_strinput_get(struct menu_item *item, char *buf)
{
  struct item_data *data = item->data;
  strcpy(buf, data->item->text);
}

void menu_strinput_set(struct menu_item *item, const char *str)
{
  struct item_data *data = item->data;
  int max = 0;
  _Bool end = 0;
  for (int i = 0; i < data->length; ++i) {
    char c;
    if (end)
      c = ' ';
    else {
      c = str[i];
      if (c == 0) {
        end = 1;
        c = ' ';
      }
      else if (!strchr(charset, c) || c == '_')
        c = ' ';
    }
    if (c == ' ')
      data->chars[i]->text[0] = '_';
    else {
      data->chars[i]->text[0] = c;
      max = i + 1;
    }
    data->item->text[i] = c;
  }
  data->item->text[max] = 0;
}
