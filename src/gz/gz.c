#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <startup.h>
#include <mips.h>
#include <n64.h>
#include "explorer.h"
#include "gfx.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "watchlist.h"
#include "z64.h"
#include "zu.h"

struct switch_info
{
  const char *name;
  void       *data;
  uint8_t     mask;
};

struct option_info
{
  void       *data;
  int         shift;
  uint32_t    mask;
};

struct dungeon_menu_data
{
  struct switch_info  boss_key_switch_info;
  struct switch_info  compass_switch_info;
  struct switch_info  map_switch_info;
  int8_t             *keys_ptr;
};

struct byte_option
{
  void    *data;
  int8_t  *option_list;
  int      num_options;
};

static struct
{
  struct menu_item *entrance;
  struct menu_item *age;
  _Bool             override_offset;
} warp_info;

static struct menu        menu_main;
static struct menu_item  *menu_font_option;
static struct menu_item  *menu_lag_unit_option;
static struct menu_item  *menu_watchlist;
static _Bool              menu_active   = 0;
static int                tp_slot       = 0;
static int                frames_queued = -1;

static int cheats_energy    = 0;
static int cheats_magic     = 0;
static int cheats_sticks    = 0;
static int cheats_nuts      = 0;
static int cheats_bombs     = 0;
static int cheats_arrows    = 0;
static int cheats_seeds     = 0;
static int cheats_bombchus  = 0;
static int cheats_beans     = 0;
static int cheats_keys      = 0;
static int cheats_rupees    = 0;
static int cheats_nayru     = 0;
static int cheats_time      = 0;

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

static struct switch_info equipment_list[] =
{
  {"kokiri sword",          (char*)&z64_file.equipment + 1, 0b00000001},
  {"master sword",          (char*)&z64_file.equipment + 1, 0b00000010},
  {"giant's knife",         (char*)&z64_file.equipment + 1, 0b00000100},
  {"deku shield",           (char*)&z64_file.equipment + 1, 0b00010000},
  {"hylian shield",         (char*)&z64_file.equipment + 1, 0b00100000},
  {"mirror shield",         (char*)&z64_file.equipment + 1, 0b01000000},
  {"kokiri tunic",          (char*)&z64_file.equipment + 0, 0b00000001},
  {"goron tunic",           (char*)&z64_file.equipment + 0, 0b00000010},
  {"zora tunic",            (char*)&z64_file.equipment + 0, 0b00000100},
  {"kokiri boots",          (char*)&z64_file.equipment + 0, 0b00010000},
  {"iron boots",            (char*)&z64_file.equipment + 0, 0b00100000},
  {"hover boots",           (char*)&z64_file.equipment + 0, 0b01000000},
  {"broken giant's knife",  (char*)&z64_file.equipment + 1, 0b00001000},
  {"biggoron's sword",             &z64_file.bgs_flag,      0b00000001},
};

static struct switch_info item_list[] =
{
  {"deku stick",
   &z64_file.items[Z64_SLOT_STICK],         Z64_ITEM_STICK},
  {"deku nut",
   &z64_file.items[Z64_SLOT_NUT],           Z64_ITEM_NUT},
  {"bomb",
   &z64_file.items[Z64_SLOT_BOMB],          Z64_ITEM_BOMB},
  {"bow",
   &z64_file.items[Z64_SLOT_BOW],           Z64_ITEM_BOW},
  {"fire arrow",
   &z64_file.items[Z64_SLOT_FIRE_ARROW],    Z64_ITEM_FIRE_ARROW},
  {"ice arrow",
   &z64_file.items[Z64_SLOT_ICE_ARROW],     Z64_ITEM_ICE_ARROW},
  {"light arrow",
   &z64_file.items[Z64_SLOT_LIGHT_ARROW],   Z64_ITEM_LIGHT_ARROW},
  {"din's fire",
   &z64_file.items[Z64_SLOT_DINS_FIRE],     Z64_ITEM_DINS_FIRE},
  {"farore's wind",
   &z64_file.items[Z64_SLOT_FARORES_WIND],  Z64_ITEM_FARORES_WIND},
  {"nayru's love",
   &z64_file.items[Z64_SLOT_NAYRUS_LOVE],   Z64_ITEM_NAYRUS_LOVE},
  {"slingshot",
   &z64_file.items[Z64_SLOT_SLINGSHOT],     Z64_ITEM_SLINGSHOT},
  {"fairy ocarina",
   &z64_file.items[Z64_SLOT_OCARINA],       Z64_ITEM_FAIRY_OCARINA},
  {"ocarina of time",
   &z64_file.items[Z64_SLOT_OCARINA],       Z64_ITEM_OCARINA_OF_TIME},
  {"bombchu",
   &z64_file.items[Z64_SLOT_BOMBCHU],       Z64_ITEM_BOMBCHU},
  {"hookshot",
   &z64_file.items[Z64_SLOT_HOOKSHOT],      Z64_ITEM_HOOKSHOT},
  {"longshot",
   &z64_file.items[Z64_SLOT_HOOKSHOT],      Z64_ITEM_LONGSHOT},
  {"boomerang",
   &z64_file.items[Z64_SLOT_BOOMERANG],     Z64_ITEM_BOOMERANG},
  {"lens of truth",
   &z64_file.items[Z64_SLOT_LENS],          Z64_ITEM_LENS},
  {"magic bean",
   &z64_file.items[Z64_SLOT_BEANS],         Z64_ITEM_BEANS},
  {"megaton hammer",
   &z64_file.items[Z64_SLOT_HAMMER],        Z64_ITEM_HAMMER},
};

static struct switch_info song_list[] =
{
  {"zelda's lullaby",       (char*)&z64_file.quest_items + 2, 0b00010000},
  {"epona's song",          (char*)&z64_file.quest_items + 2, 0b00100000},
  {"saria's song",          (char*)&z64_file.quest_items + 2, 0b01000000},
  {"sun's song",            (char*)&z64_file.quest_items + 2, 0b10000000},
  {"song of time",          (char*)&z64_file.quest_items + 1, 0b00000001},
  {"song of storms",        (char*)&z64_file.quest_items + 1, 0b00000010},
  {"minuet",                (char*)&z64_file.quest_items + 3, 0b01000000},
  {"bolero",                (char*)&z64_file.quest_items + 3, 0b10000000},
  {"serenade",              (char*)&z64_file.quest_items + 2, 0b00000001},
  {"requiem",               (char*)&z64_file.quest_items + 2, 0b00000010},
  {"nocturne",              (char*)&z64_file.quest_items + 2, 0b00000100},
  {"prelude",               (char*)&z64_file.quest_items + 2, 0b00001000},
};

