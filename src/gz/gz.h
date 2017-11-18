#ifndef GZ_H
#define GZ_H
#include <stdint.h>
#include "z64.h"

struct memory_file
{
  z64_file_t  z_file;
  uint16_t    scene_index;
  uint32_t    scene_flags[9];
  qs510_t     start_icon_dd;
  uint16_t    pause_screen;
  int16_t     item_screen_cursor;
  int16_t     quest_screen_cursor;
  int16_t     equip_screen_cursor;
  int16_t     map_screen_cursor;
  int16_t     item_screen_x;
  int16_t     equipment_screen_x;
  int16_t     item_screen_y;
  int16_t     equipment_screen_y;
  int16_t     pause_screen_cursor;
  int16_t     quest_screen_item;
  int16_t     quest_screen_hilite;
  int16_t     quest_screen_song;
};

void save_memfile(struct memory_file *memfile);
void load_memfile(struct memory_file *memfile);

#endif
