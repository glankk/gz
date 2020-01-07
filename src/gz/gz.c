#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <startup.h>
#include <mips.h>
#include <n64.h>
#include <vector/vector.h>
#include "explorer.h"
#include "geometry.h"
#include "gfx.h"
#include "gu.h"
#include "gz.h"
#include "input.h"
#include "io.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "start.h"
#include "util.h"
#include "watchlist.h"
#include "z64.h"
#include "zu.h"

__attribute__((section(".data")))
struct gz gz =
{
  .ready = 0,
};

static void update_cpu_counter(void)
{
  static uint32_t count = 0;
  uint32_t new_count = clock_ticks();
  gz.cpu_counter_freq = clock_freq();
  gz.cpu_counter += new_count - count;
  count = new_count;
}

static void main_hook(void)
{
  update_cpu_counter();
  input_update();
  gfx_mode_init();

  {
    /* handle emergency settings reset */
    uint16_t pad_pressed = input_pressed();
    if (pad_pressed) {
      static const uint16_t input_list[] =
      {
        BUTTON_D_UP,
        BUTTON_D_UP,
        BUTTON_D_DOWN,
        BUTTON_D_DOWN,
        BUTTON_D_LEFT,
        BUTTON_D_RIGHT,
        BUTTON_D_LEFT,
        BUTTON_D_RIGHT,
        BUTTON_B,
        BUTTON_A,
      };
      static int input_pos = 0;
      size_t input_list_length = sizeof(input_list) / sizeof(*input_list);
      if (pad_pressed == input_list[input_pos]) {
        ++input_pos;
        if (input_pos == input_list_length) {
          input_pos = 0;
          settings_load_default();
          gz_apply_settings();
        }
      }
      else
        input_pos = 0;
    }
  }

  /* handle menu input */
  if (gz.menu_active) {
    if (input_bind_pressed_raw(COMMAND_MENU))
      gz_hide_menu();
    else if (input_bind_pressed(COMMAND_RETURN))
      menu_return(gz.menu_main);
    else {
      uint16_t pad_pressed = input_pressed();
      if (pad_pressed & BUTTON_D_UP)
        menu_navigate(gz.menu_main, MENU_NAVIGATE_UP);
      if (pad_pressed & BUTTON_D_DOWN)
        menu_navigate(gz.menu_main, MENU_NAVIGATE_DOWN);
      if (pad_pressed & BUTTON_D_LEFT)
        menu_navigate(gz.menu_main, MENU_NAVIGATE_LEFT);
      if (pad_pressed & BUTTON_D_RIGHT)
        menu_navigate(gz.menu_main, MENU_NAVIGATE_RIGHT);
      if (pad_pressed & BUTTON_L)
        menu_activate(gz.menu_main);
    }
  }
  else if (input_bind_pressed_raw(COMMAND_MENU))
    gz_show_menu();

  /* apply cheats */
  if (settings->cheats & (1 << CHEAT_ENERGY))
    z64_file.energy = z64_file.energy_capacity;
  if (settings->cheats & (1 << CHEAT_MAGIC))
    z64_file.magic = (z64_file.magic_capacity + 1) * 0x30;
  if (settings->cheats & (1 << CHEAT_STICKS)) {
    int stick_capacity[] = {1, 10, 20, 30, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_STICK] = stick_capacity[z64_file.stick_upgrade];
  }
  if (settings->cheats & (1 << CHEAT_NUTS)) {
    int nut_capacity[] = {1, 20, 30, 40, 1, 0x7F, 1, 0x7F};
    z64_file.ammo[Z64_SLOT_NUT] = nut_capacity[z64_file.nut_upgrade];
  }
  if (settings->cheats & (1 << CHEAT_BOMBS)) {
    int bomb_bag_capacity[] = {1, 20, 30, 40, 1, 1, 1, 1};
    z64_file.ammo[Z64_SLOT_BOMB] = bomb_bag_capacity[z64_file.bomb_bag];
  }
  if (settings->cheats & (1 << CHEAT_ARROWS)) {
    int quiver_capacity[] = {1, 30, 40, 50, 1, 20, 30, 40};
    z64_file.ammo[Z64_SLOT_BOW] = quiver_capacity[z64_file.quiver];
  }
  if (settings->cheats & (1 << CHEAT_SEEDS)) {
    int bullet_bag_capacity[] = {1, 30, 40, 50, 1, 10, 20, 30};
    z64_file.ammo[Z64_SLOT_SLINGSHOT] =
      bullet_bag_capacity[z64_file.bullet_bag];
  }
  if (settings->cheats & (1 << CHEAT_BOMBCHUS))
    z64_file.ammo[Z64_SLOT_BOMBCHU] = 50;
  if (settings->cheats & (1 << CHEAT_BEANS))
    z64_file.ammo[Z64_SLOT_BEANS] = 1;
  if (settings->cheats & (1 << CHEAT_KEYS)) {
    if (z64_game.scene_index >= 0x0000 && z64_game.scene_index <= 0x0010)
      z64_file.dungeon_keys[z64_game.scene_index] = 1;
  }
  if (settings->cheats & (1 << CHEAT_RUPEES)) {
    int wallet_capacity[] = {99, 200, 500, 0xFFFF};
    z64_file.rupees = wallet_capacity[z64_file.wallet];
  }
  if (settings->cheats & (1 << CHEAT_NL))
    z64_file.nayrus_love_timer = 0x044B;
  if (settings->cheats & (1 << CHEAT_FREEZETIME)) {
    uint16_t time_advance_1 = gz.day_time_prev + z64_day_speed;
    uint16_t time_advance_2 = gz.day_time_prev + z64_day_speed * 2;
    if (gz.target_day_time == -1 && z64_day_speed < 0x0190 &&
        (z64_file.day_time == time_advance_1 ||
         z64_file.day_time == time_advance_2))
    {
      z64_file.day_time = gz.day_time_prev;
    }
  }
  if (settings->cheats & (1 << CHEAT_NOMUSIC)) {
    zu_audio_cmd(0x100000FF);
    zu_audio_cmd(0x110000FF);
    zu_audio_cmd(0x130000FF);
    z64_file.seq_index = -1;
    z64_file.night_sfx = -1;
  }
  if (settings->cheats & (1 << CHEAT_USEITEMS)) {
    memset(&z64_game.if_ctxt.restriction_flags, 0,
           sizeof(z64_game.if_ctxt.restriction_flags));
  }
  if (settings->cheats & (1 << CHEAT_NOMAP))
    z64_file.gameinfo->minimap_disabled = 1;
  if (settings->cheats & (1 << CHEAT_ISG))
    z64_link.sword_state = 1;
  if (settings->cheats & (1 << CHEAT_QUICKTEXT))
    *(uint8_t *)(&z64_message_state[0x000C]) = 0x01;
  if (settings->cheats & (1 << CHEAT_NOHUD))
      z64_file.hud_flag = 0x001;

  /* handle commands */
  for (int i = 0; i < COMMAND_MAX; ++i) {
    _Bool active = 0;
    switch (command_info[i].activation_type) {
      case CMDACT_HOLD:       active = input_bind_held(i);        break;
      case CMDACT_PRESS:      active = input_bind_pressed(i);     break;
      case CMDACT_PRESS_ONCE: active = input_bind_pressed_raw(i); break;
    }
    if (command_info[i].proc && active)
      command_info[i].proc();
  }
  if (input_bind_pressed(COMMAND_PREVROOM))
    explorer_room_prev(gz.menu_explorer);
  if (input_bind_pressed(COMMAND_NEXTROOM))
    explorer_room_next(gz.menu_explorer);

  /* animate menus */
  while (gz.menu_active && menu_think(gz.menu_main))
    ;
  while (menu_think(gz.menu_global))
    ;

  /* update daytime after menu processing to avoid desync */
  if (gz.target_day_time != -1) {
    const uint16_t speed = 0x0800;
    if (z64_file.day_time < gz.target_day_time &&
        gz.target_day_time - z64_file.day_time <= speed)
    {
      z64_file.day_time = gz.target_day_time;
      gz.target_day_time = -1;
    }
    else
      z64_file.day_time += speed;
  }
  gz.day_time_prev = z64_file.day_time;

  /* save gfx pointer offsets for frame advance display list copy */
  zu_save_disp_p(&gz.z_disp_p);

  /* get menu appearance */
  struct gfx_font *font = menu_get_font(gz.menu_main, 1);
  uint8_t alpha = menu_get_alpha_i(gz.menu_main, 1);
  int cw = menu_get_cell_width(gz.menu_main, 1);
  int ch = menu_get_cell_height(gz.menu_main, 1);

  /* draw pause/movie display */
  if (settings->bits.pause_display) {
    if (gz.movie_state == MOVIE_RECORDING) {
      struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
      struct gfx_sprite sprite =
      {
        t, gz.frames_queued == -1 ? 3 : gz.frames_queued == 0 ? 4 : 5,
        32, 32, 1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_sprite_draw(&sprite);
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
      gfx_printf(font, 32, 48 + ch, "%i / %i",
                 gz.movie_frame, gz.movie_input.size);
    }
    else if (gz.movie_state == MOVIE_PLAYING) {
      struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
      struct gfx_sprite sprite =
      {
        t, gz.frames_queued == -1 ? 0 : gz.frames_queued == 0 ? 1 : 2,
        32, 32, 1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_sprite_draw(&sprite);
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
      gfx_printf(font, 32, 48 + ch, "%i / %i",
                 gz.movie_frame, gz.movie_input.size);
    }
    else if (gz.frames_queued != -1) {
      struct gfx_texture *t = resource_get(RES_ICON_PAUSE);
      struct gfx_sprite sprite =
      {
        t, gz.frames_queued == 0 ? 1 : 2,
        32, 32, 1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
      gfx_sprite_draw(&sprite);
    }
  }

  /* draw input display */
  if (settings->bits.input_display) {
    int d_x;
    int d_y;
    uint16_t d_pad;
    uint16_t d_pressed;
    uint16_t d_released;
    if (gz.movie_state == MOVIE_PLAYING &&
        gz.movie_frame < gz.movie_input.size)
    {
      z64_input_t zi;
      _Bool reset_flag;
      movie_to_z(gz.movie_frame, &zi, &reset_flag);
      d_x = zi.raw.x;
      d_y = zi.raw.y;
      d_pad = zi.raw.pad;
      d_pressed = zi.pad_pressed;
      d_released = zi.pad_released;
      if (!settings->bits.input_pressrel)
        d_pad |= d_pressed;
      if (reset_flag)
        d_pad |= 0x0080;
      if (settings->bits.macro_input) {
        if (abs(d_x) < 8)
          d_x = input_x();
        if (abs(d_y) < 8)
          d_y = input_y();
        d_pad |= input_pad();
      }
      uint16_t mask = BUTTON_L | BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT |
                      BUTTON_D_RIGHT;
      d_pad &= ~mask;
      d_pressed &= ~mask;
      d_released &= ~mask;
    }
    else {
      d_x = input_x();
      d_y = input_y();
      d_pad = input_pad();
      if (gz.frames_queued == 0) {
        d_pressed = z64_input_direct.pad_pressed;
        d_released = z64_input_direct.pad_released;
      }
      else {
        d_pressed = input_pressed_raw();
        d_released = input_released();
      }
      if (gz.reset_flag)
        d_pad |= 0x0080;
    }
    struct gfx_texture *texture = resource_get(RES_ICON_BUTTONS);
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    gfx_printf(font, settings->input_display_x, settings->input_display_y,
               "%4i %4i", d_x, d_y);
    static const int buttons[] =
    {
      15, 14, 12, 3, 2, 1, 0, 13, 5, 4, 11, 10, 9, 8, 7,
    };
    for (int i = 0; i < sizeof(buttons) / sizeof(*buttons); ++i) {
      int b = buttons[i];
      int bit = 1 << b;
      int b_alpha;
      if (settings->bits.input_pressrel &&
          ((d_pressed | d_released) & ~d_pad & bit))
        b_alpha = 0x7F;
      else if (d_pad & bit)
        b_alpha = 0xFF;
      else
        continue;
      int x = (cw - texture->tile_width) / 2 + i * 10;
      int y = -(gfx_font_xheight(font) + texture->tile_height + 1) / 2;
      struct gfx_sprite sprite =
      {
        texture, b,
        settings->input_display_x + cw * 10 + x, settings->input_display_y + y,
        1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(input_button_color[b],
                                                 b_alpha * alpha / 0xFF));
      gfx_sprite_draw(&sprite);
      if (settings->bits.input_pressrel && (d_pressed & bit)) {
        struct gfx_sprite arrow_sprite =
        {
          texture, 16, sprite.x - 1, sprite.y - 1, 1.f, 1.f,
        };
        gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(0x00C000, alpha));
        gfx_sprite_draw(&arrow_sprite);
      }
      if (settings->bits.input_pressrel && (d_released & bit)) {
        struct gfx_sprite arrow_sprite =
        {
          texture, 16, sprite.x + 9, sprite.y + 9, -1.f, -1.f,
        };
        gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(0xC00000, alpha));
        gfx_sprite_draw(&arrow_sprite);
      }
    }
  }

  /* execute and draw lag counter */
  if (settings->bits.lag_counter) {
    int32_t lag_frames = (int32_t)__osViIntrCount +
                         gz.lag_vi_offset - gz.frame_counter;
    int x = settings->lag_counter_x - cw * 8;
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    if (settings->bits.lag_unit == SETTINGS_LAG_FRAMES)
      gfx_printf(font, x , settings->lag_counter_y, "%8d", lag_frames);
    else if (settings->bits.lag_unit == SETTINGS_LAG_SECONDS)
      gfx_printf(font, x, settings->lag_counter_y, "%8.2f", lag_frames / 60.f);
  }
  gz.frame_counter += z64_file.gameinfo->update_rate;

  /* execute and draw timer */
  if (!gz.timer_active)
    gz.timer_counter_offset -= gz.cpu_counter - gz.timer_counter_prev;
  gz.timer_counter_prev = gz.cpu_counter;
  if (settings->bits.timer) {
    int64_t count = gz.cpu_counter + gz.timer_counter_offset;
    int tenths = count * 10 / gz.cpu_counter_freq;
    int seconds = tenths / 10;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    tenths %= 10;
    seconds %= 60;
    minutes %= 60;
    int x = settings->timer_x;
    int y = settings->timer_y;
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    if (hours > 0)
      gfx_printf(font, x, y, "%d:%02d:%02d.%d",
                 hours, minutes, seconds, tenths);
    else if (minutes > 0)
      gfx_printf(font, x, y, "%d:%02d.%d", minutes, seconds, tenths);
    else
      gfx_printf(font, x, y, "%d.%d", seconds, tenths);
  }

  /* draw menus */
  if (gz.menu_active)
    menu_draw(gz.menu_main);
  menu_draw(gz.menu_global);

  /* execute and draw collision view */
  gz_col_view();
  gz_hit_view();
  gz_cull_view();
  gz_path_view();
  gz_holl_view();
  gz_guard_view();

  /* execute free camera in view mode */
  gz_free_view();

  {
    /* draw splash */
#define STRINGIFY(S)  STRINGIFY_(S)
#define STRINGIFY_(S) #S
    static int splash_time = 230;
    static struct gfx_texture *logo_texture;
    if (splash_time > 0) {
      --splash_time;
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      const char *tarname = STRINGIFY(PACKAGE_TARNAME);
      const char *url = STRINGIFY(PACKAGE_URL);
      const char *version = STRINGIFY(PACKAGE_VERSION);
      gfx_printf(font, 20, Z64_SCREEN_HEIGHT - 12 - ch * 2, tarname);
      gfx_printf(font, 20, Z64_SCREEN_HEIGHT - 10 - ch, version);
      gfx_printf(font, Z64_SCREEN_WIDTH - 16 - cw * strlen(url),
                 Z64_SCREEN_HEIGHT - 12 - ch * 2, url);
      if (!logo_texture)
        logo_texture = resource_load_grc_texture("logo");
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, alpha));
      for (int i = 0; i < logo_texture->tiles_y; ++i) {
        struct gfx_sprite logo_sprite =
        {
          logo_texture, i,
          Z64_SCREEN_WIDTH - 16 - logo_texture->tile_width,
          Z64_SCREEN_HEIGHT - 12 - ch * 3 -
          (logo_texture->tiles_y - i) * logo_texture->tile_height,
          1.f, 1.f,
        };
        gfx_sprite_draw(&logo_sprite);
      }
    }
    else if (splash_time == 0) {
      --splash_time;
      gfx_texture_free(logo_texture);
    }
#undef STRINGIFY
#undef STRINGIFY_
  }

  /* draw log */
  for (int i = SETTINGS_LOG_MAX - 1; i >= 0; --i) {
    const int fade_begin = 20;
    const int fade_duration = 20;
    struct log_entry *ent = &gz.log[i];
    uint8_t msg_alpha;
    if (!ent->msg)
      continue;
    ++ent->age;
    if (ent->age > (fade_begin + fade_duration)) {
      free(ent->msg);
      ent->msg = NULL;
      continue;
    }
    else if (!settings->bits.log)
      continue;
    else if (ent->age > fade_begin)
      msg_alpha = 0xFF - (ent->age - fade_begin) * 0xFF / fade_duration;
    else
      msg_alpha = 0xFF;
    msg_alpha = msg_alpha * alpha / 0xFF;
    int msg_x = settings->log_x - cw * strlen(ent->msg);
    int msg_y = settings->log_y - ch * i;
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(0xC0C0C0, msg_alpha));
    gfx_printf(font, msg_x, msg_y, "%s", ent->msg);
  }

  /* finish frame */
  gfx_flush();
}

