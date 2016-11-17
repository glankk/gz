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

#define       GFX_DISP_SIZE   0x3000
#define       GFX_DATA_SIZE   0x400
static char   gfx_disp_buffers[2][GFX_DISP_SIZE];
static char   gfx_data_buffers[2][GFX_DATA_SIZE];
Gfx          *gfx_disp      = (void*)&gfx_disp_buffers[0][0];
Gfx          *gfx_disp_p    = (void*)&gfx_disp_buffers[0][0];
Gfx          *gfx_disp_e    = (void*)&gfx_disp_buffers[0][GFX_DISP_SIZE];
Gfx          *gfx_disp_w    = (void*)&gfx_disp_buffers[1][0];
char         *gfx_data      = (void*)&gfx_data_buffers[0][0];
char         *gfx_data_p    = (void*)&gfx_data_buffers[0][0];
char         *gfx_data_e    = (void*)&gfx_data_buffers[0][GFX_DATA_SIZE];
char         *gfx_data_w    = (void*)&gfx_data_buffers[1][0];

const struct gfx_colormatrix gfx_cm_desaturate =
{
  0.3086, 0.6094, 0.0820, 0.,
  0.3086, 0.6094, 0.0820, 0.,
  0.3086, 0.6094, 0.0820, 0.,
  0.,    0.,    0.,    1.,
};

void gfx_mode_init(int filter, _Bool blend)
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

void gfx_texldr_init(struct gfx_texldr *texldr)
{
  texldr->file_vaddr = GFX_FILE_DRAM;
  texldr->file_data = NULL;
}

struct gfx_texture *gfx_texldr_load(struct gfx_texldr *texldr,
                                    const struct gfx_texdesc *texdesc,
                                    struct gfx_texture *texture)
{
  struct gfx_texture *new_texture = NULL;
  if (!texture) {
    new_texture = malloc(sizeof(*new_texture));
    if (!new_texture)
      return new_texture;
    texture = new_texture;
  }
  size_t texture_size = (texdesc->width * texdesc->height *
                         G_SIZ_BITS(texdesc->siz) + 7) / 8 * texdesc->images;
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
  texture->fmt = texdesc->fmt;
  texture->siz = texdesc->siz;
  texture->data = texture_data;
  texture->width = texdesc->width;
  texture->height = texdesc->height;
  texture->images = texdesc->images;
  return texture;
}

void gfx_texldr_destroy(struct gfx_texldr *texldr)
{
  if (texldr->file_data)
    free(texldr->file_data);
}

struct gfx_texture *gfx_texture_load(const struct gfx_texdesc *texdesc,
                                     struct gfx_texture *texture)
{
  struct gfx_texldr texldr;
  gfx_texldr_init(&texldr);
  texture = gfx_texldr_load(&texldr, texdesc, texture);
  gfx_texldr_destroy(&texldr);
  return texture;
}

void gfx_texture_destroy(struct gfx_texture *texture)
{
  if (texture->data)
    free(texture->data);
}

void gfx_texture_free(struct gfx_texture *texture)
{
  gfx_texture_destroy(texture);
  free(texture);
}

size_t gfx_texture_image_size(const struct gfx_texture *texture)
{
  return (texture->width * texture->height * G_SIZ_BITS(texture->siz) + 7) / 8;
}

void *gfx_texture_data(const struct gfx_texture *texture, int16_t image)
{
  return (char*)texture->data + gfx_texture_image_size(texture) * image;
}

