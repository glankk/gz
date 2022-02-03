#include <stdint.h>
#include <string.h>
#include "explorer.h"
#include "gz.h"
#include "menu.h"
#include "settings.h"
#include "z64.h"

static void set_entrance_proc(struct menu_item *item, void *data)
{
  z64_file.void_pos = z64_link.common.pos_2;
  z64_file.void_yaw = z64_link.common.rot_2.y;
  z64_file.void_var = z64_link.common.variable;
  z64_file.void_entrance = z64_file.entrance_index;
  z64_file.void_room_index = z64_game.room_ctxt.rooms[0].index;
}

static void clear_scene_flags_proc(struct menu_item *item, void *data)
{
  z64_game.swch_flags = 0x00000000;
  z64_game.temp_swch_flags = 0x00000000;
  z64_game.unk_flags_0 = 0x00000000;
  z64_game.unk_flags_1 = 0x00000000;
  z64_game.chest_flags = 0x00000000;
  z64_game.clear_flags = 0x00000000;
  z64_game.temp_clear_flags = 0x00000000;
  z64_game.collect_flags = 0x00000000;
  z64_game.temp_collect_flags = 0x00000000;
}

static void set_scene_flags_proc(struct menu_item *item, void *data)
{
  z64_game.swch_flags = 0xFFFFFFFF;
  z64_game.temp_swch_flags = 0xFFFFFFFF;
  z64_game.unk_flags_0 = 0xFFFFFFFF;
  z64_game.unk_flags_1 = 0xFFFFFFFF;
  z64_game.chest_flags = 0xFFFFFFFF;
  z64_game.clear_flags = 0xFFFFFFFF;
  z64_game.temp_clear_flags = 0xFFFFFFFF;
  z64_game.collect_flags = 0xFFFFFFFF;
  z64_game.temp_collect_flags = 0xFFFFFFFF;
}

static void load_room_proc(struct menu_item *item, void *data)
{
  uint8_t new_room_index = menu_intinput_get(data);
  if (new_room_index < z64_game.n_rooms) {
    if (new_room_index == z64_game.room_ctxt.rooms[0].index) {
      z64_game.room_ctxt.rooms[0].index = -1;
      z64_game.room_ctxt.rooms[0].file = NULL;
      z64_UnloadRoom(&z64_game, &z64_game.room_ctxt);
      z64_game.room_ctxt.rooms[0].index = -1;
      z64_game.room_ctxt.rooms[0].file = NULL;
    }
    else {
      z64_game.room_ctxt.rooms[0].index = -1;
      z64_game.room_ctxt.rooms[0].file = NULL;
      z64_UnloadRoom(&z64_game, &z64_game.room_ctxt);
      z64_game.room_ctxt.rooms[0].index = -1;
      z64_game.room_ctxt.rooms[0].file = NULL;
      z64_LoadRoom(&z64_game, &z64_game.room_ctxt, new_room_index);
    }
  }
}

static int col_view_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    if (gz.col_view_state == COLVIEW_INACTIVE)
      gz.col_view_state = COLVIEW_START;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    if (gz.col_view_state != COLVIEW_INACTIVE)
      gz.col_view_state = COLVIEW_STOP;
  }
  else if (reason == MENU_CALLBACK_THINK) {
    _Bool state = gz.col_view_state == COLVIEW_START ||
                  gz.col_view_state == COLVIEW_ACTIVE ||
                  gz.col_view_state == COLVIEW_RESTART ||
                  gz.col_view_state == COLVIEW_RESTARTING;
    if (menu_checkbox_get(item) != state)
      menu_checkbox_set(item, state);
  }
  return 0;
}

static int holl_view_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    if (gz.holl_view_state == HOLLVIEW_INACTIVE)
      gz.holl_view_state = HOLLVIEW_START;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    if (gz.holl_view_state != HOLLVIEW_INACTIVE)
      gz.holl_view_state = HOLLVIEW_BEGIN_STOP;
  }
  else if (reason == MENU_CALLBACK_THINK) {
    _Bool state = gz.holl_view_state == HOLLVIEW_START ||
                  gz.holl_view_state == HOLLVIEW_ACTIVE;
    if (menu_checkbox_get(item) != state)
      menu_checkbox_set(item, state);
  }
  return 0;
}

static int holl_view_xlu_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.holl_view_xlu = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.holl_view_xlu = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.holl_view_xlu);
  return 0;
}

static int holl_view_all_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.holl_view_all = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.holl_view_all = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.holl_view_all);
  return 0;
}