static void macro_oom(void)
{
    gz_log("out of memory, stopping macro");
    gz.frames_queued = 0;
    gz.movie_state = MOVIE_IDLE;
}

HOOK int32_t room_load_sync_hook(OSMesgQueue *mq, OSMesg *msg, int32_t flag)
{
  maybe_init_gp();
  if (!gz.ready || gz.movie_state == MOVIE_IDLE) {
    /* default behavior */
    return osRecvMesg(mq, msg, flag);
  }
  /* if recording, use sync hack setting to decide whether or not to block */
  else if (gz.movie_state == MOVIE_RECORDING) {
    if (settings->bits.hack_room_load)
      flag = OS_MESG_BLOCK;
    int result = osRecvMesg(mq, msg, flag);
    /* record a load frame if the result differs from the hack value */
    if (result == -1) {
      struct movie_room_load *rl;
      rl = vector_at(&gz.movie_room_load, gz.movie_room_load_pos);
      if (!rl || rl->frame_idx != gz.movie_frame) {
        rl = vector_insert(&gz.movie_room_load,
                           gz.movie_room_load_pos, 1, NULL);
      }
      if (rl) {
        rl->frame_idx = gz.movie_frame;
        ++gz.movie_room_load_pos;
      }
      else
        macro_oom();
      gz.room_load_flag = 1;
    }
    return result;
  }
  /* if in playback, lag if there is a record of this frame, otherwise block */
  else {
    struct movie_room_load *rl;
    rl = vector_at(&gz.movie_room_load, gz.movie_room_load_pos);
    if (rl && rl->frame_idx == gz.movie_frame) {
      ++gz.movie_room_load_pos;
      return -1;
    }
    else
      return osRecvMesg(mq, msg, OS_MESG_BLOCK);
  }
}

