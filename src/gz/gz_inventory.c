#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "gfx.h"
#include "gz.h"
#include "item_option.h"
#include "menu.h"
#include "resource.h"
#include "z64.h"
#include "zu.h"

struct equipment_item_option
{
  int                   shift;
  uint32_t              mask;
  int                   x;
  int                   y;
  int                   start;
  int                   length;
  const char           *tooltip;
};

struct equipment_item_data
{
  int                   shift;
  uint32_t              mask;
  int                   start;
  int8_t                value;
};

struct capacity_item_option
{
  int                   shift;
  int                   x;
  int                   y;
  int                   item_tile;
  _Bool                 multi_tile;
  int                   amount_tiles[8];
  const char           *tooltip;
};

struct capacity_item_data
{
  int                   shift;
  _Bool                 multi_tile;
  int8_t                value;
};

struct equipment_switch
{
  uint16_t              mask;
  int8_t                item_id;
};

struct item_switch
{
  int8_t               *data;
  int8_t                item_id;
};

struct quest_item_switch
{
  uint32_t              mask;
  int                   tile;
};

struct switch_info
{
  void                 *data;
  uint32_t              mask;
};

struct dungeon_items
{
  struct switch_info    boss_key;
  struct switch_info    compass;
  struct switch_info    map;
  int8_t               *small_keys;
};

struct song_switch
{
  uint32_t              mask;
  uint32_t              color;
  const char           *tooltip;
};

static struct equipment_item_option equipment_item_list[] =
{
  {6,  0b111, 0, 3, Z64_ITEM_GORONS_BRACELET, 6, "strength upgrade"},
  {9,  0b111, 0, 4, Z64_ITEM_SILVER_SCALE,    6, "diving upgrade"},
  {12, 0b11,  0, 5, Z64_ITEM_ADULTS_WALLET,   3, "wallet"},
};

static struct capacity_item_option capacity_item_list[] =
{
  {
    14, 0, 0, Z64_ITEM_BULLET_BAG_30, 1,
    {0, 3, 4, 5, 7, 8, 9, 10}, "bullet bag",
  },
  {
    0,  0, 1, Z64_ITEM_QUIVER_30, 1,
    {0, 3, 4, 5, 7, 9, 10, 11}, "quiver",
  },
  {
    3,  0, 2, Z64_ITEM_BOMB_BAG_20, 1,
    {0, 2, 3, 4, 7, 7, 7, 7},  "bomb bag",
  },
  {
    17, 1, 5, Z64_ITEM_STICK, 0,
    {0, 1, 2, 3, 7, 9, 10, 11}, "stick capacity",
  },
  {
    20, 2, 5, Z64_ITEM_NUT, 0,
    {0, 2, 3, 4,  7, 13, 7, 13}, "nut capacity",
  },
};

static struct equipment_switch equipment_list[] =
{
  {0x0001, Z64_ITEM_KOKIRI_SWORD},
  {0x0002, Z64_ITEM_MASTER_SWORD},
  {0x0004, Z64_ITEM_BIGGORON_SWORD},
  {0x0010, Z64_ITEM_DEKU_SHIELD},
  {0x0020, Z64_ITEM_HYLIAN_SHIELD},
  {0x0040, Z64_ITEM_MIRROR_SHIELD},
  {0x0100, Z64_ITEM_KOKIRI_TUNIC},
  {0x0200, Z64_ITEM_GORON_TUNIC},
  {0x0400, Z64_ITEM_ZORA_TUNIC},
  {0x1000, Z64_ITEM_KOKIRI_BOOTS},
  {0x2000, Z64_ITEM_IRON_BOOTS},
  {0x4000, Z64_ITEM_HOVER_BOOTS},
  {0x0008, Z64_ITEM_BROKEN_GIANTS_KNIFE},
};

