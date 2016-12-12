#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <startup.h>
#include <mips.h>
#include <n64.h>
#include "explorer.h"
#include "gfx.h"
#include "item_option.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "watchlist.h"
#include "z64.h"
#include "zu.h"

struct equipment_item_option
{
  int       shift;
  uint32_t  mask;
  int       start;
  int       length;
};

struct equipment_item_data
{
  int       shift;
  uint32_t  mask;
  int       start;
  int8_t    value;
};

struct capacity_item_data
{
  int       shift;
  uint32_t  mask;
  int8_t    value;
};

struct equipment_switch
{
  uint16_t  mask;
  int8_t    item_id;
};

struct item_switch
{
  int8_t *data;
  int8_t  item_id;
};

struct quest_item_switch
{
  uint32_t  mask;
  int       tile;
};

struct song_switch
{
  uint32_t  mask;
  uint32_t  color;
};

struct switch_info
{
  void     *data;
  uint32_t  mask;
};

struct dungeon_items
{
  struct switch_info  boss_key;
  struct switch_info  compass;
  struct switch_info  map;
  int8_t             *small_keys;
};

struct scene_category
{
  const char     *category_name;
  int             no_scenes;
  const uint8_t  *scenes;
};

struct byte_option
{
  void    *data;
  int8_t  *option_list;
  int      num_options;
};

struct slot_info
{
  uint8_t  *data;
  size_t    max;
};

struct memory_file
{
  z64_file_t  z_file;
  uint16_t    scene_index;
  uint32_t    scene_flags[9];
};

static uint8_t              profile = 0;
static struct menu          menu_main;
static struct menu          menu_global_watches;
static struct menu_item    *menu_font_option;
static struct menu_item    *menu_lag_unit_option;
static struct menu_item    *menu_watchlist;
static _Bool                menu_active = 0;
static int32_t              frames_queued = -1;
static int32_t              frame_counter = 0;
static int32_t              vi_offset = 0;
static struct memory_file  *memfile;
static _Bool                memfile_saved[SETTINGS_MEMFILE_MAX] = {0};
static uint8_t              memfile_slot = 0;
static _Bool                override_offset;

static uint16_t menu_font_options[] =
{
  RES_FONT_FIPPS,
  RES_FONT_NOTALOT35,
  RES_FONT_ORIGAMIMOMMY,
  RES_FONT_PCSENIOR,
  RES_FONT_PIXELINTV,
  RES_FONT_PRESSSTART2P,
  RES_FONT_SMWTEXTNC,
  RES_FONT_WERDNASRETURN,
};

static struct equipment_item_option equipment_item_list[] =
{
  {14, 0b111, Z64_ITEM_BULLET_BAG_30,   6},
  {0,  0b111, Z64_ITEM_QUIVER_30,       6},
  {3,  0b111, Z64_ITEM_BOMB_BAG_20,     6},
  {6,  0b111, Z64_ITEM_GORONS_BRACELET, 6},
  {9,  0b111, Z64_ITEM_SILVER_SCALE,    6},
  {12, 0b11,  Z64_ITEM_ADULTS_WALLET,   3},
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
  {0x00001000, 0xFFFFFF},
  {0x00002000, 0xFFFFFF},
  {0x00004000, 0xFFFFFF},
  {0x00008000, 0xFFFFFF},
  {0x00010000, 0xFFFFFF},
  {0x00020000, 0xFFFFFF},
  {0x00000040, 0x96FF64},
  {0x00000080, 0xFF5028},
  {0x00000100, 0x6496FF},
  {0x00000200, 0xFFA000},
  {0x00000400, 0xFF64FF},
  {0x00000800, 0xFFF064},
};

static struct scene_category scene_categories[] =
{
  {
    "dungeons",
    17, (uint8_t[])
    {
      0,   1,   2,   3,
      4,   5,   6,   7,
      8,   9,   10,  11,
      12,  13,  14,  15,
      26,
    },
  },
  {
    "bosses",
    10, (uint8_t[])
    {
      17,  18,  19,  20,
      21,  22,  23,  24,
      25,  79,
    },
  },
  {
    "towns",
    16, (uint8_t[])
    {
      82,  88,  85,  98,
      99,  32,  33,  34,
      27,  28,  29,  30,
      31,  35,  36,  37,
    },
  },
  {
    "houses",
    15, (uint8_t[])
    {
      52,  38,  39,  40,
      41,  42,  43,  53,
      54,  55,  57,  58,
      76,  77,  80,
    },
  },
  {
    "shops",
    12, (uint8_t[])
    {
      44,  45,  46,  47,
      48,  49,  50,  51,
      66,  75,  78,  16,
    },
  },
  {
    "misc",
    15, (uint8_t[])
    {
      67,  74,  72,  69,
      70,  56,  59,  61,
      65,  62,  63,  64,
      68,  71,  60,
    },
  },
  {
    "overworld",
    16, (uint8_t[])
    {
      81,  91,   86,  96,
      97,  84,   89,  87,
      95,  100,  90,  93,
      94,  92,   83,  73,
    },
  },
  {
    "beta",
    9, (uint8_t[])
    {
      101, 102, 103, 104,
      105, 106, 107, 108,
      109,
    },
  },
};

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
  "advance time",
  "no music",
  "items usable",
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

static void save_memory_file(void)
{
  struct memory_file *file = &memfile[memfile_slot];
  memcpy(&file->z_file, &z64_file, sizeof(file->z_file));
  file->scene_index = z64_game.scene_index;
  memcpy(&file->scene_flags, &z64_game.switch_flags,
         sizeof(file->scene_flags));
  memfile_saved[memfile_slot] = 1;
}

static void load_memory_file(void)
{
  if (!memfile_saved[memfile_slot])
    return;
  struct memory_file *file = &memfile[memfile_slot];
  memcpy(&z64_file, &file->z_file, sizeof(file->z_file));
  if (file->scene_index == z64_game.scene_index)
    memcpy(&z64_game.switch_flags, &file->scene_flags,
           sizeof(file->scene_flags));
}

static int generic_checkbox_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  int8_t *state = data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *state = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *state = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, *state);
  return 0;
}

static void main_return_proc(struct menu_item *item, void *data)
{
  menu_signal_leave(&menu_main);
  menu_active = 0;
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
  if (reason == MENU_CALLBACK_CHANGED)
    z64_file.equipment_items = (z64_file.equipment_items &
                                ~(d->mask << d->shift)) |
                                (d->value << d->shift);
  else if (reason == MENU_CALLBACK_THINK)
    d->value = (z64_file.equipment_items >> d->shift) & d->mask;
  return 0;
}

