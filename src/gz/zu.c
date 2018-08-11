#include <stdlib.h>
#include <mips.h>
#include <n64.h>
#include <vector/vector.h>
#include "gu.h"
#include "z64.h"
#include "zu.h"

static const size_t work_length     = 0x0080;
static const size_t poly_opa_length = 0x17E0;
static const size_t poly_xlu_length = 0x0800;
static const size_t overlay_length  = 0x0400;

void *zu_seg_locate(const z64_stab_t *stab, uint32_t seg_addr)
{
  uint8_t seg = (seg_addr >> 24) & 0x0000000F;
  uint32_t off = (seg_addr >> 0) & 0x00FFFFFF;
  uint32_t phys = stab->seg[seg] + off;
  if (!phys)
    return NULL;
  return (void*)MIPS_PHYS_TO_KSEG0(phys);
}

void *zu_zseg_locate(uint32_t seg_addr)
{
  return zu_seg_locate(&z64_stab, seg_addr);
}

void *zu_seg_relocate(void *p_seg_addr, const z64_stab_t *stab)
{
  uint32_t *p_seg_addr_u32 = p_seg_addr;
  uint32_t seg_addr = *p_seg_addr_u32;
  uint8_t seg = (seg_addr >> 24) & 0x0000000F;
  uint32_t off = (seg_addr >> 0) & 0x00FFFFFF;
  return (void*)(*p_seg_addr_u32 = MIPS_PHYS_TO_KSEG0(stab->seg[seg] + off));
}

void *zu_zseg_relocate(void *p_seg_addr)
{
  return zu_seg_relocate(p_seg_addr, &z64_stab);
}

void zu_getfile(uint32_t vrom_addr, void *dram_addr, size_t size)
{
  OSMesgQueue notify_mq;
  OSMesg notify_m;
  z64_osCreateMesgQueue(&notify_mq, &notify_m, 1);
  z64_getfile_t f =
  {
    vrom_addr,
    dram_addr,
    size,
    NULL, 0, 0,
    &notify_mq, 0,
  };
  z64_osSendMesg(&z64_file_mq, &f, OS_MESG_NOBLOCK);
  z64_osRecvMesg(&notify_mq, NULL, OS_MESG_BLOCK);
}

void zu_getfile_idx(int file_idx, void *dram_addr)
{
  z64_ftab_t *file = &z64_ftab[file_idx];
  OSMesgQueue notify_mq;
  OSMesg notify_m;
  z64_osCreateMesgQueue(&notify_mq, &notify_m, 1);
  z64_getfile_t f =
  {
    file->vrom_start,
    dram_addr,
    file->vrom_end - file->vrom_start,
    NULL, 0, 0,
    &notify_mq, 0,
  };
  z64_osSendMesg(&z64_file_mq, &f, OS_MESG_NOBLOCK);
  z64_osRecvMesg(&notify_mq, NULL, OS_MESG_BLOCK);
}

void *zu_sr_header(void *sr, int header_index, const z64_stab_t *stab)
{
  void *header = sr;
  uint32_t *sr_command = sr;
  _Bool eof = 0;
  if (header_index > 0) {
    while (!eof) {
      uint32_t c_hi = *sr_command++;
      uint32_t c_lo = *sr_command++;
      switch ((c_hi >> 24) & 0x000000FF) {
        case 0x14: {
          eof = 1;
          break;
        }
        case 0x18: {
          uint32_t *ah_list = zu_seg_locate(stab, c_lo);
          for (int i = 0; i < header_index; ++i, ++ah_list)
            if (*ah_list != 0)
              header = zu_seg_locate(stab, *ah_list);
          eof = 1;
          break;
        }
      }
    }
  }
  return header;
}

void zu_scene_rooms(const void *scene, struct zu_file *ftab, int ftab_size,
                    int *n_rooms, const z64_stab_t *stab)
{
  if (n_rooms)
    *n_rooms = 0;
  const uint32_t *scene_command = scene;
  _Bool eof = 0;
  while (!eof) {
    uint32_t c_hi = *scene_command++;
    uint32_t c_lo = *scene_command++;
    switch ((c_hi >> 24) & 0x000000FF) {
      case 0x14: {
        eof = 1;
        break;
      }
      case 0x04: {
        int room_list_size = (c_hi >> 16) & 0x000000FF;
        if (n_rooms)
          *n_rooms = room_list_size;
        uint32_t *room_list = zu_seg_locate(stab, c_lo);
        for (int i = 0; i < room_list_size && i < ftab_size; ++i) {
          ftab[i].vrom_start = *room_list++;
          ftab[i].vrom_end = *room_list++;
        }
        eof = 1;
        break;
      }
    }
  }
}

