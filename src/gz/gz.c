#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <startup.h>
#include <mips.h>
#include <n64.h>
#include "input.h"
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
  int         shift;
  uint32_t    mask;
  int         x;
  int         y;
  int         start;
  int         length;
  const char *tooltip;
};

struct equipment_item_data
{
  int       shift;
  uint32_t  mask;
  int       start;
  int8_t    value;
};

struct capacity_item_option
{
  int         shift;
  int         x;
  int         y;
  int         item_tile;
  _Bool       multi_tile;
  int         amount_tiles[8];
  const char *tooltip;
};

struct capacity_item_data
{
  int       shift;
  _Bool     multi_tile;
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
  uint32_t    mask;
  uint32_t    color;
  const char *tooltip;
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

enum cmdact
{
  CMDACT_HOLD,
  CMDACT_PRESS,
  CMDACT_PRESS_ONCE,
};

struct command_info
{
  const char   *name;
  void        (*proc)(void);
  enum cmdact   activation_type;
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

#define                     CPU_COUNTER_FREQ 46875000
static uint8_t              profile = 0;
static struct menu          menu_main;
static struct menu          menu_explorer;
static struct menu          menu_global_watches;
static struct menu_item    *menu_font_option;
static struct menu_item    *menu_watchlist;
static _Bool                menu_active = 0;
static int32_t              frames_queued = -1;
static int32_t              frame_counter = 0;
static int32_t              lag_vi_offset;
static int64_t              cpu_counter = 0;
static _Bool                timer_active = 0;
static int64_t              timer_counter_offset;
static int64_t              timer_counter_prev;
static uint16_t             day_time_prev;
static int                  target_day_time = -1;
static struct memory_file  *memfile;
static _Bool                memfile_saved[SETTINGS_MEMFILE_MAX] = {0};
static uint8_t              memfile_slot = 0;
static _Bool                override_offset = 0;

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
  "freze time",
  "no music",
  "items usable",
  "no minimap",
};

void command_break(void)
{
  if (z64_game.event_flag != -1)
    z64_game.event_flag = 0x0000;
  if (z64_game.cutscene_state != 0x00)
    z64_game.cutscene_state = 0x03;
  if (z64_game.textbox_state_1 != 0x00) {
    z64_game.textbox_state_1 = 0x36;
    z64_game.textbox_state_2 = 0x00;
    z64_game.textbox_state_3 = 0x02;
  }
  if (settings->menu_settings.break_type == SETTINGS_BREAK_AGGRESSIVE) {
    z64_game.camera_mode = 0x0001;
    z64_game.camera_flag_1 = 0x0000;
    z64_link.state_flags_1 = 0x00000000;
    z64_link.state_flags_2 = 0x00000000;
    if (z64_link.action != 0x00)
      z64_link.action = 0x07;
  }
}

void command_levitate(void)
{
  z64_link.common.vel_1.y = 6.34375f;
}

void command_savepos(void)
{
  uint8_t slot = settings->teleport_slot;
  settings->teleport_pos[slot] = z64_link.common.pos_2;
  settings->teleport_rot[slot] = z64_link.common.rot_2.y;
}

void command_loadpos(void)
{
  uint8_t slot = settings->teleport_slot;
  z64_link.common.pos_1 = settings->teleport_pos[slot];
  z64_link.common.pos_2 = settings->teleport_pos[slot];
  z64_link.common.rot_2.y = settings->teleport_rot[slot];
  z64_link.target_yaw = settings->teleport_rot[slot];
}

void command_savememfile(void)
{
  struct memory_file *file = &memfile[memfile_slot];
  memcpy(&file->z_file, &z64_file, sizeof(file->z_file));
  file->scene_index = z64_game.scene_index;
  uint32_t f;
  f = z64_game.chest_flags;
  file->z_file.scene_flags[z64_game.scene_index].chests = f;
  f = z64_game.switch_flags;
  file->z_file.scene_flags[z64_game.scene_index].switches = f;
  f = z64_game.room_clear_flags;
  file->z_file.scene_flags[z64_game.scene_index].rooms_cleared = f;
  f = z64_game.collectible_flags;
  file->z_file.scene_flags[z64_game.scene_index].collectibles = f;
  memcpy(&file->scene_flags, &z64_game.switch_flags,
         sizeof(file->scene_flags));
  memfile_saved[memfile_slot] = 1;
}

void command_loadmemfile(void)
{
  if (!memfile_saved[memfile_slot])
    return;
  struct memory_file *file = &memfile[memfile_slot];
  /* keep some data intact to prevent glitchiness */
  int8_t seq_index = z64_file.seq_index;
  uint8_t minimap_index = z64_file.minimap_index;
  int32_t link_age = z64_file.link_age;
  memcpy(&z64_file, &file->z_file, sizeof(file->z_file));
  z64_game.link_age = z64_file.link_age;
  z64_file.seq_index = seq_index;
  z64_file.minimap_index = minimap_index;
  z64_file.link_age = link_age;
  if (file->scene_index == z64_game.scene_index)
    memcpy(&z64_game.switch_flags, &file->scene_flags,
           sizeof(file->scene_flags));
  else {
    uint32_t f;
    f = z64_file.scene_flags[z64_game.scene_index].chests;
    z64_game.chest_flags = f;
    f = z64_file.scene_flags[z64_game.scene_index].switches;
    z64_game.switch_flags = f;
    f = z64_file.scene_flags[z64_game.scene_index].rooms_cleared;
    z64_game.room_clear_flags = f;
    f = z64_file.scene_flags[z64_game.scene_index].collectibles;
    z64_game.collectible_flags = f;
  }
  for (int i = 0; i < 4; ++i)
    if (z64_file.button_items[i] != Z64_ITEM_NULL)
      z64_UpdateItemButton(&z64_game, i);
  z64_UpdateEquipment(&z64_game, &z64_link);
}

void command_resetlag(void)
{
  frame_counter = 0;
  lag_vi_offset = -(int32_t)z64_vi_counter;
}

void command_timer(void)
{
  timer_active = !timer_active;
}

void command_resettimer(void)
{
  timer_counter_offset = -cpu_counter;
  timer_counter_prev = cpu_counter;
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

void command_pause(void)
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

void command_advance(void)
{
  if (frames_queued >= 0)
    ++frames_queued;
  else
    command_pause();
}

void command_fileselect(void)
{
  zu_execute_filemenu();
}

static void do_warp(int16_t entrance_index, uint16_t cutscene_index)
{
  override_offset = 1;
  zu_execute_game(entrance_index, cutscene_index);
}

void command_reload(void)
{
  do_warp(z64_file.entrance_index, 0x0000);
}

void command_void(void)
{
  zu_void();
}

void command_turbo(void)
{
  z64_link.linear_vel = 27.f;
}

void command_fall(void)
{
  z64_link.common.pos_1.y = -32768.f;
}

void command_age(void)
{
  z64_game.link_age = !z64_game.link_age;
}

void command_starttimer(void)
{
  if (!timer_active)
    command_timer();
}

void command_stoptimer(void)
{
  if (timer_active)
    command_timer();
}

void command_prevpos(void)
{
  settings->teleport_slot = (settings->teleport_slot + SETTINGS_TELEPORT_MAX -
                             1) % SETTINGS_TELEPORT_MAX;
}

void command_nextpos(void)
{
  settings->teleport_slot = (settings->teleport_slot +
                             1) % SETTINGS_TELEPORT_MAX;
}

void command_prevfile(void)
{
  memfile_slot = (memfile_slot + SETTINGS_MEMFILE_MAX -
                  1) % SETTINGS_MEMFILE_MAX;
}

void command_nextfile(void)
{
  memfile_slot = (memfile_slot + 1) % SETTINGS_MEMFILE_MAX;
}

static struct command_info command_info[] =
{
  {"show/hide menu",    NULL,                 CMDACT_PRESS_ONCE},
  {"return from menu",  NULL,                 CMDACT_PRESS_ONCE},
  {"break free",        command_break,        CMDACT_HOLD},
  {"levitate",          command_levitate,     CMDACT_HOLD},
  {"save position",     command_savepos,      CMDACT_HOLD},
  {"load position",     command_loadpos,      CMDACT_HOLD},
  {"save memfile",      command_savememfile,  CMDACT_PRESS_ONCE},
  {"load memfile",      command_loadmemfile,  CMDACT_PRESS_ONCE},
  {"reset lag counter", command_resetlag,     CMDACT_HOLD},
  {"start/stop timer",  command_timer,        CMDACT_PRESS_ONCE},
  {"reset timer",       command_resettimer,   CMDACT_HOLD},
  {"pause/unpause",     command_pause,        CMDACT_PRESS_ONCE},
  {"frame advance",     command_advance,      CMDACT_PRESS},
  {"file select",       command_fileselect,   CMDACT_PRESS_ONCE},
  {"reload scene",      command_reload,       CMDACT_PRESS_ONCE},
  {"void out",          command_void,         CMDACT_PRESS_ONCE},
  {"turbo",             command_turbo,        CMDACT_HOLD},
  {"fall",              command_fall,         CMDACT_HOLD},
  {"toggle age",        command_age,          CMDACT_PRESS_ONCE},
  {"start timer",       command_starttimer,   CMDACT_PRESS_ONCE},
  {"stop timer",        command_stoptimer,    CMDACT_PRESS_ONCE},
  {"previous position", command_prevpos,      CMDACT_PRESS_ONCE},
  {"next position",     command_nextpos,      CMDACT_PRESS_ONCE},
  {"previous memfile",  command_prevfile,     CMDACT_PRESS_ONCE},
  {"next memfile",      command_nextfile,     CMDACT_PRESS_ONCE},
  {"explore prev room", NULL,                 CMDACT_PRESS},
  {"explore next room", NULL,                 CMDACT_PRESS},
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

static void update_cpu_counter()
{
  static uint32_t count = 0;
  uint32_t new_count;
  __asm__ volatile ("mfc0 $t0, $9\n"
                    "nop\n"
                    "sw $t0, %0" : "=m"(new_count) :: "t0");
  cpu_counter += new_count - count;
  count = new_count;
}

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

static int input_display_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.input_display = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.input_display = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.input_display);
  return 0;
}

static int pause_display_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.pause_display = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.pause_display = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.pause_display);
  return 0;
}

