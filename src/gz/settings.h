#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdint.h>
#include "z64.h"

#define SETTINGS_ADDRESS      0x7A00
#define SETTINGS_MAXSIZE      (0x8000-(SETTINGS_ADDRESS))
#define SETTINGS_PADSIZE      ((sizeof(struct settings)+1)/2*2)
#define SETTINGS_PROFILE_MAX  ((SETTINGS_MAXSIZE)/(SETTINGS_PADSIZE))
#define SETTINGS_VERSION      0x0000

#define SETTINGS_WATCHES_MAX  14
#define SETTINGS_TELEPORT_MAX 5
#define SETTINGS_MEMFILE_MAX  10

#define SETTINGS_LAG_FRAMES   0
#define SETTINGS_LAG_SECONDS  1

enum cheats
{
  CHEAT_ENERGY,
  CHEAT_MAGIC,
  CHEAT_STICKS,
  CHEAT_NUTS,
  CHEAT_BOMBS,
  CHEAT_ARROWS,
  CHEAT_SEEDS,
  CHEAT_BOMBCHUS,
  CHEAT_BEANS,
  CHEAT_KEYS,
  CHEAT_RUPEES,
  CHEAT_NL,
  CHEAT_FFTIME,
  CHEAT_NOMUSIC,
  CHEAT_USEITEMS,
  CHEAT_MAX,
};

enum commands
{
  COMMAND_MENU,
  COMMAND_RETURN,
  COMMAND_CLEARCSP,
  COMMAND_VOID,
  COMMAND_RELOAD,
  COMMAND_FILESELECT,
  COMMAND_LEVITATE,
  COMMAND_TURBO,
  COMMAND_SAVEPOS,
  COMMAND_LOADPOS,
  COMMAND_SAVEMEMFILE,
  COMMAND_LOADMEMFILE,
  COMMAND_RESETLAG,
  COMMAND_PAUSE,
  COMMAND_ADVANCE,
  COMMAND_MAX,
};

struct settings_data
{
  /* order elements by size for space-efficient packing */
  z64_xyz_t   teleport_pos[SETTINGS_TELEPORT_MAX];
  uint32_t    watch_address[SETTINGS_WATCHES_MAX];
  z64_angle_t teleport_rot[SETTINGS_TELEPORT_MAX];
  int16_t     menu_x;
  int16_t     menu_y;
  int16_t     input_display_x;
  int16_t     input_display_y;
  int16_t     lag_counter_x;
  int16_t     lag_counter_y;
  int16_t     watch_x[SETTINGS_WATCHES_MAX];
  int16_t     watch_y[SETTINGS_WATCHES_MAX];
  uint16_t    warp_entrance;
  uint16_t    command_binds[COMMAND_MAX];
  uint8_t     menu_font_resource_id;
  int8_t      menu_drop_shadow;
  int8_t      input_display_enabled;
  int8_t      lag_counter_enabled;
  int8_t      lag_unit;
  uint8_t     teleport_slot;
  int8_t      warp_age;
  uint8_t     no_watches;
  int8_t      cheats[CHEAT_MAX];
  uint8_t     watch_type[SETTINGS_WATCHES_MAX];
  uint8_t     watch_anchored[SETTINGS_WATCHES_MAX];
  int8_t      watch_position_set[SETTINGS_WATCHES_MAX];
};

struct settings_header
{
  uint16_t  version;
  uint16_t  data_size;
  uint16_t  data_checksum;
};

struct settings
{
  struct settings_header  header;
  struct settings_data    data;
};

void  settings_load_default(void);
void  settings_save(int profile);
_Bool settings_load(int profile);

extern struct settings_data *settings;

#endif