void zu_room_mesh(const void *room, struct zu_mesh *mesh,
                  const z64_stab_t *stab)
{
  struct vector entries[ZU_MESH_TYPES];
  for (int i = 0; i < 4; ++i)
    vector_init(&entries[i], sizeof(uint32_t));
  const uint32_t *room_command = room;
  _Bool eof = 0;
  while (!eof) {
    uint32_t c_hi = *room_command++;
    uint32_t c_lo = *room_command++;
    switch ((c_hi >> 24) & 0x000000FF) {
      case 0x14: {
        eof = 1;
        break;
      }
      case 0x0A: {
        struct
        {
          uint8_t   type;
          uint8_t   n_entries;
          uint32_t  start;
          uint32_t  end;
        } *mesh_header = zu_seg_locate(stab, c_lo);
        uint32_t *dl = zu_seg_locate(stab, mesh_header->start);
        for (int i = 0; i < mesh_header->n_entries; ++i) {
          if (mesh_header->type == 0x02) {
            dl += 2;
            uint32_t near_dl = *dl++;
            if (near_dl)
              vector_push_back(&entries[ZU_MESH_NEAR], 1, &near_dl);
            uint32_t far_dl = *dl++;
            if (far_dl)
              vector_push_back(&entries[ZU_MESH_FAR], 1, &far_dl);
          }
          else {
            uint32_t opa_dl = *dl++;
            if (opa_dl)
              vector_push_back(&entries[ZU_MESH_OPA], 1, &opa_dl);
            uint32_t xlu_dl = *dl++;
            if (xlu_dl)
              vector_push_back(&entries[ZU_MESH_XLU], 1, &xlu_dl);
          }
        }
        break;
      }
    }
  }
  for (int i = 0; i < ZU_MESH_TYPES; ++i) {
    vector_shrink_to_fit(&entries[i]);
    mesh->all[i].size = entries[i].size;
    mesh->all[i].dlists = vector_release(&entries[i]);
  }
}

void zu_mesh_destroy(struct zu_mesh *mesh)
{
  for (int i = 0; i < ZU_MESH_TYPES; ++i)
    if (mesh->all[i].dlists)
      free(mesh->all[i].dlists);
}

void zu_vlist_init(struct zu_vlist *vlist)
{
  vector_init(&vlist->v, sizeof(Vtx*));
}

void zu_vlist_add_dl(struct zu_vlist *vlist,
                     const z64_stab_t *stab, const Gfx *dl)
{
  static z64_stab_t t_stab;
  static uint32_t rdphalf_1;
  if (stab)
    t_stab = *stab;
  _Bool eof = 0;
  while (!eof) {
    switch ((dl->hi >> 24) & 0x000000FF) {
      case G_MOVEWORD: {
        uint8_t index = (dl->hi >> 16) & 0x000000FF;
        if (index == G_MW_SEGMENT) {
          uint16_t seg = ((dl->hi >> 0) & 0x000000FF) / 4;
          t_stab.seg[seg] = MIPS_KSEG0_TO_PHYS(dl->lo);
        }
        break;
      }
      case G_MTX: {
        /* don't grab transformed vertices */
        eof = 1;
        break;
      }
      case G_VTX: {
        uint8_t vn = (dl->hi >> 12) & 0x000000FF;
        Vtx *v = zu_seg_locate(&t_stab, dl->lo);
        while (vn--) {
          _Bool found = 0;
          for (size_t i = 0; i < vlist->v.size; ++i) {
            Vtx **sv = vector_at(&vlist->v, i);
            if (*sv == v) {
              found = 1;
              break;
            }
          }
          if (!found)
            vector_push_back(&vlist->v, 1, &v);
          ++v;
        }
        break;
      }
      case G_BRANCH_Z: {
        zu_vlist_add_dl(vlist, NULL, zu_seg_locate(&t_stab, rdphalf_1));
        break;
      }
      case G_RDPHALF_1: {
        rdphalf_1 = dl->lo;
        break;
      }
      case G_DL: {
        zu_vlist_add_dl(vlist, NULL, zu_seg_locate(&t_stab, dl->lo));
        break;
      }
      case G_ENDDL: {
        eof = 1;
        break;
      }
    }
    ++dl;
  }
}

