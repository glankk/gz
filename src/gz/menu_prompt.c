#include <stdlib.h>
#include <string.h>
#include "menu.h"

static struct menu            prompt_menu;
static menu_prompt_callback   prompt_callback_proc;
static void                  *prompt_callback_data;

static int do_callback(int index)
{
  if (prompt_callback_proc) {
    menu_prompt_callback proc = prompt_callback_proc;
    prompt_callback_proc = NULL;
    return proc(index, prompt_callback_data);
  }
  else
    return 0;
}

static int leave_proc(struct menu_item *item, enum menu_switch_reason reason)
{
  if (reason == MENU_SWITCH_RETURN)
    do_callback(-1);
  return 0;
}

static int activate_proc(struct menu_item *item)
{
  int index = (int)item->data;
  if (!do_callback(index))
    menu_return(&prompt_menu);
  return 0;
}

static int destroy_proc(struct menu_item *item)
{
  item->data = NULL;
  return 0;
}

void menu_prompt(struct menu *menu, const char *prompt,
                 const char *options, int default_option,
                 menu_prompt_callback callback_proc, void *callback_data)
{
  static _Bool ready = 0;
  if (ready) {
    if (menu == &prompt_menu) {
      menu = prompt_menu.parent;
      menu_return(&prompt_menu);
    }
    menu_destroy(&prompt_menu);
  }
  else
    ready = 1;
  menu_init(&prompt_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  struct menu_item *item = menu_add_static(&prompt_menu, 0, 0,
                                           prompt, 0xC0C0C0);
  item->leave_proc = leave_proc;
  const char *option = options;
  for (int i = 0; *option; ++i) {
    item = menu_item_add(&prompt_menu, option - options, 1, NULL, 0xFFFFFF);
    item->data = (void *)i;
    size_t option_length = strlen(option);
    char *new_option = malloc(option_length + 1);
    strcpy(new_option, option);
    item->text = new_option;
    item->activate_proc = activate_proc;
    item->destroy_proc = destroy_proc;
    option += option_length + 1;
    if (i == default_option)
      prompt_menu.selector = item;
  }
  prompt_callback_proc = callback_proc;
  prompt_callback_data = callback_data;
  menu_enter(menu, &prompt_menu);
}
