#include <stdlib.h>
#include "gfx.h"
#include "menu.h"
#include "resource.h"

struct item_data
{
  menu_generic_callback    callback_proc;
  void                    *callback_data;
  _Bool                    state;
  int                      anim_state;
  enum menu_checkbox_type  type;
};

static int enter_proc(struct menu_item *item, enum menu_switch_reason reason)
{
  struct item_data *data = item->data;
  data->anim_state = 0;
  return 0;
}

static int think_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->callback_proc)
    return data->callback_proc(item, MENU_CALLBACK_THINK, data->callback_data);
  return 0;
}

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  static struct gfx_texture *texture = NULL;
  if (!texture)
    texture = resource_load_grc_texture("checkbox");
  int cw = menu_get_cell_width(item->owner, 1);
  struct gfx_sprite sprite =
  {
    texture,
    data->anim_state == 0 ? 0 : 1,
    draw_params->x +
    (cw - texture->tile_width) / 2,
    draw_params->y -
    (gfx_font_xheight(draw_params->font) + texture->tile_height + 1) / 2,
    1.f, 1.f,
  };
  gfx_sprite_draw(&sprite);
  if ((data->anim_state > 0) != data->state) {
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF,
                                                draw_params->alpha));
    switch(data->type)
    {
      case MENU_CHECKBOX_POINTER:  sprite.texture_tile = 3;  break;
      case MENU_CHECKBOX_CHECK:
      default:                     sprite.texture_tile = 2;  break;
    }
    gfx_sprite_draw(&sprite);
  }
  if (data->anim_state > 0)
    data->anim_state = (data->anim_state + 1) % 3;
  return 1;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (!data->callback_proc ||
      !data->callback_proc(item, data->state ? MENU_CALLBACK_SWITCH_OFF :
                           MENU_CALLBACK_SWITCH_ON, data->callback_data))
  {
    data->state = !data->state;
    data->anim_state = 1;
    if (data->callback_proc)
      data->callback_proc(item, MENU_CALLBACK_CHANGED, data->callback_data);
  }
  return 1;
}

struct menu_item *menu_add_checkbox(struct menu *menu, int x, int y,
                                    menu_generic_callback callback_proc,
                                    void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->anim_state = 0;
  data->type = MENU_CHECKBOX_CHECK;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, 0xFFFFFF);
  item->data = data;
  item->enter_proc = enter_proc;
  item->think_proc = think_proc;
  item->draw_proc = draw_proc;
  item->activate_proc = activate_proc;
  return item;
}

void menu_checkbox_set_type(struct menu_item *item, 
                            enum menu_checkbox_type type)
{
  struct item_data *data = item->data;
  data->type = type;
}

_Bool menu_checkbox_get(struct menu_item *item)
{
  struct item_data *data = item->data;
  return data->state;
}

void menu_checkbox_set(struct menu_item *item, _Bool state)
{
  struct item_data *data = item->data;
  data->state = state;
}