static int col_view_mode_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->bits.col_view_mode)
      menu_option_set(item, settings->bits.col_view_mode);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->bits.col_view_mode = menu_option_get(item);
  return 0;
}

static int col_view_xlu_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_xlu = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_xlu = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_xlu);
  return 0;
}

static int col_view_water_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_water = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_water = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_water);
  return 0;
}

static int col_view_wfc_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_wfc = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_wfc = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_wfc);
  return 0;
}

static int col_view_line_proc(struct menu_item *item,
                              enum menu_callback_reason reason,
                              void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_line = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_line = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_line);
  return 0;
}

static int col_view_shade_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_shade = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_shade = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_shade);
  return 0;
}

static int col_view_rd_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_rd = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_rd = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_rd);
  return 0;
}

static int col_view_upd_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.col_view_upd = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.col_view_upd = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.col_view_upd);
  return 0;
}

static int hit_view_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    if (gz.hit_view_state == HITVIEW_INACTIVE)
      gz.hit_view_state = HITVIEW_START;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    if (gz.hit_view_state != HITVIEW_INACTIVE)
      gz.hit_view_state = HITVIEW_STOP;
  }
  else if (reason == MENU_CALLBACK_THINK) {
    _Bool state = gz.hit_view_state == HITVIEW_START ||
                  gz.hit_view_state == HITVIEW_ACTIVE;
    if (menu_checkbox_get(item) != state)
      menu_checkbox_set(item, state);
  }
  return 0;
}

static int hit_view_at_proc(struct menu_item *item,
                       enum menu_callback_reason reason,
                       void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hit_view_at = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hit_view_at = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.hit_view_at);
  return 0;
}

static int hit_view_ac_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hit_view_ac = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hit_view_ac = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.hit_view_ac);
  return 0;
}

static int hit_view_oc_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hit_view_oc = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hit_view_oc = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.hit_view_oc);
  return 0;
}

static int hit_view_xlu_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hit_view_xlu = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hit_view_xlu = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.hit_view_xlu);
  return 0;
}

static int hit_view_shade_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.hit_view_shade = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.hit_view_shade = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.hit_view_shade);
  return 0;
}

static int path_view_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    if (gz.path_view_state == PATHVIEW_INACTIVE)
      gz.path_view_state = PATHVIEW_START;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    if (gz.path_view_state != PATHVIEW_INACTIVE)
      gz.path_view_state = PATHVIEW_STOP;
  }
  else if (reason == MENU_CALLBACK_THINK) {
    _Bool state = gz.path_view_state == PATHVIEW_START ||
                  gz.path_view_state == PATHVIEW_ACTIVE ||
                  gz.path_view_state == PATHVIEW_RESTART ||
                  gz.path_view_state == PATHVIEW_RESTARTING;
    if (menu_checkbox_get(item) != state)
      menu_checkbox_set(item, state);
  }
  return 0;
}

static int path_view_xlu_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.path_view_xlu = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.path_view_xlu = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.path_view_xlu);
  return 0;
}

static int path_view_points_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.path_view_points = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.path_view_points = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.path_view_points);
  return 0;
}

static int path_view_lines_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    settings->bits.path_view_lines = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    settings->bits.path_view_lines = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, settings->bits.path_view_lines);
  return 0;
}

static void set_cam_input_mask(void)
{
  if (gz.free_cam && !gz.lock_cam) {
      gz_set_input_mask(BUTTON_C_RIGHT | BUTTON_C_LEFT | BUTTON_C_DOWN |
                        BUTTON_C_UP | BUTTON_Z | BUTTON_L, 0xFF, 0xFF);
  }
  else
    gz_set_input_mask(BUTTON_L, 0x00, 0x00);
}

static int enable_cam_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    gz.free_cam = 1;
    set_cam_input_mask();
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    gz.free_cam = 0;
    set_cam_input_mask();
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, gz.free_cam);
  return 0;
}

static int lock_cam_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON) {
    gz.lock_cam = 1;
    set_cam_input_mask();
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF) {
    gz.lock_cam = 0;
    set_cam_input_mask();
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, gz.lock_cam);
  return 0;
}

static void reset_cam_proc(struct menu_item *item, void *data)
{
  gz.cam_yaw = 0.f;
  gz.cam_pitch = 0.f;
  gz.cam_pos.x = 0.f;
  gz.cam_pos.y = 0.f;
  gz.cam_pos.z = 0.f;
}

