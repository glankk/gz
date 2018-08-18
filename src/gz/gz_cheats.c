#include "menu.h"
#include "settings.h"

static const char *cheat_names[] =
{
  "energy",
  "magic",
  "deku sticks",
  "deku nuts",
  "bombs",
  "arrows",
  "deku seeds",
  "bombchus",
  "magic beans",
  "small keys",
  "rupees",
  "nayru's love",
  "freeze time",
  "no music",
  "items usable",
  "no minimap",
  "isg",
};

static int cheat_proc(struct menu_item *item,
                      enum menu_callback_reason reason,
                      void *data)
{
  int cheat_index = (int)data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->cheats |= (1 << cheat_index);
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->cheats &= ~(1 << cheat_index);
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->cheats & (1 << cheat_index));
  return 0;
}

struct menu *gz_cheats_menu(void)
{
  static struct menu menu;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /* populate cheat list */
  for (int i = 0; i < CHEAT_MAX; ++i) {
    menu_add_static(&menu, 0, 1 + i, cheat_names[i], 0xC0C0C0);
    menu_add_checkbox(&menu, 14, 1 + i, cheat_proc, (void*)i);
  }

  return &menu;
}
