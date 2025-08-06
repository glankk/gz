#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "files.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "state.h"
#include "sys.h"
#include "z64.h"
#include "zu.h"

static _Bool            vcont_plugged[4];
static z64_controller_t vcont_raw[4];

static int pause_switch_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, gz.frames_queued != -1);
  else if (reason == MENU_CALLBACK_CHANGED)
    command_pause();
  return 0;
}

static int advance_switch_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    command_advance();
  return 0;
}

static int record_switch_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, gz.movie_state == MOVIE_RECORDING);
  else if (reason == MENU_CALLBACK_CHANGED)
    command_recordmacro();
  return 0;
}

static int play_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, gz.movie_state == MOVIE_PLAYING);
  else if (reason == MENU_CALLBACK_CHANGED) {
    if (gz.movie_state != MOVIE_PLAYING &&
        gz.movie_frame == gz.movie_input.size)
    {
      gz_movie_rewind();
    }
    command_playmacro();
  }
  return 0;
}

static int rewind_switch_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    gz_movie_rewind();
  return 0;
}

static int trim_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_CHANGED) {
    vector_erase(&gz.movie_input, gz.movie_frame,
                 gz.movie_input.size - gz.movie_frame);
    vector_erase(&gz.movie_seed, gz.movie_seed_pos,
                 gz.movie_seed.size - gz.movie_seed_pos);
    vector_erase(&gz.movie_oca_input, gz.movie_oca_input_pos,
                 gz.movie_oca_input.size - gz.movie_oca_input_pos);
    vector_erase(&gz.movie_oca_sync, gz.movie_oca_sync_pos,
                 gz.movie_oca_sync.size - gz.movie_oca_sync_pos);
    vector_erase(&gz.movie_room_load, gz.movie_room_load_pos,
                 gz.movie_room_load.size - gz.movie_room_load_pos);
    vector_shrink_to_fit(&gz.movie_input);
    vector_shrink_to_fit(&gz.movie_seed);
    vector_shrink_to_fit(&gz.movie_oca_input);
    vector_shrink_to_fit(&gz.movie_oca_sync);
    vector_shrink_to_fit(&gz.movie_room_load);
  }
  return 0;
}

static int movie_pos_proc(struct menu_item *item,
                          enum menu_callback_reason reason,
                          void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != gz.movie_frame)
      menu_intinput_set(item, gz.movie_frame);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    gz_movie_seek(menu_intinput_get(item));
  return 0;
}

