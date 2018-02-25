#include <stdlib.h>
#include <math.h>
#include <n64.h>
#include "gfx.h"
#include "gu.h"
#include "menu.h"
#include "resource.h"
#include "z64.h"
#include "zu.h"

struct item_data
{
  int                   n_items;
  const int8_t         *item_list;
  int8_t               *value;
  struct gfx_texture   *texture;
  struct gfx_texture   *bg_texture;
  int                   bg_texture_tile;
  struct gfx_texture   *empty_texture;
  int                   empty_texture_tile;
  uint32_t              color;
  float                 scale;
  float                 bg_scale;
  float                 empty_scale;
  menu_generic_callback callback_proc;
  void                 *callback_data;
  int                   anim_state;
  struct menu_item     *wheel_item;
  int                   wheel_index;
  float                 wheel_rot;
};

static int find_item_index(struct item_data *data, int item_id)
{
  int item_index = 0;
  if (data->item_list) {
    for (int i = 0; i < data->n_items; ++i)
      if (data->item_list[i] == item_id) {
        item_index = i;
        break;
      }
  }
  else if (item_id >= 0 && item_id + 1 < data->n_items)
    item_index = item_id + 1;
  return item_index;
}

static int get_item_id(struct item_data *data, int item_index)
{
  if (data->item_list)
    return data->item_list[item_index];
  else
    return item_index - 1;
}

static void animate_wheel(struct item_data *data, float speed)
{
  if (data->wheel_item) {
    float dest = M_PI * 2.f * -data->wheel_index / data->n_items;
    float diff = dest - data->wheel_rot;
    if (fabsf(diff) < .001f)
      data->wheel_rot = dest;
    else
      data->wheel_rot += diff * speed;
  }
}

static void wheel_draw_item(struct item_data *data, int item_id,
                            float rot)
{
  static Vtx mesh[] =
  {
    gdSPDefVtx(-1,  1, 0, 0,  0),
    gdSPDefVtx(1,   1, 0, 62, 0),
    gdSPDefVtx(-1, -1, 0, 0,  62),
    gdSPDefVtx(1,  -1, 0, 62, 62),
  };
  /* create modelview matrix */
  Mtx m;
  MtxF mf;
  MtxF mt;
  {
    guRotateF(&mf, rot, 0.f, 1.f, 0.f);
  }
  {
    guTranslateF(&mt, 0.f, 0.f, data->n_items / M_PI);
    guMtxCatF(&mt, &mf, &mf);
  }
  guMtxF2L(&mf, &m);
  Mtx *p_mtx = gfx_data_append(&m, sizeof(m));
  /* build dlist */
  gfx_disp
  (
    gsSPMatrix(p_mtx, G_MTX_MODELVIEW | G_MTX_LOAD),
    gsSPVertex(&mesh, 4, 0),
  );
  if (item_id == Z64_ITEM_NULL) {
    if (data->bg_texture) {
      gfx_rdp_load_tile(data->bg_texture, data->bg_texture_tile);
      gfx_disp(gsSP2Triangles(0, 1, 2, 0, 2, 1, 3, 0));
    }
    if (data->empty_texture) {
      gfx_rdp_load_tile(data->empty_texture, data->empty_texture_tile);
      gfx_disp(gsSP2Triangles(0, 1, 2, 0, 2, 1, 3, 0));
    }
  }
  else {
    gfx_rdp_load_tile(data->texture, item_id);
    gfx_disp(gsSP2Triangles(0, 1, 2, 0, 2, 1, 3, 0));
  }
}