static int equipment_switch_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  struct equipment_switch *d = data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    z64_file.equipment |= d->mask;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    z64_file.equipment &= ~d->mask;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, z64_file.equipment & d->mask);
  return 0;
}

static int bgs_switch_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    z64_file.bgs_flag = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    z64_file.bgs_flag = 0;
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
      z64_file.button_items[Z64_ITEMBTN_B] = Z64_ITEM_KOKIRI_SWORD +
                                             equip_value - 1;
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

static int byte_option_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  struct byte_option *option = data;
  uint8_t *p = option->data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    int item_index = 0;
    for (int i = 0; i < option->num_options; ++i)
      if (*p == option->option_list[i]) {
        item_index = i;
        break;
      }
    if (menu_option_get(item) != item_index)
      menu_option_set(item, item_index);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *p = option->option_list[menu_option_get(item)];
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

static void reset_gs_proc(struct menu_item *item, void *data)
{
  memset(&z64_file.gs_flags, 0x00, sizeof(z64_file.gs_flags));
}

static void clear_flags_proc(struct menu_item *item, void *data)
{
  memset(&z64_game.switch_flags, 0x00, 0x24);
}

static void set_flags_proc(struct menu_item *item, void *data)
{
  memset(&z64_game.switch_flags, 0xFF, 0x24);
}

static void slot_dec_proc(struct menu_item *item, void *data)
{
  struct slot_info *info = data;
  *info->data = (*info->data + info->max - 1) % info->max;
}

static void slot_inc_proc(struct menu_item *item, void *data)
{
  struct slot_info *info = data;
  *info->data = (*info->data + 1) % info->max;
}

static void warp_proc(struct menu_item *item, void *data)
{
  z64_game.entrance_index = settings->warp_entrance;
  z64_file.cutscene_index = 0;
  z64_game.link_age = settings->warp_age;
  z64_game.scene_load_flags = 0x0014;
  override_offset = 1;
}

static void places_proc(struct menu_item *item, void *data)
{
  uintptr_t d = (uintptr_t)data;
  int scene_index = (d >> 8) & 0x00FF;
  int entrance_index = (d >> 0) & 0x00FF;
  for (int i = 0; i < Z64_ETAB_LENGTH; ++i) {
    z64_entrance_table_t *e = &z64_entrance_table[i];
    if (e->scene_index == scene_index && e->entrance_index == entrance_index) {
      z64_game.entrance_index = i;
      z64_file.cutscene_index = 0;
      z64_game.link_age = settings->warp_age;
      z64_game.scene_load_flags = 0x0014;
      override_offset = 1;
      if (zu_scene_info[scene_index].no_entrances > 1)
        menu_return(&menu_main);
      menu_return(&menu_main);
      menu_return(&menu_main);
      break;
    }
  }
}

static int menu_font_option_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  if (reason == MENU_CALLBACK_CHANGED) {
    settings->menu_font_resource_id = menu_font_options[menu_option_get(item)];
    struct gfx_font *font = resource_get(settings->menu_font_resource_id);
    menu_set_font(&menu_main, font);
    menu_set_cell_width(&menu_main, font->char_width + font->letter_spacing);
    menu_set_cell_height(&menu_main, font->char_height + font->line_spacing);
    menu_imitate(&menu_global_watches, &menu_main);
  }
  return 0;
}

static int menu_drop_shadow_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  if (reason == MENU_CALLBACK_CHANGED) {
    settings->menu_drop_shadow = menu_checkbox_get(item);
    gfx_mode_set(GFX_MODE_DROPSHADOW, settings->menu_drop_shadow);
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_drop_shadow);
  return 0;
}

static int menu_position_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  int dist = 2;
  if (z64_input_direct.pad & BUTTON_Z)
    dist *= 2;
  switch (reason) {
    case MENU_CALLBACK_NAV_UP:    settings->menu_y -= dist; break;
    case MENU_CALLBACK_NAV_DOWN:  settings->menu_y += dist; break;
    case MENU_CALLBACK_NAV_LEFT:  settings->menu_x -= dist; break;
    case MENU_CALLBACK_NAV_RIGHT: settings->menu_x += dist; break;
    default:
      break;
  }
  menu_set_pxoffset(&menu_main, settings->menu_x);
  menu_set_pyoffset(&menu_main, settings->menu_y);
  return 0;
}

static int generic_position_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  int16_t *x = data;
  int16_t *y = x + 1;
  int dist = 2;
  if (z64_input_direct.pad & BUTTON_Z)
    dist *= 2;
  switch (reason) {
    case MENU_CALLBACK_NAV_UP:    *y -= dist; break;
    case MENU_CALLBACK_NAV_DOWN:  *y += dist; break;
    case MENU_CALLBACK_NAV_LEFT:  *x -= dist; break;
    case MENU_CALLBACK_NAV_RIGHT: *x += dist; break;
    default:
      break;
  }
  return 0;
}

static void apply_settings()
{
  size_t no_font_options = sizeof(menu_font_options) /
                           sizeof(*menu_font_options);
  for (int i = 0; i < no_font_options; ++i)
    if (menu_font_options[i] == settings->menu_font_resource_id) {
      menu_option_set(menu_font_option, i);
      break;
    }
  struct gfx_font *font = resource_get(settings->menu_font_resource_id);
  menu_set_font(&menu_main, font);
  menu_set_cell_width(&menu_main, font->char_width + font->letter_spacing);
  menu_set_cell_height(&menu_main, font->char_height + font->line_spacing);
  gfx_mode_set(GFX_MODE_DROPSHADOW, settings->menu_drop_shadow);
  menu_set_pxoffset(&menu_main, settings->menu_x);
  menu_set_pyoffset(&menu_main, settings->menu_y);
  menu_imitate(&menu_global_watches, &menu_main);
  watchlist_fetch(menu_watchlist);
}

static void save_settings_proc(struct menu_item *item, void *data)
{
  watchlist_store(menu_watchlist);
  settings_save(profile);
}

static void load_settings_proc(struct menu_item *item, void *data)
{
  if (settings_load(profile))
    apply_settings();
}

static void restore_settings_proc(struct menu_item *item, void *data)
{
  settings_load_default();
  apply_settings();
}

static void clear_csp_proc(struct menu_item *item, void *data)
{
  static uint32_t null_cs[] = {0, 0};
  z64_game.cutscene_ptr = &null_cs;
}

static void load_room_proc(struct menu_item *item, void *data)
{
  uint8_t new_room_index = menu_intinput_get(data);
  if (new_room_index < z64_game.no_rooms) {
    if (new_room_index == z64_game.room_index) {
      z64_game.room_index = -1;
      z64_UnloadRoom(&z64_game, &z64_game.room_index);
      z64_game.room_ptr = NULL;
    }
    else {
      z64_LoadRoom(&z64_game, &z64_game.room_index, new_room_index);
      z64_UnloadRoom(&z64_game, &z64_game.room_index);
    }
  }
}

