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
#include "hb.h"
#include "input.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "util.h"
#include "watchlist.h"
#include "z64.h"
#include "zu.h"

__attribute__((section(".data")))
struct gz gz =
{
  .ready = 0,
};

static __attribute__((section(".sdata")))
void *stack_ptr;

static inline void init_stack(void)
{
  static __attribute__((section(".stack"))) _Alignas(8)
  char _stack[0x2000];
  static __attribute__((section(".sbss")))
  void *t0_save;
  __asm__ volatile ("sw      $t0, %[t0_save];"
                    "la      $t0, %[stack_top];"
                    "sw      $t0, %[stack_ptr];"
                    "lw      $t0, %[t0_save];"
                    : [stack_ptr] "=m"(stack_ptr), [t0_save] "=m"(t0_save)
                    : [stack_top] "i"(&_stack[sizeof(_stack) - 0x10]));
}

static inline void xchg_stack(void)
{
  static __attribute__((section(".sbss")))
  void *t0_save;
  __asm__ volatile ("sw      $t0, %[t0_save];"
                    "lw      $t0, %[stack_ptr];"
                    "sw      $sp, %[stack_ptr];"
                    "move    $sp, $t0;"
                    "lw      $t0, %[t0_save];"
                    : [stack_ptr] "+m"(stack_ptr), [t0_save] "=m"(t0_save));
}