static int cam_mode_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    gz.cam_mode = menu_option_get(item);
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_option_get(item) != gz.cam_mode)
      menu_option_set(item, gz.cam_mode);
  }
  return 0;
}

static int cam_bhv_proc(struct menu_item *item,
                        enum menu_callback_reason reason,
                        void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    gz.cam_bhv = menu_option_get(item);
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_option_get(item) != gz.cam_bhv)
      menu_option_set(item, gz.cam_bhv);
  }
  return 0;
}

static int cam_dist_min_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    gz.cam_dist_min = menu_intinput_gets(item);
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_intinput_gets(item) != gz.cam_dist_min)
      menu_intinput_set(item, gz.cam_dist_min);
  }
  return 0;
}

static int cam_dist_max_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    gz.cam_dist_max = menu_intinput_gets(item);
  else if (reason == MENU_CALLBACK_THINK) {
    if (menu_intinput_gets(item) != gz.cam_dist_max)
      menu_intinput_set(item, gz.cam_dist_max);
  }
  return 0;
}

static int hide_rooms_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    gz.hide_rooms = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    gz.hide_rooms = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, gz.hide_rooms);
  return 0;
}

static int hide_actors_proc(struct menu_item *item,
                            enum menu_callback_reason reason,
                            void *data)
{
  if (reason == MENU_CALLBACK_SWITCH_ON)
    gz.hide_actors = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    gz.hide_actors = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, gz.hide_actors);
  return 0;
}

static void teleport_dec_proc(struct menu_item *item, void *data)
{
  settings->teleport_slot += SETTINGS_TELEPORT_MAX - 1;
  settings->teleport_slot %= SETTINGS_TELEPORT_MAX;
}

static void teleport_inc_proc(struct menu_item *item, void *data)
{
  settings->teleport_slot += 1;
  settings->teleport_slot %= SETTINGS_TELEPORT_MAX;
}

struct menu *gz_scene_menu(void)
{
  static struct menu menu;
  static struct menu explorer;
  static struct menu collision;
  static struct menu viewers;
  static struct menu camera;
  struct menu_item *item;

  /* initialize menus */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&collision, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&viewers, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&camera, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* populate scene menu */
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  /* explorer */
  explorer_create(&explorer);
  menu_add_submenu(&menu, 0, 1, &explorer, "explorer");
  gz.menu_explorer = &explorer;
  /* scene controls */
  menu_add_button(&menu, 0, 2, "set entrance point", set_entrance_proc, NULL);
  menu_add_button(&menu, 0, 3, "clear flags", clear_scene_flags_proc, NULL);
  menu_add_button(&menu, 0, 4, "set flags", set_scene_flags_proc, NULL);
  item = menu_add_intinput(&menu, 16, 5, 10, 2, NULL, NULL);
  menu_add_button(&menu, 0, 5, "load room", load_room_proc, item);
  /* visuals controls */
  menu_add_submenu(&menu, 0, 6, &collision, "collision");
  menu_add_submenu(&menu, 0, 7, &viewers, "viewers");
  menu_add_submenu(&menu, 0, 8, &camera, "free camera");
  menu_add_static(&menu, 0, 9, "hide rooms", 0xC0C0C0);
  menu_add_checkbox(&menu, 16, 9, hide_rooms_proc, NULL);
  menu_add_static(&menu, 0, 10, "hide actors", 0xC0C0C0);
  menu_add_checkbox(&menu, 16, 10, hide_actors_proc, NULL);
  /* teleport controls */
  menu_add_static(&menu, 0, 11, "teleport slot", 0xC0C0C0);
  menu_add_watch(&menu, 18, 11,
                 (uint32_t)&settings->teleport_slot, WATCH_TYPE_U8);
  menu_add_button(&menu, 16, 11, "-", teleport_dec_proc, NULL);
  menu_add_button(&menu, 20, 11, "+", teleport_inc_proc, NULL);
  /* scene info watches */
  menu_add_static(&menu, 0, 12, "current scene", 0xC0C0C0);
  menu_add_watch(&menu, 16, 12,
                 (uint32_t)&z64_game.scene_index, WATCH_TYPE_U16);
  menu_add_static(&menu, 0, 13, "current room", 0xC0C0C0);
  menu_add_watch(&menu, 16, 13,
                 (uint32_t)&z64_game.room_ctxt.rooms[0].index, WATCH_TYPE_U8);
  menu_add_static(&menu, 0, 14, "no. rooms", 0xC0C0C0);
  menu_add_watch(&menu, 16, 14, (uint32_t)&z64_game.n_rooms, WATCH_TYPE_U8);

