#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "files.h"
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "sys.h"
#include "z64.h"
#include "zu.h"

static int byte_mod_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  uint8_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static int byte_switch_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  uint8_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    _Bool value = *p;
    if (menu_option_get(item) != value)
      menu_option_set(item, value);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *p = menu_option_get(item);
  return 0;
}

static int halfword_mod_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  uint16_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static int halfword_optionmod_proc(struct menu_item *item,
                                   enum menu_callback_reason reason,
                                   void *data)
{
  uint16_t *v = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != *v)
      menu_option_set(item, *v);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *v = menu_option_get(item);
  return 0;
}

static void restore_gs_proc(struct menu_item *item, void *data)
{
  memset(&z64_file.gs_flags, 0x00, sizeof(z64_file.gs_flags));
}

static void call_navi_proc(struct menu_item *item, void *data)
{
  z64_file.navi_timer = 0x025A;
}

static void memfile_dec_proc(struct menu_item *item, void *data)
{
  gz.memfile_slot += SETTINGS_MEMFILE_MAX - 1;
  gz.memfile_slot %= SETTINGS_MEMFILE_MAX;
}

static void memfile_inc_proc(struct menu_item *item, void *data)
{
  gz.memfile_slot += 1;
  gz.memfile_slot %= SETTINGS_MEMFILE_MAX;
}

static void set_time_proc(struct menu_item *item, void *data)
{
  gz.target_day_time = (uint32_t)data;
}

static void set_carpenter_flags_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0x90);
  zu_set_event_flag(0x91);
  zu_set_event_flag(0x92);
  zu_set_event_flag(0x93);
}

static void clear_carpenter_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0x90);
  zu_clear_event_flag(0x91);
  zu_clear_event_flag(0x92);
  zu_clear_event_flag(0x93);
}

static void set_intro_flags_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0xA0);
  zu_set_event_flag(0xA1);
  zu_set_event_flag(0xA3);
  zu_set_event_flag(0xA4);
  zu_set_event_flag(0xA5);
  zu_set_event_flag(0xA6);
  zu_set_event_flag(0xA7);
  zu_set_event_flag(0xA8);
  zu_set_event_flag(0xB1);
  zu_set_event_flag(0xB2);
  zu_set_event_flag(0xB3);
  zu_set_event_flag(0xB4);
  zu_set_event_flag(0xB5);
  zu_set_event_flag(0xB6);
  zu_set_event_flag(0xB7);
  zu_set_event_flag(0xB8);
  zu_set_event_flag(0xB9);
  zu_set_event_flag(0xBA);
  zu_set_event_flag(0xC0);
  zu_set_event_flag(0xC7);
}

static void clear_intro_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0xA0);
  zu_clear_event_flag(0xA1);
  zu_clear_event_flag(0xA3);
  zu_clear_event_flag(0xA4);
  zu_clear_event_flag(0xA5);
  zu_clear_event_flag(0xA6);
  zu_clear_event_flag(0xA7);
  zu_clear_event_flag(0xA8);
  zu_clear_event_flag(0xB1);
  zu_clear_event_flag(0xB2);
  zu_clear_event_flag(0xB3);
  zu_clear_event_flag(0xB4);
  zu_clear_event_flag(0xB5);
  zu_clear_event_flag(0xB6);
  zu_clear_event_flag(0xB7);
  zu_clear_event_flag(0xB8);
  zu_clear_event_flag(0xB9);
  zu_clear_event_flag(0xBA);
  zu_clear_event_flag(0xC0);
  zu_clear_event_flag(0xC7);
}

static void set_reward_flags_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0x19);
  zu_set_event_flag(0x25);
  zu_set_event_flag(0x37);
  zu_set_event_flag(0x48);
  zu_set_event_flag(0x49);
  zu_set_event_flag(0x4A);
  zu_set_event_flag(0xC8);
}

static void clear_reward_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0x19);
  zu_clear_event_flag(0x25);
  zu_clear_event_flag(0x37);
  zu_clear_event_flag(0x48);
  zu_clear_event_flag(0x49);
  zu_clear_event_flag(0x4A);
  zu_clear_event_flag(0xC8);
}