static void update_cpu_counter(void)
{
  static uint32_t count = 0;
  uint32_t new_count;
  if (hb_get_timebase(NULL, &new_count) == 0)
    gz.cpu_counter_freq = HB_TIMEBASE_FREQ;
  else {
    __asm__ volatile ("mfc0    $t0, $9;"
                      "nop;"
                      "sw      $t0, %[new_count];"
                      : [new_count] "=m"(new_count) :: "t0");
    gz.cpu_counter_freq = OS_CPU_COUNTER;
  }
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
    *(uint8_t*)(&z64_message_state[0x000C]) = 0x01;

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
    if (gz.movie_state == MOVIE_PLAYING &&
        gz.movie_frame < gz.movie_input.size)
    {
      z64_input_t zi;
      movie_to_z(gz.movie_frame, &zi, NULL);
      d_x = zi.raw.x;
      d_y = zi.raw.y;
      d_pad = zi.raw.pad;
      d_pad |= zi.pad_pressed;
      if (settings->bits.macro_input) {
        if (abs(d_x) < 8)
          d_x = input_x();
        if (abs(d_y) < 8)
          d_y = input_y();
        d_pad |= input_pad();
      }
      d_pad &= ~BUTTON_L;
      d_pad &= ~BUTTON_D_UP;
      d_pad &= ~BUTTON_D_DOWN;
      d_pad &= ~BUTTON_D_LEFT;
      d_pad &= ~BUTTON_D_RIGHT;
    }
    else {
      d_x = input_x();
      d_y = input_y();
      d_pad = input_pad();
    }
    struct gfx_texture *texture = resource_get(RES_ICON_BUTTONS);
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0xC0, 0xC0, alpha));
    gfx_printf(font, settings->input_display_x, settings->input_display_y,
               "%4i %4i", d_x, d_y);
    static const int buttons[] =
    {
      15, 14, 12, 3, 2, 1, 0, 13, 5, 4, 11, 10, 9, 8,
    };
    for (int i = 0; i < sizeof(buttons) / sizeof(*buttons); ++i) {
      int b = buttons[i];
      if (!(d_pad & (1 << b)))
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
                                                 alpha));
      gfx_sprite_draw(&sprite);
    }
  }

  /* execute and draw lag counter */
  if (settings->bits.lag_counter) {
    int32_t lag_frames = (int32_t)z64_vi_counter +
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

HOOK int32_t room_load_sync_hook(OSMesgQueue *mq, OSMesg *msg, int32_t flag)
{
  maybe_init_gp();
  if (!gz.ready || gz.movie_state == MOVIE_IDLE) {
    /* default behavior */
    return z64_osRecvMesg(mq, msg, flag);
  }
  /* if recording, use sync hack setting to decide whether or not to block */
  else if (gz.movie_state == MOVIE_RECORDING) {
    if (settings->bits.hack_room_load)
      flag = OS_MESG_BLOCK;
    int result = z64_osRecvMesg(mq, msg, flag);
    /* record a load frame if the result differs from the hack value */
    if (result == -1) {
      struct movie_room_load *rl;
      rl = vector_at(&gz.movie_room_load, gz.movie_room_load_pos);
      if (!rl || rl->frame_idx != gz.movie_frame) {
        rl = vector_insert(&gz.movie_room_load,
                           gz.movie_room_load_pos, 1, NULL);
      }
      rl->frame_idx = gz.movie_frame;
      ++gz.movie_room_load_pos;
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
      return z64_osRecvMesg(mq, msg, OS_MESG_BLOCK);
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
  __asm__ volatile (".set  noat;"
                    "lw    $v1, %0;"
                    "la    $at, %1;" :: "m"(offset), "i"(0x51) : "v1", "at");
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
      if (gz.movie_frame >= gz.movie_input.size) {
        if (gz.movie_input.size == gz.movie_input.capacity)
          vector_reserve(&gz.movie_input, 128);
        vector_push_back(&gz.movie_input, 1, NULL);
      }
      z_to_movie(gz.movie_frame++, &zi[0], gz.reset_flag);
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
        gz.disp_hook_p[i] = (char*)disp_buf->p - (char*)disp_buf->buf;
        gz.disp_hook_d[i] = (char*)disp_buf->d - (char*)disp_buf->buf;
        break;
      }
    }
  }
  disp_buf->size = size;
  disp_buf->buf = buf;
  disp_buf->p = buf;
  disp_buf->d = (void*)((char*)buf + size);
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
    {
      register void *a0 __asm__ ("a0") = &z64_ctxt;
      register void *t9 __asm__ ("t9") = z64_ctxt.state_main;
      xchg_stack();
      __asm__ volatile ("jalr    %[state_main];"
                        : [state_main] "+r"(t9), "+r"(a0)
                        :
                        : "at", "v0", "v1",       "a1", "a2", "a3", "t0", "t1",
                          "t2", "t3", "t4", "t5", "t6", "t7", "t8",       "ra",
                          "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
                          "f9", "f10", "f11", "f12", "f13", "f14", "f15", "f16",
                          "f17", "f18", "f19", "cc", "memory");
      xchg_stack();
    }
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
      gz.lag_vi_offset += (int32_t)z64_vi_counter - gz.frame_counter;
      gz.frame_counter = 0;
      /* try doing a homeboy reset */
      if (hb_check() == 0) {
        /* simulate 0.5s nmi delay */
        uint64_t tb;
        uint64_t tb_wait;
        hb_get_timebase64(&tb);
        tb_wait = tb + HB_TIMEBASE_FREQ / 2;
        while (tb < tb_wait)
          hb_get_timebase64(&tb);
        hb_reset(0x00400000, 0x00400000);
      }
      else {
        /* no homeboy interface, do a normal reset */
        zu_reset();
      }
    }
  }
  else {
    z64_gfx_t *gfx = z64_ctxt.gfx;
    if (z64_ctxt.state_frames != 0) {
      /* copy gfx buffer from previous frame */
      if (gfx->frame_count_1 & 1) {
        memcpy((void*)(&z64_disp[z64_disp_size]),
               (void*)(&z64_disp[0]), z64_disp_size);
      }
      else {
        memcpy((void*)(&z64_disp[0]),
               (void*)(&z64_disp[z64_disp_size]), z64_disp_size);
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
      ms->frame_idx = gz.movie_frame;
      ms->old_seed = z64_random;
      ms->new_seed = seed;
      ++gz.movie_seed_pos;
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
    /* update audio counters manually to avoid desync when resuming */
    z64_song_counter = z64_afx_counter;
    z64_ocarina_counter = z64_song_counter;
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
        oi->frame_idx = gz.movie_frame;
        oi->pad = input->raw.pad;
        oi->adjusted_x = input->adjusted_x;
        oi->adjusted_y = input->adjusted_y;
        ++gz.movie_oca_input_pos;
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

HOOK void ocarina_sync_hook(void)
{
  maybe_init_gp();
  /* the hook is placed in the middle of a function where there is not normally
     a function call, so some registers need to be saved
  */
  struct
  {
    uint32_t v1;
    uint32_t a0;
    uint32_t a3;
    uint32_t t0;
    uint32_t t1;
    uint32_t t6;
  } regs;
  __asm__ volatile ("la      $v0, %[regs];"
                    "sw      $v1, 0x0000($v0);"
                    "sw      $a3, 0x0008($v0);"
                    "sw      $t0, 0x000C($v0);"
                    "sw      $t1, 0x0010($v0);"
                    "sw      $t6, 0x0014($v0);"
                    : [regs] "=m"(regs) :: "v0");
  int audio_frames;
  /* default behavior */
  if (regs.t6)
    audio_frames = z64_song_counter - z64_ocarina_counter;
  else
    audio_frames = 3;
  /* gz override */
  if (gz.ready) {
    /* sync hack by default when frame advancing or the song will softlock */
    if (gz.frames_queued >= 0)
      audio_frames = 3;
    /* if recording, use the sync hack setting to decide the value to use */
    if (gz.movie_state == MOVIE_RECORDING) {
      if (settings->bits.hack_oca_sync)
        audio_frames = 3;
      /* record the value if it differs from the sync hack */
      if (audio_frames != 3) {
        struct movie_oca_sync *os;
        os = vector_at(&gz.movie_oca_sync, gz.movie_oca_sync_pos);
        if (!os || os->frame_idx != gz.movie_frame) {
          os = vector_insert(&gz.movie_oca_sync,
                             gz.movie_oca_sync_pos, 1, NULL);
        }
        os->frame_idx = gz.movie_frame;
        os->audio_frames = audio_frames;
        ++gz.movie_oca_sync_pos;
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
        audio_frames = 3;
    }
  }
  regs.a0 = audio_frames;
  __asm__ volatile ("la      $v0, %[regs];"
                    "lw      $v1, 0x0000($v0);"
                    "lw      $a0, 0x0004($v0);"
                    "lw      $a3, 0x0008($v0);"
                    "lw      $t0, 0x000C($v0);"
                    "lw      $t1, 0x0010($v0);"
                    :: [regs] "m"(regs) : "v0", "v1", "a0", "a3", "t0", "t1");
}

HOOK uint32_t afx_rand_hook(void)
{
  maybe_init_gp();
  if (!gz.ready || gz.movie_state == MOVIE_IDLE) {
    /* produce a number using the audio rng, as normal */
    uint32_t (*z64_afx_rand_func)(void) = (void*)&z64_afx_rand_func;
    return z64_afx_rand_func();
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
    __asm__ volatile ("la      $t0, 0x3F800000;"
                      "sw      $t0, 0x0048($sp);"
                      ::: "t0");
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
  void (*camera_func)(void *camera);
  __asm__ volatile ("sw      $t9, %[camera_func]"
                    : [camera_func] "=m"(camera_func));

  if (!gz.ready || !gz.free_cam || gz.cam_mode != CAMMODE_CAMERA)
    return camera_func(camera);

  gz_update_cam();

  z64_xyzf_t *camera_at = (void*)((char*)camera + 0x0050);
  z64_xyzf_t *camera_eye = (void*)((char*)camera + 0x005C);

  *camera_eye = gz.cam_pos;

  z64_xyzf_t vf;
  vec3f_py(&vf, gz.cam_pitch, gz.cam_yaw);
  vec3f_add(camera_at, camera_eye, &vf);
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
  gz.z_input_mask.pad = 0;
  gz.z_input_mask.x = 0;
  gz.z_input_mask.y = 0;
  for (int i = 0; i < 4; ++i) {
    gz.vcont_enabled[i] = 0;
    gz_vcont_set(i, 0, NULL);
  }
  gz.frame_counter = 0;
  gz.lag_vi_offset = -(int32_t)z64_vi_counter;
  gz.cpu_counter = 0;
  update_cpu_counter();
  gz.timer_active = 0;
  gz.timer_counter_offset = -gz.cpu_counter;
  gz.timer_counter_prev = gz.cpu_counter;
  gz.col_view_state = COLVIEW_INACTIVE;
  gz.hit_view_state = HITVIEW_INACTIVE;
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
  gz.memfile = malloc(sizeof(*gz.memfile) * SETTINGS_MEMFILE_MAX);
  for (int i = 0; i < SETTINGS_MEMFILE_MAX; ++i)
    gz.memfile_saved[i] = 0;
  gz.memfile_slot = 0;
  for (int i = 0; i < SETTINGS_STATE_MAX; ++i)
    gz.state_buf[i] = NULL;
  gz.state_slot = 0;
  gz.reset_flag = 0;

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
  if (!gz.ready)
    init();
  state_main_hook();
  main_hook();
}

ENTRY void _start()
{
  static __attribute__((section(".sdata")))
  void *ra_save;
  maybe_init_gp();
  init_stack();
  xchg_stack();
  __asm__ volatile ("sw      $ra, %[ra_save];"
                    "jal     %[main];"
                    "lw      $ra, %[ra_save];"
                    : [ra_save] "=m"(ra_save)
                    : [main] "i"(main)
                    : "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1",
                      "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
                      "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
                      "f9", "f10", "f11", "f12", "f13", "f14", "f15", "f16",
                      "f17", "f18", "f19", "cc", "memory");
  xchg_stack();
}

/* support libraries */
#include <startup.c>
#include <set/set.c>
#include <vector/vector.c>
#include <list/list.c>
#include <grc.c>