static struct item_switch item_list[] =
{
  {&z64_file.items[Z64_SLOT_STICK],         Z64_ITEM_STICK},
  {&z64_file.items[Z64_SLOT_NUT],           Z64_ITEM_NUT},
  {&z64_file.items[Z64_SLOT_BOMB],          Z64_ITEM_BOMB},
  {&z64_file.items[Z64_SLOT_BOW],           Z64_ITEM_BOW},
  {&z64_file.items[Z64_SLOT_FIRE_ARROW],    Z64_ITEM_FIRE_ARROW},
  {&z64_file.items[Z64_SLOT_ICE_ARROW],     Z64_ITEM_ICE_ARROW},
  {&z64_file.items[Z64_SLOT_LIGHT_ARROW],   Z64_ITEM_LIGHT_ARROW},
  {&z64_file.items[Z64_SLOT_DINS_FIRE],     Z64_ITEM_DINS_FIRE},
  {&z64_file.items[Z64_SLOT_FARORES_WIND],  Z64_ITEM_FARORES_WIND},
  {&z64_file.items[Z64_SLOT_NAYRUS_LOVE],   Z64_ITEM_NAYRUS_LOVE},
  {&z64_file.items[Z64_SLOT_SLINGSHOT],     Z64_ITEM_SLINGSHOT},
  {&z64_file.items[Z64_SLOT_OCARINA],       Z64_ITEM_FAIRY_OCARINA},
  {&z64_file.items[Z64_SLOT_OCARINA],       Z64_ITEM_OCARINA_OF_TIME},
  {&z64_file.items[Z64_SLOT_BOMBCHU],       Z64_ITEM_BOMBCHU},
  {&z64_file.items[Z64_SLOT_HOOKSHOT],      Z64_ITEM_HOOKSHOT},
  {&z64_file.items[Z64_SLOT_HOOKSHOT],      Z64_ITEM_LONGSHOT},
  {&z64_file.items[Z64_SLOT_BOOMERANG],     Z64_ITEM_BOOMERANG},
  {&z64_file.items[Z64_SLOT_LENS],          Z64_ITEM_LENS},
  {&z64_file.items[Z64_SLOT_BEANS],         Z64_ITEM_BEANS},
  {&z64_file.items[Z64_SLOT_HAMMER],        Z64_ITEM_HAMMER},
};

static int8_t bottle_options[] =
{
  Z64_ITEM_NULL,          Z64_ITEM_BOTTLE,      Z64_ITEM_RED_POTION,
  Z64_ITEM_GREEN_POTION,  Z64_ITEM_BLUE_POTION, Z64_ITEM_FAIRY,
  Z64_ITEM_FISH,          Z64_ITEM_MILK,        Z64_ITEM_LETTER,
  Z64_ITEM_BLUE_FIRE,     Z64_ITEM_BUG,         Z64_ITEM_BIG_POE,
  Z64_ITEM_HALF_MILK,     Z64_ITEM_POE,
};

static int8_t adult_trade_options[] =
{
  Z64_ITEM_NULL,          Z64_ITEM_POCKET_EGG,
  Z64_ITEM_POCKET_CUCCO,  Z64_ITEM_COJIRO,
  Z64_ITEM_ODD_MUSHROOM,  Z64_ITEM_ODD_POTION,
  Z64_ITEM_POACHERS_SAW,  Z64_ITEM_BROKEN_GORONS_SWORD,
  Z64_ITEM_PRESCRIPTION,  Z64_ITEM_EYEBALL_FROG,
  Z64_ITEM_EYE_DROPS,     Z64_ITEM_CLAIM_CHECK,
};

static int8_t child_trade_options[] =
{
  Z64_ITEM_NULL,          Z64_ITEM_WEIRD_EGG,   Z64_ITEM_CHICKEN,
  Z64_ITEM_ZELDAS_LETTER, Z64_ITEM_KEATON_MASK, Z64_ITEM_SKULL_MASK,
  Z64_ITEM_SPOOKY_MASK,   Z64_ITEM_BUNNY_HOOD,  Z64_ITEM_GORON_MASK,
  Z64_ITEM_ZORA_MASK,     Z64_ITEM_GERUDO_MASK, Z64_ITEM_MASK_OF_TRUTH,
  Z64_ITEM_SOLD_OUT,
};

static struct quest_item_switch quest_item_list[] =
{
  {0x00000020, 5},
  {0x00000001, 0},
  {0x00000002, 1},
  {0x00000004, 2},
  {0x00000008, 3},
  {0x00000010, 4},
  {0x00040000, 6},
  {0x00080000, 7},
  {0x00100000, 8},
  {0x00200000, 9},
  {0x00400000, 10},
  {0x00800000, 11},
};

