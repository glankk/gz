#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <startup.h>
#include <mips.h>
#include <n64.h>
#include "explorer.h"
#include "files.h"
#include "flags.h"
#include "gfx.h"
#include "gz.h"
#include "input.h"
#include "item_option.h"
#include "mem.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "sys.h"
#include "ucode.h"
#include "watchlist.h"
#include "z64.h"
#include "zu.h"

#define STRINGIZE(S)  STRINGIZE_(S)
#define STRINGIZE_(S) #S

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
  int             n_scenes;
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

struct heap_node
{
  uint16_t          magic;
  uint16_t          free;
  size_t            size;
  struct heap_node *next;
  struct heap_node *prev;
  char              unk_00_[0x0008];
  OSId              thread_id;
  void             *arena;
  uint32_t          count_hi;
  uint32_t          count_lo;
  char              pad_00_[0x0008];
  char              data[];
};

struct actor_debug_info
{
  struct menu_item *edit_item;
  uint8_t           type;
  uint8_t           index;
};

struct actor_spawn_info
{
  uint16_t        actor_no;
  uint16_t        variable;
  int32_t         x;
  int32_t         y;
  int32_t         z;
  uint16_t        rx;
  uint16_t        ry;
  uint16_t        rz;
};

enum movie_state
{
  MOVIE_IDLE,
  MOVIE_RECORDING,
  MOVIE_PLAYING,
};


#define                     CPU_COUNTER_FREQ 46875000
__attribute__((section(".data")))
static _Bool                gz_ready = 0;
static uint8_t              profile = 0;
static struct menu          menu_main;
static struct menu          menu_explorer;
static struct menu          menu_global_watches;
static struct menu          menu_mem;
static struct menu_item    *menu_font_option;
static struct menu_item    *menu_watchlist;
static _Bool                menu_active = 0;
static int32_t              frames_queued = -1;
static int32_t              frame_counter = 0;
static int32_t              lag_vi_offset;
#ifndef WIIVC
static int64_t              cpu_counter = 0;
static _Bool                timer_active = 0;
static int64_t              timer_counter_offset;
static int64_t              timer_counter_prev;
#endif
static uint16_t             day_time_prev;
static int                  target_day_time = -1;
static struct memory_file  *memfile;
static _Bool                memfile_saved[SETTINGS_MEMFILE_MAX] = {0};
static uint8_t              memfile_slot = 0;
static _Bool                entrance_override_once = 0;
static _Bool                entrance_override_next = 0;
static int32_t              next_entrance = -1;
static enum movie_state     movie_state = MOVIE_IDLE;
static struct vector        movie_inputs;
static int                  movie_frame;
static int                  col_view_state = 0;

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
  RES_FONT_PIXELZIM,
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
  "freeze time",
  "no music",
  "items usable",
  "no minimap",
  "isg",
};

static void do_warp(int16_t entrance_index, uint16_t cutscene_index, int age)
{
  if (age == 0)
    age = z64_game.link_age;
  else
    --age;
  if (z64_game.link_age == age)
    z64_file.link_age = age;
  else
    z64_game.link_age = age;
  zu_execute_game(entrance_index, cutscene_index);
}

void save_memfile(struct memory_file *file)
{
  memcpy(&file->z_file, &z64_file, sizeof(file->z_file));
  file->entrance_override = entrance_override_next;
  file->next_entrance = next_entrance;
  file->scene_index = z64_game.scene_index;
  uint32_t f;
  f = z64_game.chest_flags;
  file->z_file.scene_flags[z64_game.scene_index].chest = f;
  f = z64_game.swch_flags;
  file->z_file.scene_flags[z64_game.scene_index].swch = f;
  f = z64_game.clear_flags;
  file->z_file.scene_flags[z64_game.scene_index].clear = f;
  f = z64_game.collect_flags;
  file->z_file.scene_flags[z64_game.scene_index].collect = f;
  memcpy(&file->scene_flags, &z64_game.swch_flags,
         sizeof(file->scene_flags));
  /* pause screen stuff */
  {
    file->start_icon_dd = z64_gameinfo.start_icon_dd;
    file->pause_screen = z64_game.pause_screen;
    file->item_screen_cursor = z64_game.item_screen_cursor;
    file->quest_screen_cursor = z64_game.quest_screen_cursor;
    file->equip_screen_cursor = z64_game.equip_screen_cursor;
    file->map_screen_cursor = z64_game.map_screen_cursor;
    file->item_screen_x = z64_game.item_screen_x;
    file->equipment_screen_x = z64_game.equipment_screen_x;
    file->item_screen_y = z64_game.item_screen_y;
    file->equipment_screen_y = z64_game.equipment_screen_y;
    file->pause_screen_cursor = z64_game.pause_screen_cursor;
    file->quest_screen_item = z64_game.quest_screen_item;
    file->quest_screen_hilite = z64_game.quest_screen_hilite;
    file->quest_screen_song = z64_game.quest_screen_song;
  }
}

void load_memfile(struct memory_file *file)
{
  /* keep some data intact to prevent glitchiness */
  int32_t entrance_index = z64_file.entrance_index;
  int32_t link_age = z64_file.link_age;
  int8_t seq_index = z64_file.seq_index;
  int8_t night_sfx = z64_file.night_sfx;
  uint8_t minimap_index = z64_file.minimap_index;
  z64_gameinfo_t *gameinfo = z64_file.gameinfo;
  memcpy(&z64_file, &file->z_file, sizeof(file->z_file));
  z64_game.link_age = z64_file.link_age;
  z64_file.entrance_index = entrance_index;
  z64_file.seq_index = seq_index;
  z64_file.night_sfx = night_sfx;
  z64_file.minimap_index = minimap_index;
  z64_file.gameinfo = gameinfo;
  z64_file.link_age = link_age;
  entrance_override_next = file->entrance_override;
  next_entrance = file->next_entrance;
  if (next_entrance == -1)
    next_entrance = file->z_file.entrance_index;
  if (file->scene_index == z64_game.scene_index)
    memcpy(&z64_game.swch_flags, &file->scene_flags,
           sizeof(file->scene_flags));
  else {
    uint32_t f;
    f = z64_file.scene_flags[z64_game.scene_index].chest;
    z64_game.chest_flags = f;
    f = z64_file.scene_flags[z64_game.scene_index].swch;
    z64_game.swch_flags = f;
    f = z64_file.scene_flags[z64_game.scene_index].clear;
    z64_game.clear_flags = f;
    f = z64_file.scene_flags[z64_game.scene_index].collect;
    z64_game.collect_flags = f;
  }
  /* pause screen stuff */
  if (z64_game.pause_state == 0) {
    z64_gameinfo.start_icon_dd = file->start_icon_dd;
    z64_game.pause_screen = file->pause_screen;
    z64_game.item_screen_cursor = file->item_screen_cursor;
    z64_game.quest_screen_cursor = file->quest_screen_cursor;
    z64_game.equip_screen_cursor = file->equip_screen_cursor;
    z64_game.map_screen_cursor = file->map_screen_cursor;
    z64_game.item_screen_x = file->item_screen_x;
    z64_game.equipment_screen_x = file->equipment_screen_x;
    z64_game.item_screen_y = file->item_screen_y;
    z64_game.equipment_screen_y = file->equipment_screen_y;
    z64_game.pause_screen_cursor = file->pause_screen_cursor;
    z64_game.quest_screen_item = file->quest_screen_item;
    z64_game.quest_screen_hilite = file->quest_screen_hilite;
    z64_game.quest_screen_song = file->quest_screen_song;
  }
  for (int i = 0; i < 4; ++i)
    if (z64_file.button_items[i] != Z64_ITEM_NULL)
      z64_UpdateItemButton(&z64_game, i);
  z64_UpdateEquipment(&z64_game, &z64_link);
}

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
  save_memfile(&memfile[memfile_slot]);
  memfile_saved[memfile_slot] = 1;
}

void command_loadmemfile(void)
{
  if (!memfile_saved[memfile_slot])
    return;
  load_memfile(&memfile[memfile_slot]);
}

void command_resetlag(void)
{
  frame_counter = 0;
  lag_vi_offset = -(int32_t)z64_vi_counter;
}

#ifndef WIIVC
void command_timer(void)
{
  timer_active = !timer_active;
}

void command_resettimer(void)
{
  timer_counter_offset = -cpu_counter;
  timer_counter_prev = cpu_counter;
}
#endif

