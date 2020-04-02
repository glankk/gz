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

  if(gz.frame_ran){
    update_roll();
    check_streak();
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

static int sidehop_timing_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;

  if(gz.frame_ran){
    update_sidehop();
  }

  set_rgb_white();
  int log_y = 1;
  gfx_printf(font, x, y + ch * 0, "streak: ");
  //early isnt working for some reason
  if((sidehop.a_press != 0) && (sidehop.landing)){
    set_rgb_red();
    gfx_printf(font, x,  y + ch * log_y, "early by %i frames", (sidehop.sidehop_timer - sidehop.a_press));
  }
  if(sidehop.result == 1){
    set_rgb_green();
    gfx_printf(font, x,  y + ch * log_y, "perfect! (frame perfect)");
  }
  if(sidehop.result > 1){
    set_rgb_red();
    gfx_printf(font, x,  y + ch * log_y, "late by %i frames", sidehop.result-1);
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

  if(gz.frame_ran){
    update_bombs();
    hess_setup(&bomb1);
    hess_roll(&bomb1);
    //check ess when hit
    //get z+r frame
    //check z+r frame
  }

  set_rgb_white();
  gfx_printf(font, x, y + ch * 0, "instance: %x", bomb1.instance);
  gfx_printf(font, x, y + ch * 1, "timer: %i", bomb1.timer);
  gfx_printf(font, x, y + ch * 2, "distance from link: %g", bomb1.distance_from_link);
  gfx_printf(font, x, y + ch * 3, "current animation: %8x", z64_link.current_animation);
  gfx_printf(font, x, y + ch * 4, "roll frame: %i", hess.roll_frame);
  gfx_printf(font, x, y + ch * 5, "setup: %i", hess.setup);

  return 1;
}

static int equip_swap_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{

  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;

  if(gz.frame_ran){
    update_equip_swap();
  }

  set_rgb_white();
  gfx_printf(font, x, y + ch * 0, "streak: %d", equip_swap.streak);

  if (equip_swap.c_button_press_time > 0)
  {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * 1, "c button: early by %i frames", equip_swap.c_button_press_time);
  }
  else if (equip_swap.c_button_press_time == 0)
  {
    set_rgb_green();
    gfx_printf(font, x,  y + ch * 1, "c button: perfect! (frame perfect)");
  }
  else if (equip_swap.c_button_press_time < 0)
  {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * 1, "c button: late by %i frames", -equip_swap.c_button_press_time);
  }

  if (equip_swap.control_stick_moved_time > 0)
  {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * 2, "stick: early by %i frames", equip_swap.control_stick_moved_time);
  }
  else if (equip_swap.control_stick_moved_time == 0)
  {
    set_rgb_green();
    gfx_printf(font, x,  y + ch * 2, "stick: perfect! (frame perfect)");
  }
  else if (equip_swap.control_stick_moved_time < 0)
  {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * 2, "stick: late by %i frames", -equip_swap.control_stick_moved_time);
  }
  return 1;
}

static int equip_swap_toggle_proc(struct menu_item *item,
                      enum menu_callback_reason reason,
                      void *data)
{
  struct menu_item* global_equip_swap_item = (struct menu_item*)data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
  {
    global_equip_swap_item->enabled = 1;
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
  {
    global_equip_swap_item->enabled = 0;
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, global_equip_swap_item->enabled);
  return 0;
}

struct menu *gz_trainer_menu(void)
{
  static struct menu menu;
  static struct menu roll_timing;
  static struct menu bomb_hess;
  static struct menu sidehop_timing;
  static struct menu equip_swap_timing;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&roll_timing, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&bomb_hess, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&sidehop_timing, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&equip_swap_timing, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* populate trainer top menu*/
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  menu_add_submenu(&menu, 0, 1, &roll_timing, "roll timing");
  menu_add_submenu(&menu, 0, 2, &sidehop_timing, "sidehop timing");
  menu_add_submenu(&menu, 0, 3, &bomb_hess, "bomb hess");
  menu_add_submenu(&menu, 0, 4, &equip_swap_timing, "equip swap timing");

  /*populate roll timing menu*/
  roll_timing.selector = menu_add_submenu(&roll_timing, 0, 0, NULL, "return");
  menu_add_static_custom(&roll_timing, 0, 1, roll_timing_draw_proc, NULL, 0xFFFFFF);

  /*populate sidehop timing menu*/
  sidehop_timing.selector = menu_add_submenu(&sidehop_timing, 0, 0, NULL, "return");
  menu_add_static_custom(&sidehop_timing, 0, 1, sidehop_timing_draw_proc, NULL, 0xFFFFFF);

  /*populate bomb_hess menu*/
  bomb_hess.selector = menu_add_submenu(&bomb_hess, 0, 0, NULL, "return");
  menu_add_static_custom(&bomb_hess, 0, 1, bomb_hess_draw_proc, NULL, 0xFFFFFF);

  /*add equip swap display to global menu*/
  struct menu_item* global_equip_swap_item =
        menu_add_static_custom(gz.menu_global, 2, 10, equip_swap_draw_proc, NULL, 0xFFFFFF);
  global_equip_swap_item->enabled = 0;

  /*populate equip swap timing menu*/
  equip_swap_timing.selector = menu_add_submenu(&equip_swap_timing, 0, 0, NULL, "return");
  menu_add_checkbox(&equip_swap_timing, 0, 1, equip_swap_toggle_proc, global_equip_swap_item);
  menu_add_static(&equip_swap_timing, 2, 1, "Enable Equip Swap Trainer", 0xC0C0C0);
  return &menu;
}