void zu_vlist_bbox(const struct zu_vlist *vlist, struct zu_bbox *bbox)
{
  bbox->x1 = bbox->x2 = bbox->y1 = bbox->y2 = bbox->z1 = bbox->z2 = 0.f;
  for (size_t i = 0; i < vlist->v.size; ++i) {
    Vtx **p_v = vector_at(&vlist->v, i);
    Vtx *v = *p_v;
    float x = v->v.ob[0];
    float y = v->v.ob[1];
    float z = v->v.ob[2];
    if (i == 0) {
      bbox->x1 = bbox->x2 = x;
      bbox->y1 = bbox->y2 = y;
      bbox->z1 = bbox->z2 = z;
    }
    else {
      if (x < bbox->x1) bbox->x1 = x;
      if (x > bbox->x2) bbox->x2 = x;
      if (y < bbox->y1) bbox->y1 = y;
      if (y > bbox->y2) bbox->y2 = y;
      if (z < bbox->z1) bbox->z1 = z;
      if (z > bbox->z2) bbox->z2 = z;
    }
  }
}

void zu_vlist_transform(const struct zu_vlist *vlist, const MtxF *mf)
{
  for (size_t i = 0; i < vlist->v.size; ++i) {
    Vtx **p_v = vector_at(&vlist->v, i);
    Vtx *v = *p_v;
    float x = v->v.ob[0];
    float y = v->v.ob[1];
    float z = v->v.ob[2];
    v->v.ob[0] = x * mf->xx + y * mf->yx + z * mf->zx + mf->wx;
    v->v.ob[1] = x * mf->xy + y * mf->yy + z * mf->zy + mf->wy;
    v->v.ob[2] = x * mf->xz + y * mf->yz + z * mf->zz + mf->wz;
  }
}

void zu_vlist_destroy(struct zu_vlist *vlist)
{
  vector_destroy(&vlist->v);
}

void zu_sram_read(void *dram_addr, uint32_t sram_addr, size_t size)
{
  z64_Io(0x08000000 + sram_addr, dram_addr, size, OS_READ);
}

void zu_sram_write(void *dram_addr, uint32_t sram_addr, size_t size)
{
  z64_Io(0x08000000 + sram_addr, dram_addr, size, OS_WRITE);
}

void zu_reset(void)
{
  volatile uint32_t *mi_intr = (void*)0xA4300008;
  volatile uint32_t *sp_status = (void*)0xA4040010;
  /* disable interrupts */
  __asm__ volatile ("mfc0 $t0, $12  \n"
                    "and  $t0, %0   \n"
                    "mtc0 $t0, $12  \n" :: "r"(~MIPS_STATUS_IE));
  /* wait for rsp to halt */
  while (!(*sp_status & 3))
    ;
  /* wait for dma busy */
  while (pi_regs.status & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY))
    ;
  /* reset pi */
  pi_regs.status = PI_STATUS_RESET | PI_STATUS_CLR_INTR;
  /* wait for vblank */
  while (!(*mi_intr & 8))
    ;
  /* copy pif code to rsp imem */
  uint32_t imem[] =
  {
    MIPS_LUI(MIPS_T5, 0xBFC0),
    MIPS_LW(MIPS_T0, 0x07FC, MIPS_T5),
    MIPS_ADDIU(MIPS_T5, MIPS_T5, 0x07C0),
    MIPS_ANDI(MIPS_T0, MIPS_T0, 0x0080),
    MIPS_BNEZL(MIPS_T0, -4 * 4),
    MIPS_LUI(MIPS_T5, 0xBFC0),
    MIPS_LW(MIPS_T0, 0x0024, MIPS_T5),
    MIPS_LUI(MIPS_T3, 0xB000),
  };
  memcpy((void*)0xA4001000, imem, sizeof(imem));
  /* copy cic boot code to rsp dmem */
  memcpy((void*)0xA4000040, (void*)0xB0000040, 0x0FC0);
  /* simulate boot from cic boot code */
  __asm__ volatile ("lui  $t0, 0x8000       \n"
                    /* osRomType (0: N64, 1: 64DD) */
                    "li   $s3, 0x0000       \n"
                    /* osTvType (0: PAL, 1: NTSC, 2: MPAL) */
                    "lw   $s4, 0x0300($t0)  \n"
                    /* osResetType (0: Cold, 1: NMI) */
                    "li   $s5, 0x0001       \n"
                    /* osCicId (3F: 6101/6102, 78: 6103, 91: 6105, 85: 6106) */
                    "li   $s6, 0x0091       \n"
                    /* osVersion */
                    "lw   $s7, 0x0314($t0)  \n"
                    /* go */
                    "la   $t3, 0xA4000040   \n"
                    "la   $sp, 0xA4001FF0   \n"
                    "la   $ra, 0xA4001550   \n"
                    "jr   $t3               \n");
}