void command_pause(void)
{
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

void command_reload(void)
{
  entrance_override_once = entrance_override_next;
  if (next_entrance != -1)
    do_warp(next_entrance, 0x0000, 0);
  else
    do_warp(z64_file.entrance_index, 0x0000, 0);
}

void command_void(void)
{
  z64_file.link_age = z64_game.link_age;
  zu_void();
}

void command_reset(void)
{
  zu_reset();
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
  int age = z64_file.link_age;
  z64_file.link_age = z64_game.link_age;
  z64_game.link_age = !z64_game.link_age;
  z64_SwitchAgeEquips();
  z64_file.link_age = age;
  for (int i = 0; i < 4; ++i)
    if (z64_file.button_items[i] != Z64_ITEM_NULL)
      z64_UpdateItemButton(&z64_game, i);
  z64_UpdateEquipment(&z64_game, &z64_link);
}

#ifndef WIIVC
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
#endif

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

void command_colview(void)
{
  if (col_view_state == 0)
    col_view_state = 1;
  else if (col_view_state == 2)
    col_view_state = 3;
}

void command_recordmacro(void)
{
  if (movie_state == MOVIE_RECORDING)
    movie_state = MOVIE_IDLE;
  else {
    movie_state = MOVIE_RECORDING;
    vector_erase(&movie_inputs, 0, movie_inputs.size);
  }
}

void command_playmacro(void)
{
  if (movie_state == MOVIE_PLAYING)
    movie_state = MOVIE_IDLE;
  else if (movie_inputs.size > 0) {
    movie_state = MOVIE_PLAYING;
    movie_frame = 0;
  }
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
#ifndef WIIVC
  {"start/stop timer",  command_timer,        CMDACT_PRESS_ONCE},
  {"reset timer",       command_resettimer,   CMDACT_HOLD},
#endif
  {"pause/unpause",     command_pause,        CMDACT_PRESS_ONCE},
  {"frame advance",     command_advance,      CMDACT_PRESS},
  {"file select",       command_fileselect,   CMDACT_PRESS_ONCE},
  {"reload scene",      command_reload,       CMDACT_PRESS_ONCE},
  {"void out",          command_void,         CMDACT_PRESS_ONCE},
#ifndef WIIVC
  {"reset",             command_reset,        CMDACT_PRESS_ONCE},
#endif
  {"turbo",             command_turbo,        CMDACT_HOLD},
  {"fall",              command_fall,         CMDACT_HOLD},
  {"toggle age",        command_age,          CMDACT_PRESS_ONCE},
#ifndef WIIVC
  {"start timer",       command_starttimer,   CMDACT_PRESS_ONCE},
  {"stop timer",        command_stoptimer,    CMDACT_PRESS_ONCE},
#endif
  {"previous position", command_prevpos,      CMDACT_PRESS_ONCE},
  {"next position",     command_nextpos,      CMDACT_PRESS_ONCE},
  {"previous memfile",  command_prevfile,     CMDACT_PRESS_ONCE},
  {"next memfile",      command_nextfile,     CMDACT_PRESS_ONCE},
  {"collision view",    command_colview,      CMDACT_PRESS_ONCE},
  {"record macro",      command_recordmacro,  CMDACT_PRESS_ONCE},
  {"play macro",        command_playmacro,    CMDACT_PRESS_ONCE},
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

#ifndef WIIVC
static void update_cpu_counter(void)
{
  static uint32_t count = 0;
  uint32_t new_count;
  __asm__ volatile ("mfc0 $t0, $9 \n"
                    "nop          \n"
                    "sw   $t0, %0 \n" : "=m"(new_count) :: "t0");
  cpu_counter += new_count - count;
  count = new_count;
}
#endif

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

static int macro_input_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.macro_input = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.macro_input = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.macro_input);
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

#ifndef WIIVC
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
#endif

static void show_menu(void)
{
  menu_signal_enter(&menu_main, MENU_SWITCH_SHOW);
  menu_active = 1;
  input_reserve(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
                BUTTON_L);
  input_reservation_set(1);
}

static void hide_menu(void)
{
  menu_signal_leave(&menu_main, MENU_SWITCH_HIDE);
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

static int word_mod_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  uint32_t *p = data;
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

static int byte_optionmod_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  uint8_t *v = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != *v)
      menu_option_set(item, *v);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *v = menu_option_get(item);
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

#ifndef WIIVC
static int do_save_file(const char *path, void *data)
{
  const char *err_str = NULL;
  struct memory_file *file = NULL;
  FILE *f = fopen(path, "wb");
  if (f) {
    file = malloc(sizeof(*file));
    save_memfile(file);
    if (fwrite(file, 1, sizeof(*file), f) == sizeof(*file)) {
      if (fclose(f))
        err_str = strerror(errno);
      f = NULL;
    }
    else
      err_str = strerror(ferror(f));
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
  if (file)
    free(file);
  if (err_str) {
    menu_prompt(&menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else
    return 0;
}

static int do_load_file(const char *path, void *data)
{
  const char *err_str = NULL;
  struct memory_file *file = NULL;
  FILE *f = fopen(path, "rb");
  if (f) {
    struct stat st;
    fstat(fileno(f), &st);
    if (st.st_size == sizeof(*file)) {
      file = malloc(sizeof(*file));
      if (fread(file, 1, sizeof(*file), f) == sizeof(*file)) {
        if (settings->menu_settings.load_to == SETTINGS_LOADTO_ZFILE ||
            settings->menu_settings.load_to == SETTINGS_LOADTO_BOTH)
        {
          load_memfile(file);
        }
        if (settings->menu_settings.load_to == SETTINGS_LOADTO_MEMFILE ||
            settings->menu_settings.load_to == SETTINGS_LOADTO_BOTH)
        {
          memfile[memfile_slot] = *file;
          memfile_saved[memfile_slot] = 1;
        }
      }
      else
        err_str = strerror(ferror(f));
    }
    else
      err_str = "the file is corrupted";
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
  if (file)
    free(file);
  if (err_str) {
    menu_prompt(&menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else {
    if (settings->menu_settings.load_to == SETTINGS_LOADTO_ZFILE ||
        settings->menu_settings.load_to == SETTINGS_LOADTO_BOTH)
    {
      if (settings->menu_settings.on_load == SETTINGS_ONLOAD_RELOAD)
        command_reload();
      else if (settings->menu_settings.on_load == SETTINGS_ONLOAD_VOID)
        command_void();
    }
    return 0;
  }
}

static void save_file_proc(struct menu_item *item, void *data)
{
  menu_get_file(&menu_main, GETFILE_SAVE, "file", ".ootsave",
                do_save_file, NULL);
}

static void load_file_proc(struct menu_item *item, void *data)
{
  menu_get_file(&menu_main, GETFILE_LOAD, NULL, ".ootsave",
                do_load_file, NULL);
}

static int load_file_to_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.load_to)
      menu_option_set(item, settings->menu_settings.load_to);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.load_to = menu_option_get(item);
  return 0;
}

static int on_file_load_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.on_load)
      menu_option_set(item, settings->menu_settings.on_load);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.on_load = menu_option_get(item);
  return 0;
}
#endif

static void set_entrance_proc(struct menu_item *item, void *data)
{
  z64_file.void_pos = z64_link.common.pos_2;
  z64_file.void_yaw = z64_link.common.rot_2.y;
  z64_file.void_var = z64_link.common.variable;
  z64_file.void_entrance = z64_file.entrance_index;
  z64_file.void_room_index = z64_game.room_index;
}

static void clear_scene_flags_proc(struct menu_item *item, void *data)
{
  memset(&z64_game.swch_flags, 0x00, 0x24);
}

static void set_scene_flags_proc(struct menu_item *item, void *data)
{
  memset(&z64_game.swch_flags, 0xFF, 0x24);
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
  uint16_t cutscene = settings->menu_settings.warp_cutscene;
  if (cutscene > 0x0000)
    cutscene += 0xFFEF;
  entrance_override_once = 1;
  do_warp(settings->warp_entrance, cutscene, settings->menu_settings.warp_age);
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
      uint16_t cutscene = settings->menu_settings.warp_cutscene;
      if (cutscene > 0x0000)
        cutscene += 0xFFEF;
      entrance_override_once = 1;
      do_warp(i, cutscene, settings->menu_settings.warp_age);
      if (zu_scene_info[scene_index].n_entrances > 1)
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
    if (settings->menu_settings.font_resource == RES_FONT_FIPPS)
      gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_NORMAL);
    else
      gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_FAST);
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
  size_t n_font_options = sizeof(menu_font_options) /
                          sizeof(*menu_font_options);
  for (int i = 0; i < n_font_options; ++i)
    if (menu_font_options[i] == settings->menu_settings.font_resource) {
      menu_option_set(menu_font_option, i);
      break;
    }
  struct gfx_font *font = resource_get(settings->menu_settings.font_resource);
  menu_set_font(&menu_main, font);
  menu_set_cell_width(&menu_main, font->char_width + font->letter_spacing);
  menu_set_cell_height(&menu_main, font->char_height + font->line_spacing);
  gfx_mode_set(GFX_MODE_DROPSHADOW, settings->menu_settings.drop_shadow);
  if (settings->menu_settings.font_resource == RES_FONT_FIPPS)
    gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_NORMAL);
  else
    gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_FAST);
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
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
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
               z64_file.cutscene_index - 0xFFF0);
  else
    gfx_printf(font, x, y + ch * 2, "current cutscene none");
  return 1;
}