static int lag_counter_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.lag_counter = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.lag_counter = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.lag_counter);
  return 0;
}

static int timer_proc(struct menu_item *item,
                      enum menu_callback_reason reason,
                      void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.timer = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.timer = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.timer);
  return 0;
}

static void show_menu(void)
{
  menu_signal_enter(&menu_main);
  menu_active = 1;
  input_reserve(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
                BUTTON_L);
  input_reservation_set(1);
}

static void hide_menu(void)
{
  menu_signal_leave(&menu_main);
  menu_active = 0;
  input_free(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
             BUTTON_L);
  input_reservation_set(0);
}

static void main_return_proc(struct menu_item *item, void *data)
{
  hide_menu();
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
      int v = Z64_ITEM_KOKIRI_SWORD + equip_value - 1;
      if (v == Z64_ITEM_BIGGORON_SWORD &&
          !z64_file.bgs_flag && z64_file.broken_giants_knife)
        v = Z64_ITEM_BROKEN_GIANTS_KNIFE;
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

static int halfword_optionmod_proc(struct menu_item *item,
                                   enum menu_callback_reason reason,
                                   void *data)
{
  uint16_t *v = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != *v)
      menu_option_set(item, *v);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *v = menu_option_get(item);
  return 0;
}

static int age_option_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.warp_age)
      menu_option_set(item, settings->menu_settings.warp_age);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.warp_age = menu_option_get(item);
  return 0;
}

static int lag_unit_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.lag_unit)
      menu_option_set(item, settings->menu_settings.lag_unit);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.lag_unit = menu_option_get(item);
  return 0;
}

static int break_type_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.break_type)
      menu_option_set(item, settings->menu_settings.break_type);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.break_type = menu_option_get(item);
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

