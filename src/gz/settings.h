#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdint.h>

#define SETTINGS_WATCHES_MAX  20
#define SETTINGS_LAG_FRAMES   0
#define SETTINGS_LAG_SECONDS  1

struct settings_data
{
  uint16_t  menu_font_resource_id;
  int       menu_drop_shadow;
  int16_t   menu_x;
  int16_t   menu_y;
  int       input_display_enabled;
  int16_t   input_display_x;
  int16_t   input_display_y;
  int       lag_counter_enabled;
  int8_t    lag_unit;
  int16_t   lag_counter_x;
  int16_t   lag_counter_y;
  uint16_t  no_watches;
  uint8_t   watch_type[SETTINGS_WATCHES_MAX];
  uint32_t  watch_address[SETTINGS_WATCHES_MAX];
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
  _Alignas(uint32_t)
  struct settings_data    data;
};

void  settings_load_default();
void  settings_save();
int   settings_load();

extern struct settings_data *settings;

#endif
