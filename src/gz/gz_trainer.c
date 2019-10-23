#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"
#include "trainer.h"

static int roll_timing_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;

  update_roll();

  if((roll_pressed()) && (roll.last_roll_frame == 16)){
    roll.streak++;
  }
  if(roll.last_roll_frame != 16){
    roll.streak = 0;
  }

  set_rgb_white();
  gfx_printf(font, x, y + ch * 0, "streak: %i", roll.streak);
  int log_y = 1;

  if (!roll.is_first_roll && roll.timer_active){
    if((roll.last_roll_frame < 14) || (roll.last_roll_frame > 19)){
      set_rgb_red();
      int amnt = roll.last_roll_frame - 16;
      if (amnt < 0)
        gfx_printf(font, x, y + ch * log_y, "bad (%i frames early)", abs(amnt));
      if (amnt > 0)
        gfx_printf(font, x, y + ch * log_y, "bad (%i frames late)", abs(amnt));
    }else{
      switch(roll.last_roll_frame)
      {
        case 14: set_rgb_orange();
                 gfx_printf(font, x, y + ch * log_y, "okay (2 frames early)");
                 break;
        case 15: set_rgb_lgreen();
                 gfx_printf(font, x, y + ch * log_y, "good (1 frame early)");
                 break;
        case 16: set_rgb_green();
                 gfx_printf(font, x, y + ch * log_y, "perfect! (frame perfect)");
                 break;
        case 17: set_rgb_lgreen();
                 gfx_printf(font, x, y + ch * log_y, "good (1 frame late)");
                 break;
        case 18: set_rgb_yellow();
                 gfx_printf(font, x, y + ch * log_y, "okay (2 frames late)");
                 break;
        case 19: set_rgb_orange();
                 gfx_printf(font, x, y + ch * log_y, "okay (3 frames late)");
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

  update_bombs();

  set_rgb_white();
  gfx_printf(font, x, y + ch * 0, "instance: %8x", bomb1.instance);
  gfx_printf(font, x, y + ch * 1, "cooking: %i", bomb1.cooking);
  gfx_printf(font, x, y + ch * 2, "cook timer: %i", bomb1.cook_timer-1);
  gfx_printf(font, x, y + ch * 3, "exploding: %i", bomb1.exploding);
  gfx_printf(font, x, y + ch * 4, "explode timer: %i", bomb1.explode_timer-1);

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
