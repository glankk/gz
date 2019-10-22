#include <stdlib.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"

static uint8_t roll_timer;
static uint8_t last_roll;
static uint8_t is_first;
static _Bool   roll_timer_active;

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

static void update_roll_timer()
{
  if(gz.frames_queued == 0){
    return;
  }

  if(roll_pressed() && !roll_timer_active){
    roll_timer_active = 1;
    is_first = 1;
    last_roll = 0;
  } else if (roll_pressed() && roll_timer_active){
    is_first = 0;
    last_roll = roll_timer;
    roll_timer = 0;
  }

  if(roll_timer_active){
    roll_timer++;
  } else{
    roll_timer = 0;
  }

  if(roll_timer == 30){
    roll_timer_active = 0;
  }
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
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, 0xFF));

  update_roll_timer();
  gfx_printf(font, x, y + ch * 0, "%i", roll_timer);
  gfx_printf(font, x, y + ch * 1, "%i", last_roll);

  if (!is_first && roll_timer_active){
    if((last_roll < 14) || (last_roll > 19)){
      gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0x10, 0x10, 0xFF));
      int amnt = last_roll - 16;
      if (amnt < 0)
        gfx_printf(font, x, y + ch * 2, "bad (%i frames early)", abs(amnt));
      if (amnt > 0)
        gfx_printf(font, x, y + ch * 2, "bad (%i frames late)", abs(amnt));
    }else{
      switch(last_roll)
      {
        case 14: gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xBE, 0x4F, 0xFF));
                 gfx_printf(font, x, y + ch * 2, "okay (2 frames early)");
                 break;
        case 15: gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x7D, 0xFF, 0x95, 0xFF));
                 gfx_printf(font, x, y + ch * 2, "good (1 frame early)");
                 break;
        case 16: gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x00, 0xFF, 0x2F, 0xFF));
                 gfx_printf(font, x, y + ch * 2, "perfect! (frame perfect)");
                 break;
        case 17: gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0x7D, 0xFF, 0x95, 0xFF));
                 gfx_printf(font, x, y + ch * 2, "good (1 frame late)");
                 break;
        case 18: gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xEF, 0xFF, 0x20, 0xFF));
                 gfx_printf(font, x, y + ch * 2, "okay (2 frames late)");
                 break;
        case 19: gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xBE, 0x4F, 0xFF));
                 gfx_printf(font, x, y + ch * 2, "okay (3 frames late)");
                 break;
        default: break;
      }
    }
  }
  return 1;
}

struct menu *gz_trainer_menu(void)
{
  static struct menu menu;
  static struct menu roll_timing;

  /* initialize menu */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&roll_timing, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* populate trainer top menu*/
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  menu_add_submenu(&menu, 0, 1, &roll_timing, "roll timing");

  /*populate roll timing menu*/
  roll_timing.selector = menu_add_submenu(&roll_timing, 0, 0, NULL, "return");
  menu_add_static_custom(&roll_timing, 0, 1, roll_timing_draw_proc, NULL, 0xFFFFFF);

  return &menu;
}
