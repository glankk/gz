#include <startup.h>
#include <n64.h>
#include <z64.h>
#include "console.h"
#include "menu.h"

void* g_text_ptr;
static struct menu g_menu;


ENTRY void _start(void* text_ptr)
{
  {
    init_gp();
    g_text_ptr = text_ptr;
    static struct init_t
    {
      init_t(void* text_ptr)
      {
        do_global_ctors();
#if 0
        console_init(36, 288);
        console_set_view(2, 8, 36, 18);
#endif
        menu_init(&g_menu);
        menu_add_watchlist(&g_menu, 2, 6, 0);
      }
    } init(text_ptr);
  }

#if 1
  static uint16_t pad_prev = 0;
  uint16_t pad_pressed = (pad_prev ^ z64_controller_1.pad) &
                         z64_controller_1.pad;
  pad_prev = z64_controller_1.pad;

  /* infinite energy */
  (*(uint16_t*)0x8011A600) = 0x0140;
  /* infinite magic */
  if ((*(uint8_t*) 0x8011A609) == 0x08)
    (*(uint8_t*) 0x8011A60A) = 0x01;
  (*(uint8_t*) 0x8011A60C) = 0x01;
  (*(uint8_t*) 0x8011A603) = 0x60;
  /* infinite bombs */
  (*(uint8_t*) 0x8011A646) = 0x02;
  (*(uint8_t*) 0x8011A65E) = 0x09;
  /* infinite bombchus */
  (*(uint8_t*) 0x8011A64C) = 0x09;
  (*(uint8_t*) 0x8011A664) = 0x09;
  /* activated */
  if (z64_controller_1.pad & BUTTON_Z) {
    /* reload zone with d-pad down */
    if (z64_controller_1.pad & BUTTON_D_DOWN)
      (*(uint16_t*)0x801DA2B4) = 0x0014;
    /* title screen with d-pad up */
    if (z64_controller_1.pad & BUTTON_D_UP) {
      (*(uint8_t*) 0x8011B92F) = 0x02;
      (*(uint16_t*)0x801DA2B4) = 0x0014;
    }
    /* levitate with l */
    if (z64_controller_1.pad & BUTTON_L)
      (*(uint16_t*)0x801DAA90) = 0x40CB;
    /* teleportation */
    static z64_xyz_t stored_pos;
    static z64_rot_t stored_rot;
    if (z64_controller_1.pad & BUTTON_D_LEFT) {
      stored_pos = z64_link_pos;
      stored_rot = z64_link_rot;
    }
    if (z64_controller_1.pad & BUTTON_D_RIGHT) {
      z64_link_pos = stored_pos;
      z64_link_rot = stored_rot;
    }
  }
  else {
    if (pad_pressed & BUTTON_D_UP)
      menu_navigate(&g_menu, MENU_NAVIGATE_UP);
    if (pad_pressed & BUTTON_D_DOWN)
      menu_navigate(&g_menu, MENU_NAVIGATE_DOWN);
    if (pad_pressed & BUTTON_D_LEFT)
      menu_navigate(&g_menu, MENU_NAVIGATE_LEFT);
    if (pad_pressed & BUTTON_D_RIGHT)
      menu_navigate(&g_menu, MENU_NAVIGATE_RIGHT);
    if (pad_pressed & BUTTON_L)
      menu_activate(&g_menu);
  }
  menu_draw(&g_menu);
#endif

#if 0
  if (z64_controller_1.du)
    console_scroll(0, -1);
  if (z64_controller_1.dd)
    console_scroll(0, 1);
  if (z64_controller_1.dl)
    console_scroll(-1, 0);
  if (z64_controller_1.dr)
    console_scroll(1, 0);
  console_print();
#endif
}


/* Custom Support Library */
#include <startup.c>
#include <icxxabi.cpp>
#include <vector/vector.c>
#include <list/list.c>
