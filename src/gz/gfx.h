#ifndef GFX_H
#define GFX_H
#include <stddef.h>
#include <n64.h>
#include "gu.h"

#define GFX_FILE_DRAM   (-1)
#define gfx_disp(...)   {Gfx gfx_disp__[]={__VA_ARGS__};                      \
                         gfx_disp_append(gfx_disp__,sizeof(gfx_disp__));}
#define GFX_TEXT_NORMAL 0
#define GFX_TEXT_FAST   1

enum gfx_mode
{
  GFX_MODE_FILTER,
  GFX_MODE_COMBINE,
  GFX_MODE_COLOR,
  GFX_MODE_DROPSHADOW,
  GFX_MODE_TEXT,
  GFX_MODE_ALL,
};

struct gfx_texdesc
{
  g_ifmt_t  im_fmt;
  g_isiz_t  im_siz;
  uint32_t  address;
  int16_t   tile_width;
  int16_t   tile_height;
  int16_t   tiles_x;
  int16_t   tiles_y;
  uint32_t  file_vaddr;
  size_t    file_vsize;
};

struct gfx_texldr
{
  uint32_t  file_vaddr;
  void     *file_data;
};

struct gfx_texture
{
  g_ifmt_t  im_fmt;
  g_isiz_t  im_siz;
  void     *data;
  int16_t   tile_width;
  int16_t   tile_height;
  int16_t   tiles_x;
  int16_t   tiles_y;
  size_t    tile_size;
};

struct gfx_sprite
{
  struct gfx_texture *texture;
  int16_t             texture_tile;
  float               x;
  float               y;
  float               xscale;
  float               yscale;
};

struct gfx_font
{
  struct gfx_texture *texture;
  int16_t             char_width;
  int16_t             char_height;
  int16_t             chars_xtile;
  int16_t             chars_ytile;
  uint8_t             code_start;
  int16_t             letter_spacing;
  int16_t             line_spacing;
  int16_t             baseline;
  int16_t             median;
  int16_t             x;
};

void  gfx_start(void);
void  gfx_mode_init(void);
void  gfx_mode_configure(enum gfx_mode mode, uint64_t value);
void  gfx_mode_apply(enum gfx_mode mode);
void  gfx_mode_set(enum gfx_mode mode, uint64_t value);
void  gfx_mode_push(enum gfx_mode mode);
void  gfx_mode_pop(enum gfx_mode mode);
void  gfx_mode_replace(enum gfx_mode mode, uint64_t value);
/* all sizes are specified in number of bytes */
Gfx  *gfx_disp_append(Gfx *disp, size_t size);
void *gfx_data_append(void *data, size_t size);
void  gfx_flush(void);

void                gfx_texldr_init(struct gfx_texldr *texldr);
struct gfx_texture *gfx_texldr_load(struct gfx_texldr *texldr,
                                    const struct gfx_texdesc *texdesc,
                                    struct gfx_texture *texture);
void                gfx_texldr_destroy(struct gfx_texldr *texldr);

struct gfx_texture *gfx_texture_create(g_ifmt_t im_fmt, g_isiz_t im_siz,
                                       int tile_width, int tile_height,
                                       int tiles_x, int tiles_y);
struct gfx_texture *gfx_texture_load(const struct gfx_texdesc *texdesc,
                                     struct gfx_texture *texture);
void                gfx_texture_destroy(struct gfx_texture *texture);
void                gfx_texture_free(struct gfx_texture *texture);
void               *gfx_texture_data(const struct gfx_texture *texture,
                                     int16_t image);
struct gfx_texture *gfx_texture_copy(const struct gfx_texture *src,
                                     struct gfx_texture *dest);
void                gfx_texture_copy_tile(struct gfx_texture *dest,
                                          int dest_tile,
                                          const struct gfx_texture *src,
                                          int src_tile,
                                          _Bool blend);
void                gfx_texture_colortransform(struct gfx_texture *texture,
                                               const MtxF *matrix);

void gfx_disp_rdp_load_tile(Gfx **disp,
                            const struct gfx_texture *texture,
                            int16_t texture_tile);
void gfx_rdp_load_tile(const struct gfx_texture *texture,
                       int16_t texture_tile);

void gfx_sprite_draw(const struct gfx_sprite *sprite);

int  gfx_font_xheight(const struct gfx_font *font);
void gfx_printf(const struct gfx_font *font, int x, int y,
                const char *format, ...);
void gfx_printf_n(const struct gfx_font *font, int x, int y,
                  const char *format, ...);
void gfx_printf_f(const struct gfx_font *font, int x, int y,
                  const char *format, ...);

extern const MtxF gfx_cm_desaturate;

#endif