static void input_hook()
{
  if (frames_queued != 0)
    ((void(*)())z64_frame_input_func_addr)();
}

static void update_hook()
{
  if (frames_queued != 0) {
    if (frames_queued > 0)
      --frames_queued;
    ((void(*)())z64_frame_update_func_addr)();
  }
}

static void pause_proc(struct menu_item *item, void *data)
{
  uint32_t *input_call = (void*)z64_frame_input_call_addr;
  *input_call = MIPS_JAL(&input_hook);
  uint32_t *update_call = (void*)z64_frame_update_call_addr;
  *update_call = MIPS_JAL(&update_hook);
  if (frames_queued >= 0)
    frames_queued = -1;
  else
    frames_queued = 0;
}

static void advance_proc(struct menu_item *item, void *data)
{
  if (frames_queued >= 0)
    ++frames_queued;
  else
    pause_proc(item, data);
}

void main_hook()
{
  static uint16_t pad = 0;
  uint16_t pad_pressed = (pad ^ z64_input_direct.pad) &
                         z64_input_direct.pad;
  uint16_t pad_pressed_raw = pad_pressed;
  pad = z64_input_direct.pad;
  static int button_time[16] = {0};
  for (int i = 0; i < 16; ++i) {
    int button_state = (pad >> i) & 0x0001;
    button_time[i] = (button_time[i] + button_state) * button_state;
    if (button_time[i] >= 8)
      pad_pressed |= 1 << i;
  }

  gfx_mode_init();

  if (settings->cheats[CHEAT_ENERGY])
    z64_file.energy = z64_file.energy_capacity;
  if (settings->cheats[CHEAT_MAGIC])
    z64_file.magic = (z64_file.magic_capacity + 1) * 0x30;
  if (settings->cheats[CHEAT_STICKS]) {
    int stick_capacity[] = {1, 10, 20, 30, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_STICK] = stick_capacity[z64_file.stick_upgrade];
  }
  if (settings->cheats[CHEAT_NUTS]) {
    int nut_capacity[] = {1, 20, 30, 40, 1, 0x7F, 1, 0x7F};
    z64_file.ammo[Z64_SLOT_NUT] = nut_capacity[z64_file.nut_upgrade];
  }
  if (settings->cheats[CHEAT_BOMBS]) {
    int bomb_bag_capacity[] = {1, 20, 30, 40, 1, 1, 1, 1};
    z64_file.ammo[Z64_SLOT_BOMB] = bomb_bag_capacity[z64_file.bomb_bag];
  }
  if (settings->cheats[CHEAT_ARROWS]) {
    int quiver_capacity[] = {1, 30, 40, 50, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_BOW] = quiver_capacity[z64_file.quiver];
  }
  if (settings->cheats[CHEAT_SEEDS]) {
    int bullet_bag_capacity[] = {1, 30, 40, 50, 1, 10, 20, 30};
    z64_file.ammo[Z64_SLOT_SLINGSHOT] =
      bullet_bag_capacity[z64_file.bullet_bag];
  }
  if (settings->cheats[CHEAT_BOMBCHUS])
    z64_file.ammo[Z64_SLOT_BOMBCHU] = 50;
  if (settings->cheats[CHEAT_BEANS])
    z64_file.ammo[Z64_SLOT_BEANS] = 1;
  if (settings->cheats[CHEAT_KEYS]) {
    if (z64_game.scene_index >= 0x0000 && z64_game.scene_index <= 0x0010)
      z64_file.dungeon_keys[z64_game.scene_index] = 1;
  }
  if (settings->cheats[CHEAT_RUPEES]) {
    int wallet_capacity[] = {99, 200, 500, 0xFFFF};
    z64_file.rupees = wallet_capacity[z64_file.wallet];
  }
  if (settings->cheats[CHEAT_NL])
    z64_file.nayrus_love_timer = 0x044B;
  if (settings->cheats[CHEAT_FFTIME])
    z64_file.day_time += 0x0100;
  if (settings->cheats[CHEAT_NOMUSIC])
    z64_file.disable_music_flag = 0x0001;
  if (settings->cheats[CHEAT_USEITEMS])
    memset(&z64_game.restriction_flags, 0, sizeof(z64_game.restriction_flags));

  while (menu_think(&menu_global_watches))
    ;
  if (menu_active) {
    if (pad_pressed & BUTTON_D_UP)
      menu_navigate(&menu_main, MENU_NAVIGATE_UP);
    if (pad_pressed & BUTTON_D_DOWN)
      menu_navigate(&menu_main, MENU_NAVIGATE_DOWN);
    if (pad_pressed & BUTTON_D_LEFT) {
      if (pad & BUTTON_R)
        menu_return(&menu_main);
      else
        menu_navigate(&menu_main, MENU_NAVIGATE_LEFT);
    }
    if (pad_pressed & BUTTON_D_RIGHT)
      menu_navigate(&menu_main, MENU_NAVIGATE_RIGHT);
    if (pad_pressed & BUTTON_L) {
      if (pad & BUTTON_R) {
        menu_signal_leave(&menu_main);
        menu_active = 0;
      }
      else
        menu_activate(&menu_main);
    }
    while (menu_think(&menu_main))
      ;
  }
  else {
    /* l-activated commands */
    static uint16_t l_pad = 0;
    if (pad_pressed_raw & BUTTON_L)
      l_pad = pad;
    else if (!(pad & BUTTON_L))
      l_pad = 0;
    if (l_pad & BUTTON_R) {
      /* show menu */
      menu_signal_enter(&menu_main);
      menu_active = 1;
      l_pad = 0;
    }
    else if (l_pad & BUTTON_C_UP) {
      static uint32_t null_cs[] = {0, 0};
      z64_game.cutscene_ptr = &null_cs;
    }
    else if ((l_pad & BUTTON_A) && (l_pad & BUTTON_B))
      /* void out */
      z64_link.common.pos_1.y = z64_link.common.pos_2.y = -0x8000;
    else if (l_pad & BUTTON_A)
      /* reload zone */
      z64_game.scene_load_flags = 0x0014;
    else if (l_pad & BUTTON_B) {
      /* go to file select */
      z64_file.interface_flags  = 0x02;
      z64_game.scene_load_flags = 0x0014;
    }
    else if (l_pad)
      /* levitate */
      z64_link.common.vel_1.y = 6.34375;
    /* dpad-activated commands */
    static uint16_t d_pad = 0;
    if (!d_pad && (pad_pressed_raw & (BUTTON_D_UP | BUTTON_D_DOWN |
                                      BUTTON_D_LEFT | BUTTON_D_RIGHT)))
      d_pad = pad;
    else if (!(pad & (BUTTON_D_UP | BUTTON_D_DOWN |
                      BUTTON_D_LEFT | BUTTON_D_RIGHT)))
      d_pad = 0;
    if (d_pad & BUTTON_D_LEFT) {
      if (d_pad & BUTTON_R) {
        if (pad_pressed_raw & BUTTON_D_LEFT)
          /* save memory file */
          save_memory_file();
      }
      else {
        /* save position and orientation */
        uint8_t slot = settings->teleport_slot;
        settings->teleport_pos[slot] = z64_link.common.pos_2;
        settings->teleport_rot[slot] = z64_link.common.rot_2.y;
      }
    }
    else if (d_pad & BUTTON_D_RIGHT) {
      if (d_pad & BUTTON_R) {
        if (d_pad & BUTTON_A) {
          /* reset lag counter */
          frame_counter = 0;
          vi_offset = -(int32_t)z64_vi_counter;
        }
        else if (pad_pressed_raw & BUTTON_D_RIGHT)
          /* load memory file */
          load_memory_file();
      }
      else {
        /* teleport */
        uint8_t slot = settings->teleport_slot;
        z64_link.common.pos_1 = settings->teleport_pos[slot];
        z64_link.common.pos_2 = settings->teleport_pos[slot];
        z64_link.common.rot_2.y = settings->teleport_rot[slot];
        z64_link.target_yaw = settings->teleport_rot[slot];
      }
    }
    else if (d_pad & pad_pressed & BUTTON_D_DOWN)
      pause_proc(NULL, NULL);
    else if (d_pad & pad_pressed & BUTTON_D_UP)
      advance_proc(NULL, NULL);
  }

  struct gfx_font *font = menu_get_font(&menu_main, 1);
  uint8_t alpha = menu_get_alpha_i(&menu_main, 1);
  int cw = menu_get_cell_width(&menu_main, 1);
  int ch = menu_get_cell_height(&menu_main, 1);

  if (settings->input_display_enabled) {
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    gfx_printf(font, settings->input_display_x, settings->input_display_y,
               "%4i %4i", z64_input_direct.x, z64_input_direct.y);
    static struct
    {
      uint16_t    mask;
      const char *name;
      uint32_t    color;
    }
    buttons[] =
    {
      {BUTTON_A,        "A", 0x0000FF},
      {BUTTON_B,        "B", 0x00FF00},
      {BUTTON_START,    "S", 0xFF0000},
      {BUTTON_L,        "L", 0xC0C0C0},
      {BUTTON_R,        "R", 0xC0C0C0},
      {BUTTON_Z,        "Z", 0xC0C0C0},
      {BUTTON_C_UP,     "u", 0xFFFF00},
      {BUTTON_C_DOWN,   "d", 0xFFFF00},
      {BUTTON_C_LEFT,   "l", 0xFFFF00},
      {BUTTON_C_RIGHT,  "r", 0xFFFF00},
      {BUTTON_D_UP,     "u", 0xC0C0C0},
      {BUTTON_D_DOWN,   "d", 0xC0C0C0},
      {BUTTON_D_LEFT,   "l", 0xC0C0C0},
      {BUTTON_D_RIGHT,  "r", 0xC0C0C0},
    };
    for (int i = 0; i < sizeof(buttons) / sizeof(*buttons); ++i) {
      if (!(pad & buttons[i].mask))
        continue;
      gfx_mode_set(GFX_MODE_COLOR, (buttons[i].color << 8) | alpha);
      gfx_printf(font,
                 settings->input_display_x + cw * (12 + i),
                 settings->input_display_y,
                 "%s", buttons[i].name);
    }
  }

  {
    if (settings->lag_counter_enabled) {
      int cw = menu_get_cell_width(&menu_main, 1);
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
      int32_t lag_frames = (int32_t)z64_vi_counter + vi_offset - frame_counter;
      if (settings->lag_unit == SETTINGS_LAG_FRAMES)
        gfx_printf(menu_get_font(&menu_main, 1),
                   settings->lag_counter_x - cw * 8, settings->lag_counter_y,
                   "%8d", lag_frames);
      else if (settings->lag_unit == SETTINGS_LAG_SECONDS)
        gfx_printf(menu_get_font(&menu_main, 1),
                   settings->lag_counter_x - cw * 8, settings->lag_counter_y,
                   "%8.2f", lag_frames / 60.f);
    }
    frame_counter += z64_gameinfo.update_rate;
  }

  if (menu_active)
    menu_draw(&menu_main);
  menu_draw(&menu_global_watches);

  {
    static int splash_time = 230;
    if (splash_time > 0) {
      --splash_time;
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_printf(font, 16, Z64_SCREEN_HEIGHT - 6 - ch,
                 "gz-0.2.0 github.com/glankk/gz");
    }
  }

  gfx_flush();
}

