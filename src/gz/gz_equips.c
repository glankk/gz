#include <stdint.h>
#include "gfx.h"
#include "gz.h"
#include "input.h"
#include "item_option.h"
#include "menu.h"
#include "resource.h"
#include "z64.h"

static int equip_switch_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  int equip_row = ((int)data >> 0) & 0xF;
  int equip_value = ((int)data >> 4) & 0xF;
  if (reason == MENU_CALLBACK_THINK) {
    int value = (z64_file.equips >> equip_row * 4) & 0x000F;
    menu_switch_set(item, equip_value == value);
  }
  else if (reason == MENU_CALLBACK_SWITCH_ON) {
    z64_file.equips = (z64_file.equips & ~(0x000F << equip_row * 4)) |
                      (equip_value << equip_row * 4);
    if (equip_row == 0) {
      int v = Z64_ITEM_KOKIRI_SWORD + equip_value - 1;
      if (v == Z64_ITEM_BIGGORON_SWORD &&
          !z64_file.bgs_flag && z64_file.broken_giants_knife)
      {
        v = Z64_ITEM_BROKEN_GIANTS_KNIFE;
      }
      z64_file.button_items[Z64_ITEMBTN_B] = v;
      z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_B);
    }
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    z64_file.equips &= ~(0x000F << equip_row * 4);
    if (equip_row == 0) {
      z64_file.inf_table[29] |= 0x0001;
      z64_file.button_items[Z64_ITEMBTN_B] = Z64_ITEM_NULL;
    }
  }
  else if (reason == MENU_CALLBACK_CHANGED) {
    if (equip_row != 0)
      z64_UpdateEquipment(&z64_game, &z64_link);
  }
  return 0;
}

static int button_item_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_CHANGED) {
    int button_index = (int)data;
    int8_t *item_id = &z64_file.button_items[button_index];
    if (*item_id != Z64_ITEM_NULL)
      z64_UpdateItemButton(&z64_game, button_index);
    else if (button_index == Z64_ITEMBTN_B)
      z64_file.inf_table[29] |= 0x0001;
  }
  return 0;
}

struct menu *gz_equips_menu(void)
{
  static struct menu menu;
  struct menu_item *item;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /* create equipment switches */
  struct gfx_texture *t_icon = resource_get(RES_ZICON_ITEM);
  struct gfx_texture *t_icon_gray = resource_get(RES_ZICON_ITEM_GRAY);
  for (int i = 0; i < 3 * 4; ++i) {
    struct menu_item *item;
    item = menu_add_switch(&menu, 0, 2,
                           t_icon, Z64_ITEM_KOKIRI_SWORD + i, 0xFFFFFF,
                           t_icon_gray, Z64_ITEM_KOKIRI_SWORD + i, 0xFFFFFF,
                           .5f, 1,
                           equip_switch_proc,
                           (void*)((i / 3) | ((1 + i % 3) << 4)));
    item->pxoffset = i % 3 * 18;
    item->pyoffset = i / 3 * 18;
  }

  /* create item buttons controls */
  struct gfx_texture *t_ab = resource_get(RES_ZICON_ACTION_BUTTONS);
  item = item_option_create(&menu, 0, 2, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_B],
                            NULL, t_ab, 0, NULL, 0,
                            input_button_color[14], .5625f, 0.f, 0.f,
                            button_item_proc, (void*)Z64_ITEMBTN_B);
  item->pxoffset = 0;
  item->pyoffset = 74;
  item = item_option_create(&menu, 0, 2, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_CL],
                            NULL, t_ab, 0, t_ab, 2,
                            input_button_color[1], .5f, 0.f, 0.f,
                            button_item_proc, (void*)Z64_ITEMBTN_CL);
  item->pxoffset = 20;
  item->pyoffset = 74;
  item = item_option_create(&menu, 0, 2, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_CD],
                            NULL, t_ab, 0, t_ab, 3,
                            input_button_color[2], .5f, 0.f, 0.f,
                            button_item_proc, (void*)Z64_ITEMBTN_CD);
  item->pxoffset = 34;
  item->pyoffset = 86;
  item = item_option_create(&menu, 0, 2, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_CR],
                            NULL, t_ab, 0, t_ab, 4,
                            input_button_color[0], .5, 0.f, 0.f,
                            button_item_proc, (void*)Z64_ITEMBTN_CR);
  item->pxoffset = 48;
  item->pyoffset = 74;

  return &menu;
}