  /* populate collision menu */
  collision.selector = menu_add_submenu(&collision, 0, 0, NULL, "return");
  /* collision view controls */
  menu_add_static(&collision, 0, 1, "show collision", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 1, col_view_proc, NULL);
  menu_add_static(&collision, 2, 2, "mode", 0xC0C0C0);
  menu_add_option(&collision, 16, 2, "decal\0""surface\0",
                  col_view_mode_proc, NULL);
  menu_add_static(&collision, 2, 3, "translucent", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 3, col_view_xlu_proc, NULL);
  menu_add_static(&collision, 2, 4, "waterboxes", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 4, col_view_water_proc, NULL);
  menu_add_static(&collision, 2, 5, "polygon class", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 5, col_view_wfc_proc, NULL);
  menu_add_static(&collision, 2, 6, "wireframe", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 6, col_view_line_proc, NULL);
  menu_add_static(&collision, 2, 7, "shaded", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 7, col_view_shade_proc, NULL);
  menu_add_static(&collision, 2, 8, "reduced", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 8, col_view_rd_proc, NULL);
  menu_add_static(&collision, 2, 9, "auto update", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 9, col_view_upd_proc, NULL);
  /* hitbox view controls */
  menu_add_static(&collision, 0, 10, "show colliders", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 10, hit_view_proc, NULL);
  menu_add_static(&collision, 2, 11, "hit", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 11, hit_view_at_proc, NULL);
  menu_add_static(&collision, 2, 12, "hurt", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 12, hit_view_ac_proc, NULL);
  menu_add_static(&collision, 2, 13, "bump", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 13, hit_view_oc_proc, NULL);
  menu_add_static(&collision, 2, 14, "translucent", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 14, hit_view_xlu_proc, NULL);
  menu_add_static(&collision, 2, 15, "shaded", 0xC0C0C0);
  menu_add_checkbox(&collision, 16, 15, hit_view_shade_proc, NULL);

  /* populate viewers menu */
  viewers.selector = menu_add_submenu(&viewers, 0, 0, NULL, "return");
  /* path view controls */
  menu_add_static(&viewers, 0, 1, "show paths", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 1, path_view_proc, NULL);
  menu_add_static(&viewers, 2, 2, "points", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 2, path_view_points_proc, NULL);
  menu_add_static(&viewers, 2, 3, "lines", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 3, path_view_lines_proc, NULL);
  menu_add_static(&viewers, 2, 4, "translucent", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 4, path_view_xlu_proc, NULL);
  /* holl view controls */
  menu_add_static(&viewers, 0, 5, "show transitions", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 5, holl_view_proc, NULL);
  menu_add_static(&viewers, 2, 6, "translucent", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 6, holl_view_xlu_proc, NULL);
  menu_add_static(&viewers, 2, 7, "show inactive", 0xC0C0C0);
  menu_add_checkbox(&viewers, 16, 7, holl_view_all_proc, NULL);

  /* populate camera menu */
  camera.selector = menu_add_submenu(&camera, 0, 0, NULL, "return");
  menu_add_static(&camera, 0, 1, "enable", 0xC0C0C0);
  menu_add_checkbox(&camera, 16, 1, enable_cam_proc, NULL);
  menu_add_static(&camera, 0, 2, "lock", 0xC0C0C0);
  menu_add_checkbox(&camera, 16, 2, lock_cam_proc, NULL);
  menu_add_static(&camera, 0, 3, "mode", 0xC0C0C0);
  menu_add_option(&camera, 16, 3, "camera\0" "view\0", cam_mode_proc, NULL);
  menu_add_static(&camera, 0, 4, "behavior", 0xC0C0C0);
  menu_add_option(&camera, 16, 4,
                  "manual\0" "birdseye follow\0" "radial follow\0",
                  cam_bhv_proc, NULL);
  menu_add_static(&camera, 0, 5, "distance min", 0xC0C0C0);
  menu_add_intinput(&camera, 16, 5, -10, 5, cam_dist_min_proc, NULL);
  menu_add_static(&camera, 0, 6, "distance max", 0xC0C0C0);
  menu_add_intinput(&camera, 16, 6, -10, 5, cam_dist_max_proc, NULL);
  menu_add_button(&camera, 16, 7, "reset", reset_cam_proc, item);

  return &menu;
}
