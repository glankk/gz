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

enum hess_setup
{
  HESS_UNKOWN,
  HESS_ROLL_INSTA,
  HESS_ROLL_OVERHEAD,
  HESS_BACKWALK_3,
  HESS_BACKWALK_4
};

typedef struct
{
  z64_actor_t common;          /* 0x0000 */
  char        unk_0x13C[0xAC]; /* 0x013C */
  uint16_t    timer;           /* 0x01E8 */
  uint16_t    state;           /* 0x01EA */
  char        unk_0x1EA[0x1C]; /* 0x01EC */
}z64_bomb_t;                   /* 0x0208 */

typedef struct
{
  z64_bomb_t* instance;
  uint16_t    timer;
  float       distance_from_link;
}bomb_t;

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
  _Bool   sidehopping;
  _Bool   landing;
  _Bool   sidehop_timer_active;
  _Bool   land_timer_active;
  uint8_t sidehop_timer;
  uint8_t land_timer;
  int     a_press;
  int     result;
};

_Bool is_rolling();
_Bool roll_pressed();
void update_roll();
void check_streak();

void update_sidehop();

void update_bombs();
void hess_roll(bomb_t* bomb);
void hess_setup(bomb_t* bomb);

void set_rgb_green();
void set_rgb_lgreen();
void set_rgb_yellow();
void set_rgb_orange();
void set_rgb_red();
void set_rgb_white();

extern struct roll roll;
extern struct sidehop sidehop;
extern struct hess hess;
extern bomb_t bomb1;

#endif