static struct switch_info reward_list[] =
{
  {"kokiri's emerald",      (char*)&z64_file.quest_items + 1, 0b00000100},
  {"goron's ruby",          (char*)&z64_file.quest_items + 1, 0b00001000},
  {"zora's sapphire",       (char*)&z64_file.quest_items + 1, 0b00010000},
  {"light medal",           (char*)&z64_file.quest_items + 3, 0b00100000},
  {"forest medal",          (char*)&z64_file.quest_items + 3, 0b00000001},
  {"fire   medal",          (char*)&z64_file.quest_items + 3, 0b00000010},
  {"water  medal",          (char*)&z64_file.quest_items + 3, 0b00000100},
  {"spirit medal",          (char*)&z64_file.quest_items + 3, 0b00001000},
  {"shadow medal",          (char*)&z64_file.quest_items + 3, 0b00010000},
};

struct scene_category
{
  const char     *category_name;
  int             no_scenes;
  const uint8_t  *scenes;
}
scene_categories[] =
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
    12, (uint8_t[])
    {
      27,  28,  29,  30,
      31,  32,  33,  34,
      35,  36,  37,  82,
    },
  },
  {
    "houses",
    15, (uint8_t[])
    {
      38,  39,  40,  41,
      42,  43,  52,  53,
      54,  55,  57,  58,
      76,  77,  80,
    },
  },
  {
    "shops",
    12, (uint8_t[])
    {
      16,  44,  45,  46,
      47,  48,  49,  50,
      51,  66,  75,  78,
    },
  },
  {
    "misc",
    15, (uint8_t[])
    {
      56,  59,  60,  61,
      62,  63,  64,  65,
      67,  68,  69,  70,
      71,  72,  74,
    },
  },
  {
    "overworld",
    20, (uint8_t[])
    {
      73,  81,  83,  84,
      85,  86,  87,  88,
      89,  90,  91,  92,
      93,  94,  95,  96,
      97,  98,  99,  100,
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
static int no_scene_categories = sizeof(scene_categories) /
                                 sizeof(*scene_categories);

static int generic_switch_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  int *state = data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *state = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *state = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, *state);
  return 0;
}

static int generic_checkbox_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  int *state = data;
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

static int equipment_switch_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  struct switch_info *e = data;
  uint8_t *p = e->data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *p |= e->mask;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *p &= ~e->mask;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, (*p & e->mask) == e->mask);
  return 0;
}

static int equipment_option_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  struct option_info *e = data;
  uint32_t *p = e->data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    int value = (*p >> e->shift) & e->mask;
    if (menu_option_get(item) != value)
      menu_option_set(item, value);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *p = (*p & ~(e->mask << e->shift)) |
         (menu_option_get(item) << e->shift);
  return 0;
}

static int equip_option_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  int equip_row = (int)data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    int value = (z64_file.equips >> equip_row * 4) & 0x000F;
    if (menu_option_get(item) != value)
      menu_option_set(item, value);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    z64_file.equips = (z64_file.equips & ~(0x000F << equip_row * 4)) |
                      (menu_option_get(item) << equip_row * 4);
  return 0;
}

static int byte_mod_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  uint8_t *data_ptr = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *data_ptr)
      menu_intinput_set(item, *data_ptr);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *data_ptr = menu_intinput_get(item);
  return 0;
}

static int halfword_mod_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  uint16_t *data_ptr = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *data_ptr)
      menu_intinput_set(item, *data_ptr);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *data_ptr = menu_intinput_get(item);
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

static int magic_switch_proc(struct menu_item *item,
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
    menu_switch_set(item, z64_file.magic_acquired != 0x00);
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
  struct dungeon_menu_data *dungeon_menu_data = data;
  if (reason == MENU_CALLBACK_DEACTIVATE) {
    int dungeon = menu_option_get(item);
    uint8_t *items = &z64_file.dungeon_items[dungeon].items;
    int8_t *keys = &z64_file.dungeon_keys[dungeon];
    dungeon_menu_data->boss_key_switch_info.data = items;
    dungeon_menu_data->map_switch_info.data = items;
    dungeon_menu_data->compass_switch_info.data = items;
    dungeon_menu_data->keys_ptr = keys;
  }
  return 0;
}

static int dungeon_keys_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  struct dungeon_menu_data *dungeon_menu_data = data;
  int8_t *keys = dungeon_menu_data->keys_ptr;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *keys)
      menu_intinput_set(item, *keys);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *keys = menu_intinput_get(item);
  return 0;
}

static int swordless_proc(struct menu_item *item,
                          enum menu_callback_reason reason,
                          void *data)
{
  uint16_t *flag_ptr = &z64_file.inf_table[29];
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != (*flag_ptr & 0x0001))
      menu_option_set(item, *flag_ptr & 0x0001);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *flag_ptr = (*flag_ptr & ~0x0001) | menu_option_get(item);
  return 0;
}

static int item_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  struct switch_info *e = data;
  uint8_t *p = e->data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *p = e->mask;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *p = -1;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, *p == e->mask);
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

static void tp_slot_dec_proc(struct menu_item *item, void *data)
{
  struct menu_item *tp_slot_display = data;
  tp_slot = (tp_slot - 1) % 10;
  if (tp_slot < 0)
    tp_slot += 10;
  tp_slot_display->text[0] = '0' + tp_slot;
}

static void tp_slot_inc_proc(struct menu_item *item, void *data)
{
  struct menu_item *tp_slot_display = data;
  tp_slot = (tp_slot + 1) % 10;
  tp_slot_display->text[0] = '0' + tp_slot;
}

static void warp_proc(struct menu_item *item, void *data)
{
  z64_game.entrance_index = menu_intinput_get(warp_info.entrance);
  z64_file.cutscene_index = 0;
  z64_game.link_age = menu_option_get(warp_info.age);
  z64_game.scene_load_flags = 0x0014;
  warp_info.override_offset = 1;
}

