#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "gfx.h"
#include "gz.h"
#include "menu.h"
#include "settings.h"
#include "z64.h"
#include "zu.h"

struct scene_category
{
  const char           *category_name;
  int                   n_scenes;
  const uint8_t        *scenes;
};

static struct scene_category scene_categories[] =
{
  {
    "dungeons",
    17, (uint8_t[])
    {
      0,   1,   2,   3,
      4,   5,   6,   7,
      8,   9,   10,  11,
      12,  13,  14,  15,
      26,
    },
  },
  {
    "bosses",
    10, (uint8_t[])
    {
      17,  18,  19,  20,
      21,  22,  23,  24,
      25,  79,
    },
  },
  {
    "towns",
    16, (uint8_t[])
    {
      82,  88,  85,  98,
      99,  32,  33,  34,
      27,  28,  29,  30,
      31,  35,  36,  37,
    },
  },
  {
    "houses",
    15, (uint8_t[])
    {
      52,  38,  39,  40,
      41,  42,  43,  53,
      54,  55,  57,  58,
      76,  77,  80,
    },
  },
  {
    "shops",
    12, (uint8_t[])
    {
      44,  45,  46,  47,
      48,  49,  50,  51,
      66,  75,  78,  16,
    },
  },
  {
    "misc",
    15, (uint8_t[])
    {
      67,  74,  72,  69,
      70,  56,  59,  61,
      65,  62,  63,  64,
      68,  71,  60,
    },
  },
  {
    "overworld",
    16, (uint8_t[])
    {
      81,  91,   86,  96,
      97,  84,   89,  87,
      95,  100,  90,  93,
      94,  92,   83,  73,
    },
  },
  {
    "beta",
    9, (uint8_t[])
    {
      101, 102, 103, 104,
      105, 106, 107, 108,
      109,
    },
  },
};

static int halfword_mod_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  uint16_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static int age_option_proc(struct menu_item *item,
                           enum menu_callback_reason reason,
                           void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->bits.warp_age)
      menu_option_set(item, settings->bits.warp_age);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->bits.warp_age = menu_option_get(item);
  return 0;
}

static int cutscene_option_proc(struct menu_item *item,
                                enum menu_callback_reason reason,
                                void *data)
{
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != settings->bits.warp_cutscene)
      menu_option_set(item, settings->bits.warp_cutscene);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    settings->bits.warp_cutscene = menu_option_get(item);
  return 0;
}

static void warp_proc(struct menu_item *item, void *data)
{
  uint16_t cutscene = settings->bits.warp_cutscene;
  if (cutscene > 0x0000)
    cutscene += 0xFFEF;
  gz.entrance_override_once = 1;
  gz_warp(settings->warp_entrance, cutscene, settings->bits.warp_age);
}

static void clear_csp_proc(struct menu_item *item, void *data)
{
  static uint32_t null_cs[] = {0, 0};
  z64_game.cutscene_ptr = &null_cs;
}

static int warp_info_draw_proc(struct menu_item *item,
                               struct menu_draw_params *draw_params)
{
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  struct gfx_font *font = draw_params->font;
  int ch = menu_get_cell_height(item->owner, 1);
  int x = draw_params->x;
  int y = draw_params->y;
  if (z64_game.link_age == 0)
    gfx_printf(font, x, y + ch * 0, "current age      adult");
  else
    gfx_printf(font, x, y + ch * 0, "current age      child");
  gfx_printf(font, x, y + ch * 1, "current entrance %04" PRIx16,
             z64_file.entrance_index);
  if (z64_file.cutscene_index >= 0xFFF0 && z64_file.cutscene_index <= 0xFFFF) {
    gfx_printf(font, x, y + ch * 2, "current cutscene %" PRIu16,
               z64_file.cutscene_index - 0xFFF0);
  }
  else
    gfx_printf(font, x, y + ch * 2, "current cutscene none");
  return 1;
}

