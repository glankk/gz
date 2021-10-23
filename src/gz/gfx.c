#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <mips.h>
#include <n64.h>
#include "gfx.h"
#include "util.h"
#include "z64.h"
#include "zu.h"

#define           GFX_DISP_SIZE     0x10000
static Gfx       *gfx_disp;
static Gfx       *gfx_disp_w;
static Gfx       *gfx_disp_p;
static Gfx       *gfx_disp_d;

#define           GFX_STACK_LENGTH  8
static uint64_t   gfx_modes[GFX_MODE_ALL];
static uint64_t   gfx_mode_stack[GFX_MODE_ALL][GFX_STACK_LENGTH];
static int        gfx_mode_stack_pos[GFX_MODE_ALL];
static _Bool      gfx_synced;

#define           CHAR_TILE_MAX     8
struct gfx_char
{
  int             tile_char;
  uint32_t        color;
  int             x;
  int             y;
};
static const
struct gfx_font  *gfx_char_font;
static
struct vector     gfx_chars[CHAR_TILE_MAX];

static void draw_chars(const struct gfx_font *font, int x, int y,
                       const char *buf, size_t l);
static void flush_chars(void);
static void gfx_printf_n_va(const struct gfx_font *font, int x, int y,
                            const char *format, va_list args);
static void gfx_printf_f_va(const struct gfx_font *font, int x, int y,
                            const char *format, va_list args);

static inline void gfx_sync(void)
{
  if (!gfx_synced) {
    gDPPipeSync(gfx_disp_p++);
    gfx_synced = 1;
  }
}

const MtxF gfx_cm_desaturate = guDefMtxF(0.3086f, 0.6094f, 0.0820f, 0.f,
                                         0.3086f, 0.6094f, 0.0820f, 0.f,
                                         0.3086f, 0.6094f, 0.0820f, 0.f,
                                         0.f,     0.f,     0.f,     1.f);

void gfx_start(void)
{
  for (int i = 0; i < CHAR_TILE_MAX; ++i)
    vector_init(&gfx_chars[i], sizeof(struct gfx_char));
  gfx_disp = malloc(GFX_DISP_SIZE);
  gfx_disp_w = malloc(GFX_DISP_SIZE);
  gfx_disp_p = gfx_disp;
  gfx_disp_d = gfx_disp + (GFX_DISP_SIZE + sizeof(*gfx_disp) - 1) /
               sizeof(*gfx_disp);
}

void gfx_mode_init(void)
{
  gfx_sync();
  static Gfx mode_init[] = {
    gsSPLoadGeometryMode(0),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsDPSetScissor(G_SC_NON_INTERLACE,
                  0, 0, Z64_SCREEN_WIDTH, Z64_SCREEN_HEIGHT),
    gsDPSetAlphaDither(G_AD_DISABLE),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPPipelineMode(G_PM_NPRIMITIVE),
    gsSPEndDisplayList(),
  };
  gSPDisplayList(gfx_disp_p++, mode_init);
  gfx_mode_apply(GFX_MODE_ALL);
}

void gfx_mode_configure(enum gfx_mode mode, uint64_t value)
{
  gfx_modes[mode] = value;
}

void gfx_mode_apply(enum gfx_mode mode)
{
  Gfx dl[GFX_MODE_ALL];
  Gfx *pdl = dl;
  switch (mode) {
    case GFX_MODE_ALL:
    case GFX_MODE_FILTER: {
      gDPSetTextureFilter(pdl++, gfx_modes[GFX_MODE_FILTER]);
      if (mode != GFX_MODE_ALL)
        break;
    }
    case GFX_MODE_COMBINE: {
      gDPSetCombine(pdl++, gfx_modes[GFX_MODE_COMBINE]);
      if (mode != GFX_MODE_ALL)
        break;
    }
    case GFX_MODE_COLOR: {
      uint32_t c = gfx_modes[GFX_MODE_COLOR];
      gDPSetPrimColor(pdl++, 0, 0, (c >> 24) & 0xFF, (c >> 16) & 0xFF,
                                   (c >> 8)  & 0xFF, (c >> 0)  & 0xFF);
      if (mode != GFX_MODE_ALL)
        break;
    }
    default:
      break;
  }
  size_t s = pdl - dl;
  if (s > 0) {
    gfx_sync();
    memcpy(gfx_disp_p, dl, s * sizeof(*dl));
    gfx_disp_p += s;
  }
}

void gfx_mode_set(enum gfx_mode mode, uint64_t value)
{
  gfx_mode_configure(mode, value);
  gfx_mode_apply(mode);
}

