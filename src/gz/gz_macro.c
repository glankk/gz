#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "files.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "state.h"
#include "sys.h"
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

static int trim_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_CHANGED) {
    vector_erase(&gz.movie_inputs, gz.movie_frame,
                 gz.movie_inputs.size - gz.movie_frame);
    vector_erase(&gz.movie_seeds, gz.movie_seed_pos,
                 gz.movie_seeds.size - gz.movie_seed_pos);
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

#ifndef WIIVC
static int do_import_state(const char *path, void *data)
{
  const char *err_str = NULL;
  struct state_meta *state = NULL;
  FILE *f = fopen(path, "rb");
  if (f) {
    setbuf(f, NULL);
    struct stat st;
    fstat(fileno(f), &st);
    if (gz.state_buf[gz.state_slot])
      free(gz.state_buf[gz.state_slot]);
    gz.state_buf[gz.state_slot] = malloc(st.st_size);
    state = gz.state_buf[gz.state_slot];
    sys_io_mode(SYS_IO_DMA);
    if (fread(state, 1, st.st_size, f) == st.st_size)
      state = NULL;
    else
      err_str = strerror(ferror(f));
    sys_io_mode(SYS_IO_PIO);
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
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
  FILE *f = fopen(path, "wb");
  if (f) {
    setbuf(f, NULL);
    struct state_meta *state = gz.state_buf[gz.state_slot];
    if (fwrite(state, 1, state->size, f) == state->size) {
      if (fclose(f))
        err_str = strerror(errno);
      f = NULL;
    }
    else
      err_str = strerror(ferror(f));
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
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
    menu_get_file(gz.menu_main, GETFILE_SAVE, "state", ".gzs",
                  do_export_state, NULL);
  }
}
#endif

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
    gfx_printf(font, x + cw * 8, y + ch * 2,
               "%" PRIu32 "kb", state->size / 1024);
    if (state->movie_frame != -1) {
      struct gfx_texture *t_macro = resource_get(RES_ICON_MACRO);
      int w = t_macro->tile_width;
      int h = t_macro->tile_height;
      struct gfx_sprite sprite =
      {
        t_macro, 2,
        x + cw * 15 + (cw - w) / 2,
        y + ch * 2 - (gfx_font_xheight(font) + h + 1) / 2,
        1.f, 1.f,
      };
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xC0, 0x00, 0x00, alpha));
      gfx_sprite_draw(&sprite);
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
      gfx_printf(font, x + cw * 17, y + ch * 2, "%i", state->movie_frame);
    }
  }
  else
    gfx_printf(font, x + cw * 2, y, "no state");

  return 1;
}

struct menu *gz_macro_menu(void)
{
  static struct menu menu;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /* create movie controls */
  struct gfx_texture *t_macro = resource_get(RES_ICON_MACRO);
  menu_add_switch(&menu, 0, 2,
                  t_macro, 0, 0xFF0000, t_macro, 0, 0xFFFFFF, 1.f,
                  pause_switch_proc, NULL);
  menu_add_switch(&menu, 2, 2,
                  t_macro, 1, 0xFFFFFF, t_macro, 1, 0xFFFFFF, 1.f,
                  advance_switch_proc, NULL);
  menu_add_switch(&menu, 0, 4,
                  t_macro, 2, 0xFF0000, t_macro, 2, 0xFFFFFF, 1.f,
                  record_switch_proc, NULL);
  menu_add_switch(&menu, 2, 4,
                  t_macro, 3, 0xFF0000, t_macro, 3, 0xFFFFFF, 1.f,
                  play_switch_proc, NULL);
  menu_add_switch(&menu, 4, 4,
                  t_macro, 4, 0xFFFFFF, t_macro, 4, 0xFFFFFF, 1.f,
                  trim_switch_proc, NULL);
  menu_add_intinput(&menu, 6, 4, 10, 6, movie_pos_proc, NULL);

  /* create state controls */
  struct gfx_texture *t_arrow = resource_get(RES_ICON_ARROW);
  menu_add_button_icon(&menu, 0, 6, t_arrow, 3, 0xFFFFFF,
                       prev_state_proc, NULL);
  menu_add_button_icon(&menu, 2, 6, t_arrow, 2, 0xFFFFFF,
                       next_state_proc, NULL);
  menu_add_static_custom(&menu, 4, 6, state_info_draw_proc, NULL, 0xC0C0C0);
  struct gfx_texture *t_save = resource_get(RES_ICON_SAVE);
  menu_add_button_icon(&menu, 0, 8, t_save, 2, 0xFFFFFF,
                       load_state_proc, NULL);
  menu_add_button_icon(&menu, 3, 8, t_save, 3, 0xFFFFFF,
                       save_state_proc, NULL);
#ifndef WIIVC
  menu_add_button_icon(&menu, 6, 8, t_save, 0, 0xFFFFFF,
                       import_state_proc, NULL);
  menu_add_button_icon(&menu, 9, 8, t_save, 1, 0xFFFFFF,
                       export_state_proc, NULL);
#endif

  return &menu;
}