static void places_proc(struct menu_item *item, void *data)
{
  uintptr_t d = (uintptr_t)data;
  int scene_index = (d & 0xFF00) >> 8;
  int entrance_index = (d & 0x00FF) >> 0;
  for (int i = 0; i < Z64_ETAB_LENGTH; ++i) {
    z64_entrance_table_t *e = &z64_entrance_table[i];
    if (e->scene_index == scene_index && e->entrance_index == entrance_index) {
      z64_game.entrance_index = i;
      z64_file.cutscene_index = 0;
      z64_game.link_age = menu_option_get(warp_info.age);
      z64_game.scene_load_flags = 0x0014;
      warp_info.override_offset = 1;
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
  }
  return 0;
}

static int menu_drop_shadow_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  if (reason == MENU_CALLBACK_CHANGED) {
    settings->menu_drop_shadow = menu_checkbox_get(item);
    gfx_mode_drop_shadow(settings->menu_drop_shadow);
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_drop_shadow);
  return 0;
}

static int menu_position_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  switch (reason) {
    case MENU_CALLBACK_NAV_UP:    settings->menu_y -= 2; break;
    case MENU_CALLBACK_NAV_DOWN:  settings->menu_y += 2; break;
    case MENU_CALLBACK_NAV_LEFT:  settings->menu_x -= 2; break;
    case MENU_CALLBACK_NAV_RIGHT: settings->menu_x += 2; break;
    default:                                             return 0;
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
  switch (reason) {
    case MENU_CALLBACK_NAV_UP:    *y -= 2; break;
    case MENU_CALLBACK_NAV_DOWN:  *y += 2; break;
    case MENU_CALLBACK_NAV_LEFT:  *x -= 2; break;
    case MENU_CALLBACK_NAV_RIGHT: *x += 2; break;
    default:                               break;
  }
  return 0;
}

static void save_settings_proc(struct menu_item *item, void *data)
{
  watchlist_store(menu_watchlist);
  settings_save();
}