static void places_proc(struct menu_item *item, void *data)
{
  uintptr_t d = (uintptr_t)data;
  int scene_index = (d >> 8) & 0x00FF;
  int entrance_index = (d >> 0) & 0x00FF;
  for (int i = 0; i < Z64_ETAB_LENGTH; ++i) {
    z64_entrance_table_t *e = &z64_entrance_table[i];
    if (e->scene_index == scene_index && e->entrance_index == entrance_index) {
      uint16_t cutscene = settings->bits.warp_cutscene;
      if (cutscene > 0x0000)
        cutscene += 0xFFEF;
      gz.entrance_override_once = 1;
      gz_warp(i, cutscene, settings->bits.warp_age);
      if (zu_scene_info[scene_index].n_entrances > 1)
        menu_return(gz.menu_main);
      menu_return(gz.menu_main);
      menu_return(gz.menu_main);
      break;
    }
  }
}

struct menu *gz_warps_menu(void)
{
  static struct menu menu;
  static struct menu places;

  /* initialize menus */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&places, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

  /* populate warps top menu */
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  /* warp options */
  menu_add_submenu(&menu, 0, 1, &places, "places");
  menu_add_static(&menu, 0, 2, "age", 0xC0C0C0);
  menu_add_option(&menu, 9, 2, "current age\0""adult\0""child\0",
                  age_option_proc, NULL);
  menu_add_static(&menu, 0, 3, "entrance", 0xC0C0C0);
  menu_add_intinput(&menu, 9, 3, 16, 4,
                    halfword_mod_proc, &settings->warp_entrance);
  menu_add_static(&menu, 0, 4, "cutscene", 0xC0C0C0);
  menu_add_option(&menu, 9, 4,
                  "none\0""0\0""1\0""2\0""3\0""4\0""5\0""6\0""7\0"
                  "8\0""9\0""10\0""11\0""12\0""13\0""14\0""15\0",
                  cutscene_option_proc, NULL);
  menu_add_button(&menu, 0, 5, "warp", warp_proc, NULL);
  menu_add_button(&menu, 0, 6, "clear cs pointer", clear_csp_proc, NULL);
  /* warp info */
  menu_add_static_custom(&menu, 0, 7, warp_info_draw_proc, NULL, 0xC0C0C0);

  /* populate places menu */
  places.selector = menu_add_submenu(&places, 0, 0, NULL, "return");
  int n_scene_cat = sizeof(scene_categories) / sizeof(*scene_categories);
  for (int i = 0; i < n_scene_cat; ++i) {
    struct scene_category *cat = &scene_categories[i];
    struct menu *cat_menu = malloc(sizeof(*cat_menu));
    menu_init(cat_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    cat_menu->selector = menu_add_submenu(cat_menu, 0, 0, NULL, "return");
    for (int j = 0; j < cat->n_scenes; ++j) {
      uint8_t scene_index = cat->scenes[j];
      struct zu_scene_info *scene = &zu_scene_info[scene_index];
      if (scene->n_entrances > 1) {
        struct menu *scene_menu = malloc(sizeof(*scene_menu));
        menu_init(scene_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
        scene_menu->selector = menu_add_submenu(scene_menu, 0, 0, NULL,
                                                "return");
        for (uint8_t k = 0; k < scene->n_entrances; ++k) {
          menu_add_button(scene_menu, 0, 1 + k, scene->entrance_names[k],
                          places_proc, (void*)((scene_index << 8) | k));
        }
        menu_add_submenu(cat_menu, 0, 1 + j, scene_menu, scene->scene_name);
      }
      else {
        menu_add_button(cat_menu, 0, 1 + j, scene->scene_name,
                        places_proc, (void*)((scene_index << 8) | 0));
      }
    }
    menu_add_submenu(&places, 0, 1 + i, cat_menu, cat->category_name);
  }

  return &menu;
}
