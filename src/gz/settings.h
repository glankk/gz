#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdint.h>

#define SETTINGS_WATCHES_MAX 20

struct settings_data
{
  uint16_t  font_resource_id;
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
