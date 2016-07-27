#include <startup.h>
#include <n64.h>
#include <z64.h>
#include <mips.h>
#include "console.h"
#include "menu.h"

void* g_text_ptr;
static int ready = 0;
static struct menu g_menu_main;
static struct menu g_menu_watches;
static struct menu g_menu_equipment;
static struct menu g_menu_misc;
static struct menu g_menu_cheats;
static struct menu g_menu_warps;

static int g_cheats_energy    = 0;
static int g_cheats_magic     = 0;
static int g_cheats_bombs     = 0;
static int g_cheats_bombchus  = 0;

static int tp_slot = 0;

static z64_controller_t *input_ptr = (z64_controller_t*)0x8011D730;

static int input_frames = -1;
static int update_frames = -1;


enum equipment_entry
{
  EQUIPMENT_SWORD_KOKIRI,
  EQUIPMENT_SWORD_MASTER,
  EQUIPMENT_SWORD_GIANT,
  EQUIPMENT_SHIELD_DEKU,
  EQUIPMENT_SHIELD_HYLIAN,
  EQUIPMENT_SHIELD_MIRROR,
  EQUIPMENT_TUNIC_KOKIRI,
  EQUIPMENT_TUNIC_GORON,
  EQUIPMENT_TUNIC_ZORA,
  EQUIPMENT_BOOTS_KOKIRI,
  EQUIPMENT_BOOTS_IRON,
  EQUIPMENT_BOOTS_HOVER,
  EQUIPMENT_SWORD_BROKEN,
  EQUIPMENT_SWORD_BIGGORON,
  EQUIPMENT_MAX,
};

struct equipment_info
{
  const char *name;
  uint8_t    *address;
  uint8_t     mask;
}
equipment_list[] =
{
  {"kokiri sword",          (uint8_t*)0x8011A66D, 0b00000001},
  {"master sword",          (uint8_t*)0x8011A66D, 0b00000010},
  {"giant's knife",         (uint8_t*)0x8011A66D, 0b00000100},
  {"deku shield",           (uint8_t*)0x8011A66D, 0b00010000},
  {"hylian shield",         (uint8_t*)0x8011A66D, 0b00100000},
  {"mirror shield",         (uint8_t*)0x8011A66D, 0b01000000},
  {"kokiri tunic",          (uint8_t*)0x8011A66C, 0b00000001},
  {"goron tunic",           (uint8_t*)0x8011A66C, 0b00000010},
  {"zora tunic",            (uint8_t*)0x8011A66C, 0b00000100},
  {"kokiri boots",          (uint8_t*)0x8011A66C, 0b00010000},
  {"iron boots",            (uint8_t*)0x8011A66C, 0b00100000},
  {"hover boots",           (uint8_t*)0x8011A66C, 0b01000000},
  {"broken giant's knife",  (uint8_t*)0x8011A66D, 0b00001000},
  {"biggoron's sword",      (uint8_t*)0x8011A60E, 0b00000001},
};

struct warp_info
{
  struct menu_item *entrance;
  struct menu_item *age;
};

static int generic_switch_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  int *state = data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *state = 1;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *state = 0;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, *state);
  return 0;
}

static int equipment_switch_proc(struct menu_item *item,
                                 enum menu_callback_reason reason,
                                 void *data)
{
  struct equipment_info *e = data;
  if (reason == MENU_CALLBACK_SWITCH_ON)
    *e->address |= e->mask;
  else if (reason == MENU_CALLBACK_SWITCH_OFF)
    *e->address &= ~e->mask;
  else if (reason == MENU_CALLBACK_THINK)
    menu_switch_set(item, *e->address & e->mask);
  return 0;
}

static void clear_flags_proc(struct menu_item *item, void *data)
{
  (*(uint32_t*)0x801CA1C8) = 0x00000000;
  (*(uint32_t*)0x801CA1CC) = 0x00000000;
  (*(uint32_t*)0x801CA1D8) = 0x00000000;
  (*(uint32_t*)0x801CA1DC) = 0x00000000;
}
static void set_flags_proc(struct menu_item *item, void *data)
{
  (*(uint32_t*)0x801CA1C8) = 0xFFFFFFFF;
  (*(uint32_t*)0x801CA1CC) = 0xFFFFFFFF;
  (*(uint32_t*)0x801CA1D8) = 0xFFFFFFFF;
  (*(uint32_t*)0x801CA1DC) = 0xFFFFFFFF;
}

static void tp_slot_dec_proc(struct menu_item *item, void *data)
{
  struct menu_item *tp_slot_display = data;
  tp_slot = (tp_slot - 1) % 10;
  if (tp_slot < 0)
    tp_slot += 10;
  tp_slot_display->text[0] = '0' + tp_slot;
}