static struct song_switch song_list[] =
{
  {0x00001000, 0xFFFFFF, "zelda's lullaby"},
  {0x00002000, 0xFFFFFF, "epona's song"},
  {0x00004000, 0xFFFFFF, "saria's song"},
  {0x00008000, 0xFFFFFF, "sun's song"},
  {0x00010000, 0xFFFFFF, "song of time"},
  {0x00020000, 0xFFFFFF, "song of storms"},
  {0x00000040, 0x96FF64, "minuet of forest"},
  {0x00000080, 0xFF5028, "bolero of fire"},
  {0x00000100, 0x6496FF, "serenade of water"},
  {0x00000200, 0xFFA000, "requiem of spirit"},
  {0x00000400, 0xFF64FF, "nocturne of shadow"},
  {0x00000800, 0xFFF064, "prelude of light"},
};

static int byte_mod_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  uint8_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static int byte_mod_indirect_proc(struct menu_item *item,
                                  enum menu_callback_reason reason,
                                  void *data)
{
  uint8_t **pp = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != **pp)
      menu_intinput_set(item, **pp);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    **pp = menu_intinput_get(item);
  return 0;
}

static int byte_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  struct switch_info *d = data;
  uint8_t *p = d->data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *p |= d->mask;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *p &= ~d->mask;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, (*p & d->mask) == d->mask);
  return 0;
}

static int halfword_mod_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  uint16_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static int equipment_item_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  struct equipment_item_data *d = data;
  if (reason == MENU_CALLBACK_CHANGED) {
    if (d->value == Z64_ITEM_NULL)
      z64_file.equipment_items &= ~(d->mask << d->shift);
    else
      z64_file.equipment_items = (z64_file.equipment_items &
                                  ~(d->mask << d->shift)) |
                                 ((d->value - d->start + 1) << d->shift);
  }
  else if (reason == MENU_CALLBACK_THINK) {
    int8_t v = (z64_file.equipment_items >> d->shift) & d->mask;
    if (v == 0)
      d->value = Z64_ITEM_NULL;
    else
      d->value = d->start + v - 1;
  }
  return 0;
}

static int capacity_item_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  struct capacity_item_data *d = data;
  if (reason == MENU_CALLBACK_CHANGED) {
    int v = d->value;
    if (d->multi_tile)
      ++v;
    z64_file.equipment_items = (z64_file.equipment_items &
                                ~(0b111 << d->shift)) | (v << d->shift);
  }
  else if (reason == MENU_CALLBACK_THINK) {
    int v = (z64_file.equipment_items >> d->shift) & 0b111;
    if (d->multi_tile)
      --v;
    d->value = v;
  }
  return 0;
}

static void update_bgs(void)
{
  if (z64_file.bgs_flag || !z64_file.broken_giants_knife) {
    if (z64_file.button_items[Z64_ITEMBTN_B] == Z64_ITEM_BROKEN_GIANTS_KNIFE) {
      z64_file.button_items[Z64_ITEMBTN_B] = Z64_ITEM_BIGGORON_SWORD;
      z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_B);
    }
  }
  else {
    if (z64_file.button_items[Z64_ITEMBTN_B] == Z64_ITEM_BIGGORON_SWORD) {
      z64_file.button_items[Z64_ITEMBTN_B] = Z64_ITEM_BROKEN_GIANTS_KNIFE;
      z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_B);
    }
  }
}

static int equipment_switch_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  struct equipment_switch *d = data;
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    z64_file.equipment |= d->mask;
    if (d->mask == 0x0008)
      update_bgs();
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    z64_file.equipment &= ~d->mask;
    if (d->mask == 0x0008)
      update_bgs();
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, z64_file.equipment & d->mask);
  return 0;
}

static int bgs_switch_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    z64_file.bgs_flag = 1;
    z64_file.bgs_hits_left = 8;
    update_bgs();
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    z64_file.bgs_flag = 0;
    update_bgs();
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, z64_file.bgs_flag);
  return 0;
}

