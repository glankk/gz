#ifndef GFX_H
#define GFX_H
#include <stddef.h>
#include <n64.h>

#define GFX_FILE_DRAM (-1)
#define gfx_disp(...) {Gfx gfx_disp__[]={__VA_ARGS__};                        \
                       gfx_disp_append(gfx_disp__,sizeof(gfx_disp__));}

struct gfx_texdesc
{
  g_ifmt_t  fmt;
  g_isiz_t  siz;
  uint32_t  address;
  int16_t   width;
  int16_t   height;
  int16_t   images;
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
  g_ifmt_t  fmt;
  g_isiz_t  siz;
  void     *data;
  int16_t   width;
  int16_t   height;
  int16_t   images;
};

struct gfx_colormatrix
{
  float rr, rg, rb, ra;
  float gr, gg, gb, ga;
  float br, bg, bb, ba;
  float ar, ag, ab, aa;
};

struct gfx_sprite
{
  struct gfx_texture *texture;
  int16_t             texture_image;
  float               x;
  float               y;
  float               xscale;
  float               yscale;
};

struct gfx_font
{
  struct gfx_texture *texture;
  uint8_t code_start;
  int16_t spacing;
};

void gfx_mode_init(int filter);
void gfx_mode_default();
void gfx_mode_filter(int filter);
/* all sizes are specified in number of bytes */
Gfx  *gfx_disp_append(Gfx *disp, size_t size);
void *gfx_data_append(void *data, size_t size);
int   gfx_flush();

void                gfx_texldr_init(struct gfx_texldr *texldr);
struct gfx_texture *gfx_texldr_load(struct gfx_texldr *texldr,
                                    const struct gfx_texdesc *texdesc,
                                    struct gfx_texture *texture);
void                gfx_texldr_destroy(struct gfx_texldr *texldr);

struct gfx_texture *gfx_texture_load(const struct gfx_texdesc *texdesc,
                                     struct gfx_texture *texture);
void                gfx_texture_destroy(struct gfx_texture *texture);
void                gfx_texture_free(struct gfx_texture *texture);
size_t              gfx_texture_image_size(const struct gfx_texture *texture);
void               *gfx_texture_data(const struct gfx_texture *texture,
                                     int16_t image);
struct gfx_texture *gfx_texture_copy(const struct gfx_texture *src,
                                     struct gfx_texture *dest);
void                gfx_texture_colortransform(struct gfx_texture *texture,
                                               const struct gfx_colormatrix
                                               *matrix);

void gfx_sprite_draw(const struct gfx_sprite *sprite);

void gfx_printf(const struct gfx_font *font, int x, int y,
                const char *format, ...);

extern const struct gfx_colormatrix gfx_cm_desaturate;

#endif
