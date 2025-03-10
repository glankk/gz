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
  uint16_t *p = (void *)&settings->data;
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
  d->bits.input_pressrel = 0;
  d->bits.log = 1;
  d->bits.lag_counter = 0;
  d->bits.lag_unit = SETTINGS_LAG_FRAMES;
  d->bits.timer = 0;
  d->bits.pause_display = 1;
  d->bits.macro_input = 0;
  d->bits.hack_oca_input = 1;
  d->bits.hack_oca_sync = 1;
  d->bits.hack_room_load = 1;
#ifdef WIIVC
  d->bits.wiivc_cam = 1;
#else
  d->bits.wiivc_cam = 0;
#endif
  d->bits.ignore_target = 0;
  d->bits.break_type = SETTINGS_BREAK_NORMAL;
  d->bits.warp_age = 0;
  d->bits.warp_cutscene = 0;
  d->bits.col_view_mode = SETTINGS_COLVIEW_DECAL;
  d->bits.col_view_xlu = 1;
  d->bits.col_view_water = 1;
  d->bits.col_view_wfc = 0;
  d->bits.col_view_line = 0;
  d->bits.col_view_shade = 1;
  d->bits.col_view_rd = 0;
  d->bits.col_view_upd = 1;
  d->bits.hit_view_at = 1;
  d->bits.hit_view_ac = 1;
  d->bits.hit_view_oc = 1;
  d->bits.hit_view_xlu = 1;
  d->bits.hit_view_shade = 1;
  d->bits.path_view_xlu = 1;
  d->bits.path_view_points = 1;
  d->bits.path_view_lines = 1;
  d->bits.holl_view_xlu = 1;
  d->bits.holl_view_all = 0;
  d->bits.watches_visible = 1;
  d->menu_x = 20;
  d->menu_y = 64;
  d->input_display_x = 20;
  d->input_display_y = Z64_SCREEN_HEIGHT - 10;
  d->log_x = Z64_SCREEN_WIDTH - 16;
  d->log_y = Z64_SCREEN_HEIGHT - 18;
  d->lag_counter_x = Z64_SCREEN_WIDTH - 16;
  d->lag_counter_y = 24;
  d->timer_x = 20;
  d->timer_y = 24;
  d->n_watches = 0;
  d->trainer_roll_pb = 0;
  d->trainer_equip_swap_pb = 0;
  d->trainer_sidehop_pb = 0;
  d->cheats = 0;
  for (int i = 0; i < SETTINGS_TELEPORT_MAX; ++i) {
    d->teleport_pos[i].x = 0.f;
    d->teleport_pos[i].y = 0.f;
    d->teleport_pos[i].z = 0.f;
    d->teleport_rot[i] = 0;
  }
  d->teleport_slot = 0;
  d->warp_entrance = 0;
  d->binds[COMMAND_MENU] = bind_make(2, BUTTON_R, BUTTON_L);
  d->binds[COMMAND_RETURN] = bind_make(2, BUTTON_R, BUTTON_D_LEFT);
#ifndef WIIVC
  d->binds[COMMAND_BREAK] = bind_make(2, BUTTON_C_UP, BUTTON_L);
#else
  d->binds[COMMAND_BREAK] = bind_make(2, BUTTON_START, BUTTON_L);
#endif
  d->binds[COMMAND_LEVITATE] = bind_make(1, BUTTON_L);
  d->binds[COMMAND_FALL] = bind_make(2, BUTTON_Z, BUTTON_L);
  d->binds[COMMAND_TURBO] = bind_make(0);
  d->binds[COMMAND_NOCLIP] = bind_make(2, BUTTON_L, BUTTON_D_RIGHT);
  d->binds[COMMAND_FILESELECT] = bind_make(2, BUTTON_B, BUTTON_L);
  d->binds[COMMAND_RELOAD] = bind_make(2, BUTTON_A, BUTTON_L);
  d->binds[COMMAND_VOID] = bind_make(3, BUTTON_A, BUTTON_B, BUTTON_L);
  d->binds[COMMAND_AGE] = bind_make(0);
  d->binds[COMMAND_SAVESTATE] = bind_make(1, BUTTON_D_LEFT);
  d->binds[COMMAND_LOADSTATE] = bind_make(1, BUTTON_D_RIGHT);
  d->binds[COMMAND_SAVEPOS] = bind_make(0);
  d->binds[COMMAND_LOADPOS] = bind_make(0);
  d->binds[COMMAND_PREVSTATE] = bind_make(0);
  d->binds[COMMAND_NEXTSTATE] = bind_make(0);
  d->binds[COMMAND_PREVPOS] = bind_make(0);
  d->binds[COMMAND_NEXTPOS] = bind_make(0);
  d->binds[COMMAND_PAUSE] = bind_make(1, BUTTON_D_DOWN);
  d->binds[COMMAND_ADVANCE] = bind_make(1, BUTTON_D_UP);
  d->binds[COMMAND_RECORDMACRO] = bind_make(0);
  d->binds[COMMAND_PLAYMACRO] = bind_make(0);
  d->binds[COMMAND_COLVIEW] = bind_make(0);
  d->binds[COMMAND_HITVIEW] = bind_make(0);
  d->binds[COMMAND_PATHVIEW] = bind_make(0);
  d->binds[COMMAND_PREVROOM] = bind_make(2, BUTTON_R, BUTTON_D_DOWN);
  d->binds[COMMAND_NEXTROOM] = bind_make(2, BUTTON_R, BUTTON_D_UP);
  d->binds[COMMAND_RESETLAG] = bind_make(3, BUTTON_R, BUTTON_B, BUTTON_D_RIGHT);
  d->binds[COMMAND_TOGGLEWATCHES] = bind_make(2, BUTTON_R, BUTTON_D_RIGHT);
  d->binds[COMMAND_TIMER] = bind_make(3, BUTTON_R, BUTTON_A, BUTTON_D_LEFT);
  d->binds[COMMAND_RESETTIMER] = bind_make(3, BUTTON_R, BUTTON_B,
                                           BUTTON_D_LEFT);
  d->binds[COMMAND_STARTTIMER] = bind_make(0);
  d->binds[COMMAND_STOPTIMER] = bind_make(0);
  d->binds[COMMAND_RESET] = bind_make(0);
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