static int item_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  struct item_switch *d = data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *d->data = d->item_id;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *d->data = Z64_ITEM_NULL;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, *d->data == d->item_id);
  return 0;
}

static int double_defense_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    z64_file.double_defense = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    z64_file.double_defense = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, z64_file.double_defense);
  return 0;
}

static int magic_acquired_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    z64_file.magic_acquired = 1;
    z64_file.magic_capacity_set = 0;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    z64_file.magic_acquired = 0;
    z64_file.magic_capacity_set = 0;
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, z64_file.magic_acquired);
  return 0;
}

static int magic_capacity_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != z64_file.magic_capacity)
      menu_intinput_set(item, z64_file.magic_capacity);
  }
  else if (reason == MENU_CALLBACK_CHANGED) {
    z64_file.magic_capacity = menu_intinput_get(item);
    z64_file.magic_capacity_set = 0;
  }
  return 0;
}

static int dungeon_option_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  struct dungeon_items *d = data;
  if (reason == MENU_CALLBACK_DEACTIVATE) {
    int dungeon = menu_option_get(item);
    uint8_t *items = &z64_file.dungeon_items[dungeon].items;
    d->boss_key.data = items;
    d->map.data = items;
    d->compass.data = items;
    d->small_keys = &z64_file.dungeon_keys[dungeon];
  }
  return 0;
}

static int quest_item_switch_proc(struct menu_item *item,
                                  enum menu_callback_reason reason,
                                  void *data)
{
  uint32_t mask = (uint32_t)data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    z64_file.quest_items |= mask;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    z64_file.quest_items &= ~mask;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, z64_file.quest_items & mask);
  return 0;
}

struct menu *gz_inventory_menu(void)
{
  static struct menu menu;
  static struct menu equipment;
  static struct menu quest;
  static struct menu amount;
  struct menu_item *item;

  /* load icon textures */
  struct gfx_texture *t_ab = resource_get(RES_ZICON_ACTION_BUTTONS);
  struct gfx_texture *t_am = resource_get(RES_ICON_AMOUNT);
  struct gfx_texture *t_item = resource_get(RES_ZICON_ITEM);
  struct gfx_texture *t_item_gray = resource_get(RES_ZICON_ITEM_GRAY);
  struct gfx_texture *t_item_24 = resource_get(RES_ZICON_ITEM_24);
  struct gfx_texture *t_rupee = resource_get(RES_ZICON_RUPEE);
  struct gfx_texture *t_note = resource_get(RES_ZICON_NOTE);

  /* initialize menus */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&equipment, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&quest, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&amount, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* populate inventory top menu */
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  menu_add_submenu(&menu, 0, 1, &equipment, "equipment and items");
  menu_add_submenu(&menu, 0, 2, &quest, "quest items");
  menu_add_submenu(&menu, 0, 3, &amount, "amounts");