static void restore_gs_proc(struct menu_item *item, void *data)
{
  memset(&z64_file.gs_flags, 0x00, sizeof(z64_file.gs_flags));
}

static void call_navi_proc(struct menu_item *item, void *data)
{
  z64_file.navi_timer = 0x025A;
}

static void set_time_proc(struct menu_item *item, void *data)
{
  target_day_time = (uint32_t)data;
}

static void set_carpenter_flags_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0x90);
  zu_set_event_flag(0x91);
  zu_set_event_flag(0x92);
  zu_set_event_flag(0x93);
}

static void clear_carpenter_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0x90);
  zu_clear_event_flag(0x91);
  zu_clear_event_flag(0x92);
  zu_clear_event_flag(0x93);
}

static void set_intro_flags_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0xA0);
  zu_set_event_flag(0xA1);
  zu_set_event_flag(0xA3);
  zu_set_event_flag(0xA4);
  zu_set_event_flag(0xA5);
  zu_set_event_flag(0xA6);
  zu_set_event_flag(0xA7);
  zu_set_event_flag(0xA8);
  zu_set_event_flag(0xB1);
  zu_set_event_flag(0xB2);
  zu_set_event_flag(0xB3);
  zu_set_event_flag(0xB4);
  zu_set_event_flag(0xB5);
  zu_set_event_flag(0xB6);
  zu_set_event_flag(0xB7);
  zu_set_event_flag(0xB8);
  zu_set_event_flag(0xB9);
  zu_set_event_flag(0xBA);
  zu_set_event_flag(0xC0);
  zu_set_event_flag(0xC7);
}

static void clear_intro_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0xA0);
  zu_clear_event_flag(0xA1);
  zu_clear_event_flag(0xA3);
  zu_clear_event_flag(0xA4);
  zu_clear_event_flag(0xA5);
  zu_clear_event_flag(0xA6);
  zu_clear_event_flag(0xA7);
  zu_clear_event_flag(0xA8);
  zu_clear_event_flag(0xB1);
  zu_clear_event_flag(0xB2);
  zu_clear_event_flag(0xB3);
  zu_clear_event_flag(0xB4);
  zu_clear_event_flag(0xB5);
  zu_clear_event_flag(0xB6);
  zu_clear_event_flag(0xB7);
  zu_clear_event_flag(0xB8);
  zu_clear_event_flag(0xB9);
  zu_clear_event_flag(0xBA);
  zu_clear_event_flag(0xC0);
  zu_clear_event_flag(0xC7);
}

static void set_reward_flags_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0x19);
  zu_set_event_flag(0x25);
  zu_set_event_flag(0x37);
  zu_set_event_flag(0x48);
  zu_set_event_flag(0x49);
  zu_set_event_flag(0x4A);
  zu_set_event_flag(0xC8);
}

static void clear_reward_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0x19);
  zu_clear_event_flag(0x25);
  zu_clear_event_flag(0x37);
  zu_clear_event_flag(0x48);
  zu_clear_event_flag(0x49);
  zu_clear_event_flag(0x4A);
  zu_clear_event_flag(0xC8);
}

static void clear_scene_flags_proc(struct menu_item *item, void *data)
{
  memset(&z64_game.switch_flags, 0x00, 0x24);
}

static void set_scene_flags_proc(struct menu_item *item, void *data)
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
  z64_game.link_age = settings->menu_settings.warp_age;
  uint16_t cutscene = settings->menu_settings.warp_cutscene;
  if (cutscene > 0x0000)
    cutscene += 0xFFEF;
  do_warp(settings->warp_entrance, cutscene);
}

static int cutscene_option_proc(struct menu_item *item,
                                enum menu_callback_reason reason,
                                void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.warp_cutscene)
      menu_option_set(item, settings->menu_settings.warp_cutscene);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.warp_cutscene = menu_option_get(item);
  return 0;
}

