#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <mips.h>
#include <n64.h>
#include "gfx.h"
#include "zu.h"
#include "z64.h"

#define   GFX_DISP_SIZE 0x8000
#define   GFX_DATA_SIZE 0x0800
Gfx      *gfx_disp;
Gfx      *gfx_disp_w;
Gfx      *gfx_disp_p;
Gfx      *gfx_disp_e;
char     *gfx_data;
char     *gfx_data_w;
char     *gfx_data_p;
char     *gfx_data_e;

const gfx_colormatrix_t gfx_cm_desaturate =
{
  0.3086, 0.6094, 0.0820, 0.,
  0.3086, 0.6094, 0.0820, 0.,
  0.3086, 0.6094, 0.0820, 0.,
  0.,    0.,    0.,    1.,
};

void gfx_mode_init(int filter, _Bool blend)
{
  static _Bool alloc = 1;
  if (alloc) {
    alloc = 0;
    gfx_disp = malloc(GFX_DISP_SIZE);
    gfx_disp_w = malloc(GFX_DISP_SIZE);
    gfx_disp_p = gfx_disp;
    gfx_disp_e = (void*)((char*)gfx_disp + GFX_DISP_SIZE);
    gfx_data = malloc(GFX_DATA_SIZE);
    gfx_data_w = malloc(GFX_DATA_SIZE);
    gfx_data_p = gfx_data;
    gfx_data_e = (void*)((char*)gfx_data + GFX_DATA_SIZE);
  }

  Gfx g_blend[] =
  {
    gsDPSetCombineMode(G_CC_DECALRGBA,
                       G_CC_DECALRGBA),
    gsDPSetCombineMode(G_CC_MODULATERGBA_PRIM,
                       G_CC_MODULATERGBA_PRIM),
  };
  gfx_disp
  (
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH |
                          G_CULL_BOTH | G_FOG | G_LIGHTING),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTextureFilter(filter),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    g_blend[blend],
  );
}

void gfx_mode_default()
{
  gfx_mode_init(G_TF_BILERP, 0);
}

void gfx_mode_filter(int filter)
{
  gfx_disp
  (
    gsDPPipeSync(),
    gsDPSetTextureFilter(filter),
  );
}

void gfx_mode_blend(_Bool blend)
{
  Gfx g_blend[] =
  {
    gsDPSetCombineMode(G_CC_DECALRGBA,
                       G_CC_DECALRGBA),
    gsDPSetCombineMode(G_CC_MODULATERGBA_PRIM,
                       G_CC_MODULATERGBA_PRIM),
  };
  gfx_disp
  (
    gsDPPipeSync(),
    g_blend[blend],
  );
}

void gfx_mode_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  gfx_disp
  (
    gsDPPipeSync(),
    gsDPSetPrimColor(0, 0, r, g, b, a),
  );
}

Gfx *gfx_disp_append(Gfx *disp, size_t size)
{
  Gfx *p = gfx_disp_p;
  memcpy(gfx_disp_p, disp, size);
  gfx_disp_p += (size + sizeof(Gfx) - 1) / sizeof(Gfx);
  return p;
}

void *gfx_data_append(void *data, size_t size)
{
  void *p = gfx_data_p;
  memcpy(gfx_data_p, data, size);
  gfx_data_p += (size + 7) / 8 * 8;
  return p;
}

void gfx_flush()
{
  gSPEndDisplayList(gfx_disp_p++);
  gSPDisplayList(z64_ctxt.gfx->overlay_disp_p++, gfx_disp);
  Gfx *disp_w = gfx_disp_w;
  gfx_disp_w = gfx_disp;
  gfx_disp = disp_w;
  gfx_disp_p = gfx_disp;
  gfx_disp_e = gfx_disp + (GFX_DISP_SIZE + sizeof(Gfx) - 1) / sizeof(Gfx);
  char *data_w = gfx_data_w;
  gfx_data_w = gfx_data;
  gfx_data = data_w;
  gfx_data_p = gfx_data;
  gfx_data_e = gfx_data + GFX_DATA_SIZE;
}

void gfx_texldr_init(gfx_texldr_t *texldr)
{
  texldr->file_vaddr = GFX_FILE_DRAM;
  texldr->file_data = NULL;
}

