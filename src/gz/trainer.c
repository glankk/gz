#include <stdlib.h>
#include <math.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"
#include "input.h"
#include "trainer.h"


struct roll roll;
struct sidehop sidehop;
struct hess hess;
struct equip_swap equip_swap;

_Bool is_rolling()
{
  if((z64_link.current_animation == ANIM_ROLL) || (z64_link.current_animation == ANIM_ROLL_SHIELD))
    return 1;
  else
    return 0;
}

_Bool is_sidehopping()
{
  if((z64_link.current_animation == ANIM_SIDEHOP_L) || (z64_link.current_animation == ANIM_SIDEHOP_R))
    return 1;
  else
    return 0;
}

_Bool is_landing()
{
  if((z64_link.current_animation == ANIM_LANDING_L) || (z64_link.current_animation == ANIM_LANDING_R))
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

_Bool sidehop_pressed()
{
  if(is_sidehopping() && (z64_link.animation_timer == 0))
    return 1;
  else
    return 0;
}

_Bool sidehop_a_pressed()
{
  uint16_t pad = z64_game.common.input[0].raw.pad;
  if(pad & BUTTON_A && !(sidehop.pad_prev & BUTTON_A)){
    return 1;
  }else{
    return 0;
  }
}

_Bool z_pressed()
{
  uint16_t pad = input_pad();
  if (pad & BUTTON_Z){
    return 1;
  } else {
    return 0;
  }
}

_Bool r_pressed()
{
  uint16_t pad = input_pad();
  if (pad & BUTTON_R){
    return 1;
  } else {
    return 0;
  }
}

_Bool equip_button_pressed()
{
  uint16_t pad = input_pad();
  if (pad & BUTTON_C_LEFT || pad & BUTTON_C_DOWN || pad & BUTTON_C_RIGHT){
    return 1;
  } else {
    return 0;
  }
}

// Using z64_game.common.input[0].raw_prev was inconsistent so I store previous input myself
_Bool equip_swap_equip_button_pressed_previous()
{
  uint16_t pad = equip_swap.pad_prev;
  if (pad & BUTTON_C_LEFT || pad & BUTTON_C_DOWN || pad & BUTTON_C_RIGHT){
    return 1;
  } else {
    return 0;
  }
}

int16_t equip_swap_control_stick_x_previous()
{
  return equip_swap.x_prev;
}

int16_t equip_swap_control_stick_y_previous()
{
  return equip_swap.y_prev;
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

void roll_check_streak()
{
  if((roll_pressed()) && (roll.last_roll_frame == 16)){
    roll.streak++;
    if (roll.streak > settings->trainer_roll_pb) {
      settings->trainer_roll_pb = roll.streak;
      settings_save(gz.profile);
    }
  }
  if(roll.last_roll_frame != 16){
    roll.streak = 0;
  }
}

// returns whether or not a new log entry should be added
_Bool update_sidehop()
{
  _Bool ret = 0;
  if(sidehop_pressed()){
    sidehop.sidehop_timer = 0;
    sidehop.sidehop_timer_active = 1;
  }

  if(sidehop.sidehop_timer_active && (sidehop_a_pressed())){
    sidehop.a_press = sidehop.sidehop_timer;
    // if they may be early
    if (sidehop.result == 0)
      ret = 1;
  }

  if(sidehop.sidehop_timer_active){
    sidehop.sidehop_timer++;
  }

  if(is_landing()) {
    // if they pressed a early
    if (!sidehop.land_timer_active && sidehop.a_press != 0) {
      sidehop.result = sidehop.a_press - sidehop.sidehop_timer;
      ret = 1;
    }
    sidehop.land_timer_active = 1;
    sidehop.landing = 1;
  } else {
    sidehop.landing = 0;
  }

  if(sidehop_pressed() && sidehop.land_timer_active){
    if (sidehop.result == 0) {
      sidehop.result = sidehop.land_timer;
      ret = 1;
    }
    sidehop.land_timer = 0;
    sidehop.land_timer_active = 0;
  }

  if(sidehop.land_timer_active){
    sidehop.land_timer++;
  }

  if(sidehop.land_timer == 20){
    sidehop.land_timer = 0;
    sidehop.sidehop_timer = 0;
    sidehop.land_timer_active = 0;
    sidehop.sidehop_timer_active = 0;
    sidehop.result = 0;
    sidehop.a_press = 0;
  }

  // reset the result after they pressed a early then sucessfully sidehopped
  if (sidehop.result < 0 && sidehop_pressed()) {
    sidehop.result = 0;
  }

  // manually store previous inputs
  sidehop.pad_prev = input_pad();
  return ret;
}

void update_equip_swap()
{
  int8_t x = input_x();
  int8_t y = input_y();

  // 0 is items, 1 is map, 2 is quest status, 3 is equipment
  if (!equip_swap.changing_screen &&
            ((z64_game.pause_ctxt.screen_idx == 3 && r_pressed()) || (z64_game.pause_ctxt.screen_idx == 1 && z_pressed())))
  {
    equip_swap.changing_screen = 1;
    equip_swap.timer = 0;
    equip_swap.diagonal_warning = 0;
  }
  else if (equip_swap.changing_screen)
  {
    equip_swap.timer += 1;

    int8_t abs_x = abs(x);
    int8_t abs_y = abs(y);
    _Bool stickSet = 0;
    // 38 is min control stick value to move cursor
    if ((abs_x >= 38 || abs_y >= 38) &&
            abs(equip_swap_control_stick_x_previous()) < 38 && abs(equip_swap_control_stick_y_previous()) < 38)
    {
      equip_swap.control_stick_moved_time = 15 - equip_swap.timer;
      stickSet = 1;
      equip_swap.diagonal_warning = abs_x < 38 || abs_y < 38;
    }

    if (equip_button_pressed() && !equip_swap_equip_button_pressed_previous())
    {
      equip_swap.c_button_press_time = 15 - equip_swap.timer;

      if (stickSet && equip_swap.control_stick_moved_time == 0 && equip_swap.c_button_press_time == 0 &&
            !equip_swap.diagonal_warning)
      {
        /*
          They properly did equip swap so we can increment their streak and stop checking inputs.
          This also avoids letting the timer expire which clears their streak.
        */
        equip_swap.streak += 1;
        if (equip_swap.streak > settings->trainer_equip_swap_pb) {
            settings->trainer_equip_swap_pb = equip_swap.streak;
            settings_save(gz.profile);
        }

        equip_swap.changing_screen = 0;
      }
    }

    if (equip_swap.timer >= 20)
    {
      equip_swap.changing_screen = 0;
      equip_swap.streak = 0;
    }
  }

  // manually store previous inputs
  equip_swap.x_prev = x;
  equip_swap.y_prev = y;
  equip_swap.pad_prev = input_pad();
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