static int do_import_macro(const char *path, void *data)
{
  const char *s_eof = "unexpected end of file";
  const char *s_memory = "out of memory";
  const char *err_str = NULL;
  int f = open(path, O_RDONLY);
  if (f != -1) {
    struct stat st;
    fstat(f, &st);
    int n;
    size_t n_input;
    size_t n_seed;
    errno = 0;
    n = sizeof(n_input);
    if (read(f, &n_input, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    n = sizeof(n_seed);
    if (read(f, &n_seed, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    vector_clear(&gz.movie_input);
    vector_clear(&gz.movie_seed);
    if (!vector_reserve(&gz.movie_input, n_input) ||
        !vector_reserve(&gz.movie_seed, n_seed))
    {
      err_str = s_memory;
      goto error;
    }
    gz_movie_rewind();
    vector_insert(&gz.movie_input, 0, n_input, NULL);
    vector_insert(&gz.movie_seed, 0, n_seed, NULL);
    vector_shrink_to_fit(&gz.movie_input);
    vector_shrink_to_fit(&gz.movie_seed);
    n = sizeof(gz.movie_input_start);
    if (read(f, &gz.movie_input_start, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    n = gz.movie_input.element_size * n_input;
    if (read(f, gz.movie_input.begin, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    n = gz.movie_seed.element_size * n_seed;
    if (read(f, gz.movie_seed.begin, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    /* read sync info if it exists */
    if (lseek(f, 0, SEEK_CUR) < st.st_size) {
      size_t n_oca_input;
      size_t n_oca_sync;
      size_t n_room_load;
      n = sizeof(n_oca_input);
      if (read(f, &n_oca_input, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
      n = sizeof(n_oca_sync);
      if (read(f, &n_oca_sync, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
      n = sizeof(n_room_load);
      if (read(f, &n_room_load, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
      vector_clear(&gz.movie_oca_input);
      vector_clear(&gz.movie_oca_sync);
      vector_clear(&gz.movie_room_load);
      if (!vector_reserve(&gz.movie_oca_input, n_oca_input) ||
          !vector_reserve(&gz.movie_oca_sync, n_oca_sync) ||
          !vector_reserve(&gz.movie_room_load, n_room_load))
      {
        err_str = s_memory;
        goto error;
      }
      vector_insert(&gz.movie_oca_input, 0, n_oca_input, NULL);
      vector_insert(&gz.movie_oca_sync, 0, n_oca_sync, NULL);
      vector_insert(&gz.movie_room_load, 0, n_room_load, NULL);
      vector_shrink_to_fit(&gz.movie_oca_input);
      vector_shrink_to_fit(&gz.movie_oca_sync);
      vector_shrink_to_fit(&gz.movie_room_load);
      n = gz.movie_oca_input.element_size * n_oca_input;
      if (read(f, gz.movie_oca_input.begin, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
      n = gz.movie_oca_sync.element_size * n_oca_sync;
      if (read(f, gz.movie_oca_sync.begin, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
      n = gz.movie_room_load.element_size * n_room_load;
      if (read(f, gz.movie_room_load.begin, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
    }
    else {
      vector_clear(&gz.movie_oca_input);
      vector_clear(&gz.movie_oca_sync);
      vector_clear(&gz.movie_room_load);
      vector_shrink_to_fit(&gz.movie_oca_input);
      vector_shrink_to_fit(&gz.movie_oca_sync);
      vector_shrink_to_fit(&gz.movie_room_load);
    }
    /* read rerecords info if it exists */
    if (lseek(f, 0, SEEK_CUR) < st.st_size) {
      n = sizeof(gz.movie_rerecords);
      if (read(f, &gz.movie_rerecords, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
      n = sizeof(gz.movie_last_recorded_frame);
      if (read(f, &gz.movie_last_recorded_frame, n) != n) {
        err_str = s_eof;
        goto f_err;
      }
    }
f_err:
    if (errno != 0)
      err_str = strerror(errno);
  }
  else
    err_str = strerror(errno);
error:
  if (f != -1)
    close(f);
  if (err_str) {
    menu_prompt(gz.menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else
    return 0;
}

static int do_export_macro(const char *path, void *data)
{
  const char *err_str = NULL;
  int f = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (f != -1) {
    int n;
    size_t n_input = gz.movie_input.size;
    size_t n_seed = gz.movie_seed.size;
    errno = 0;
    n = sizeof(n_input);
    if (write(f, &n_input, n) != n)
      goto f_err;
    n = sizeof(n_seed);
    if (write(f, &n_seed, n) != n)
      goto f_err;
    n = sizeof(gz.movie_input_start);
    if (write(f, &gz.movie_input_start, n) != n)
      goto f_err;
    n = gz.movie_input.element_size * n_input;
    if (write(f, gz.movie_input.begin, n) != n)
      goto f_err;
    n = gz.movie_seed.element_size * n_seed;
    if (write(f, gz.movie_seed.begin, n) != n)
      goto f_err;
    size_t n_oca_input = gz.movie_oca_input.size;
    size_t n_oca_sync = gz.movie_oca_sync.size;
    size_t n_room_load = gz.movie_room_load.size;
    /* write sync info */
    n = sizeof(n_oca_input);
    if (write(f, &n_oca_input, n) != n)
      goto f_err;
    n = sizeof(n_oca_sync);
    if (write(f, &n_oca_sync, n) != n)
      goto f_err;
    n = sizeof(n_room_load);
    if (write(f, &n_room_load, n) != n)
      goto f_err;
    n = gz.movie_oca_input.element_size * n_oca_input;
    if (write(f, gz.movie_oca_input.begin, n) != n)
      goto f_err;
    n = gz.movie_oca_sync.element_size * n_oca_sync;
    if (write(f, gz.movie_oca_sync.begin, n) != n)
      goto f_err;
    n = gz.movie_room_load.element_size * n_room_load;
    if (write(f, gz.movie_room_load.begin, n) != n)
      goto f_err;
    /* write rerecords info */
    n = sizeof(gz.movie_rerecords);
    if (write(f, &gz.movie_rerecords, n) != n)
      goto f_err;
    n = sizeof(gz.movie_last_recorded_frame);
    if (write(f, &gz.movie_last_recorded_frame, n) != n)
      goto f_err;
f_err:
    if (errno != 0)
      err_str = strerror(errno);
    else {
      if (close(f))
        err_str = strerror(errno);
      f = -1;
    }
  }
  else
    err_str = strerror(errno);
  if (f != -1)
    close(f);
  if (err_str) {
    menu_prompt(gz.menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else
    return 0;
}

static void import_macro_proc(struct menu_item *item, void *data)
{
  menu_get_file(gz.menu_main, GETFILE_LOAD, NULL, ".gzm",
                do_import_macro, NULL);
}

static void export_macro_proc(struct menu_item *item, void *data)
{
  menu_get_file(gz.menu_main, GETFILE_SAVE, "macro", ".gzm",
                do_export_macro, NULL);
}

static void prev_state_proc(struct menu_item *item, void *data)
{
  gz.state_slot += SETTINGS_STATE_MAX - 1;
  gz.state_slot %= SETTINGS_STATE_MAX;
}

static void next_state_proc(struct menu_item *item, void *data)
{
  gz.state_slot += 1;
  gz.state_slot %= SETTINGS_STATE_MAX;
}

static void load_state_proc(struct menu_item *item, void *data)
{
  command_loadstate();
}

static void save_state_proc(struct menu_item *item, void *data)
{
  command_savestate();
}

static void clear_state_proc(struct menu_item *item, void *data)
{
  if (gz.state_buf[gz.state_slot]) {
    free(gz.state_buf[gz.state_slot]);
    gz.state_buf[gz.state_slot] = NULL;
  }
}

static int do_import_state(const char *path, void *data)
{
  const char *s_invalid = "invalid state file";
  const char *s_version = "incompatible state file";
  const char *s_memory = "out of memory";
  const char *err_str = NULL;
  struct state_meta *state = NULL;
  int f = open(path, O_RDONLY);
  if (f != -1) {
    struct stat st;
    if (fstat(f, &st)) {
      err_str = strerror(errno);
      goto error;
    }
    if (st.st_size < sizeof(struct state_meta)) {
      err_str = s_invalid;
      goto error;
    }
    if (gz.state_buf[gz.state_slot])
      free(gz.state_buf[gz.state_slot]);
    gz.state_buf[gz.state_slot] = malloc(st.st_size);
    state = gz.state_buf[gz.state_slot];
    if (!state) {
      err_str = s_memory;
      goto error;
    }
    if (read(f, state, st.st_size) != st.st_size)
      err_str = strerror(errno);
    else if (state->z64_version != Z64_VERSION ||
             state->state_version < SETTINGS_STATE_MIN_VER)
    {
      err_str = s_version;
    }
    else if (state->size != st.st_size)
      err_str = s_invalid;
    else
      state = NULL;
  }
  else
    err_str = strerror(errno);
error:
  if (f != -1)
    close(f);
  if (state) {
    free(state);
    gz.state_buf[gz.state_slot] = NULL;
  }
  if (err_str) {
    menu_prompt(gz.menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else
    return 0;
}

static int do_export_state(const char *path, void *data)
{
  const char *err_str = NULL;
  int f = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (f != -1) {
    struct state_meta *state = gz.state_buf[gz.state_slot];
    if (write(f, state, state->size) == state->size) {
      if (close(f))
        err_str = strerror(errno);
      f = -1;
    }
    else
      err_str = strerror(errno);
  }
  else
    err_str = strerror(errno);
  if (f != -1)
    close(f);
  if (err_str) {
    menu_prompt(gz.menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else
    return 0;
}

static void import_state_proc(struct menu_item *item, void *data)
{
  menu_get_file(gz.menu_main, GETFILE_LOAD, NULL, ".gzs",
                do_import_state, NULL);
}

static void export_state_proc(struct menu_item *item, void *data)
{
  if (gz.state_buf[gz.state_slot]) {
    struct state_meta *state = gz.state_buf[gz.state_slot];
    char defname[32];
    snprintf(defname, sizeof(defname), "000-%s",
             zu_scene_info[state->scene_idx].scene_name);
    menu_get_file(gz.menu_main, GETFILE_SAVE_PREFIX_INC, defname, ".gzs",
                  do_export_state, NULL);
  }
}

static int state_info_draw_proc(struct menu_item *item,
                                struct menu_draw_params *draw_params)
{
  struct gfx_font *font = draw_params->font;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;

  struct state_meta *state = gz.state_buf[gz.state_slot];

  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  gfx_printf(font, x, y, "%i", gz.state_slot);
  if (state) {
    gfx_printf(font, x + cw * 2, y,
               "%s", zu_scene_info[state->scene_idx].scene_name);
    int cx = 11;
    gfx_printf(font, x + cw * cx, y + ch * 2,
               "%" PRIu32 "kb", state->size / 1024);
    if (state->movie_frame != -1) {
      struct gfx_texture *t_macro = resource_get(RES_ICON_MACRO);
      int w = t_macro->tile_width;
      int h = t_macro->tile_height;
      cx += 7;
      struct gfx_sprite sprite =
      {
        t_macro, 2,
        x + cw * cx + (cw - w) / 2,
        y + ch * 2 - (gfx_font_xheight(font) + h + 1) / 2,
        1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_sprite_draw(&sprite);
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
      cx += 2;
      gfx_printf(font, x + cw * cx, y + ch * 2, "%i", state->movie_frame);
    }
  }
  else
    gfx_printf(font, x + cw * 2, y, "no state");

  return 1;
}

static void quick_record_proc(struct menu_item *item, void *data)
{
  if (!zu_in_game())
    gz_log("can not save here");
  else {
    gz_movie_rewind();
    gz.movie_state = MOVIE_RECORDING;
    gz.state_slot = 0;
    command_savestate();
    gz.state_slot = 1;
    command_savestate();
  }
}

static void quick_play_proc(struct menu_item *item, void *data)
{
  struct state_meta *state = gz.state_buf[0];
  if (!zu_in_game())
    gz_log("can not load here");
  else if (!state || state->movie_frame != 0 || gz.movie_input.size == 0)
    gz_log("no movie recorded");
  else {
    gz_movie_rewind();
    gz.movie_state = MOVIE_PLAYING;
    int slot = gz.state_slot;
    gz.state_slot = 0;
    command_loadstate();
    gz.state_slot = slot;
  }
}

static int hack_oca_input_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hack_oca_input = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hack_oca_input = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.hack_oca_input)
      menu_checkbox_set(item, settings->bits.hack_oca_input);
  }
  return 0;
}

static int hack_oca_sync_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hack_oca_sync = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hack_oca_sync = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.hack_oca_sync)
      menu_checkbox_set(item, settings->bits.hack_oca_sync);
  }
  return 0;
}

static int hack_room_load_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hack_room_load = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hack_room_load = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.hack_room_load)
      menu_checkbox_set(item, settings->bits.hack_room_load);
  }
  return 0;
}

static int wiivc_cam_proc(struct menu_item *item,
                          enum menu_callback_reason reason,
                          void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.wiivc_cam = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.wiivc_cam = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.wiivc_cam)
      menu_checkbox_set(item, settings->bits.wiivc_cam);
  }
  return 0;
}

static int gc_oob_chu_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.gc_oob_chu = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.gc_oob_chu = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.gc_oob_chu)
      menu_checkbox_set(item, settings->bits.gc_oob_chu);
  }
  return 0;
}

static int vcont_enable_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  int port = (int)data;
  if (reason == MENU_CALLBACK_CHANGED) {
    gz.vcont_enabled[port] = menu_checkbox_get(item);
    gz_vcont_set(port, vcont_plugged[port], &vcont_raw[port]);
  }
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != gz.vcont_enabled[port])
      menu_checkbox_set(item, gz.vcont_enabled[port]);
  }
  return 0;
}

static int vcont_plugged_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  int port = (int)data;
  if (reason == MENU_CALLBACK_CHANGED) {
    vcont_plugged[port] = menu_checkbox_get(item);
    gz_vcont_set(port, vcont_plugged[port], &vcont_raw[port]);
  }
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != vcont_plugged[port])
      menu_checkbox_set(item, vcont_plugged[port]);
  }
  return 0;
}

static int vcont_joy_proc(struct menu_item *item,
                          enum menu_callback_reason reason,
                          void *data)
{
  int port = (int)data / 2;
  int axis = (int)data % 2;
  int8_t *v = (axis == 0 ? &vcont_raw[port].x : &vcont_raw[port].y);
  if (reason == MENU_CALLBACK_CHANGED) {
    *v = menu_intinput_gets(item);
    gz_vcont_set(port, vcont_plugged[port], &vcont_raw[port]);
  }
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_intinput_gets(item) != *v)
      menu_intinput_set(item, *v);
  }
  return 0;
}

static int vcont_button_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  int port = (int)data / 16;
  int button = (int)data % 16;
  uint16_t button_mask = (1 << button);
  if (reason == MENU_CALLBACK_CHANGED) {
    if (menu_switch_get(item))
      vcont_raw[port].pad |= button_mask;
    else
      vcont_raw[port].pad &= ~button_mask;
    gz_vcont_set(port, vcont_plugged[port], &vcont_raw[port]);
  }
  else if (reason == MENU_CALLBACK_THINK) {
    _Bool state = vcont_raw[port].pad & button_mask;
    if (menu_switch_get(item) != state)
      menu_switch_set(item, state);
  }
  return 0;
}

static int byte_ztarget_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.ignore_target = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.ignore_target = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.ignore_target)
      menu_checkbox_set(item, settings->bits.ignore_target);
  }
  return 0;
}

static int state_rng_proc(struct menu_item *item,
                          enum menu_callback_reason reason,
                          void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.ignore_state_rng = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.ignore_state_rng = 0;
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_checkbox_get(item) != settings->bits.ignore_state_rng)
      menu_checkbox_set(item, settings->bits.ignore_state_rng);
  }
  return 0;
}

struct menu *gz_macro_menu(void)
{
  static struct menu menu;
  static struct menu menu_settings;
  static struct menu menu_vcont;
  struct menu_item *item;

  /* initialize menus */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&menu_settings, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&menu_vcont, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* load textures */
  struct gfx_texture *t_macro = resource_get(RES_ICON_MACRO);
  struct gfx_texture *t_movie = resource_get(RES_ICON_MOVIE);
  struct gfx_texture *t_arrow = resource_get(RES_ICON_ARROW);
  struct gfx_texture *t_save = resource_get(RES_ICON_SAVE);

  /* populate macro top menu */
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  /* create movie controls */
  item = menu_add_switch(&menu, 0, 2,
                         t_macro, 0, 0xFF0000, t_macro, 0, 0xFFFFFF, 1.f, 0,
                         pause_switch_proc, NULL);
  item->tooltip = "pause";
  item = menu_add_switch(&menu, 3, 2,
                         t_macro, 1, 0xFFFFFF, t_macro, 1, 0xFFFFFF, 1.f, 0,
                         advance_switch_proc, NULL);
  item->tooltip = "frame advance";
  item = menu_add_switch(&menu, 0, 4,
                         t_macro, 2, 0xFF0000, t_macro, 2, 0xFFFFFF, 1.f, 0,
                         record_switch_proc, NULL);
  item->tooltip = "record macro";
  item = menu_add_switch(&menu, 3, 4,
                         t_macro, 3, 0xFF0000, t_macro, 3, 0xFFFFFF, 1.f, 0,
                         play_switch_proc, NULL);
  item->tooltip = "play macro";
  item = menu_add_switch(&menu, 6, 4,
                         t_macro, 4, 0xFFFFFF, t_macro, 4, 0xFFFFFF, 1.f, 0,
                         rewind_switch_proc, NULL);
  item->tooltip = "rewind macro";
  item = menu_add_switch(&menu, 9, 4,
                         t_macro, 5, 0xFFFFFF, t_macro, 5, 0xFFFFFF, 1.f, 0,
                         trim_switch_proc, NULL);
  item->tooltip = "trim macro";
  item = menu_add_intinput(&menu, 12, 4, 10, 6, movie_pos_proc, NULL);
  item->tooltip = "macro frame";
  menu_add_watch(&menu, 19, 4, (uint32_t)&gz.movie_input.size,
                 WATCH_TYPE_U32);
  item = menu_add_button_icon(&menu, 0, 6, t_save, 0, 0xFFFFFF,
                              import_macro_proc, NULL);
  item->tooltip = "import macro";
  item = menu_add_button_icon(&menu, 3, 6, t_save, 1, 0xFFFFFF,
                              export_macro_proc, NULL);
  item->tooltip = "export macro";
  /* create state controls */
  menu_add_button_icon(&menu, 0, 8, t_arrow, 3, 0xFFFFFF,
                       prev_state_proc, NULL);
  menu_add_button_icon(&menu, 3, 8, t_arrow, 2, 0xFFFFFF,
                       next_state_proc, NULL);
  menu_add_static_custom(&menu, 6, 8, state_info_draw_proc, NULL, 0xC0C0C0);
  item = menu_add_button_icon(&menu, 0, 10, t_save, 2, 0xFFFFFF,
                              load_state_proc, NULL);
  item->tooltip = "load state";
  item = menu_add_button_icon(&menu, 3, 10, t_save, 3, 0xFFFFFF,
                              save_state_proc, NULL);
  item->tooltip = "save state";
  item = menu_add_button_icon(&menu, 6, 10, t_save, 4, 0xFFFFFF,
                              clear_state_proc, NULL);
  item->tooltip = "clear state";
  item = menu_add_button_icon(&menu, 9, 10, t_save, 0, 0xFFFFFF,
                              import_state_proc, NULL);
  item->tooltip = "import state";
  item = menu_add_button_icon(&menu, 12, 10, t_save, 1, 0xFFFFFF,
                              export_state_proc, NULL);
  item->tooltip = "export state";
  /* create movie controls */
  item = menu_add_button_icon(&menu, 0, 13, t_movie, 0, 0xFFFFFF,
                              quick_record_proc, NULL);
  item->tooltip = "quick record movie";
  item = menu_add_button_icon(&menu, 3, 13, t_movie, 1, 0xFFFFFF,
                              quick_play_proc, NULL);
  item->tooltip = "quick play movie";
  /* display rerecord count */
  menu_add_static(&menu, 0, 15, "rerecords:", 0xC0C0C0);
  menu_add_watch(&menu, 11, 15, (uint32_t)&gz.movie_rerecords,
                 WATCH_TYPE_U32);
  /* create settings controls */
  menu_add_submenu(&menu, 0, 16, &menu_settings, "settings");
  /* create virtual controller controls */
  menu_add_submenu(&menu, 0, 17, &menu_vcont, "virtual controller");
  /* create tooltip */
  menu_add_tooltip(&menu, 8, 0, gz.menu_main, 0xC0C0C0);

  /* populate settings menu */
  menu_settings.selector = menu_add_submenu(&menu_settings, 0, 0, NULL,
                                            "return");
  menu_add_static(&menu_settings, 0, 1, "recording settings", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 2, hack_oca_input_proc, NULL);
  menu_add_static(&menu_settings, 4, 2, "ocarina input hack", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 3, hack_oca_sync_proc, NULL);
  menu_add_static(&menu_settings, 4, 3, "ocarina sync hack", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 4, hack_room_load_proc, NULL);
  menu_add_static(&menu_settings, 4, 4, "room load hack", 0xC0C0C0);
  menu_add_static(&menu_settings, 0, 5, "game settings", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 6, wiivc_cam_proc, NULL);
  menu_add_static(&menu_settings, 4, 6, "wii vc camera", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 7, gc_oob_chu_proc, NULL);
  menu_add_static(&menu_settings, 4, 7, "gc oob chu", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 8, byte_ztarget_proc, NULL);
  menu_add_static(&menu_settings, 4, 8, "ignore state's z-target", 0xC0C0C0);
  menu_add_checkbox(&menu_settings, 2, 9, state_rng_proc, NULL);
  menu_add_static(&menu_settings, 4, 9, "ignore state's rng", 0xC0C0C0);

  /* populate virtual pad menu */
  menu_vcont.selector = menu_add_submenu(&menu_vcont, 0, 0, NULL, "return");
  struct gfx_texture *t_buttons = resource_get(RES_ICON_BUTTONS);
  for (int i = 0; i < 4; ++i) {
    char s[16];
    sprintf(s, "controller %i", i + 1);
    menu_add_static(&menu_vcont, 0, 1 + i * 4, s, 0xC0C0C0);
    menu_add_checkbox(&menu_vcont, 14, 1 + i * 4,
                      vcont_enable_proc, (void *)i);

    menu_add_static(&menu_vcont, 2, 2 + i * 4, "plugged in", 0xC0C0C0);
    menu_add_checkbox(&menu_vcont, 14, 2 + i * 4,
                      vcont_plugged_proc, (void *)i);

    menu_add_static(&menu_vcont, 2, 3 + i * 4, "joystick", 0xC0C0C0);
    menu_add_intinput(&menu_vcont, 14, 3 + i * 4, -10, 4,
                      vcont_joy_proc, (void *)(i * 2 + 0));
    menu_add_intinput(&menu_vcont, 19, 3 + i * 4, -10, 4,
                      vcont_joy_proc, (void *)(i * 2 + 1));

    menu_add_static(&menu_vcont, 2, 4 + i * 4, "buttons", 0xC0C0C0);
    static const int buttons[] =
    {
      15, 14, 12, 3, 2, 1, 0, 13, 5, 4, 11, 10, 9, 8,
    };
    for (int j = 0; j < sizeof(buttons) / sizeof(*buttons); ++j) {
      int b = buttons[j];
      item = menu_add_switch(&menu_vcont, 14, 4 + i * 4,
                             t_buttons, b, input_button_color[b],
                             t_buttons, b, 0x808080,
                             1.f, 0, vcont_button_proc, (void *)(i * 16 + b));
      item->pxoffset = j * 10;
    }
  }

  return &menu;
}

void gz_vcont_set(int port, _Bool plugged, z64_controller_t *cont)
{
  z64_input_t *vcont = &gz.vcont_input[port];

  vcont->raw_prev = vcont->raw;
  vcont->status_prev = vcont->status;

  if (plugged) {
    vcont->raw = *cont;
    vcont->status = 0x0000;
  }
  else {
    vcont->raw.pad = 0x0000;
    vcont->raw.x = 0;
    vcont->raw.y = 0;
    vcont->status = 0x0800;
  }

  vcont->x_diff += (vcont->raw.x - vcont->raw_prev.x);
  vcont->y_diff += (vcont->raw.y - vcont->raw_prev.y);

  uint16_t pad_changed = (vcont->raw.pad ^ vcont->raw_prev.pad);
  vcont->pad_pressed |= (pad_changed & vcont->raw.pad);
  vcont->pad_released |= (pad_changed & vcont->raw_prev.pad);

  vcont->adjusted_x = zu_adjust_joystick(vcont->raw.x);
  vcont->adjusted_y = zu_adjust_joystick(vcont->raw.y);
}

void gz_vcont_get(int port, z64_input_t *input)
{
  z64_input_t *vcont = &gz.vcont_input[port];

  input->raw_prev = input->raw;
  input->status_prev = input->status;

  input->raw = vcont->raw;
  input->status = vcont->status;
  input->x_diff = vcont->x_diff;
  input->y_diff = vcont->y_diff;
  input->pad_pressed = vcont->pad_pressed;
  input->pad_released = vcont->pad_released;
  input->adjusted_x = vcont->adjusted_x;
  input->adjusted_y = vcont->adjusted_y;

  vcont->x_diff = 0;
  vcont->y_diff = 0;
  vcont->pad_pressed = 0;
  vcont->pad_released = 0;
}