void entrance_offset_hook()
{
  uint32_t at;
  uint32_t offset;
  __asm__ volatile (".set noat  \n"
                    "sw $at, %0 \n" : "=m"(at));
  if (override_offset) {
    offset = 0;
    override_offset = 0;
  }
  else
    offset = z64_file.scene_setup_index;
  __asm__ volatile ("lw $v1, %0 \n"
                    "lw $at, %1 \n" :: "m"(offset), "m"(at));
}

ENTRY void _start()
{
  /* startup */
  {
    init_gp();
    clear_bss();
    do_global_ctors();
  }

  /* load settings */
  if ((z64_input_direct.pad & BUTTON_START) || !settings_load(profile))
    settings_load_default();

  /* initialize gfx */
  {
    gfx_start();
    gfx_mode_configure(GFX_MODE_FILTER, G_TF_POINT);
    gfx_mode_configure(GFX_MODE_COMBINE, G_CC_MODE(G_CC_MODULATEIA_PRIM,
                                                   G_CC_MODULATEIA_PRIM));
  }

  /* install entrance offset hook */
  *(uint32_t*)z64_entrance_offset_hook_addr = MIPS_JAL(&entrance_offset_hook);

  /* disable map toggling */
  *(uint32_t*)z64_minimap_disable_1_addr = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x82C);
  *(uint32_t*)z64_minimap_disable_2_addr = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x98);

  /* initialize menus */
  {
    menu_init(&menu_main, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_main.selector = menu_add_button(&menu_main, 0, 0, "return",
                                         main_return_proc, NULL);
    static struct menu menu_warps;
    menu_add_submenu(&menu_main, 0, 1, &menu_warps, "warps");
    static struct menu menu_scene;
    menu_add_submenu(&menu_main, 0, 2, &menu_scene, "scene");
    static struct menu menu_cheats;
    menu_add_submenu(&menu_main, 0, 3, &menu_cheats, "cheats");
    static struct menu menu_inventory;
    menu_add_submenu(&menu_main, 0, 4, &menu_inventory, "inventory");
    static struct menu menu_equips;
    menu_add_submenu(&menu_main, 0, 5, &menu_equips, "equips");
    static struct menu menu_file;
    menu_add_submenu(&menu_main, 0, 6, &menu_file, "file");
    static struct menu menu_watches;
    menu_add_submenu(&menu_main, 0, 7, &menu_watches, "watches");
    static struct menu menu_settings;
    menu_add_submenu(&menu_main, 0, 8, &menu_settings, "settings");

    /* warps */
    menu_init(&menu_warps, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_warps.selector = menu_add_submenu(&menu_warps, 0, 0, NULL,
                                           "return");
    static struct menu places;
    menu_init(&places, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    places.selector = menu_add_submenu(&places, 0, 0, NULL, "return");
    size_t no_scene_categories = sizeof(scene_categories) /
                                 sizeof(*scene_categories);
    for (int i = 0; i < no_scene_categories; ++i) {
      struct scene_category *cat = &scene_categories[i];
      struct menu *cat_menu = malloc(sizeof(*cat_menu));
      menu_init(cat_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
      cat_menu->selector = menu_add_submenu(cat_menu, 0, 0, NULL, "return");
      for (int j = 0; j < cat->no_scenes; ++j) {
        uint8_t scene_index = cat->scenes[j];
        struct zu_scene_info *scene = &zu_scene_info[scene_index];
        if (scene->no_entrances > 1) {
          struct menu *scene_menu = malloc(sizeof(*scene_menu));
          menu_init(scene_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
          scene_menu->selector = menu_add_submenu(scene_menu, 0, 0, NULL,
                                                  "return");
          for (uint8_t k = 0; k < scene->no_entrances; ++k)
            menu_add_button(scene_menu, 0, 1 + k, scene->entrance_names[k],
                            places_proc, (void*)((scene_index << 8) | k));
          menu_add_submenu(cat_menu, 0, 1 + j, scene_menu,
                           scene->scene_name);
        }
        else
          menu_add_button(cat_menu, 0, 1 + j, scene->scene_name,
                          places_proc, (void*)((scene_index << 8) | 0));
      }
      menu_add_submenu(&places, 0, 1 + i, cat_menu,
                       cat->category_name);
    }
    menu_add_submenu(&menu_warps, 0, 1, &places, "places");
    menu_add_static(&menu_warps, 0, 2, "entrance", 0xC0C0C0);
    menu_add_intinput(&menu_warps, 9, 2, 16, 4,
                      halfword_mod_proc, &settings->warp_entrance);
    {
      static int8_t age_options[] = {0, 1};
      static struct byte_option option = {NULL, age_options, 2};
      option.data = &settings->warp_age;
      menu_add_static(&menu_warps, 0, 3, "age", 0xC0C0C0);
      menu_add_option(&menu_warps, 9, 3, "adult\0""child\0",
                      byte_option_proc, &option);
    }
    menu_add_button(&menu_warps, 0, 4, "warp", warp_proc, NULL);
    menu_add_button(&menu_warps, 0, 5, "clear cutscene pointer",
                    clear_csp_proc, NULL);

    /* scene */
    menu_init(&menu_scene, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_scene.selector = menu_add_submenu(&menu_scene, 0, 0, NULL,
                                           "return");
    {
      static struct menu explorer;
      explorer_create(&explorer);
      menu_add_submenu(&menu_scene, 0, 1, &explorer, "explorer");
    }
    menu_add_button(&menu_scene, 0, 2, "clear flags",
                    clear_flags_proc, NULL);
    menu_add_button(&menu_scene, 0, 3, "set flags",
                    set_flags_proc, NULL);
    menu_add_static(&menu_scene, 0, 4, "room", 0xC0C0C0);
    {
      struct menu_item *item;
      item = menu_add_intinput(&menu_scene, 5, 4, 16, 2, NULL, NULL);
      menu_add_button(&menu_scene, 0, 5, "load room", load_room_proc, item);
    }
    {
      static struct slot_info teleport_slot_info;
      teleport_slot_info.data = &settings->teleport_slot;
      teleport_slot_info.max = SETTINGS_TELEPORT_MAX;
      menu_add_static(&menu_scene, 0, 6, "teleport slot", 0xC0C0C0);
      menu_add_watch(&menu_scene, 16, 6,
                     (uint32_t)teleport_slot_info.data, WATCH_TYPE_U8);
      menu_add_button(&menu_scene, 14, 6, "-",
                      slot_dec_proc, &teleport_slot_info);
      menu_add_button(&menu_scene, 18, 6, "+",
                      slot_inc_proc, &teleport_slot_info);
    }
    menu_add_static(&menu_scene, 0, 7, "current scene", 0xC0C0C0);
    menu_add_watch(&menu_scene, 14, 7,
                   (uint32_t)&z64_game.scene_index, WATCH_TYPE_U16);
    menu_add_static(&menu_scene, 0, 8, "current room", 0xC0C0C0);
    menu_add_watch(&menu_scene, 14, 8,
                   (uint32_t)&z64_game.room_index, WATCH_TYPE_U8);
    menu_add_static(&menu_scene, 0, 9, "no. rooms", 0xC0C0C0);
    menu_add_watch(&menu_scene, 14, 9,
                   (uint32_t)&z64_game.no_rooms, WATCH_TYPE_U8);

    /* cheats */
    menu_init(&menu_cheats, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_cheats.selector = menu_add_submenu(&menu_cheats, 0, 0, NULL,
                                            "return");
    {
      for (int i = 0; i < CHEAT_MAX; ++i) {
        menu_add_static(&menu_cheats, 0, 1 + i, cheat_names[i], 0xC0C0C0);
        menu_add_checkbox(&menu_cheats, 14, 1 + i,
                          generic_checkbox_proc, &settings->cheats[i]);
      }
    }

    /* inventory */
    menu_init(&menu_inventory, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_inventory.selector = menu_add_submenu(&menu_inventory, 0, 0, NULL,
                                               "return");
    static struct menu menu_equipment_items;
    menu_add_submenu(&menu_inventory, 0, 1, &menu_equipment_items,
                     "equipment and items");
    static struct menu menu_quest_items;
    menu_add_submenu(&menu_inventory, 0, 2, &menu_quest_items,
                     "quest items");

    menu_init(&menu_equipment_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_equipment_items.selector = menu_add_submenu(&menu_equipment_items,
                                                     0, 0, NULL, "return");
    {
      struct gfx_texture *t_ab = resource_get(RES_ZICON_ACTION_BUTTONS);
      struct gfx_texture *t_am = resource_get(RES_ICON_AMOUNT);
      struct gfx_texture *t_on = resource_get(RES_ZICON_ITEM);
      struct gfx_texture *t_off = resource_get(RES_ZICON_ITEM_GRAY);
      size_t equipment_item_list_length = sizeof(equipment_item_list) /
                                          sizeof(*equipment_item_list);
      for (int i = 0; i < equipment_item_list_length; ++i) {
        struct equipment_item_option *option = &equipment_item_list[i];
        struct equipment_item_data *data = malloc(sizeof(*data));
        data->shift = option->shift;
        data->mask = option->mask;
        data->start = option->start;
        int8_t *item_list = malloc(option->length + 1);
        item_list[0] = Z64_ITEM_NULL;
        for (int j = 0; j < option->length; ++j)
          item_list[j + 1] = option->start + j;
        struct menu_item *item;
        item = item_option_create(&menu_equipment_items, 0, 2,
                                  option->length + 1, item_list,
                                  &data->value,
                                  NULL, t_ab, 1, NULL, 0,
                                  0xFFFFFF, .5f, .5625f, 0.f,
                                  equipment_item_proc, data);
        item->pyoffset = i * 18;
      }
      {
        struct gfx_texture *t = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                                   32, 32, 1, 8);
        for (int i = 0; i < 8; ++i)
          gfx_texture_copy_tile(t, i, t_on, Z64_ITEM_STICK, 0);
        gfx_texture_copy_tile(t, 0, t_am, 0, 1);
        gfx_texture_copy_tile(t, 1, t_am, 1, 1);
        gfx_texture_copy_tile(t, 2, t_am, 2, 1);
        gfx_texture_copy_tile(t, 3, t_am, 3, 1);
        gfx_texture_copy_tile(t, 4, t_am, 6, 1);
        gfx_texture_copy_tile(t, 5, t_am, 8, 1);
        gfx_texture_copy_tile(t, 6, t_am, 9, 1);
        gfx_texture_copy_tile(t, 7, t_am, 10, 1);
        static int8_t options[] = {0, 1, 2, 3, 4, 5, 6, 7};
        static struct capacity_item_data data = {17, 0b111};
        struct menu_item *item;
        item = item_option_create(&menu_equipment_items, 0, 2,
                                  8, options, &data.value,
                                  t, t_ab, 1, NULL, 0,
                                  0xFFFFFF, .5f, .5625f, 0.f,
                                  capacity_item_proc, &data);
        item->pxoffset = 1 * 18;
        item->pyoffset = 5 * 18;
      }
      {
        struct gfx_texture *t = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                                   32, 32, 1, 8);
        for (int i = 0; i < 8; ++i)
          gfx_texture_copy_tile(t, i, t_on, Z64_ITEM_NUT, 0);
        gfx_texture_copy_tile(t, 0, t_am, 0, 1);
        gfx_texture_copy_tile(t, 1, t_am, 2, 1);
        gfx_texture_copy_tile(t, 2, t_am, 3, 1);
        gfx_texture_copy_tile(t, 3, t_am, 4, 1);
        gfx_texture_copy_tile(t, 4, t_am, 6, 1);
        gfx_texture_copy_tile(t, 5, t_am, 11, 1);
        gfx_texture_copy_tile(t, 6, t_am, 6, 1);
        gfx_texture_copy_tile(t, 7, t_am, 11, 1);
        static int8_t options[] = {0, 1, 2, 3, 4, 5, 6, 7};
        static struct capacity_item_data data = {20, 0b111};
        struct menu_item *item;
        item = item_option_create(&menu_equipment_items, 0, 2,
                                  8, options, &data.value,
                                  t, t_ab, 1, NULL, 0,
                                  0xFFFFFF, .5f, .5625f, 0.f,
                                  capacity_item_proc, &data);
        item->pxoffset = 2 * 18;
        item->pyoffset = 5 * 18;
      }
      size_t equipment_list_length = sizeof(equipment_list) /
                                     sizeof(*equipment_list);
      for (int i = 0; i < equipment_list_length; ++i) {
        struct menu_item *item;
        item = menu_add_switch(&menu_equipment_items, 0, 2,
                               t_on, equipment_list[i].item_id, 0xFFFFFF,
                               t_off, equipment_list[i].item_id, 0xFFFFFF,
                               .5f,
                               equipment_switch_proc, &equipment_list[i]);
        item->pxoffset = 20 + i % 3 * 18;
        item->pyoffset = i / 3 * 18;
      }
      {
        struct menu_item *item;
        item = menu_add_switch(&menu_equipment_items, 0, 2,
                               t_on, Z64_ITEM_BIGGORON_SWORD, 0xFFFFFF,
                               t_off, Z64_ITEM_BIGGORON_SWORD, 0xFFFFFF,
                               .5f,
                               bgs_switch_proc, NULL);
        item->pxoffset = 20 + 1 * 18;
        item->pyoffset = 4 * 18;
      }
      size_t item_list_length = sizeof(item_list) / sizeof(*item_list);
      for (int i = 0; i < item_list_length; ++i) {
        struct menu_item *item;
        item = menu_add_switch(&menu_equipment_items, 0, 2,
                               t_on, item_list[i].item_id, 0xFFFFFF,
                               t_off, item_list[i].item_id, 0xFFFFFF,
                               .5f,
                               item_switch_proc, &item_list[i]);
        item->pxoffset = 76 + i % 6 * 18;
        item->pyoffset = i / 6 * 18;
      }
      {
        size_t bottle_options_length = sizeof(bottle_options) /
                                       sizeof(*bottle_options);
        for (int i = 0; i < 4; ++i) {
          struct menu_item *item;
          item = item_option_create(&menu_equipment_items, 0, 2,
                                    bottle_options_length, bottle_options,
                                    &z64_file.items[Z64_SLOT_BOTTLE_1 + i],
                                    NULL, t_ab, 1, NULL, 0,
                                    0xFFFFFF, .5f, .5625f, 0.f,
                                    NULL, NULL);
          item->pxoffset = 76 + i * 18;
          item->pyoffset = 72;
        }
        size_t adult_trade_options_length = sizeof(adult_trade_options) /
                                            sizeof(*adult_trade_options);
        struct menu_item *item;
        item = item_option_create(&menu_equipment_items, 0, 2,
                                  adult_trade_options_length,
                                  adult_trade_options,
                                  &z64_file.items[Z64_SLOT_ADULT_TRADE],
                                  NULL, t_ab, 1, NULL, 0,
                                  0xFFFFFF, .5f, .5625f, 0.f,
                                  NULL, NULL);
        item->pxoffset = 76 + 4 * 18;
        item->pyoffset = 72;
        size_t child_trade_options_length = sizeof(child_trade_options) /
                                            sizeof(*child_trade_options);
        item = item_option_create(&menu_equipment_items, 0, 2,
                                  child_trade_options_length,
                                  child_trade_options,
                                  &z64_file.items[Z64_SLOT_CHILD_TRADE],
                                  NULL, t_ab, 1, NULL, 0,
                                  0xFFFFFF, .5f, .5625f, 0.f,
                                  NULL, NULL);
        item->pxoffset = 76 + 5 * 18;
        item->pyoffset = 72;
      }
    }
    menu_init(&menu_quest_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_quest_items.selector = menu_add_submenu(&menu_quest_items, 0, 0, NULL,
                                                 "return");
    menu_add_static(&menu_quest_items, 0, 1, "max energy", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 1, 16, 4,
                      halfword_mod_proc, &z64_file.energy_capacity);
    menu_add_static(&menu_quest_items, 0, 2, "defense", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 2, 16, 2,
                      byte_mod_proc, &z64_file.defense_hearts);
    menu_add_static(&menu_quest_items, 0, 3, "magic", 0xC0C0C0);
    menu_add_checkbox(&menu_quest_items, 14, 3, magic_acquired_proc, NULL);
    menu_add_static(&menu_quest_items, 0, 4, "max magic", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 4, 16, 2,
                      magic_capacity_proc, NULL);
    menu_add_static(&menu_quest_items, 0, 5, "gs tokens", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 5, 10, 3,
                      halfword_mod_proc, &z64_file.gs_tokens);
    menu_add_static(&menu_quest_items, 0, 6, "heart pieces", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 6, 16, 2,
                      byte_mod_proc, &z64_file.quest_items);
    {
      struct gfx_texture *t_on = resource_get(RES_ZICON_ITEM_24);
      static struct dungeon_items data =
      {
        {&z64_file.dungeon_items[0].items, 0b001},
        {&z64_file.dungeon_items[0].items, 0b010},
        {&z64_file.dungeon_items[0].items, 0b100},
        &z64_file.dungeon_keys[0],
      };
      menu_add_static(&menu_quest_items, 0, 7, "dungeon", 0xC0C0C0);
      {
        char options[1024];
        char *p = options;
        for (int i = 0; i < 17; ++i) {
          size_t l = strlen(zu_scene_info[i].scene_name) + 1;
          memcpy(p, zu_scene_info[i].scene_name, l);
          p += l;
        }
        *p = 0;
        menu_add_option(&menu_quest_items, 14, 7, options,
                        dungeon_option_proc, &data);
      }
      menu_add_static(&menu_quest_items, 0, 8, "small keys", 0xC0C0C0);
      menu_add_intinput(&menu_quest_items, 14, 8, 16, 2,
                        byte_mod_indirect_proc, &data.small_keys);
      struct menu_item *item;
      item = menu_add_switch(&menu_quest_items, 0, 10,
                             t_on, 14, 0xFFFFFF,
                             t_on, 14, 0x606060,
                             2.f / 3.f,
                             byte_switch_proc, &data.boss_key);
      item->pxoffset = 5 * 18 + 20;
      item = menu_add_switch(&menu_quest_items, 0, 10,
                             t_on, 15, 0xFFFFFF,
                             t_on, 15, 0x606060,
                             2.f / 3.f,
                             byte_switch_proc, &data.compass);
      item->pxoffset = 5 * 18 + 20;
      item->pyoffset = 1 * 18;
      item = menu_add_switch(&menu_quest_items, 0, 10,
                             t_on, 16, 0xFFFFFF,
                             t_on, 16, 0x606060,
                             2.f / 3.f,
                             byte_switch_proc, &data.map);
      item->pxoffset = 5 * 18 + 20;
      item->pyoffset = 2 * 18;
    }
    {
      struct gfx_texture *t_on = resource_get(RES_ZICON_ITEM_24);
      struct gfx_texture *t_n = resource_get(RES_ZICON_NOTE);
      size_t quest_item_list_length = sizeof(quest_item_list) /
                                      sizeof(*quest_item_list);
      for (int i = 0; i < quest_item_list_length; ++i) {
        struct quest_item_switch *d = &quest_item_list[i];
        struct menu_item *item;
        item = menu_add_switch(&menu_quest_items, 0, 10,
                               t_on, d->tile, 0xFFFFFF,
                               t_on, d->tile, 0x606060,
                               2.f / 3.f,
                               quest_item_switch_proc, (void*)d->mask);
        item->pxoffset = i % 6 * 18;
        item->pyoffset = i / 6 * 18;
      }
      size_t song_list_length = sizeof(song_list) / sizeof(*song_list);
      for (int i = 0; i < song_list_length; ++i) {
        struct song_switch *d = &song_list[i];
        struct menu_item *item;
        uint8_t r = (d->color >> 16) & 0xFF;
        uint8_t g = (d->color >> 8)  & 0xFF;
        uint8_t b = (d->color >> 0)  & 0xFF;
        uint32_t color_off = ((r * 0x60 / 0xFF) << 16) |
                             ((g * 0x60 / 0xFF) << 8)  |
                             ((b * 0x60 / 0xFF) << 0);
        item = menu_add_switch(&menu_quest_items, 0, 10,
                               t_n, 0, d->color,
                               t_n, 0, color_off,
                               2.f / 3.f,
                               quest_item_switch_proc, (void*)d->mask);
        item->pxoffset = i % 6 * 18;
        item->pyoffset = 2 * 18 + i / 6 * 18;
      }
    }

    /* equips */
    menu_init(&menu_equips, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_equips.selector = menu_add_submenu(&menu_equips, 0, 0, NULL,
                                            "return");
    {
      struct gfx_texture *t_on = resource_get(RES_ZICON_ITEM);
      struct gfx_texture *t_off = resource_get(RES_ZICON_ITEM_GRAY);
      for (int i = 0; i < 3 * 4; ++i) {
        struct menu_item *item;
        item = menu_add_switch(&menu_equips, 0, 2,
                               t_on, Z64_ITEM_KOKIRI_SWORD + i, 0xFFFFFF,
                               t_off, Z64_ITEM_KOKIRI_SWORD + i, 0xFFFFFF,
                               .5f,
                               equip_switch_proc,
                               (void*)((i / 3) | ((1 + i % 3) << 4)));
        item->pxoffset = i % 3 * 18;
        item->pyoffset = i / 3 * 18;
      }
    }
    {
      struct gfx_texture *t_ab = resource_get(RES_ZICON_ACTION_BUTTONS);
      struct menu_item *item;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_B],
                                NULL, t_ab, 0, NULL, 0,
                                0x009600, .5625f, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_B);
      item->pxoffset = 0;
      item->pyoffset = 74;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_CL],
                                NULL, t_ab, 0, t_ab, 2,
                                0xFFA000, .5f, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_CL);
      item->pxoffset = 20;
      item->pyoffset = 74;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_CD],
                                NULL, t_ab, 0, t_ab, 3,
                                0xFFA000, .5f, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_CD);
      item->pxoffset = 34;
      item->pyoffset = 86;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_CR],
                                NULL, t_ab, 0, t_ab, 4,
                                0xFFA000, .5, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_CR);
      item->pxoffset = 48;
      item->pyoffset = 74;
    }

    /* file */
    menu_init(&menu_file, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_file.selector = menu_add_submenu(&menu_file, 0, 0, NULL,
                                          "return");
    menu_add_button(&menu_file, 0, 1, "restore skulltulas",
                    reset_gs_proc, NULL);
    {
      memfile = malloc(sizeof(*memfile) * SETTINGS_MEMFILE_MAX);
      static struct slot_info memfile_slot_info =
      {
        &memfile_slot, SETTINGS_MEMFILE_MAX,
      };
      menu_add_static(&menu_file, 0, 2, "memory file", 0xC0C0C0);
      menu_add_watch(&menu_file, 14, 2,
                     (uint32_t)memfile_slot_info.data, WATCH_TYPE_U8);
      menu_add_button(&menu_file, 12, 2, "-",
                      slot_dec_proc, &memfile_slot_info);
      menu_add_button(&menu_file, 16, 2, "+",
                      slot_inc_proc, &memfile_slot_info);
    }
    {
      static int8_t language_options[] = {0x00, 0x01};
      static struct byte_option language_option_data =
      {
        &z64_file.language, language_options, 2,
      };
      menu_add_static(&menu_file, 0, 3, "language", 0xC0C0C0);
      menu_add_option(&menu_file, 12, 3, "japanese\0""english\0",
                      byte_option_proc, &language_option_data);
      static int8_t target_options[] = {0x00, 0x01};
      static struct byte_option target_option_data =
      {
        &z64_file.z_targeting, target_options, 2,
      };
      menu_add_static(&menu_file, 0, 4, "z targeting", 0xC0C0C0);
      menu_add_option(&menu_file, 12, 4, "switch\0""hold\0",
                      byte_option_proc, &target_option_data);
    }

    menu_init(&menu_watches, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_watches.selector = menu_add_submenu(&menu_watches, 0, 0, NULL,
                                             "return");
    menu_init(&menu_global_watches, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_imitate(&menu_global_watches, &menu_main);
    menu_watchlist = watchlist_create(&menu_watches, &menu_global_watches,
                                      0, 1);
    watchlist_fetch(menu_watchlist);

    /* settings */
    menu_init(&menu_settings, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_settings.selector = menu_add_submenu(&menu_settings, 0, 0, NULL,
                                              "return");
    {
      static struct slot_info profile_slot_info =
      {
        &profile, SETTINGS_PROFILE_MAX,
      };
      menu_add_static(&menu_settings, 0, 1, "profile", 0xC0C0C0);
      menu_add_watch(&menu_settings, 16, 1,
                     (uint32_t)profile_slot_info.data, WATCH_TYPE_U8);
      menu_add_button(&menu_settings, 14, 1, "-",
                      slot_dec_proc, &profile_slot_info);
      menu_add_button(&menu_settings, 18, 1, "+",
                      slot_inc_proc, &profile_slot_info);
    }
    menu_add_static(&menu_settings, 0, 2, "menu", 0xFFFFFF);
    menu_add_static(&menu_settings, 2, 3, "font", 0xC0C0C0);
    menu_font_option = menu_add_option(&menu_settings, 14, 2,
                                       "fipps\0""notalot35\0"
                                       "origami mommy\0""pc senior\0"
                                       "pixel intv\0""press start 2p\0"
                                       "smw text nc\0""werdna's return\0",
                                       menu_font_option_proc, NULL);
    menu_add_static(&menu_settings, 2, 4, "drop shadow", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 4,
                      menu_drop_shadow_proc, NULL);
    menu_add_static(&menu_settings, 2, 5, "position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 5, menu_position_proc, NULL);
    menu_add_static(&menu_settings, 0, 6, "input display", 0xFFFFFF);
    menu_add_static(&menu_settings, 2, 7, "enabled", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 7,
                      generic_checkbox_proc, &settings->input_display_enabled);
    menu_add_static(&menu_settings, 2, 8, "position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 8,
                         generic_position_proc, &settings->input_display_x);
    menu_add_static(&menu_settings, 0, 9, "lag counter", 0xFFFFFF);
    menu_add_static(&menu_settings, 2, 10, "enabled", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 10,
                      generic_checkbox_proc, &settings->lag_counter_enabled);
    menu_add_static(&menu_settings, 2, 11, "unit", 0xC0C0C0);
    static int8_t lag_unit_options[] =
    {
      SETTINGS_LAG_FRAMES,
      SETTINGS_LAG_SECONDS,
    };
    static struct byte_option lag_unit_data = {NULL, lag_unit_options, 2};
    lag_unit_data.data = &settings->lag_unit;
    menu_lag_unit_option = menu_add_option(&menu_settings, 14, 11,
                                           "frames\0""seconds\0",
                                           byte_option_proc, &lag_unit_data);
    menu_add_static(&menu_settings, 2, 12, "position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 12,
                         generic_position_proc, &settings->lag_counter_x);
    menu_add_button(&menu_settings, 0, 13, "save settings",
                    save_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, 14, "load settings",
                    load_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, 15, "restore defaults",
                    restore_settings_proc, NULL);
  }

  /* reflect loaded settings */
  apply_settings();

  /* replace start routine with a jump to main hook */
  {
    uint32_t main_jump[] =
    {
      MIPS_SW(MIPS_RA, -4, MIPS_SP),
      MIPS_LA(MIPS_RA, &main_hook),
      MIPS_JR(MIPS_RA),
      MIPS_LW(MIPS_RA, -4, MIPS_SP),
    };
    memcpy(&_start, &main_jump, sizeof(main_jump));
  }
}


/* custom support library */
#include <startup.c>
#include <vector/vector.c>
#include <list/list.c>
#include <grc.c>