struct gfx_texture *gfx_texture_copy(const struct gfx_texture *src,
                                     struct gfx_texture *dest)
{
  struct gfx_texture *new_texture = NULL;
  if (!dest) {
    new_texture = malloc(sizeof(*new_texture));
    if (!new_texture)
      return new_texture;
    dest = new_texture;
  }
  size_t texture_size = gfx_texture_image_size(src) * src->images;
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

void gfx_texture_colortransform(struct gfx_texture *texture,
                                const struct gfx_colormatrix *matrix)
{
  struct rgba32
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  size_t texture_pixels = texture->width * texture->height * texture->images;
  struct rgba32 *pixel_data = texture->data;
  struct gfx_colormatrix m = *matrix;
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

void gfx_sprite_draw(const struct gfx_sprite *sprite)
{
  struct gfx_texture *texture = sprite->texture;
  void *pixel_data = gfx_texture_data(texture, sprite->texture_image);
  size_t tmem_size = 0x1000;
  if (texture->siz == G_IM_SIZ_4b)
    tmem_size /= 2;
  int tile_pixels = tmem_size * 8 / G_SIZ_BITS(texture->siz);
  int tile_width = (texture->width < tile_pixels ?
                    texture->width : tile_pixels);
  int tile_height = tile_pixels / tile_width;
  for (int y1 = 0; y1 < texture->height; y1 += tile_height) {
    int y2 = y1 + tile_height;
    if (y2 > texture->height)
      y2 = texture->height;
    for (int x1 = 0; x1 < texture->width; x1 += tile_width) {
      int x2 = x1 + tile_width;
      if (x2 > texture->width)
        x2 = texture->width;
      if (texture->siz == G_IM_SIZ_4b) {
        gfx_disp
        (
          gsDPLoadTextureTile_4b(pixel_data,
                                 texture->fmt,
                                 texture->width, texture->height,
                                 x1, y1,
                                 x2 - 1, y2 - 1,
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
          gsDPLoadTextureTile(pixel_data,
                              texture->fmt, texture->siz,
                              texture->width, texture->height,
                              x1, y1,
                              x2 - 1, y2 - 1,
                              0,
                              G_TX_NOMIRROR | G_TX_WRAP,
                              G_TX_NOMIRROR | G_TX_WRAP,
                              G_TX_NOMASK, G_TX_NOMASK,
                              G_TX_NOLOD, G_TX_NOLOD),
        );
      }
      gfx_disp
      (
        gsSPScisTextureRectangle(qs102(sprite->x + x1 * sprite->xscale) & ~3,
                                 qs102(sprite->y + y1 * sprite->yscale) & ~3,
                                 qs102(sprite->x + x2 * sprite->xscale) & ~3,
                                 qs102(sprite->y + y2 * sprite->yscale) & ~3,
                                 G_TX_RENDERTILE,
                                 qu105(x1), qu105(y1),
                                 qu510(1.f / sprite->xscale),
                                 qu510(1.f / sprite->yscale)),
      );
    }
  }
}

void gfx_printf(const struct gfx_font *font, int x, int y,
                const char *format, ...)
{
  const size_t bufsize = 1024;
  struct gfx_texture *texture = font->texture;
  size_t char_size = gfx_texture_image_size(texture);
  size_t tmem_size = 0x1000;
  if (texture->siz == G_IM_SIZ_4b)
    tmem_size /= 2;
  int chars_per_tile = tmem_size / char_size;
  int num_tiles = (texture->images + chars_per_tile - 1) / chars_per_tile;
  char buf[bufsize];
  va_list args;
  va_start(args, format);
  int l = vsnprintf(buf, bufsize, format, args);
  if (l > bufsize - 1)
    l = bufsize - 1;
  va_end(args);
  for (int i = 0; i < num_tiles; ++i) {
    int tile_begin = chars_per_tile * i;
    int tile_end = tile_begin + chars_per_tile;
    if (tile_end > texture->images) {
      tile_end = texture->images;
      chars_per_tile = tile_end - tile_begin;
    }
    _Bool tile_loaded = 0;
    int cx = 0;
    int cy = 0;
    for (int j = 0; j < l; ++j, cx += texture->width + font->spacing) {
      uint8_t c = buf[j];
      if (c < font->code_start || c >= font->code_start + texture->images)
        continue;
      c -= font->code_start;
      if (c < tile_begin || c >= tile_end)
        continue;
      c -= tile_begin;
      if (!tile_loaded) {
        tile_loaded = 1;
        void *pixel_data = gfx_texture_data(texture, tile_begin);
        int tile_width = texture->width;
        int tile_height = texture->height * chars_per_tile;
        if (texture->siz == G_IM_SIZ_4b) {
          gfx_disp
          (
            gsDPLoadTextureTile_4b(pixel_data,
                                   texture->fmt,
                                   tile_width, tile_height,
                                   0, 0,
                                   tile_width - 1, tile_height - 1,
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
            gsDPLoadTextureTile(pixel_data,
                                texture->fmt, texture->siz,
                                tile_width, tile_height,
                                0, 0,
                                tile_width - 1, tile_height - 1,
                                0,
                                G_TX_NOMIRROR | G_TX_WRAP,
                                G_TX_NOMIRROR | G_TX_WRAP,
                                G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD),
          );
        }
      }
      gfx_disp
      (
        gsSPScisTextureRectangle(qs102(x + cx),
                                 qs102(y + cy),
                                 qs102(x + cx + texture->width),
                                 qs102(y + cy + texture->height),
                                 G_TX_RENDERTILE,
                                 qu105(0), qu105(c * texture->height),
                                 qu510(1), qu510(1)),
      );
    }
  }
}