static void places_proc(struct menu_item *item, void *data)
{
  uintptr_t d = (uintptr_t)data;
  int scene_index = (d >> 8) & 0x00FF;
  int entrance_index = (d >> 0) & 0x00FF;
  for (int i = 0; i < Z64_ETAB_LENGTH; ++i) {
    z64_entrance_table_t *e = &z64_entrance_table[i];
    if (e->scene_index == scene_index && e->entrance_index == entrance_index) {
      z64_game.link_age = settings->menu_settings.warp_age;
      uint16_t cutscene = settings->menu_settings.warp_cutscene;
      if (cutscene > 0x0000)
        cutscene += 0xFFEF;
      do_warp(i, cutscene);
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
    int font_resource = menu_font_options[menu_option_get(item)];
    settings->menu_settings.font_resource = font_resource;
    struct gfx_font *font = resource_get(font_resource);
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
    settings->menu_settings.drop_shadow = menu_checkbox_get(item);
    gfx_mode_set(GFX_MODE_DROPSHADOW, settings->menu_settings.drop_shadow);
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.drop_shadow);
  return 0;
}

static int menu_position_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  int dist = 2;
  if (input_pad() & BUTTON_Z)
    dist *= 2;
  switch (reason) {
    case MENU_CALLBACK_ACTIVATE:    input_reserve(BUTTON_Z);  break;
    case MENU_CALLBACK_DEACTIVATE:  input_free(BUTTON_Z);     break;
    case MENU_CALLBACK_NAV_UP:      settings->menu_y -= dist; break;
    case MENU_CALLBACK_NAV_DOWN:    settings->menu_y += dist; break;
    case MENU_CALLBACK_NAV_LEFT:    settings->menu_x -= dist; break;
    case MENU_CALLBACK_NAV_RIGHT:   settings->menu_x += dist; break;
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
  if (input_pad() & BUTTON_Z)
    dist *= 2;
  switch (reason) {
    case MENU_CALLBACK_ACTIVATE:    input_reserve(BUTTON_Z);  break;
    case MENU_CALLBACK_DEACTIVATE:  input_free(BUTTON_Z);     break;
    case MENU_CALLBACK_NAV_UP:      *y -= dist;               break;
    case MENU_CALLBACK_NAV_DOWN:    *y += dist;               break;
    case MENU_CALLBACK_NAV_LEFT:    *x -= dist;               break;
    case MENU_CALLBACK_NAV_RIGHT:   *x += dist;               break;
    default:
      break;
  }
  return 0;
}

static void activate_command_proc(struct menu_item *item, void *data)
{
  int command_index = (int)data;
  if (command_info[command_index].proc)
    command_info[command_index].proc();
}

static void apply_settings()
{
  size_t no_font_options = sizeof(menu_font_options) /
                           sizeof(*menu_font_options);
  for (int i = 0; i < no_font_options; ++i)
    if (menu_font_options[i] == settings->menu_settings.font_resource) {
      menu_option_set(menu_font_option, i);
      break;
    }
  struct gfx_font *font = resource_get(settings->menu_settings.font_resource);
  menu_set_font(&menu_main, font);
  menu_set_cell_width(&menu_main, font->char_width + font->letter_spacing);
  menu_set_cell_height(&menu_main, font->char_height + font->line_spacing);
  gfx_mode_set(GFX_MODE_DROPSHADOW, settings->menu_settings.drop_shadow);
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

static int warp_info_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{
  gfx_mode_set(GFX_MODE_COLOR, (draw_params->color << 8) | draw_params->alpha);
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;
  if (z64_game.link_age == 0)
    gfx_printf(font, x, y + ch * 0, "current age      adult");
  else
    gfx_printf(font, x, y + ch * 0, "current age      child");
  gfx_printf(font, x, y + ch * 1,
             "current entrance %04" PRIx16, z64_file.entrance_index);
  if (z64_file.cutscene_index >= 0xFFF0 && z64_file.cutscene_index <= 0xFFFF)
    gfx_printf(font, x, y + ch * 2, "current cutscene %" PRIu16,
               z64_file.cutscene_index - 0xFFEF);
  else
    gfx_printf(font, x, y + ch * 2, "current cutscene none");
  return 1;
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

static void tab_prev_proc(struct menu_item *item, void *data)
{
  menu_tab_previous(data);
}

static void tab_next_proc(struct menu_item *item, void *data)
{
  menu_tab_next(data);
}

void main_hook()
{
  update_cpu_counter();
  input_update();
  gfx_mode_init();

  {
    /* emergency settings reset */
    uint16_t pad_pressed = input_pressed();
    if (pad_pressed) {
      static const uint16_t input_list[] =
      {
        BUTTON_D_UP,
        BUTTON_D_UP,
        BUTTON_D_DOWN,
        BUTTON_D_DOWN,
        BUTTON_D_LEFT,
        BUTTON_D_RIGHT,
        BUTTON_D_LEFT,
        BUTTON_D_RIGHT,
        BUTTON_B,
        BUTTON_A,
      };
      static int input_pos = 0;
      size_t input_list_length = sizeof(input_list) / sizeof(*input_list);
      if (pad_pressed == input_list[input_pos]) {
        ++input_pos;
        if (input_pos == input_list_length) {
          input_pos = 0;
          settings_load_default();
          apply_settings();
        }
      }
      else
        input_pos = 0;
    }
  }

  if (menu_active) {
    if (input_bind_pressed_raw(COMMAND_MENU))
      hide_menu();
    else if (input_bind_pressed(COMMAND_RETURN))
      menu_return(&menu_main);
    else {
      uint16_t pad_pressed = input_pressed();
      if (pad_pressed & BUTTON_D_UP)
        menu_navigate(&menu_main, MENU_NAVIGATE_UP);
      if (pad_pressed & BUTTON_D_DOWN)
        menu_navigate(&menu_main, MENU_NAVIGATE_DOWN);
      if (pad_pressed & BUTTON_D_LEFT)
        menu_navigate(&menu_main, MENU_NAVIGATE_LEFT);
      if (pad_pressed & BUTTON_D_RIGHT)
        menu_navigate(&menu_main, MENU_NAVIGATE_RIGHT);
      if (pad_pressed & BUTTON_L)
        menu_activate(&menu_main);
    }
  }
  else if (input_bind_pressed_raw(COMMAND_MENU))
    show_menu();

  if (settings->cheats & (1 << CHEAT_ENERGY))
    z64_file.energy = z64_file.energy_capacity;
  if (settings->cheats & (1 << CHEAT_MAGIC))
    z64_file.magic = (z64_file.magic_capacity + 1) * 0x30;
  if (settings->cheats & (1 << CHEAT_STICKS)) {
    int stick_capacity[] = {1, 10, 20, 30, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_STICK] = stick_capacity[z64_file.stick_upgrade];
  }
  if (settings->cheats & (1 << CHEAT_NUTS)) {
    int nut_capacity[] = {1, 20, 30, 40, 1, 0x7F, 1, 0x7F};
    z64_file.ammo[Z64_SLOT_NUT] = nut_capacity[z64_file.nut_upgrade];
  }
  if (settings->cheats & (1 << CHEAT_BOMBS)) {
    int bomb_bag_capacity[] = {1, 20, 30, 40, 1, 1, 1, 1};
    z64_file.ammo[Z64_SLOT_BOMB] = bomb_bag_capacity[z64_file.bomb_bag];
  }
  if (settings->cheats & (1 << CHEAT_ARROWS)) {
    int quiver_capacity[] = {1, 30, 40, 50, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_BOW] = quiver_capacity[z64_file.quiver];
  }
  if (settings->cheats & (1 << CHEAT_SEEDS)) {
    int bullet_bag_capacity[] = {1, 30, 40, 50, 1, 10, 20, 30};
    z64_file.ammo[Z64_SLOT_SLINGSHOT] =
      bullet_bag_capacity[z64_file.bullet_bag];
  }
  if (settings->cheats & (1 << CHEAT_BOMBCHUS))
    z64_file.ammo[Z64_SLOT_BOMBCHU] = 50;
  if (settings->cheats & (1 << CHEAT_BEANS))
    z64_file.ammo[Z64_SLOT_BEANS] = 1;
  if (settings->cheats & (1 << CHEAT_KEYS)) {
    if (z64_game.scene_index >= 0x0000 && z64_game.scene_index <= 0x0010)
      z64_file.dungeon_keys[z64_game.scene_index] = 1;
  }
  if (settings->cheats & (1 << CHEAT_RUPEES)) {
    int wallet_capacity[] = {99, 200, 500, 0xFFFF};
    z64_file.rupees = wallet_capacity[z64_file.wallet];
  }
  if (settings->cheats & (1 << CHEAT_NL))
    z64_file.nayrus_love_timer = 0x044B;
  if (settings->cheats & (1 << CHEAT_FREEZETIME)) {
    if (target_day_time == -1 && z64_day_speed < 0x0190 &&
        (z64_file.day_time == (uint16_t)(day_time_prev + z64_day_speed) ||
         z64_file.day_time == (uint16_t)(day_time_prev + z64_day_speed * 2)))
      z64_file.day_time = day_time_prev;
  }
  if (settings->cheats & (1 << CHEAT_NOMUSIC)) {
    zu_setmusic(0x100000FF);
    zu_setmusic(0x110000FF);
    zu_setmusic(0x130000FF);
    z64_file.seq_index = -1;
  }
  if (settings->cheats & (1 << CHEAT_USEITEMS))
    memset(&z64_game.restriction_flags, 0, sizeof(z64_game.restriction_flags));
  if (settings->cheats & (1 << CHEAT_NOMAP))
    z64_gameinfo.minimap_disabled = 1;

  for (int i = 0; i < COMMAND_MAX; ++i) {
    _Bool active = 0;
    switch (command_info[i].activation_type) {
      case CMDACT_HOLD:       active = input_bind_held(i);        break;
      case CMDACT_PRESS:      active = input_bind_pressed(i);     break;
      case CMDACT_PRESS_ONCE: active = input_bind_pressed_raw(i); break;
    }
    if (command_info[i].proc && active)
      command_info[i].proc();
  }
  if (input_bind_pressed(COMMAND_PREVROOM))
    explorer_room_prev(&menu_explorer);
  if (input_bind_pressed(COMMAND_NEXTROOM))
    explorer_room_next(&menu_explorer);

  while (menu_active && menu_think(&menu_main))
    ;
  while (menu_think(&menu_global_watches))
    ;

  /* update daytime after menu processing to avoid desync */
  if (target_day_time != -1) {
    const uint16_t speed = 0x0800;
    if (z64_file.day_time < target_day_time &&
        target_day_time - z64_file.day_time <= speed)
    {
      z64_file.day_time = target_day_time;
      target_day_time = -1;
    }
    else
      z64_file.day_time += speed;
  }
  day_time_prev = z64_file.day_time;

  struct gfx_font *font = menu_get_font(&menu_main, 1);
  uint8_t alpha = menu_get_alpha_i(&menu_main, 1);
  int cw = menu_get_cell_width(&menu_main, 1);
  int ch = menu_get_cell_height(&menu_main, 1);

  if (settings->menu_settings.pause_display && frames_queued != -1) {
    struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
    struct gfx_sprite sprite =
    {
      t, frames_queued == 0 ? 0 : 1,
      32, 32, 1.f, 1.f,
    };
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    gfx_sprite_draw(&sprite);
  }

  if (settings->menu_settings.input_display) {
    struct gfx_texture *texture = resource_get(RES_ICON_BUTTONS);
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    gfx_printf(font, settings->input_display_x, settings->input_display_y,
               "%4i %4i", z64_input_direct.x, z64_input_direct.y);
    static const int buttons[] =
    {
      15, 14, 12, 3, 2, 1, 0, 13, 5, 4, 11, 10, 9, 8,
    };
    for (int i = 0; i < sizeof(buttons) / sizeof(*buttons); ++i) {
      int b = buttons[i];
      if (!(z64_input_direct.pad & (1 << b)))
        continue;
      int x = (cw - texture->tile_width) / 2 + i * 10;
      int y = -(gfx_font_xheight(font) + texture->tile_height + 1) / 2;
      struct gfx_sprite sprite =
      {
        texture, b,
        settings->input_display_x + cw * 10 + x, settings->input_display_y + y,
        1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, (input_button_color[b] << 8) | alpha);
      gfx_sprite_draw(&sprite);
    }
  }

  if (settings->menu_settings.lag_counter) {
    int32_t lag_frames = (int32_t)z64_vi_counter +
                         lag_vi_offset - frame_counter;
    int x = settings->lag_counter_x - cw * 8;
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    if (settings->menu_settings.lag_unit == SETTINGS_LAG_FRAMES)
      gfx_printf(font, x , settings->lag_counter_y, "%8d", lag_frames);
    else if (settings->menu_settings.lag_unit == SETTINGS_LAG_SECONDS)
      gfx_printf(font, x, settings->lag_counter_y, "%8.2f", lag_frames / 60.f);
  }
  frame_counter += z64_gameinfo.update_rate;

  if (!timer_active)
    timer_counter_offset -= cpu_counter - timer_counter_prev;
  timer_counter_prev = cpu_counter;
  if (settings->menu_settings.timer) {
    int64_t count = cpu_counter + timer_counter_offset;
    int tenths = count * 10 / CPU_COUNTER_FREQ;
    int seconds = tenths / 10;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    tenths %= 10;
    seconds %= 60;
    minutes %= 60;
    int x = settings->timer_x;
    int y = settings->timer_y;
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    if (hours > 0)
      gfx_printf(font, x, y, "%d:%02d:%02d.%d",
                 hours, minutes, seconds, tenths);
    else if (minutes > 0)
      gfx_printf(font, x, y, "%d:%02d.%d", minutes, seconds, tenths);
    else
      gfx_printf(font, x, y, "%d.%d", seconds, tenths);
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
                 "gz-0.3.0 github.com/glankk/gz");
      static struct gfx_texture *logo_texture = NULL;
      if (!logo_texture)
        logo_texture = resource_load_grc_texture("logo");
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, alpha));
      for (int i = 0; i < logo_texture->tiles_y; ++i) {
        struct gfx_sprite logo_sprite =
        {
          logo_texture, i,
          Z64_SCREEN_WIDTH - 12 - logo_texture->tile_width,
          Z64_SCREEN_HEIGHT - 6 - ch * 2 -
          (logo_texture->tiles_y - i) * logo_texture->tile_height,
          1.f, 1.f,
        };
        gfx_sprite_draw(&logo_sprite);
      }
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

  /* initialize variables */
  update_cpu_counter();
  lag_vi_offset = -(int32_t)z64_vi_counter;
  timer_counter_offset = -cpu_counter;
  timer_counter_prev = cpu_counter;
  day_time_prev = z64_file.day_time;

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
    menu_add_static(&menu_warps, 0, 2, "age", 0xC0C0C0);
    menu_add_option(&menu_warps, 9, 2, "adult\0""child\0",
                    age_option_proc, NULL);
    menu_add_static(&menu_warps, 0, 3, "entrance", 0xC0C0C0);
    menu_add_intinput(&menu_warps, 9, 3, 16, 4,
                      halfword_mod_proc, &settings->warp_entrance);
    menu_add_static(&menu_warps, 0, 4, "cutscene", 0xC0C0C0);
    menu_add_option(&menu_warps, 9, 4,
                    "none\0""1\0""2\0""3\0""4\0""5\0""6\0""7\0""8\0"
                    "9\0""10\0""11\0""12\0""13\0""14\0""15\0""16\0",
                    cutscene_option_proc, NULL);
    menu_add_button(&menu_warps, 0, 5, "warp", warp_proc, NULL);
    menu_add_button(&menu_warps, 0, 6, "clear cs pointer",
                    clear_csp_proc, NULL);
    {
      struct menu_item *item = menu_item_add(&menu_warps, 0, 7,
                                             NULL, 0xC0C0C0);
      item->selectable = 0;
      item->draw_proc = warp_info_draw_proc;
    }

    /* scene */
    menu_init(&menu_scene, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_scene.selector = menu_add_submenu(&menu_scene, 0, 0, NULL,
                                           "return");
    explorer_create(&menu_explorer);
    menu_add_submenu(&menu_scene, 0, 1, &menu_explorer, "explorer");
    menu_add_button(&menu_scene, 0, 2, "clear flags",
                    clear_scene_flags_proc, NULL);
    menu_add_button(&menu_scene, 0, 3, "set flags",
                    set_scene_flags_proc, NULL);
    menu_add_static(&menu_scene, 0, 4, "room", 0xC0C0C0);
    {
      struct menu_item *item;
      item = menu_add_intinput(&menu_scene, 5, 4, 10, 2, NULL, NULL);
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
        menu_add_checkbox(&menu_cheats, 14, 1 + i, cheat_proc, (void*)i);
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
    static struct menu menu_amounts;
    menu_add_submenu(&menu_inventory, 0, 3, &menu_amounts,
                     "amounts");

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
        item->pxoffset = option->x * 18;
        item->pyoffset = option->y * 18;
        item->tooltip = option->tooltip;
      }
      size_t capacity_item_list_length = sizeof(capacity_item_list) /
                                         sizeof(*capacity_item_list);
      for (int i = 0; i < capacity_item_list_length; ++i) {
        struct capacity_item_option *option = &capacity_item_list[i];
        int no_tiles = option->multi_tile ? 7 : 8;
        struct gfx_texture *t = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                                   32, 32, 1, no_tiles);
        for (int j = 0; j < no_tiles; ++j) {
          if (option->multi_tile)
            gfx_texture_copy_tile(t, j, t_on, option->item_tile + j, 0);
          else
            gfx_texture_copy_tile(t, j, t_on, option->item_tile, 0);
          gfx_texture_copy_tile(t, j, t_am,
                                option->amount_tiles[8 - no_tiles + j], 1);
        }
        struct capacity_item_data *data = malloc(sizeof(*data));
        data->shift = option->shift;
        data->multi_tile = option->multi_tile;
        static int8_t options[] = {0, 1, 2, 3, 4, 5, 6, 7};
        static int8_t multi_options[] = {Z64_ITEM_NULL, 0, 1, 2, 3, 4, 5, 6};
        struct menu_item *item;
        item = item_option_create(&menu_equipment_items, 0, 2, 8,
                                  option->multi_tile ? multi_options : options,
                                  &data->value, t, t_ab, 1, NULL, 0,
                                  0xFFFFFF, .5f, .5625f, 0.f,
                                  capacity_item_proc, data);
        item->pxoffset = option->x * 18;
        item->pyoffset = option->y * 18;
        item->tooltip = option->tooltip;
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
          char *tooltip = malloc(9);
          sprintf(tooltip, "bottle %d", i + 1);
          item->tooltip = tooltip;
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
        item->tooltip = "adult trade item";
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
        item->tooltip = "child trade item";
      }
      struct menu_item *item;
      item = menu_add_tooltip(&menu_equipment_items, 0, 2, &menu_main,
                              0xC0C0C0);
      item->pxoffset = 76;
      item->pyoffset = 90;
    }
    menu_init(&menu_quest_items, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_quest_items.selector = menu_add_submenu(&menu_quest_items, 0, 0, NULL,
                                                 "return");
    menu_add_static(&menu_quest_items, 0, 1, "energy cap.", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 1, 16, 4,
                      halfword_mod_proc, &z64_file.energy_capacity);
    menu_add_static(&menu_quest_items, 0, 2, "defense", 0xC0C0C0);
    menu_add_intinput(&menu_quest_items, 14, 2, 16, 2,
                      byte_mod_proc, &z64_file.defense_hearts);
    menu_add_static(&menu_quest_items, 0, 3, "magic", 0xC0C0C0);
    menu_add_checkbox(&menu_quest_items, 14, 3, magic_acquired_proc, NULL);
    menu_add_static(&menu_quest_items, 0, 4, "magic cap.", 0xC0C0C0);
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
        item->tooltip = d->tooltip;
      }
      struct menu_item *tooltip = menu_add_tooltip(&menu_quest_items, 0, 10,
                                                   &menu_main, 0xC0C0C0);
      tooltip->pyoffset = 4 * 18;
    }
    menu_init(&menu_amounts, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_amounts.selector = menu_add_submenu(&menu_amounts, 0, 0, NULL,
                                             "return");
    {
      struct gfx_texture *t_item = resource_get(RES_ZICON_ITEM);
      struct gfx_texture *t_item_24 = resource_get(RES_ZICON_ITEM_24);
      struct gfx_texture *t_rupee = resource_get(RES_ZICON_RUPEE);
      menu_add_static_icon(&menu_amounts, 0, 2, t_item, Z64_ITEM_STICK,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 2, 2, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_STICK]);
      menu_add_static_icon(&menu_amounts, 5, 2, t_item, Z64_ITEM_NUT,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 7, 2, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_NUT]);
      menu_add_static_icon(&menu_amounts, 10, 2, t_item, Z64_ITEM_BOMB,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 12, 2, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_BOMB]);
      menu_add_static_icon(&menu_amounts, 0, 4, t_item, Z64_ITEM_BOW,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 2, 4, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_BOW]);
      menu_add_static_icon(&menu_amounts, 5, 4, t_item, Z64_ITEM_SLINGSHOT,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 7, 4, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_SLINGSHOT]);
      menu_add_static_icon(&menu_amounts, 10, 4, t_item, Z64_ITEM_BOMBCHU,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 12, 4, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_BOMBCHU]);
      menu_add_static_icon(&menu_amounts, 0, 6, t_item, Z64_ITEM_BEANS,
                           0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 2, 6, 10, 2,
                        byte_mod_proc, &z64_file.ammo[Z64_SLOT_BEANS]);
      menu_add_static_icon(&menu_amounts, 5, 6, t_item_24, 19,
                           0xFFFFFF, 2.f / 3.f);
      menu_add_intinput(&menu_amounts, 7, 6, 16, 2,
                        byte_mod_proc, &z64_file.magic);
      menu_add_static_icon(&menu_amounts, 10, 6, t_item_24, 12,
                           0xFFFFFF, 2.f / 3.f);
      menu_add_intinput(&menu_amounts, 12, 6, 16, 4,
                        halfword_mod_proc, &z64_file.energy);
      menu_add_static_icon(&menu_amounts, 0, 8,
                           t_item, Z64_ITEM_BIGGORON_SWORD, 0xFFFFFF, .5f);
      menu_add_intinput(&menu_amounts, 2, 8, 16, 4,
                        halfword_mod_proc, &z64_file.bgs_hits_left);
      menu_add_static_icon(&menu_amounts, 10, 8, t_rupee, 0,
                           0xC8FF64, 1.f);
      menu_add_intinput(&menu_amounts, 12, 8, 10, 5,
                        halfword_mod_proc, &z64_file.rupees);
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
                                input_button_color[14], .5625f, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_B);
      item->pxoffset = 0;
      item->pyoffset = 74;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_CL],
                                NULL, t_ab, 0, t_ab, 2,
                                input_button_color[1], .5f, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_CL);
      item->pxoffset = 20;
      item->pyoffset = 74;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_CD],
                                NULL, t_ab, 0, t_ab, 3,
                                input_button_color[2], .5f, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_CD);
      item->pxoffset = 34;
      item->pyoffset = 86;
      item = item_option_create(&menu_equips, 0, 2, 91, NULL,
                                &z64_file.button_items[Z64_ITEMBTN_CR],
                                NULL, t_ab, 0, t_ab, 4,
                                input_button_color[0], .5, 0.f, 0.f,
                                button_item_proc, (void*)Z64_ITEMBTN_CR);
      item->pxoffset = 48;
      item->pyoffset = 74;
    }

    /* file */
    menu_init(&menu_file, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_file.selector = menu_add_submenu(&menu_file, 0, 0, NULL,
                                          "return");
    menu_add_button(&menu_file, 0, 1, "restore skulltulas",
                    restore_gs_proc, NULL);
    menu_add_button(&menu_file, 0, 2, "call navi",
                    call_navi_proc, NULL);
    {
      memfile = malloc(sizeof(*memfile) * SETTINGS_MEMFILE_MAX);
      static struct slot_info memfile_slot_info =
      {
        &memfile_slot, SETTINGS_MEMFILE_MAX,
      };
      menu_add_static(&menu_file, 0, 3, "memory file", 0xC0C0C0);
      menu_add_watch(&menu_file, 19, 3,
                     (uint32_t)memfile_slot_info.data, WATCH_TYPE_U8);
      menu_add_button(&menu_file, 17, 3, "-",
                      slot_dec_proc, &memfile_slot_info);
      menu_add_button(&menu_file, 21, 3, "+",
                      slot_inc_proc, &memfile_slot_info);
    }
    {
      struct gfx_texture *t = resource_get(RES_ICON_DAYTIME);
      menu_add_static(&menu_file, 0, 4, "time of day", 0xC0C0C0);
      menu_add_button_icon(&menu_file, 17, 4, t, 0, 0xFFC800,
                           set_time_proc, (void*)0x4AB0);
      menu_add_button_icon(&menu_file, 19, 4, t, 1, 0xA0A0E0,
                           set_time_proc, (void*)0xC010);
      menu_add_intinput(&menu_file, 21, 4, 16, 4,
                        halfword_mod_proc, &z64_file.day_time);
    }
    {
      struct gfx_texture *t = resource_get(RES_ICON_CHECK);
      menu_add_static(&menu_file, 0, 5, "carpenters freed", 0xC0C0C0);
      menu_add_button_icon(&menu_file, 17, 5, t, 0, 0x00FF00,
                           set_carpenter_flags_proc, NULL);
      menu_add_button_icon(&menu_file, 19, 5, t, 1, 0xFF0000,
                           clear_carpenter_flags_proc, NULL);
      menu_add_static(&menu_file, 0, 6, "intro cutscenes", 0xC0C0C0);
      menu_add_button_icon(&menu_file, 17, 6, t, 0, 0x00FF00,
                           set_intro_flags_proc, NULL);
      menu_add_button_icon(&menu_file, 19, 6, t, 1, 0xFF0000,
                           clear_intro_flags_proc, NULL);
      menu_add_static(&menu_file, 0, 7, "rewards obtained", 0xC0C0C0);
      menu_add_button_icon(&menu_file, 17, 7, t, 0, 0x00FF00,
                           set_reward_flags_proc, NULL);
      menu_add_button_icon(&menu_file, 19, 7, t, 1, 0xFF0000,
                           clear_reward_flags_proc, NULL);
    }
    menu_add_static(&menu_file, 0, 8, "timer 1", 0xC0C0C0);
    menu_add_intinput(&menu_file, 17, 8, 10, 5,
                      halfword_mod_proc, &z64_file.timer_1_value);
    menu_add_option(&menu_file, 23, 8,
                    "inactive\0""heat starting\0""heat initial\0"
                    "heat moving\0""heat active\0""race starting\0"
                    "race initial\0""race moving\0""race active\0"
                    "race stopped\0""race ending\0",
                    halfword_optionmod_proc, &z64_file.timer_1_state);
    menu_add_static(&menu_file, 0, 9, "timer 2", 0xC0C0C0);
    menu_add_intinput(&menu_file, 17, 9, 10, 5,
                      halfword_mod_proc, &z64_file.timer_2_value);
    menu_add_option(&menu_file, 23, 9,
                    "inactive\0""starting\0""initial\0"
                    "moving\0""active\0""stopped\0",
                    halfword_optionmod_proc, &z64_file.timer_2_state);
    menu_add_static(&menu_file, 0, 10, "file index", 0xC0C0C0);
    menu_add_intinput(&menu_file, 17, 10, 16, 2,
                      byte_mod_proc, &z64_file.file_index);
    {
      static int8_t language_options[] = {0x00, 0x01};
      static struct byte_option language_option_data =
      {
        &z64_file.language, language_options, 2,
      };
      menu_add_static(&menu_file, 0, 11, "language", 0xC0C0C0);
      menu_add_option(&menu_file, 17, 11, "japanese\0""english\0",
                      byte_option_proc, &language_option_data);
      static int8_t target_options[] = {0x00, 0x01};
      static struct byte_option target_option_data =
      {
        &z64_file.z_targeting, target_options, 2,
      };
      menu_add_static(&menu_file, 0, 12, "z targeting", 0xC0C0C0);
      menu_add_option(&menu_file, 17, 12, "switch\0""hold\0",
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
    menu_add_static(&menu_settings, 0, 2, "font", 0xC0C0C0);
    menu_font_option = menu_add_option(&menu_settings, 14, 2,
                                       "fipps\0""notalot35\0"
                                       "origami mommy\0""pc senior\0"
                                       "pixel intv\0""press start 2p\0"
                                       "smw text nc\0""werdna's return\0",
                                       menu_font_option_proc, NULL);
    menu_add_static(&menu_settings, 0, 3, "drop shadow", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 3,
                      menu_drop_shadow_proc, NULL);
    menu_add_static(&menu_settings, 0, 4, "menu position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 14, 4, menu_position_proc, NULL);
    menu_add_static(&menu_settings, 0, 5, "input display", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 5, input_display_proc, NULL);
    menu_add_positioning(&menu_settings, 16, 5,
                         generic_position_proc, &settings->input_display_x);
    menu_add_static(&menu_settings, 0, 6, "lag counter", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 6, lag_counter_proc, NULL);
    menu_add_positioning(&menu_settings, 16, 6,
                         generic_position_proc, &settings->lag_counter_x);
    menu_add_option(&menu_settings, 18, 6, "frames\0""seconds\0",
                    lag_unit_proc, NULL);
    menu_add_static(&menu_settings, 0, 7, "timer", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 7, timer_proc, NULL);
    menu_add_positioning(&menu_settings, 16, 7,
                         generic_position_proc, &settings->timer_x);
    menu_add_static(&menu_settings, 0, 8, "pause display", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 14, 8, pause_display_proc, NULL);
    menu_add_static(&menu_settings, 0, 9, "break type", 0xC0C0C0);
    menu_add_option(&menu_settings, 14, 9, "normal\0""aggressive\0",
                    break_type_proc, NULL);
    static struct menu menu_commands;
    menu_add_submenu(&menu_settings, 0, 10, &menu_commands, "commands");
    menu_add_button(&menu_settings, 0, 11, "save settings",
                    save_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, 12, "load settings",
                    load_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, 13, "restore defaults",
                    restore_settings_proc, NULL);
    menu_init(&menu_commands, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_commands.selector = menu_add_submenu(&menu_commands, 0, 0,
                                              NULL, "return");
    {
      const int page_length = 16;
      int no_pages = (COMMAND_MAX + page_length - 1) / page_length;
      struct menu *pages = malloc(sizeof(*pages) * no_pages);
      struct menu_item *tab = menu_add_tab(&menu_commands, 0, 1,
                                           pages, no_pages);
      for (int i = 0; i < no_pages; ++i) {
        struct menu *page = &pages[i];
        menu_init(page, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
        for (int j = 0; j < page_length; ++j) {
          int n = i * page_length + j;
          if (n >= COMMAND_MAX)
            break;
          if (command_info[n].proc)
            menu_add_button(page, 0, j, command_info[n].name,
                            activate_command_proc, (void*)n);
          else
            menu_add_static(page, 0, j, command_info[n].name, 0xC0C0C0);
          binder_create(page, 18, j, n);
        }
      }
      if (no_pages > 0)
        menu_tab_goto(tab, 0);
      menu_add_button(&menu_commands, 8, 0, "<", tab_prev_proc, tab);
      menu_add_button(&menu_commands, 10, 0, ">", tab_next_proc, tab);
    }
    input_bind_set_override(COMMAND_MENU, 1);
    input_bind_set_override(COMMAND_RETURN, 1);
    input_bind_set_override(COMMAND_PREVROOM, 1);
    input_bind_set_override(COMMAND_NEXTROOM, 1);
    input_bind_set_disable(COMMAND_PREVROOM, 1);
    input_bind_set_disable(COMMAND_NEXTROOM, 1);
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

/* support libraries */
#include <startup.c>
#include <vector/vector.c>
#include <list/list.c>
#include <grc.c>