void gfx_mode_push(enum gfx_mode mode)
{
  if (mode == GFX_MODE_ALL) {
    for (int i = 0; i < GFX_MODE_ALL; ++i) {
      int *p = &gfx_mode_stack_pos[i];
      gfx_mode_stack[i][*p] = gfx_modes[i];
      *p = (*p + 1) % GFX_STACK_LENGTH;
    }
  }
  else {
    int *p = &gfx_mode_stack_pos[mode];
    gfx_mode_stack[mode][*p] = gfx_modes[mode];
    *p = (*p + 1) % GFX_STACK_LENGTH;
  }
}

void gfx_mode_pop(enum gfx_mode mode)
{
  if (mode == GFX_MODE_ALL) {
    for (int i = 0; i < GFX_MODE_ALL; ++i) {
      int *p = &gfx_mode_stack_pos[i];
      *p = (*p + GFX_STACK_LENGTH - 1) % GFX_STACK_LENGTH;
      gfx_mode_set(i, gfx_mode_stack[i][*p]);
    }
  }
  else {
    int *p = &gfx_mode_stack_pos[mode];
    *p = (*p + GFX_STACK_LENGTH - 1) % GFX_STACK_LENGTH;
    gfx_mode_set(mode, gfx_mode_stack[mode][*p]);
  }
}

void gfx_mode_replace(enum gfx_mode mode, uint64_t value)
{
  gfx_mode_push(mode);
  gfx_mode_configure(mode, value);
  gfx_mode_apply(mode);
}

Gfx *gfx_disp_append(Gfx *disp, size_t size)
{
  Gfx *p = gfx_disp_p;
  memcpy(gfx_disp_p, disp, size);
  gfx_disp_p += (size + sizeof(*gfx_disp_p) - 1) / sizeof(*gfx_disp_p);
  gfx_synced = 0;
  return p;
}

void *gfx_data_append(void *data, size_t size)
{
  gfx_disp_d -= (size + sizeof(*gfx_disp_d) - 1) / sizeof(*gfx_disp_d);
  memcpy(gfx_disp_d, data, size);
  return gfx_disp_d;
}

