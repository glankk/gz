#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "state.h"
#include "watchlist.h"
#include "z64.h"
#include "zu.h"

struct command_info command_info[COMMAND_MAX] =
{
  {"show/hide menu",    NULL,                  CMDACT_PRESS_ONCE},
  {"return from menu",  NULL,                  CMDACT_PRESS_ONCE},
  {"break free",        command_break,         CMDACT_HOLD},
  {"levitate",          command_levitate,      CMDACT_HOLD},
  {"fall",              command_fall,          CMDACT_HOLD},
  {"turbo",             command_turbo,         CMDACT_HOLD},
  {"noclip",            command_noclip,        CMDACT_PRESS_ONCE},
  {"file select",       command_fileselect,    CMDACT_PRESS_ONCE},
  {"reload scene",      command_reload,        CMDACT_PRESS_ONCE},
  {"void out",          command_void,          CMDACT_PRESS_ONCE},
  {"toggle age",        command_age,           CMDACT_PRESS_ONCE},
  {"save state",        command_savestate,     CMDACT_PRESS_ONCE},
  {"load state",        command_loadstate,     CMDACT_PRESS_ONCE},
  {"save position",     command_savepos,       CMDACT_HOLD},
  {"load position",     command_loadpos,       CMDACT_HOLD},
  {"previous state",    command_prevstate,     CMDACT_PRESS_ONCE},
  {"next state",        command_nextstate,     CMDACT_PRESS_ONCE},
  {"previous position", command_prevpos,       CMDACT_PRESS_ONCE},
  {"next position",     command_nextpos,       CMDACT_PRESS_ONCE},
  {"pause/unpause",     command_pause,         CMDACT_PRESS_ONCE},
  {"frame advance",     command_advance,       CMDACT_PRESS},
  {"record macro",      command_recordmacro,   CMDACT_PRESS_ONCE},
  {"play macro",        command_playmacro,     CMDACT_PRESS_ONCE},
  {"collision view",    command_colview,       CMDACT_PRESS_ONCE},
  {"hitbox view",       command_hitview,       CMDACT_PRESS_ONCE},
  {"path view",         command_pathview,      CMDACT_PRESS_ONCE},
  {"explore prev room", NULL,                  CMDACT_PRESS},
  {"explore next room", NULL,                  CMDACT_PRESS},
  {"reset lag counter", command_resetlag,      CMDACT_HOLD},
  {"toggle watches",    command_togglewatches, CMDACT_PRESS},
  {"start/stop timer",  command_timer,         CMDACT_PRESS_ONCE},
  {"reset timer",       command_resettimer,    CMDACT_HOLD},
  {"start timer",       command_starttimer,    CMDACT_PRESS_ONCE},
  {"stop timer",        command_stoptimer,     CMDACT_PRESS_ONCE},
  {"reset",             command_reset,         CMDACT_PRESS_ONCE},
};

void gz_apply_settings()
{
  struct gfx_font *font = resource_get(settings->bits.font_resource);
  menu_set_font(gz.menu_main, font);
  menu_set_cell_width(gz.menu_main, font->char_width + font->letter_spacing);
  menu_set_cell_height(gz.menu_main, font->char_height + font->line_spacing);
  gfx_mode_set(GFX_MODE_DROPSHADOW, settings->bits.drop_shadow);
  if (settings->bits.font_resource == RES_FONT_FIPPS)
    gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_NORMAL);
  else
    gfx_mode_configure(GFX_MODE_TEXT, GFX_TEXT_FAST);
  menu_set_pxoffset(gz.menu_main, settings->menu_x);
  menu_set_pyoffset(gz.menu_main, settings->menu_y);
  menu_imitate(gz.menu_global, gz.menu_main);
  watchlist_fetch(gz.menu_watchlist);
}

void gz_show_menu(void)
{
  menu_signal_enter(gz.menu_main, MENU_SWITCH_SHOW);
  gz.menu_active = 1;
  input_reserve(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
                BUTTON_L);
}

void gz_hide_menu(void)
{
  menu_signal_leave(gz.menu_main, MENU_SWITCH_HIDE);
  gz.menu_active = 0;
  input_free(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
             BUTTON_L);
}

