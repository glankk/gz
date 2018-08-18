#include <stdlib.h>
#include <n64.h>
#include <vector/vector.h>
#include <stdint.h>
#include "gfx.h"
#include "gu.h"
#include "gz.h"
#include "settings.h"
#include "ucode.h"
#include "z64.h"

void gz_col_view(void)
{
  static Gfx *poly_disp;
  static Gfx *line_disp;
  static _Bool xlu;
  /* build collision view display list */
  if (gz.col_view_state == 1) {
    xlu = settings->bits.col_view_xlu;
    z64_col_hdr_t *col_hdr = z64_game.col_ctxt.col_hdr;
    uint8_t alpha = xlu ? 0x80 : 0xFF;
    /* initialize polygon dlist */
    Gfx *poly_p = NULL;
    Gfx *poly_d = NULL;
    {
      size_t poly_size = 0x10 + 9 * col_hdr->n_poly;
      if (poly_disp)
        free(poly_disp);
      poly_disp = malloc(sizeof(*poly_disp) * poly_size);
      poly_p = poly_disp;
      poly_d = poly_disp + poly_size;
      uint32_t rm;
      uint32_t blc1;
      uint32_t blc2;
      if (xlu) {
        rm = Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL;
        blc1 = GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA);
        blc2 = GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA);
      }
      else {
        rm = Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP | FORCE_BL;
        blc1 = GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
        blc2 = GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
      }
      if (settings->bits.col_view_mode == SETTINGS_COLVIEW_DECAL)
        rm |= ZMODE_DEC;
      else if (xlu)
        rm |= ZMODE_XLU;
      else
        rm |= ZMODE_OPA;
      gDPPipeSync(poly_p++);
      gDPSetRenderMode(poly_p++, rm | blc1, rm | blc2);
      gSPTexture(poly_p++, qu016(0.5), qu016(0.5), 0, G_TX_RENDERTILE, G_OFF);
      if (settings->bits.col_view_shade) {
        gDPSetCycleType(poly_p++, G_CYC_2CYCLE);
        gDPSetCombineMode(poly_p++, G_CC_PRIMITIVE, G_CC_MODULATERGBA2);
        gSPLoadGeometryMode(poly_p++,
                            G_SHADE | G_LIGHTING | G_ZBUFFER | G_CULL_BACK);
      }
      else {
        gDPSetCycleType(poly_p++, G_CYC_1CYCLE);
        gDPSetCombineMode(poly_p++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gSPLoadGeometryMode(poly_p++, G_ZBUFFER | G_CULL_BACK);
      }
      Mtx m;
      guMtxIdent(&m);
      gSPMatrix(poly_p++,
                gDisplayListData(&poly_d, m), G_MTX_MODELVIEW | G_MTX_LOAD);
    }
    /* initialize line dlist */
    struct line
    {
      int va;
      int vb;
    };
    struct vector line_set;
    Gfx *line_p = NULL;
    Gfx *line_d = NULL;
    if (settings->bits.col_view_line) {
      vector_init(&line_set, sizeof(struct line));
      size_t line_size = 0x18 + 11 * col_hdr->n_poly;
      if (line_disp)
        free(line_disp);
      line_disp = malloc(sizeof(*line_disp) * line_size);
      line_p = line_disp;
      line_d = line_disp + line_size;
      load_l3dex2(&line_p);
      gDPPipeSync(line_p++);
      if (xlu)
        gDPSetRenderMode(line_p++, G_RM_AA_ZB_XLU_LINE, G_RM_AA_ZB_XLU_LINE2);
      else
        gDPSetRenderMode(line_p++, G_RM_AA_ZB_DEC_LINE, G_RM_AA_ZB_DEC_LINE2);
      gSPTexture(line_p++, qu016(0.5), qu016(0.5), 0, G_TX_RENDERTILE, G_OFF);
      gDPSetCycleType(line_p++, G_CYC_1CYCLE);
      gDPSetCombineMode(line_p++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
      gSPLoadGeometryMode(line_p++, G_ZBUFFER);
      Mtx m;
      guMtxIdent(&m);
      gSPMatrix(line_p++,
                gDisplayListData(&line_d, m), G_MTX_MODELVIEW | G_MTX_LOAD);
      gDPSetPrimColor(line_p++, 0, 0, 0x00, 0x00, 0x00, alpha);
    }
    /* enumerate collision polys */
    for (int i = 0; i < col_hdr->n_poly; ++i) {
      z64_col_poly_t *poly = &col_hdr->poly[i];
      z64_col_type_t *type = &col_hdr->type[poly->type];
      z64_xyz_t *va = &col_hdr->vtx[poly->va];
      z64_xyz_t *vb = &col_hdr->vtx[poly->vb];
      z64_xyz_t *vc = &col_hdr->vtx[poly->vc];
      /* generate polygon */
      _Bool skip = 0;
      if (type->flags_2.hookshot)
        gDPSetPrimColor(poly_p++, 0, 0, 0x80, 0x80, 0xFF, alpha);
      else if (type->flags_1.interaction > 0x01)
        gDPSetPrimColor(poly_p++, 0, 0, 0xC0, 0x00, 0xC0, alpha);
      else if (type->flags_1.special == 0x0C)
        gDPSetPrimColor(poly_p++, 0, 0, 0xFF, 0x00, 0x00, alpha);
      else if (type->flags_1.exit != 0x00 || type->flags_1.special == 0x05)
        gDPSetPrimColor(poly_p++, 0, 0, 0x00, 0xFF, 0x00, alpha);
      else if (type->flags_1.behavior != 0 || type->flags_2.wall_damage)
        gDPSetPrimColor(poly_p++, 0, 0, 0xC0, 0xFF, 0xC0, alpha);
      else if (type->flags_2.terrain == 0x01)
        gDPSetPrimColor(poly_p++, 0, 0, 0xFF, 0xFF, 0x80, alpha);
      else if (settings->bits.col_view_rd)
        skip = 1;
      else
        gDPSetPrimColor(poly_p++, 0, 0, 0xFF, 0xFF, 0xFF, alpha);
      if (!skip) {
        Vtx pvg[3] =
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
        gSPVertex(poly_p++, gDisplayListData(&poly_d, pvg), 3, 0);
        gSP1Triangle(poly_p++, 0, 1, 2, 0);
      }
      /* generate lines */
      if (settings->bits.col_view_line) {
        struct line lab = {poly->va, poly->vb};
        struct line lbc = {poly->vb, poly->vc};
        struct line lca = {poly->vc, poly->va};
        _Bool ab = 1;
        _Bool bc = 1;
        _Bool ca = 1;
        for (int i = 0; i < line_set.size; ++i) {
          struct line *l = vector_at(&line_set, i);
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
        if (!ab && !bc && !ca)
          continue;
        Vtx lvg[3] =
        {
          gdSPDefVtxC(va->x, va->y, va->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
          gdSPDefVtxC(vb->x, vb->y, vb->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
          gdSPDefVtxC(vc->x, vc->y, vc->z, 0, 0, 0x00, 0x00, 0x00, 0xFF),
        };
        gSPVertex(line_p++, gDisplayListData(&line_d, lvg), 3, 0);
        if (ab) {
          vector_push_back(&line_set, 1, &lab);
          gSPLine3D(line_p++, 0, 1, 0);
        }
        if (bc) {
          vector_push_back(&line_set, 1, &lbc);
          gSPLine3D(line_p++, 1, 2, 0);
        }
        if (ca) {
          vector_push_back(&line_set, 1, &lca);
          gSPLine3D(line_p++, 2, 0, 0);
        }
      }
    }
    /* finalize dlists */
    gSPEndDisplayList(poly_p++);
    if (settings->bits.col_view_line) {
      vector_destroy(&line_set);
      unload_l3dex2(&line_p, 1);
      gSPEndDisplayList(line_p++);
    }
    gz.col_view_state = 2;
  }
  if (gz.col_view_state == 2 && z64_game.pause_ctxt.state == 0) {
    if (xlu) {
      if (poly_disp)
        gSPDisplayList(z64_ctxt.gfx->poly_xlu.p++, poly_disp);
      if (line_disp)
        gSPDisplayList(z64_ctxt.gfx->poly_xlu.p++, line_disp);
    }
    else {
      if (poly_disp)
        gSPDisplayList(z64_ctxt.gfx->poly_opa.p++, poly_disp);
      if (line_disp)
        gSPDisplayList(z64_ctxt.gfx->poly_opa.p++, line_disp);
    }
  }
  if (gz.col_view_state == 3)
    gz.col_view_state = 4;
  else if (gz.col_view_state == 4) {
    if (poly_disp) {
      free(poly_disp);
      poly_disp = NULL;
    }
    if (line_disp) {
      free(line_disp);
      line_disp = NULL;
    }
    gz.col_view_state = 0;
  }
}
