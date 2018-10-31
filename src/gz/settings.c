#include <stdint.h>
#include <string.h>
#include "input.h"
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
  d->bits.font_resource = RES_FONT_PRESSSTART2P;
  d->bits.drop_shadow = 1;
  d->bits.input_display = 1;
  d->bits.log = 1;
  d->bits.lag_counter = 0;
  d->bits.lag_unit = SETTINGS_LAG_FRAMES;
#ifndef WIIVC
  d->bits.timer = 0;
#endif
  d->bits.pause_display = 1;
  d->bits.macro_input = 0;
  d->bits.break_type = SETTINGS_BREAK_NORMAL;
  d->bits.warp_age = 0;
  d->bits.warp_cutscene = 0;
#ifndef WIIVC
  d->bits.load_to = SETTINGS_LOADTO_ZFILE;
  d->bits.on_load = SETTINGS_ONLOAD_NOTHING;
#endif
  d->bits.col_view_mode = SETTINGS_COLVIEW_DECAL;
  d->bits.col_view_xlu = 1;
  d->bits.col_view_line = 0;
  d->bits.col_view_shade = 1;
  d->bits.col_view_rd = 0;
  d->menu_x = 16;
  d->menu_y = 64;
  d->input_display_x = 16;
  d->input_display_y = Z64_SCREEN_HEIGHT - 6;
  d->log_x = Z64_SCREEN_WIDTH - 12;
  d->log_y = Z64_SCREEN_HEIGHT - 14;
  d->lag_counter_x = Z64_SCREEN_WIDTH - 12;
  d->lag_counter_y = 20;
#ifndef WIIVC
  d->timer_x = 16;
  d->timer_y = 20;
#endif
  d->n_watches = 0;
  d->cheats = 0;
  for (int i = 0; i < SETTINGS_TELEPORT_MAX; ++i) {
    d->teleport_pos[i].x = 0.f;
    d->teleport_pos[i].y = 0.f;
    d->teleport_pos[i].z = 0.f;
    d->teleport_rot[i] = 0;
  }
  d->teleport_slot = 0;
  d->warp_entrance = 0;
  d->binds[COMMAND_MENU] = input_bind_make(2, BUTTON_R, BUTTON_L);
  d->binds[COMMAND_RETURN] = input_bind_make(2, BUTTON_R, BUTTON_D_LEFT);
#ifndef WIIVC
  d->binds[COMMAND_BREAK] = input_bind_make(2, BUTTON_C_UP, BUTTON_L);
#else
  d->binds[COMMAND_BREAK] = input_bind_make(2, BUTTON_START, BUTTON_L);
#endif
  d->binds[COMMAND_LEVITATE] = input_bind_make(1, BUTTON_L);
  d->binds[COMMAND_FALL] = input_bind_make(2, BUTTON_Z, BUTTON_L);
  d->binds[COMMAND_TURBO] = input_bind_make(0);
  d->binds[COMMAND_FILESELECT] = input_bind_make(2, BUTTON_B, BUTTON_L);
  d->binds[COMMAND_RELOAD] = input_bind_make(2, BUTTON_A, BUTTON_L);
  d->binds[COMMAND_VOID] = input_bind_make(3, BUTTON_A, BUTTON_B, BUTTON_L);
  d->binds[COMMAND_AGE] = input_bind_make(0);
  d->binds[COMMAND_SAVESTATE] = input_bind_make(1, BUTTON_D_LEFT);
  d->binds[COMMAND_LOADSTATE] = input_bind_make(1, BUTTON_D_RIGHT);
  d->binds[COMMAND_SAVEMEMFILE] = input_bind_make(0);
  d->binds[COMMAND_LOADMEMFILE] = input_bind_make(0);
  d->binds[COMMAND_SAVEPOS] = input_bind_make(0);
  d->binds[COMMAND_LOADPOS] = input_bind_make(0);
  d->binds[COMMAND_PREVSTATE] = input_bind_make(0);
  d->binds[COMMAND_NEXTSTATE] = input_bind_make(0);
  d->binds[COMMAND_PREVFILE] = input_bind_make(0);
  d->binds[COMMAND_NEXTFILE] = input_bind_make(0);
  d->binds[COMMAND_PREVPOS] = input_bind_make(0);
  d->binds[COMMAND_NEXTPOS] = input_bind_make(0);
  d->binds[COMMAND_PAUSE] = input_bind_make(1, BUTTON_D_DOWN);
  d->binds[COMMAND_ADVANCE] = input_bind_make(1, BUTTON_D_UP);
  d->binds[COMMAND_RECORDMACRO] = input_bind_make(0);
  d->binds[COMMAND_PLAYMACRO] = input_bind_make(0);
  d->binds[COMMAND_COLVIEW] = input_bind_make(0);
  d->binds[COMMAND_PREVROOM] = input_bind_make(2, BUTTON_R, BUTTON_D_DOWN);
  d->binds[COMMAND_NEXTROOM] = input_bind_make(2, BUTTON_R, BUTTON_D_UP);
  d->binds[COMMAND_RESETLAG] = input_bind_make(3, BUTTON_R, BUTTON_B,
                                               BUTTON_D_RIGHT);
#ifndef WIIVC
  d->binds[COMMAND_TIMER] = input_bind_make(3, BUTTON_R, BUTTON_A,
                                            BUTTON_D_LEFT);
  d->binds[COMMAND_RESETTIMER] = input_bind_make(3, BUTTON_R, BUTTON_B,
                                                 BUTTON_D_LEFT);
  d->binds[COMMAND_STARTTIMER] = input_bind_make(0);
  d->binds[COMMAND_STOPTIMER] = input_bind_make(0);
  d->binds[COMMAND_RESET] = input_bind_make(0);
#endif
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
