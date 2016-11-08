#include <stdlib.h>
#include "z64.h"
#include "resource.h"
#include "gfx.h"

/* resource data table */
static void *res_data[RES_MAX] = {NULL};

/* resource constructors */
static void *rc_font_generic(struct gfx_texdesc *texdesc,
                             int code_start, int spacing)
{
  struct gfx_font *font = malloc(sizeof(font));
  if (!font)
    return font;
  font->texture = gfx_texture_load(texdesc, NULL);
  if (!font->texture) {
    free(font);
    return NULL;
  }
  font->code_start = code_start;
  font->spacing = spacing;
  return font;
}

static void *rc_zicon_item()
{
  static struct gfx_texdesc td_icon_item_static =
  {
    G_IM_FMT_RGBA, G_IM_SIZ_32b, 0x0,
    32, 32, 90,
    z64_icon_item_static_vaddr,
    z64_icon_item_static_vsize,
  };
  return gfx_texture_load(&td_icon_item_static, NULL);
}

static void *rc_zfont_nes()
{
  struct gfx_texdesc td_nes_font_static =
  {
    G_IM_FMT_I, G_IM_SIZ_4b, 0x0,
    16, 16, 140,
    z64_nes_font_static_vaddr,
    z64_nes_font_static_vsize,
  };
  return rc_font_generic(&td_nes_font_static, 32, -6);
}

static void *rc_font_origamimommy10()
{
#include "origamimommy10.h"
  struct gfx_texdesc td_origamimommy10 =
  {
    G_IM_FMT_RGBA, G_IM_SIZ_32b, (uint32_t)origamimommy10.pixel_data,
    origamimommy10.width, origamimommy10.height / 94, 94,
    GFX_FILE_DRAM, 0,
  };
  return rc_font_generic(&td_origamimommy10, 33, -2);
}

/* resource destructors */
static void rd_font_generic(enum resource_id resource)
{
  struct gfx_font *font = res_data[resource];
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
};

static void (*res_dtor[RES_MAX])() =
{
  gfx_texture_free,
  rd_zfont_nes,
  rd_font_origamimommy10,
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