  /* populate equipment menu */
  equipment.selector = menu_add_submenu(&equipment, 0, 0, NULL, "return");
  /* equipment items */
  int n_eq_items = sizeof(equipment_item_list) / sizeof(*equipment_item_list);
  for (int i = 0; i < n_eq_items; ++i) {
    struct equipment_item_option *option = &equipment_item_list[i];
    struct equipment_item_data *data = malloc(sizeof(*data));
    data->shift = option->shift;
    data->mask = option->mask;
    data->start = option->start;
    int8_t *item_list = malloc(option->length + 1);
    item_list[0] = Z64_ITEM_NULL;
    for (int j = 0; j < option->length; ++j)
      item_list[j + 1] = option->start + j;
    item = item_option_create(&equipment, 0, 2,
                              option->length + 1, item_list,
                              &data->value,
                              NULL, t_ab, 1, NULL, 0,
                              0xFFFFFF, .5f, .5625f, 0.f,
                              equipment_item_proc, data);
    item->pxoffset = option->x * 18;
    item->pyoffset = option->y * 18;
    item->tooltip = option->tooltip;
  }
  /* capacity items */
  int n_cap_item = sizeof(capacity_item_list) / sizeof(*capacity_item_list);
  for (int i = 0; i < n_cap_item; ++i) {
    struct gfx_texture *tex;
    struct capacity_item_option *option = &capacity_item_list[i];
    int n_tiles = option->multi_tile ? 7 : 8;
    tex = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 1, n_tiles);
    for (int j = 0; j < n_tiles; ++j) {
      if (option->multi_tile)
        gfx_texture_copy_tile(tex, j, t_item, option->item_tile + j, 0);
      else
        gfx_texture_copy_tile(tex, j, t_item, option->item_tile, 0);
      gfx_texture_copy_tile(tex, j, t_am,
                            option->amount_tiles[8 - n_tiles + j], 1);
    }
    struct capacity_item_data *data = malloc(sizeof(*data));
    data->shift = option->shift;
    data->multi_tile = option->multi_tile;
    static int8_t options[] = {0, 1, 2, 3, 4, 5, 6, 7};
    static int8_t multi_options[] = {Z64_ITEM_NULL, 0, 1, 2, 3, 4, 5, 6};
    item = item_option_create(&equipment, 0, 2, 8,
                              option->multi_tile ? multi_options : options,
                              &data->value, tex, t_ab, 1, NULL, 0,
                              0xFFFFFF, .5f, .5625f, 0.f,
                              capacity_item_proc, data);
    item->pxoffset = option->x * 18;
    item->pyoffset = option->y * 18;
    item->tooltip = option->tooltip;
  }
  /* equipment */
  int n_eq = sizeof(equipment_list) / sizeof(*equipment_list);
  for (int i = 0; i < n_eq; ++i) {
    item = menu_add_switch(&equipment, 0, 2,
                           t_item, equipment_list[i].item_id, 0xFFFFFF,
                           t_item_gray, equipment_list[i].item_id, 0xFFFFFF,
                           .5f, 1,
                           equipment_switch_proc, &equipment_list[i]);
    item->pxoffset = 20 + i % 3 * 18;
    item->pyoffset = i / 3 * 18;
  }
  /* giant's knife / biggoron's sword switch */
  item = menu_add_switch(&equipment, 0, 2,
                         t_item, Z64_ITEM_BIGGORON_SWORD, 0xFFFFFF,
                         t_item_gray, Z64_ITEM_BIGGORON_SWORD, 0xFFFFFF,
                         .5f, 1,
                         bgs_switch_proc, NULL);
  item->pxoffset = 20 + 1 * 18;
  item->pyoffset = 4 * 18;
  /* normal items */
  int n_item = sizeof(item_list) / sizeof(*item_list);
  for (int i = 0; i < n_item; ++i) {
    item = menu_add_switch(&equipment, 0, 2,
                           t_item, item_list[i].item_id, 0xFFFFFF,
                           t_item_gray, item_list[i].item_id, 0xFFFFFF,
                           .5f, 1,
                           item_switch_proc, &item_list[i]);
    item->pxoffset = 76 + i % 6 * 18;
    item->pyoffset = i / 6 * 18;
  }
  /* multi-option items */
  /* bottles */
  int n_bottle_opt = sizeof(bottle_options) / sizeof(*bottle_options);
  for (int i = 0; i < 4; ++i) {
    item = item_option_create(&equipment, 0, 2,
                              n_bottle_opt, bottle_options,
                              &z64_file.items[Z64_SLOT_BOTTLE_1 + i],
                              NULL, t_ab, 1, NULL, 0,
                              0xFFFFFF, .5f, .5625f, 0.f,
                              NULL, NULL);
    item->pxoffset = 76 + i * 18;
    item->pyoffset = 72;
    char *tooltip = malloc(19);
    sprintf(tooltip, "bottle %d", i + 1);
    item->tooltip = tooltip;
  }
  /* adult trade item */
  int n_adult_opt = sizeof(adult_trade_options) / sizeof(*adult_trade_options);
  item = item_option_create(&equipment, 0, 2,
                            n_adult_opt, adult_trade_options,
                            &z64_file.items[Z64_SLOT_ADULT_TRADE],
                            NULL, t_ab, 1, NULL, 0,
                            0xFFFFFF, .5f, .5625f, 0.f,
                            NULL, NULL);
  item->pxoffset = 76 + 4 * 18;
  item->pyoffset = 72;
  item->tooltip = "adult trade item";
  /* child trade item */
  int n_child_opt = sizeof(child_trade_options) / sizeof(*child_trade_options);
  item = item_option_create(&equipment, 0, 2,
                            n_child_opt, child_trade_options,
                            &z64_file.items[Z64_SLOT_CHILD_TRADE],
                            NULL, t_ab, 1, NULL, 0,
                            0xFFFFFF, .5f, .5625f, 0.f,
                            NULL, NULL);
  item->pxoffset = 76 + 5 * 18;
  item->pyoffset = 72;
  item->tooltip = "child trade item";
  /* equipment tooltip */
  item = menu_add_tooltip(&equipment, 0, 2, gz.menu_main, 0xC0C0C0);
  item->pxoffset = 76;
  item->pyoffset = 90;

  /* populate quest items menu */
  quest.selector = menu_add_submenu(&quest, 0, 0, NULL, "return");
  /* energy options */
  menu_add_static(&quest, 0, 1, "energy cap.", 0xC0C0C0);
  menu_add_intinput(&quest, 14, 1, 16, 4,
                    halfword_mod_proc, &z64_file.energy_capacity);
  menu_add_static(&quest, 0, 2, "defense", 0xC0C0C0);
  menu_add_checkbox(&quest, 14, 2, double_defense_proc, NULL);
  menu_add_intinput(&quest, 16, 2, 16, 2,
                    byte_mod_proc, &z64_file.defense_hearts);
  /* magic options */
  menu_add_static(&quest, 0, 3, "magic", 0xC0C0C0);
  menu_add_checkbox(&quest, 14, 3, magic_acquired_proc, NULL);
  menu_add_static(&quest, 0, 4, "magic cap.", 0xC0C0C0);
  menu_add_intinput(&quest, 14, 4, 16, 2, magic_capacity_proc, NULL);
  /* quest status amounts */
  menu_add_static(&quest, 0, 5, "gs tokens", 0xC0C0C0);
  menu_add_intinput(&quest, 14, 5, 10, 3,
                    halfword_mod_proc, &z64_file.gs_tokens);
  menu_add_static(&quest, 0, 6, "heart pieces", 0xC0C0C0);
  menu_add_intinput(&quest, 14, 6, 16, 2,
                    byte_mod_proc, &z64_file.quest_items);
  /* dungeon items */
  static struct dungeon_items di =
  {
    {&z64_file.dungeon_items[0].items, 0b001},
    {&z64_file.dungeon_items[0].items, 0b010},
    {&z64_file.dungeon_items[0].items, 0b100},
    &z64_file.dungeon_keys[0],
  };
  menu_add_static(&quest, 0, 7, "dungeon", 0xC0C0C0);
  {
    char options[1024];
    char *p = options;
    for (int i = 0; i < 17; ++i) {
      size_t l = strlen(zu_scene_info[i].scene_name) + 1;
      memcpy(p, zu_scene_info[i].scene_name, l);
      p += l;
    }
    *p = 0;
    menu_add_option(&quest, 14, 7, options, dungeon_option_proc, &di);
  }
  menu_add_static(&quest, 0, 8, "small keys", 0xC0C0C0);
  menu_add_intinput(&quest, 14, 8, 16, 2,
                    byte_mod_indirect_proc, &di.small_keys);
  item = menu_add_switch(&quest, 0, 10,
                         t_item_24, 14, 0xFFFFFF,
                         t_item_24, 14, 0x606060,
                         2.f / 3.f, 1,
                         byte_switch_proc, &di.boss_key);
  item->pxoffset = 5 * 18 + 20;
  item = menu_add_switch(&quest, 0, 10,
                         t_item_24, 15, 0xFFFFFF,
                         t_item_24, 15, 0x606060,
                         2.f / 3.f, 1,
                         byte_switch_proc, &di.compass);
  item->pxoffset = 5 * 18 + 20;
  item->pyoffset = 1 * 18;
  item = menu_add_switch(&quest, 0, 10,
                         t_item_24, 16, 0xFFFFFF,
                         t_item_24, 16, 0x606060,
                         2.f / 3.f, 1,
                         byte_switch_proc, &di.map);
  item->pxoffset = 5 * 18 + 20;
  item->pyoffset = 2 * 18;
  /* quest items */
  int n_quest_item = sizeof(quest_item_list) / sizeof(*quest_item_list);
  for (int i = 0; i < n_quest_item; ++i) {
    struct quest_item_switch *d = &quest_item_list[i];
    item = menu_add_switch(&quest, 0, 10,
                           t_item_24, d->tile, 0xFFFFFF,
                           t_item_24, d->tile, 0x606060,
                           2.f / 3.f, 1,
                           quest_item_switch_proc, (void*)d->mask);
    item->pxoffset = i % 6 * 18;
    item->pyoffset = i / 6 * 18;
  }
  /* songs */
  int n_song = sizeof(song_list) / sizeof(*song_list);
  for (int i = 0; i < n_song; ++i) {
    struct song_switch *d = &song_list[i];
    uint8_t r = (d->color >> 16) & 0xFF;
    uint8_t g = (d->color >> 8)  & 0xFF;
    uint8_t b = (d->color >> 0)  & 0xFF;
    uint32_t color_off = ((r * 0x60 / 0xFF) << 16) |
                         ((g * 0x60 / 0xFF) << 8)  |
                         ((b * 0x60 / 0xFF) << 0);
    item = menu_add_switch(&quest, 0, 10,
                           t_note, 0, d->color,
                           t_note, 0, color_off,
                           2.f / 3.f, 1,
                           quest_item_switch_proc, (void*)d->mask);
    item->pxoffset = i % 6 * 18;
    item->pyoffset = 2 * 18 + i / 6 * 18;
    item->tooltip = d->tooltip;
  }
  /* quest items tooltip */
  item = menu_add_tooltip(&quest, 0, 10, gz.menu_main, 0xC0C0C0);
  item->pyoffset = 4 * 18;

  /* populate amounts menu */
  amount.selector = menu_add_submenu(&amount, 0, 0, NULL, "return");
  menu_add_static_icon(&amount, 0, 2, t_item, Z64_ITEM_STICK, 0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 5, 2, t_item, Z64_ITEM_NUT, 0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 10, 2, t_item, Z64_ITEM_BOMB, 0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 0, 4, t_item, Z64_ITEM_BOW, 0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 5, 4, t_item, Z64_ITEM_SLINGSHOT,
                       0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 10, 4, t_item, Z64_ITEM_BOMBCHU,
                       0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 0, 6, t_item, Z64_ITEM_BEANS, 0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 5, 6, t_item_24, 19, 0xFFFFFF, 2.f / 3.f);
  menu_add_static_icon(&amount, 10, 6, t_item_24, 12, 0xFFFFFF, 2.f / 3.f);
  menu_add_static_icon(&amount, 0, 8, t_item, Z64_ITEM_BIGGORON_SWORD,
                       0xFFFFFF, .5f);
  menu_add_static_icon(&amount, 10, 8, t_rupee, 0, 0xC8FF64, 1.f);
  menu_add_intinput(&amount, 2, 2, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_STICK]);
  menu_add_intinput(&amount, 7, 2, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_NUT]);
  menu_add_intinput(&amount, 12, 2, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_BOMB]);
  menu_add_intinput(&amount, 2, 4, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_BOW]);
  menu_add_intinput(&amount, 7, 4, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_SLINGSHOT]);
  menu_add_intinput(&amount, 12, 4, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_BOMBCHU]);
  menu_add_intinput(&amount, 2, 6, 10, 2,
                    byte_mod_proc, &z64_file.ammo[Z64_SLOT_BEANS]);
  menu_add_intinput(&amount, 7, 6, 16, 2,
                    byte_mod_proc, &z64_file.magic);
  menu_add_intinput(&amount, 12, 6, 16, 4,
                    halfword_mod_proc, &z64_file.energy);
  menu_add_intinput(&amount, 2, 8, 16, 4,
                    halfword_mod_proc, &z64_file.bgs_hits_left);
  menu_add_intinput(&amount, 12, 8, 10, 5,
                    halfword_mod_proc, &z64_file.rupees);

  return &menu;
}