void gfx_flush(void)
{
  flush_chars();
  gSPEndDisplayList(gfx_disp_p++);
  dcache_wb(gfx_disp, GFX_DISP_SIZE);
  gSPDisplayList(z64_ctxt.gfx->overlay.p++, gfx_disp);

  Gfx *disp_w = gfx_disp_w;
  gfx_disp_w = gfx_disp;
  gfx_disp = disp_w;
  gfx_disp_p = gfx_disp;
  gfx_disp_d = gfx_disp + (GFX_DISP_SIZE + sizeof(*gfx_disp) - 1) /
               sizeof(*gfx_disp);
  gfx_synced = 0;
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
  texture->im_fmt = texdesc->im_fmt;
  texture->im_siz = texdesc->im_siz;
  texture->tile_width = texdesc->tile_width;
  texture->tile_height = texdesc->tile_height;
  texture->tiles_x = texdesc->tiles_x;
  texture->tiles_y = texdesc->tiles_y;
  texture->tile_size = (texture->tile_width * texture->tile_height *
                        G_SIZ_BITS(texture->im_siz) + 63) / 64 * 8;
  size_t texture_size = texture->tile_size *
                        texture->tiles_x * texture->tiles_y;
  void *texture_data = NULL;
  void *file_start = NULL;
  if (texdesc->file_vaddr != GFX_FILE_DRAM) {
    if (texldr->file_vaddr != texdesc->file_vaddr) {
      if (texldr->file_data)
        free(texldr->file_data);
      texldr->file_data = malloc(texdesc->file_vsize);
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
    texture_data = malloc(texture_size);
    if (!texture_data) {
      if (new_texture)
        free(new_texture);
      return NULL;
    }
    memcpy(texture_data, (char *)file_start + texdesc->address, texture_size);
  }
  texture->data = texture_data;
  return texture;
}

void gfx_texldr_destroy(struct gfx_texldr *texldr)
{
  if (texldr->file_data)
    free(texldr->file_data);
}

struct gfx_texture *gfx_texture_create(g_ifmt_t im_fmt, g_isiz_t im_siz,
                                       int tile_width, int tile_height,
                                       int tiles_x, int tiles_y)
{
  struct gfx_texture *texture = malloc(sizeof(*texture));
  if (!texture)
    return texture;
  texture->tile_size = (tile_width * tile_height *
                        G_SIZ_BITS(im_siz) + 63) / 64 * 8;
  texture->data = malloc(tiles_x * tiles_y * texture->tile_size);
  if (!texture->data) {
    free(texture);
    return NULL;
  }
  texture->im_fmt = im_fmt;
  texture->im_siz = im_siz;
  texture->tile_width = tile_width;
  texture->tile_height = tile_height;
  texture->tiles_x = tiles_x;
  texture->tiles_y = tiles_y;
  return texture;
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

void *gfx_texture_data(const struct gfx_texture *texture, int16_t tile)
{
  return (char *)texture->data + texture->tile_size * tile;
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
  size_t texture_size = src->tile_size * src->tiles_x * src->tiles_y;
  void *texture_data = malloc(texture_size);
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

void gfx_texture_copy_tile(struct gfx_texture *dest, int dest_tile,
                           const struct gfx_texture *src, int src_tile,
                           _Bool blend)
{
  if (src->im_fmt != G_IM_FMT_RGBA || src->im_siz != G_IM_SIZ_32b ||
      dest->im_fmt != src->im_fmt || dest->im_siz != src->im_siz ||
      dest->tile_width != src->tile_width ||
      dest->tile_height != src->tile_height)
    return;
  struct rgba32
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };
  size_t tile_pixels = src->tile_width * src->tile_height;
  struct rgba32 *p_dest = gfx_texture_data(dest, dest_tile);
  struct rgba32 *p_src = gfx_texture_data(src, src_tile);
  for (size_t i = 0; i < tile_pixels; ++i) {
    if (blend) {
      p_dest->r = p_dest->r + (p_src->r - p_dest->r) * p_src->a / 0xFF;
      p_dest->g = p_dest->g + (p_src->g - p_dest->g) * p_src->a / 0xFF;
      p_dest->b = p_dest->b + (p_src->b - p_dest->b) * p_src->a / 0xFF;
      p_dest->a = p_src->a + (0xFF - p_src->a) * p_dest->a / 0xFF;
    }
    else
      *p_dest = *p_src;
    ++p_dest;
    ++p_src;
  }
}

void gfx_texture_colortransform(struct gfx_texture *texture,
                                const MtxF *matrix)
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
  MtxF m = *matrix;
  for (size_t i = 0; i < texture_pixels; ++i)
  {
    struct rgba32 p = pixel_data[i];
    float r = p.r * m.xx + p.g * m.xy + p.b * m.xz + p.a * m.xw;
    float g = p.r * m.yx + p.g * m.yy + p.b * m.yz + p.a * m.yw;
    float b = p.r * m.zx + p.g * m.zy + p.b * m.zz + p.a * m.zw;
    float a = p.r * m.wx + p.g * m.wy + p.b * m.wz + p.a * m.ww;
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

void gfx_disp_rdp_load_tile(Gfx **disp,
                            const struct gfx_texture *texture,
                            int16_t texture_tile)
{
  if (texture->im_siz == G_IM_SIZ_4b) {
    gDPLoadTextureTile_4b((*disp)++,
                          gfx_texture_data(texture, texture_tile),
                          texture->im_fmt,
                          texture->tile_width, texture->tile_height,
                          0, 0,
                          texture->tile_width - 1, texture->tile_height - 1,
                          0,
                          G_TX_NOMIRROR | G_TX_WRAP,
                          G_TX_NOMIRROR | G_TX_WRAP,
                          G_TX_NOMASK, G_TX_NOMASK,
                          G_TX_NOLOD, G_TX_NOLOD);
  }
  else {
    gDPLoadTextureTile((*disp)++,
                       gfx_texture_data(texture, texture_tile),
                       texture->im_fmt, texture->im_siz,
                       texture->tile_width, texture->tile_height,
                       0, 0,
                       texture->tile_width - 1, texture->tile_height - 1,
                       0,
                       G_TX_NOMIRROR | G_TX_WRAP,
                       G_TX_NOMIRROR | G_TX_WRAP,
                       G_TX_NOMASK, G_TX_NOMASK,
                       G_TX_NOLOD, G_TX_NOLOD);
  }
}

void gfx_rdp_load_tile(const struct gfx_texture *texture, int16_t texture_tile)
{
  gfx_disp_rdp_load_tile(&gfx_disp_p, texture, texture_tile);
  gfx_synced = 1;
}

void gfx_sprite_draw(const struct gfx_sprite *sprite)
{
  struct gfx_texture *texture = sprite->texture;
  qs102_t ulx;
  qs102_t uly;
  qs102_t lrx;
  qs102_t lry;
  qs105_t uls;
  qs105_t ult;
  qs510_t dsdx = qs510(1.f / sprite->xscale);
  qs510_t dtdy = qs510(1.f / sprite->yscale);
  if (dsdx < 0) {
    ulx = qs102(sprite->x + texture->tile_width * sprite->xscale);
    lrx = qs102(sprite->x);
    uls = qs105(texture->tile_width - 1);
  }
  else {
    ulx = qs102(sprite->x);
    lrx = qs102(sprite->x + texture->tile_width * sprite->xscale);
    uls = qs105(0);
  }
  if (dtdy < 0) {
    uly = qs102(sprite->y + texture->tile_height * sprite->yscale);
    lry = qs102(sprite->y);
    ult = qs105(texture->tile_height - 1);
  }
  else {
    uly = qs102(sprite->y);
    lry = qs102(sprite->y + texture->tile_height * sprite->yscale);
    ult = qs105(0);
  }
  gfx_rdp_load_tile(texture, sprite->texture_tile);
  if (gfx_modes[GFX_MODE_DROPSHADOW]) {
    uint8_t a = gfx_modes[GFX_MODE_COLOR] & 0xFF;
    a = a * a / 0xFF;
    gfx_mode_replace(GFX_MODE_COLOR, GPACK_RGBA8888(0x00, 0x00, 0x00, a));
    gSPScisTextureRectangle(gfx_disp_p++,
                            ulx + qs102(1), uly + qs102(1),
                            lrx + qs102(1), lry + qs102(1),
                            G_TX_RENDERTILE,
                            uls, ult,
                            dsdx, dtdy);
    gfx_mode_pop(GFX_MODE_COLOR);
  }
  gfx_sync();
  gSPScisTextureRectangle(gfx_disp_p++,
                          ulx, uly,
                          lrx, lry,
                          G_TX_RENDERTILE,
                          uls, ult,
                          dsdx, dtdy);
  gfx_synced = 0;
}

int gfx_font_xheight(const struct gfx_font *font)
{
  return font->baseline - font->median;
}

void gfx_printf(const struct gfx_font *font, int x, int y,
                const char *format, ...)
{
  if (gfx_modes[GFX_MODE_TEXT] == GFX_TEXT_NORMAL) {
    va_list args;
    va_start(args, format);
    gfx_printf_n_va(font, x, y, format, args);
    va_end(args);
  }
  else if (gfx_modes[GFX_MODE_TEXT] == GFX_TEXT_FAST) {
    va_list args;
    va_start(args, format);
    gfx_printf_f_va(font, x, y, format, args);
    va_end(args);
  }
}

void gfx_printf_n(const struct gfx_font *font, int x, int y,
                  const char *format, ...)
{
  va_list args;
  va_start(args, format);
  gfx_printf_n_va(font, x, y, format, args);
  va_end(args);
}

void gfx_printf_f(const struct gfx_font *font, int x, int y,
                  const char *format, ...)
{
  va_list args;
  va_start(args, format);
  gfx_printf_f_va(font, x, y, format, args);
  va_end(args);
}

static void draw_chars(const struct gfx_font *font, int x, int y,
                       const char *buf, size_t l)
{
  x -= font->x;
  y -= font->baseline;
  struct gfx_texture *texture = font->texture;
  int chars_per_tile = font->chars_xtile * font->chars_ytile;
  int n_tiles = texture->tiles_x * texture->tiles_y;
  int n_chars = chars_per_tile * n_tiles;
  for (int i = 0; i < n_tiles; ++i) {
    int tile_begin = chars_per_tile * i;
    int tile_end = tile_begin + chars_per_tile;
    _Bool tile_loaded = 0;
    int cx = 0;
    int cy = 0;
    for (int j = 0; j < l; ++j, cx += font->char_width + font->letter_spacing) {
      uint8_t c = buf[j];
      if (c < font->code_start || c >= font->code_start + n_chars)
        continue;
      c -= font->code_start;
      if (c < tile_begin || c >= tile_end)
        continue;
      c -= tile_begin;
      if (!tile_loaded) {
        tile_loaded = 1;
        gfx_rdp_load_tile(texture, i);
      }
      gSPScisTextureRectangle(gfx_disp_p++,
                              qs102(x + cx),
                              qs102(y + cy),
                              qs102(x + cx + font->char_width),
                              qs102(y + cy + font->char_height),
                              G_TX_RENDERTILE,
                              qu105(c % font->chars_xtile *
                                    font->char_width),
                              qu105(c / font->chars_xtile *
                                    font->char_height),
                              qu510(1), qu510(1));
    }
  }
  gfx_synced = 0;
}

static void flush_chars(void)
{
  const struct gfx_font *font = gfx_char_font;
  uint32_t color = 0;
  _Bool first = 1;
  for (int i = 0; i < CHAR_TILE_MAX; ++i) {
    struct vector *tile_vect = &gfx_chars[i];
    for (int j = 0; j < tile_vect->size; ++j) {
      struct gfx_char *gc = vector_at(tile_vect, j);
      if (j == 0)
        gfx_rdp_load_tile(font->texture, i);
      if (first || color != gc->color) {
        color = gc->color;
        gfx_sync();
        gDPSetPrimColor(gfx_disp_p++, 0, 0,
                        (color >> 24) & 0xFF,
                        (color >> 16) & 0xFF,
                        (color >> 8)  & 0xFF,
                        (color >> 0)  & 0xFF);
      }
      first = 0;
      gSPScisTextureRectangle(gfx_disp_p++,
                              qs102(gc->x),
                              qs102(gc->y),
                              qs102(gc->x + font->char_width),
                              qs102(gc->y + font->char_height),
                              G_TX_RENDERTILE,
                              qu105(gc->tile_char % font->chars_xtile *
                                    font->char_width),
                              qu105(gc->tile_char / font->chars_xtile *
                                    font->char_height),
                              qu510(1), qu510(1));
      gfx_synced = 0;
    }
    vector_clear(tile_vect);
  }
}

static void gfx_printf_n_va(const struct gfx_font *font, int x, int y,
                            const char *format, va_list args)
{
  const size_t bufsize = 1024;
  char buf[bufsize];
  int l = vsnprintf(buf, bufsize, format, args);
  if (l > bufsize - 1)
    l = bufsize - 1;
  if (gfx_modes[GFX_MODE_DROPSHADOW]) {
    uint8_t a = gfx_modes[GFX_MODE_COLOR] & 0xFF;
    a = a * a / 0xFF;
    gfx_mode_replace(GFX_MODE_COLOR, GPACK_RGBA8888(0x00, 0x00, 0x00, a));
    draw_chars(font, x + 1, y + 1, buf, l);
    gfx_mode_pop(GFX_MODE_COLOR);
  }
  draw_chars(font, x, y, buf, l);
}

static void gfx_printf_f_va(const struct gfx_font *font, int x, int y,
                            const char *format, va_list args)
{
  const size_t bufsize = 1024;
  char buf[bufsize];
  int l = vsnprintf(buf, bufsize, format, args);
  if (l > bufsize - 1)
    l = bufsize - 1;
  x -= font->x;
  y -= font->baseline;
  struct gfx_texture *texture = font->texture;
  int chars_per_tile = font->chars_xtile * font->chars_ytile;
  int n_tiles = texture->tiles_x * texture->tiles_y;
  int n_chars = chars_per_tile * n_tiles;
  if (gfx_modes[GFX_MODE_DROPSHADOW]) {
    uint8_t a = gfx_modes[GFX_MODE_COLOR] & 0xFF;
    a = a * a / 0xFF;
    uint32_t color = GPACK_RGBA8888(0x00, 0x00, 0x00, a);
    int cx = x + 1;
    int cy = y + 1;
    for (int i = 0; i < l; ++i, cx += font->char_width + font->letter_spacing) {
      uint8_t c = buf[i];
      if (c < font->code_start || c >= font->code_start + n_chars)
        continue;
      c -= font->code_start;
      int tile_idx = c / chars_per_tile;
      int tile_char = c % chars_per_tile;
      struct gfx_char gc =
      {
        tile_char, color, cx, cy,
      };
      vector_push_back(&gfx_chars[tile_idx], 1, &gc);
    }
  }
  int cx = x;
  int cy = y;
  for (int i = 0; i < l; ++i, cx += font->char_width + font->letter_spacing) {
    uint8_t c = buf[i];
    if (c < font->code_start || c >= font->code_start + n_chars)
      continue;
    c -= font->code_start;
    int tile_idx = c / chars_per_tile;
    int tile_char = c % chars_per_tile;
    struct gfx_char gc =
    {
      tile_char, gfx_modes[GFX_MODE_COLOR], cx, cy,
    };
    vector_push_back(&gfx_chars[tile_idx], 1, &gc);
  }

  gfx_char_font = font;
}