void zu_void(void)
{
  z64_file.temp_swch_flags = z64_game.temp_swch_flags;
  z64_file.temp_collect_flags = z64_game.temp_collect_flags;
  z64_file.void_flag = 1;
  zu_execute_game(z64_file.void_entrance, 0x0000);
}

void zu_execute_game(int16_t entrance_index, uint16_t cutscene_index)
{
  if (entrance_index != z64_file.entrance_index ||
      cutscene_index != z64_file.cutscene_index)
  {
    z64_file.seq_index = -1;
    z64_file.night_sfx = -1;
    zu_audio_cmd(0x101E00FF);
    zu_audio_cmd(0x620A0000);
    zu_audio_cmd(0x620A0100);
    zu_audio_cmd(0x620A0200);
    zu_audio_cmd(0x620A0300);
    zu_audio_cmd(0x620A0400);
    zu_audio_cmd(0x620A0500);
    zu_audio_cmd(0x620A0600);
    zu_audio_cmd(0x620A0700);
    zu_audio_cmd(0x620A0800);
    zu_audio_cmd(0x620A0900);
    zu_audio_cmd(0x620A0A00);
    zu_audio_cmd(0x620A0B00);
    zu_audio_cmd(0x620A0C00);
    zu_audio_cmd(0x620A0E00);
    zu_audio_cmd(0x620A0F00);
  }
  zu_audio_cmd(0x111E00FF);
  zu_audio_cmd(0x131E00FF);
  z64_file.entrance_index = entrance_index;
  z64_file.cutscene_index = cutscene_index;
  z64_file.interface_flag = 0;
  if (z64_file.minigame_state == 1)
    z64_file.minigame_state = 3;
  z64_game.entrance_index = entrance_index;
  z64_ctxt.state_continue = 0;
  z64_ctxt.next_ctor = (void*)z64_ctxt_game_ctor;
  z64_ctxt.next_size = z64_ctxt_game_size;
}

void zu_execute_filemenu(void)
{
  z64_file.interface_flag = 0;
  z64_ctxt.state_continue = 0;
  z64_ctxt.next_ctor = (void*)z64_ctxt_filemenu_ctor;
  z64_ctxt.next_size = z64_ctxt_filemenu_size;
}

void zu_audio_cmd(uint32_t cmd)
{
  z64_audio_cmd_buf[z64_audio_cmd_write_pos++] = cmd;
}

void zu_set_event_flag(int flag_index)
{
  z64_file.event_chk_inf[flag_index / 0x10] |= (1 << (flag_index % 0x10));
}

void zu_clear_event_flag(int flag_index)
{
  z64_file.event_chk_inf[flag_index / 0x10] &= ~(1 << (flag_index % 0x10));
}

void zu_gfx_init(struct zu_gfx *gfx)
{
  /* allocate buffers */
  gfx->work.size = sizeof(Gfx) * work_length;
  gfx->work.buf = malloc(gfx->work.size);
  gfx->work_w = malloc(gfx->work.size);
  gfx->poly_opa.size = sizeof(Gfx) * poly_opa_length;
  gfx->poly_opa.buf = malloc(gfx->poly_opa.size);
  gfx->poly_opa_w = malloc(gfx->poly_opa.size);
  gfx->poly_xlu.size = sizeof(Gfx) * poly_xlu_length;
  gfx->poly_xlu.buf = malloc(gfx->poly_xlu.size);
  gfx->poly_xlu_w = malloc(gfx->poly_xlu.size);
  gfx->overlay.size = sizeof(Gfx) * overlay_length;
  gfx->overlay.buf = malloc(gfx->overlay.size);
  gfx->overlay_w = malloc(gfx->overlay.size);
  gfx->z_work.size = 0;
  gfx->z_work.buf = NULL;
  gfx->z_poly_opa.size = 0;
  gfx->z_poly_opa.buf = NULL;
  gfx->z_poly_xlu.size = 0;
  gfx->z_poly_xlu.buf = NULL;
  gfx->z_overlay.size = 0;
  gfx->z_overlay.buf = NULL;
  /* set pointers */
  gfx->work.p = &gfx->work.buf[0];
  gfx->work.d = &gfx->work.buf[work_length];
  gfx->poly_opa.p = &gfx->poly_opa.buf[0];
  gfx->poly_opa.d = &gfx->poly_opa.buf[poly_opa_length];
  gfx->poly_xlu.p = &gfx->poly_xlu.buf[0];
  gfx->poly_xlu.d = &gfx->poly_xlu.buf[poly_xlu_length];
  gfx->overlay.p = &gfx->overlay.buf[0];
  gfx->overlay.d = &gfx->overlay.buf[overlay_length];
  gfx->z_work.p = NULL;
  gfx->z_work.d = NULL;
  gfx->z_poly_opa.p = NULL;
  gfx->z_poly_opa.d = NULL;
  gfx->z_poly_xlu.p = NULL;
  gfx->z_poly_xlu.d = NULL;
  gfx->z_overlay.p = NULL;
  gfx->z_overlay.d = NULL;
}

