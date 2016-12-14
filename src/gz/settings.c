#include <stdint.h>
#include <string.h>
#include "resource.h"
#include "settings.h"
#include "zu.h"

_Static_assert(SETTINGS_PROFILE_MAX != 0,
               "settings data size exceeds sram capacity");

static _Alignas(16)
struct settings       settings_store;
struct settings_data *settings = &settings_store.data;

static uint16_t settings_checksum_compute(struct settings *settings)
{
  uint16_t checksum = 0;
  uint16_t *p = (void*)&settings->data;
  uint16_t *e = p + sizeof(settings->data) / sizeof(*p);
  while (p < e)
    checksum += *p++;
  return checksum;
}

static _Bool settings_validate(struct settings *settings)
{
  return settings->header.version == SETTINGS_VERSION &&
         settings->header.data_size == sizeof(settings->data) &&
         settings->header.data_checksum == settings_checksum_compute(settings);
}

void settings_load_default(void)
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
  for (int i = 0; i < CHEAT_MAX; ++i)
    d->cheats[i] = 0;
  for (int i = 0; i < SETTINGS_TELEPORT_MAX; ++i) {
    d->teleport_pos[i].x = 0.f;
    d->teleport_pos[i].y = 0.f;
    d->teleport_pos[i].z = 0.f;
    d->teleport_rot[i] = 0;
  }
  d->teleport_slot = 0;
  d->warp_entrance = 0;
  d->warp_age = 0;
  d->command_binds[COMMAND_MENU] = BUTTON_R | BUTTON_L;
  d->command_binds[COMMAND_RETURN] = BUTTON_R | BUTTON_D_LEFT;
  d->command_binds[COMMAND_CLEARCSP] = BUTTON_L | BUTTON_C_UP;
  d->command_binds[COMMAND_VOID] = BUTTON_A | BUTTON_B | BUTTON_L;
  d->command_binds[COMMAND_RELOAD] = BUTTON_A | BUTTON_L;
  d->command_binds[COMMAND_FILESELECT] = BUTTON_B | BUTTON_L;
  d->command_binds[COMMAND_LEVITATE] = BUTTON_L;
  d->command_binds[COMMAND_TURBO] = 0;
  d->command_binds[COMMAND_SAVEPOS] = BUTTON_D_LEFT;
  d->command_binds[COMMAND_LOADPOS] = BUTTON_D_RIGHT;
  d->command_binds[COMMAND_SAVEMEMFILE] = BUTTON_R | BUTTON_D_LEFT;
  d->command_binds[COMMAND_LOADMEMFILE] = BUTTON_R | BUTTON_D_RIGHT;
  d->command_binds[COMMAND_RESETLAG] = BUTTON_R | BUTTON_A | BUTTON_D_RIGHT;
  d->command_binds[COMMAND_PAUSE] = BUTTON_D_DOWN;
  d->command_binds[COMMAND_ADVANCE] = BUTTON_D_UP;
}

void settings_save(int profile)
{
  uint16_t *checksum = &settings_store.header.data_checksum;
  *checksum = settings_checksum_compute(&settings_store);
  zu_sram_write(&settings_store, SETTINGS_ADDRESS + SETTINGS_PADSIZE * profile,
                sizeof(settings_store));
}

_Bool settings_load(int profile)
{
  _Alignas(16)
  struct settings settings_temp;
  zu_sram_read(&settings_temp, SETTINGS_ADDRESS + SETTINGS_PADSIZE * profile,
               sizeof(settings_temp));
  if (settings_validate(&settings_temp)) {
    memcpy(&settings_store, &settings_temp, sizeof(settings_temp));
    return 1;
  }
  else
    return 0;
}