#ifndef WIIVC
static int load_file_to_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->bits.load_to)
      menu_option_set(item, settings->bits.load_to);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->bits.load_to = menu_option_get(item);
  return 0;
}

static int on_file_load_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->bits.on_load)
      menu_option_set(item, settings->bits.on_load);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->bits.on_load = menu_option_get(item);
  return 0;
}

static int do_save_file(const char *path, void *data)
{
  const char *s_memory = "out of memory";
  const char *err_str = NULL;
  struct memory_file *file = NULL;
  FILE *f = fopen(path, "wb");
  if (f) {
    file = malloc(sizeof(*file));
    if (!file)
      err_str = s_memory;
    else {
      gz_save_memfile(file);
      if (fwrite(file, 1, sizeof(*file), f) != sizeof(*file))
        err_str = strerror(ferror(f));
      else {
        if (fclose(f))
          err_str = strerror(errno);
        f = NULL;
      }
    }
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
  if (file)
    free(file);
  if (err_str) {
    menu_prompt(gz.menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else
    return 0;
}

static int do_load_file(const char *path, void *data)
{
  const char *s_invalid = "invalid or corrupt memfile";
  const char *s_memory = "out of memory";
  const char *err_str = NULL;
  struct memory_file *file = NULL;
  FILE *f = fopen(path, "rb");
  if (f) {
    struct stat st;
    if (fstat(fileno(f), &st))
      err_str = strerror(errno);
    else if (st.st_size != sizeof(*file))
      err_str = s_invalid;
    else {
      file = malloc(sizeof(*file));
      if (!file)
        err_str = s_memory;
      else if (fread(file, 1, sizeof(*file), f) != sizeof(*file))
        err_str = strerror(ferror(f));
      else {
        if (settings->bits.load_to == SETTINGS_LOADTO_ZFILE ||
            settings->bits.load_to == SETTINGS_LOADTO_BOTH)
        {
          gz_load_memfile(file);
        }
        if (settings->bits.load_to == SETTINGS_LOADTO_MEMFILE ||
            settings->bits.load_to == SETTINGS_LOADTO_BOTH)
        {
          gz.memfile[gz.memfile_slot] = *file;
          gz.memfile_saved[gz.memfile_slot] = 1;
        }
      }
    }
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
  if (file)
    free(file);
  if (err_str) {
    menu_prompt(gz.menu_main, err_str, "return\0", 0, NULL, NULL);
    return 1;
  }
  else {
    if (settings->bits.load_to == SETTINGS_LOADTO_ZFILE ||
        settings->bits.load_to == SETTINGS_LOADTO_BOTH)
    {
      if (settings->bits.on_load == SETTINGS_ONLOAD_RELOAD)
        command_reload();
      else if (settings->bits.on_load == SETTINGS_ONLOAD_VOID)
        command_void();
    }
    return 0;
  }
}

static void save_file_proc(struct menu_item *item, void *data)
{
  menu_get_file(gz.menu_main, GETFILE_SAVE, "file", ".ootsave",
                do_save_file, NULL);
}

static void load_file_proc(struct menu_item *item, void *data)
{
  menu_get_file(gz.menu_main, GETFILE_LOAD, NULL, ".ootsave",
                do_load_file, NULL);
}
#endif

struct menu *gz_file_menu(void)
{
  static struct menu menu;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /* create file commands */
  menu_add_button(&menu, 0, 1, "restore skulltulas", restore_gs_proc, NULL);
  menu_add_button(&menu, 0, 2, "call navi", call_navi_proc, NULL);

  /* create memfile controls */
  menu_add_static(&menu, 0, 3, "memory file", 0xC0C0C0);
  menu_add_watch(&menu, 19, 3, (uint32_t)&gz.memfile_slot, WATCH_TYPE_U8);
  menu_add_button(&menu, 17, 3, "-", memfile_dec_proc, NULL);
  menu_add_button(&menu, 21, 3, "+", memfile_inc_proc, NULL);

  /* create time of day controls */
  struct gfx_texture *t_daytime = resource_get(RES_ICON_DAYTIME);
  menu_add_static(&menu, 0, 4, "time of day", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 4, t_daytime, 0, 0xFFC800,
                       set_time_proc, (void*)0x4AB0);
  menu_add_button_icon(&menu, 19, 4, t_daytime, 1, 0xA0A0E0,
                       set_time_proc, (void*)0xC010);
  menu_add_intinput(&menu, 21, 4, 16, 4,
                    halfword_mod_proc, &z64_file.day_time);

  /* create flag controls */
  struct gfx_texture *t_check = resource_get(RES_ICON_CHECK);
  menu_add_static(&menu, 0, 5, "carpenters freed", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 5, t_check, 0, 0x00FF00,
                       set_carpenter_flags_proc, NULL);
  menu_add_button_icon(&menu, 19, 5, t_check, 1, 0xFF0000,
                       clear_carpenter_flags_proc, NULL);
  menu_add_static(&menu, 0, 6, "intro cutscenes", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 6, t_check, 0, 0x00FF00,
                       set_intro_flags_proc, NULL);
  menu_add_button_icon(&menu, 19, 6, t_check, 1, 0xFF0000,
                       clear_intro_flags_proc, NULL);
  menu_add_static(&menu, 0, 7, "rewards obtained", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 7, t_check, 0, 0x00FF00,
                       set_reward_flags_proc, NULL);
  menu_add_button_icon(&menu, 19, 7, t_check, 1, 0xFF0000,
                       clear_reward_flags_proc, NULL);

  /* create timer controls */
  menu_add_static(&menu, 0, 8, "timer 1", 0xC0C0C0);
  menu_add_intinput(&menu, 17, 8, 10, 5,
                    halfword_mod_proc, &z64_file.timer_1_value);
  menu_add_option(&menu, 23, 8,
                  "inactive\0""heat starting\0""heat initial\0"
                  "heat moving\0""heat active\0""race starting\0"
                  "race initial\0""race moving\0""race active\0"
                  "race stopped\0""race ending\0""timer starting\0"
                  "timer initial\0""timer moving\0""timer active\0"
                  "timer stopped\0",
                  halfword_optionmod_proc, &z64_file.timer_1_state);
  menu_add_static(&menu, 0, 9, "timer 2", 0xC0C0C0);
  menu_add_intinput(&menu, 17, 9, 10, 5,
                    halfword_mod_proc, &z64_file.timer_2_value);
  menu_add_option(&menu, 23, 9,
                  "inactive\0""starting\0""initial\0""moving\0""active\0"
                  "stopped\0""ending\0""timer starting\0""timer initial\0"
                  "timer moving\0""timer active\0""timer stopped\0",
                  halfword_optionmod_proc, &z64_file.timer_2_state);

  /* create file settings controls */
  menu_add_static(&menu, 0, 10, "file index", 0xC0C0C0);
  menu_add_intinput(&menu, 17, 10, 16, 2, byte_mod_proc, &z64_file.file_index);
  menu_add_static(&menu, 0, 11, "language", 0xC0C0C0);
  menu_add_option(&menu, 17, 11, "japanese\0""english\0",
                  byte_switch_proc, &z64_file.language);
  menu_add_static(&menu, 0, 12, "z targeting", 0xC0C0C0);
  menu_add_option(&menu, 17, 12, "switch\0""hold\0",
                  byte_switch_proc, &z64_file.z_targeting);

#ifndef WIIVC
  /* create disk file controls */
  menu_add_static(&menu, 0, 13, "load file to", 0xC0C0C0);
  menu_add_option(&menu, 17, 13, "zelda file\0""current memfile\0""both\0",
                  load_file_to_proc, NULL);
  menu_add_static(&menu, 0, 14, "after loading", 0xC0C0C0);
  menu_add_option(&menu, 17, 14, "do nothing\0""reload scene\0""void out\0",
                  on_file_load_proc, NULL);
  menu_add_button(&menu, 0, 15, "save to disk", save_file_proc, NULL);
  menu_add_button(&menu, 0, 16, "load from disk", load_file_proc, NULL);
#endif

  return &menu;
}
