#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"
#include "trainer.h"

struct roll roll;
bomb_t bomb1;


_Bool is_rolling()
{
  if(z64_link.current_animation == 0x04003038)
    return 1;
  else
    return 0;
}

_Bool roll_pressed()
{
  if(is_rolling() && (z64_link.animation_timer == 0))
    return 1;
  else
    return 0;
}

void update_roll()
{
  if(roll_pressed() && !roll.timer_active){
    roll.timer_active = 1;
    roll.is_first_roll = 1;
    roll.last_roll_frame = 0;
  } else if (roll_pressed() && roll.timer_active){
    roll.is_first_roll = 0;
    roll.last_roll_frame = roll.timer;
    roll.timer = 0;
  }

  if(roll.timer_active){
    roll.timer++;
  } else{
    roll.timer = 0;
  }

  if(roll.timer == 30){
    roll.timer_active = 0;
  }
}

static void update_bomb_timers(bomb_t* bomb)
{
  if(bomb->instance){
    bomb->raw_timer = bomb->instance->timer;

    if(bomb->instance->timer == 0x45){
      bomb->cooking = 1;
      bomb->cook_timer = 0x45;
    }

    if((bomb->cooking) && (bomb->instance->timer < 0x45)){
      bomb->cook_timer--;
    }

    if((bomb->cooking) && (bomb->cook_timer == 0)){
      bomb->cooking = 0;
      bomb->exploding = 1;
      bomb->explode_timer = 0xA;
    }

    if((bomb->exploding) && (bomb->instance->timer < 0xA)){
        bomb->explode_timer--;
    }

    if((bomb->exploding) && (bomb->explode_timer == 0)){
      bomb->exploding = 0;
      bomb->cook_timer = 0;
      bomb->explode_timer = 0;
    }
  }
}

void update_bombs()
{
  bomb1.instance = (z64_bomb_t*)z64_game.actor_list[3].first;
  update_bomb_timers(&bomb1);
}

/* set color functions */
void set_rgb_green()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x00, 0xFF, 0x2F, 0xFF));
}

void set_rgb_lgreen()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x7D, 0xFF, 0x95, 0xFF));
}

void set_rgb_yellow()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xEF, 0xFF, 0x20, 0xFF));
}

void set_rgb_orange()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xA5, 0x10, 0xFF));
}

void set_rgb_red()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0x24, 0x24, 0xFF));
}

void set_rgb_white()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, 0xFF));
}