HOOK void entrance_offset_hook(void)
{
  maybe_init_gp();
  uint32_t offset;
  if (!gz.ready)
    offset = z64_file.scene_setup_index;
  else {
    if (z64_file.void_flag && z64_file.cutscene_index == 0x0000)
      gz.entrance_override_once = gz.entrance_override_next;
    if (gz.entrance_override_once) {
      offset = 0;
      gz.entrance_override_once = 0;
      gz.entrance_override_next = 1;
    }
    else {
      offset = z64_file.scene_setup_index;
      gz.entrance_override_next = 0;
    }
    gz.next_entrance = -1;
  }
  __asm__ (".set    push;"
           ".set    noat;"
           "lw      $v1, %0;"
           "la      $at, %1;"
           ".set    pop;" :: "m"(offset), "i"(0x51) : "v1", "at");
}

HOOK void draw_room_hook(z64_game_t *game, z64_room_t *room, int unk_a2)
{
  maybe_init_gp();
  if (gz.ready && gz.hide_rooms) {
    struct zu_disp_p disp_p;
    zu_save_disp_p(&disp_p);
    z64_DrawRoom(game, room, unk_a2);
    zu_load_disp_p(&disp_p);
  }
  else
    z64_DrawRoom(game, room, unk_a2);
}

HOOK void draw_actors_hook(z64_game_t *game, void *actor_ctxt)
{
  maybe_init_gp();
  if (gz.ready && gz.hide_actors) {
    struct zu_disp_p disp_p;
    zu_save_disp_p(&disp_p);
    z64_DrawActors(game, actor_ctxt);
    zu_load_disp_p(&disp_p);
  }
  else
    z64_DrawActors(game, actor_ctxt);
}