void zu_gfx_destroy(struct zu_gfx *gfx)
{
  /* deallocate buffers */
  free(gfx->work.buf);
  free(gfx->work_w);
  free(gfx->poly_opa.buf);
  free(gfx->poly_opa_w);
  free(gfx->poly_xlu.buf);
  free(gfx->poly_xlu_w);
  free(gfx->overlay.buf);
  free(gfx->overlay_w);
}

void zu_gfx_inject(struct zu_gfx *gfx)
{
  /* save graphics context */
  gfx->z_work = z64_ctxt.gfx->work;
  gfx->z_poly_opa = z64_ctxt.gfx->poly_opa;
  gfx->z_poly_xlu = z64_ctxt.gfx->poly_xlu;
  gfx->z_overlay = z64_ctxt.gfx->overlay;
  /* inject context variables */
  z64_ctxt.gfx->work = gfx->work;
  z64_ctxt.gfx->poly_opa = gfx->poly_opa;
  z64_ctxt.gfx->poly_xlu = gfx->poly_xlu;
  z64_ctxt.gfx->overlay = gfx->overlay;
}

void zu_gfx_restore(struct zu_gfx *gfx)
{
  /* retrieve updated pointers */
  gfx->work = z64_ctxt.gfx->work;
  gfx->poly_opa = z64_ctxt.gfx->poly_opa;
  gfx->poly_xlu = z64_ctxt.gfx->poly_xlu;
  gfx->overlay = z64_ctxt.gfx->overlay;
  /* restore graphics context */
  z64_ctxt.gfx->work = gfx->z_work;
  z64_ctxt.gfx->poly_opa = gfx->z_poly_opa;
  z64_ctxt.gfx->poly_xlu = gfx->z_poly_xlu;
  z64_ctxt.gfx->overlay = gfx->z_overlay;
}

Gfx *zu_gfx_flush(struct zu_gfx *gfx)
{
  /* end dlist buffers and branch */
  gSPEndDisplayList(gfx->poly_opa.p++);
  gSPEndDisplayList(gfx->poly_xlu.p++);
  gSPEndDisplayList(gfx->overlay.p++);
  gSPDisplayList(gfx->work.p++, gfx->poly_opa.buf);
  gSPDisplayList(gfx->work.p++, gfx->poly_xlu.buf);
  gSPDisplayList(gfx->work.p++, gfx->overlay.buf);
  gSPEndDisplayList(gfx->work.p++);
  /* swap buffers */
  Gfx *p;
  p = gfx->work.buf;
  gfx->work.buf = gfx->work_w;
  gfx->work_w = p;
  p = gfx->poly_opa.buf;
  gfx->poly_opa.buf = gfx->poly_opa_w;
  gfx->poly_opa_w = p;
  p = gfx->poly_xlu.buf;
  gfx->poly_xlu.buf = gfx->poly_xlu_w;
  gfx->poly_xlu_w = p;
  p = gfx->overlay.buf;
  gfx->overlay.buf = gfx->overlay_w;
  gfx->overlay_w = p;
  /* set pointers */
  gfx->work.p = &gfx->work.buf[0];
  gfx->work.d = &gfx->work.buf[work_length];
  gfx->poly_opa.p = &gfx->poly_opa.buf[0];
  gfx->poly_opa.d = &gfx->poly_opa.buf[poly_opa_length];
  gfx->poly_xlu.p = &gfx->poly_xlu.buf[0];
  gfx->poly_xlu.d = &gfx->poly_xlu.buf[poly_xlu_length];
  gfx->overlay.p = &gfx->overlay.buf[0];
  gfx->overlay.d = &gfx->overlay.buf[overlay_length];
  /* flush */
  return gfx->work_w;
}

