#include <stdlib.h>
#include "gfx.h"
#include "menu.h"

struct item_data
{
  menu_action_callback  callback_proc;
  void                 *callback_data;
  struct gfx_texture   *texture;
  int                   texture_tile;
  int                   anim_state;
};

static int enter_proc(struct menu_item *item, enum menu_switch_reason reason)
{
  struct item_data *data = item->data;
  data->anim_state = 0;
  return 0;
}

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  if (data->anim_state > 0) {
    ++draw_params->x;
    ++draw_params->y;
    data->anim_state = (data->anim_state + 1) % 3;
  }
  if (data->texture) {
    int cw = menu_get_cell_width(item->owner, 1);
    struct gfx_sprite sprite =
    {
      data->texture,
      data->texture_tile,
      draw_params->x +
      (cw - data->texture->tile_width) / 2,
      draw_params->y - (gfx_font_xheight(draw_params->font) +
                        data->texture->tile_height + 1) / 2,
      1.f, 1.f,
    };
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                               draw_params->alpha));
    gfx_sprite_draw(&sprite);
    return 1;
  }
  return 0;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  data->callback_proc(item, data->callback_data);
  data->anim_state = 1;
  return 1;
}

struct menu_item *menu_add_button(struct menu *menu, int x, int y,
                                  const char *name,
                                  menu_action_callback callback_proc,
                                  void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->texture = NULL;
  data->anim_state = 0;
  struct menu_item *item = menu_item_add(menu, x, y, name, 0xFFFFFF);
  item->data = data;
  item->enter_proc = enter_proc;
  item->draw_proc = draw_proc;
  item->activate_proc = activate_proc;
  return item;
}

struct menu_item *menu_add_button_icon(struct menu *menu, int x, int y,
                                       struct gfx_texture *texture,
                                       int texture_tile, uint32_t color,
                                       menu_action_callback callback_proc,
                                       void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->texture = texture;
  data->texture_tile = texture_tile;
  data->anim_state = 0;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, color);
  item->data = data;
  item->enter_proc = enter_proc;
  item->draw_proc = draw_proc;
  item->activate_proc = activate_proc;
  return item;
}