static void mask_input(z64_input_t *input)
{
  z64_controller_t *mask = &gz.z_input_mask;
  input->pad_pressed &= ~mask->pad;
  input->pad_released |= (input->raw.pad & mask->pad);
  input->raw.pad &= ~mask->pad;
  input->raw.x &= ~mask->x;
  input->raw.y &= ~mask->y;
  input->adjusted_x = zu_adjust_joystick(input->raw.x);
  input->adjusted_y = zu_adjust_joystick(input->raw.y);
}

HOOK void input_hook(void)
{
  maybe_init_gp();
  if (!gz.ready)
    z64_UpdateCtxtInput(&z64_ctxt);
  else if (gz.frames_queued != 0) {
    z64_input_t di = z64_input_direct;
    z64_input_t *zi = z64_ctxt.input;
    {
      z64_controller_t raw_save[4];
      uint16_t status_save[4];
      for (int i = 0; i < 4; ++i)
        if (gz.vcont_enabled[i]) {
          /* save raw and status to get correct raw_prev and status_prev */
          raw_save[i] = zi[i].raw;
          status_save[i] = zi[i].status;
        }
      z64_UpdateCtxtInput(&z64_ctxt);
      mask_input(&zi[0]);
      for (int i = 0; i < 4; ++i)
        if (gz.vcont_enabled[i]) {
          zi[i].raw = raw_save[i];
          zi[i].status = status_save[i];
          gz_vcont_get(i, &zi[i]);
        }
    }
    if (gz.movie_state == MOVIE_RECORDING) {
      /* clear rerecords for empty movies */
      if (gz.movie_frame == 0 && gz.movie_input.size == 0) {
        gz.movie_last_recorded_frame = -1;
        gz.movie_rerecords = 0;
      }
      if (gz.movie_frame >= gz.movie_input.size) {
        if (gz.movie_input.size == gz.movie_input.capacity)
          vector_reserve(&gz.movie_input, 128);
        vector_push_back(&gz.movie_input, 1, NULL);
      }
      if (gz.movie_frame < gz.movie_input.size) {
        /* if the last recorded frame is not the previous frame,
           increment the rerecord count */
        if (gz.movie_last_recorded_frame >= gz.movie_frame)
          ++gz.movie_rerecords;
        gz.movie_last_recorded_frame = gz.movie_frame++;
        z_to_movie(gz.movie_last_recorded_frame, &zi[0], gz.reset_flag);
      }
      else
        macro_oom();
    }
    else if (gz.movie_state == MOVIE_PLAYING) {
      if (gz.movie_frame >= gz.movie_input.size) {
        if (input_bind_held(COMMAND_PLAYMACRO) && gz.movie_input.size > 0)
          gz_movie_rewind();
        else
          gz.movie_state = MOVIE_IDLE;
      }
      if (gz.movie_state == MOVIE_PLAYING) {
        _Bool reset;
        movie_to_z(gz.movie_frame++, &zi[0], &reset);
        if (settings->bits.macro_input) {
          gz.reset_flag |= reset;
          if (abs(zi[0].raw.x) < 8) {
            zi[0].raw.x = di.raw.x;
            zi[0].x_diff = di.x_diff;
            zi[0].adjusted_x = di.adjusted_x;
          }
          if (abs(zi[0].raw.y) < 8) {
            zi[0].raw.y = di.raw.y;
            zi[0].y_diff = di.y_diff;
            zi[0].adjusted_y = di.adjusted_y;
          }
          zi[0].raw.pad |= di.raw.pad;
          zi[0].pad_pressed |= di.pad_pressed;
          zi[0].pad_released |= di.pad_released;
        }
        else
          gz.reset_flag = reset;
      }
    }
  }
}