static int wheel_draw_proc(struct menu_item *item,
                           struct menu_draw_params *draw_params)
{
  struct item_data *data = item->data;
  /* initialize rcp */
  gfx_disp
  (
    /* rsp state */
    gsDPPipeSync(),
    gsSPLoadGeometryMode(0),
    /* rdp state */
    gsDPSetCycleType(G_CYC_1CYCLE),
    /* texture engine */
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureLOD(G_TL_TILE),
    /* texture filter */
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureConvert(G_TC_FILT),
    /* color combiner */
    gsDPSetCombineKey(G_OFF),
    gsDPSetCombineMode(G_CC_MODULATERGBA_PRIM, G_CC_MODULATERGBA_PRIM),
    /* blender */
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    /* memory interface */
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetAlphaDither(G_AD_DISABLE),
    gsDPPipelineMode(G_PM_NPRIMITIVE),
  );
  /* create projection matrix */
  {
    Mtx m;
    MtxF mf;
    MtxF mt;
    {
      guPerspectiveF(&mf, NULL, M_PI / 4.f,
                     (float)Z64_SCREEN_WIDTH / (float)Z64_SCREEN_HEIGHT,
                     1.f, 100.f, 1.f);
    }
    {
      guTranslateF(&mt, 0.f, 0.f, 1.f - (Z64_SCREEN_WIDTH / 32.f * 2.f +
                                         data->n_items / M_PI));
      guMtxCatF(&mt, &mf, &mf);
    }
    guMtxF2L(&mf, &m);
    gfx_disp(gsSPMatrix(gfx_data_append(&m, sizeof(m)),
                        G_MTX_PROJECTION | G_MTX_LOAD));
  }
  /* draw items */
  animate_wheel(data, 1.f / 3.f);
  int n = (lroundf(-data->wheel_rot / (M_PI * 2.f) * data->n_items) +
           (data->n_items + 3) / 4) % data->n_items;
  if (n < 0)
    n += data->n_items;
  uint32_t color = -1;
  for (int i = 0; i < data->n_items; ++i) {
    float rot = data->wheel_rot + M_PI * 2.f / data->n_items * n;
    int item_id = get_item_id(data, n);
    uint8_t r = item_id == Z64_ITEM_NULL ? (data->color >> 16) & 0xFF : 0xFF;
    uint8_t g = item_id == Z64_ITEM_NULL ? (data->color >> 8)  & 0xFF : 0xFF;
    uint8_t b = item_id == Z64_ITEM_NULL ? (data->color >> 0)  & 0xFF : 0xFF;
    if (n != data->wheel_index) {
      r = r * 0x60 / 0xFF;
      g = g * 0x60 / 0xFF;
      b = b * 0x60 / 0xFF;
    }
    uint32_t c = (r << 16) | (g << 8) | (b << 0);
    if (color != c) {
      color = c;
      gfx_disp(gsDPPipeSync(),
               gsDPSetPrimColor(0, 0, r, g, b, draw_params->alpha));
    }
    wheel_draw_item(data, item_id, rot);
    n = (n + 1) % data->n_items;
  }
  /* restore rcp modes */
  gfx_mode_init();
  return 1;
}

static int wheel_navigate_proc(struct menu_item *item,
                               enum menu_navigation nav)
{
  struct item_data *data = item->data;
  int n = 0;
  switch (nav) {
    case MENU_NAVIGATE_UP:    n = 3;  break;
    case MENU_NAVIGATE_DOWN:  n = -3; break;
    case MENU_NAVIGATE_LEFT:  n = -1; break;
    case MENU_NAVIGATE_RIGHT: n = 1;  break;
  }
  data->wheel_index += n;
  while (data->wheel_index >= data->n_items) {
    data->wheel_index -= data->n_items;
    data->wheel_rot += M_PI * 2.f;
  }
  while (data->wheel_index < 0) {
    data->wheel_index += data->n_items;
    data->wheel_rot -= M_PI * 2.f;
  }
  return 1;
}

static int wheel_destroy_proc(struct menu_item *item)
{
  item->data = NULL;
  return 0;
}

static void create_wheel(struct menu *menu, struct item_data *data)
{
  struct menu_item *item = menu_item_add(menu, 0, 0, NULL, 0);
  item->data = data;
  item->selectable = 0;
  item->draw_proc = wheel_draw_proc;
  item->navigate_proc = wheel_navigate_proc;
  item->destroy_proc = wheel_destroy_proc;
  data->wheel_item = item;
  data->wheel_index = find_item_index(data, *data->value);
  data->wheel_rot = 0.f;
  animate_wheel(data, 1.f);
}