static void load_room_proc(struct menu_item *item, void *data)
{
  uint8_t new_room_index = menu_intinput_get(data);
  if (new_room_index < z64_game.n_rooms) {
    if (new_room_index == z64_game.room_index) {
      z64_game.room_index = -1;
      z64_game.room_ptr = NULL;
      z64_UnloadRoom(&z64_game, &z64_game.room_index);
      z64_game.room_index = -1;
      z64_game.room_ptr = NULL;
    }
    else {
      z64_game.room_index = -1;
      z64_game.room_ptr = NULL;
      z64_UnloadRoom(&z64_game, &z64_game.room_index);
      z64_game.room_index = -1;
      z64_game.room_ptr = NULL;
      z64_LoadRoom(&z64_game, &z64_game.room_index, new_room_index);
    }
  }
}

static int col_view_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    if (col_view_state == 0)
      col_view_state = 1;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    if (col_view_state == 2)
      col_view_state = 3;
  }
  else if (reason == MENU_CALLBACK_THINK) {
    _Bool state = col_view_state == 1 || col_view_state == 2;
    if (menu_checkbox_get(item) != state)
      menu_checkbox_set(item, state);
  }
  return 0;
}

static int col_view_mode_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->menu_settings.col_view_mode)
      menu_option_set(item, settings->menu_settings.col_view_mode);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->menu_settings.col_view_mode = menu_option_get(item);
  return 0;
}

static int col_view_xlu_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.col_view_xlu = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.col_view_xlu = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.col_view_xlu);
  return 0;
}

static int col_view_line_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.col_view_line = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.col_view_line = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.col_view_line);
  return 0;
}

static int col_view_shade_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.col_view_shade = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.col_view_shade = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.col_view_shade);
  return 0;
}

static int col_view_rd_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->menu_settings.col_view_rd = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->menu_settings.col_view_rd = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->menu_settings.col_view_rd);
  return 0;
}

static _Bool heap_node_validate(struct heap_node *node)
{
  return node->magic == 0x7373 &&
         (!node->next ||
          ((uint32_t)node->next >= 0x80000000 &&
           (uint32_t)node->next < 0x80400000 &&
           (uint32_t)node->next > (uint32_t)node &&
           (uint32_t)node->next - (uint32_t)node == node->size + 0x30)) &&
         (!node->prev ||
          ((uint32_t)node->prev >= 0x80000000 &&
           (uint32_t)node->prev < 0x80400000 &&
           (uint32_t)node->prev < (uint32_t)node));
}

static int heap_draw_proc(struct menu_item *item,
                          struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int ch = menu_get_cell_height(item->owner, 1);
  /* collect heap data */
  const uint32_t heap_start = (uint32_t)&z64_link - 0x30;
  size_t max_alloc = 0;
  size_t min_alloc = 0;
  size_t total_alloc = 0;
  size_t max_avail = 0;
  size_t min_avail = 0;
  size_t total_avail = 0;
  size_t overhead = 0;
  size_t total = 0;
  struct heap_node *p = (void*)heap_start;
  struct heap_node *error_node = NULL;
  int node_count_total = 0;
  int node_count_free = 0;
  while (p) {
    ++node_count_total;
    if (!heap_node_validate(p)) {
      error_node = p;
      break;
    }
    if (p->free) {
      ++node_count_free;
      total_avail += p->size;
      if (p->size > max_avail)
        max_avail = p->size;
      if (min_avail == 0 || p->size < min_avail)
        min_avail = p->size;
    }
    else {
      total_alloc += p->size;
      if (p->size > max_alloc)
        max_alloc = p->size;
      if (min_alloc == 0 || p->size < min_alloc)
        min_alloc = p->size;
    }
    overhead += 0x30;
    total += p->size + 0x30;
    p = p->next;
  }
  /* show heap data */
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  gfx_printf(font, x, y + ch * 0, "node count     %i (%i free)",
             node_count_total, node_count_free);
  gfx_printf(font, x, y + ch * 1, "allocated      0x%8" PRIx32, total_alloc);
  gfx_printf(font, x, y + ch * 2, "available      0x%8" PRIx32, total_avail);
  gfx_printf(font, x, y + ch * 3, "max allocated  0x%8" PRIx32, max_alloc);
  gfx_printf(font, x, y + ch * 4, "min allocated  0x%8" PRIx32, min_alloc);
  gfx_printf(font, x, y + ch * 5, "max available  0x%8" PRIx32, max_avail);
  gfx_printf(font, x, y + ch * 6, "min available  0x%8" PRIx32, min_avail);
  gfx_printf(font, x, y + ch * 7, "overhead       0x%8" PRIx32, overhead);
  gfx_printf(font, x, y + ch * 8, "total          0x%8" PRIx32, total);
  if (error_node)
    gfx_printf(font, x, y + ch * 9, "erroneous node 0x%08" PRIx32,
               (uint32_t)error_node);
  /* plot graph */
  static struct gfx_texture *t = NULL;
  if (!t)
    t = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 1, 9, 1);
  size_t graph_width = t->tile_width * t->tiles_x;
  uint16_t *d = t->data;
  p = (void*)heap_start;
  while (p) {
    if (p == error_node)
      break;
    size_t b = (uint32_t)p - heap_start;
    size_t e = b + p->size + 0x30;
    b = b * graph_width / total;
    e = e * graph_width / total;
    for (size_t i = b; i < e; ++i) {
      uint16_t c;
      if (p->free)
        c = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
      else
        c = GPACK_RGBA5551(0x1F, 0x1F - p->size * 0x1F / max_alloc,
                           i == b ? 0x1F : 0x00, 0x01);
      d[i] = c;
    }
    p = p->next;
  }
  /* draw graph */
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, alpha));
  for (int i = 0; i < t->tiles_x; ++i) {
    struct gfx_sprite s =
    {
      t, i,
      x + t->tile_width * i,
      y + ch * 10,
      1.f, 16.f,
    };
    gfx_sprite_draw(&s);
  }
  return 1;
}

static size_t disp_size[4];
static size_t disp_p[4];
static size_t disp_d[4];

static int disp_draw_proc(struct menu_item *item,
                          struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  for (int i = 0; i < 4; ++i) {
    /* plot graph */
    static struct gfx_texture *t[4];
    if (!t[i])
      t[i] = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 1, 2, 1);
    size_t graph_width = t[i]->tile_width * t[i]->tiles_x;
    uint16_t *d = t[i]->data;
    size_t b = disp_p[i] * graph_width / disp_size[i];
    size_t e = disp_d[i] * graph_width / disp_size[i];
    for (size_t j = 0; j < graph_width; ++j) {
      uint16_t c;
      if (j < b) {
        if (j >= e)
          c = GPACK_RGBA5551(0xFF, 0x00, 0x00, 0x01);
        else
          c = GPACK_RGBA5551(0x00, 0xFF, 0x00, 0x01);
      }
      else if (j >= e)
        c = GPACK_RGBA5551(0x00, 0x00, 0xFF, 0x01);
      else
        c = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
      d[j] = c;
    }
    /* display info */
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
    static const char *disp_name[4] =
    {
      "work",
      "poly_opa",
      "poly_xlu",
      "overlay",
    };
    gfx_printf(font, x, y + ch * i * 2, "%s", disp_name[i]);
    gfx_printf(font, x + cw * 2, y + ch * (i * 2 + 1),
               "0x%04" PRIx32 "  0x%04" PRIx32, disp_size[i], disp_p[i]);
    gfx_printf(font, x + cw * 2 + 192, y + ch * (i * 2 + 1),
               "0x%04" PRIx32, disp_size[i] - disp_d[i]);
    /* draw graph */
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, alpha));
    for (int j = 0; j < t[i]->tiles_x; ++j) {
      struct gfx_sprite s =
      {
        t[i], j,
        x + cw * 2 + 120 + t[i]->tile_width * j,
        y + ch * i * 2 + 1,
        1.f, ch - 2,
      };
      gfx_sprite_draw(&s);
    }
  }
  return 1;
}

static void push_object_proc(struct menu_item *item, void *data)
{
  int16_t object_id = menu_intinput_get(data);
  if (object_id == 0)
    return;
  int n = z64_game.obj_ctxt.n_objects++;
  z64_mem_obj_t *obj = z64_game.obj_ctxt.objects;
  obj[n].id = -object_id;
  obj[n].getfile.vrom_addr = 0;
  size_t size = z64_object_table[object_id].vrom_end -
                z64_object_table[object_id].vrom_start;
  obj[n + 1].data = (char*)obj[n].data + size;
}

