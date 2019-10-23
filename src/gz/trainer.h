#ifndef TRAINER_H
#define TRAINER_H

#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"

typedef struct
{
  z64_actor_t common;          /* 0x0000 */
  char        unk_0x13C[0xAC]; /* 0x013C */
  uint16_t    timer;      /* 0x01E8 */
  uint16_t    state;      /* 0x01EA */
  char        unk_0x1EA[0x1C]; /* 0x01EC */
}z64_bomb_t;                   /* 0x0208 */

typedef struct
{
  z64_bomb_t* instance;
  uint16_t    raw_timer;
  uint16_t    cook_timer;
  uint16_t    explode_timer;
  uint8_t     cooking;
  uint8_t     exploding;
}bomb_t;

struct roll
{
  uint8_t     timer;
  uint8_t     timer_active;
  int         streak;
  uint8_t     is_first_roll;
  uint8_t     last_roll_frame;
};

_Bool is_rolling();
_Bool roll_pressed();
void update_roll();

void update_bombs();

void set_rgb_green();
void set_rgb_lgreen();
void set_rgb_yellow();
void set_rgb_orange();
void set_rgb_red();
void set_rgb_white();

extern struct roll roll;
extern bomb_t bomb1;

#endif