void gz_log(const char *fmt, ...)
{
  struct log_entry *ent = &gz.log[SETTINGS_LOG_MAX - 1];
  if (ent->msg)
    free(ent->msg);
  for (int i = SETTINGS_LOG_MAX - 1; i > 0; --i)
    gz.log[i] = gz.log[i - 1];

  va_list va;
  va_start(va, fmt);
  int l = vsnprintf(NULL, 0, fmt, va);
  va_end(va);

  ent = &gz.log[0];
  ent->msg = malloc(l + 1);
  if (!ent->msg)
    return;
  va_start(va, fmt);
  vsprintf(ent->msg, fmt, va);
  va_end(va);
  ent->age = 0;
}

void gz_warp(int16_t entrance_index, uint16_t cutscene_index, int age)
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

void gz_set_input_mask(uint16_t pad, uint8_t x, uint8_t y)
{
  gz.z_input_mask.pad = pad;
  gz.z_input_mask.x = x;
  gz.z_input_mask.y = y;
}

void command_break(void)
{
  if (z64_game.event_flag != -1)
    z64_game.event_flag = 0x0000;
  if (z64_game.cutscene_state != 0x00)
    z64_game.cutscene_state = 0x03;
  if (z64_game.message_state_1 != 0x00) {
    z64_game.message_state_1 = 0x36;
    z64_game.message_state_2 = 0x00;
    z64_game.message_state_3 = 0x02;
  }
  if (settings->bits.break_type == SETTINGS_BREAK_AGGRESSIVE) {
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

void command_fall(void)
{
  z64_link.common.pos_1.y = -4096.f;
}

void command_turbo(void)
{
  z64_link.linear_vel = 27.f;
}

void command_noclip(void)
{
    if (!gz.noclip_on) {
      gz_noclip_start();
      gz_log("noclip on");
    }
    else {
      gz_noclip_stop();
      gz_log("noclip off");
    }
}

void command_fileselect(void)
{
  zu_execute_filemenu();
}

void command_reload(void)
{
  gz.entrance_override_once = gz.entrance_override_next;
  if (gz.next_entrance != -1)
    gz_warp(gz.next_entrance, 0x0000, 0);
  else
    gz_warp(z64_file.entrance_index, 0x0000, 0);
}

void command_void(void)
{
  z64_file.link_age = z64_game.link_age;
  zu_void();
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

void command_savestate(void)
{
  if (!zu_in_game())
    gz_log("can not save here");
  else {
    if (gz.state_buf[gz.state_slot])
      free(gz.state_buf[gz.state_slot]);
    struct state_meta *state = malloc(368 * 1024);
    state->z64_version = Z64_VERSION;
    state->state_version = SETTINGS_STATE_VERSION;
    state->scene_idx = z64_game.scene_index;
    if (gz.movie_state == MOVIE_IDLE)
      state->movie_frame = -1;
    else
      state->movie_frame = gz.movie_frame;
    state->size = save_state(state);
    gz.state_buf[gz.state_slot] = realloc(state, state->size);
    gz_log("saved state %i", gz.state_slot);
  }
}

void command_loadstate(void)
{
  if (!zu_in_game())
    gz_log("can not load here");
  else if (gz.state_buf[gz.state_slot]) {
    struct state_meta *state = gz.state_buf[gz.state_slot];
    load_state(state);
    if (gz.movie_state != MOVIE_IDLE && state->movie_frame != -1)
      gz_movie_seek(state->movie_frame);
    /* connect direct input with state's context input */
    z64_input_t *di = &z64_input_direct;
    z64_input_t *zi = &z64_ctxt.input[0];
    di->raw_prev = zi->raw;
    di->status_prev = zi->status;
    di->pad_pressed = (di->raw.pad ^ zi->raw.pad) & di->raw.pad;
    di->pad_released = (di->raw.pad ^ zi->raw.pad) & zi->raw.pad;
    di->x_diff = di->raw.x - zi->raw.x;
    di->y_diff = di->raw.y - zi->raw.y;
    gz_log("loaded state %i", gz.state_slot);
  }
  else
    gz_log("state %i is empty", gz.state_slot);
}

void command_savepos(void)
{
  uint8_t slot = settings->teleport_slot;
  settings->teleport_pos[slot] = z64_link.common.pos_2;
  settings->teleport_rot[slot] = z64_link.common.rot_2.y;
  gz_log("saved position %i", slot);
}

void command_loadpos(void)
{
  uint8_t slot = settings->teleport_slot;
  z64_link.common.pos_1 = settings->teleport_pos[slot];
  z64_link.common.pos_2 = settings->teleport_pos[slot];
  z64_link.common.rot_2.y = settings->teleport_rot[slot];
  z64_link.target_yaw = settings->teleport_rot[slot];
  gz_log("loaded position %i", slot);
}

void command_prevstate(void)
{
  gz.state_slot += SETTINGS_STATE_MAX - 1;
  gz.state_slot %= SETTINGS_STATE_MAX;
  gz_log("state %i", gz.state_slot);
}

void command_nextstate(void)
{
  gz.state_slot += 1;
  gz.state_slot %= SETTINGS_STATE_MAX;
  gz_log("state %i", gz.state_slot);
}

void command_prevpos(void)
{
  settings->teleport_slot += SETTINGS_TELEPORT_MAX - 1;
  settings->teleport_slot %= SETTINGS_TELEPORT_MAX;
  gz_log("position %i", settings->teleport_slot);
}

void command_nextpos(void)
{
  settings->teleport_slot += 1;
  settings->teleport_slot %= SETTINGS_TELEPORT_MAX;
  gz_log("position %i", settings->teleport_slot);
}

void command_pause(void)
{
  if (gz.frames_queued >= 0)
    gz.frames_queued = -1;
  else
    gz.frames_queued = 0;
}

void command_advance(void)
{
  if (gz.frames_queued >= 0)
    ++gz.frames_queued;
  else
    command_pause();
}

void command_recordmacro(void)
{
  if (gz.movie_state == MOVIE_RECORDING)
    gz.movie_state = MOVIE_IDLE;
  else
    gz.movie_state = MOVIE_RECORDING;
}

void command_playmacro(void)
{
  if (gz.movie_state == MOVIE_PLAYING)
    gz.movie_state = MOVIE_IDLE;
  else if (gz.movie_input.size > 0)
    gz.movie_state = MOVIE_PLAYING;
}

void command_colview(void)
{
  if (gz.col_view_state == COLVIEW_INACTIVE)
    gz.col_view_state = COLVIEW_START;
  else
    gz.col_view_state = COLVIEW_STOP;
}

void command_hitview(void)
{
  if (gz.hit_view_state == HITVIEW_INACTIVE)
    gz.hit_view_state = HITVIEW_START;
  else
    gz.hit_view_state = HITVIEW_STOP;
}

void command_pathview(void)
{
  if (gz.path_view_state == PATHVIEW_INACTIVE)
    gz.path_view_state = PATHVIEW_START;
  else
    gz.path_view_state = PATHVIEW_STOP;
}

void command_hollview(void)
{
  if (gz.holl_view_state == HOLLVIEW_INACTIVE)
    gz.holl_view_state = HOLLVIEW_START;
  else
    gz.holl_view_state = HOLLVIEW_BEGIN_STOP;
}

void command_resetlag(void)
{
  gz.frame_counter = 0;
  gz.lag_vi_offset = -(int32_t)__osViIntrCount;
}

void command_togglewatches(void)
{
  settings->bits.watches_visible = !settings->bits.watches_visible;
  if (settings->bits.watches_visible)
    watchlist_show(gz.menu_watchlist);
  else
    watchlist_hide(gz.menu_watchlist);
}

void command_timer(void)
{
  gz.timer_active = !gz.timer_active;
}

void command_resettimer(void)
{
  gz.timer_counter_offset = -gz.cpu_counter;
  gz.timer_counter_prev = gz.cpu_counter;
}

void command_starttimer(void)
{
  if (!gz.timer_active)
    command_timer();
}

void command_stoptimer(void)
{
  if (gz.timer_active)
    command_timer();
}

void command_reset(void)
{
  gz.reset_flag = 1;
}