static void restore_settings_proc(struct menu_item *item, void *data)
{
  settings_load_default();
  int no_font_options = sizeof(menu_font_options) / sizeof(*menu_font_options);
  for (int i = 0; i < no_font_options; ++i)
    if (menu_font_options[i] == settings->menu_font_resource_id) {
      menu_option_set(menu_font_option, i);
      break;
    }
  struct gfx_font *font = resource_get(settings->menu_font_resource_id);
  menu_set_font(&menu_main, font);
  menu_set_cell_width(&menu_main, font->char_width + font->letter_spacing);
  menu_set_cell_height(&menu_main, font->char_height + font->line_spacing);
  gfx_mode_drop_shadow(settings->menu_drop_shadow);
  menu_set_pxoffset(&menu_main, settings->menu_x);
  menu_set_pyoffset(&menu_main, settings->menu_y);
  watchlist_fetch(menu_watchlist);
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
      z64_UnloadRoom(&z64_ctxt, &z64_game.room_index);
      z64_game.room_ptr = NULL;
    }
    else {
      z64_LoadRoom(&z64_ctxt, &z64_game.room_index, new_room_index);
      z64_UnloadRoom(&z64_ctxt, &z64_game.room_index);
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

#if 0
static uint8_t old_exc_handler[MIPS_EVSIZE];
static uint8_t new_exc_handler[4 * 32];

static void sup_exc_proc(struct menu_item *item, void *data)
{
  memcpy(&old_exc_handler, (void*)MIPS_EV_GE, MIPS_EVSIZE);
  uint32_t exc_handler[] =
  {
    MIPS_MFC0(MIPS_K0, MIPS_CP0_CAUSE),
    MIPS_LA(MIPS_K1, MIPS_CAUSE_EXCMASK),
    MIPS_AND(MIPS_K0, MIPS_K0, MIPS_K1),
    MIPS_LA(MIPS_K1, MIPS_MAKE_FIELD(MIPS_CAUSE_EXC, MIPS_EXC_TLBL)),
    MIPS_BNE(MIPS_K0, MIPS_K1, 4 * 7),
    MIPS_NOP,
    MIPS_MFC0(MIPS_K0, MIPS_CP0_SR),
    MIPS_LA(MIPS_K1, ~MIPS_STATUS_EXL),
    MIPS_AND(MIPS_K0, MIPS_K0, MIPS_K1),
    MIPS_MTC0(MIPS_K0, MIPS_CP0_SR),
    MIPS_ERET,
    MIPS_LA(MIPS_K0, &old_exc_handler),
    MIPS_JR(MIPS_K0),
    MIPS_NOP,
  };
  memcpy(&new_exc_handler, &exc_handler, sizeof(exc_handler));
  uint32_t exc_hook[] =
  {
    MIPS_LA(MIPS_K0, &new_exc_handler),
    MIPS_JR(MIPS_K0),
    MIPS_NOP,
  };
  memcpy((void*)MIPS_EV_GE, &exc_hook, sizeof(exc_hook));
}
#endif

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

  gfx_mode_init(G_TF_POINT, G_ON);

  struct gfx_font *font = menu_get_font(&menu_main, 1);
  uint8_t alpha = menu_get_alpha_i(&menu_main, 1);
  int cw = menu_get_cell_width(&menu_main, 1);
  int ch = menu_get_cell_height(&menu_main, 1);

  if (cheats_energy)
    z64_file.energy = z64_file.energy_capacity;
  if (cheats_magic)
    z64_file.magic = (z64_file.magic_capacity + 1) * 0x30;
  if (cheats_sticks) {
    int stick_capacity[] = {1, 10, 20, 30, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_STICK] = stick_capacity[z64_file.stick_upgrade];
  }
  if (cheats_nuts) {
    int nut_capacity[] = {1, 20, 30, 40, 1, 0x7F, 1, 0x7F};
    z64_file.ammo[Z64_SLOT_NUT] = nut_capacity[z64_file.nut_upgrade];
  }
  if (cheats_bombs) {
    int bomb_bag_capacity[] = {1, 20, 30, 40, 1, 1, 1, 1};
    z64_file.ammo[Z64_SLOT_BOMB] = bomb_bag_capacity[z64_file.bomb_bag];
  }
  if (cheats_arrows) {
    int quiver_capacity[] = {1, 30, 40, 50, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_BOW] = quiver_capacity[z64_file.quiver];
  }
  if (cheats_seeds) {
    int bullet_bag_capacity[] = {1, 30, 40, 50, 1, 10, 20, 30};
    z64_file.ammo[Z64_SLOT_SLINGSHOT] =
      bullet_bag_capacity[z64_file.bullet_bag];
  }
  if (cheats_bombchus)
    z64_file.ammo[Z64_SLOT_BOMBCHU] = 50;
  if (cheats_beans)
    z64_file.ammo[Z64_SLOT_BEANS] = 0x01;
  if (cheats_keys) {
    if (z64_game.scene_index >= 0x0000 && z64_game.scene_index <= 0x0010)
      z64_file.dungeon_keys[z64_game.scene_index] = 1;
  }
  if (cheats_rupees) {
    int wallet_capacity[] = {99, 200, 500, 0xFFFF};
    z64_file.rupees = wallet_capacity[z64_file.wallet];
  }
  if (cheats_nayru)
    z64_file.nayrus_love_timer = 0x044B;
  if (cheats_time)
    z64_file.day_time += 0x0100;

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
    else if (l_pad & BUTTON_C_UP)
      clear_csp_proc(NULL, NULL);
    else if ((l_pad & BUTTON_A) && (l_pad & BUTTON_B))
      /* void out */
      z64_link.common.pos_1.y = z64_link.common.pos_2.y = -0x8000;
    else if (l_pad & BUTTON_A)
      /* reload zone */
      z64_game.scene_load_flags = 0x0014;
    else if (l_pad & BUTTON_B) {
      /* go to title screen */
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
    static z64_xyz_t stored_pos[10];
    static z64_rot_t stored_rot[10];
    if (d_pad & BUTTON_D_LEFT) {
      /* save position and orientation */
      stored_pos[tp_slot] = z64_link.common.pos_2;
      stored_rot[tp_slot] = z64_link.common.rot_2;
    }
    else if (d_pad & BUTTON_D_RIGHT) {
      /* teleport */
      z64_link.common.pos_1 = z64_link.common.pos_2 = stored_pos[tp_slot];
      z64_link.common.rot_2 = stored_rot[tp_slot];
      z64_link.target_yaw = stored_rot[tp_slot].y;
    }
    else if (d_pad & pad_pressed & BUTTON_D_DOWN)
      pause_proc(NULL, NULL);
    else if (d_pad & pad_pressed & BUTTON_D_UP)
      advance_proc(NULL, NULL);
  }

  if (settings->input_display_enabled) {
    gfx_mode_color(0xC0, 0xC0, 0xC0, alpha);
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
      gfx_mode_color((buttons[i].color >> 16) & 0xFF,
                     (buttons[i].color >> 8)  & 0xFF,
                     (buttons[i].color >> 0)  & 0xFF,
                     alpha);
      gfx_printf(font,
                 settings->input_display_x + cw * (12 + i),
                 settings->input_display_y,
                 "%s", buttons[i].name);
    }
  }

  {
    static int32_t frame_counter = 0;
    if (settings->lag_counter_enabled) {
      if ((pad_pressed & BUTTON_A) && (pad & BUTTON_R)) {
        frame_counter = 0;
        z64_vi_counter = 0;
      }
      int cw = menu_get_cell_width(&menu_main, 1);
      gfx_mode_color(0xC0, 0xC0, 0xC0, menu_get_alpha_i(&menu_main, 1));
      if (settings->lag_unit == SETTINGS_LAG_FRAMES)
        gfx_printf(menu_get_font(&menu_main, 1),
                   settings->lag_counter_x - cw * 8, settings->lag_counter_y,
                   "%8d", z64_vi_counter - frame_counter);
      else if (settings->lag_unit == SETTINGS_LAG_SECONDS)
        gfx_printf(menu_get_font(&menu_main, 1),
                   settings->lag_counter_x - cw * 8, settings->lag_counter_y,
                   "%8.2f", ((int32_t)z64_vi_counter - frame_counter) / 60.f);
    }
    frame_counter += z64_update_rate;
  }

  if (menu_active)
    menu_draw(&menu_main);

  {
    static int splash_time = 230;
    if (splash_time > 0) {
      --splash_time;
      gfx_mode_color(0xC0, 0x00, 0x00, alpha);
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
  if (warp_info.override_offset) {
    offset = 0;
    warp_info.override_offset = 0;
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
  if ((z64_input_direct.pad & BUTTON_START) || !settings_load())
    settings_load_default();

  /* install entrance offset hook */
  *(uint32_t*)z64_entrance_offset_hook_addr = MIPS_JAL(&entrance_offset_hook);

  /* disable map toggling */
  *(uint32_t*)z64_minimap_disable_1_addr = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x82C);
  *(uint32_t*)z64_minimap_disable_2_addr = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x98);

  /* initialize menus */
  {
    gfx_mode_drop_shadow(settings->menu_drop_shadow);
    struct gfx_font *font = resource_get(settings->menu_font_resource_id);
    menu_init(&menu_main,
              font->char_width + font->letter_spacing,
              font->char_height + font->line_spacing,
              font);
    menu_set_pxoffset(&menu_main, settings->menu_x);
    menu_set_pyoffset(&menu_main, settings->menu_y);
    menu_main.selector = menu_add_button(&menu_main, 0, 0, "return",
                                         main_return_proc, NULL);
    static struct menu menu_inventory;
    menu_add_submenu(&menu_main, 0, 1, &menu_inventory, "inventory");
    static struct menu menu_equips;
    menu_add_submenu(&menu_main, 0, 2, &menu_equips, "equips");
    static struct menu menu_misc;
    menu_add_submenu(&menu_main, 0, 3, &menu_misc, "misc");
    static struct menu menu_cheats;
    menu_add_submenu(&menu_main, 0, 4, &menu_cheats, "cheats");
    static struct menu menu_warps;
    menu_add_submenu(&menu_main, 0, 5, &menu_warps, "warps");
    static struct menu menu_watches;
    menu_add_submenu(&menu_main, 0, 6, &menu_watches, "watches");
    static struct menu menu_settings;
    menu_add_submenu(&menu_main, 0, 7, &menu_settings, "settings");

    menu_init(&menu_inventory, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_inventory.selector = menu_add_submenu(&menu_inventory, 0, 0, NULL,
                                               "return");
    static struct menu menu_equipment;
    menu_add_submenu(&menu_inventory, 0, 1, &menu_equipment, "equipment");
    static struct menu menu_items;
    menu_add_submenu(&menu_inventory, 0, 2, &menu_items, "items");
    static struct menu menu_variable_items;
    menu_add_submenu(&menu_inventory, 0, 3, &menu_variable_items,
                     "variable items");
    static struct menu menu_quest_items;
    menu_add_submenu(&menu_inventory, 0, 4, &menu_quest_items,
                     "quest items");
    static struct menu menu_dungeon_items;
    menu_add_submenu(&menu_inventory, 0, 5, &menu_dungeon_items,
                     "dungeon items");
    static struct menu menu_songs;
    menu_add_submenu(&menu_inventory, 0, 6, &menu_songs, "songs");
    static struct menu menu_rewards;
    menu_add_submenu(&menu_inventory, 0, 7, &menu_rewards, "rewards");

    menu_init(&menu_equipment, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_equipment.selector = menu_add_submenu(&menu_equipment, 0, 0, NULL,
                                               "return");
    for (int i = 0; i < sizeof(equipment_list) / sizeof(*equipment_list); ++i)
      menu_add_switch(&menu_equipment,
                      i / 3 % 2 * 20,
                      1 + i / 6 * 3 + i % 3,
                      equipment_list[i].name,
                      equipment_switch_proc,
                      &equipment_list[i]);

    menu_init(&menu_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_items.selector = menu_add_submenu(&menu_items, 0, 0, NULL, "return");
    for (int i = 0; i < sizeof(item_list) / sizeof(*item_list); ++i)
      menu_add_switch(&menu_items,
                      i % 2 * 20,
                      1 + i / 2,
                      item_list[i].name,
                      item_switch_proc,
                      &item_list[i]);

    menu_init(&menu_variable_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_variable_items.selector = menu_add_submenu(&menu_variable_items,
                                                    0, 0, NULL, "return");
    static int8_t bottle_options[] =
    {
      Z64_ITEM_NULL,          Z64_ITEM_BOTTLE,      Z64_ITEM_RED_POTION,
      Z64_ITEM_GREEN_POTION,  Z64_ITEM_BLUE_POTION, Z64_ITEM_FAIRY,
      Z64_ITEM_FISH,          Z64_ITEM_MILK,        Z64_ITEM_LETTER,
      Z64_ITEM_BLUE_FIRE,     Z64_ITEM_BUG,         Z64_ITEM_BIG_POE,
      Z64_ITEM_HALF_MILK,     Z64_ITEM_POE
    };
    static struct byte_option bottle_option_data[] =
    {
      {&z64_file.items[Z64_SLOT_BOTTLE_1], bottle_options, 14},
      {&z64_file.items[Z64_SLOT_BOTTLE_2], bottle_options, 14},
      {&z64_file.items[Z64_SLOT_BOTTLE_3], bottle_options, 14},
      {&z64_file.items[Z64_SLOT_BOTTLE_4], bottle_options, 14},
    };
    for (int i = 0; i < 4; ++i) {
      char s[41];
      sprintf(s, "bottle %i", i + 1);
      menu_add_static(&menu_variable_items, 0, 1 + i, s, 0xC0C0C0);
      menu_add_option(&menu_variable_items, 18, 1 + i,
                      "nothing\0""bottle\0""red potion\0""green potion\0"
                      "blue potion\0""fairy\0""fish\0""milk\0""ruto's letter\0"
                      "blue fire\0""bug\0""big poe\0""half milk\0""poe\0",
                      byte_option_proc, &bottle_option_data[i]);
    }
    static int8_t adult_trade_options[] =
    {
      Z64_ITEM_NULL,          Z64_ITEM_POCKET_EGG,
      Z64_ITEM_POCKET_CUCCO,  Z64_ITEM_COJIRO,
      Z64_ITEM_ODD_MUSHROOM,  Z64_ITEM_ODD_POTION,
      Z64_ITEM_POACHERS_SAW,  Z64_ITEM_BROKEN_GORONS_SWORD,
      Z64_ITEM_PRESCRIPTION,  Z64_ITEM_EYEBALL_FROG,
      Z64_ITEM_EYE_DROPS,     Z64_ITEM_CLAIM_CHECK,
    };
    static struct byte_option adult_trade_option_data =
    {
      &z64_file.items[Z64_SLOT_ADULT_TRADE], adult_trade_options, 12,
    };
    menu_add_static(&menu_variable_items, 0, 5, "adult trade item",
                    0xC0C0C0);
    menu_add_option(&menu_variable_items, 18, 5,
                    "nothing\0""pocket egg\0""pocket cucco\0""cojiro\0"
                    "odd mushroom\0""odd potion\0""poacher's saw\0"
                    "goron's sword\0""prescription\0""eyeball frog\0"
                    "eye drops\0""claim check\0",
                    byte_option_proc, &adult_trade_option_data);
    static int8_t child_trade_options[] =
    {
      Z64_ITEM_NULL,          Z64_ITEM_WEIRD_EGG,   Z64_ITEM_CHICKEN,
      Z64_ITEM_ZELDAS_LETTER, Z64_ITEM_KEATON_MASK, Z64_ITEM_SKULL_MASK,
      Z64_ITEM_SPOOKY_MASK,   Z64_ITEM_BUNNY_HOOD,  Z64_ITEM_GORON_MASK,
      Z64_ITEM_ZORA_MASK,     Z64_ITEM_GERUDO_MASK, Z64_ITEM_MASK_OF_TRUTH,
      Z64_ITEM_SOLD_OUT,
    };
    static struct byte_option child_trade_option_data =
    {
      &z64_file.items[Z64_SLOT_CHILD_TRADE], child_trade_options, 13,
    };
    menu_add_static(&menu_variable_items, 0, 6, "child trade item",
                    0xC0C0C0);
    menu_add_option(&menu_variable_items, 18, 6,
                    "nothing\0""weird egg\0""chicken\0""zelda's letter\0"
                    "keaton mask\0""skull mask\0""spooky mask\0""bunny hood\0"
                    "goron mask\0""zora mask\0""gerudo mask\0""mask of truth\0"
                    "sold out\0",
                    byte_option_proc, &child_trade_option_data);

    menu_init(&menu_quest_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_quest_items.selector = menu_add_submenu(&menu_quest_items, 0, 0, NULL,
                                                 "return");
    menu_add_switch(&menu_quest_items, 0, 1, "magic",
                    magic_switch_proc, NULL);
    static struct switch_info stone_switch =
    {
      "stone of agony", (char*)&z64_file.quest_items + 1, 0b00100000,
    };
    menu_add_switch(&menu_quest_items, 0, 2, stone_switch.name,
                    equipment_switch_proc, &stone_switch);
    static struct switch_info gerudo_switch =
    {
      "gerudo's card", (char*)&z64_file.quest_items + 1, 0b01000000,
    };
    menu_add_switch(&menu_quest_items, 0, 3, gerudo_switch.name,
                    equipment_switch_proc, &gerudo_switch);
    static struct switch_info gs_switch =
    {
      "gs visible", (char*)&z64_file.quest_items + 1, 0b10000000,
    };
    menu_add_switch(&menu_quest_items, 0, 4, gs_switch.name,
                    equipment_switch_proc, &gs_switch);
    menu_add_static(&menu_quest_items, 0, 5, "energy capacity", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 16, 5, 16, 4,
                      halfword_mod_proc, &z64_file.energy_capacity);
    menu_add_static(&menu_quest_items, 0, 6, "defense hearts", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 16, 6, 16, 2,
                      byte_mod_proc, &z64_file.defense_hearts);
    menu_add_static(&menu_quest_items, 0, 7, "magic capacity", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 16, 7, 16, 2,
                      magic_capacity_proc, NULL);
    menu_add_static(&menu_quest_items, 0, 8, "gs tokens", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 16, 8, 10, 3,
                      halfword_mod_proc, &z64_file.gs_tokens);
    menu_add_static(&menu_quest_items, 0, 9, "heart pieces", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 16, 9, 16, 2,
                      byte_mod_proc, (char*)&z64_file.quest_items + 0);
    static struct option_info bulletbag_option =
    {
      &z64_file.equipment_items, 14, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 10, "bullet bag", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 10,
                    "none\0""bullet bag 30\0""bullet bag 40\0""bullet bag 50\0"
                    "* quiver 30\0""* quiver 40\0""* quiver 50\0"
                    "* bomb bag 20\0",
                    equipment_option_proc, &bulletbag_option);
    static struct option_info quiver_option =
    {
      &z64_file.equipment_items, 0, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 11, "quiver", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 11,
                    "none\0""quiver 30\0""quiver 40\0""quiver 50\0"
                    "* bomb bag 20\0""* bomb bag 30\0""* bomb bag 40\0"
                    "* goron's bracelet\0",
                    equipment_option_proc, &quiver_option);
    static struct option_info bombbag_option =
    {
      &z64_file.equipment_items, 3, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 12, "bomb bag", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 12,
                    "none\0""bomb bag 20\0""bomb bag 30\0""bomb bag 40\0"
                    "* goron's bracelet\0""* silver gaunlets\0"
                    "* golden gaunlets\0""* silver scale\0",
                    equipment_option_proc, &bombbag_option);
    static struct option_info strength_option =
    {
      &z64_file.equipment_items, 6, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 13, "strength", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 13,
                    "none\0""goron's bracelet\0""silver gaunlets\0"
                    "golden gaunlets\0""* silver scale\0""* gold scale\0"
                    "* goron's sword\0""* adult's wallet\0",
                    equipment_option_proc, &strength_option);
    static struct option_info diving_option =
    {
      &z64_file.equipment_items, 9, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 14, "diving", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 14,
                    "none\0""silver scale\0""gold scale\0"
                    "* goron's sword\0""* adult's wallet\0"
                    "* giant's wallet\0""* deku seeds\0""* fishing pole\0",
                    equipment_option_proc, &diving_option);
    static struct option_info wallet_option =
    {
      &z64_file.equipment_items, 12, 0b11,
    };
    menu_add_static(&menu_quest_items, 0, 15, "wallet", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 15,
                    "none\0""adult's wallet\0""giant's wallet\0"
                    "* deku seeds\0",
                    equipment_option_proc, &wallet_option);
    static struct option_info stick_option =
    {
      &z64_file.equipment_items, 17, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 16, "stick capacity", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 16,
                    "0 (1)\0""10\0""20 (1)\0""30 (1)\0""* 0 (2)\0""* 20 (2)\0"
                    "* 30 (2)\0""* 40\0",
                    equipment_option_proc, &stick_option);
    static struct option_info nut_option =
    {
      &z64_file.equipment_items, 20, 0b111,
    };
    menu_add_static(&menu_quest_items, 0, 17, "nut capacity", 0xC0C0C0);
    menu_add_option(&menu_quest_items, 16, 17,
                    "0 (1)\0""20\0""30\0""40\0""* 0 (2)\0""* many (1)\0"
                    "* 0 (3)\0""* many (2)\0",
                    equipment_option_proc, &nut_option);

    menu_init(&menu_dungeon_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_dungeon_items.selector = menu_add_submenu(&menu_dungeon_items, 0, 0,
                                                   NULL, "return");
    static struct dungeon_menu_data dungeon_menu_data =
    {
      {"boss key",  &z64_file.dungeon_items[0], 0b001},
      {"compass",   &z64_file.dungeon_items[0], 0b010},
      {"map",       &z64_file.dungeon_items[0], 0b100},
      &z64_file.dungeon_keys[0],
    };
    menu_add_static(&menu_dungeon_items, 0, 1, "dungeon", 0xC0C0C0);
    menu_add_option(&menu_dungeon_items, 12, 1,
                    "inside the deku tree\0""dodongo's cavern\0"
                    "inside jabu jabu's belly\0""forest temple\0"
                    "fire temple\0""water temple\0""spirit temple\0"
                    "shadow temple\0""bottom of the well\0""ice cavern\0"
                    "ganon's tower\0""gerudo training ground\0"
                    "thieves' hideout\0""inside ganon's castle\0"
                    "ganon's tower collapse\0""inside ganon's castle collapse\0"
                    "treasure box shop\0", dungeon_option_proc,
                    &dungeon_menu_data);
    menu_add_static(&menu_dungeon_items, 0, 2, "small keys", 0xC0C0C0);
    menu_add_intinput(&menu_dungeon_items, 12, 2, 16, 2,
                      dungeon_keys_proc, &dungeon_menu_data);
    menu_add_switch(&menu_dungeon_items, 0, 3,
                    dungeon_menu_data.boss_key_switch_info.name,
                    equipment_switch_proc,
                    &dungeon_menu_data.boss_key_switch_info);
    menu_add_switch(&menu_dungeon_items, 0, 4,
                    dungeon_menu_data.compass_switch_info.name,
                    equipment_switch_proc,
                    &dungeon_menu_data.compass_switch_info);
    menu_add_switch(&menu_dungeon_items, 0, 5,
                    dungeon_menu_data.map_switch_info.name,
                    equipment_switch_proc,
                    &dungeon_menu_data.map_switch_info);

    menu_init(&menu_songs, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_songs.selector = menu_add_submenu(&menu_songs, 0, 0, NULL,
                                           "return");
    for (int i = 0; i < sizeof(song_list) / sizeof(*song_list); ++i)
      menu_add_switch(&menu_songs,
                      i / 6 * 20,
                      1 + i % 6,
                      song_list[i].name,
                      equipment_switch_proc,
                      &song_list[i]);

    menu_init(&menu_rewards, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_rewards.selector = menu_add_submenu(&menu_rewards, 0, 0, NULL,
                                             "return");
    for (int i = 0; i < sizeof(reward_list) / sizeof(*reward_list); ++i)
      menu_add_switch(&menu_rewards,
                      i % 2 * 20,
                      1 + i / 2,
                      reward_list[i].name,
                      equipment_switch_proc,
                      &reward_list[i]);

    menu_init(&menu_equips, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_equips.selector = menu_add_submenu(&menu_equips, 0, 0, NULL,
                                            "return");
    menu_add_static(&menu_equips, 0, 1, "sword", 0xC0C0C0);
    menu_add_option(&menu_equips, 10, 1,
                    "none\0""kokiri\0""master\0""biggoron\0",
                    equip_option_proc, (void*)0);
    menu_add_static(&menu_equips, 0, 2, "shield", 0xC0C0C0);
    menu_add_option(&menu_equips, 10, 2,
                    "none\0""deku\0""hylian\0""mirror\0",
                    equip_option_proc, (void*)1);
    menu_add_static(&menu_equips, 0, 3, "tunic", 0xC0C0C0);
    menu_add_option(&menu_equips, 10, 3,
                    "none\0""kokiri\0""goron\0""zora\0",
                    equip_option_proc, (void*)2);
    menu_add_static(&menu_equips, 0, 4, "boots", 0xC0C0C0);
    menu_add_option(&menu_equips, 10, 4,
                    "none\0""kokiri\0""iron\0""hover\0",
                    equip_option_proc, (void*)3);
    menu_add_static(&menu_equips, 0, 5, "b button", 0xC0C0C0);
    menu_add_intinput(&menu_equips, 10, 5, 16, 2, byte_mod_proc,
                      &z64_file.button_items[0]);
    menu_add_static(&menu_equips, 0, 6, "c left", 0xC0C0C0);
    menu_add_intinput(&menu_equips, 10, 6, 16, 2, byte_mod_proc,
                      &z64_file.button_items[1]);
    menu_add_static(&menu_equips, 0, 7, "c down", 0xC0C0C0);
    menu_add_intinput(&menu_equips, 10, 7, 16, 2, byte_mod_proc,
                      &z64_file.button_items[2]);
    menu_add_static(&menu_equips, 0, 8, "c right", 0xC0C0C0);
    menu_add_intinput(&menu_equips, 10, 8, 16, 2, byte_mod_proc,
                      &z64_file.button_items[3]);
    menu_add_static(&menu_equips, 0, 9, "swordless", 0xC0C0C0);
    menu_add_option(&menu_equips, 10, 9, "no\0""yes\0",
                    swordless_proc, NULL);

    menu_init(&menu_misc, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_misc.selector = menu_add_submenu(&menu_misc, 0, 0, NULL,
                                          "return");
    menu_add_button(&menu_misc, 0, 1, "reset all gs",
                    reset_gs_proc, NULL);
    menu_add_button(&menu_misc, 0, 2, "clear scene flags",
                    clear_flags_proc, NULL);
    menu_add_button(&menu_misc, 0, 3, "set scene flags",
                    set_flags_proc, NULL);
    menu_add_static(&menu_misc, 0, 4, "teleport slot", 0xC0C0C0);
    struct menu_item *tp_slot_display = menu_add_static(&menu_misc,
                                                        16, 4, "0",
                                                        0xA0A0A0);
    menu_add_button(&menu_misc, 14, 4, "-", tp_slot_dec_proc,
                    tp_slot_display);
    menu_add_button(&menu_misc, 18, 4, "+", tp_slot_inc_proc,
                    tp_slot_display);
    static int8_t language_options[] = {0x00, 0x01};
    static struct byte_option language_option_data =
    {
      &z64_file.language, language_options, 2,
    };
    menu_add_static(&menu_misc, 0, 5, "language", 0xC0C0C0);
    menu_add_option(&menu_misc, 14, 5, "japanese\0""english\0",
                    byte_option_proc, &language_option_data);
    static int8_t target_options[] = {0x00, 0x01};
    static struct byte_option target_option_data =
    {
      &z64_file.z_targeting, target_options, 2,
    };
    menu_add_static(&menu_misc, 0, 6, "z targeting", 0xC0C0C0);
    menu_add_option(&menu_misc, 14, 6, "switch\0""hold\0",
                    byte_option_proc, &target_option_data);
#if 0
    menu_add_button(&menu_misc, 0, 7, "suppress exceptions", sup_exc_proc,
                    NULL);
#endif

    menu_init(&menu_cheats, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_cheats.selector = menu_add_submenu(&menu_cheats, 0, 0, NULL,
                                            "return");
    menu_add_switch(&menu_cheats, 0, 1, "energy",
                    generic_switch_proc, &cheats_energy);
    menu_add_switch(&menu_cheats, 0, 2, "magic",
                    generic_switch_proc, &cheats_magic);
    menu_add_switch(&menu_cheats, 0, 3, "deku sticks",
                    generic_switch_proc, &cheats_sticks);
    menu_add_switch(&menu_cheats, 0, 4, "deku nuts",
                    generic_switch_proc, &cheats_nuts);
    menu_add_switch(&menu_cheats, 0, 5, "bombs",
                    generic_switch_proc, &cheats_bombs);
    menu_add_switch(&menu_cheats, 0, 6, "arrows",
                    generic_switch_proc, &cheats_arrows);
    menu_add_switch(&menu_cheats, 0, 7, "deku seeds",
                    generic_switch_proc, &cheats_seeds);
    menu_add_switch(&menu_cheats, 0, 8, "bombchus",
                    generic_switch_proc, &cheats_bombchus);
    menu_add_switch(&menu_cheats, 0, 9, "magic beans",
                    generic_switch_proc, &cheats_beans);
    menu_add_switch(&menu_cheats, 0, 10, "small keys",
                    generic_switch_proc, &cheats_keys);
    menu_add_switch(&menu_cheats, 0, 11, "rupees",
                    generic_switch_proc, &cheats_rupees);
    menu_add_switch(&menu_cheats, 0, 12, "nayru's love",
                    generic_switch_proc, &cheats_nayru);
    menu_add_switch(&menu_cheats, 0, 13, "advance time",
                    generic_switch_proc, &cheats_time);

    menu_init(&menu_warps, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_warps.selector = menu_add_submenu(&menu_warps, 0, 0, NULL,
                                           "return");
    menu_add_static(&menu_warps, 0, 1, "entrance", 0xC0C0C0);
    warp_info.entrance = menu_add_intinput(&menu_warps, 10, 1, 16, 4,
                                           NULL, NULL);
    menu_add_static(&menu_warps, 0, 2, "age", 0xC0C0C0);
    warp_info.age = menu_add_option(&menu_warps, 10, 2,
                                    "adult\0""child\0", NULL, NULL);
    warp_info.override_offset = 0;
    menu_add_button(&menu_warps, 0, 3, "clear cutscene pointer",
                    clear_csp_proc, NULL);
    menu_add_button(&menu_warps, 0, 4, "warp", warp_proc, NULL);
    menu_add_static(&menu_warps, 0, 5, "room", 0xC0C0C0);
    struct menu_item *room_index_input = menu_add_intinput(&menu_warps, 10, 5,
                                                           16, 2, NULL, NULL);
    menu_add_button(&menu_warps, 0, 6, "load room", load_room_proc,
                    room_index_input);
    static struct menu places;
    menu_init(&places, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    places.selector = menu_add_submenu(&places, 0, 0, NULL, "return");
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
    menu_add_submenu(&menu_warps, 0, 7, &places, "places");
    static struct menu explorer;
    explorer_create(&explorer);
    menu_add_submenu(&menu_warps, 0, 8, &explorer, "scene explorer");

    menu_init(&menu_watches, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_watches.selector = menu_add_submenu(&menu_watches, 0, 0, NULL,
                                             "return");
    menu_watchlist = watchlist_create(&menu_watches, 0, 1);
    watchlist_fetch(menu_watchlist);

    menu_init(&menu_settings, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_settings.selector = menu_add_submenu(&menu_settings, 0, 0, NULL,
                                              "return");
    menu_add_static(&menu_settings, 0, 1, "menu", 0xFFFFFF);
    menu_add_static(&menu_settings, 2, 2, "font", 0xC0C0C0);
    menu_font_option = menu_add_option(&menu_settings, 14, 2,
                                       "fipps\0""notalot35\0"
                                       "origami mommy\0""pc senior\0"
                                       "pixel intv\0""press start 2p\0"
                                       "smw text nc\0""werdna's return\0",
                                       menu_font_option_proc, NULL);
    int no_font_options = sizeof(menu_font_options) /
                          sizeof(*menu_font_options);
    for (int i = 0; i < no_font_options; ++i)
      if (menu_font_options[i] == settings->menu_font_resource_id) {
        menu_option_set(menu_font_option, i);
        break;
      }
    menu_add_static(&menu_settings, 2, 3, "drop shadow", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 3,
                      menu_drop_shadow_proc, NULL);
    menu_add_static(&menu_settings, 2, 4, "position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 4, menu_position_proc, NULL);
    menu_add_static(&menu_settings, 0, 5, "input display", 0xFFFFFF);
    menu_add_static(&menu_settings, 2, 6, "enabled", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 6,
                      generic_checkbox_proc, &settings->input_display_enabled);
    menu_add_static(&menu_settings, 2, 7, "position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 7,
                         generic_position_proc, &settings->input_display_x);
    menu_add_static(&menu_settings, 0, 8, "lag counter", 0xFFFFFF);
    menu_add_static(&menu_settings, 2, 9, "enabled", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 9,
                      generic_checkbox_proc, &settings->lag_counter_enabled);
    menu_add_static(&menu_settings, 2, 10, "unit", 0xC0C0C0);
    static int8_t lag_unit_options[] =
    {
      SETTINGS_LAG_FRAMES,
      SETTINGS_LAG_SECONDS,
    };
    static struct byte_option lag_unit_data =
    {
      NULL, lag_unit_options, 2,
    };
    lag_unit_data.data = &settings->lag_unit;
    menu_lag_unit_option = menu_add_option(&menu_settings, 14, 10,
                                           "frames\0""seconds\0",
                                           byte_option_proc, &lag_unit_data);
    menu_add_static(&menu_settings, 2, 11, "position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 11,
                         generic_position_proc, &settings->lag_counter_x);
    menu_add_button(&menu_settings, 0, 12, "save settings",
                    save_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, 13, "restore defaults",
                    restore_settings_proc, NULL);
  }

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
