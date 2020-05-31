#ifndef GZ_H
#define GZ_H
#include <stdint.h>
#include <vector/vector.h>
#include <n64.h>
#include "settings.h"
#include "z64.h"
#include "zu.h"

enum cmdact
{
  CMDACT_HOLD,
  CMDACT_PRESS,
  CMDACT_PRESS_ONCE,
};

struct command_info
{
  const char           *name;
  void                (*proc)(void);
  enum cmdact           activation_type;
};

enum col_view_state
{
  COLVIEW_INACTIVE,
  COLVIEW_START,
  COLVIEW_ACTIVE,
  COLVIEW_BEGIN_STOP,
  COLVIEW_STOP,
  COLVIEW_BEGIN_RESTART,
  COLVIEW_RESTART,
};

enum hit_view_state
{
  HITVIEW_INACTIVE,
  HITVIEW_START,
  HITVIEW_ACTIVE,
  HITVIEW_BEGIN_STOP,
  HITVIEW_STOP,
};

enum cam_mode
{
  CAMMODE_CAMERA,
  CAMMODE_VIEW,
};

enum cam_bhv
{
  CAMBHV_MANUAL,
  CAMBHV_BIRDSEYE,
  CAMBHV_RADIAL,
};

struct memory_file
{
  z64_file_t            z_file;
  _Bool                 entrance_override;
  int32_t               next_entrance;
  uint16_t              scene_index;
  uint32_t              scene_flags[9];
  qs510_t               start_icon_dd;
  uint16_t              pause_screen;
  int16_t               item_screen_cursor;
  int16_t               quest_screen_cursor;
  int16_t               equip_screen_cursor;
  int16_t               map_screen_cursor;
  int16_t               item_screen_x;
  int16_t               equipment_screen_x;
  int16_t               item_screen_y;
  int16_t               equipment_screen_y;
  int16_t               pause_screen_cursor;
  int16_t               quest_screen_item;
  int16_t               quest_screen_hilite;
  int16_t               quest_screen_song;
};

enum movie_state
{
  MOVIE_IDLE,
  MOVIE_RECORDING,
  MOVIE_PLAYING,
};

struct movie_input
{
  z64_controller_t      raw;            /* 0x0000 */
  uint16_t              pad_delta;      /* 0x0004 */
                                        /* 0x0006 */
};

struct movie_seed
{
  int                   frame_idx;
  uint32_t              old_seed;
  uint32_t              new_seed;
};

struct movie_oca_input
{
  int32_t               frame_idx;
  uint16_t              pad;
  int8_t                adjusted_x;
  int8_t                adjusted_y;
};

struct movie_oca_sync
{
  int32_t               frame_idx;
  int32_t               audio_frames;
};

struct movie_room_load
{
  int32_t               frame_idx;
};

struct log_entry
{
  char                 *msg;
  int                   age;
};