static void tp_slot_inc_proc(struct menu_item *item, void *data)
{
  struct menu_item *tp_slot_display = data;
  tp_slot = (tp_slot + 1) % 10;
  tp_slot_display->text[0] = '0' + tp_slot;
}

static void warp_proc(struct menu_item *item, void *data)
{
  struct warp_info *warp_info = data;
  uint16_t entrance = menu_intinput_get(warp_info->entrance);
  uint8_t  age      = menu_option_get(warp_info->age);
  (*(uint16_t*)0x801DA2BA) = entrance;
  (*(uint8_t*) 0x801DA288) = age;
  (*(uint16_t*)0x801DA2B4) = 0x0014;
}

static void input_hook()
{
  if (input_frames != 0) {
    if (input_frames > 0)
      --input_frames;
    ((void(*)())0x800A0BA0)();
  }
}

static void update_hook()
{
  if (update_frames != 0) {
    if (update_frames > 0)
      --update_frames;
    ((void(*)())0x8009AF1C)();
  }
}

static void pause_proc(struct menu_item *item, void *data)
{
  uint32_t *input_call = (uint32_t*)0x800A16AC;
  *input_call = MIPS_JAL(&input_hook);
  uint32_t *update_call = (uint32_t*)0x8009CAE8;
  *update_call = MIPS_JAL(&update_hook);
  if (input_frames >= 0)
    input_frames = -1;
  else
    input_frames = 0;
  if (update_frames >= 0)
    update_frames = -1;
  else
    update_frames = 0;
}

static void advance_proc(struct menu_item *item, void *data)
{
  if (input_frames >= 0)
    ++input_frames;
  if (update_frames >= 0)
    ++update_frames;
}

ENTRY void _start(void* text_ptr)
{
  init_gp();
  if (!ready) {
    ready = 1;
    do_global_ctors();

  /* disable map toggling */
  (*(uint32_t*)0x8006CD50) = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x82C);
  (*(uint32_t*)0x8006D4E4) = MIPS_BEQ(MIPS_R0, MIPS_R0, 0x98);

  g_text_ptr = text_ptr;

#if 0
    console_init(36, 288);
    console_set_view(2, 8, 36, 18);
#endif
    menu_init(&g_menu_main);
    g_menu_main.selector = menu_add_submenu(&g_menu_main, 2, 6, &g_menu_watches,
                                            "watches", 0);
    menu_add_submenu(&g_menu_main, 2, 7, &g_menu_equipment, "equipment", 0);
    menu_add_submenu(&g_menu_main, 2, 8, &g_menu_misc, "misc", 0);
    menu_add_submenu(&g_menu_main, 2, 9, &g_menu_cheats, "cheats", 0);
    menu_add_submenu(&g_menu_main, 2, 10, &g_menu_warps, "warps", 0);

    menu_init(&g_menu_watches);
    g_menu_watches.selector = menu_add_submenu(&g_menu_watches, 2, 6, NULL,
                                               "return", 0);
    menu_add_watchlist(&g_menu_watches, 2, 7, 0);

    menu_init(&g_menu_equipment);
    g_menu_equipment.selector = menu_add_submenu(&g_menu_equipment, 2, 6, NULL,
                                                 "return", 0);
    for (int i = 0; i < EQUIPMENT_MAX; ++i)
      menu_add_switch(&g_menu_equipment,
                      2 + i / 3 % 2 * 20,
                      7 + i / 6 * 3 + i % 3,
                      equipment_list[i].name,
                      equipment_switch_proc,
                      &equipment_list[i],
                      0);

    menu_init(&g_menu_misc);
    g_menu_misc.selector = menu_add_submenu(&g_menu_misc, 2, 6, NULL,
                                            "return", 0);
    menu_add_button(&g_menu_misc, 2, 7, "clear flags",
                    clear_flags_proc, NULL, 0);
    menu_add_button(&g_menu_misc, 2, 8, "set flags",
                    set_flags_proc, NULL, 0);
    menu_add_static(&g_menu_misc, 2, 9, "teleport slot", 0xFFFFFF);
    struct menu_item *tp_slot_display = menu_add_static(&g_menu_misc,
                                                        18, 9, "0",
                                                        0xA0A0A0);
    menu_add_button(&g_menu_misc, 16, 9, "-", tp_slot_dec_proc,
                    tp_slot_display, 0);
    menu_add_button(&g_menu_misc, 20, 9, "+", tp_slot_inc_proc,
                    tp_slot_display, 0);
    menu_add_button(&g_menu_misc, 2, 10, "pause / unpause", pause_proc, NULL, 0);
    menu_add_button(&g_menu_misc, 2, 11, "frame advance", advance_proc, NULL, 0);

    menu_init(&g_menu_cheats);
    g_menu_cheats.selector = menu_add_submenu(&g_menu_cheats, 2, 6, NULL,
                                              "return", 0);
    menu_add_switch(&g_menu_cheats, 2, 7, "energy",
                    generic_switch_proc, &g_cheats_energy, 0);
    menu_add_switch(&g_menu_cheats, 2, 8, "magic",
                    generic_switch_proc, &g_cheats_magic, 0);
    menu_add_switch(&g_menu_cheats, 2, 9, "bombs",
                    generic_switch_proc, &g_cheats_bombs, 0);
    menu_add_switch(&g_menu_cheats, 2, 10, "bombchus",
                    generic_switch_proc, &g_cheats_bombchus, 0);

    menu_init(&g_menu_warps);
    g_menu_warps.selector = menu_add_submenu(&g_menu_warps, 2, 6, NULL,
                                             "return", 0);
    static struct warp_info warp_info;
    menu_add_static(&g_menu_warps, 2, 7, "entrance", 0xFFFFFF);
    warp_info.entrance = menu_add_intinput(&g_menu_warps, 12, 7, 16, 4, 0);
    menu_add_static(&g_menu_warps, 2, 8, "age", 0xFFFFFF);
    warp_info.age = menu_add_option(&g_menu_warps, 12, 8,
                                    "adult\0""child\0", 0);
    menu_add_button(&g_menu_warps, 2, 9, "warp", warp_proc, &warp_info, 0);
  }

