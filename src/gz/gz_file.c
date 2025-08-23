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

#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12 || \
    Z64_VERSION == Z64_OOTMQJ || \
    Z64_VERSION == Z64_OOTMQU || \
    Z64_VERSION == Z64_OOTGCJ || \
    Z64_VERSION == Z64_OOTGCU || \
    Z64_VERSION == Z64_OOTCEJ
#define LANGS "japanese\0""english\0"
#elif Z64_VERSION == Z64_OOTIQC
#define LANGS "japanese\0""chinese\0"
#endif

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

#if Z64_VERSION == Z64_OOTIQC
static int ique_input_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    int v = __osBbHackFlags & 3;
    if (menu_option_get(item) != v)
      menu_option_set(item, v);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE) {
    __osBbHackFlags &= ~3;
    __osBbHackFlags |= menu_option_get(item);
  }
  return 0;
}
#endif

static void restore_gs_proc(struct menu_item *item, void *data)
{
  memset(&z64_file.gs_flags, 0x00, sizeof(z64_file.gs_flags));
}

static void call_navi_proc(struct menu_item *item, void *data)
{
  z64_file.navi_timer = 0x025A;
}

static void load_debug_file_proc(struct menu_item *item, void *data)
{
  z64_Sram_LoadDebugSave();
  z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_B);
  z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_CL);
  z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_CD);
  z64_UpdateItemButton(&z64_game, Z64_ITEMBTN_CR);
  z64_UpdateEquipment(&z64_game, &z64_link);
}

static void set_time_proc(struct menu_item *item, void *data)
{
  gz.target_day_time = (uint32_t)data;
}

static void set_epona_flag_proc(struct menu_item *item, void *data)
{
  zu_set_event_flag(0x18);
}

static void clear_epona_flag_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0x18);
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
  zu_set_event_flag(0x07);
  zu_set_event_flag(0x25);
  zu_set_event_flag(0x37);
  zu_set_event_flag(0x48);
  zu_set_event_flag(0x49);
  zu_set_event_flag(0x4A);
  zu_set_event_flag(0xC8);
}

static void clear_reward_flags_proc(struct menu_item *item, void *data)
{
  zu_clear_event_flag(0x07);
  zu_clear_event_flag(0x25);
  zu_clear_event_flag(0x37);
  zu_clear_event_flag(0x48);
  zu_clear_event_flag(0x49);
  zu_clear_event_flag(0x4A);
  zu_clear_event_flag(0xC8);
}

struct menu *gz_file_menu(void)
{
  static struct menu menu;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /* create file commands */
  menu_add_button(&menu, 0, 1, "restore skulltulas", restore_gs_proc, NULL);
  menu_add_button(&menu, 0, 2, "call navi", call_navi_proc, NULL);
  menu_add_button(&menu, 0, 3, "load debug file", load_debug_file_proc, NULL);

  /* create time of day controls */
  struct gfx_texture *t_daytime = resource_get(RES_ICON_DAYTIME);
  menu_add_static(&menu, 0, 4, "time of day", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 4, t_daytime, 0, 0xFFC800,
                       set_time_proc, (void *)0x4AB0);
  menu_add_button_icon(&menu, 19, 4, t_daytime, 1, 0xA0A0E0,
                       set_time_proc, (void *)0xC010);
  menu_add_intinput(&menu, 21, 4, 16, 4,
                    halfword_mod_proc, &z64_file.day_time);

  /* create flag controls */
  struct gfx_texture *t_check = resource_get(RES_ICON_CHECK);
  menu_add_static(&menu, 0, 5, "epona freed", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 5, t_check, 0, 0x00FF00,
                       set_epona_flag_proc, NULL);
  menu_add_button_icon(&menu, 19, 5, t_check, 1, 0xFF0000,
                       clear_epona_flag_proc, NULL);
  menu_add_static(&menu, 0, 6, "carpenters freed", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 6, t_check, 0, 0x00FF00,
                       set_carpenter_flags_proc, NULL);
  menu_add_button_icon(&menu, 19, 6, t_check, 1, 0xFF0000,
                       clear_carpenter_flags_proc, NULL);
  menu_add_static(&menu, 0, 7, "intro cutscenes", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 7, t_check, 0, 0x00FF00,
                       set_intro_flags_proc, NULL);
  menu_add_button_icon(&menu, 19, 7, t_check, 1, 0xFF0000,
                       clear_intro_flags_proc, NULL);
  menu_add_static(&menu, 0, 8, "rewards obtained", 0xC0C0C0);
  menu_add_button_icon(&menu, 17, 8, t_check, 0, 0x00FF00,
                       set_reward_flags_proc, NULL);
  menu_add_button_icon(&menu, 19, 8, t_check, 1, 0xFF0000,
                       clear_reward_flags_proc, NULL);

  /* create timer controls */
  menu_add_static(&menu, 0, 9, "timer 1", 0xC0C0C0);
  menu_add_intinput(&menu, 17, 9, 10, 5,
                    halfword_mod_proc, &z64_file.timer_1_value);
  menu_add_option(&menu, 23, 9,
                  "inactive\0""heat starting\0""heat initial\0"
                  "heat moving\0""heat active\0""race starting\0"
                  "race initial\0""race moving\0""race active\0"
                  "race stopped\0""race ending\0""timer starting\0"
                  "timer initial\0""timer moving\0""timer active\0"
                  "timer stopped\0",
                  halfword_optionmod_proc, &z64_file.timer_1_state);
  menu_add_static(&menu, 0, 10, "timer 2", 0xC0C0C0);
  menu_add_intinput(&menu, 17, 10, 10, 5,
                    halfword_mod_proc, &z64_file.timer_2_value);
  menu_add_option(&menu, 23, 10,
                  "inactive\0""starting\0""initial\0""moving\0""active\0"
                  "stopped\0""ending\0""timer starting\0""timer initial\0"
                  "timer moving\0""timer active\0""timer stopped\0",
                  halfword_optionmod_proc, &z64_file.timer_2_state);

  /* create file settings controls */
  menu_add_static(&menu, 0, 11, "file index", 0xC0C0C0);
  menu_add_intinput(&menu, 17, 11, 16, 2, byte_mod_proc, &z64_file.file_index);
  menu_add_static(&menu, 0, 12, "language", 0xC0C0C0);
  menu_add_option(&menu, 17, 12, LANGS,
                  byte_switch_proc, &z64_file.language);
  menu_add_static(&menu, 0, 13, "z targeting", 0xC0C0C0);
  menu_add_option(&menu, 17, 13, "switch\0""hold\0",
                  byte_switch_proc, &z64_file.z_targeting);

#if Z64_VERSION == Z64_OOTIQC
  menu_add_static(&menu, 0, 14, "input via", 0xC0C0C0);
  menu_add_option(&menu, 17, 14, "ique player\0""controller 2\0""controller 3\0""controller 4\0", ique_input_proc, NULL);
#endif

  return &menu;
}
