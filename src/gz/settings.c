#include "resource.h"
#include "settings.h"
#include "zu.h"

#define SETTINGS_ADDRESS 0x7A00
#define SETTINGS_MAXSIZE 0x0600
#define SETTINGS_VERSION 0x0000

_Static_assert(sizeof(struct settings) < SETTINGS_MAXSIZE,
               "settings data size exceeds sram capacity");

static _Alignas(16)
struct settings       settings_store;
struct settings_data *settings = &settings_store.data;

static uint16_t settings_checksum_compute()
{
  uint16_t checksum = 0;
  uint16_t *p = (void*)&settings_store.data;
  uint16_t *e = p + sizeof(settings_store.data) / sizeof(*p);
  while (p < e)
    checksum += *p++;
  return checksum;
}

static int settings_validate()
{
  return settings_store.header.version == SETTINGS_VERSION &&
         settings_store.header.data_size == sizeof(settings_store.data) &&
         settings_store.header.data_checksum == settings_checksum_compute();
}

void settings_load_default()
{
  settings_store.header.version = SETTINGS_VERSION;
  settings_store.header.data_size = sizeof(settings_store.data);
  struct settings_data *d = &settings_store.data;
  d->menu_font_resource_id = RES_FONT_PRESSSTART2P;
  d->menu_drop_shadow = 1;
  d->menu_x = 16;
  d->menu_y = 64;
  d->input_display_enabled = 0;
  d->input_display_x = 16;
  d->input_display_y = Z64_SCREEN_HEIGHT - 6;
  d->lag_counter_enabled = 0;
  d->lag_unit = SETTINGS_LAG_FRAMES;
  d->lag_counter_x = Z64_SCREEN_WIDTH - 12;
  d->lag_counter_y = 20;
  d->no_watches = 0;
}

void settings_save()
{
  settings_store.header.data_checksum = settings_checksum_compute();
  zu_sram_write(&settings_store, SETTINGS_ADDRESS, sizeof(settings_store));
}

int settings_load()
{
  zu_sram_read(&settings_store, SETTINGS_ADDRESS, sizeof(settings_store));
  return settings_validate();
}
