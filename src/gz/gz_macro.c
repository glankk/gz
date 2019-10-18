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
  else if (reason == MENU_CALLBACK_CHANGED)
    command_playmacro();
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
    vector_erase(&gz.movie_inputs, gz.movie_frame,
                 gz.movie_inputs.size - gz.movie_frame);
    vector_erase(&gz.movie_seeds, gz.movie_seed_pos,
                 gz.movie_seeds.size - gz.movie_seed_pos);
    vector_shrink_to_fit(&gz.movie_inputs);
    vector_shrink_to_fit(&gz.movie_seeds);
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
    vector_clear(&gz.movie_inputs);
    vector_clear(&gz.movie_seeds);
    if (!vector_reserve(&gz.movie_inputs, n_input) ||
        !vector_reserve(&gz.movie_seeds, n_seed))
    {
      err_str = s_memory;
      goto error;
    }
    gz_movie_rewind();
    vector_insert(&gz.movie_inputs, 0, n_input, NULL);
    vector_insert(&gz.movie_seeds, 0, n_seed, NULL);
    vector_shrink_to_fit(&gz.movie_inputs);
    vector_shrink_to_fit(&gz.movie_seeds);
    n = sizeof(gz.movie_input_start);
    if (read(f, &gz.movie_input_start, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    sys_io_mode(SYS_IO_DMA);
    n = gz.movie_inputs.element_size * n_input;
    if (read(f, gz.movie_inputs.begin, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
    n = gz.movie_seeds.element_size * n_seed;
    if (read(f, gz.movie_seeds.begin, n) != n) {
      err_str = s_eof;
      goto f_err;
    }
f_err:
    sys_io_mode(SYS_IO_PIO);
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
    size_t n_input = gz.movie_inputs.size;
    size_t n_seed = gz.movie_seeds.size;
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
    n = gz.movie_inputs.element_size * n_input;
    if (write(f, gz.movie_inputs.begin, n) != n)
      goto f_err;
    n = gz.movie_seeds.element_size * n_seed;
    if (write(f, gz.movie_seeds.begin, n) != n)
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
    sys_io_mode(SYS_IO_DMA);
    if (read(f, state, st.st_size) != st.st_size)
      err_str = strerror(errno);
    else if (state->z64_version != Z64_VERSION ||
             state->state_version != SETTINGS_STATE_VERSION)
    {
      err_str = s_version;
    }
    else if (state->size != st.st_size)
      err_str = s_invalid;
    else
      state = NULL;
    sys_io_mode(SYS_IO_PIO);
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
    char buf[100] = "00-";
    buf[3] = 0;
    menu_get_file(gz.menu_main, GETFILE_SAVE, strcat(buf, zu_scene_info[state->scene_idx].scene_name), ".gzs",
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
  else if (!state || state->movie_frame != 0 || gz.movie_inputs.size == 0)
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

struct menu *gz_macro_menu(void)
{
  static struct menu menu;
  struct menu_item *item;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /* load textures */
  struct gfx_texture *t_macro = resource_get(RES_ICON_MACRO);
  struct gfx_texture *t_movie = resource_get(RES_ICON_MOVIE);
  struct gfx_texture *t_arrow = resource_get(RES_ICON_ARROW);
  struct gfx_texture *t_save = resource_get(RES_ICON_SAVE);

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
  menu_add_watch(&menu, 19, 4, (uint32_t)&gz.movie_inputs.size,
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

  /* create tooltip */
  menu_add_tooltip(&menu, 0, 15, gz.menu_main, 0xC0C0C0);

  return &menu;
}
