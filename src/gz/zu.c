#include <stdlib.h>
#include <mips.h>
#include <n64.h>
#include <vector/vector.h>
#include "gu.h"
#include "z64.h"
#include "zu.h"

void *zu_seg_locate(const z64_stab_t *stab, uint32_t seg_addr)
{
  uint8_t   seg   = (seg_addr >> 24) & 0x0000000F;
  uint32_t  off   = (seg_addr >> 0)  & 0x00FFFFFF;
  uint32_t  phys  = stab->seg[seg] + off;
  if (!phys)
    return NULL;
  return (void*)MIPS_PHYS_TO_KSEG0(phys);
}

void *zu_zseg_locate(uint32_t seg_addr)
{
  return zu_seg_locate(&z64_stab, seg_addr);
}

void zu_getfile(uint32_t vrom_addr, void *dram_addr, size_t size)
{
  OSMesgQueue   notify_mq;
  OSMesg        notify_m;
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
                    int *no_rooms, const z64_stab_t *stab)
{
  if (no_rooms)
    *no_rooms = 0;
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
        if (no_rooms)
          *no_rooms = room_list_size;
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
          uint8_t   no_entries;
          uint32_t  start;
          uint32_t  end;
        } *mesh_header = zu_seg_locate(stab, c_lo);
        uint32_t *dl = zu_seg_locate(stab, mesh_header->start);
        for (int i = 0; i < mesh_header->no_entries; ++i) {
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
    v->v.ob[0] = x*mf->xx+y*mf->yx+z*mf->zx+mf->wx;
    v->v.ob[1] = x*mf->xy+y*mf->yy+z*mf->zy+mf->wy;
    v->v.ob[2] = x*mf->xz+y*mf->yz+z*mf->zz+mf->wz;
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
