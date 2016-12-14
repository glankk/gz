#include <stdlib.h>
#include <string.h>
#include "binder.h"
#include "menu.h"
#include "settings.h"
#include "z64.h"

struct item_data
{
  int       command_index;
  _Bool    *override_input;
  uint16_t  pad;
  int       state;
};

static void update(struct menu_item *item)
{
  static const struct
  {
    uint16_t    mask;
    const char *name;
  }
  buttons[] =
  {
    {BUTTON_A,        "a"},
    {BUTTON_B,        "b"},
    {BUTTON_Z,        "z"},
    {BUTTON_START,    "s"},
    {BUTTON_C_UP,     "cu"},
    {BUTTON_C_DOWN,   "cd"},
    {BUTTON_C_LEFT,   "cl"},
    {BUTTON_C_RIGHT,  "cr"},
    {BUTTON_L,        "l"},
    {BUTTON_R,        "r"},
    {BUTTON_D_UP,     "du"},
    {BUTTON_D_DOWN,   "dd"},
    {BUTTON_D_LEFT,   "dl"},
    {BUTTON_D_RIGHT,  "dr"},
  };
  size_t no_buttons = sizeof(buttons) / sizeof(*buttons);
  struct item_data *data = item->data;
  char *p = item->text;
  for (int i = 0; i < no_buttons; ++i)
    if (data->pad & buttons[i].mask) {
      if (p > item->text)
        *p++ = '+';
      size_t l = strlen(buttons[i].name);
      memcpy(p, buttons[i].name, l);
      p += l;
    }
  if (p == item->text)
    strcpy(p, "<unbound>");
  else
    *p = 0;
}

static int think_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->state == 1) {
    if (z64_input_direct.pad == 0)
      data->state = 2;
  }
  if (data->state == 2) {
    if (z64_input_direct.pad != 0)
      data->state = 3;
  }
  if (data->state == 3) {
    data->pad |= z64_input_direct.pad;
    if (z64_input_direct.pad != data->pad) {
      item->animate_highlight = 0;
      *data->override_input = 0;
      if (data->pad == BUTTON_START)
        data->pad = 0;
      settings->command_binds[data->command_index] = data->pad;
      data->state = 0;
      update(item);
    }
  }
  return 0;
}

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  if (data->state == 0 &&
      data->pad != settings->command_binds[data->command_index])
  {
    data->pad = settings->command_binds[data->command_index];
    update(item);
  }
  return 0;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->state == 0) {
    item->animate_highlight = 1;
    *data->override_input = 1;
    data->pad = 0;
    data->state = 1;
  }
  return 1;
}

struct menu_item *binder_create(struct menu *menu, int x, int y,
                                int command_index, _Bool *override_input)
{
  struct item_data *data = malloc(sizeof(*data));
  data->command_index = command_index;
  data->override_input = override_input;
  data->pad = settings->command_binds[data->command_index];
  data->state = 0;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0xFFFFFF);
  item->data = data;
  item->text = malloc(49);
  item->think_proc = think_proc;
  item->draw_proc = draw_proc;
  item->activate_proc = activate_proc;
  update(item);
  return item;
}