HOOK void disp_hook(z64_disp_buf_t *disp_buf, Gfx *buf, uint32_t size)
{
  maybe_init_gp();
  if (gz.ready) {
    z64_disp_buf_t *z_disp[4] =
    {
      &z64_ctxt.gfx->work,
      &z64_ctxt.gfx->poly_opa,
      &z64_ctxt.gfx->poly_xlu,
      &z64_ctxt.gfx->overlay,
    };
    for (int i = 0; i < 4; ++i) {
      if (disp_buf == z_disp[i]) {
        gz.disp_hook_size[i] = disp_buf->size;
        gz.disp_hook_p[i] = (char *)disp_buf->p - (char *)disp_buf->buf;
        gz.disp_hook_d[i] = (char *)disp_buf->d - (char *)disp_buf->buf;
        break;
      }
    }
  }
  disp_buf->size = size;
  disp_buf->buf = buf;
  disp_buf->p = buf;
  disp_buf->d = (void *)((char *)buf + size);
}

static void state_main_hook(void)
{
  if (gz.frames_queued != 0) {
    if (gz.frames_queued > 0)
      --gz.frames_queued;
    /* reset sync event flags */
    gz.oca_input_flag = 0;
    gz.oca_sync_flag = 0;
    gz.room_load_flag = 0;
    /* execute state */
    gz_leave_func = z64_ctxt.state_main;
    gz_leave(&z64_ctxt);
    /* if recording over a previous seed, erase it */
    if (gz.movie_state == MOVIE_RECORDING) {
      struct movie_seed *ms;
      ms = vector_at(&gz.movie_seed, gz.movie_seed_pos);
      if (ms && ms->frame_idx == gz.movie_frame)
        vector_erase(&gz.movie_seed, gz.movie_seed_pos, 1);
    }
    /* ditto for room load */
    if (gz.movie_state == MOVIE_RECORDING && !gz.room_load_flag) {
      struct movie_room_load *rl;
      rl = vector_at(&gz.movie_room_load, gz.movie_room_load_pos);
      if (rl && rl->frame_idx == gz.movie_frame)
        vector_erase(&gz.movie_room_load, gz.movie_room_load_pos, 1);
    }
    /* set frame flag to execute an ocarina frame */
    gz.frame_flag = 1;
    /* execute a scheduled reset */
    if (gz.reset_flag) {
      gz.reset_flag = 0;
      gz.lag_vi_offset += (int32_t)__osViIntrCount - gz.frame_counter;
      gz.frame_counter = 0;
      cpu_reset();
    }
  }
  else {
    z64_gfx_t *gfx = z64_ctxt.gfx;
    if (z64_ctxt.state_frames != 0) {
      /* copy gfx buffer from previous frame */
      if (gfx->frame_count_1 & 1) {
        memcpy((void *)&z64_disp[z64_disp_size],
               (void *)&z64_disp[0], z64_disp_size);
      }
      else {
        memcpy((void *)&z64_disp[0],
               (void *)&z64_disp[z64_disp_size], z64_disp_size);
      }
      /* set pointers */
      zu_load_disp_p(&gz.z_disp_p);
      /* relocate */
      zu_reloc_gfx(1 - (gfx->frame_count_1 & 1), 1 - (gfx->frame_count_2 & 1));
      /* isn't that just beautiful */
    }
    else {
      /* non-frame, clear screen */
      gDPSetColorImage(gfx->poly_opa.p++,
                       G_IM_FMT_RGBA, G_IM_SIZ_16b, Z64_SCREEN_WIDTH,
                       ZU_MAKE_SEG(Z64_SEG_CIMG, 0));
      gDPSetCycleType(gfx->poly_opa.p++, G_CYC_FILL);
      gDPSetRenderMode(gfx->poly_opa.p++, G_RM_NOOP, G_RM_NOOP2);
      gDPSetFillColor(gfx->poly_opa.p++,
                      (GPACK_RGBA5551(0x00, 0x00, 0x00, 0x01) << 16) |
                      GPACK_RGBA5551(0x00, 0x00, 0x00, 0x01));
      gDPFillRectangle(gfx->poly_opa.p++,
                       0, 0, Z64_SCREEN_WIDTH - 1, Z64_SCREEN_HEIGHT - 1);
      gDPPipeSync(gfx->poly_opa.p++);
    }
    /* undo frame counter increment */
    --z64_ctxt.state_frames;
    /* do not execute an ocarina frame */
    gz.frame_flag = 0;
  }
}