static void pop_object_proc(struct menu_item *item, void *data)
{
  if (z64_game.obj_ctxt.n_objects > 0)
    z64_game.obj_ctxt.objects[--z64_game.obj_ctxt.n_objects].id = 0;
}

static int objects_draw_proc(struct menu_item *item,
                             struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  char *s = z64_game.obj_ctxt.obj_space_start;
  char *e = z64_game.obj_ctxt.obj_space_end;
  char *p = z64_game.obj_ctxt.objects[z64_game.obj_ctxt.n_objects].data;
  size_t max_objects = sizeof(z64_game.obj_ctxt.objects) /
                       sizeof(*z64_game.obj_ctxt.objects) - 1;
  gfx_printf(font, x, y + ch * 0, "objects   %i / %i",
             z64_game.obj_ctxt.n_objects, max_objects);
  gfx_printf(font, x, y + ch * 1, "allocated %08" PRIx32, p - s);
  gfx_printf(font, x, y + ch * 2, "available %08" PRIx32, e - p);
  gfx_printf(font, x, y + ch * 3, "total     %08" PRIx32, e - s);
  for (int i = 0; i < z64_game.obj_ctxt.n_objects; ++i)
    gfx_printf(font, x + cw * (i % 2) * 16, y + ch * (i / 2 + 5),
               "%08" PRIx32 " %04" PRIx16,
               z64_game.obj_ctxt.objects[i].data,
               z64_game.obj_ctxt.objects[i].id);
  return 1;
}

static void actor_index_dec_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *info = data;
  int type = info->type;
  int index = info->index;
  do {
    if (--index >= 0)
      break;
    type = (type + 12 - 1) % 12;
    index = z64_game.actor_list[type].length;
  } while (type != info->type || index != info->index);
  info->type = type;
  info->index = index;
}

static void actor_index_inc_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *info = data;
  int type = info->type;
  int index = info->index + 1;
  do {
    if (index < z64_game.actor_list[type].length)
      break;
    type = (type + 1) % 12;
    index = 0;
  } while (type != info->type || index != info->index);
  info->type = type;
  info->index = index;
}

static int actor_draw_proc(struct menu_item *item,
                           struct menu_draw_params *draw_params)
{
  struct actor_debug_info *info = item->data;
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  uint32_t max = z64_game.actor_list[info->type].length;
  if (info->index >= max) {
    if (max > 0)
      info->index = max - 1;
    else
      info->index = 0;
  }
  gfx_printf(font, x + cw * 14, y, "%i / %i", info->index, max);
  if (info->index < max) {
    z64_actor_t *actor = z64_game.actor_list[info->type].first;
    for (int i = 0; i < info->index; ++i)
      actor = actor->next;
    sprintf(info->edit_item->text, "%08" PRIx32, (uint32_t)actor);
    gfx_printf(font, x + cw * 10, y + ch * 1, "%04" PRIx16, actor->actor_id);
    gfx_printf(font, x, y + ch * 2, "variable  %04" PRIx16, actor->variable);

    {
      Mtx m;
      {
        MtxF mf;
        guTranslateF(&mf, actor->pos_2.x, actor->pos_2.y, actor->pos_2.z);
        MtxF mt;
        guRotateRPYF(&mt,
                     actor->rot_2.x * M_PI / 0x8000,
                     actor->rot_2.y * M_PI / 0x8000,
                     actor->rot_2.z * M_PI / 0x8000);
        guMtxCatF(&mt, &mf, &mf);
        guMtxF2L(&mf, &m);
      }
      Vtx v[6] =
      {
        gdSPDefVtxC(-8192, 0,      0,      0, 0, 0xFF, 0x00, 0x00, 0x80),
        gdSPDefVtxC(8192,  0,      0,      0, 0, 0xFF, 0x00, 0x00, 0x80),
        gdSPDefVtxC(0,      -8192, 0,      0, 0, 0x00, 0xFF, 0x00, 0x80),
        gdSPDefVtxC(0,      8192,  0,      0, 0, 0x00, 0xFF, 0x00, 0x80),
        gdSPDefVtxC(0,      0,      -8192, 0, 0, 0x00, 0x00, 0xFF, 0x80),
        gdSPDefVtxC(0,      0,      8192,  0, 0, 0x00, 0x00, 0xFF, 0x80),
      };
      load_l3dex2(&z64_ctxt.gfx->poly_xlu.p);
      gDPPipeSync(z64_ctxt.gfx->poly_xlu.p++);
      gDPSetCycleType(z64_ctxt.gfx->poly_xlu.p++, G_CYC_1CYCLE);
      gDPSetRenderMode(z64_ctxt.gfx->poly_xlu.p++,
                       G_RM_AA_ZB_XLU_LINE, G_RM_AA_ZB_XLU_LINE2);
      gDPSetCombineMode(z64_ctxt.gfx->poly_xlu.p++,
                        G_CC_PRIMITIVE, G_CC_PRIMITIVE);
      gSPLoadGeometryMode(z64_ctxt.gfx->poly_xlu.p++, G_ZBUFFER);
      gSPTexture(z64_ctxt.gfx->poly_xlu.p++, 0, 0, 0, 0, G_OFF);
      gSPMatrix(z64_ctxt.gfx->poly_xlu.p++,
                gDisplayListData(&z64_ctxt.gfx->poly_xlu.d, m),
                G_MTX_LOAD | G_MTX_NOPUSH | G_MTX_MODELVIEW);
      gSPVertex(z64_ctxt.gfx->poly_xlu.p++,
                gDisplayListData(&z64_ctxt.gfx->poly_xlu.d, v), 6, 0);
      gDPSetPrimColor(z64_ctxt.gfx->poly_xlu.p++,
                      0, 0, 0xFF, 0x00, 0x00, 0x80);
      gSPLine3D(z64_ctxt.gfx->poly_xlu.p++, 0, 1, 0);
      gDPSetPrimColor(z64_ctxt.gfx->poly_xlu.p++,
                      0, 0, 0x00, 0xFF, 0x00, 0x80);
      gSPLine3D(z64_ctxt.gfx->poly_xlu.p++, 2, 3, 0);
      gDPSetPrimColor(z64_ctxt.gfx->poly_xlu.p++,
                      0, 0, 0x00, 0x00, 0xFF, 0x80);
      gSPLine3D(z64_ctxt.gfx->poly_xlu.p++, 4, 5, 0);
      unload_l3dex2(&z64_ctxt.gfx->poly_xlu.p, 1);
    }
  }
  else
    strcpy(info->edit_item->text, "<none>");
  return 1;
}

static void edit_actor_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *info = data;
  if (info->index < z64_game.actor_list[info->type].length) {
    z64_actor_t *actor = z64_game.actor_list[info->type].first;
    for (int i = 0; i < info->index; ++i)
      actor = actor->next;
    menu_enter(&menu_main, &menu_mem);
    mem_goto((uint32_t)actor);
  }
}

static void delete_actor_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *info = data;
  if (info->index < z64_game.actor_list[info->type].length) {
    z64_actor_t *actor = z64_game.actor_list[info->type].first;
    for (int i = 0; i < info->index; ++i)
      actor = actor->next;
    z64_DeleteActor(&z64_game, &z64_game.actor_ctxt, actor);
  }
}

static void goto_actor_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *info = data;
  if (info->index < z64_game.actor_list[info->type].length) {
    z64_actor_t *actor = z64_game.actor_list[info->type].first;
    for (int i = 0; i < info->index; ++i)
      actor = actor->next;
    z64_link.common.pos_1 = actor->pos_2;
    z64_link.common.pos_2 = actor->pos_2;
  }
}

static void spawn_actor_proc(struct menu_item *item, void *data)
{
  struct actor_spawn_info *info = data;
  z64_SpawnActor(&z64_game.actor_ctxt, &z64_game, info->actor_no,
                  info->x, info->y, info->z, info->rx, info->ry, info->rz,
                  info->variable);
}

static void fetch_actor_info_proc(struct menu_item *item, void *data)
{
  struct actor_spawn_info *info = data;
  info->x = floorf(z64_link.common.pos_2.x + 0.5f);
  info->y = floorf(z64_link.common.pos_2.y + 0.5f);
  info->z = floorf(z64_link.common.pos_2.z + 0.5f);
  info->rx = z64_link.common.rot_2.x;
  info->ry = z64_link.common.rot_2.y;
  info->rz = z64_link.common.rot_2.z;
}

static void tab_prev_proc(struct menu_item *item, void *data)
{
  menu_tab_previous(data);
}

static void tab_next_proc(struct menu_item *item, void *data)
{
  menu_tab_next(data);
}

