#include <stdint.h>
#include <string.h>
#include "gfx.h"
#include "gz.h"
#include "input.h"
#include "item_option.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
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

static inline uint8_t adjust_item(int item) {
  // don't separate ocarinas and hookshots
  if (item == Z64_ITEM_FAIRY_OCARINA && z64_file.items[Z64_SLOT_OCARINA] == Z64_ITEM_OCARINA_OF_TIME) {
    return Z64_ITEM_OCARINA_OF_TIME;
  } else if (item == Z64_ITEM_HOOKSHOT && z64_file.items[Z64_SLOT_HOOKSHOT] == Z64_ITEM_LONGSHOT) {
    return Z64_ITEM_LONGSHOT;
  }

  return item;
}

static inline uint8_t get_pref(uint8_t item_id) {
  int index = item_id / 2;

  uint8_t value;

  // when uneven we want the upper bytes
  if (item_id % 2) {
    value = settings->equip_prefs[index] >> 4;
  } else {
    value = settings->equip_prefs[index];
  }

  value &= 0x0F; // unnecessary but better make sure
  return value;
}

static int byte_switch_proc(struct menu_item *item, enum menu_callback_reason reason, void *data) {
  uint8_t *p = data;
  uint8_t item_id = adjust_item(*p);
  int index = item_id / 2;
  
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    uint8_t value = get_pref(item_id);
    
    if (menu_option_get(item) != value) {
      menu_option_set(item, value);
    }
  
  } else if (reason == MENU_CALLBACK_DEACTIVATE) {
    uint8_t value = menu_option_get(item) & 0x0F;
    
    if (item_id % 2) {
      settings->equip_prefs[index] &= 0x0F;
      settings->equip_prefs[index] |= (value << 4);
    } else {
      settings->equip_prefs[index] &= 0xF0;
      settings->equip_prefs[index] |= value;
    }
  }
  
  return 0;
}

static struct menu* gz_equips_prefs_menu(void) {
  static struct menu menu;

  static uint8_t item_list[] = {
    Z64_ITEM_STICK,
    Z64_ITEM_NUT,
    Z64_ITEM_BOW,
    Z64_ITEM_FIRE_ARROW,
    Z64_ITEM_ICE_ARROW,
    Z64_ITEM_LIGHT_ARROW,

    Z64_ITEM_BOMB,
    Z64_ITEM_DINS_FIRE,
    Z64_ITEM_FARORES_WIND,
    Z64_ITEM_NAYRUS_LOVE,
    Z64_ITEM_SLINGSHOT,
    Z64_ITEM_FAIRY_OCARINA,

    Z64_ITEM_BOMBCHU,
    Z64_ITEM_HOOKSHOT,
    Z64_ITEM_BOOMERANG,
    Z64_ITEM_LENS,
    Z64_ITEM_BEANS,
    Z64_ITEM_HAMMER,
  };

  #define E_NONE "none\0"
  #define E_CLEFT "c-left\0"
  #define E_CDOWN "c-down\0"
  #define E_CRIGHT "c-right\0"
  #define EQUIP_OPTIONS E_NONE E_CLEFT E_CDOWN E_CRIGHT

  struct gfx_texture *t_icon = resource_get(RES_ZICON_ITEM);
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  int pos_x = 0, pos_y = 2, n_row = 0;
  for (int i = 0; i < (sizeof(item_list) / sizeof(*item_list)); i++) {
    uint8_t item = adjust_item(item_list[i]);
    menu_add_static_icon(&menu, pos_x, pos_y, t_icon, item, 0xFFFFFF, 0.5f);
    menu_add_option(&menu, pos_x + 2, pos_y, EQUIP_OPTIONS, byte_switch_proc, &item_list[i]);

    // handle row/columns display
    pos_y += 2;
    n_row++;
    if (n_row > 0 && (n_row % 6) == 0) {
      pos_x += 10;
      pos_y = 2;
      n_row = 0;
    }
  }

  return &menu;
}

void apply_equips_prefs(void) {
  for (int i = 1; i < (sizeof(z64_file.button_items) / sizeof(*z64_file.button_items)); i++) {
    uint8_t cur_item = z64_file.button_items[i];
    uint8_t value = get_pref(cur_item);

    if (value > 0 && value != i) {
      z64_file.button_items[i] = z64_file.button_items[value];
      z64_UpdateItemButton(&z64_game, i);

      z64_file.button_items[value] = cur_item;
      z64_UpdateItemButton(&z64_game, value);
    }
  }
}

struct menu *gz_equips_menu(void)
{
  static struct menu menu;
  struct menu_item *item;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  menu_add_submenu(&menu, 0, 1, gz_equips_prefs_menu(), "settings");

  /* create equipment switches */
  struct gfx_texture *t_icon = resource_get(RES_ZICON_ITEM);
  struct gfx_texture *t_icon_gray = resource_get(RES_ZICON_ITEM_GRAY);
  for (int i = 0; i < 3 * 4; ++i) {
    struct menu_item *item;
    item = menu_add_switch(&menu, 0, 3,
                           t_icon, Z64_ITEM_KOKIRI_SWORD + i, 0xFFFFFF,
                           t_icon_gray, Z64_ITEM_KOKIRI_SWORD + i, 0xFFFFFF,
                           .5f, 1,
                           equip_switch_proc,
                           (void *)((i / 3) | ((1 + i % 3) << 4)));
    item->pxoffset = i % 3 * 18;
    item->pyoffset = i / 3 * 18;
  }

  /* create item buttons controls */
  struct gfx_texture *t_ab = resource_get(RES_ZICON_ACTION_BUTTONS);
  item = item_option_create(&menu, 0, 3, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_B],
                            NULL, t_ab, 0, NULL, 0,
                            input_button_color[14], .5625f, 0.f, 0.f,
                            button_item_proc, (void *)Z64_ITEMBTN_B);
  item->pxoffset = 0;
  item->pyoffset = 75;
  item = item_option_create(&menu, 0, 3, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_CL],
                            NULL, t_ab, 0, t_ab, 2,
                            input_button_color[1], .5f, 0.f, 0.f,
                            button_item_proc, (void *)Z64_ITEMBTN_CL);
  item->pxoffset = 20;
  item->pyoffset = 75;
  item = item_option_create(&menu, 0, 3, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_CD],
                            NULL, t_ab, 0, t_ab, 3,
                            input_button_color[2], .5f, 0.f, 0.f,
                            button_item_proc, (void *)Z64_ITEMBTN_CD);
  item->pxoffset = 34;
  item->pyoffset = 86;
  item = item_option_create(&menu, 0, 3, 91, NULL,
                            &z64_file.button_items[Z64_ITEMBTN_CR],
                            NULL, t_ab, 0, t_ab, 4,
                            input_button_color[0], .5, 0.f, 0.f,
                            button_item_proc, (void *)Z64_ITEMBTN_CR);
  item->pxoffset = 48;
  item->pyoffset = 75;

  return &menu;
}
