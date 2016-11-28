#include <stdlib.h>
#include <grc.h>
#include "z64.h"
#include "resource.h"
#include "gfx.h"

/* resource data table */
static void *res_data[RES_MAX] = {NULL};

/* resource constructors */
static void *rc_font_generic(gfx_texdesc_t *texdesc,
                             int char_width, int char_height,
                             int code_start, int spacing)
{
  gfx_font_t *font = malloc(sizeof(*font));
  if (!font)
    return font;
  gfx_texture_t *texture = gfx_texture_load(texdesc, NULL);
  if (!texture) {
    free(font);
    return NULL;
  }
  font->texture = texture;
  font->char_width = char_width;
  font->char_height = char_height;
  font->chars_xtile = texture->tile_width / font->char_width;
  font->chars_ytile = texture->tile_height / font->char_height;
  font->code_start = code_start;
  font->spacing = spacing;
  return font;
}

static void *rc_zicon_item()
{
  static gfx_texdesc_t td_icon_item_static =
  {
    G_IM_FMT_RGBA, G_IM_SIZ_32b, 0x0,
    32, 32, 1, 90,
    z64_icon_item_static_vaddr,
    z64_icon_item_static_vsize,
  };
  return gfx_texture_load(&td_icon_item_static, NULL);
}

static void *rc_zfont_nes()
{
  static gfx_texdesc_t td_nes_font_static =
  {
    G_IM_FMT_I, G_IM_SIZ_4b, 0x0,
    16, 224, 1, 10,
    z64_nes_font_static_vaddr,
    z64_nes_font_static_vsize,
  };
  return rc_font_generic(&td_nes_font_static, 16, 16, 32, -6);
}

static void *rc_font_origamimommy10()
{
  void *p_t;
  grc_resource_get("origamimommy10", &p_t, NULL);
  grc_texture_t *t = p_t;
  gfx_texdesc_t td_origamimommy10 =
  {
    t->im_fmt, t->im_siz, (uint32_t)&t->texture_data,
    t->tile_width, t->tile_height, t->tiles_x, t->tiles_y,
    GFX_FILE_DRAM, 0,
  };
  return rc_font_generic(&td_origamimommy10, 8, 10, 33, -2);
}

static void *rc_texture_crosshair()
{
  void *p_t;
  grc_resource_get("crosshair", &p_t, NULL);
  grc_texture_t *t = p_t;
  gfx_texdesc_t td_crosshair =
  {
    t->im_fmt, t->im_siz, (uint32_t)&t->texture_data,
    t->tile_width, t->tile_height, t->tiles_x, t->tiles_y,
    GFX_FILE_DRAM, 0,
  };
  return gfx_texture_load(&td_crosshair, NULL);
}

/* resource destructors */
static void rd_font_generic(enum resource_id resource)
{
  gfx_font_t *font = res_data[resource];
  gfx_texture_free(font->texture);
  free(font);
}

static void rd_zfont_nes()
{
  rd_font_generic(RES_ZFONT_NES);
}

static void rd_font_origamimommy10()
{
  rd_font_generic(RES_FONT_ORIGAMIMOMMY10);
}

/* resource management tables */
static void *(*res_ctor[RES_MAX])() =
{
  rc_zicon_item,
  rc_zfont_nes,
  rc_font_origamimommy10,
  rc_texture_crosshair,
};

static void (*res_dtor[RES_MAX])() =
{
  gfx_texture_free,
  rd_zfont_nes,
  rd_font_origamimommy10,
  gfx_texture_free,
};


void *resource_get(enum resource_id res)
{
  if (!res_data[res])
    res_data[res] = res_ctor[res]();
  return res_data[res];
}

void resource_free(enum resource_id res)
{
  if (res_data[res]) {
    res_dtor[res]();
    res_data[res] = NULL;
  }
}
