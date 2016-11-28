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

typedef struct
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
} gfx_texdesc_t;

typedef struct
{
  uint32_t  file_vaddr;
  void     *file_data;
} gfx_texldr_t;

typedef struct
{
  g_ifmt_t  im_fmt;
  g_isiz_t  im_siz;
  void     *data;
  int16_t   tile_width;
  int16_t   tile_height;
  int16_t   tiles_x;
  int16_t   tiles_y;
  size_t    tile_size;
} gfx_texture_t;

typedef struct
{
  float rr, rg, rb, ra;
  float gr, gg, gb, ga;
  float br, bg, bb, ba;
  float ar, ag, ab, aa;
} gfx_colormatrix_t;

typedef struct
{
  gfx_texture_t  *texture;
  int16_t         texture_tile;
  float           x;
  float           y;
  float           xscale;
  float           yscale;
} gfx_sprite_t;

typedef struct
{
  gfx_texture_t  *texture;
  int16_t         char_width;
  int16_t         char_height;
  int16_t         chars_xtile;
  int16_t         chars_ytile;
  uint8_t         code_start;
  int16_t         spacing;
} gfx_font_t;

void gfx_mode_init(int filter, _Bool blend);
void gfx_mode_default();
void gfx_mode_filter(int filter);
void gfx_mode_blend(_Bool blend);
void gfx_mode_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
/* all sizes are specified in number of bytes */
Gfx  *gfx_disp_append(Gfx *disp, size_t size);
void *gfx_data_append(void *data, size_t size);
void  gfx_flush();

void            gfx_texldr_init(gfx_texldr_t *texldr);
gfx_texture_t  *gfx_texldr_load(gfx_texldr_t *texldr,
                                const gfx_texdesc_t *texdesc,
                                gfx_texture_t *texture);
void            gfx_texldr_destroy(gfx_texldr_t *texldr);

gfx_texture_t  *gfx_texture_load(const gfx_texdesc_t *texdesc,
                                 gfx_texture_t *texture);
void            gfx_texture_destroy(gfx_texture_t *texture);
void            gfx_texture_free(gfx_texture_t *texture);
void           *gfx_texture_data(const gfx_texture_t *texture,
                                 int16_t image);
gfx_texture_t  *gfx_texture_copy(const gfx_texture_t *src,
                                 gfx_texture_t *dest);
void            gfx_texture_colortransform(gfx_texture_t *texture,
                                           const gfx_colormatrix_t *matrix);

void gfx_rdp_load_tile(const gfx_texture_t *texture, int16_t texture_tile);

void gfx_sprite_draw(const gfx_sprite_t *sprite);

void gfx_printf(const gfx_font_t *font, int x, int y, const char *format, ...);

extern const gfx_colormatrix_t gfx_cm_desaturate;

#endif