HOOK void srand_hook(uint32_t seed)
{
  maybe_init_gp();
  if (gz.ready) {
    if (gz.movie_state == MOVIE_RECORDING) {
      /* insert a recorded seed */
      struct movie_seed *ms;
      ms = vector_insert(&gz.movie_seed, gz.movie_seed_pos, 1, NULL);
      if (ms) {
        ms->frame_idx = gz.movie_frame;
        ms->old_seed = z64_random;
        ms->new_seed = seed;
        ++gz.movie_seed_pos;
      }
      else
        macro_oom();
    }
    else if (gz.movie_state == MOVIE_PLAYING) {
      /* restore a recorded seed, if conditions match */
      struct movie_seed *ms = vector_at(&gz.movie_seed, gz.movie_seed_pos);
#ifdef DEBUG_SRAND
      if (ms) {
        ++gz.movie_seed_pos;
        if (ms->frame_idx != gz.movie_frame) {
          gz_log("srand frame_idx desync;");
          gz_log("  have  %08i", gz.movie_frame);
          gz_log("  want  %08i", ms->frame_idx);
        }
        if (ms->old_seed != z64_random) {
          gz_log("srand old_seed desync;");
          gz_log("  have  %08x", z64_random);
          gz_log("  want  %08x", ms->old_seed);
        }
        if (ms->frame_idx == gz.movie_frame && ms->old_seed == z64_random)
          gz_log("srand sync");
        ms->frame_idx = gz.movie_frame;
        ms->old_seed = z64_random;
        z64_random = ms->new_seed;
        return;
      }
#else
      if (ms && ms->frame_idx == gz.movie_frame) {
        ++gz.movie_seed_pos;
        if (ms->old_seed == z64_random) {
          z64_random = ms->new_seed;
          return;
        }
        else
          gz_log("rng desync detected");
      }
#endif
    }
  }
  z64_SeedRandom(seed);
}

HOOK void ocarina_update_hook(void)
{
  maybe_init_gp();
  if (!gz.ready)
    z64_OcarinaUpdate();
  else if (gz.frame_flag) {
    /* do ocarina sync */
    int audio_frames;
    /* sync hack by default when frame advancing or the song will softlock */
    if (gz.frames_queued >= 0)
      audio_frames = z64_file.gameinfo->update_rate;
    else
      audio_frames = z64_afx_counter - z64_ocarina_counter;
    /* if recording, use the sync hack setting to decide the value to use */
    if (gz.movie_state == MOVIE_RECORDING) {
      /** note: when loading a state where the saved frame flag is 1 (i.e. the
          game was running and not paused with frame advance when the state was
          saved), and ocarina input/sync hacks are disabled, ocarina input and
          sync data for the loaded state frame will be overwritten with the
          current actual sync info (which will be lots of frames due to
          savestate load lag) and real controller input immediately after
          loading a state (regardless of being paused or not). this seems to be
          unavoidable due to how states are currently implemented. **/
      if (settings->bits.hack_oca_sync)
        audio_frames = z64_file.gameinfo->update_rate;
      /* record the value if it differs from the sync hack */
      if (audio_frames != z64_file.gameinfo->update_rate) {
        struct movie_oca_sync *os;
        os = vector_at(&gz.movie_oca_sync, gz.movie_oca_sync_pos);
        if (!os || os->frame_idx != gz.movie_frame) {
          os = vector_insert(&gz.movie_oca_sync,
                             gz.movie_oca_sync_pos, 1, NULL);
        }
        if (os) {
          os->frame_idx = gz.movie_frame;
          os->audio_frames = audio_frames;
          ++gz.movie_oca_sync_pos;
        }
        else
          macro_oom();
        gz.oca_sync_flag = 1;
      }
    }
    /* if in playback, use a recorded value, or sync hack if there is none */
    else if (gz.movie_state == MOVIE_PLAYING) {
      struct movie_oca_sync *os;
      os = vector_at(&gz.movie_oca_sync, gz.movie_oca_sync_pos);
      if (os && os->frame_idx == gz.movie_frame) {
        audio_frames = os->audio_frames;
        ++gz.movie_oca_sync_pos;
      }
      else
        audio_frames = z64_file.gameinfo->update_rate;
    }
    /* update audio counters */
    {
      z64_ocarina_counter += audio_frames;
      uint32_t play_frames = z64_ocarina_counter - z64_song_play_counter;
      uint32_t rec_frames = z64_ocarina_counter - z64_song_rec_counter;
      z64_ocarina_counter = z64_afx_counter;
      z64_song_play_counter = z64_ocarina_counter - play_frames;
      z64_song_rec_counter = z64_ocarina_counter - rec_frames;
    }
    /* advance ocarina minigame metronome timer */
    if (gz.metronome_timer > 0)
      gz.metronome_timer--;
    /* execute ocarina frame */
    z64_OcarinaUpdate();
    /* if recording over sync events, remove them */
    if (gz.movie_state == MOVIE_RECORDING && !gz.oca_input_flag) {
      /* ocarina inputs can happen multiple times per frame */
      while (1) {
        struct movie_oca_input *oi;
        oi = vector_at(&gz.movie_oca_input, gz.movie_oca_input_pos);
        if (oi && oi->frame_idx == gz.movie_frame)
          vector_erase(&gz.movie_oca_input, gz.movie_oca_input_pos, 1);
        else
          break;
      }
    }
    if (gz.movie_state == MOVIE_RECORDING && !gz.oca_sync_flag) {
      struct movie_oca_sync *os;
      os = vector_at(&gz.movie_oca_sync, gz.movie_oca_sync_pos);
      if (os && os->frame_idx == gz.movie_frame)
        vector_erase(&gz.movie_oca_sync, gz.movie_oca_sync_pos, 1);
    }
  }
  else {
    /* update audio counters to avoid desync when resuming */
    uint32_t play_frames = z64_ocarina_counter - z64_song_play_counter;
    uint32_t rec_frames = z64_ocarina_counter - z64_song_rec_counter;
    z64_ocarina_counter = z64_afx_counter;
    z64_song_play_counter = z64_ocarina_counter - play_frames;
    z64_song_rec_counter = z64_ocarina_counter - rec_frames;
  }
}