#if 1
  static uint16_t pad_prev = 0;
  uint16_t pad_pressed = (pad_prev ^ input_ptr->pad) &
                         input_ptr->pad;
  pad_prev = input_ptr->pad;

  /* infinite energy */
  if (g_cheats_energy)
    (*(uint16_t*)0x8011A600) = 0x0140;
  /* infinite magic */
  if (g_cheats_magic) {
    if ((*(uint8_t*) 0x8011A609) == 0x08)
      (*(uint8_t*) 0x8011A60A) = 0x01;
    (*(uint8_t*) 0x8011A60C) = 0x01;
    (*(uint8_t*) 0x8011A603) = 0x60;
  }
  /* infinite bombs */
  if (g_cheats_bombs) {
    (*(uint8_t*) 0x8011A646) = 0x02;
    (*(uint8_t*) 0x8011A65E) = 0x09;
  }
  /* infinite bombchus */
  if (g_cheats_bombchus) {
    (*(uint8_t*) 0x8011A64C) = 0x09;
    (*(uint8_t*) 0x8011A664) = 0x09;
  }
  /* activated */
  if (input_frames == -1 && input_ptr->pad & BUTTON_Z) {
    /* reload zone with d-pad down */
    if (input_ptr->pad & BUTTON_D_DOWN)
      (*(uint16_t*)0x801DA2B4) = 0x0014;
    /* title screen with d-pad up */
    if (input_ptr->pad & BUTTON_D_UP) {
      (*(uint8_t*) 0x8011B92F) = 0x02;
      (*(uint16_t*)0x801DA2B4) = 0x0014;
    }
    /* levitate with l */
    if (input_ptr->pad & BUTTON_L)
      (*(uint16_t*)0x801DAA90) = 0x40CB;
    /* teleportation */
    static z64_xyz_t stored_pos[10];
    static z64_rot_t stored_rot[10];
    if (input_ptr->pad & BUTTON_D_LEFT) {
      stored_pos[tp_slot] = z64_link_pos;
      stored_rot[tp_slot] = z64_link_rot;
    }
    if (input_ptr->pad & BUTTON_D_RIGHT) {
      (*(z64_xyz_t*)0x801DAA38) = stored_pos[tp_slot]; /* secondary position */
      z64_link_pos = stored_pos[tp_slot];
      z64_link_rot = stored_rot[tp_slot];
      /* (*(uint8_t*)0x801DAADE) = 0xFF; prevents collision with actors */
    }
  }
  else {
    if (pad_pressed & BUTTON_D_UP)
      menu_navigate(&g_menu_main, MENU_NAVIGATE_UP);
    if (pad_pressed & BUTTON_D_DOWN)
      menu_navigate(&g_menu_main, MENU_NAVIGATE_DOWN);
    if (pad_pressed & BUTTON_D_LEFT)
      menu_navigate(&g_menu_main, MENU_NAVIGATE_LEFT);
    if (pad_pressed & BUTTON_D_RIGHT)
      menu_navigate(&g_menu_main, MENU_NAVIGATE_RIGHT);
    if (pad_pressed & BUTTON_L)
      menu_activate(&g_menu_main);
  }
  menu_draw(&g_menu_main);
#endif

#if 0
  if (input_ptr->du)
    console_scroll(0, -1);
  if (input_ptr->dd)
    console_scroll(0, 1);
  if (input_ptr->dl)
    console_scroll(-1, 0);
  if (input_ptr->dr)
    console_scroll(1, 0);
  console_print();
#endif
}


/* custom support library */
#include <startup.c>
#include <vector/vector.c>
#include <list/list.c>
