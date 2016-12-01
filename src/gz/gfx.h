#ifndef GFX_H
#define GFX_H
#include <stddef.h>
#include <n64.h>

#define GFX_FILE_DRAM (-1)
#define gfx_disp(...) {Gfx gfx_disp__[]={__VA_ARGS__};                        \
                       gfx_disp_append(gfx_disp__,sizeof(gfx_disp__));}

extern Gfx   *gfx_disp;
extern Gfx   *gfx_disp_p;
extern Gfx   *gfx_disp_e;
extern Gfx   *gfx_disp_w;
extern char  *gfx_data;
extern char  *gfx_data_p;
extern char  *gfx_data_e;
extern char  *gfx_data_w;

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
  int16_t             spacing;
};

void gfx_mode_init(int filter, _Bool blend);
void gfx_mode_default();
void gfx_mode_filter(int filter);
void gfx_mode_blend(_Bool blend);
void gfx_mode_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
/* all sizes are specified in number of bytes */
Gfx  *gfx_disp_append(Gfx *disp, size_t size);
void *gfx_data_append(void *data, size_t size);
void  gfx_flush();

void                gfx_texldr_init(struct gfx_texldr *texldr);
struct gfx_texture *gfx_texldr_load(struct gfx_texldr *texldr,
                                    const struct gfx_texdesc *texdesc,
                                    struct gfx_texture *texture);
void                gfx_texldr_destroy(struct gfx_texldr *texldr);

struct gfx_texture *gfx_texture_load(const struct gfx_texdesc *texdesc,
                                     struct gfx_texture *texture);
void                gfx_texture_destroy(struct gfx_texture *texture);
void                gfx_texture_free(struct gfx_texture *texture);
void               *gfx_texture_data(const struct gfx_texture *texture,
                                     int16_t image);
struct gfx_texture *gfx_texture_copy(const struct gfx_texture *src,
                                     struct gfx_texture *dest);
void                gfx_texture_colortransform(struct gfx_texture *texture,
                                               const struct gfx_colormatrix
                                               *matrix);

void gfx_rdp_load_tile(const struct gfx_texture *texture,
                       int16_t texture_tile);

void gfx_sprite_draw(const struct gfx_sprite *sprite);

void gfx_printf(const struct gfx_font *font, int x, int y,
                const char *format, ...);

extern const struct gfx_colormatrix gfx_cm_desaturate;

#endif