gfx_texture_t *gfx_texldr_load(gfx_texldr_t *texldr,
                               const gfx_texdesc_t *texdesc,
                               gfx_texture_t *texture)
{
  gfx_texture_t *new_texture = NULL;
  if (!texture) {
    new_texture = malloc(sizeof(*new_texture));
    if (!new_texture)
      return new_texture;
    texture = new_texture;
  }
  texture->im_fmt = texdesc->im_fmt;
  texture->im_siz = texdesc->im_siz;
  texture->tile_width = texdesc->tile_width;
  texture->tile_height = texdesc->tile_height;
  texture->tiles_x = texdesc->tiles_x;
  texture->tiles_y = texdesc->tiles_y;
  texture->tile_size = ((texture->tile_width * texture->tile_height *
                        G_SIZ_BITS(texture->im_siz) + 7) / 8 + 63) / 64 * 64;
  size_t texture_size = texture->tile_size *
                        texture->tiles_x * texture->tiles_y;
  void *texture_data = NULL;
  void *file_start = NULL;
  if (texdesc->file_vaddr != GFX_FILE_DRAM) {
    if (texldr->file_vaddr != texdesc->file_vaddr) {
      if (texldr->file_data)
        free(texldr->file_data);
      texldr->file_data = memalign(64, texdesc->file_vsize);
      if (!texldr->file_data) {
        texldr->file_vaddr = GFX_FILE_DRAM;
        if (new_texture)
          free(new_texture);
        return NULL;
      }
      texldr->file_vaddr = texdesc->file_vaddr;
      zu_getfile(texldr->file_vaddr, texldr->file_data, texdesc->file_vsize);
    }
    if (texdesc->file_vsize == texture_size) {
      texture_data = texldr->file_data;
      texldr->file_vaddr = GFX_FILE_DRAM;
      texldr->file_data = NULL;
    }
    else
      file_start = texldr->file_data;
  }
  if (!texture_data) {
    texture_data = memalign(64, texture_size);
    if (!texture_data) {
      if (new_texture)
        free(new_texture);
      return NULL;
    }
    memcpy(texture_data, (char*)file_start + texdesc->address, texture_size);
  }
  texture->data = texture_data;
  return texture;
}

void gfx_texldr_destroy(gfx_texldr_t *texldr)
{
  if (texldr->file_data)
    free(texldr->file_data);
}

gfx_texture_t *gfx_texture_load(const gfx_texdesc_t *texdesc,
                                gfx_texture_t *texture)
{
  gfx_texldr_t texldr;
  gfx_texldr_init(&texldr);
  texture = gfx_texldr_load(&texldr, texdesc, texture);
  gfx_texldr_destroy(&texldr);
  return texture;
}

void gfx_texture_destroy(gfx_texture_t *texture)
{
  if (texture->data)
    free(texture->data);
}

void gfx_texture_free(gfx_texture_t *texture)
{
  gfx_texture_destroy(texture);
  free(texture);
}

void *gfx_texture_data(const gfx_texture_t *texture, int16_t tile)
{
  return (char*)texture->data + texture->tile_size * tile;
}

gfx_texture_t *gfx_texture_copy(const gfx_texture_t *src, gfx_texture_t *dest)
{
  gfx_texture_t *new_texture = NULL;
  if (!dest) {
    new_texture = malloc(sizeof(*new_texture));
    if (!new_texture)
      return new_texture;
    dest = new_texture;
  }
  size_t texture_size = src->tile_size * src->tiles_x * src->tiles_y;
  void *texture_data = memalign(64, texture_size);
  if (!texture_data) {
    if (new_texture)
      free(new_texture);
    return NULL;
  }
  *dest = *src;
  dest->data = texture_data;
  memcpy(dest->data, src->data, texture_size);
  return dest;
}

void gfx_texture_colortransform(gfx_texture_t *texture,
                                const gfx_colormatrix_t *matrix)
{
  if (texture->im_fmt != G_IM_FMT_RGBA || texture->im_siz != G_IM_SIZ_32b)
    return;
  struct rgba32
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  size_t texture_pixels = texture->tile_width * texture->tile_height *
                          texture->tiles_x * texture->tiles_y;
  struct rgba32 *pixel_data = texture->data;
  gfx_colormatrix_t m = *matrix;
  for (size_t i = 0; i < texture_pixels; ++i)
  {
    struct rgba32 p = pixel_data[i];
    float r = p.r * m.rr + p.g * m.rg + p.b * m.rb + p.a * m.ra;
    float g = p.r * m.gr + p.g * m.gg + p.b * m.gb + p.a * m.ga;
    float b = p.r * m.br + p.g * m.bg + p.b * m.bb + p.a * m.ba;
    float a = p.r * m.ar + p.g * m.ag + p.b * m.ab + p.a * m.aa;
    struct rgba32 n =
    {
      r < 0x00 ? 0x00 : r > 0xFF ? 0xFF : r,
      g < 0x00 ? 0x00 : g > 0xFF ? 0xFF : g,
      b < 0x00 ? 0x00 : b > 0xFF ? 0xFF : b,
      a < 0x00 ? 0x00 : a > 0xFF ? 0xFF : a,
    };
    pixel_data[i] = n;
  }
}

