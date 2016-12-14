#include <stdlib.h>
#include <stdint.h>
#include "gfx.h"
#include "menu.h"

struct static_icon_data
{
  struct gfx_texture *texture;
  int                 texture_tile;
  float               scale;
};

struct menu_item *menu_add_static(struct menu *menu, int x, int y,
                                  const char *text, uint32_t color)
{
  struct menu_item *item = menu_item_add(menu, x, y, text, color);
  item->selectable = 0;
  return item;
}

static int static_icon_draw_proc(struct menu_item *item,
                                 struct menu_draw_params *draw_params)
{
  struct static_icon_data *data = item->data;
  int cw = menu_get_cell_width(item->owner, 1);
  int w = data->texture->tile_width * data->scale;
  int h = data->texture->tile_height * data->scale;
  struct gfx_sprite sprite =
  {
    data->texture, data->texture_tile,
    draw_params->x + (cw - w) / 2,
    draw_params->y - (gfx_font_xheight(draw_params->font) + h + 1) / 2,
    data->scale, data->scale,
  };
  gfx_mode_replace(GFX_MODE_FILTER, G_TF_BILERP);
  gfx_mode_replace(GFX_MODE_DROPSHADOW, 0);
  gfx_mode_set(GFX_MODE_COLOR, (draw_params->color << 8) |
               draw_params->alpha);
  gfx_sprite_draw(&sprite);
  gfx_mode_pop(GFX_MODE_FILTER);
  gfx_mode_pop(GFX_MODE_DROPSHADOW);
  return 1;
}

struct menu_item *menu_add_static_icon(struct menu *menu, int x, int y,
                                       struct gfx_texture *texture,
                                       int texture_tile,
                                       uint32_t color, float scale)
{
  struct static_icon_data *data = malloc(sizeof(*data));
  data->texture = texture;
  data->texture_tile = texture_tile;
  data->scale = scale;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, color);
  item->data = data;
  item->selectable = 0;
  item->draw_proc = static_icon_draw_proc;
  return item;
}

static int imenu_think_proc(struct menu_item *item)
{
  if (item->imenu) {
    item->imenu->cxoffset = item->x;
    item->imenu->cyoffset = item->y;
    item->imenu->pxoffset = item->pxoffset;
    item->imenu->pyoffset = item->pyoffset;
  }
  return 0;
}

static int imenu_navigate_proc(struct menu_item *item,
                               enum menu_navigation nav)
{
  if (item->imenu) {
    menu_navigate(item->imenu, nav);
    return 1;
  }
  return 0;
}

static int imenu_activate_proc(struct menu_item *item)
{
  if (item->imenu) {
    menu_activate(item->imenu);
    return 1;
  }
  return 0;
}

struct menu_item *menu_add_imenu(struct menu *menu, int x, int y,
                                 struct menu **p_imenu)
{
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0);
  item->selectable = 0;
  item->think_proc = imenu_think_proc;
  item->navigate_proc = imenu_navigate_proc;
  item->activate_proc = imenu_activate_proc;
  struct menu *imenu = malloc(sizeof(*imenu));
  menu_init(imenu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  imenu->parent = menu;
  item->imenu = imenu;
  if (p_imenu)
    *p_imenu = imenu;
  return item;
}
