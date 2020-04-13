#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"
#include "trainer.h"

#define TRAINER_MENU_ITEM_COUNT 4

static struct menu_item* trainer_menu_data[TRAINER_MENU_ITEM_COUNT];

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
  gfx_printf(font, x, y + ch * 2, "sidehop timer: %d", sidehop.sidehop_timer);
  gfx_printf(font, x, y + ch * 3, "landing timer: %d", sidehop.land_timer);
  gfx_printf(font, x, y + ch * 4, "a press: %d", sidehop.a_press);
  gfx_printf(font, x, y + ch * 5, "landing: %d", sidehop.landing);

  if (sidehop.result < 0) {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * log_y, "early by %i frames", -sidehop.result);
  } else if (sidehop.result == 1) {
    set_rgb_green();
    gfx_printf(font, x,  y + ch * log_y, "perfect! (frame perfect)");
  } else if (sidehop.result > 1) {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * log_y, "late by %i frames", sidehop.result - 1);
  } else if (sidehop.a_press != 0) {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * log_y, "early");
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
    gfx_printf(font, x,  y + ch * 2, "stick: %s (frame perfect)", equip_swap.diagonal_warning ? "good" : "perfect!");
  }
  else if (equip_swap.control_stick_moved_time < 0)
  {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * 2, "stick: late by %i frames", -equip_swap.control_stick_moved_time);
  }

  if (equip_swap.diagonal_warning)
  {
    set_rgb_red();
    gfx_printf(font, x,  y + ch * 3, "stick input must be diagonal");
  }

  return 1;
}

static int trainer_radio_button_toggle_proc(struct menu_item *item,
                      enum menu_callback_reason reason,
                      void *data)
{
  int index = (int)data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
  {
    for (int i = 0; i < TRAINER_MENU_ITEM_COUNT; i += 1)
    {
      if (i == index)
        trainer_menu_data[i]->enabled = 1;
      else
        trainer_menu_data[i]->enabled = 0;
    }
  }
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
  {
    trainer_menu_data[index]->enabled = 0;
  }
  else if (reason == MENU_CALLBACK_THINK)
    menu_checkbox_set(item, trainer_menu_data[index]->enabled);
  return 0;
}

struct menu *gz_trainer_menu(void)
{
  static struct menu menu;
  int global_x = 2;
  int global_y = 10 + TRAINER_MENU_ITEM_COUNT;
  int index = 0;

  /* setup menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");

  /*add roll timing option*/
  trainer_menu_data[index] = menu_add_static_custom(gz.menu_global, global_x, global_y, roll_timing_draw_proc, NULL, 0xFFFFFF);
  trainer_menu_data[index]->enabled = 0;
  menu_add_checkbox(&menu, 0, index + 1, trainer_radio_button_toggle_proc, (void*)index);
  menu_add_static(&menu, 2, index + 1, "Enable Roll Timing Trainer", 0xC0C0C0);
  index += 1;

  /*add sidehop timing option*/
  trainer_menu_data[index] = menu_add_static_custom(gz.menu_global, global_x, global_y, sidehop_timing_draw_proc, NULL, 0xFFFFFF);
  trainer_menu_data[index]->enabled = 0;
  menu_add_checkbox(&menu, 0, index + 1, trainer_radio_button_toggle_proc, (void*)index);
  menu_add_static(&menu, 2, index + 1, "Enable Sidehop Timing Trainer", 0xC0C0C0);
  index += 1;

  /*add bomb hess training option*/
  trainer_menu_data[index] = menu_add_static_custom(gz.menu_global, global_x, global_y, bomb_hess_draw_proc, NULL, 0xFFFFFF);
  trainer_menu_data[index]->enabled = 0;
  menu_add_checkbox(&menu, 0, index + 1, trainer_radio_button_toggle_proc, (void*)index);
  menu_add_static(&menu, 2, index + 1, "Enable Bomb Hess Trainer", 0xC0C0C0);
  index += 1;

  /*add equip swap training option*/
  trainer_menu_data[index] = menu_add_static_custom(gz.menu_global, global_x, global_y, equip_swap_draw_proc, NULL, 0xFFFFFF);
  trainer_menu_data[index]->enabled = 0;
  menu_add_checkbox(&menu, 0, index + 1, trainer_radio_button_toggle_proc, (void*)index);
  menu_add_static(&menu, 2, index + 1, "Enable Equip Swap Trainer", 0xC0C0C0);
  index += 1;

  return &menu;
}