void gfx_rdp_load_tile(const gfx_texture_t *texture, int16_t texture_tile)
{
  if (texture->im_siz == G_IM_SIZ_4b) {
    gfx_disp
    (
      gsDPLoadTextureTile_4b(gfx_texture_data(texture, texture_tile),
                             texture->im_fmt,
                             texture->tile_width, texture->tile_height,
                             0, 0,
                             texture->tile_width - 1, texture->tile_height - 1,
                             0,
                             G_TX_NOMIRROR | G_TX_WRAP,
                             G_TX_NOMIRROR | G_TX_WRAP,
                             G_TX_NOMASK, G_TX_NOMASK,
                             G_TX_NOLOD, G_TX_NOLOD),
    );
  }
  else {
    gfx_disp
    (
      gsDPLoadTextureTile(gfx_texture_data(texture, texture_tile),
                          texture->im_fmt, texture->im_siz,
                          texture->tile_width, texture->tile_height,
                          0, 0,
                          texture->tile_width - 1, texture->tile_height - 1,
                          0,
                          G_TX_NOMIRROR | G_TX_WRAP,
                          G_TX_NOMIRROR | G_TX_WRAP,
                          G_TX_NOMASK, G_TX_NOMASK,
                          G_TX_NOLOD, G_TX_NOLOD),
    );
  }
}

void gfx_sprite_draw(const gfx_sprite_t *sprite)
{
  gfx_texture_t *texture = sprite->texture;
  gfx_rdp_load_tile(texture, sprite->texture_tile);
  gfx_disp
  (
    gsSPScisTextureRectangle(qs102(sprite->x) & ~3,
                             qs102(sprite->y) & ~3,
                             qs102(sprite->x + texture->tile_width *
                                   sprite->xscale) & ~3,
                             qs102(sprite->y + texture->tile_height *
                                   sprite->yscale) & ~3,
                             G_TX_RENDERTILE,
                             qu105(0), qu105(0),
                             qu510(1.f / sprite->xscale),
                             qu510(1.f / sprite->yscale)),
  );
}

void gfx_printf(const gfx_font_t *font, int x, int y, const char *format, ...)
{
  const size_t bufsize = 1024;
  gfx_texture_t *texture = font->texture;
  int chars_per_tile = font->chars_xtile * font->chars_ytile;
  int no_tiles = texture->tiles_x * texture->tiles_y;
  int no_chars = chars_per_tile * no_tiles;
  char buf[bufsize];
  va_list args;
  va_start(args, format);
  int l = vsnprintf(buf, bufsize, format, args);
  if (l > bufsize - 1)
    l = bufsize - 1;
  va_end(args);
  for (int i = 0; i < no_tiles; ++i) {
    int tile_begin = chars_per_tile * i;
    int tile_end = tile_begin + chars_per_tile;
    _Bool tile_loaded = 0;
    int cx = 0;
    int cy = 0;
    for (int j = 0; j < l; ++j, cx += font->char_width + font->spacing) {
      uint8_t c = buf[j];
      if (c < font->code_start || c >= font->code_start + no_chars)
        continue;
      c -= font->code_start;
      if (c < tile_begin || c >= tile_end)
        continue;
      c -= tile_begin;
      if (!tile_loaded) {
        tile_loaded = 1;
        gfx_rdp_load_tile(texture, i);
      }
      gfx_disp
      (
        gsSPScisTextureRectangle(qs102(x + cx),
                                 qs102(y + cy),
                                 qs102(x + cx + font->char_width),
                                 qs102(y + cy + font->char_height),
                                 G_TX_RENDERTILE,
                                 qu105(c % font->chars_xtile *
                                       font->char_width),
                                 qu105(c / font->chars_xtile *
                                       font->char_height),
                                 qu510(1), qu510(1)),
      );
    }
  }
}
