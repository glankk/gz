#include <stdlib.h>
#include "gfx.h"
#include "menu.h"
#include "resource.h"

struct item_data
{
  menu_generic_callback   callback_proc;
  void                   *callback_data;
  _Bool                   active;
};

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  gfx_mode_color((draw_params->color >> 16) & 0xFF,
                 (draw_params->color >> 8)  & 0xFF,
                 (draw_params->color >> 0)  & 0xFF,
                 draw_params->alpha);
  static struct gfx_texture *texture = NULL;
  if (!texture)
    texture = resource_load_grc_texture("move_icon");
  int cw = menu_get_cell_width(item->owner, 1);
  struct gfx_sprite sprite =
  {
    texture,
    0,
    draw_params->x +
    (cw - texture->tile_width) / 2,
    draw_params->y +
    (draw_params->font->median - draw_params->font->baseline -
     texture->tile_height) / 2,
    1.f,
    1.f,
  };
  gfx_sprite_draw(&sprite);
  return 1;
}

static int navigate_proc(struct menu_item *item, enum menu_navigation nav)
{
  struct item_data *data = item->data;
  if (data->active && data->callback_proc)
    data->callback_proc(item, MENU_CALLBACK_NAV_UP + nav, data->callback_data);
  return data->active;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (!data->callback_proc ||
      !data->callback_proc(item, data->active ? MENU_CALLBACK_DEACTIVATE :
                           MENU_CALLBACK_ACTIVATE, data->callback_data))
  {
    data->active = !data->active;
    item->animate_highlight = data->active;
  }
  return 1;
}

struct menu_item *menu_add_positioning(struct menu *menu, int x, int y,
                                       menu_generic_callback callback_proc,
                                       void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->active = 0;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0xFFFFFF);
  item->data = data;
  item->draw_proc = draw_proc;
  item->navigate_proc = navigate_proc;
  item->activate_proc = activate_proc;
  return item;
}