HOOK void ocarina_input_hook(void *a0, z64_input_t *input, int a2)
{
  maybe_init_gp();
  z64_GetInput(a0, input, a2);
  if (gz.ready)
    mask_input(input);
  if (gz.ready && gz.movie_state != MOVIE_IDLE && gz.movie_frame > 0) {
    /* if recording, use the sync hack setting to decide the input to use */
    if (gz.movie_state == MOVIE_RECORDING) {
      /** see note in ocarina_update_hook **/
      if (settings->bits.hack_oca_input)
        movie_to_z(gz.movie_frame - 1, input, NULL);
      else {
        /* record ocarina input */
        struct movie_oca_input *oi;
        oi = vector_at(&gz.movie_oca_input, gz.movie_oca_input_pos);
        if (!oi || oi->frame_idx != gz.movie_frame) {
          oi = vector_insert(&gz.movie_oca_input,
                             gz.movie_oca_input_pos, 1, NULL);
        }
        if (oi) {
          oi->frame_idx = gz.movie_frame;
          oi->pad = input->raw.pad;
          oi->adjusted_x = input->adjusted_x;
          oi->adjusted_y = input->adjusted_y;
          ++gz.movie_oca_input_pos;
        }
        else
          macro_oom();
        gz.oca_input_flag = 1;
      }
    }
    /* if in playback, use a recorded value, or sync hack if there is none */
    else {
      struct movie_oca_input *oi;
      oi = vector_at(&gz.movie_oca_input, gz.movie_oca_input_pos);
      if (oi && oi->frame_idx == gz.movie_frame) {
        input->raw.pad = oi->pad;
        input->adjusted_x = oi->adjusted_x;
        input->adjusted_y = oi->adjusted_y;
        ++gz.movie_oca_input_pos;
      }
      else {
        /* ocarina inputs happen after the movie counter has been advanced,
           so use the previous movie frame */
        movie_to_z(gz.movie_frame - 1, input, NULL);
      }
    }
  }
}

HOOK uint32_t afx_rand_hook(void)
{
  maybe_init_gp();
  if (!gz.ready || gz.movie_state == MOVIE_IDLE) {
    /* produce a number using the audio rng, as normal */
    return z64_AfxRand();
  }
  else {
    /* produce a number that is deterministic within gz movies */
    int n = z64_ocarina_song_length + 1;
    uint32_t v = z64_random;
    for (int i = 0; i < n; ++i)
      v = v * 0x0019660D + 0x3C6EF35F;
    return v;
  }
}

HOOK void guPerspectiveF_hook(MtxF *mf)
{
  /* replaces the guMtxIdentF function in guPerspectiveF */
  maybe_init_gp();
  if (gz.ready && settings->bits.wiivc_cam) {
    /* overwrite the scale argument in guPerspectiveF */
    /* this assumes that mf is at 0($sp) on entry, which should be true */
    __asm__ ("la      $t0, 0x3F800000;"
             "sw      $t0, 0x0048 + %0;"
             :: "R"(mf) : "t0");
  }
  mf->xx = 1.f;
  mf->xy = 0.f;
  mf->xz = 0.f;
  mf->xw = 0.f;
  mf->yx = 0.f;
  mf->yy = 1.f;
  mf->yz = 0.f;
  mf->yw = 0.f;
  mf->zx = 0.f;
  mf->zy = 0.f;
  mf->zz = 1.f;
  mf->zw = 0.f;
  mf->wx = 0.f;
  mf->wy = 0.f;
  mf->wz = 0.f;
  mf->ww = 1.f;
}

HOOK void camera_hook(void *camera)
{
  maybe_init_gp();

  void (*camera_func)(void *camera);
  __asm__ ("sw      $t9, %[camera_func]"
           : [camera_func] "=m"(camera_func));

  if (!gz.ready || !gz.free_cam || gz.cam_mode != CAMMODE_CAMERA)
    return camera_func(camera);

  gz_update_cam();

  z64_xyzf_t *camera_at = (void *)((char *)camera + 0x0050);
  z64_xyzf_t *camera_eye = (void *)((char *)camera + 0x005C);

  *camera_eye = gz.cam_pos;

  z64_xyzf_t vf;
  vec3f_py(&vf, gz.cam_pitch, gz.cam_yaw);
  vec3f_add(camera_at, camera_eye, &vf);
}

HOOK void metronome_start_hook(uint16_t sfx_id, z64_xyzf_t *pos, uint8_t token,
                               float *freq_scale, float *vol,
                               int8_t *reverb_add)
{
  maybe_init_gp();

  if (gz.ready)
    gz.metronome_timer = 17;

  return z64_Audio_PlaySfxGeneral(sfx_id, pos, token, freq_scale, vol,
                                  reverb_add);
}

HOOK uint8_t metronome_check_hook(uint32_t sfx_id)
{
  maybe_init_gp();

  /* use a sync hack when playing, recording, or frame advancing */
  if (gz.ready && (gz.movie_state != MOVIE_IDLE || gz.frames_queued >= 0))
    return gz.metronome_timer > 0;
  else
    return z64_Audio_IsSfxPlaying(sfx_id);
}

/* Replaces call to Actor_UpdateBgCheckInfo in EnBomChu_WaitForRelease to
   simulate GC double-explosion behavior, probably caused by the GC emulator
   reading 0s when the floor poly is NULL. */
HOOK void bombchu_floor_poly_hook(z64_game_t *game, z64_actor_t *actor,
                                  float wall_check_height,
                                  float wall_check_radius,
                                  float ceiling_check_height, int32_t flags)
{
  maybe_init_gp();
  z64_Actor_UpdateBgCheckInfo(game, actor, wall_check_height,
                              wall_check_radius, ceiling_check_height, flags);
  if (settings->bits.gc_oob_chu && actor->floor_poly == NULL) {
    static z64_col_poly_t zero_poly = { 0 };
    actor->floor_poly = &zero_poly;
  }
}

static void main_return_proc(struct menu_item *item, void *data)
{
  gz_hide_menu();
}