struct gz
{
  _Bool                 ready;
  uint8_t               profile;
  struct menu          *menu_global;
  struct menu          *menu_main;
  struct menu          *menu_explorer;
  struct menu          *menu_watches;
  struct menu          *menu_mem;
  struct menu_item     *menu_watchlist;
  _Bool                 menu_active;
  struct log_entry      log[SETTINGS_LOG_MAX];
  _Bool                 entrance_override_once;
  _Bool                 entrance_override_next;
  int32_t               next_entrance;
  uint16_t              day_time_prev;
  int                   target_day_time;
  int32_t               frames_queued;
  struct zu_disp_p      z_disp_p;
  uint32_t              disp_hook_size[4];
  uint32_t              disp_hook_p[4];
  uint32_t              disp_hook_d[4];
  enum movie_state      movie_state;
  z64_controller_t      movie_input_start;
  struct vector         movie_input;
  struct vector         movie_seed;
  struct vector         movie_oca_input;
  struct vector         movie_oca_sync;
  struct vector         movie_room_load;
  int                   movie_frame;
  int                   movie_seed_pos;
  int                   movie_oca_input_pos;
  int                   movie_oca_sync_pos;
  int                   movie_room_load_pos;
  _Bool                 oca_input_flag;
  _Bool                 oca_sync_flag;
  _Bool                 room_load_flag;
  z64_controller_t      z_input_mask;
  _Bool                 vcont_enabled[4];
  z64_input_t           vcont_input[4];
  int32_t               frame_counter;
  int32_t               lag_vi_offset;
  int64_t               cpu_counter;
  int32_t               cpu_counter_freq;
  _Bool                 timer_active;
  int64_t               timer_counter_offset;
  int64_t               timer_counter_prev;
  int                   col_view_state;
  int                   hit_view_state;
  _Bool                 hide_rooms;
  _Bool                 hide_actors;
  _Bool                 free_cam;
  _Bool                 lock_cam;
  enum cam_mode         cam_mode;
  enum cam_bhv          cam_bhv;
  int16_t               cam_dist_min;
  int16_t               cam_dist_max;
  float                 cam_pitch;
  float                 cam_yaw;
  z64_xyzf_t            cam_pos;
  struct memory_file   *memfile;
  _Bool                 memfile_saved[SETTINGS_MEMFILE_MAX];
  uint8_t               memfile_slot;
  void                 *state_buf[SETTINGS_STATE_MAX];
  uint8_t               state_slot;
  _Bool                 reset_flag;
  _Bool                 frame_flag;
};

void          gz_apply_settings();
void          gz_show_menu(void);
void          gz_hide_menu(void);
void          gz_log(const char *fmt, ...);
void          gz_save_memfile(struct memory_file *memfile);
void          gz_load_memfile(struct memory_file *memfile);
void          gz_warp(int16_t entrance_index,
                      uint16_t cutscene_index, int age);
void          gz_set_input_mask(uint16_t pad, uint8_t x, uint8_t y);

void          command_break(void);
void          command_levitate(void);
void          command_fall(void);
void          command_turbo(void);
void          command_fileselect(void);
void          command_reload(void);
void          command_void(void);
void          command_age(void);
void          command_savestate(void);
void          command_loadstate(void);
void          command_savememfile(void);
void          command_loadmemfile(void);
void          command_savepos(void);
void          command_loadpos(void);
void          command_prevstate(void);
void          command_nextstate(void);
void          command_prevfile(void);
void          command_nextfile(void);
void          command_prevpos(void);
void          command_nextpos(void);
void          command_pause(void);
void          command_advance(void);
void          command_recordmacro(void);
void          command_playmacro(void);
void          command_colview(void);
void          command_hitview(void);
void          command_resetlag(void);
void          command_togglewatches(void);
void          command_timer(void);
void          command_resettimer(void);
void          command_starttimer(void);
void          command_stoptimer(void);
void          command_reset(void);

void          z_to_movie(int movie_frame, z64_input_t *zi, _Bool reset);
void          movie_to_z(int movie_frame, z64_input_t *zi, _Bool *reset);
void          gz_movie_rewind(void);
void          gz_movie_seek(int frame);

void          gz_vcont_set(int port, _Bool plugged, z64_controller_t *cont);
void          gz_vcont_get(int port, z64_input_t *input);

void          gz_col_view(void);
void          gz_hit_view(void);

void          gz_update_cam(void);
void          gz_free_view(void);

struct menu  *gz_warps_menu(void);
struct menu  *gz_scene_menu(void);
struct menu  *gz_cheats_menu(void);
struct menu  *gz_inventory_menu(void);
struct menu  *gz_equips_menu(void);
struct menu  *gz_file_menu(void);
struct menu  *gz_macro_menu(void);
struct menu  *gz_debug_menu(void);
struct menu  *gz_settings_menu(void);

extern struct gz            gz;
extern struct command_info  command_info[COMMAND_MAX];

#endif