/* relocate the commands in the current display list buffer */
void zu_reloc_gfx(int src_gfx_idx, int src_cimg_idx)
{
  z64_gfx_t *gfx = z64_ctxt.gfx;
  z64_disp_buf_t *z_disp[4] =
  {
    &gfx->work,
    &gfx->poly_opa,
    &gfx->poly_xlu,
    &gfx->overlay,
  };
  uint32_t src_gfx = z64_disp_addr + src_gfx_idx * z64_disp_size;
  uint32_t dst_gfx = z64_disp_addr + (gfx->frame_count_1 & 1) * z64_disp_size;
  uint32_t src_cimg = z64_cimg_addr + src_cimg_idx * z64_cimg_size;
  uint32_t dst_cimg = z64_cimg_addr + (gfx->frame_count_2 & 1) * z64_cimg_size;
  for (int i = 0; i < sizeof(z_disp) / sizeof(*z_disp); ++i) {
    z64_disp_buf_t *disp = z_disp[i];
    for (Gfx *p = disp->buf; p != disp->p; ++p) {
      /* check gbi commands with addresses in the low word */
      switch (p->hi >> 24) {
        case G_VTX:             break;
        case G_DMA_IO:          break;
        case G_MTX:             break;
        case G_MOVEWORD:
          switch ((p->hi >> 16) & 0xFF) {
            case G_MW_SEGMENT:  break;
            default:            continue;
          }                     break;
        case G_MOVEMEM:         break;
        case G_LOAD_UCODE:      break;
        case G_DL:              break;
        case G_RDPHALF_1:
          switch (p[1].hi >> 24) {
            case G_BRANCH_Z:    break;
            case G_LOAD_UCODE:  break;
            default:            continue;
          }                     break;
        case G_SETTIMG:         break;
        case G_SETZIMG:         break;
        case G_SETCIMG:         break;
        default:                continue;
      }
      /* relocate previous gfx -> current gfx, previous cimg -> current cimg */
      if (p->lo >= src_gfx && p->lo < src_gfx + z64_disp_size)
        p->lo += dst_gfx - src_gfx;
      else if (p->lo >= src_cimg && p->lo < src_cimg + z64_cimg_size)
        p->lo += dst_cimg - src_cimg;
    }
  }
}

void zu_save_disp_p(struct zu_disp_p *disp_p)
{
  z64_gfx_t *gfx = z64_ctxt.gfx;
  disp_p->work_p = gfx->work.p - gfx->work.buf;
  disp_p->work_d = gfx->work.d - gfx->work.buf;
  disp_p->poly_opa_p = gfx->poly_opa.p - gfx->poly_opa.buf;
  disp_p->poly_opa_d = gfx->poly_opa.d - gfx->poly_opa.buf;
  disp_p->poly_xlu_p = gfx->poly_xlu.p - gfx->poly_xlu.buf;
  disp_p->poly_xlu_d = gfx->poly_xlu.d - gfx->poly_xlu.buf;
  disp_p->overlay_p = gfx->overlay.p - gfx->overlay.buf;
  disp_p->overlay_d = gfx->overlay.d - gfx->overlay.buf;
}

void zu_load_disp_p(struct zu_disp_p *disp_p)
{
  z64_gfx_t *gfx = z64_ctxt.gfx;
  gfx->work.p = gfx->work.buf + disp_p->work_p;
  gfx->work.d = gfx->work.buf + disp_p->work_d;
  gfx->poly_opa.p = gfx->poly_opa.buf + disp_p->poly_opa_p;
  gfx->poly_opa.d = gfx->poly_opa.buf + disp_p->poly_opa_d;
  gfx->poly_xlu.p = gfx->poly_xlu.buf + disp_p->poly_xlu_p;
  gfx->poly_xlu.d = gfx->poly_xlu.buf + disp_p->poly_xlu_d;
  gfx->overlay.p = gfx->overlay.buf + disp_p->overlay_p;
  gfx->overlay.d = gfx->overlay.buf + disp_p->overlay_d;
}
