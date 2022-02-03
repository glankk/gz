#include <stdlib.h>
#include <math.h>
#include <n64.h>
#include <vector/vector.h>
#include <stdint.h>
#include "geometry.h"
#include "gfx.h"
#include "gu.h"
#include "gz.h"
#include "settings.h"
#include "ucode.h"
#include "util.h"
#include "z64.h"
#include "zu.h"

/* blend primitive and shading color, output environment alpha */
#define G_CC_MODULATERGB_PRIM_ENVA  PRIMITIVE, 0, SHADE, 0, \
                                    0,         0, 0,     ENVIRONMENT
/* output primitive color, output environment alpha */
#define G_CC_PRIMITIVE_ENVA         0,         0, 0,     PRIMITIVE, \
                                    0,         0, 0,     ENVIRONMENT

struct line
{
  int va;
  int vb;
};

static void vtxn_f2l(Vtx *r, z64_xyzf_t *v)
{
  *r = gdSPDefVtxN(floorf(0.5f + v->x * 128.f),
                   floorf(0.5f + v->y * 128.f),
                   floorf(0.5f + v->z * 128.f),
                   0, 0,
                   v->x * 127.f, v->y * 127.f, v->z * 127.f,
                   0xFF);
}

static void draw_line(Gfx **p_gfx_p, Gfx **p_gfx_d,
                     z64_xyz_t *v1, z64_xyz_t *v2)
{
  Vtx v[2] =
  {
    gdSPDefVtxC(v1->x, v1->y, v1->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
    gdSPDefVtxC(v2->x, v2->y, v2->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
  };

  gSPVertex((*p_gfx_p)++, gDisplayListData(p_gfx_d, v), 2, 0);
  gSPLine3D((*p_gfx_p)++, 0, 1, 0);
}

static void tri_norm(z64_xyzf_t *v1, z64_xyzf_t *v2, z64_xyzf_t *v3,
                     z64_xyzf_t *norm)
{
  norm->x = (v2->y - v1->y) * (v3->z - v1->z) -
            (v2->z - v1->z) * (v3->y - v1->y);
  norm->y = (v2->z - v1->z) * (v3->x - v1->x) -
            (v2->x - v1->x) * (v3->z - v1->z);
  norm->z = (v2->x - v1->x) * (v3->y - v1->y) -
            (v2->y - v1->y) * (v3->x - v1->x);
  float norm_d = sqrtf(norm->x * norm->x +
                       norm->y * norm->y +
                       norm->z * norm->z);
  if (norm_d != 0.f) {
    norm->x *= 127.f / norm_d;
    norm->y *= 127.f / norm_d;
    norm->z *= 127.f / norm_d;
  }
}

static void draw_tri(Gfx **p_gfx_p, Gfx **p_gfx_d,
                     z64_xyzf_t *v1, z64_xyzf_t *v2, z64_xyzf_t *v3)
{
  z64_xyzf_t norm;
  tri_norm(v1, v2, v3, &norm);

  Vtx v[3] =
  {
    gdSPDefVtxN(v1->x, v1->y, v1->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
    gdSPDefVtxN(v2->x, v2->y, v2->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
    gdSPDefVtxN(v3->x, v3->y, v3->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
  };

  gSPVertex((*p_gfx_p)++, gDisplayListData(p_gfx_d, v), 3, 0);
  gSP1Triangle((*p_gfx_p)++, 0, 1, 2, 0);
}

static void draw_quad(Gfx **p_gfx_p, Gfx **p_gfx_d,
                      z64_xyzf_t *v1, z64_xyzf_t *v2,
                      z64_xyzf_t *v3, z64_xyzf_t *v4)
{
  z64_xyzf_t norm;
  tri_norm(v1, v2, v4, &norm);

  Vtx v[4] =
  {
    gdSPDefVtxN(v1->x, v1->y, v1->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
    gdSPDefVtxN(v2->x, v2->y, v2->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
    gdSPDefVtxN(v3->x, v3->y, v3->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
    gdSPDefVtxN(v4->x, v4->y, v4->z, 0, 0, norm.x, norm.y, norm.z, 0xFF),
  };

  gSPVertex((*p_gfx_p)++, gDisplayListData(p_gfx_d, v), 4, 0);
  gSP2Triangles((*p_gfx_p)++, 0, 1, 2, 0, 0, 2, 3, 0);
}

static void draw_cuboid(Gfx **p_gfx_p, Gfx **p_gfx_d,
                        z64_xyzf_t *v_min, z64_xyzf_t *v_max)
{
  z64_xyzf_t points[8] =
  {
    { v_min->x, v_min->y, v_min->z },
    { v_max->x, v_min->y, v_min->z },
    { v_min->x, v_max->y, v_min->z },
    { v_max->x, v_max->y, v_min->z },
    { v_min->x, v_min->y, v_max->z },
    { v_max->x, v_min->y, v_max->z },
    { v_min->x, v_max->y, v_max->z },
    { v_max->x, v_max->y, v_max->z },
  };

  draw_quad(p_gfx_p, p_gfx_d, &points[6], &points[7], &points[3], &points[2]);
  draw_quad(p_gfx_p, p_gfx_d, &points[6], &points[2], &points[0], &points[4]);
  draw_quad(p_gfx_p, p_gfx_d, &points[7], &points[6], &points[4], &points[5]);
  draw_quad(p_gfx_p, p_gfx_d, &points[3], &points[7], &points[5], &points[1]);
  draw_quad(p_gfx_p, p_gfx_d, &points[2], &points[3], &points[1], &points[0]);
  draw_quad(p_gfx_p, p_gfx_d, &points[0], &points[1], &points[5], &points[4]);
}

static void draw_cyl(Gfx **p_gfx_p, Gfx **p_gfx_d,
                     float x, float y, float z, int radius, int height)
{
  static Gfx *p_cyl_gfx = NULL;

  if (!p_cyl_gfx) {
#define CYL_DIVS 12
    static Gfx cyl_gfx[5 + CYL_DIVS * 2];
    static Vtx cyl_vtx[2 + CYL_DIVS * 2];

    p_cyl_gfx = cyl_gfx;
    Gfx *cyl_gfx_p = p_cyl_gfx;

    cyl_vtx[0] = gdSPDefVtxN(0, 0,   0, 0, 0, 0, -127, 0, 0xFF);
    cyl_vtx[1] = gdSPDefVtxN(0, 128, 0, 0, 0, 0, 127,  0, 0xFF);
    for (int i = 0; i < CYL_DIVS; ++i) {
      int vtx_x = floorf(0.5f + cosf(2.f * M_PI * i / CYL_DIVS) * 128.f);
      int vtx_z = floorf(0.5f - sinf(2.f * M_PI * i / CYL_DIVS) * 128.f);
      int norm_x = cosf(2.f * M_PI * i / CYL_DIVS) * 127.f;
      int norm_z = -sinf(2.f * M_PI * i / CYL_DIVS) * 127.f;
      cyl_vtx[2 + i * 2 + 0] = gdSPDefVtxN(vtx_x, 0,   vtx_z, 0, 0,
                                           norm_x, 0, norm_z, 0xFF);
      cyl_vtx[2 + i * 2 + 1] = gdSPDefVtxN(vtx_x, 128, vtx_z, 0, 0,
                                           norm_x, 0, norm_z, 0xFF);
    }

    gSPSetGeometryMode(cyl_gfx_p++, G_CULL_BACK | G_SHADING_SMOOTH);

    gSPVertex(cyl_gfx_p++, cyl_vtx, 2 + CYL_DIVS * 2, 0);
    for (int i = 0; i < CYL_DIVS; ++i) {
      int p = (i + CYL_DIVS - 1) % CYL_DIVS;
      int v[4] =
      {
        2 + p * 2 + 0,
        2 + i * 2 + 0,
        2 + i * 2 + 1,
        2 + p * 2 + 1,
      };
      gSP2Triangles(cyl_gfx_p++, v[0], v[1], v[2], 0, v[0], v[2], v[3], 0);
    }

    gSPClearGeometryMode(cyl_gfx_p++, G_SHADING_SMOOTH);
    for (int i = 0; i < CYL_DIVS; ++i) {
      int p = (i + CYL_DIVS - 1) % CYL_DIVS;
      int v[4] =
      {
        2 + p * 2 + 0,
        2 + i * 2 + 0,
        2 + i * 2 + 1,
        2 + p * 2 + 1,
      };
      gSP2Triangles(cyl_gfx_p++, 0, v[1], v[0], 0, 1, v[3], v[2], 0);
    }

    gSPClearGeometryMode(cyl_gfx_p++, G_CULL_BACK);
    gSPEndDisplayList(cyl_gfx_p++);
#undef CYL_DIVS
  }

  Mtx m;
  {
    MtxF mf;
    guTranslateF(&mf, x, y, z);
    MtxF ms;
    guScaleF(&ms, radius / 128.f, height / 128.f, radius / 128.f);
    guMtxCatF(&ms, &mf, &mf);
    guMtxF2L(&mf, &m);
  }

  gSPMatrix((*p_gfx_p)++, gDisplayListData(p_gfx_d, m),
            G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
  gSPDisplayList((*p_gfx_p)++, p_cyl_gfx);
  gSPPopMatrix((*p_gfx_p)++, G_MTX_MODELVIEW);
}

static void ico_sph_subdivide_edge(z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b)
{
  vec3f_add(r, a, b);
  vec3f_norm(r, r);
}

static void draw_ico_sphere(Gfx **p_gfx_p, Gfx **p_gfx_d,
                            float x, float y, float z, float radius)
{
  static Gfx *p_sph_gfx = NULL;

  if (!p_sph_gfx) {
    z64_xyzf_t vtx[42];
    int r0_n = 1,   r0_m = r0_n / 5,  r0_i = 0    + 0;
    int r1_n = 5,   r1_m = r1_n / 5,  r1_i = r0_i + r0_n;
    int r2_n = 10,  r2_m = r2_n / 5,  r2_i = r1_i + r1_n;
    int r3_n = 10,  r3_m = r3_n / 5,  r3_i = r2_i + r2_n;
    int r4_n = 10,  r4_m = r4_n / 5,  r4_i = r3_i + r3_n;
    int r5_n = 5,   r5_m = r5_n / 5,  r5_i = r4_i + r4_n;
    int r6_n = 1,   r6_m = r6_n / 5,  r6_i = r5_i + r5_n;

    vtx[r0_i + (0 * r0_m + 0) % r0_n] = (z64_xyzf_t){0.f, 1.f, 0.f};
    vtx[r6_i + (0 * r6_m + 0) % r6_n] = (z64_xyzf_t){0.f, -1.f, 0.f};
    for (int i = 0; i < 5; ++i) {
      float a_xz = 2.f * M_PI / 10.f;
      float a_y = atanf(1.f / 2.f);
      vtx[r2_i + (i * r2_m + 0) % r2_n] = (z64_xyzf_t)
      {
        cos(a_xz * (i * r2_m + 0)) * cos(a_y * 1.f),
        sin(a_y * 1.f),
        -sin(a_xz * (i * r2_m + 0)) * cos(a_y * 1.f),
      };
      vtx[r4_i + (i * r4_m + 0) % r4_n] = (z64_xyzf_t)
      {
        cos(a_xz * (i * r4_m + 1)) * cos(a_y * -1.f),
        sin(a_y * -1.f),
        -sin(a_xz * (i * r4_m + 1)) * cos(a_y * -1.f),
      };
    }
    for (int i = 0; i < 5; ++i) {
      ico_sph_subdivide_edge(&vtx[r1_i + (i * r1_m + 0) % r1_n],
                             &vtx[r0_i + (i * r0_m + 0) % r0_n],
                             &vtx[r2_i + (i * r2_m + 0) % r2_n]);
      ico_sph_subdivide_edge(&vtx[r2_i + (i * r2_m + 1) % r2_n],
                             &vtx[r2_i + (i * r2_m + 0) % r2_n],
                             &vtx[r2_i + (i * r2_m + 2) % r2_n]);
      ico_sph_subdivide_edge(&vtx[r3_i + (i * r3_m + 0) % r3_n],
                             &vtx[r2_i + (i * r2_m + 0) % r2_n],
                             &vtx[r4_i + (i * r4_m + 0) % r4_n]);
      ico_sph_subdivide_edge(&vtx[r3_i + (i * r3_m + 1) % r3_n],
                             &vtx[r4_i + (i * r4_m + 0) % r4_n],
                             &vtx[r2_i + (i * r2_m + 2) % r2_n]);
      ico_sph_subdivide_edge(&vtx[r4_i + (i * r4_m + 1) % r4_n],
                             &vtx[r4_i + (i * r4_m + 0) % r4_n],
                             &vtx[r4_i + (i * r4_m + 2) % r4_n]);
      ico_sph_subdivide_edge(&vtx[r5_i + (i * r5_m + 0) % r5_n],
                             &vtx[r4_i + (i * r4_m + 0) % r4_n],
                             &vtx[r6_i + (i * r6_m + 0) % r6_n]);
    }

    static Vtx sph_vtx[42];
    static Gfx sph_gfx[45];

    for (int i = 0; i < 42; ++i)
      vtxn_f2l(&sph_vtx[i], &vtx[i]);

    p_sph_gfx = sph_gfx;
    Gfx *sph_gfx_p = p_sph_gfx;

    gSPSetGeometryMode(sph_gfx_p++, G_CULL_BACK | G_SHADING_SMOOTH);

    gSPVertex(sph_gfx_p++, &sph_vtx[r0_i], r0_n + r1_n + r2_n + r3_n,
              r0_i - r0_i);
    r3_i -= r0_i;
    r2_i -= r0_i;
    r1_i -= r0_i;
    r0_i -= r0_i;
    for (int i = 0; i < 5; ++i) {
      int v[24] =
      {
        r0_i + (i * r0_m + 0) % r0_n,
        r1_i + (i * r1_m + 0) % r1_n,
        r1_i + (i * r1_m + 1) % r1_n,

        r1_i + (i * r1_m + 0) % r1_n,
        r2_i + (i * r2_m + 0) % r2_n,
        r2_i + (i * r2_m + 1) % r2_n,

        r1_i + (i * r1_m + 0) % r1_n,
        r2_i + (i * r2_m + 1) % r2_n,
        r1_i + (i * r1_m + 1) % r1_n,

        r1_i + (i * r1_m + 1) % r1_n,
        r2_i + (i * r2_m + 1) % r2_n,
        r2_i + (i * r2_m + 2) % r2_n,

        r2_i + (i * r2_m + 0) % r2_n,
        r3_i + (i * r3_m + 0) % r3_n,
        r2_i + (i * r2_m + 1) % r2_n,

        r2_i + (i * r2_m + 1) % r2_n,
        r3_i + (i * r3_m + 0) % r3_n,
        r3_i + (i * r3_m + 1) % r3_n,

        r2_i + (i * r2_m + 1) % r2_n,
        r3_i + (i * r3_m + 1) % r3_n,
        r2_i + (i * r2_m + 2) % r2_n,

        r2_i + (i * r2_m + 2) % r2_n,
        r3_i + (i * r3_m + 1) % r3_n,
        r3_i + (i * r3_m + 2) % r3_n,
      };
      gSP2Triangles(sph_gfx_p++,
                    v[0],   v[1],   v[2],   0,
                    v[3],   v[4],   v[5],   0);
      gSP2Triangles(sph_gfx_p++,
                    v[6],   v[7],   v[8],   0,
                    v[9],   v[10],  v[11],  0);
      gSP2Triangles(sph_gfx_p++,
                    v[12],  v[13],  v[14],  0,
                    v[15],  v[16],  v[17],  0);
      gSP2Triangles(sph_gfx_p++,
                    v[18],  v[19],  v[20],  0,
                    v[21],  v[22],  v[23],  0);
    }

    gSPVertex(sph_gfx_p++, &sph_vtx[r4_i], r4_n + r5_n + r6_n,
              r4_i - r4_i);
    r6_i -= r4_i;
    r5_i -= r4_i;
    r4_i -= r4_i;
    for (int i = 0; i < 5; ++i) {
      int v[24] =
      {
        r3_i + (i * r3_m + 1) % r3_n,
        r4_i + (i * r4_m + 0) % r4_n,
        r4_i + (i * r4_m + 1) % r4_n,

        r3_i + (i * r3_m + 1) % r3_n,
        r4_i + (i * r4_m + 1) % r4_n,
        r3_i + (i * r3_m + 2) % r3_n,

        r3_i + (i * r3_m + 2) % r3_n,
        r4_i + (i * r4_m + 1) % r4_n,
        r4_i + (i * r4_m + 2) % r4_n,

        r3_i + (i * r3_m + 2) % r3_n,
        r4_i + (i * r4_m + 2) % r4_n,
        r3_i + (i * r3_m + 3) % r3_n,

        r4_i + (i * r4_m + 0) % r4_n,
        r5_i + (i * r5_m + 0) % r5_n,
        r4_i + (i * r4_m + 1) % r4_n,

        r4_i + (i * r4_m + 1) % r4_n,
        r5_i + (i * r5_m + 0) % r5_n,
        r5_i + (i * r5_m + 1) % r5_n,

        r4_i + (i * r4_m + 1) % r4_n,
        r5_i + (i * r5_m + 1) % r5_n,
        r4_i + (i * r4_m + 2) % r4_n,

        r5_i + (i * r5_m + 0) % r5_n,
        r6_i + (i * r6_m + 0) % r6_n,
        r5_i + (i * r5_m + 1) % r5_n,
      };
      gSP2Triangles(sph_gfx_p++,
                    v[0],   v[1],   v[2],   0,
                    v[3],   v[4],   v[5],   0);
      gSP2Triangles(sph_gfx_p++,
                    v[6],   v[7],   v[8],   0,
                    v[9],   v[10],  v[11],  0);
      gSP2Triangles(sph_gfx_p++,
                    v[12],  v[13],  v[14],  0,
                    v[15],  v[16],  v[17],  0);
      gSP2Triangles(sph_gfx_p++,
                    v[18],  v[19],  v[20],  0,
                    v[21],  v[22],  v[23],  0);
    }

    gSPClearGeometryMode(sph_gfx_p++, G_CULL_BACK | G_SHADING_SMOOTH);
    gSPEndDisplayList(sph_gfx_p++);
  }

  Mtx m;
  {
    MtxF mf;
    guTranslateF(&mf, x, y, z);
    MtxF ms;
    guScaleF(&ms, radius / 128.f, radius / 128.f, radius / 128.f);
    guMtxCatF(&ms, &mf, &mf);
    guMtxF2L(&mf, &m);
  }

  gSPMatrix((*p_gfx_p)++, gDisplayListData(p_gfx_d, m),
            G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
  gSPDisplayList((*p_gfx_p)++, p_sph_gfx);
  gSPPopMatrix((*p_gfx_p)++, G_MTX_MODELVIEW);
}

typedef struct
{
  Gfx      *gfx_p;
  Gfx      *gfx_d;
  Vtx       vtx_buf[30];
  Gfx      *vtx_cmd;
  Gfx      *tri_cmd;
  int       n_vtx;
  uint32_t  last_color;
} poly_writer_t;

static void poly_writer_init(poly_writer_t *writer, Gfx *gfx_p, Gfx *gfx_d)
{
  writer->gfx_p = gfx_p;
  writer->gfx_d = gfx_d;
  writer->vtx_cmd = NULL;
  writer->tri_cmd = NULL;
  writer->n_vtx = 0;
  writer->last_color = 0x00000000;

  gDPSetPrimColor(writer->gfx_p++, 0, 0, 0x00, 0x00, 0x00, 0x00);
}

static void poly_writer_flush(poly_writer_t *writer)
{
  if (writer->vtx_cmd) {
    size_t vtx_size = sizeof(Vtx) * writer->n_vtx;

    Vtx *p_vtx = gDisplayListAlloc(&writer->gfx_d, vtx_size);
    memcpy(p_vtx, writer->vtx_buf, vtx_size);
    *writer->vtx_cmd = gsSPVertex(p_vtx, writer->n_vtx, 0);
    writer->vtx_cmd = NULL;
  }

  if (writer->n_vtx % 6 != 0)
    *writer->tri_cmd = gsSP1Triangle(writer->n_vtx - 3, writer->n_vtx - 2,
                                     writer->n_vtx - 1, 0);
  writer->tri_cmd = NULL;

  writer->n_vtx = 0;
}

static void poly_writer_finish(poly_writer_t *writer,
                               Gfx **p_gfx_p, Gfx **p_gfx_d)
{
  poly_writer_flush(writer);

  *p_gfx_p = writer->gfx_p;
  *p_gfx_d = writer->gfx_d;
}

static void poly_writer_add(poly_writer_t *writer, Vtx (*v)[3], uint32_t color)
{
  if (!writer->vtx_cmd)
    writer->vtx_cmd = writer->gfx_p++;

  memcpy(&writer->vtx_buf[writer->n_vtx], v, sizeof(*v));

  if (writer->last_color != color) {
    gDPSetPrimColor(writer->gfx_p++, 0, 0,
                    (color >> 24) & 0xFF, (color >> 16) & 0xFF,
                    (color >> 8)  & 0xFF, (color >> 0)  & 0xFF);
    if (writer->n_vtx % 6 != 0) {
      *writer->tri_cmd = gsSP1Triangle(writer->n_vtx - 3, writer->n_vtx - 2,
                                       writer->n_vtx - 1, 0);
      writer->tri_cmd = writer->gfx_p++;
      *writer->tri_cmd = gsSP1Triangle(writer->n_vtx + 0, writer->n_vtx + 1,
                                       writer->n_vtx + 2, 0);
    }
    writer->last_color = color;
  }

  if (writer->n_vtx % 6 == 0) {
    writer->tri_cmd = writer->gfx_p++;
    *writer->tri_cmd = gsSP2Triangles(writer->n_vtx + 0, writer->n_vtx + 1,
                                      writer->n_vtx + 2, 0,
                                      writer->n_vtx + 3, writer->n_vtx + 4,
                                      writer->n_vtx + 5, 0);
  }

  writer->n_vtx += 3;
  if (writer->n_vtx == 30)
    poly_writer_flush(writer);
}

typedef struct
{
  Gfx      *gfx_p;
  Gfx      *gfx_d;
  Vtx       vtx_buf[32];
  Gfx      *vtx_cmd;
  int       n_vtx;
} line_writer_t;

static void line_writer_init(line_writer_t *writer, Gfx *gfx_p, Gfx *gfx_d)
{
  writer->gfx_p = gfx_p;
  writer->gfx_d = gfx_d;
  writer->vtx_cmd = NULL;
  writer->n_vtx = 0;
}

static void line_writer_flush(line_writer_t *writer)
{
  if (writer->vtx_cmd) {
    size_t vtx_size = sizeof(Vtx) * writer->n_vtx;

    Vtx *p_vtx = gDisplayListAlloc(&writer->gfx_d, vtx_size);
    memcpy(p_vtx, writer->vtx_buf, vtx_size);
    *writer->vtx_cmd = gsSPVertex(p_vtx, writer->n_vtx, 0);
    writer->vtx_cmd = NULL;
  }

  writer->n_vtx = 0;
}

static void line_writer_finish(line_writer_t *writer,
                               Gfx **p_gfx_p, Gfx **p_gfx_d)
{
  line_writer_flush(writer);

  *p_gfx_p = writer->gfx_p;
  *p_gfx_d = writer->gfx_d;
}

static void line_writer_add(line_writer_t *writer, Vtx *v, int n_vtx)
{
  if (n_vtx <= 1)
    return;

  if (writer->n_vtx + n_vtx > 32)
    line_writer_flush(writer);

  if (!writer->vtx_cmd)
    writer->vtx_cmd = writer->gfx_p++;

  memcpy(&writer->vtx_buf[writer->n_vtx], v, sizeof(*v) * n_vtx);

  for (int i = 0; i < n_vtx; ++i)
    for (int j = i + 1; j < n_vtx; ++j)
      gSPLine3D(writer->gfx_p++, writer->n_vtx + i, writer->n_vtx + j, 0);

  writer->n_vtx += n_vtx;
}

static void do_poly_list(poly_writer_t *poly_writer,
                         line_writer_t *line_writer, struct vector *line_set,
                         z64_xyz_t *vtx_list, z64_col_poly_t *poly_list,
                         z64_col_type_t *type_list, int n_poly, _Bool rd,
                         _Bool wfc)
{
  for (int i = 0; i < n_poly; ++i) {
    z64_col_poly_t *poly = &poly_list[i];
    z64_col_type_t *type = &type_list[poly->type];
    z64_xyz_t *va = &vtx_list[poly->va];
    z64_xyz_t *vb = &vtx_list[poly->vb];
    z64_xyz_t *vc = &vtx_list[poly->vc];

    /* generate polygon */
    if (poly_writer) {
      uint32_t color;
      _Bool skip = 0;
      if (wfc) {
#define COLPOLY_SNORMAL(x) ((int16_t)((x) * 32767.0f))
        int16_t normal_y = poly->norm.y;
        if (normal_y < COLPOLY_SNORMAL(-0.8f))
          color = 0xFF0000FF;
        else if (normal_y > COLPOLY_SNORMAL(0.5f))
          color = 0x0000FFFF;
        else
          color = 0x00FF00FF;
#undef COLPOLY_SNORMAL
      } else {
        if (type->flags_2.hookshot)
          color = 0x8080FFFF;
        else if (type->flags_1.interaction > 0x01)
          color = 0xC000C0FF;
        else if (type->flags_1.special == 0x0C)
          color = 0xFF0000FF;
        else if (type->flags_1.exit != 0x00 || type->flags_1.special == 0x05)
          color = 0x00FF00FF;
        else if (type->flags_1.behavior != 0 || type->flags_2.wall_damage)
          color = 0xC0FFC0FF;
        else if (type->flags_2.terrain == 0x01)
          color = 0xFFFF80FF;
        else if (rd)
          skip = 1;
        else
          color = 0xFFFFFFFF;
      }
      if (!skip) {
        Vtx v[3] =
        {
          gdSPDefVtxN(va->x, va->y, va->z, 0, 0,
                      poly->norm.x / 0x100, poly->norm.y / 0x100,
                      poly->norm.z / 0x100, 0xFF),
          gdSPDefVtxN(vb->x, vb->y, vb->z, 0, 0,
                      poly->norm.x / 0x100, poly->norm.y / 0x100,
                      poly->norm.z / 0x100, 0xFF),
          gdSPDefVtxN(vc->x, vc->y, vc->z, 0, 0,
                      poly->norm.x / 0x100, poly->norm.y / 0x100,
                      poly->norm.z / 0x100, 0xFF),
        };
        poly_writer_add(poly_writer, &v, color);
      }
    }

    /* generate lines */
    if (line_writer) {
      struct line lab = {poly->va, poly->vb};
      struct line lbc = {poly->vb, poly->vc};
      struct line lca = {poly->vc, poly->va};
      _Bool ab = 1;
      _Bool bc = 1;
      _Bool ca = 1;
      if (line_set) {
        for (int i = 0; i < line_set->size; ++i) {
          struct line *l = vector_at(line_set, i);
          if ((l->va == lab.va && l->vb == lab.vb) ||
              (l->va == lab.vb && l->vb == lab.va))
            ab = 0;
          if ((l->va == lbc.va && l->vb == lbc.vb) ||
              (l->va == lbc.vb && l->vb == lbc.va))
            bc = 0;
          if ((l->va == lca.va && l->vb == lca.vb) ||
              (l->va == lca.vb && l->vb == lca.va))
            ca = 0;
        }
        if (ab)
          vector_push_back(line_set, 1, &lab);
        if (bc)
          vector_push_back(line_set, 1, &lbc);
        if (ca)
          vector_push_back(line_set, 1, &lca);
      }
      Vtx v[3];
      int n_vtx = 0;
      if (ab || ca)
        v[n_vtx++] = gdSPDefVtxC(va->x, va->y, va->z, 0, 0,
                                 0x00, 0x00, 0x00, 0xFF);
      if (ab || bc)
        v[n_vtx++] = gdSPDefVtxC(vb->x, vb->y, vb->z, 0, 0,
                                 0x00, 0x00, 0x00, 0xFF);
      if (bc || ca)
        v[n_vtx++] = gdSPDefVtxC(vc->x, vc->y, vc->z, 0, 0,
                                 0x00, 0x00, 0x00, 0xFF);
      line_writer_add(line_writer, v, n_vtx);
    }
  }
}

static void do_dyn_list(poly_writer_t *poly_writer,
                        line_writer_t *line_writer,
                        z64_col_hdr_t *col_hdr,
                        uint16_t list_idx, _Bool rd, _Bool wfc)
{
  z64_col_ctxt_t *col_ctxt = &z64_game.col_ctxt;

  while (list_idx != 0xFFFF) {
    z64_col_list_t *list = &col_ctxt->dyn_list[list_idx];

    do_poly_list(poly_writer, line_writer, NULL,
                 col_ctxt->dyn_vtx, &col_ctxt->dyn_poly[list->poly_idx],
                 col_hdr->type, 1, rd, wfc);

    list_idx = list->list_next;
  }
}

static void do_dyn_col(poly_writer_t *poly_writer,
                       line_writer_t *line_writer,
                       _Bool rd, _Bool wfc)
{
  z64_col_ctxt_t *col_ctxt = &z64_game.col_ctxt;

  for (int i = 0; i < 50; ++i)
    if (col_ctxt->dyn_flags[i].active) {
      z64_dyn_col_t *dyn_col = &col_ctxt->dyn_col[i];

      do_dyn_list(poly_writer, line_writer,
                  dyn_col->col_hdr, dyn_col->ceil_list_idx, rd, wfc);
      do_dyn_list(poly_writer, line_writer,
                  dyn_col->col_hdr, dyn_col->wall_list_idx, rd, wfc);
      do_dyn_list(poly_writer, line_writer,
                  dyn_col->col_hdr, dyn_col->floor_list_idx, rd, wfc);
    }
}

static void do_waterbox_list(Gfx **p_gfx_p, Gfx **p_gfx_d,
                            int n_waterboxes, z64_col_water_t *waterbox_list)
{
  const float water_max_depth = -4000.0f;

  if (waterbox_list == NULL)
    return;

  for (int i = 0; i < n_waterboxes; ++i) {
    z64_col_water_t *w = &waterbox_list[i];
    if (w->flags.group == z64_game.room_ctxt.rooms[0].index ||
        w->flags.group == z64_game.room_ctxt.rooms[1].index ||
        w->flags.group == 0x3F)
    {
      z64_xyzf_t min_pos = (z64_xyzf_t){ w->pos.x,            water_max_depth, w->pos.z            };
      z64_xyzf_t max_pos = (z64_xyzf_t){ w->pos.x + w->width, w->pos.y,        w->pos.z + w->depth };

      draw_cuboid(p_gfx_p, p_gfx_d, &min_pos, &max_pos);
    }
  }
}

static void init_poly_gfx(Gfx **p_gfx_p, Gfx **p_gfx_d,
                          int mode, _Bool xlu, _Bool shade)
{
  uint32_t rm;
  uint32_t blc1;
  uint32_t blc2;
  uint8_t alpha;
  uint64_t cm;
  uint32_t gm;

  if (xlu) {
    rm = Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL;
    blc1 = GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA);
    blc2 = GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA);
    alpha = 0x80;
  }
  else {
    rm = Z_CMP | Z_UPD | CVG_DST_CLAMP | FORCE_BL;
    blc1 = GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
    blc2 = GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
    alpha = 0xFF;
  }
  if (mode == SETTINGS_COLVIEW_DECAL)
    rm |= ZMODE_DEC;
  else if (xlu)
    rm |= ZMODE_XLU;
  else
    rm |= ZMODE_OPA;
  if (shade) {
    cm = G_CC_MODE(G_CC_MODULATERGB_PRIM_ENVA, G_CC_MODULATERGB_PRIM_ENVA);
    gm = G_ZBUFFER | G_SHADE | G_LIGHTING;
  }
  else {
    cm = G_CC_MODE(G_CC_PRIMITIVE_ENVA, G_CC_PRIMITIVE_ENVA);
    gm = G_ZBUFFER;
  }

  Mtx *p_m = gDisplayListAlloc(p_gfx_d, sizeof(*p_m));
  guMtxIdent(p_m);

  gSPLoadGeometryMode((*p_gfx_p)++, gm);
  gSPTexture((*p_gfx_p)++, qu016(0.5), qu016(0.5), 0, G_TX_RENDERTILE, G_OFF);
  gSPMatrix((*p_gfx_p)++, p_m, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

  gDPPipeSync((*p_gfx_p)++);
  gDPSetCycleType((*p_gfx_p)++, G_CYC_1CYCLE);
  gDPSetRenderMode((*p_gfx_p)++, rm | blc1, rm | blc2);
  gDPSetCombine((*p_gfx_p)++, cm);
  gDPSetEnvColor((*p_gfx_p)++, 0xFF, 0xFF, 0xFF, alpha);
}

static void init_line_gfx(Gfx **p_gfx_p, Gfx **p_gfx_d, int mode, _Bool xlu)
{
  uint32_t rm_c1;
  uint32_t rm_c2;
  uint8_t alpha;

  if (xlu)
    alpha = 0x80;
  else
    alpha = 0xFF;
  if (mode == SETTINGS_COLVIEW_DECAL) {
    rm_c1 = G_RM_AA_ZB_DEC_LINE;
    rm_c2 = G_RM_AA_ZB_DEC_LINE2;
  }
  else {
    rm_c1 = G_RM_AA_ZB_XLU_LINE;
    rm_c2 = G_RM_AA_ZB_XLU_LINE2;
  }

  Mtx *p_m = gDisplayListAlloc(p_gfx_d, sizeof(*p_m));
  guMtxIdent(p_m);

  gSPLoadGeometryMode((*p_gfx_p)++, G_ZBUFFER);
  gSPTexture((*p_gfx_p)++, qu016(0.5), qu016(0.5), 0, G_TX_RENDERTILE, G_OFF);
  gSPMatrix((*p_gfx_p)++, p_m, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);

  gDPPipeSync((*p_gfx_p)++);
  gDPSetCycleType((*p_gfx_p)++, G_CYC_1CYCLE);
  gDPSetRenderMode((*p_gfx_p)++, rm_c1, rm_c2);
  gDPSetCombineMode((*p_gfx_p)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
  gDPSetPrimColor((*p_gfx_p)++, 0, 0, 0x00, 0x00, 0x00, alpha);
}

static void release_mem(void *p_ptr)
{
  void **p_void = (void **)p_ptr;
  if (*p_void) {
    free(*p_void);
    *p_void = NULL;
  }
}

void gz_col_view(void)
{
  const int dyn_poly_cap = 0x1000;
  const int dyn_line_cap = 0x1000;

  static Gfx *stc_poly;
  static Gfx *stc_line;
  static Gfx *dyn_poly_buf[2];
  static Gfx *dyn_line_buf[2];
  static int  dyn_gfx_idx = 0;

  static int col_view_scene;
  static int col_view_water;
  static int col_view_wfc;
  static int col_view_line;
  static int col_view_rd;

  poly_writer_t poly_writer;
  line_writer_t line_writer;

  _Bool enable = zu_in_game() && z64_game.pause_ctxt.state == 0;
  _Bool init = gz.col_view_state == COLVIEW_START ||
               gz.col_view_state == COLVIEW_RESTARTING;

  /* restart if needed */
  if (enable && gz.col_view_state == COLVIEW_ACTIVE &&
      settings->bits.col_view_upd &&
      (col_view_scene != z64_game.scene_index ||
       col_view_water != settings->bits.col_view_water ||
       col_view_wfc != settings->bits.col_view_wfc ||
       col_view_line != settings->bits.col_view_line ||
       col_view_rd != settings->bits.col_view_rd))
  {
    gz.col_view_state = COLVIEW_RESTART;
  }

  /* update state */
  switch (gz.col_view_state) {
    case COLVIEW_STOP:
      gz.col_view_state = COLVIEW_STOPPING;
      break;

    case COLVIEW_RESTART:
      gz.col_view_state = COLVIEW_RESTARTING;
      break;

    case COLVIEW_STOPPING:
      gz.col_view_state = COLVIEW_INACTIVE;
    case COLVIEW_RESTARTING:
      release_mem(&stc_poly);
      release_mem(&stc_line);
      release_mem(&dyn_poly_buf[0]);
      release_mem(&dyn_poly_buf[1]);
      release_mem(&dyn_line_buf[0]);
      release_mem(&dyn_line_buf[1]);
      break;

    default:
      break;
  }

  /* initialize */
  if (enable && init) {
    col_view_scene = z64_game.scene_index;
    col_view_water = settings->bits.col_view_water;
    col_view_wfc = settings->bits.col_view_wfc;
    col_view_line = settings->bits.col_view_line;
    col_view_rd = settings->bits.col_view_rd;

    z64_col_hdr_t *col_hdr = z64_game.col_ctxt.col_hdr;

    size_t stc_poly_cap = 17 + 9 * col_hdr->n_poly;
    size_t stc_line_cap = 24 + 11 * col_hdr->n_poly;

    /* allocate static polygon display list */
    stc_poly = malloc(sizeof(*stc_poly) * stc_poly_cap);
    Gfx *stc_poly_p = stc_poly;
    Gfx *stc_poly_d = stc_poly + stc_poly_cap;
    poly_writer_t *p_poly_writer = &poly_writer;
    poly_writer_init(p_poly_writer, stc_poly_p, stc_poly_d);

    /* allocate static line display list */
    Gfx *stc_line_p = NULL;
    Gfx *stc_line_d = NULL;
    line_writer_t *p_line_writer = NULL;
    struct vector line_set;
    if (col_view_line) {
      stc_line = malloc(sizeof(*stc_line) * stc_line_cap);
      stc_line_p = stc_line;
      stc_line_d = stc_line + stc_line_cap;
      p_line_writer = &line_writer;
      line_writer_init(p_line_writer, stc_line_p, stc_line_d);
      vector_init(&line_set, sizeof(struct line));
    }

    /* allocate dynamic display lists */
    dyn_poly_buf[0] = malloc(sizeof(*dyn_poly_buf[0]) * dyn_poly_cap);
    dyn_poly_buf[1] = malloc(sizeof(*dyn_poly_buf[1]) * dyn_poly_cap);

    if (col_view_line) {
      dyn_line_buf[0] = malloc(sizeof(*dyn_line_buf[0]) * dyn_line_cap);
      dyn_line_buf[1] = malloc(sizeof(*dyn_line_buf[1]) * dyn_line_cap);
    }

    /* generate static display lists */
    do_poly_list(p_poly_writer, p_line_writer, &line_set,
                 col_hdr->vtx, col_hdr->poly, col_hdr->type, col_hdr->n_poly,
                 col_view_rd, col_view_wfc);

    poly_writer_finish(p_poly_writer, &stc_poly_p, &stc_poly_d);

    gSPEndDisplayList(stc_poly_p++);
    dcache_wb(stc_poly, sizeof(*stc_poly) * stc_poly_cap);

    if (col_view_line) {
      line_writer_finish(p_line_writer, &stc_line_p, &stc_line_d);
      gSPEndDisplayList(stc_line_p++);
      dcache_wb(stc_line, sizeof(*stc_line) * stc_line_cap);
      vector_destroy(&line_set);
    }

    gz.col_view_state = COLVIEW_ACTIVE;
  }

  _Bool active = gz.col_view_state == COLVIEW_ACTIVE;

  /* generate dynamic display lists */
  if (enable && (init || (active && settings->bits.col_view_upd))) {
    Gfx *dyn_poly = NULL;
    Gfx *dyn_poly_p = NULL;
    Gfx *dyn_poly_d = NULL;
    poly_writer_t *p_poly_writer = NULL;
    Gfx *dyn_line = NULL;
    Gfx *dyn_line_p = NULL;
    Gfx *dyn_line_d = NULL;
    line_writer_t *p_line_writer = NULL;

    dyn_gfx_idx = (dyn_gfx_idx + 1) % 2;

    dyn_poly = dyn_poly_buf[dyn_gfx_idx];
    dyn_poly_p = dyn_poly;
    dyn_poly_d = dyn_poly + dyn_poly_cap;
    p_poly_writer = &poly_writer;
    poly_writer_init(p_poly_writer, dyn_poly_p, dyn_poly_d);

    if (col_view_line) {
      dyn_line = dyn_line_buf[dyn_gfx_idx];
      dyn_line_p = dyn_line;
      dyn_line_d = dyn_line + dyn_line_cap;
      p_line_writer = &line_writer;
      line_writer_init(p_line_writer, dyn_line_p, dyn_line_d);
    }

    do_dyn_col(p_poly_writer, p_line_writer, col_view_rd, col_view_wfc);

    poly_writer_finish(p_poly_writer, &dyn_poly_p, &dyn_poly_d);

    if (col_view_water) {
      gDPSetPrimColor(dyn_poly_p++, 0, 0, 0x57, 0xAC, 0xF3, 0xFF);

      /* which waterbox to draw depends on currently loaded room, so even
       * static waterboxes may need updating */
      do_waterbox_list(&dyn_poly_p, &dyn_poly_d,
                       z64_game.col_ctxt.col_hdr->n_water,
                       z64_game.col_ctxt.col_hdr->water);

      for (int i = 0; i < 50; ++i) {
        if (z64_game.col_ctxt.dyn_flags[i].active) {
          z64_dyn_col_t *dyn_col = &z64_game.col_ctxt.dyn_col[i];
          do_waterbox_list(&dyn_poly_p, &dyn_poly_d,
                           dyn_col->col_hdr->n_water, dyn_col->col_hdr->water);
        }
      }
      /*
      * There is a special hardcoded check for Zora's Domain in a function
      * related to handling collision detection with waterboxes that creates a
      * "fake" waterbox between two hardcoded positions. Unlike every other
      * waterbox in the game, this one has a depth below which you fall out of
      * the bottom.
      */
      if (z64_game.scene_index == 0x58) {
        z64_xyzf_t min_pos = (z64_xyzf_t){ -348.0f, 777.0f, -1746.0f };
        z64_xyzf_t max_pos = (z64_xyzf_t){  205.0f, 877.0f, -967.0f  };

        draw_cuboid(&dyn_poly_p, &dyn_poly_d, &min_pos, &max_pos);
      }
    }

    gSPEndDisplayList(dyn_poly_p++);
    dcache_wb(dyn_poly, sizeof(*dyn_poly) * dyn_poly_cap);

    if (col_view_line) {
      line_writer_finish(p_line_writer, &dyn_line_p, &dyn_line_d);
      gSPEndDisplayList(dyn_line_p++);
      dcache_wb(dyn_line, sizeof(*dyn_line) * dyn_line_cap);
    }
  }

  /* draw it! */
  if (enable && active) {
    Gfx **p_gfx_p;
    Gfx **p_gfx_d;
    if (settings->bits.col_view_xlu) {
      p_gfx_p = &z64_ctxt.gfx->poly_xlu.p;
      p_gfx_d = &z64_ctxt.gfx->poly_xlu.d;
    }
    else {
      p_gfx_p = &z64_ctxt.gfx->poly_opa.p;
      p_gfx_d = &z64_ctxt.gfx->poly_opa.d;
    }

    /* polys */
    init_poly_gfx(p_gfx_p, p_gfx_d,
                  settings->bits.col_view_mode,
                  settings->bits.col_view_xlu,
                  settings->bits.col_view_shade);
    gSPSetGeometryMode((*p_gfx_p)++, G_CULL_BACK);
    gSPDisplayList((*p_gfx_p)++, stc_poly);
    gSPDisplayList((*p_gfx_p)++, dyn_poly_buf[dyn_gfx_idx]);

    /* lines */
    if (col_view_line) {
      load_l3dex2(p_gfx_p);
      init_line_gfx(p_gfx_p, p_gfx_d, settings->bits.col_view_mode,
                    settings->bits.col_view_xlu);
      gSPDisplayList((*p_gfx_p)++, stc_line);
      gSPDisplayList((*p_gfx_p)++, dyn_line_buf[dyn_gfx_idx]);
      unload_l3dex2(p_gfx_p);
      zu_set_lighting_ext(p_gfx_p, p_gfx_d);
    }
  }
}

static void do_hitbox_list(Gfx **p_gfx_p, Gfx **p_gfx_d,
                           int n_hit, z64_hit_t **hit_list,
                           uint32_t color)
{
  gDPSetPrimColor((*p_gfx_p)++, 0, 0,
                  (color >> 16) & 0xFF,
                  (color >> 8)  & 0xFF,
                  (color >> 0)  & 0xFF,
                  0xFF);

  for (int i = 0; i < n_hit; ++i) {
    z64_hit_t *hit = hit_list[i];

    switch (hit->type) {
      case Z64_HIT_SPH_LIST: {
        z64_hit_sph_list_t *hit_sph_list = (z64_hit_sph_list_t *)hit;

        for (int j = 0; j < hit_sph_list->n_ent; ++j) {
          z64_hit_sph_ent_t *ent = &hit_sph_list->ent_list[j];

          /* make zero radius spheres just appear really small */
          int radius = ent->radius;
          if (radius == 0)
            radius = 1;

          draw_ico_sphere(p_gfx_p, p_gfx_d,
                          ent->pos.x, ent->pos.y, ent->pos.z, radius);
        }

        break;
      }
      case Z64_HIT_CYL: {
        z64_hit_cyl_t *hit_cyl = (z64_hit_cyl_t *)hit;

        /* ditto for cylinders */
        int radius = hit_cyl->radius;
        if (radius == 0)
          radius = 1;

        draw_cyl(p_gfx_p, p_gfx_d,
                 hit_cyl->pos.x, hit_cyl->pos.y + hit_cyl->y_offset,
                 hit_cyl->pos.z, radius, hit_cyl->height);

        break;
      }
      case Z64_HIT_TRI_LIST: {
        z64_hit_tri_list_t *hit_tri_list = (z64_hit_tri_list_t *)hit;

        for (int j = 0; j < hit_tri_list->n_ent; ++j) {
          z64_hit_tri_ent_t *ent = &hit_tri_list->ent_list[j];

          draw_tri(p_gfx_p, p_gfx_d, &ent->v[0], &ent->v[2], &ent->v[1]);
        }

        break;
      }
      case Z64_HIT_QUAD: {
        z64_hit_quad_t *hit_quad = (z64_hit_quad_t *)hit;

        draw_quad(p_gfx_p, p_gfx_d,
                  &hit_quad->v[0], &hit_quad->v[2],
                  &hit_quad->v[3], &hit_quad->v[1]);

        break;
      }
    }
  }
}

void gz_hit_view(void)
{
  const int hit_gfx_cap = 0x800;

  static Gfx *hit_gfx_buf[2];
  static int  hit_gfx_idx = 0;

  _Bool enable = zu_in_game() && z64_game.pause_ctxt.state == 0;

  if (enable && gz.hit_view_state == HITVIEW_START) {
    hit_gfx_buf[0] = malloc(sizeof(*hit_gfx_buf[0]) * hit_gfx_cap);
    hit_gfx_buf[1] = malloc(sizeof(*hit_gfx_buf[1]) * hit_gfx_cap);

    gz.hit_view_state = HITVIEW_ACTIVE;
  }
  if (enable && gz.hit_view_state == HITVIEW_ACTIVE) {
    Gfx **p_gfx_p;
    if (settings->bits.hit_view_xlu)
      p_gfx_p = &z64_ctxt.gfx->poly_xlu.p;
    else
      p_gfx_p = &z64_ctxt.gfx->poly_opa.p;

    Gfx *hit_gfx = hit_gfx_buf[hit_gfx_idx];
    Gfx *hit_gfx_p = hit_gfx;
    Gfx *hit_gfx_d = hit_gfx + hit_gfx_cap;
    hit_gfx_idx = (hit_gfx_idx + 1) % 2;

    init_poly_gfx(&hit_gfx_p, &hit_gfx_d, SETTINGS_COLVIEW_SURFACE,
                                          settings->bits.hit_view_xlu,
                                          settings->bits.hit_view_shade);

    if (settings->bits.hit_view_oc)
      do_hitbox_list(&hit_gfx_p, &hit_gfx_d,
                     z64_game.hit_ctxt.n_oc, z64_game.hit_ctxt.oc_list, 0xFFFFFF);

    if (settings->bits.hit_view_ac)
      do_hitbox_list(&hit_gfx_p, &hit_gfx_d,
                     z64_game.hit_ctxt.n_ac, z64_game.hit_ctxt.ac_list, 0x0000FF);

    if (settings->bits.hit_view_at)
      do_hitbox_list(&hit_gfx_p, &hit_gfx_d,
                     z64_game.hit_ctxt.n_at, z64_game.hit_ctxt.at_list, 0xFF0000);

    gSPEndDisplayList(hit_gfx_p++);
    dcache_wb(hit_gfx, sizeof(*hit_gfx) * hit_gfx_cap);

    gSPDisplayList((*p_gfx_p)++, hit_gfx);
  }
  if (gz.hit_view_state == HITVIEW_STOP)
    gz.hit_view_state = HITVIEW_STOPPING;
  else if (gz.hit_view_state == HITVIEW_STOPPING) {
    release_mem(&hit_gfx_buf[0]);
    release_mem(&hit_gfx_buf[1]);

    gz.hit_view_state = HITVIEW_INACTIVE;
  }
}

static inline int path_valid(z64_path_t *path)
{
  uintptr_t points_addr = (uintptr_t)path->points;
  return path->numpoints != 0 && (points_addr >> 24) == Z64_SEG_SCENE;
}

void gz_path_view(void)
{
  const int poly_gfx_cap = 0x480;
  const int line_gfx_cap = 0x380;

  static Gfx *poly_gfx_buf[2];
  static Gfx *line_gfx_buf[2];
  static int path_gfx_idx = 0;

  static _Bool path_view_points;
  static _Bool path_view_lines;

  _Bool enable = zu_in_game() && z64_game.pause_ctxt.state == 0 &&
                 z64_game.path_list != NULL;
  _Bool init = gz.path_view_state == PATHVIEW_START ||
               gz.path_view_state == PATHVIEW_RESTARTING;

  /* restart if needed */
  if (enable && gz.path_view_state == PATHVIEW_ACTIVE &&
      (path_view_points != settings->bits.path_view_points ||
       path_view_lines != settings->bits.path_view_lines))
  {
    gz.path_view_state = PATHVIEW_RESTART;
  }

  /* update state */
  switch (gz.path_view_state) {
    case PATHVIEW_STOP:
      gz.path_view_state = PATHVIEW_STOPPING;
      break;

    case PATHVIEW_RESTART:
      gz.path_view_state = PATHVIEW_RESTARTING;
      break;

    case PATHVIEW_STOPPING:
      gz.path_view_state = PATHVIEW_INACTIVE;
    case PATHVIEW_RESTARTING:
      release_mem(&poly_gfx_buf[0]);
      release_mem(&poly_gfx_buf[1]);
      release_mem(&line_gfx_buf[0]);
      release_mem(&line_gfx_buf[1]);
      break;

    default:
      break;
  }

  if (enable && init) {
    path_view_points = settings->bits.path_view_points;
    path_view_lines = settings->bits.path_view_lines;

    if (path_view_points) {
      poly_gfx_buf[0] = malloc(sizeof(*poly_gfx_buf[0]) * poly_gfx_cap);
      poly_gfx_buf[1] = malloc(sizeof(*poly_gfx_buf[1]) * poly_gfx_cap);
    }

    if (path_view_lines) {
      line_gfx_buf[0] = malloc(sizeof(*line_gfx_buf[0]) * line_gfx_cap);
      line_gfx_buf[1] = malloc(sizeof(*line_gfx_buf[1]) * line_gfx_cap);
    }

    gz.path_view_state = PATHVIEW_ACTIVE;
  }

  if (enable && gz.path_view_state == PATHVIEW_ACTIVE) {
    int xlu = settings->bits.path_view_xlu;
    Gfx **p_gfx_p;
    if (xlu)
      p_gfx_p = &z64_ctxt.gfx->poly_xlu.p;
    else
      p_gfx_p = &z64_ctxt.gfx->poly_opa.p;

    if (path_view_points) {
      Gfx *poly_gfx = poly_gfx_buf[path_gfx_idx];
      Gfx *poly_gfx_p = poly_gfx;
      Gfx *poly_gfx_d = poly_gfx + poly_gfx_cap;

      init_poly_gfx(&poly_gfx_p, &poly_gfx_d, SETTINGS_COLVIEW_SURFACE, xlu, 1);

      for (z64_path_t *path = z64_game.path_list; path_valid(path); ++path) {
        z64_xyz_t *points = zu_zseg_locate(path->points);
        for (int i = 0; i < path->numpoints; ++i) {
          if (path->numpoints == 1)
            gDPSetPrimColor(poly_gfx_p++, 0, 0, 0x00, 0xFF, 0xFF, 0xFF);
          else {
            uint8_t r = 0xFF * i / (path->numpoints - 1);
            gDPSetPrimColor(poly_gfx_p++, 0, 0, r, 0xFF, 0xFF - r, 0xFF);
          }
          draw_ico_sphere(&poly_gfx_p, &poly_gfx_d,
                          points[i].x, points[i].y, points[i].z, 18.f);
        }
      }

      gSPEndDisplayList(poly_gfx_p++);
      dcache_wb(poly_gfx, sizeof(*poly_gfx) * poly_gfx_cap);
      gSPDisplayList((*p_gfx_p)++, poly_gfx);
    }

    if (path_view_lines) {
      Gfx *line_gfx = line_gfx_buf[path_gfx_idx];
      Gfx *line_gfx_p = line_gfx;
      Gfx *line_gfx_d = line_gfx + line_gfx_cap;

      load_l3dex2(&line_gfx_p);
      init_line_gfx(&line_gfx_p, &line_gfx_d, SETTINGS_COLVIEW_SURFACE, xlu);

      for (z64_path_t *path = z64_game.path_list; path_valid(path); ++path) {
        z64_xyz_t *points = zu_zseg_locate(path->points);
        for (int i = 0; i + 1 < path->numpoints; ++i)
          draw_line(&line_gfx_p, &line_gfx_d, &points[i], &points[i + 1]);
      }

      unload_l3dex2(&line_gfx_p);
      zu_set_lighting_ext(&line_gfx_p, &line_gfx_d);
      gSPEndDisplayList(line_gfx_p++);
      dcache_wb(line_gfx, sizeof(*line_gfx) * line_gfx_cap);
      gSPDisplayList((*p_gfx_p)++, line_gfx);
    }

    path_gfx_idx = (path_gfx_idx + 1) % 2;
  }
}

static void actor_cull_vertex(z64_xyzf_t *Av, z64_xyzf_t *Bv, z64_xyzf_t *Cv)
{
  MtxF mf;

  z64_xyzf_t A[4];
  float Aw;
  z64_xyzf_t B[4];
  float Bw;
  z64_xyzf_t C[4];
  float Cw;

  /* get inverse projection matrix */
  guMtxInvertF(&z64_game.mf_11D60, &mf);

  float x1;
  float x2;
  float y1;
  float y2;
  float z;

  float p1 = gz.selected_actor.ptr->uncullZoneForward;
  float p2 = gz.selected_actor.ptr->uncullZoneScale;
  float p3 = gz.selected_actor.ptr->uncullZoneDownward;

  /* Front face vertices */
  Aw = (1.f - (p1 + p2) * mf.zw) / mf.ww;
  z = p1 + p2;
  y1 = -(Aw + p3);
  y2 = Aw + p2;
  x1 = Aw + p2;
  x2 = -(Aw + p2);

  A[0].x = x1;
  A[0].y = y1;
  A[0].z = z;

  A[1].x = x1;
  A[1].y = y2;
  A[1].z = z;

  A[2].x = x2;
  A[2].y = y2;
  A[2].z = z;

  A[3].x = x2;
  A[3].y = y1;
  A[3].z = z;

  /* middle vertices */
  Bw = 1.f;
  z = (1.f - mf.ww) / mf.zw;
  y1 = -(Bw + p3);
  y2 = Bw + p2;
  x1 = Bw + p2;
  x2 = -(Bw + p2);

  B[0].x = x1;
  B[0].y = y1;
  B[0].z = z;

  B[1].x = x1;
  B[1].y = y2;
  B[1].z = z;

  B[2].x = x2;
  B[2].y = y2;
  B[2].z = z;

  B[3].x = x2;
  B[3].y = y1;
  B[3].z = z;

  /* tail face vertices */
  Cw = (1.f + p2 * mf.zw) / mf.ww;
  z = -p2;
  y1 = -(1.f + p3);
  y2 = 1.f + p2;
  x1 = 1.f + p2;
  x2 = -(1.f + p2);

  C[0].x = x1;
  C[0].y = y1;
  C[0].z = z;

  C[1].x = x1;
  C[1].y = y2;
  C[1].z = z;

  C[2].x = x2;
  C[2].y = y2;
  C[2].z = z;

  C[3].x = x2;
  C[3].y = y1;
  C[3].z = z;

  for (int i = 0; i < 4; i++) {
    vec3f_xfmw(&Av[i], &A[i], Aw, &mf);
    vec3f_xfmw(&Bv[i], &B[i], Bw, &mf);
    vec3f_xfmw(&Cv[i], &C[i], Cw, &mf);
  }
}

void gz_cull_view(void)
{
  const int cull_gfx_cap = 0x3B0;
  static Gfx *cull_gfx_buf[2];
  static int cull_gfx_idx = 0;
  _Bool enable = zu_in_game() && z64_game.pause_ctxt.state == 0;

  if (enable && gz.cull_view_state == CULLVIEW_START) {
    cull_gfx_buf[0] = malloc(sizeof(*cull_gfx_buf[0]) * cull_gfx_cap);
    cull_gfx_buf[1] = malloc(sizeof(*cull_gfx_buf[1]) * cull_gfx_cap);

    gz.cull_view_state = CULLVIEW_ACTIVE;
  }
  if (enable && gz.cull_view_state == CULLVIEW_ACTIVE) {
    /* If no actor is selected, stop */
    if (!gz.selected_actor.ptr) {
      gz.cull_view_state = CULLVIEW_STOP;
      return;
    }
    /* Now look through actor type list */
    _Bool actor_found = 0;
    uint16_t n_entries = z64_game.actor_list[gz.selected_actor.type].length;
    z64_actor_t *actor = z64_game.actor_list[gz.selected_actor.type].first;

    /* If first actor is not the same mem address and ID, loop through until
     * you find it.
     */
    if (actor != gz.selected_actor.ptr ||
        actor->actor_id != gz.selected_actor.id)
    {
      for (int i = 0; i < n_entries-1; ++i) {
        actor = actor->next;
        if (actor == gz.selected_actor.ptr &&
            actor->actor_id == gz.selected_actor.id)
        {
          actor_found = 1;
          break;
        }
      }
    }
    else
      actor_found = 1;

    /* Check if we found it and if not, turn off cull view */
    if (!actor_found) {
      gz.cull_view_state = CULLVIEW_STOP;
      return;
    }

    /* Get all 12 vertices for current frame */
    z64_xyzf_t A[4];
    z64_xyzf_t B[4];
    z64_xyzf_t C[4];
    actor_cull_vertex(A, B, C);

    /* Drawing: */
    Gfx **p_gfx_p;
    p_gfx_p = &z64_ctxt.gfx->poly_xlu.p;

    Gfx *cull_gfx = cull_gfx_buf[cull_gfx_idx];
    Gfx *cull_gfx_p = cull_gfx;
    Gfx *cull_gfx_d = cull_gfx + cull_gfx_cap;
    cull_gfx_idx = (cull_gfx_idx + 1) % 2;
    init_poly_gfx(&cull_gfx_p, &cull_gfx_d, SETTINGS_COLVIEW_SURFACE,
                  1 /* xlu */,
                  0 /* shaded */);

    uint32_t color;
    if (gz.selected_actor.ptr->flags & 0x0040)
      color = 0x008000; /* green */
    else
      color = 0x800000; /* red */

    gDPSetPrimColor((*p_gfx_p)++, 0, 0,
                    (color >> 16) & 0xFF,
                    (color >> 8)  & 0xFF,
                    (color >> 0)  & 0xFF,
                    0xFF);
    /* Front face */
    draw_quad(&cull_gfx_p, &cull_gfx_d, &A[0], &A[1], &A[2], &A[3]);
    /* Front Sides */
    draw_quad(&cull_gfx_p, &cull_gfx_d, &A[2], &A[3], &B[3], &B[2]);
    draw_quad(&cull_gfx_p, &cull_gfx_d, &A[0], &A[1], &B[1], &B[0]);
    draw_quad(&cull_gfx_p, &cull_gfx_d, &A[0], &A[3], &B[3], &B[0]);
    draw_quad(&cull_gfx_p, &cull_gfx_d, &A[1], &A[2], &B[2], &B[1]);
    /* Tail Sides */
    draw_quad(&cull_gfx_p, &cull_gfx_d, &B[2], &B[3], &C[3], &C[2]);
    draw_quad(&cull_gfx_p, &cull_gfx_d, &B[0], &B[1], &C[1], &C[0]);
    draw_quad(&cull_gfx_p, &cull_gfx_d, &B[0], &B[3], &C[3], &C[0]);
    draw_quad(&cull_gfx_p, &cull_gfx_d, &B[1], &B[2], &C[2], &C[1]);
    /* Tail face */
    draw_quad(&cull_gfx_p, &cull_gfx_d, &C[0], &C[1], &C[2], &C[3]);

    gSPEndDisplayList(cull_gfx_p++);
    dcache_wb(cull_gfx, sizeof(*cull_gfx) * cull_gfx_cap);

    gSPDisplayList((*p_gfx_p)++, cull_gfx);
  }
  if (gz.cull_view_state == CULLVIEW_STOP)
    gz.cull_view_state = CULLVIEW_STOPPING;
  else if (gz.cull_view_state == CULLVIEW_STOPPING) {
    release_mem(&cull_gfx_buf[0]);
    release_mem(&cull_gfx_buf[1]);

    gz.cull_view_state = CULLVIEW_INACTIVE;
  }
}

/*
 *  En_Holl Params
 *
 *  1111110000000000    = Transition actor id
 *  0000000111000000    = Type
 *    00  Two-Way Vertical Fading
 *    01  One-Way Horizontal Fading
 *    02  Two-Way Horizontal No Fade
 *    03  Switch Flag Vertical Fading
 *    04  Two-Way Vertical No Fade
 *    05  Two-Way Horizontal Fading
 *    06  Two-Way Vertical No Fade Wider
 *  0000000000111111    = Switch flag (relevant to type 3 only)
 */
enum en_holl_type
{
  /* 0 */ HOLL_TWO_WAY_VERTICAL_FADING,
  /* 1 */ HOLL_ONE_WAY_HORIZONTAL_FADING,
  /* 2 */ HOLL_TWO_WAY_HORIZONTAL_NO_FADE,
  /* 3 */ HOLL_SWITCH_FLAG_VERTICAL_FADING,
  /* 4 */ HOLL_TWO_WAY_VERTICAL_NO_FADE,
  /* 5 */ HOLL_TWO_WAY_HORIZONTAL_FADING,
  /* 6 */ HOLL_TWO_WAY_VERTICAL_NO_FADE_WIDE
};

enum en_holl_draw_type
{
  HOLL_DRAW_TYPE_NONE,     /* nothing     */
  HOLL_DRAW_TYPE_CUBOID_2, /* 2 cuboids   */
  HOLL_DRAW_TYPE_CUBOID_4, /* 4 cuboids   */
  HOLL_DRAW_TYPE_CYL_2,    /* 2 cylinders */
  HOLL_DRAW_TYPE_CYL_1     /* 1 cylinder  */
};

void gz_holl_view(void)
{
  const int holl_gfx_cap = 0x800;
  static Gfx *holl_gfx_buf[2];
  static int holl_gfx_idx = 0;

  _Bool enable = zu_in_game() && z64_game.pause_ctxt.state == 0;

  if (enable && gz.holl_view_state == HOLLVIEW_START) {
    holl_gfx_buf[0] = malloc(sizeof(*holl_gfx_buf[0]) * holl_gfx_cap);
    holl_gfx_buf[1] = malloc(sizeof(*holl_gfx_buf[1]) * holl_gfx_cap);

    gz.holl_view_state = HOLLVIEW_ACTIVE;
  }
  if (enable && gz.holl_view_state == HOLLVIEW_ACTIVE) {
    Gfx **p_gfx_p;
    if (settings->bits.col_view_xlu)
      p_gfx_p = &z64_ctxt.gfx->poly_xlu.p;
    else
      p_gfx_p = &z64_ctxt.gfx->poly_opa.p;

    Gfx *holl_gfx = holl_gfx_buf[holl_gfx_idx];
    Gfx *holl_gfx_p = holl_gfx;
    Gfx *holl_gfx_d = holl_gfx + holl_gfx_cap;
    holl_gfx_idx = (holl_gfx_idx + 1) % 2;

    init_poly_gfx(&holl_gfx_p, &holl_gfx_d, SETTINGS_COLVIEW_SURFACE,
                  settings->bits.holl_view_xlu, 1);

    _Bool show_all = settings->bits.holl_view_all; /* Show all regions, even inactive ones */

    for (z64_actor_t *actor = z64_game.actor_list[Z64_ACTORTYPE_DOOR].first; actor != NULL; actor = actor->next)
    {
      if (actor->actor_id != Z64_ACTOR_EN_HOLL)
        continue;

      z64_xyzf_t p_min = { 0 }, p_max = { 0 };
      z64_xyzf_t p2_min = { 0 }, p2_max = { 0 };
      float cyl_radius, cyl_offset, cyl_height;

      enum en_holl_draw_type draw_type = HOLL_DRAW_TYPE_NONE;
      enum en_holl_type holl_type = (actor->variable >> 6) & 7;

      z64_tnsn_actor_t *tnsn_entry = &z64_game.room_ctxt.tnsn_list[(actor->variable >> 10) & 0x3F];
      int side = tnsn_entry->room_idx_1 == z64_game.room_ctxt.rooms[0].index;

      /* transformation to world coordinates centered on and facing the same direction as the actor */
      Mtx *p_m;
      {
        MtxF mf;
        MtxF mt;
        guRotateF(&mf, actor->rot_2.y * M_PI / 0x8000, 0.0f, 1.0f, 0.0f);
        guTranslateF(&mt, actor->pos_2.x, actor->pos_2.y, actor->pos_2.z);
        guMtxCatF(&mf, &mt, &mf);
        p_m = gDisplayListAlloc(&holl_gfx_d, sizeof(*p_m));
        guMtxF2L(&mf, p_m);
      }
      gSPMatrix(holl_gfx_p++, p_m, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);

      /* determine the geometry */
      switch (holl_type)
      {
#define HOLL_MIN_Y -50.0f
#define HOLL_MAX_Y 200.0f
#define HOLL_HALFWIDTH_X 100.0f
        case HOLL_TWO_WAY_VERTICAL_FADING:
          p_min  = (z64_xyzf_t){ -HOLL_HALFWIDTH_X , HOLL_MIN_Y , 150.0f };
          p_max  = (z64_xyzf_t){  HOLL_HALFWIDTH_X , HOLL_MAX_Y , 200.0f };
          p2_min = (z64_xyzf_t){ -HOLL_HALFWIDTH_X , HOLL_MIN_Y ,   0.0f };
          p2_max = (z64_xyzf_t){  HOLL_HALFWIDTH_X , HOLL_MAX_Y , 150.0f };
          if (z64_game.scene_index == 6)
          {
            /* spirit temple special case */
            p_min.z /= 2.0f;
            p_max.z /= 2.0f;
            p2_min.z /= 2.0f;
            p2_max.z /= 2.0f;
          }
          draw_type = HOLL_DRAW_TYPE_CUBOID_4;
          break;
        case HOLL_ONE_WAY_HORIZONTAL_FADING:
          cyl_radius = 500.0f;
          cyl_offset = 0.0f;
          cyl_height = 95.0f - cyl_offset;
          draw_type = HOLL_DRAW_TYPE_CYL_1;
          break;
        case HOLL_TWO_WAY_HORIZONTAL_NO_FADE:
          cyl_radius = 120.0f;
          cyl_offset = 50.0f;
          cyl_height = 200.0f - cyl_offset;
          draw_type = HOLL_DRAW_TYPE_CYL_2;
          break;
        case HOLL_SWITCH_FLAG_VERTICAL_FADING:
          {
            _Bool flag_set;

            if ((actor->variable & 0x3F) < 0x20)
              flag_set = z64_game.swch_flags & (1 << (actor->variable & 0x3F));
            else
              flag_set = z64_game.temp_swch_flags & (1 << ((actor->variable & 0x3F) - 0x20));
            if (!flag_set)
              break; /* draw nothing if the relevant switch flag is not set */
          }
          p_min = (z64_xyzf_t){ -2.0f * HOLL_HALFWIDTH_X , HOLL_MIN_Y ,  0.0f };
          p_max = (z64_xyzf_t){  2.0f * HOLL_HALFWIDTH_X , HOLL_MAX_Y , 50.0f };
          draw_type = HOLL_DRAW_TYPE_CUBOID_2;
          break;
        case HOLL_TWO_WAY_VERTICAL_NO_FADE:
          p_min = (z64_xyzf_t){ -2.0f * HOLL_HALFWIDTH_X , HOLL_MIN_Y ,  50.0f };
          p_max = (z64_xyzf_t){  2.0f * HOLL_HALFWIDTH_X , HOLL_MAX_Y , 100.0f };
          draw_type = HOLL_DRAW_TYPE_CUBOID_2;
          break;
        case HOLL_TWO_WAY_HORIZONTAL_FADING:
          cyl_radius = 120.0f;
          cyl_offset = 50.0f;
          cyl_height = 200.0f - cyl_offset;
          draw_type = HOLL_DRAW_TYPE_CYL_2;
          break;
        case HOLL_TWO_WAY_VERTICAL_NO_FADE_WIDE:
          p_min = (z64_xyzf_t){ -HOLL_HALFWIDTH_X , HOLL_MIN_Y ,  50.0f };
          p_max = (z64_xyzf_t){  HOLL_HALFWIDTH_X , HOLL_MAX_Y , 100.0f };
          draw_type = HOLL_DRAW_TYPE_CUBOID_2;
          break;
      }

      /* draw it */
      switch (draw_type)
      {
        case HOLL_DRAW_TYPE_NONE:
          break;
        case HOLL_DRAW_TYPE_CUBOID_4:
          if (side == 1)
          {
            p2_max.z = -p2_max.z;
            p2_min.z = -p2_min.z;
            p_min.z = -p_min.z;
            p_max.z = -p_max.z;
          }

          _Bool both_rooms_loaded = (z64_game.room_ctxt.rooms[0].file != NULL && z64_game.room_ctxt.rooms[1].file != NULL);

          if (both_rooms_loaded)
          {
            if (show_all)
            {
              gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0, 0, 0xFF);
              draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p2_min, &p2_max);
              p2_min.z = -p2_min.z;
              p2_max.z = -p2_max.z;
              gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0, 0, 0xFF);
              draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p2_min, &p2_max);
            }

            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0xFF, 0, 0xFF);
            draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p_min, &p_max);
            p_min.z = -p_min.z;
            p_max.z = -p_max.z;
            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0, 0xFF, 0, 0xFF);
            draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p_min, &p_max);
          }
          else
          {
            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0, 0xFF, 0, 0xFF);
            draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p2_min, &p2_max);
            p2_min.z = -p2_min.z;
            p2_max.z = -p2_max.z;
            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0xFF, 0, 0xFF);
            draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p2_min, &p2_max);

            if (show_all)
            {
              gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0, 0, 0xFF);
              draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p_min, &p_max);
              p_min.z = -p_min.z;
              p_max.z = -p_max.z;
              gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0, 0, 0xFF);
              draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p_min, &p_max);
            }
          }
          break;
        case HOLL_DRAW_TYPE_CUBOID_2:
          if (side == 0)
          {
            p_min.z = -p_min.z;
            p_max.z = -p_max.z;
          }
          gDPSetPrimColor(holl_gfx_p++, 0, 0, 0, 0xFF, 0, 0xFF);
          draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p_min, &p_max);

          if (show_all)
          {
            p_min.z = -p_min.z;
            p_max.z = -p_max.z;
            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0, 0, 0x80);
            draw_cuboid(&holl_gfx_p, &holl_gfx_d, &p_min, &p_max);
          }
          break;
        case HOLL_DRAW_TYPE_CYL_2:
          {
            float offset = (side == 1) ? -(cyl_offset + cyl_height) : cyl_offset;

            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0, 0xFF, 0, 0xFF);
            draw_cyl(&holl_gfx_p, &holl_gfx_d,
                    actor->pos_2.x, actor->pos_2.y + offset, actor->pos_2.z,
                    cyl_radius, cyl_height);

            if (show_all)
            {
              offset = (side == 0) ? -(cyl_offset + cyl_height) : cyl_offset;

              gDPSetPrimColor(holl_gfx_p++, 0, 0, 0xFF, 0, 0, 0x80);
              draw_cyl(&holl_gfx_p, &holl_gfx_d,
                      actor->pos_2.x, actor->pos_2.y + offset, actor->pos_2.z,
                      cyl_radius, cyl_height); 
            }
          }
          break;
        case HOLL_DRAW_TYPE_CYL_1:
          {
            if (side == 0)
              break;

            gDPSetPrimColor(holl_gfx_p++, 0, 0, 0, 0xFF, 0, 0xFF);
            draw_cyl(&holl_gfx_p, &holl_gfx_d,
                    actor->pos_2.x, actor->pos_2.y + cyl_offset, actor->pos_2.z,
                    cyl_radius, cyl_height);
          }
          break;
      }
      gSPPopMatrix(holl_gfx_p++);
    }
    gSPEndDisplayList(holl_gfx_p++);
    dcache_wb(holl_gfx, sizeof(*holl_gfx) * holl_gfx_cap);

    gSPDisplayList((*p_gfx_p)++, holl_gfx);
  }

  if (gz.holl_view_state == HOLLVIEW_BEGIN_STOP)
    gz.holl_view_state = HOLLVIEW_STOP;
  else if (gz.holl_view_state == HOLLVIEW_STOP) {
    release_mem(&holl_gfx_buf[0]);
    release_mem(&holl_gfx_buf[1]);

    gz.holl_view_state = HOLLVIEW_INACTIVE;
  }
}
