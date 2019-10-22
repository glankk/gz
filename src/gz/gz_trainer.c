#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"

static uint8_t timer;
static _Bool   timer_active;
static int     streak;

static uint8_t last_roll;
static _Bool   is_first_roll;

static z64_actor_t* bomb_ptr;
static uint16_t bomb_timer;

static void set_rgb_green()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x00, 0xFF, 0x2F, 0xFF));
}

static void set_rgb_lgreen()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x7D, 0xFF, 0x95, 0xFF));
}

//make yellow and orange easier to tell apart on console
static void set_rgb_yellow()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xEF, 0xFF, 0x20, 0xFF));
}

static void set_rgb_orange()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xBE, 0x4F, 0xFF));
}

static void set_rgb_red()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0x24, 0x24, 0xFF));
}

static void set_rgb_white()
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, 0xFF));
}

static _Bool is_rolling()
{
  if(z64_link.current_animation == 0x04003038)
    return 1;
  else
    return 0;
}

static _Bool roll_pressed()
{
  if(is_rolling() && (z64_link.animation_timer == 0))
    return 1;
  else
    return 0;
}

static void update_timer()
{
  if(gz.frames_queued == 0){
    return;
  }

  if(roll_pressed() && !timer_active){
    timer_active = 1;
    is_first_roll = 1;
    last_roll = 0;
  } else if (roll_pressed() && timer_active){
    is_first_roll = 0;
    last_roll = timer;
    timer = 0;
  }

  if(timer_active){
    timer++;
  } else{
    timer = 0;
  }

  if(timer == 30){
    timer_active = 0;
  }
}

static void update_bomb()
{
  bomb_ptr = z64_game.actor_list[3].first;
  bomb_timer = *((uint16_t*) ((uint8_t*)bomb_ptr + 0x01E8));
}

static int roll_timing_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;

  update_timer();

  if((roll_pressed()) && (last_roll == 16)){
    streak++;
  }
  if(last_roll != 16){
    streak = 0;
  }
  set_rgb_white();
  gfx_printf(font, x, y + ch * 1, "streak: %i", streak);

  if (!is_first_roll && timer_active){
    if((last_roll < 14) || (last_roll > 19)){
      set_rgb_red();
      int amnt = last_roll - 16;
      if (amnt < 0)
        gfx_printf(font, x, y + ch * 2, "bad (%i frames early)", abs(amnt));
      if (amnt > 0)
        gfx_printf(font, x, y + ch * 2, "bad (%i frames late)", abs(amnt));
    }else{
      switch(last_roll)
      {
        case 14: set_rgb_orange();
                 gfx_printf(font, x, y + ch * 2, "okay (2 frames early)");
                 break;
        case 15: set_rgb_lgreen();
                 gfx_printf(font, x, y + ch * 2, "good (1 frame early)");
                 break;
        case 16: set_rgb_green();
                 gfx_printf(font, x, y + ch * 2, "perfect! (frame perfect)");
                 break;
        case 17: set_rgb_lgreen();
                 gfx_printf(font, x, y + ch * 2, "good (1 frame late)");
                 break;
        case 18: set_rgb_yellow();
                 gfx_printf(font, x, y + ch * 2, "okay (2 frames late)");
                 break;
        case 19: set_rgb_orange();
                 gfx_printf(font, x, y + ch * 2, "okay (3 frames late)");
                 break;
        default: break;
      }
    }
  }

  return 1;
}

static int bomb_hess_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;

  update_bomb();

  set_rgb_white();
  gfx_printf(font, x, y + ch * 0, "%8x", bomb_ptr);
  gfx_printf(font, x, y + ch * 1, "%4x", bomb_timer);
  return 1;
}

struct menu *gz_trainer_menu(void)
{
  static struct menu menu;
  static struct menu roll_timing;
  static struct menu bomb_hess;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&roll_timing, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&bomb_hess, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* populate trainer top menu*/
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  menu_add_submenu(&menu, 0, 1, &roll_timing, "roll timing");
  menu_add_submenu(&menu, 0, 2, &bomb_hess, "bomb hess");

  /*populate roll timing menu*/
  roll_timing.selector = menu_add_submenu(&roll_timing, 0, 0, NULL, "return");
  menu_add_static_custom(&roll_timing, 0, 1, roll_timing_draw_proc, NULL, 0xFFFFFF);

  /*populate bomb_hess menu*/
  bomb_hess.selector = menu_add_submenu(&bomb_hess, 0, 0, NULL, "return");
  menu_add_static_custom(&bomb_hess, 0, 1, bomb_hess_draw_proc, NULL, 0xFFFFFF);

  return &menu;
}