static void init(void)
{
  /* startup */
  clear_bss();
  do_global_ctors();

  /* initialize gz variables */
  gz.profile = 0;
  gz.menu_active = 0;
  for (int i = 0; i < SETTINGS_LOG_MAX; ++i)
    gz.log[i].msg = NULL;
  gz.selected_actor.ptr = NULL;
  gz.entrance_override_once = 0;
  gz.entrance_override_next = 0;
  gz.next_entrance = -1;
  gz.day_time_prev = z64_file.day_time;
  gz.target_day_time = -1;
  gz.frames_queued = -1;
  gz.movie_state = MOVIE_IDLE;
  vector_init(&gz.movie_input, sizeof(struct movie_input));
  vector_init(&gz.movie_seed, sizeof(struct movie_seed));
  vector_init(&gz.movie_oca_input, sizeof(struct movie_oca_input));
  vector_init(&gz.movie_oca_sync, sizeof(struct movie_oca_sync));
  vector_init(&gz.movie_room_load, sizeof(struct movie_room_load));
  gz.movie_frame = 0;
  gz.movie_seed_pos = 0;
  gz.movie_oca_input_pos = 0;
  gz.movie_oca_sync_pos = 0;
  gz.movie_room_load_pos = 0;
  gz.z_input_mask.pad = BUTTON_L | BUTTON_D_RIGHT | BUTTON_D_LEFT |
                        BUTTON_D_DOWN | BUTTON_D_UP;
  gz.z_input_mask.x = 0;
  gz.z_input_mask.y = 0;
  for (int i = 0; i < 4; ++i) {
    gz.vcont_enabled[i] = 0;
    gz_vcont_set(i, 0, NULL);
  }
  gz.frame_counter = 0;
  gz.lag_vi_offset = -(int32_t)__osViIntrCount;
  gz.cpu_counter = 0;
  update_cpu_counter();
  gz.timer_active = 0;
  gz.timer_counter_offset = -gz.cpu_counter;
  gz.timer_counter_prev = gz.cpu_counter;
  gz.col_view_state = COLVIEW_INACTIVE;
  gz.hit_view_state = HITVIEW_INACTIVE;
  gz.cull_view_state = CULLVIEW_INACTIVE;
  gz.path_view_state = PATHVIEW_INACTIVE;
  gz.holl_view_state = HOLLVIEW_INACTIVE;
  gz.noclip_on = 0;
  gz.guard_view_state = GUARDVIEW_INACTIVE;
  gz.hide_rooms = 0;
  gz.hide_actors = 0;
  gz.free_cam = 0;
  gz.lock_cam = 0;
  gz.cam_mode = CAMMODE_CAMERA;
  gz.cam_bhv = CAMBHV_MANUAL;
  gz.cam_dist_min = 100;
  gz.cam_dist_max = 400;
  gz.cam_yaw = 0.f;
  gz.cam_pitch = 0.f;
  gz.cam_pos.x = 0.f;
  gz.cam_pos.y = 0.f;
  gz.cam_pos.z = 0.f;
  for (int i = 0; i < SETTINGS_STATE_MAX; ++i)
    gz.state_buf[i] = NULL;
  gz.state_slot = 0;
  gz.reset_flag = 0;

  /* initialize io device */
  io_init();

  /* load settings */
  if (input_z_pad() == BUTTON_START || !settings_load(gz.profile))
    settings_load_default();
  input_update();

  /* initialize gfx */
  gfx_start();
  gfx_mode_configure(GFX_MODE_FILTER, G_TF_POINT);
  gfx_mode_configure(GFX_MODE_COMBINE, G_CC_MODE(G_CC_MODULATEIA_PRIM,
                                                 G_CC_MODULATEIA_PRIM));

  /* create menus */
  {
    static struct menu menu;
    static struct menu watches;
    static struct menu global;

    /* initialize menus */
    menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_init(&watches, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    menu_init(&global, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    gz.menu_main = &menu;
    gz.menu_global = &global;
    gz.menu_watches = &watches;

    /* populate top menu */
    menu.selector = menu_add_button(&menu, 0, 0, "return",
                                    main_return_proc, NULL);
    menu_add_submenu(&menu, 0, 1, gz_warps_menu(), "warps");
    menu_add_submenu(&menu, 0, 2, gz_scene_menu(), "scene");
    menu_add_submenu(&menu, 0, 3, gz_cheats_menu(), "cheats");
    menu_add_submenu(&menu, 0, 4, gz_inventory_menu(), "inventory");
    menu_add_submenu(&menu, 0, 5, gz_equips_menu(), "equips");
    menu_add_submenu(&menu, 0, 6, gz_file_menu(), "file");
    menu_add_submenu(&menu, 0, 7, gz_macro_menu(), "macro");
    menu_add_submenu(&menu, 0, 8, &watches, "watches");
    menu_add_submenu(&menu, 0, 9, gz_debug_menu(), "debug");
    menu_add_submenu(&menu, 0, 10, gz_settings_menu(), "settings");

    /* populate watches menu */
    watches.selector = menu_add_submenu(&watches, 0, 0, NULL, "return");
    gz.menu_watchlist = watchlist_create(&watches, &global, 0, 1);

    /* configure menu related commands */
    input_bind_set_override(COMMAND_MENU, 1);
    input_bind_set_override(COMMAND_RETURN, 1);
    input_bind_set_override(COMMAND_PREVROOM, 1);
    input_bind_set_override(COMMAND_NEXTROOM, 1);
    input_bind_set_disable(COMMAND_PREVROOM, 1);
    input_bind_set_disable(COMMAND_NEXTROOM, 1);
  }

  /* reflect loaded settings */
  gz_apply_settings();

  gz.ready = 1;
}

int main()
{
  maybe_init_gp();
  if (!gz.ready)
    init();
  state_main_hook();
  main_hook();
}

/* support libraries */
#include <startup.c>
#include <set/set.c>
#include <vector/vector.c>
#include <list/list.c>
#include <grc.c>
