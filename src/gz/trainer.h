#ifndef TRAINER_H
#define TRAINER_H

#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"

enum animation
{
  ANIM_ROLL        = 0x04003038,
  ANIM_ROLL_SHIELD = 0x04003030,
  ANIM_SIDEHOP_L   = 0x04002950,
  ANIM_LANDING_L   = 0x04002960,
  ANIM_SIDEHOP_R   = 0x04002988,
  ANIM_LANDING_R   = 0x04002998,

};

struct roll
{
  uint8_t     timer;
  uint8_t     timer_active;
  int         streak;
  uint8_t     is_first_roll;
  uint8_t     last_roll_frame;
};

struct hess
{
  int   roll_frame;
  _Bool ess;
  _Bool zr_same;
  int   zr_frame;
  int   setup;
};

struct sidehop
{
  _Bool     sidehopping;
  _Bool     landing;
  _Bool     sidehop_timer_active;
  _Bool     land_timer_active;
  uint8_t   sidehop_timer;
  uint8_t   land_timer;
  int8_t    a_press;
  /*
    Negative values: frames early
    Zero: default
    One: Successful
    Greater than one: frames late + 1
  */
  int8_t    result;
  uint16_t  pad_prev;
  uint8_t   streak;
};

struct equip_swap
{
  _Bool     changing_screen;
  uint8_t   timer;
  int       c_button_press_time;
  int       control_stick_moved_time;
  int8_t    x_prev;
  int8_t    y_prev;
  uint16_t  pad_prev;
  uint8_t   streak;
  _Bool     diagonal_warning;
};

_Bool is_rolling();
_Bool roll_pressed();
void update_roll();
void roll_check_streak();

_Bool update_sidehop();

void update_equip_swap();

void set_rgb_green();
void set_rgb_lgreen();
void set_rgb_yellow();
void set_rgb_orange();
void set_rgb_red();
void set_rgb_white();

extern struct roll roll;
extern struct sidehop sidehop;
extern struct hess hess;
extern struct equip_swap equip_swap;

#endif