static void main_hook(void)
{
#ifndef WIIVC
  update_cpu_counter();
#endif
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
    z64_file.night_sfx = -1;
  }
  if (settings->cheats & (1 << CHEAT_USEITEMS))
    memset(&z64_game.restriction_flags, 0, sizeof(z64_game.restriction_flags));
  if (settings->cheats & (1 << CHEAT_NOMAP))
    z64_gameinfo.minimap_disabled = 1;
  if (settings->cheats & (1 << CHEAT_ISG))
    z64_link.sword_state = 1;

  for (int i = 0; i < COMMAND_MAX; ++i) {
    /* prevent instant resets */
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

  if (settings->menu_settings.pause_display) {
    if (movie_state == MOVIE_RECORDING) {
      struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
      struct gfx_sprite sprite =
      {
        t, frames_queued == -1 ? 3 : frames_queued == 0 ? 4 : 5,
        32, 32, 1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_sprite_draw(&sprite);
    }
    else if (movie_state == MOVIE_PLAYING) {
      struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
      struct gfx_sprite sprite =
      {
        t, frames_queued == -1 ? 0 : frames_queued == 0 ? 1 : 2,
        32, 32, 1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_sprite_draw(&sprite);
    }
    else if (frames_queued != -1) {
      struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
      struct gfx_sprite sprite =
      {
        t, frames_queued == 0 ? 1 : 2,
        32, 32, 1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
      gfx_sprite_draw(&sprite);
    }
  }

  if (settings->menu_settings.input_display) {
    struct gfx_texture *texture = resource_get(RES_ICON_BUTTONS);
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    gfx_printf(font, settings->input_display_x, settings->input_display_y,
               "%4i %4i", z64_input_direct.raw.x, z64_input_direct.raw.y);
    static const int buttons[] =
    {
      15, 14, 12, 3, 2, 1, 0, 13, 5, 4, 11, 10, 9, 8,
    };
    uint16_t z_pad = input_z_pad();
    for (int i = 0; i < sizeof(buttons) / sizeof(*buttons); ++i) {
      int b = buttons[i];
      if (!(z_pad & (1 << b)))
        continue;
      int x = (cw - texture->tile_width) / 2 + i * 10;
      int y = -(gfx_font_xheight(font) + texture->tile_height + 1) / 2;
      struct gfx_sprite sprite =
      {
        texture, b,
        settings->input_display_x + cw * 10 + x, settings->input_display_y + y,
        1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(input_button_color[b],
                                                 alpha));
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

#ifndef WIIVC
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
#endif

  if (menu_active)
    menu_draw(&menu_main);
  menu_draw(&menu_global_watches);

  {
    static Gfx *poly_disp;
    static Gfx *line_disp;
    static _Bool xlu;
    /* build collision view display list */
    if (col_view_state == 1) {
      xlu = settings->menu_settings.col_view_xlu;
      z64_col_hdr_t *col_hdr = z64_game.col_hdr;
      uint8_t alpha = xlu ? 0x80 : 0xFF;
      /* initialize polygon dlist */
      Gfx *poly_p = NULL;
      Gfx *poly_d = NULL;
      {
        size_t poly_size = 0x10 + 9 * col_hdr->n_poly;
        if (poly_disp)
          free(poly_disp);
        poly_disp = malloc(sizeof(*poly_disp) * poly_size);
        poly_p = poly_disp;
        poly_d = poly_disp + poly_size;
        uint32_t rm;
        uint32_t blc1;
        uint32_t blc2;
        if (xlu) {
          rm = Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL;
          blc1 = GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA);
          blc2 = GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA);
        }
        else {
          rm = Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP | FORCE_BL;
          blc1 = GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
          blc2 = GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
        }
        if (settings->menu_settings.col_view_mode == SETTINGS_COLVIEW_DECAL)
          rm |= ZMODE_DEC;
        else if (xlu)
          rm |= ZMODE_XLU;
        else
          rm |= ZMODE_OPA;
        gDPPipeSync(poly_p++);
        gDPSetRenderMode(poly_p++, rm | blc1, rm | blc2);
        gSPTexture(poly_p++, qu016(0.5), qu016(0.5), 0, G_TX_RENDERTILE, G_OFF);
        if (settings->menu_settings.col_view_shade) {
          gDPSetCycleType(poly_p++, G_CYC_2CYCLE);
          gDPSetCombineMode(poly_p++, G_CC_PRIMITIVE, G_CC_MODULATERGBA2);
          gSPLoadGeometryMode(poly_p++,
                              G_SHADE | G_LIGHTING | G_ZBUFFER | G_CULL_BACK);
        }
        else {
          gDPSetCycleType(poly_p++, G_CYC_1CYCLE);
          gDPSetCombineMode(poly_p++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
          gSPLoadGeometryMode(poly_p++, G_ZBUFFER | G_CULL_BACK);
        }
        Mtx m;
        guMtxIdent(&m);
        gSPMatrix(poly_p++,
                  gDisplayListData(&poly_d, m), G_MTX_MODELVIEW | G_MTX_LOAD);
      }
      /* initialize line dlist */
      struct line
      {
        int va;
        int vb;
      };
      struct vector line_set;
      Gfx *line_p = NULL;
      Gfx *line_d = NULL;
      if (settings->menu_settings.col_view_line) {
        vector_init(&line_set, sizeof(struct line));
        size_t line_size = 0x18 + 11 * col_hdr->n_poly;
        if (line_disp)
          free(line_disp);
        line_disp = malloc(sizeof(*line_disp) * line_size);
        line_p = line_disp;
        line_d = line_disp + line_size;
        load_l3dex2(&line_p);
        gDPPipeSync(line_p++);
        if (xlu)
          gDPSetRenderMode(line_p++, G_RM_AA_ZB_XLU_LINE, G_RM_AA_ZB_XLU_LINE2);
        else
          gDPSetRenderMode(line_p++, G_RM_AA_ZB_DEC_LINE, G_RM_AA_ZB_DEC_LINE2);
        gSPTexture(line_p++, qu016(0.5), qu016(0.5), 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCycleType(line_p++, G_CYC_1CYCLE);
        gDPSetCombineMode(line_p++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gSPLoadGeometryMode(line_p++, G_ZBUFFER);
        Mtx m;
        guMtxIdent(&m);
        gSPMatrix(line_p++,
                  gDisplayListData(&line_d, m), G_MTX_MODELVIEW | G_MTX_LOAD);
        gDPSetPrimColor(line_p++, 0, 0, 0x00, 0x00, 0x00, alpha);
      }
      /* enumerate collision polys */
      for (int i = 0; i < col_hdr->n_poly; ++i) {
        z64_col_poly_t *poly = &col_hdr->poly[i];
        z64_col_type_t *type = &col_hdr->type[poly->type];
        z64_xyz_t *va = &col_hdr->vtx[poly->va];
        z64_xyz_t *vb = &col_hdr->vtx[poly->vb];
        z64_xyz_t *vc = &col_hdr->vtx[poly->vc];
        /* generate polygon */
        _Bool skip = 0;
        if (type->flags_2.hookshot)
          gDPSetPrimColor(poly_p++, 0, 0, 0x80, 0x80, 0xFF, alpha);
        else if (type->flags_1.interaction > 0x01)
          gDPSetPrimColor(poly_p++, 0, 0, 0xC0, 0x00, 0xC0, alpha);
        else if (type->flags_1.special == 0x0C)
          gDPSetPrimColor(poly_p++, 0, 0, 0xFF, 0x00, 0x00, alpha);
        else if (type->flags_1.exit != 0x00 || type->flags_1.special == 0x05)
          gDPSetPrimColor(poly_p++, 0, 0, 0x00, 0xFF, 0x00, alpha);
        else if (type->flags_1.behavior != 0 || type->flags_2.wall_damage)
          gDPSetPrimColor(poly_p++, 0, 0, 0xC0, 0xFF, 0xC0, alpha);
        else if (type->flags_2.terrain == 0x01)
          gDPSetPrimColor(poly_p++, 0, 0, 0xFF, 0xFF, 0x80, alpha);
        else if (settings->menu_settings.col_view_rd)
          skip = 1;
        else
          gDPSetPrimColor(poly_p++, 0, 0, 0xFF, 0xFF, 0xFF, alpha);
        if (!skip) {
          Vtx pvg[3] =
          {
            gdSPDefVtxN(va->x, va->y, va->z, 0, 0,
                        poly->norm.x / 0x100, poly->norm.y / 0x100,
                        poly->norm.z / 0x100, 0xFF),
            gdSPDefVtxN(vb->x, vb->y, vb->z, 0, 0,
                        poly->norm.x / 0x100, poly->norm.y / 0x100,
                        poly->norm.z / 0x100, 0xFF),
            gdSPDefVtxN(vc->x, vc->y, vc->z, 0, 0,
                        poly->norm.x / 0x100, poly->norm.y / 0x100,
                        poly->norm.z / 0x100, 0xFF),
          };
          gSPVertex(poly_p++, gDisplayListData(&poly_d, pvg), 3, 0);
          gSP1Triangle(poly_p++, 0, 1, 2, 0);
        }
        /* generate lines */
        if (settings->menu_settings.col_view_line) {
          struct line lab = {poly->va, poly->vb};
          struct line lbc = {poly->vb, poly->vc};
          struct line lca = {poly->vc, poly->va};
          _Bool ab = 1;
          _Bool bc = 1;
          _Bool ca = 1;
          for (int i = 0; i < line_set.size; ++i) {
            struct line *l = vector_at(&line_set, i);
            if ((l->va == lab.va && l->vb == lab.vb) ||
                (l->va == lab.vb && l->vb == lab.va))
              ab = 0;
            if ((l->va == lbc.va && l->vb == lbc.vb) ||
                (l->va == lbc.vb && l->vb == lbc.va))
              bc = 0;
            if ((l->va == lca.va && l->vb == lca.vb) ||
                (l->va == lca.vb && l->vb == lca.va))
              ca = 0;
          }
          if (!ab && !bc && !ca)
            continue;
          Vtx lvg[3] =
          {
            gdSPDefVtxC(va->x, va->y, va->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
            gdSPDefVtxC(vb->x, vb->y, vb->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
            gdSPDefVtxC(vc->x, vc->y, vc->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
          };
          gSPVertex(line_p++, gDisplayListData(&line_d, lvg), 3, 0);
          if (ab) {
            vector_push_back(&line_set, 1, &lab);
            gSPLine3D(line_p++, 0, 1, 0);
          }
          if (bc) {
            vector_push_back(&line_set, 1, &lbc);
            gSPLine3D(line_p++, 1, 2, 0);
          }
          if (ca) {
            vector_push_back(&line_set, 1, &lca);
            gSPLine3D(line_p++, 2, 0, 0);
          }
        }
      }
      /* finalize dlists */
      gSPEndDisplayList(poly_p++);
      if (settings->menu_settings.col_view_line) {
        vector_destroy(&line_set);
        unload_l3dex2(&line_p, 1);
        gSPEndDisplayList(line_p++);
      }
      col_view_state = 2;
    }
    if (col_view_state == 2 && z64_game.pause_state == 0) {
      if (xlu) {
        if (poly_disp)
          gSPDisplayList(z64_ctxt.gfx->poly_xlu.p++, poly_disp);
        if (line_disp)
          gSPDisplayList(z64_ctxt.gfx->poly_xlu.p++, line_disp);
      }
      else {
        if (poly_disp)
          gSPDisplayList(z64_ctxt.gfx->poly_opa.p++, poly_disp);
        if (line_disp)
          gSPDisplayList(z64_ctxt.gfx->poly_opa.p++, line_disp);
      }
    }
    if (col_view_state == 3)
      col_view_state = 4;
    else if (col_view_state == 4) {
      if (poly_disp) {
        free(poly_disp);
        poly_disp = NULL;
      }
      if (line_disp) {
        free(line_disp);
        line_disp = NULL;
      }
      col_view_state = 0;
    }
  }

  {
    static int splash_time = 230;
    if (splash_time > 0) {
      --splash_time;
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      const char *tarname = STRINGIZE(PACKAGE_TARNAME);
      const char *url = STRINGIZE(PACKAGE_URL);
      gfx_printf(font, 16, Z64_SCREEN_HEIGHT - 6 - ch, tarname);
      gfx_printf(font, Z64_SCREEN_WIDTH - 12 - cw * strlen(url),
                 Z64_SCREEN_HEIGHT - 6 - ch, url);
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

HOOK void entrance_offset_hook(void)
{
  init_gp();
  uint32_t offset;
  if (z64_file.void_flag && z64_file.cutscene_index == 0x0000)
    entrance_override_once = entrance_override_next;
  if (entrance_override_once) {
    offset = 0;
    entrance_override_once = 0;
    entrance_override_next = 1;
  }
  else {
    offset = z64_file.scene_setup_index;
    entrance_override_next = 0;
  }
  next_entrance = -1;
  __asm__ volatile (".set noat  \n"
                    "lw $v1, %0 \n"
                    "la $at, %1 \n" :: "m"(offset), "i"(0x51));
}

HOOK void update_hook(void)
{
  init_gp();
  void (*frame_update_func)(z64_ctxt_t *ctxt);
  frame_update_func = (void*)z64_frame_update_func_addr;
  if (!gz_ready)
    frame_update_func(&z64_ctxt);
  else if (frames_queued != 0) {
    if (frames_queued > 0)
      --frames_queued;
    frame_update_func(&z64_ctxt);
  }
}

HOOK void input_hook(void)
{
  init_gp();
  void (*frame_input_func)(z64_ctxt_t *ctxt);
  frame_input_func = (void*)z64_frame_input_func_addr;
  if (!gz_ready)
    frame_input_func(&z64_ctxt);
  else if (frames_queued != 0) {
    z64_input_t input = z64_input_direct;
    frame_input_func(&z64_ctxt);
    if (movie_state == MOVIE_RECORDING)
      vector_push_back(&movie_inputs, 1, &z64_ctxt.input[0]);
    else if (movie_state == MOVIE_PLAYING) {
      if (movie_frame >= movie_inputs.size) {
        if (input_bind_held(COMMAND_PLAYMACRO) && movie_inputs.size > 0)
          movie_frame = 0;
        else
          movie_state = MOVIE_IDLE;
      }
      if (movie_state == MOVIE_PLAYING) {
        z64_input_t *frame_input = vector_at(&movie_inputs, movie_frame++);
        z64_input_t *ctxt_input = &z64_ctxt.input[0];
        *ctxt_input = *frame_input;
        if (settings->menu_settings.macro_input) {
          if (frame_input->adjusted_x == 0) {
            ctxt_input->raw.x = input.raw.x;
            ctxt_input->x_diff = input.x_diff;
            ctxt_input->adjusted_x = input.adjusted_x;
          }
          if (frame_input->adjusted_y == 0) {
            ctxt_input->raw.y = input.raw.y;
            ctxt_input->y_diff = input.y_diff;
            ctxt_input->adjusted_y = input.adjusted_y;
          }
          ctxt_input->raw.pad |= input.raw.pad;
          ctxt_input->pad_pressed |= input.pad_pressed;
          ctxt_input->pad_released |= input.pad_released;
        }
      }
    }
  }
}

HOOK void disp_hook(z64_disp_buf_t *disp_buf, Gfx *buf, uint32_t size)
{
  init_gp();
  z64_disp_buf_t *z_disp[4] =
  {
    &z64_ctxt.gfx->work,
    &z64_ctxt.gfx->poly_opa,
    &z64_ctxt.gfx->poly_xlu,
    &z64_ctxt.gfx->overlay,
  };
  for (int i = 0; i < 4; ++i)
    if (disp_buf == z_disp[i]) {
      disp_size[i] = disp_buf->size;
      disp_p[i] = (char*)disp_buf->p - (char*)disp_buf->buf;
      disp_d[i] = (char*)disp_buf->d - (char*)disp_buf->buf;
      break;
    }
  disp_buf->size = size;
  disp_buf->buf = buf;
  disp_buf->p = buf;
  disp_buf->d = (void*)((char*)buf + size);
}

static void stack_thunk(void (*func)(void))
{
  static __attribute__((section(".stack"))) _Alignas(8)
  char stack[0x2000];
  __asm__ volatile ("la     $t0, %1       \n"
                    "sw     $sp, -4($t0)  \n"
                    "sw     $ra, -8($t0)  \n"
                    "addiu  $sp, $t0, -8  \n"
                    "jalr   %0            \n"
                    "lw     $ra, 0($sp)   \n"
                    "lw     $sp, 4($sp)   \n"
                    :: "r"(func), "i"(&stack[sizeof(stack)]));
}

static void init(void)
{
  /* startup */
  {
    clear_bss();
    do_global_ctors();
  }

  /* load settings */
  if (input_z_pad() == BUTTON_START || !settings_load(profile))
    settings_load_default();
  input_update();

  /* initialize gfx */
  {
    gfx_start();
    gfx_mode_configure(GFX_MODE_FILTER, G_TF_POINT);
    gfx_mode_configure(GFX_MODE_COMBINE, G_CC_MODE(G_CC_MODULATEIA_PRIM,
                                                   G_CC_MODULATEIA_PRIM));
  }

  /* disable map toggling */
  *(uint32_t*)z64_minimap_disable_1_addr = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x82C);
  *(uint32_t*)z64_minimap_disable_2_addr = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x98);

  /* initialize variables */
#ifndef WIIVC
  update_cpu_counter();
  timer_counter_offset = -cpu_counter;
  timer_counter_prev = cpu_counter;
#endif
  lag_vi_offset = -(int32_t)z64_vi_counter;
  day_time_prev = z64_file.day_time;
  vector_init(&movie_inputs, sizeof(z64_input_t));

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
    static struct menu menu_debug;
    menu_add_submenu(&menu_main, 0, 8, &menu_debug, "debug");
    static struct menu menu_settings;
    menu_add_submenu(&menu_main, 0, 9, &menu_settings, "settings");

    /* warps */
    menu_init(&menu_warps, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_warps.selector = menu_add_submenu(&menu_warps, 0, 0, NULL,
                                           "return");
    static struct menu places;
    menu_init(&places, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    places.selector = menu_add_submenu(&places, 0, 0, NULL, "return");
    size_t n_scene_categories = sizeof(scene_categories) /
                                sizeof(*scene_categories);
    for (int i = 0; i < n_scene_categories; ++i) {
      struct scene_category *cat = &scene_categories[i];
      struct menu *cat_menu = malloc(sizeof(*cat_menu));
      menu_init(cat_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
      cat_menu->selector = menu_add_submenu(cat_menu, 0, 0, NULL, "return");
      for (int j = 0; j < cat->n_scenes; ++j) {
        uint8_t scene_index = cat->scenes[j];
        struct zu_scene_info *scene = &zu_scene_info[scene_index];
        if (scene->n_entrances > 1) {
          struct menu *scene_menu = malloc(sizeof(*scene_menu));
          menu_init(scene_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
          scene_menu->selector = menu_add_submenu(scene_menu, 0, 0, NULL,
                                                  "return");
          for (uint8_t k = 0; k < scene->n_entrances; ++k)
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
    menu_add_option(&menu_warps, 9, 2, "current age\0""adult\0""child\0",
                    age_option_proc, NULL);
    menu_add_static(&menu_warps, 0, 3, "entrance", 0xC0C0C0);
    menu_add_intinput(&menu_warps, 9, 3, 16, 4,
                      halfword_mod_proc, &settings->warp_entrance);
    menu_add_static(&menu_warps, 0, 4, "cutscene", 0xC0C0C0);
    menu_add_option(&menu_warps, 9, 4,
                    "none\0""0\0""1\0""2\0""3\0""4\0""5\0""6\0""7\0"
                    "8\0""9\0""10\0""11\0""12\0""13\0""14\0""15\0",
                    cutscene_option_proc, NULL);
    menu_add_button(&menu_warps, 0, 5, "warp", warp_proc, NULL);
    menu_add_button(&menu_warps, 0, 6, "clear cs pointer",
                    clear_csp_proc, NULL);
    menu_add_static_custom(&menu_warps, 0, 7, warp_info_draw_proc,
                           NULL, 0xC0C0C0);

    /* scene */
    menu_init(&menu_scene, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_scene.selector = menu_add_submenu(&menu_scene, 0, 0, NULL,
                                           "return");
    explorer_create(&menu_explorer);
    menu_add_submenu(&menu_scene, 0, 1, &menu_explorer, "explorer");
    menu_add_button(&menu_scene, 0, 2, "set entrance point",
                    set_entrance_proc, NULL);
    menu_add_button(&menu_scene, 0, 3, "clear flags",
                    clear_scene_flags_proc, NULL);
    menu_add_button(&menu_scene, 0, 4, "set flags",
                    set_scene_flags_proc, NULL);
    {
      struct menu_item *item;
      item = menu_add_intinput(&menu_scene, 16, 5, 10, 2, NULL, NULL);
      menu_add_button(&menu_scene, 0, 5, "load room", load_room_proc, item);
    }
    menu_add_static(&menu_scene, 0, 6, "collision view", 0xC0C0C0);
    menu_add_checkbox(&menu_scene, 16, 6, col_view_proc, NULL);
    menu_add_static(&menu_scene, 2, 7, "mode", 0xC0C0C0);
    menu_add_option(&menu_scene, 16, 7, "decal\0""surface\0",
                    col_view_mode_proc, NULL);
    menu_add_static(&menu_scene, 2, 8, "translucent", 0xC0C0C0);
    menu_add_checkbox(&menu_scene, 16, 8, col_view_xlu_proc, NULL);
    menu_add_static(&menu_scene, 2, 9, "wireframe", 0xC0C0C0);
    menu_add_checkbox(&menu_scene, 16, 9, col_view_line_proc, NULL);
    menu_add_static(&menu_scene, 2, 10, "shaded", 0xC0C0C0);
    menu_add_checkbox(&menu_scene, 16, 10, col_view_shade_proc, NULL);
    menu_add_static(&menu_scene, 2, 11, "reduced", 0xC0C0C0);
    menu_add_checkbox(&menu_scene, 16, 11, col_view_rd_proc, NULL);
    {
      static struct slot_info teleport_slot_info;
      teleport_slot_info.data = &settings->teleport_slot;
      teleport_slot_info.max = SETTINGS_TELEPORT_MAX;
      menu_add_static(&menu_scene, 0, 12, "teleport slot", 0xC0C0C0);
      menu_add_watch(&menu_scene, 18, 12,
                     (uint32_t)teleport_slot_info.data, WATCH_TYPE_U8);
      menu_add_button(&menu_scene, 16, 12, "-",
                      slot_dec_proc, &teleport_slot_info);
      menu_add_button(&menu_scene, 20, 12, "+",
                      slot_inc_proc, &teleport_slot_info);
    }
    menu_add_static(&menu_scene, 0, 13, "current scene", 0xC0C0C0);
    menu_add_watch(&menu_scene, 16, 13,
                   (uint32_t)&z64_game.scene_index, WATCH_TYPE_U16);
    menu_add_static(&menu_scene, 0, 14, "current room", 0xC0C0C0);
    menu_add_watch(&menu_scene, 16, 14,
                   (uint32_t)&z64_game.room_index, WATCH_TYPE_U8);
    menu_add_static(&menu_scene, 0, 15, "no. rooms", 0xC0C0C0);
    menu_add_watch(&menu_scene, 16, 15,
                   (uint32_t)&z64_game.n_rooms, WATCH_TYPE_U8);

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
        int n_tiles = option->multi_tile ? 7 : 8;
        struct gfx_texture *t = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_32b,
                                                   32, 32, 1, n_tiles);
        for (int j = 0; j < n_tiles; ++j) {
          if (option->multi_tile)
            gfx_texture_copy_tile(t, j, t_on, option->item_tile + j, 0);
          else
            gfx_texture_copy_tile(t, j, t_on, option->item_tile, 0);
          gfx_texture_copy_tile(t, j, t_am,
                                option->amount_tiles[8 - n_tiles + j], 1);
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
          char *tooltip = malloc(20);
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
    menu_add_checkbox(&menu_quest_items, 14, 2, double_defense_proc, NULL);
    menu_add_intinput(&menu_quest_items, 16, 2, 16, 2,
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
                    "race stopped\0""race ending\0""timer starting\0"
                    "timer initial\0""timer moving\0""timer active\0"
                    "timer stopped\0",
                    halfword_optionmod_proc, &z64_file.timer_1_state);
    menu_add_static(&menu_file, 0, 9, "timer 2", 0xC0C0C0);
    menu_add_intinput(&menu_file, 17, 9, 10, 5,
                      halfword_mod_proc, &z64_file.timer_2_value);
    menu_add_option(&menu_file, 23, 9,
                    "inactive\0""starting\0""initial\0""moving\0""active\0"
                    "stopped\0""ending\0""timer starting\0""timer initial\0"
                    "timer moving\0""timer active\0""timer stopped\0",
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
#ifndef WIIVC
      menu_add_static(&menu_file, 0, 13, "load file to", 0xC0C0C0);
      menu_add_option(&menu_file, 17, 13,
                      "zelda file\0""current memfile\0""both\0",
                      load_file_to_proc, NULL);
      menu_add_static(&menu_file, 0, 14, "after loading", 0xC0C0C0);
      menu_add_option(&menu_file, 17, 14,
                      "do nothing\0""reload scene\0""void out\0",
                      on_file_load_proc, NULL);
      menu_add_button(&menu_file, 0, 15, "save to disk",
                      save_file_proc, NULL);
      menu_add_button(&menu_file, 0, 16, "load from disk",
                      load_file_proc, NULL);
#endif
    }

    menu_init(&menu_watches, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_watches.selector = menu_add_submenu(&menu_watches, 0, 0, NULL,
                                             "return");
    menu_init(&menu_global_watches, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_imitate(&menu_global_watches, &menu_main);
    menu_watchlist = watchlist_create(&menu_watches, &menu_global_watches,
                                      0, 1);
    watchlist_fetch(menu_watchlist);

    /* debug */
    menu_init(&menu_debug, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_debug.selector = menu_add_submenu(&menu_debug, 0, 0, NULL,
                                           "return");
    static struct menu heap;
    menu_add_submenu(&menu_debug, 0, 1, &heap, "heap");
    menu_init(&heap, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    heap.selector = menu_add_submenu(&heap, 0, 0, NULL, "return");
    menu_add_static_custom(&heap, 0, 1, heap_draw_proc, NULL, 0xC0C0C0);
    static struct menu disp;
    menu_add_submenu(&menu_debug, 0, 2, &disp, "display lists");
    menu_init(&disp, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    disp.selector = menu_add_submenu(&disp, 0, 0, NULL, "return");
    menu_add_static_custom(&disp, 0, 1, disp_draw_proc, NULL, 0xC0C0C0);
    {
      static struct menu objects;
      menu_add_submenu(&menu_debug, 0, 3, &objects, "objects");
      menu_init(&objects, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
      objects.selector = menu_add_submenu(&objects, 0, 0, NULL, "return");
      menu_add_static(&objects, 0, 1, "object id", 0xC0C0C0);
      struct menu_item *object_no = menu_add_intinput(&objects, 10, 1, 16, 4,
                                                      NULL, NULL);
      menu_add_button(&objects, 16, 1, "push", push_object_proc, object_no);
      menu_add_button(&objects, 22, 1, "pop", pop_object_proc, NULL);
      menu_add_static_custom(&objects, 0, 3, objects_draw_proc, NULL, 0xC0C0C0);
    }
    {
      static struct menu actors;
      menu_add_submenu(&menu_debug, 0, 4, &actors, "actors");
      menu_init(&actors, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
      actors.selector = menu_add_submenu(&actors, 0, 0, NULL, "return");
      {
        static struct actor_debug_info data;
        menu_add_static(&actors, 0, 1, "type", 0xC0C0C0);
        menu_add_option(&actors, 10, 1,
                        "switch\0""prop (1)\0""player\0""bomb\0""npc\0"
                        "enemy\0""prop (2)\0""item/action\0""misc\0""boss\0"
                        "door\0""chest\0",
                        byte_optionmod_proc, &data.type);
        struct menu_item *item = menu_add_static_custom(&actors, 0, 2,
                                                        actor_draw_proc,
                                                        NULL, 0xC0C0C0);
        item->data = &data;
        menu_add_static(&actors, 0, 2, "index", 0xC0C0C0);
        menu_add_button(&actors, 10, 2, "<", actor_index_dec_proc, &data);
        menu_add_button(&actors, 12, 2, ">", actor_index_inc_proc, &data);
        item = menu_add_button(&actors, 0, 3, NULL, &edit_actor_proc, &data);
        item->text = malloc(9);
        item->text[0] = 0;
        data.edit_item = item;
        menu_add_button(&actors, 0, 5, "delete", &delete_actor_proc, &data);
        menu_add_button(&actors, 10, 5, "go to", &goto_actor_proc, &data);
      }
      {
        static struct actor_spawn_info data;
        menu_add_static(&actors, 0, 7, "actor id", 0xC0C0C0);
        menu_add_intinput(&actors, 10, 7, 16, 4,
                          halfword_mod_proc, &data.actor_no);
        menu_add_static(&actors, 0, 8, "variable", 0xC0C0C0);
        menu_add_intinput(&actors, 10, 8, 16, 4,
                          halfword_mod_proc, &data.variable);
        menu_add_static(&actors, 0, 9, "position", 0xC0C0C0);
        menu_add_intinput(&actors, 10, 9, -10, 6, word_mod_proc, &data.x);
        menu_add_intinput(&actors, 17, 9, -10, 6, word_mod_proc, &data.y);
        menu_add_intinput(&actors, 24, 9, -10, 6, word_mod_proc, &data.z);
        menu_add_static(&actors, 0, 10, "rotation", 0xC0C0C0);
        menu_add_intinput(&actors, 10, 10, 10, 5, halfword_mod_proc, &data.rx);
        menu_add_intinput(&actors, 17, 10, 10, 5, halfword_mod_proc, &data.ry);
        menu_add_intinput(&actors, 24, 10, 10, 5, halfword_mod_proc, &data.rz);
        menu_add_button(&actors, 0, 11, "spawn", spawn_actor_proc, &data);
        menu_add_button(&actors, 10, 11,"fetch from link",
                        &fetch_actor_info_proc, &data);
      }
    }
    static struct menu flags;
    menu_add_submenu(&menu_debug, 0, 5, &flags, "flags");
    flag_menu_create(&flags);
    menu_add_submenu(&menu_debug, 0, 6, &menu_mem, "memory");
    mem_menu_create(&menu_mem);

    /* settings */
    menu_init(&menu_settings, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    int y = 0;
    menu_settings.selector = menu_add_submenu(&menu_settings, 0, y, NULL,
                                              "return");
    {
      static struct slot_info profile_slot_info =
      {
        &profile, SETTINGS_PROFILE_MAX,
      };
      menu_add_static(&menu_settings, 0, ++y, "profile", 0xC0C0C0);
      menu_add_watch(&menu_settings, 18, y,
                     (uint32_t)profile_slot_info.data, WATCH_TYPE_U8);
      menu_add_button(&menu_settings, 16, y, "-",
                      slot_dec_proc, &profile_slot_info);
      menu_add_button(&menu_settings, 20, y, "+",
                      slot_inc_proc, &profile_slot_info);
    }
    menu_add_static(&menu_settings, 0, ++y, "font", 0xC0C0C0);
    menu_font_option = menu_add_option(&menu_settings, 16, y,
                                       "fipps\0""notalot35\0"
                                       "origami mommy\0""pc senior\0"
                                       "pixel intv\0""press start 2p\0"
                                       "smw text nc\0""werdna's return\0"
                                       "pixelzim\0",
                                       menu_font_option_proc, NULL);
    menu_add_static(&menu_settings, 0, ++y, "drop shadow", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 16, y,
                      menu_drop_shadow_proc, NULL);
    menu_add_static(&menu_settings, 0, ++y, "menu position", 0xC0C0C0);
    menu_add_positioning(&menu_settings, 16, y, menu_position_proc, NULL);
    menu_add_static(&menu_settings, 0, ++y, "input display", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 16, y, input_display_proc, NULL);
    menu_add_positioning(&menu_settings, 18, y,
                         generic_position_proc, &settings->input_display_x);
    menu_add_static(&menu_settings, 0, ++y, "lag counter", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 16, y, lag_counter_proc, NULL);
    menu_add_positioning(&menu_settings, 18, y,
                         generic_position_proc, &settings->lag_counter_x);
    menu_add_option(&menu_settings, 20, y, "frames\0""seconds\0",
                    lag_unit_proc, NULL);
#ifndef WIIVC
    menu_add_static(&menu_settings, 0, ++y, "timer", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 16, y, timer_proc, NULL);
    menu_add_positioning(&menu_settings, 18, y,
                         generic_position_proc, &settings->timer_x);
#endif
    menu_add_static(&menu_settings, 0, ++y, "pause display", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 16, y, pause_display_proc, NULL);
    menu_add_static(&menu_settings, 0, ++y, "macro input", 0xC0C0C0);
    menu_add_checkbox(&menu_settings, 16, y, macro_input_proc, NULL);
    menu_add_static(&menu_settings, 0, ++y, "break type", 0xC0C0C0);
    menu_add_option(&menu_settings, 16, y, "normal\0""aggressive\0",
                    break_type_proc, NULL);
    static struct menu menu_commands;
    menu_add_submenu(&menu_settings, 0, ++y, &menu_commands, "commands");
    menu_add_button(&menu_settings, 0, ++y, "save settings",
                    save_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, ++y, "load settings",
                    load_settings_proc, NULL);
    menu_add_button(&menu_settings, 0, ++y, "restore defaults",
                    restore_settings_proc, NULL);
    menu_init(&menu_commands, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_commands.selector = menu_add_submenu(&menu_commands, 0, 0,
                                              NULL, "return");
    {
      const int page_length = 16;
      int n_pages = (COMMAND_MAX + page_length - 1) / page_length;
      struct menu *pages = malloc(sizeof(*pages) * n_pages);
      struct menu_item *tab = menu_add_tab(&menu_commands, 0, 1,
                                           pages, n_pages);
      for (int i = 0; i < n_pages; ++i) {
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
      if (n_pages > 0)
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

  gz_ready = 1;
}

ENTRY void _start()
{
  init_gp();
  if (gz_ready)
    stack_thunk(main_hook);
  else
    stack_thunk(init);
}

/* support libraries */
#include <startup.c>
#include <set/set.c>
#include <vector/vector.c>
#include <list/list.c>
#include <grc.c>
