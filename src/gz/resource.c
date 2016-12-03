#include <stdlib.h>
#include <grc.h>
#include "gfx.h"
#include "resource.h"
#include "z64.h"

/* resource data table */
static void *res_data[RES_MAX] = {NULL};

/* resource constructors */
static void *rc_font_generic(struct gfx_texdesc *texdesc,
                             int char_width, int char_height,
                             int code_start,
                             int letter_spacing, int line_spacing)
{
  struct gfx_font *font = malloc(sizeof(*font));
  if (!font)
    return font;
  struct gfx_texture *texture = gfx_texture_load(texdesc, NULL);
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
  font->letter_spacing = letter_spacing;
  font->line_spacing = line_spacing;
  return font;
}

static void *rc_grc_font_generic(const char *grc_resource_name,
                                 int char_width, int char_height,
                                 int code_start,
                                 int letter_spacing, int line_spacing)
{
  void *p_t;
  grc_resource_get(grc_resource_name, &p_t, NULL);
  if (!p_t)
    return p_t;
  struct grc_texture *t = p_t;
  struct gfx_texdesc td =
  {
    t->im_fmt, t->im_siz, (uint32_t)&t->texture_data,
    t->tile_width, t->tile_height, t->tiles_x, t->tiles_y,
    GFX_FILE_DRAM, 0,
  };
  return rc_font_generic(&td, char_width, char_height, code_start,
                         letter_spacing, line_spacing);
}

static void *rc_zicon_item()
{
  static struct gfx_texdesc td_icon_item_static =
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
  static struct gfx_texdesc td_nes_font_static =
  {
    G_IM_FMT_I, G_IM_SIZ_4b, 0x0,
    16, 224, 1, 10,
    z64_nes_font_static_vaddr,
    z64_nes_font_static_vsize,
  };
  return rc_font_generic(&td_nes_font_static, 16, 16, 32, -6, -6);
}

static void *rc_font_fipps()
{
  return rc_grc_font_generic("fipps", 10, 14, 33, -2, -5);
}

static void *rc_font_notalot35()
{
  return rc_grc_font_generic("notalot35", 8, 9, 33, -1, -1);
}

static void *rc_font_origamimommy()
{
  return rc_grc_font_generic("origamimommy", 8, 10, 33, -2, -2);
}

static void *rc_font_pcsenior()
{
  return rc_grc_font_generic("pcsenior", 8, 8, 33, 0, 0);
}

static void *rc_font_pixelintv()
{
  return rc_grc_font_generic("pixelintv", 8, 12, 33, 0, -4);
}

static void *rc_font_pressstart2p()
{
  return rc_grc_font_generic("pressstart2p", 8, 8, 33, 0, 0);
}

static void *rc_font_smwtextnc()
{
  return rc_grc_font_generic("smwtextnc", 12, 8, 33, -4, 0);
}

static void *rc_font_werdnasreturn()
{
  return rc_grc_font_generic("werdnasreturn", 8, 12, 33, 0, -4);
}

static void *rc_texture_crosshair()
{
  void *p_t;
  grc_resource_get("crosshair", &p_t, NULL);
  struct grc_texture *t = p_t;
  struct gfx_texdesc td_crosshair =
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
  struct gfx_font *font = res_data[resource];
  gfx_texture_free(font->texture);
  free(font);
}

static void rd_zfont_nes()
{
  rd_font_generic(RES_ZFONT_NES);
}

static void rd_font_fipps()
{
  rd_font_generic(RES_FONT_FIPPS);
}

static void rd_font_notalot35()
{
  rd_font_generic(RES_FONT_NOTALOT35);
}

static void rd_font_origamimommy()
{
  rd_font_generic(RES_FONT_ORIGAMIMOMMY);
}

static void rd_font_pcsenior()
{
  rd_font_generic(RES_FONT_PCSENIOR);
}

static void rd_font_pixelintv()
{
  rd_font_generic(RES_FONT_PIXELINTV);
}

static void rd_font_pressstart2p()
{
  rd_font_generic(RES_FONT_PRESSSTART2P);
}

static void rd_font_smwtextnc()
{
  rd_font_generic(RES_FONT_SMWTEXTNC);
}

static void rd_font_werdnasreturn()
{
  rd_font_generic(RES_FONT_WERDNASRETURN);
}

/* resource management tables */
static void *(*res_ctor[RES_MAX])() =
{
  rc_zicon_item,
  rc_zfont_nes,
  rc_font_fipps,
  rc_font_notalot35,
  rc_font_origamimommy,
  rc_font_pcsenior,
  rc_font_pixelintv,
  rc_font_pressstart2p,
  rc_font_smwtextnc,
  rc_font_werdnasreturn,
  rc_texture_crosshair,
};

static void (*res_dtor[RES_MAX])() =
{
  gfx_texture_free,
  rd_zfont_nes,
  rd_font_fipps,
  rd_font_notalot35,
  rd_font_origamimommy,
  rd_font_pcsenior,
  rd_font_pixelintv,
  rd_font_pressstart2p,
  rd_font_smwtextnc,
  rd_font_werdnasreturn,
  gfx_texture_free,
};

/* resource interface */
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
