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
  d->font_resource_id = RES_FONT_PRESSSTART2P;
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