static int enter_proc(struct menu_item *item, enum menu_switch_reason reason)
{
  struct item_data *data = item->data;
  data->anim_state = 0;
  animate_wheel(data, 1.f);
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
  if (data->anim_state > 0) {
    ++draw_params->x;
    ++draw_params->y;
    data->anim_state = (data->anim_state + 1) % 3;
  }
  int cw = menu_get_cell_width(item->owner, 1);
  /* draw background */
  gfx_mode_replace(GFX_MODE_FILTER, G_TF_BILERP);
  if (data->bg_texture) {
    int w = data->bg_texture->tile_width * data->bg_scale;
    int h = data->bg_texture->tile_height * data->bg_scale;
    struct gfx_sprite sprite =
    {
      data->bg_texture, data->bg_texture_tile,
      draw_params->x + (cw - w) / 2,
      draw_params->y - (gfx_font_xheight(draw_params->font) + h + 1) / 2,
      data->bg_scale, data->bg_scale,
    };
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                               draw_params->alpha));
    gfx_sprite_draw(&sprite);
  }
  /* draw item */
  gfx_mode_replace(GFX_MODE_DROPSHADOW, 0);
  {
    int item_id = *data->value;
    int item_index = find_item_index(data, item_id);
    item_id = get_item_id(data, item_index);
    struct gfx_texture *texture;
    int texture_tile;
    uint32_t color;
    float scale;
    if (item_id == Z64_ITEM_NULL) {
      texture = data->empty_texture;
      texture_tile = data->empty_texture_tile;
      color = GPACK_RGB24A8(draw_params->color, draw_params->alpha);
      scale = data->empty_scale;
    }
    else {
      texture = data->texture;
      texture_tile = item_id;
      color = GPACK_RGBA8888(0xFF, 0xFF, 0xFF, draw_params->alpha);
      scale = data->scale;
    }
    if (texture) {
      int w = texture->tile_width * data->scale;
      int h = texture->tile_height * data->scale;
      struct gfx_sprite sprite =
      {
        texture, texture_tile,
        draw_params->x + (cw - w) / 2,
        draw_params->y - (gfx_font_xheight(draw_params->font) + h + 1) / 2,
        scale, scale,
      };
      gfx_mode_set(GFX_MODE_COLOR, color);
      gfx_sprite_draw(&sprite);
    }
  }
  gfx_mode_pop(GFX_MODE_FILTER);
  gfx_mode_pop(GFX_MODE_DROPSHADOW);
  return 1;
}

static int navigate_proc(struct menu_item *item, enum menu_navigation nav)
{
  struct item_data *data = item->data;
  if (data->wheel_item)
    return wheel_navigate_proc(data->wheel_item, nav);
  return 0;
}

static int activate_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (!data->wheel_item) {
    create_wheel(item->owner, data);
    data->anim_state = 1;
  }
  else {
    *data->value = get_item_id(data, data->wheel_index);
    menu_item_remove(data->wheel_item);
    data->wheel_item = NULL;
    if (data->callback_proc)
      data->callback_proc(item, MENU_CALLBACK_CHANGED, data->callback_data);
  }
  return 1;
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  if (data->wheel_item)
    menu_item_remove(data->wheel_item);
  return 0;
}

struct menu_item *item_option_create(struct menu *menu, int x, int y,
                                     int n_items, const int8_t *item_list,
                                     int8_t *value,
                                     struct gfx_texture *texture,
                                     struct gfx_texture *bg_texture,
                                     int bg_texture_tile,
                                     struct gfx_texture *empty_texture,
                                     int empty_texture_tile,
                                     uint32_t color, float scale,
                                     float bg_scale, float empty_scale,
                                     menu_generic_callback callback_proc,
                                     void *callback_data)
{
  struct item_data *data = malloc(sizeof(*data));
  data->n_items = n_items;
  data->item_list = item_list;
  data->value = value;
  data->texture = (texture ? texture : resource_get(RES_ZICON_ITEM));
  data->bg_texture = bg_texture;
  data->bg_texture_tile = bg_texture_tile;
  data->empty_texture = empty_texture;
  data->empty_texture_tile = empty_texture_tile;
  data->color = color;
  data->scale = scale;
  data->bg_scale = (bg_scale != 0.f ? bg_scale : scale);
  data->empty_scale = (empty_scale != 0.f ? empty_scale : scale);
  data->callback_proc = callback_proc;
  data->callback_data = callback_data;
  data->anim_state = 0;
  data->wheel_item = NULL;
  struct menu_item *item = menu_item_add(menu, x, y, NULL, color);
  item->data = data;
  item->enter_proc = enter_proc;
  item->think_proc = think_proc;
  item->draw_proc = draw_proc;
  item->navigate_proc = navigate_proc;
  item->activate_proc = activate_proc;
  item->destroy_proc = destroy_proc;
  return item;
}
