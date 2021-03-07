#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mips.h>
#include <n64.h>
#include <set/set.h>
#include "gz.h"
#include "state.h"
#include "sys.h"
#include "yaz0.h"
#include "zu.h"
#include "z64.h"

static void serial_write(void **p, void *data, uint32_t length)
{
  char *cp = *p;
  memcpy(cp, data, length);
  cp += length;
  *p = cp;
}

static void serial_read(void **p, void *data, uint32_t length)
{
  char *cp = *p;
  memcpy(data, cp, length);
  cp += length;
  *p = cp;
}

static void serial_skip(void **p, uint32_t length)
{
  char *cp = *p;
  cp += length;
  *p = cp;
}

static void save_ovl(void **p, void *addr,
                     uint32_t vrom_start, uint32_t vrom_end)
{
  /* locate file table entry */
  z64_ftab_t *file = NULL;
  for (int i = 0; ; ++i) {
    z64_ftab_t *f = &z64_ftab[i];
    if (f->vrom_start == vrom_start && f->vrom_end == vrom_end) {
      file = f;
      break;
    }
    if (f->vrom_end == 0)
      return;
  }
  /* save overlay address */
  serial_write(p, &addr, sizeof(addr));
  /* compute segment addresses */
  char *start = addr;
  char *end = start + (vrom_end - vrom_start);
  uint32_t *hdr_off = (void*)(end - sizeof(*hdr_off));
  if (*hdr_off == 0)
    return;
  z64_ovl_hdr_t *hdr;
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
  hdr = (void*)(end - *hdr_off);
#elif Z64_VERSION == Z64_OOTMQJ || \
      Z64_VERSION == Z64_OOTMQU || \
      Z64_VERSION == Z64_OOTGCJ || \
      Z64_VERSION == Z64_OOTGCU || \
      Z64_VERSION == Z64_OOTCEJ
  z64_ovl_hdr_t l_hdr;
  hdr = &l_hdr;
  yaz0_begin(file->prom_start);
  yaz0_advance(end - *hdr_off - start);
  yaz0_read(hdr, sizeof(*hdr));
  serial_write(p, hdr, sizeof(*hdr));
#endif
  char *data = start + hdr->text_size;
  char *bss = end;
  /* save data segment */
  if (hdr->data_size > 0) {
    yaz0_begin(file->prom_start);
    yaz0_advance(hdr->text_size);
  }
  uint16_t n_copy = 0;
  uint16_t n_save = 0;
  char *save_data = NULL;
  for (uint32_t i = 0; i < hdr->data_size; ++i) {
    if (yaz0_get_byte() == data[i]) {
      if (n_save > 0) {
        serial_write(p, &n_copy, sizeof(n_copy));
        serial_write(p, &n_save, sizeof(n_save));
        serial_write(p, save_data, n_save);
        n_copy = 0;
        n_save = 0;
      }
      ++n_copy;
    }
    else {
      if (n_save == 0)
        save_data = &data[i];
      ++n_save;
    }
  }
  if (n_copy > 0 || n_save > 0) {
    serial_write(p, &n_copy, sizeof(n_copy));
    serial_write(p, &n_save, sizeof(n_save));
    serial_write(p, save_data, n_save);
  }
  /* save bss segment */
  serial_write(p, bss, hdr->bss_size);
}

static void load_ovl(void **p, void **p_addr,
                     uint32_t vrom_start, uint32_t vrom_end,
                     uint32_t vram_start, uint32_t vram_end)
{
  /* locate file table entry */
  z64_ftab_t *file = NULL;
  for (int i = 0; ; ++i) {
    z64_ftab_t *f = &z64_ftab[i];
    if (f->vrom_start == vrom_start && f->vrom_end == vrom_end) {
      file = f;
      break;
    }
    if (f->vrom_end == 0)
      return;
  }
  /* load overlay address */
  void *load_addr;
  serial_read(p, &load_addr, sizeof(load_addr));
  /* load overlay */
  void *addr = *p_addr;
  if (addr != load_addr) {
    addr = load_addr;
    *p_addr = addr;
    z64_LoadOverlay(vrom_start, vrom_end, vram_start, vram_end, addr);
  }
  /* compute segment addresses */
  char *start = addr;
  char *end = start + (vrom_end - vrom_start);
  uint32_t *hdr_off = (void*)(end - sizeof(*hdr_off));
  if (*hdr_off == 0)
    return;
  z64_ovl_hdr_t *hdr;
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
  hdr = (void*)(end - *hdr_off);
#elif Z64_VERSION == Z64_OOTMQJ || \
      Z64_VERSION == Z64_OOTMQU || \
      Z64_VERSION == Z64_OOTGCJ || \
      Z64_VERSION == Z64_OOTGCU || \
      Z64_VERSION == Z64_OOTCEJ
  z64_ovl_hdr_t l_hdr;
  hdr = &l_hdr;
  serial_read(p, hdr, sizeof(*hdr));
#endif
  char *data = start + hdr->text_size;
  char *bss = end;
  /* restore data segment */
  if (hdr->data_size > 0) {
    yaz0_begin(file->prom_start);
    yaz0_advance(hdr->text_size);
  }
  for (uint32_t i = 0; i < hdr->data_size; ) {
    uint16_t n_copy = 0;
    uint16_t n_save = 0;
    serial_read(p, &n_copy, sizeof(n_copy));
    serial_read(p, &n_save, sizeof(n_save));
    yaz0_read(&data[i], n_copy);
    i += n_copy;
    serial_read(p, &data[i], n_save);
    i += n_save;
    if (i < hdr->data_size)
      yaz0_advance(n_save);
  }
  /* restore bss segment */
  serial_read(p, bss, hdr->bss_size);
}

static void reloc_col_hdr(uint32_t seg_addr)
{
  z64_col_hdr_t *col_hdr = zu_zseg_locate(seg_addr);
  zu_zseg_relocate(&col_hdr->vtx);
  zu_zseg_relocate(&col_hdr->poly);
  zu_zseg_relocate(&col_hdr->type);
  zu_zseg_relocate(&col_hdr->camera);
  zu_zseg_relocate(&col_hdr->water);
}

static void load_sky_image(void)
{
  z64_sky_ctxt_t *sky_ctxt = &z64_game.sky_ctxt;
  switch (z64_game.skybox_type) {
    case 0x0001: {
      z64_sky_image_t *image[2] = {NULL, NULL};
      uint8_t *image_idx = z64_game.sky_image_idx;
      uint32_t file_size;
      if (image_idx[0] != 0x63) {
        image[0] = &z64_sky_images[image_idx[0]];
        file_size = image[0]->tex_end - image[0]->tex_start;
        zu_getfile(image[0]->tex_start, sky_ctxt->textures[0], file_size);
      }
      if (image_idx[1] != 0x63) {
        image[1] = &z64_sky_images[image_idx[1]];
        file_size = image[1]->tex_end - image[1]->tex_start;
        zu_getfile(image[1]->tex_start, sky_ctxt->textures[1], file_size);
      }
      if ((image_idx[0] & 1) == (image_idx[0] & 4) >> 2) {
        if (image_idx[1] != 0x63)
          zu_getfile(image[1]->pal_start, sky_ctxt->palettes, 0x0100);
        if (image_idx[0] != 0x63)
          zu_getfile(image[0]->pal_start, sky_ctxt->palettes + 0x0100, 0x0100);
      }
      else {
        if (image_idx[0] != 0x63)
          zu_getfile(image[0]->pal_start, sky_ctxt->palettes, 0x0100);
        if (image_idx[1] != 0x63)
          zu_getfile(image[1]->pal_start, sky_ctxt->palettes + 0x0100, 0x0100);
      }
      break;
    }

    case 0x0002:
      /* bazaar */
      zu_getfile_idx(z64_vr_SP1a_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_SP1a_pal_static, sky_ctxt->palettes);
      break;

    case 0x0003:
      /* overcast sunset */
      zu_getfile_idx(z64_vr_cloud2_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_cloud2_static, sky_ctxt->textures[1]);
      zu_getfile_idx(z64_vr_cloud2_pal_static, sky_ctxt->palettes);
      zu_getfile_idx(z64_vr_cloud2_pal_static, sky_ctxt->palettes + 0x100);
      break;

    case 0x0004:
      /* market ruins */
      zu_getfile_idx(z64_vr_RUVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_RUVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0005:
      /* cutscene map */
      zu_getfile_idx(z64_vr_holy0_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_holy1_static, sky_ctxt->textures[1]);
      zu_getfile_idx(z64_vr_holy0_pal_static, sky_ctxt->palettes);
      zu_getfile_idx(z64_vr_holy1_pal_static, sky_ctxt->palettes + 0x100);
      break;

    case 0x0007:
      /* link's house */
      zu_getfile_idx(z64_vr_LHVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_LHVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0009:
      /* market day */
      zu_getfile_idx(z64_vr_MDVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_MDVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x000A:
      /* market night */
      zu_getfile_idx(z64_vr_MNVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_MNVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x000B:
      /* happy mask shop */
      zu_getfile_idx(z64_vr_FCVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_FCVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x000C:
      /* know-it-all brothers' house */
      zu_getfile_idx(z64_vr_KHVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_KHVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x000E:
      /* house of twins */
      zu_getfile_idx(z64_vr_K3VR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_K3VR_pal_static, sky_ctxt->palettes);
      break;

    case 0x000F:
      /* stable */
      zu_getfile_idx(z64_vr_MLVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_MLVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0010:
      /* carpenter's house */
      zu_getfile_idx(z64_vr_KKRVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_KKRVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0011:
      /* kokiri shop */
      zu_getfile_idx(z64_vr_KSVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_KSVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0013:
      /* goron shop */
      zu_getfile_idx(z64_vr_GLVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_GLVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0014:
      /* zora shop */
      zu_getfile_idx(z64_vr_ZRVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_ZRVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0016:
      /* kakariko potion shop */
      zu_getfile_idx(z64_vr_DGVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_DGVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0017:
      /* market potion shop */
      zu_getfile_idx(z64_vr_ALVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_ALVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0018:
      /* bombchu shop */
      zu_getfile_idx(z64_vr_NSVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_NSVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x001A:
      /* richard's house */
      zu_getfile_idx(z64_vr_IPVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_IPVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x001B:
      /* impa's house */
      zu_getfile_idx(z64_vr_LBVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_LBVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x001C:
      /* carpenter's tent */
      zu_getfile_idx(z64_vr_TTVR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_TTVR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0020:
      /* mido's house */
      zu_getfile_idx(z64_vr_K4VR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_K4VR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0021:
      /* saria's house */
      zu_getfile_idx(z64_vr_K5VR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_K5VR_pal_static, sky_ctxt->palettes);
      break;

    case 0x0022:
      /* guy's house */
      zu_getfile_idx(z64_vr_KR3VR_static, sky_ctxt->textures[0]);
      zu_getfile_idx(z64_vr_KR3VR_pal_static, sky_ctxt->palettes);
      break;
  }
}

static void play_night_sfx(void)
{
  _Bool rain_effect = z64_game.rain_effect_1 || z64_game.rain_effect_2;
  uint8_t day_phase = z64_game.day_phase;
  uint8_t night_sfx_idx = z64_game.night_sfx;
  if (rain_effect && night_sfx_idx == 0x13)
    night_sfx_idx = 0x05;

  z64_night_sfx_t *night_sfx = &z64_night_sfx[night_sfx_idx];
  /* set sequencer parameters */
  z64_AfxCmdW(0x46000000, 0x01000000);
  z64_AfxCmdW(0x46000004, (night_sfx->channel_enable << 16) & 0xFF000000);
  z64_AfxCmdW(0x46000005, (night_sfx->channel_enable << 24) & 0xFF000000);
  /* play night sequence */
  z64_seq_ctl_t *sc = &z64_seq_ctl[0];
  sc->seq_idx = 0x0001;
  sc->prev_seq_idx = 0x0001;
  z64_AfxCmdW(0x82000100, 0x00000000);
  /* enable channels */
  for (int8_t i = 0; i < 0x10; ++i) {
    uint16_t bit = 1 << i;
    if ((night_sfx->channel_enable & bit) && !(night_sfx->channel_mask & bit))
      z64_AfxCmdW(0x06000001 | (i << 8), 0x01000000);
  }
  /* set channel parameters */
  for (int i = 0; i < 0x64; i += 3) {
    uint8_t c = night_sfx->params[i + 0];
    if (c == 0xFF)
      break;
    uint8_t p = night_sfx->params[i + 1];
    uint8_t v = night_sfx->params[i + 2];
    z64_AfxCmdW(0x06000000 | (c << 8) | (p << 0), v << 24);
  }
  z64_AfxCmdW(0x06000D07, 0x00000000);
  /* set day phase channel parameters */
  if (day_phase != 0xFF) {
    if (day_phase == 4 || day_phase == 5) {
      /* channel 1 on */
      z64_AfxCmdW(0x06000101, 0x01000000);
    }
    if ((day_phase == 6 || day_phase == 7) && !rain_effect) {
      /* channel 2, 3, 4 on */
      z64_AfxCmdW(0x06000201, 0x01000000);
      z64_AfxCmdW(0x06000301, 0x01000000);
      z64_AfxCmdW(0x06000401, 0x01000000);
    }
    if ((day_phase == 8 || day_phase == 0) && !rain_effect) {
      /* channel 5, 6 on */
      z64_AfxCmdW(0x06000501, 0x01000000);
      z64_AfxCmdW(0x06000601, 0x01000000);
    }
  }
  if (rain_effect) {
    /* channel 14, 15 on */
    z64_AfxCmdW(0x06000E01, 0x01000000);
    z64_AfxCmdW(0x06000F01, 0x01000000);
  }
}

static void grayscale_texture(uint32_t *pixels, uint16_t n_pixels)
{
  for (int i = 0; i < n_pixels; ++i) {
    uint32_t c = pixels[i];
    uint8_t s = (((c & 0xFF000000) >> 24) +
                 ((c & 0x00FF0000) >> 15) +
                 ((c & 0x0000FF00) >> 8)) / 7;
    pixels[i] = (s << 24) | (s << 16) | (s << 8) | (c & 0x000000FF);
  }
}

static _Bool addr_comp(void *a, void *b)
{
  uint32_t *a_u32 = a;
  uint32_t *b_u32 = b;
  return *a_u32 < *b_u32;
}

uint32_t save_state(void *state)
{
  void *p = state;

  /* allocate metadata */
  serial_skip(&p, sizeof(struct state_meta));

  /* save sequencer info */
  for (int i = 0; i < 4; ++i) {
    z64_seq_ctl_t *sc = &z64_seq_ctl[i];
    char *seq = &z64_afx[0x3530 + i * 0x0160];
    _Bool seq_active = (*(uint8_t*)(seq) & 0x80) || z64_afx_config_busy;
    serial_write(&p, &seq_active, sizeof(seq_active));
    if (seq_active) {
      serial_write(&p, &sc->stop_cmd_timer, sizeof(sc->stop_cmd_timer));
      serial_write(&p, &sc->stop_cmd_count, sizeof(sc->stop_cmd_count));
      serial_write(&p, &sc->stop_cmd_buf,
                   sizeof(*sc->stop_cmd_buf) * sc->stop_cmd_count);
    }
    serial_write(&p, &sc->seq_idx, sizeof(sc->seq_idx));
    if (sc->vs_time != 0)
      serial_write(&p, &sc->vs_target, sizeof(sc->vs_target));
    else
      serial_write(&p, &sc->vs_current, sizeof(sc->vs_current));
    serial_write(&p, &sc->vp_factors, sizeof(sc->vp_factors));
    uint16_t seq_ch_mute = 0;
    for (int j = 0; j < 16; ++j) {
      char *ch = *(void**)(seq + 0x0038 + j * 0x0004);
      _Bool ch_mute = *(uint8_t*)(ch) & 0x10;
      seq_ch_mute |= ch_mute << j;
    }
    serial_write(&p, &seq_ch_mute, sizeof(seq_ch_mute));
  }

  /* save afx config */
  serial_write(&p, &z64_afx_cfg, sizeof(z64_afx_cfg));

  int16_t sot = 0;
  int16_t eot = -1;
  /* save context */
  serial_write(&p, &z64_game, sizeof(z64_game));
  serial_write(&p, &z64_file, sizeof(z64_file));
  serial_write(&p, z64_file.gameinfo, sizeof(*z64_file.gameinfo));
  /* save overlays */
  int16_t n_ovl;
  struct set ovl_nodes;
  set_init(&ovl_nodes, sizeof(uint32_t), addr_comp);
  /* actor overlays */
  n_ovl = sizeof(z64_actor_ovl_tab) / sizeof(*z64_actor_ovl_tab);
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_actor_ovl_t *ovl = &z64_actor_ovl_tab[i];
    if (ovl->ptr) {
      serial_write(&p, &i, sizeof(i));
      serial_write(&p, &ovl->n_inst, sizeof(ovl->n_inst));
      save_ovl(&p, ovl->ptr, ovl->vrom_start, ovl->vrom_end);
      set_insert(&ovl_nodes, &ovl->ptr);
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  /* play overlays */
  n_ovl = sizeof(z64_play_ovl_tab) / sizeof(*z64_play_ovl_tab);
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_play_ovl_t *ovl = &z64_play_ovl_tab[i];
    if (ovl->ptr) {
      serial_write(&p, &i, sizeof(i));
      save_ovl(&p, ovl->ptr, ovl->vrom_start, ovl->vrom_end);
      set_insert(&ovl_nodes, &ovl->ptr);
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  serial_write(&p, &z64_play_ovl_ptr, sizeof(z64_play_ovl_ptr));
  /* particle overlays */
  n_ovl = sizeof(z64_part_ovl_tab) / sizeof(*z64_part_ovl_tab);
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_part_ovl_t *ovl = &z64_part_ovl_tab[i];
    if (ovl->ptr) {
      serial_write(&p, &i, sizeof(i));
      save_ovl(&p, ovl->ptr, ovl->vrom_start, ovl->vrom_end);
      set_insert(&ovl_nodes, &ovl->ptr);
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  /* map mark overlay */
  if (z64_map_mark_ovl.ptr) {
    z64_map_mark_ovl_t *ovl = &z64_map_mark_ovl;
    serial_write(&p, &sot, sizeof(sot));
    save_ovl(&p, ovl->ptr, ovl->vrom_start, ovl->vrom_end);
    set_insert(&ovl_nodes, &ovl->ptr);
  }
  serial_write(&p, &eot, sizeof(eot));

  /* save arena nodes */
  serial_write(&p, &z64_game_arena, sizeof(z64_game_arena));
  for (z64_arena_node_t *node = z64_game_arena.first_node;
       node; node = node->next)
  {
    serial_write(&p, &sot, sizeof(sot));
    serial_write(&p, &node->free, sizeof(node->free));
    serial_write(&p, &node->size, sizeof(node->size));
    char *data = node->data;
    if (!set_get(&ovl_nodes, &data) && !node->free)
      serial_write(&p, data, node->size);
  }
  serial_write(&p, &eot, sizeof(eot));
  set_destroy(&ovl_nodes);

  /* save light queue */
  serial_write(&p, &z64_light_queue, sizeof(z64_light_queue));
  /* save matrix stack info */
  serial_write(&p, &z64_mtx_stack, sizeof(z64_mtx_stack));
  serial_write(&p, &z64_mtx_stack_top, sizeof(z64_mtx_stack_top));
  /* save segment table */
  serial_write(&p, &z64_stab, sizeof(z64_stab));

  /* save particles */
  serial_write(&p, &z64_part_space, sizeof(z64_part_space));
  serial_write(&p, &z64_part_pos, sizeof(z64_part_pos));
  serial_write(&p, &z64_part_max, sizeof(z64_part_max));
  for (int16_t i = 0; i < z64_part_max; ++i) {
    z64_part_t *part = &z64_part_space[i];
    if (part->time >= 0) {
      serial_write(&p, &i, sizeof(i));
      serial_write(&p, part, sizeof(*part));
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  /* save static particles */
  for (int16_t i = 0; i < 3; ++i) {
    z64_dot_t *dot = &z64_pfx.dots[i];
    if (dot->active) {
      serial_write(&p, &i, sizeof(i));
      serial_write(&p, dot, sizeof(*dot));
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  for (int16_t i = 0; i < 25; ++i) {
    z64_trail_t *trail = &z64_pfx.trails[i];
    if (trail->active) {
      serial_write(&p, &i, sizeof(i));
      serial_write(&p, trail, sizeof(*trail));
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  for (int16_t i = 0; i < 3; ++i) {
    z64_spark_t *spark = &z64_pfx.sparks[i];
    if (spark->active) {
      serial_write(&p, &i, sizeof(i));
      serial_write(&p, spark, sizeof(*spark));
    }
  }
  serial_write(&p, &eot, sizeof(eot));
  /* save camera shake effects */
  serial_write(&p, &z64_n_camera_shake, 0x0002);
  serial_write(&p, z64_camera_shake, 0x0090);

  /* save transition actor list (it may have been modified during gameplay) */
  {
    z64_room_ctxt_t *room_ctxt = &z64_game.room_ctxt;
    serial_write(&p, room_ctxt->tnsn_list,
                 room_ctxt->n_tnsn * sizeof(*room_ctxt->tnsn_list));
  }

  /* save waterboxes */
  {
    z64_col_hdr_t *col_hdr = z64_game.col_ctxt.col_hdr;
    serial_write(&p, &col_hdr->n_water, sizeof(col_hdr->n_water));
    serial_write(&p, col_hdr->water,
                 sizeof(*col_hdr->water) * col_hdr->n_water);
  }
  /* save dynamic collision */
  {
    z64_col_ctxt_t *col = &z64_game.col_ctxt;
    serial_write(&p, col->dyn_list,
                 col->dyn_list_max * sizeof(*col->dyn_list));
    serial_write(&p, col->dyn_poly,
                 col->dyn_poly_max * sizeof(*col->dyn_poly));
    serial_write(&p, col->dyn_vtx,
                 col->dyn_vtx_max * sizeof(*col->dyn_vtx));
  }

  if (z64_game.elf_message)
    serial_write(&p, z64_game.elf_message, 0x0070);

  /* minimap details */
  serial_write(&p, &z64_minimap_entrance_x, sizeof(z64_minimap_entrance_x));
  serial_write(&p, &z64_minimap_entrance_y, sizeof(z64_minimap_entrance_y));
  serial_write(&p, &z64_minimap_entrance_r, sizeof(z64_minimap_entrance_r));

  /* weather / daytime state */
  serial_write(&p, z64_weather_state, 0x0018);
  serial_write(&p, &z64_temp_day_speed, sizeof(z64_temp_day_speed));

  /* hazard state */
  serial_write(&p, z64_hazard_state, 0x0008);

  /* timer state */
  serial_write(&p, z64_timer_state, 0x0008);

  /* hud state */
  serial_write(&p, z64_hud_state, 0x0008);

  /* letterboxing */
  serial_write(&p, &z64_letterbox_target, sizeof(z64_letterbox_target));
  serial_write(&p, &z64_letterbox_current, sizeof(z64_letterbox_current));
  serial_write(&p, &z64_letterbox_time, sizeof(z64_letterbox_time));

  /* poly color filter state (sepia effect) */
  serial_write(&p, z64_poly_colorfilter_state, 0x001C);

  /* sound state */
  serial_write(&p, z64_sound_state, 0x004C);

  /* event state */
  serial_write(&p, z64_event_state_1, 0x0008);
  serial_write(&p, z64_event_state_2, 0x0004);
  /* event camera parameters */
  for (int i = 0; i < 24; ++i)
    serial_write(&p, &z64_event_camera[0x28 * i + 0x10], 0x0018);

  /* oob timer */
  serial_write(&p, &z64_oob_timer, sizeof(z64_oob_timer));

  /* countdown to gameover screen */
  serial_write(&p, &z64_gameover_countdown, sizeof(z64_gameover_countdown));

  /* rng */
  serial_write(&p, &z64_random, sizeof(z64_random));

  /* spell states */
  serial_write(&p, z64_dins_state_1, 0x0004);
  serial_write(&p, &z64_dins_state_2[0x0006], 0x0002);
  serial_write(&p, &z64_dins_state_2[0x0014], 0x0004);
  serial_write(&p, &z64_dins_state_2[0x0020], 0x0004);
  serial_write(&p, &z64_dins_state_2[0x003C], 0x0004);
  serial_write(&p, z64_fw_state_1, 0x0004);
  serial_write(&p, z64_fw_state_2, 0x0004);

  /* camera state */
  serial_write(&p, z64_camera_state, 0x0020);

  /* cutscene state */
  serial_write(&p, z64_cs_state, 0x0140);
  /* cutscene message id */
  serial_write(&p, z64_cs_message, 0x0008);

  /* message state */
  serial_write(&p, z64_message_state, 0x0028);

  _Bool save_gfx = 1;
  /* save display lists */
  if (save_gfx) {
    z64_gfx_t *gfx = z64_ctxt.gfx;
    serial_write(&p, &sot, sizeof(sot));
    /* save pointers */
    struct zu_disp_p disp_p;
    zu_save_disp_p(&disp_p);
    serial_write(&p, &disp_p, sizeof(disp_p));
    /* save commands and data */
    z64_disp_buf_t *z_disp[4] =
    {
      &gfx->work,
      &gfx->poly_opa,
      &gfx->poly_xlu,
      &gfx->overlay,
    };
    for (int i = 0; i < 4; ++i) {
      z64_disp_buf_t *disp_buf = z_disp[i];
      size_t s = sizeof(Gfx);
      Gfx *e = disp_buf->buf + disp_buf->size / s;
      serial_write(&p, disp_buf->buf, (disp_buf->p - disp_buf->buf) * s);
      serial_write(&p, disp_buf->d, (e - disp_buf->d) * s);
    }
    /* save counters */
    serial_write(&p, &gfx->frame_count_1,
                 sizeof(gfx->frame_count_1));
    serial_write(&p, &gfx->frame_count_2,
                 sizeof(gfx->frame_count_2));
  }
  else
    serial_write(&p, &eot, sizeof(eot));

  /* save sfx mutes */
  serial_write(&p, z64_sfx_mute, 0x0008);
  /* save pending audio commands */
  {
    uint8_t n_cmd = z64_audio_cmd_write_pos - z64_audio_cmd_read_pos;
    serial_write(&p, &n_cmd, sizeof(n_cmd));
    for (uint8_t i = z64_audio_cmd_read_pos; i != z64_audio_cmd_write_pos; ++i)
      serial_write(&p, &z64_audio_cmd_buf[i], sizeof(*z64_audio_cmd_buf));
  }
#if 0
  {
    uint8_t n_cmd = z64_afx_cmd_write_pos - z64_afx_cmd_read_pos;
    serial_write(&p, &n_cmd, sizeof(n_cmd));
    for (uint8_t i = z64_afx_cmd_read_pos; i != z64_afx_cmd_write_pos; ++i)
      serial_write(&p, &z64_afx_cmd_buf[i], sizeof(*z64_afx_cmd_buf));
  }
#endif

  /* save ocarina state */
  serial_write(&p, z64_ocarina_state, 0x0060);
  /* ocarina minigame parameters */
  serial_write(&p, &z64_ocarina_state[0x0068], 0x0001);
  serial_write(&p, &z64_ocarina_state[0x006C], 0x0001);
  /* save song state */
  serial_write(&p, z64_song_state, 0x00AC);
  serial_write(&p, z64_scarecrow_song, 0x0140);
  serial_write(&p, z64_song_ptr, 0x0004);
  serial_write(&p, z64_staff_notes, 0x001E);

  //serial_write(&p, (void*)0x800E2FC0, 0x31E10);
  //serial_write(&p, (void*)0x8012143C, 0x41F4);
  //serial_write(&p, (void*)0x801DAA00, 0x1D4790);

  return (char*)p - (char*)state;
}

void load_state(void *state)
{
  void *p = state;

  /* skip metadata */
  serial_skip(&p, sizeof(struct state_meta));

  /* cancel queued sound effects */
  z64_sfx_read_pos = z64_sfx_write_pos;
  /* cancel queued audio commands */
  z64_audio_cmd_read_pos = z64_audio_cmd_write_pos;
  z64_afx_cmd_read_pos = z64_afx_cmd_write_pos;
  /* stop sound effects */
  /* importantly, this removes all sound effect control points, which prevents
     floating-point exception crashes due to dangling pointers in the cp's. */
  z64_StopSfx();

  /* load sequencer info */
  struct seq_info
  {
    _Bool     p_active;
    uint32_t  stop_cmd_buf[8];
    uint8_t   stop_cmd_timer;
    uint8_t   stop_cmd_count;
    uint16_t  seq_idx;
    float     volume;
    uint8_t   vp_factors[4];
    uint16_t  ch_mute;

  } seq_info[4];

  for (int i = 0; i < 4; ++i) {
    struct seq_info *si = &seq_info[i];
    serial_read(&p, &si->p_active, sizeof(si->p_active));
    if (si->p_active) {
      serial_read(&p, &si->stop_cmd_timer, sizeof(si->stop_cmd_timer));
      serial_read(&p, &si->stop_cmd_count, sizeof(si->stop_cmd_count));
      serial_read(&p, &si->stop_cmd_buf,
                  sizeof(*si->stop_cmd_buf) * si->stop_cmd_count);
    }
    serial_read(&p, &si->seq_idx, sizeof(si->seq_idx));
    serial_read(&p, &si->volume, sizeof(si->volume));
    serial_read(&p, &si->vp_factors, sizeof(si->vp_factors));
    serial_read(&p, &si->ch_mute, sizeof(si->ch_mute));
  }

  /* configure afx if needed */
  int p_afx_cfg = z64_afx_cfg;
  uint8_t c_afx_cfg;
  serial_read(&p, &c_afx_cfg, sizeof(c_afx_cfg));
  if (c_afx_cfg != p_afx_cfg) {
    z64_afx_cfg = c_afx_cfg;
    z64_ConfigureAfx(c_afx_cfg);
    z64_ResetAudio(p_afx_cfg);
    z64_AfxCmdW(0xF8000000, 0x00000000);
  }
  else {
    /* stop sequencers that should not be playing, or should be playing a
       different sequence */
    for (int i = 0; i < 4; ++i) {
      struct seq_info *si = &seq_info[i];
      z64_seq_ctl_t *sc = &z64_seq_ctl[i];
      /* sequencer 2 is used for sound effects and should always be playing */
      if (i == 2)
        continue;
      if (si->seq_idx == 0xFFFF || !si->p_active || si->seq_idx != sc->seq_idx)
        z64_AfxCmdW(0x83000000 | (i << 16), 0x00000000);
    }
    z64_FlushAfxCmd();
  }

  /* wait for gfx task to finish */
  z64_osRecvMesg(&z64_ctxt.gfx->task_mq, NULL, OS_MESG_BLOCK);
  z64_osSendMesg(&z64_ctxt.gfx->task_mq, NULL, OS_MESG_NOBLOCK);

  /* save allocation info */
  struct alloc
  {
    int   id;
    void *ptr;
  };
  struct alloc room_list[2];
  for (int i = 0; i < 2; ++i) {
    z64_room_t *room = &z64_game.room_ctxt.rooms[i];
    room_list[i].id = room->index;
    room_list[i].ptr = room->file;
  }
  struct alloc obj_list[19];
  for (int i = 0; i < 19; ++i) {
    z64_mem_obj_t *obj = &z64_game.obj_ctxt.objects[i];
    obj_list[i].id = obj->id;
    obj_list[i].ptr = obj->data;
  }
  int scene_index = z64_game.scene_index;
  int ps = z64_game.pause_ctxt.state;
  _Bool p_pause_objects = (ps > 0x0003 && ps < 0x0008) || ps > 0x000A;
  _Bool p_gameover = ps >= 0x0008 && ps <= 0x0011;

  /* load context */
  serial_read(&p, &z64_game, sizeof(z64_game));
  serial_read(&p, &z64_file, sizeof(z64_file));
  serial_read(&p, z64_file.gameinfo, sizeof(*z64_file.gameinfo));
  /* load overlays */
  int16_t n_ent;
  int16_t next_ent;
  struct set ovl_nodes;
  set_init(&ovl_nodes, sizeof(uint32_t), addr_comp);
  /* actor overlays */
  n_ent = sizeof(z64_actor_ovl_tab) / sizeof(*z64_actor_ovl_tab);
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < n_ent; ++i) {
    z64_actor_ovl_t *ovl = &z64_actor_ovl_tab[i];
    if (i == next_ent) {
      serial_read(&p, &ovl->n_inst, sizeof(ovl->n_inst));
      load_ovl(&p, &ovl->ptr,
               ovl->vrom_start, ovl->vrom_end,
               ovl->vram_start, ovl->vram_end);
      set_insert(&ovl_nodes, &ovl->ptr);
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else {
      ovl->n_inst = 0;
      ovl->ptr = NULL;
    }
  }
  /* play overlays */
  n_ent = sizeof(z64_play_ovl_tab) / sizeof(*z64_play_ovl_tab);
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < n_ent; ++i) {
    z64_play_ovl_t *ovl = &z64_play_ovl_tab[i];
    if (i == next_ent) {
      load_ovl(&p, &ovl->ptr,
               ovl->vrom_start, ovl->vrom_end,
               ovl->vram_start, ovl->vram_end);
      ovl->reloc_offset = (uint32_t)ovl->ptr - ovl->vram_start;
      set_insert(&ovl_nodes, &ovl->ptr);
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else {
      ovl->ptr = NULL;
      ovl->reloc_offset = 0;
    }
  }
  serial_read(&p, &z64_play_ovl_ptr, sizeof(z64_play_ovl_ptr));
  /* particle overlays */
  n_ent = sizeof(z64_part_ovl_tab) / sizeof(*z64_part_ovl_tab);
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < n_ent; ++i) {
    z64_part_ovl_t *ovl = &z64_part_ovl_tab[i];
    if (i == next_ent) {
      load_ovl(&p, &ovl->ptr,
               ovl->vrom_start, ovl->vrom_end,
               ovl->vram_start, ovl->vram_end);
      set_insert(&ovl_nodes, &ovl->ptr);
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else
      ovl->ptr = NULL;
  }
  /* map mark overlay */
  serial_read(&p, &next_ent, sizeof(next_ent));
  if (next_ent == 0) {
    z64_map_mark_ovl_t *ovl = &z64_map_mark_ovl;
    load_ovl(&p, &ovl->ptr,
             ovl->vrom_start, ovl->vrom_end,
             ovl->vram_start, ovl->vram_end);
    set_insert(&ovl_nodes, &ovl->ptr);
    serial_read(&p, &next_ent, sizeof(next_ent));
    /* relocate data table pointer */
    char *data_tab = ovl->ptr;
    data_tab += ovl->vram_data_tab - ovl->vram_start;
    z64_map_mark_data_tab = data_tab;
  }
  else
    z64_map_mark_ovl.ptr = NULL;

  /* load arena nodes */
  serial_read(&p, &z64_game_arena, sizeof(z64_game_arena));
  z64_arena_node_t *node = z64_game_arena.first_node;
  serial_read(&p, &next_ent, sizeof(next_ent));
  while (node) {
    node->magic = 0x7373;
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
    node->arena = &z64_game_arena;
    node->filename = NULL,
    node->line = 0;
    node->thread_id = 4;
#endif
    serial_read(&p, &node->free, sizeof(node->free));
    serial_read(&p, &node->size, sizeof(node->size));
    char *data = node->data;
    if (!set_get(&ovl_nodes, &data) && !node->free)
      serial_read(&p, data, node->size);
    if (node == z64_game_arena.first_node)
      node->prev = NULL;
    serial_read(&p, &next_ent, sizeof(next_ent));
    if (next_ent == 0) {
      node->next = (void*)&node->data[node->size];
      node->next->prev = node;
    }
    else
      node->next = NULL;
    node = node->next;
  }
  set_destroy(&ovl_nodes);

  /* load light queue */
  serial_read(&p, &z64_light_queue, sizeof(z64_light_queue));
  /* load matrix stack info */
  serial_read(&p, &z64_mtx_stack, sizeof(z64_mtx_stack));
  serial_read(&p, &z64_mtx_stack_top, sizeof(z64_mtx_stack_top));
  /* load segment table */
  serial_read(&p, &z64_stab, sizeof(z64_stab));

  /* load particles */
  serial_read(&p, &z64_part_space, sizeof(z64_part_space));
  serial_read(&p, &z64_part_pos, sizeof(z64_part_pos));
  serial_read(&p, &z64_part_max, sizeof(z64_part_max));
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < z64_part_max; ++i) {
    z64_part_t *part = &z64_part_space[i];
    if (i == next_ent) {
      serial_read(&p, part, sizeof(*part));
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else {
      memset(part, 0, sizeof(*part));
      part->time = -1;
      part->priority = 0x80;
      part->part_id = 0x25;
    }
  }
  /* load static particles */
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < 3; ++i) {
    z64_dot_t *dot = &z64_pfx.dots[i];
    if (i == next_ent) {
      serial_read(&p, dot, sizeof(*dot));
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else
      dot->active = 0;
  }
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < 25; ++i) {
    z64_trail_t *trail = &z64_pfx.trails[i];
    if (i == next_ent) {
      serial_read(&p, trail, sizeof(*trail));
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else
      trail->active = 0;
  }
  serial_read(&p, &next_ent, sizeof(next_ent));
  for (int16_t i = 0; i < 3; ++i) {
    z64_spark_t *spark = &z64_pfx.sparks[i];
    if (i == next_ent) {
      serial_read(&p, spark, sizeof(*spark));
      serial_read(&p, &next_ent, sizeof(next_ent));
    }
    else
      spark->active = 0;
  }
  /* load camera shake effects */
  serial_read(&p, &z64_n_camera_shake, 0x0002);
  serial_read(&p, z64_camera_shake, 0x0090);

  /* load scene */
  if (z64_game.scene_index != scene_index) {
    z64_scene_table_t *scene = &z64_scene_table[z64_game.scene_index];
    uint32_t size = scene->scene_vrom_end - scene->scene_vrom_start;
    zu_getfile(scene->scene_vrom_start, z64_game.scene_file, size);
    reloc_col_hdr((uint32_t)z64_game.col_ctxt.col_hdr);
    /* create static collision */
    z64_game.col_ctxt.stc_list_pos = 0;
    z64_CreateStaticCollision(&z64_game.col_ctxt, &z64_game,
                              z64_game.col_ctxt.stc_lut);

  }
  {
    /* load transition actor list */
    z64_room_ctxt_t *room_ctxt = &z64_game.room_ctxt;
    serial_read(&p, room_ctxt->tnsn_list,
                room_ctxt->n_tnsn * sizeof(*room_ctxt->tnsn_list));
    /* load rooms */
    for (int i = 0; i < 2; ++i) {
      struct alloc *p_room = &room_list[i];
      z64_room_t *c_room = &room_ctxt->rooms[i];
      int p_id = p_room->id;
      int c_id = c_room->index;
      void *p_ptr = p_room->ptr;
      void *c_ptr = c_room->file;
      if (c_ptr && c_id != -1 && (z64_game.scene_index != scene_index ||
                                  c_id != p_id || c_ptr != p_ptr))
      {
        uint32_t start = z64_game.room_list[c_id].vrom_start;
        uint32_t end = z64_game.room_list[c_id].vrom_end;
        zu_getfile(start, c_ptr, end - start);
      }
    }
    /* start async room load */
    if (room_ctxt->load_active)
      z64_osSendMesg(&z64_file_mq, &room_ctxt->load_getfile, OS_MESG_NOBLOCK);
  }

  /* load objects */
  ps = z64_game.pause_ctxt.state;
  _Bool c_pause_objects = (ps > 0x0003 && ps < 0x0008) || ps > 0x000A;
  _Bool c_gameover = ps >= 0x0008 && ps <= 0x0011;
  int si = z64_game.scene_index;
  _Bool dungeon_map = si < 0x000A || (si > 0x0010 && si < 0x0019);
  if (c_pause_objects) {
    /* gameover-specific states */
    if (c_gameover) {
      zu_getfile_idx(z64_icon_item_gameover_static,
                     z64_game.pause_ctxt.icon_item_s);
    }
    else {
      z64_InitPauseObjects(&z64_game, z64_game.pause_ctxt.p_0x13C,
                           &z64_game.pause_ctxt.s_0x27C);
      if (dungeon_map) {
        zu_getfile_idx(z64_icon_item_dungeon_static,
                       z64_game.pause_ctxt.icon_item_s);
        uint32_t vaddr = z64_ftab[z64_map_48x85_static].vrom_start;
        vaddr += z64_file.gameinfo->dungeon_map_floor * 0x07F8;
        zu_getfile(vaddr, z64_game.if_ctxt.minimap_texture, 0x07F8);
        vaddr += 0x07F8;
        zu_getfile(vaddr, z64_game.if_ctxt.minimap_texture + 0x0800, 0x07F8);
      }
      else
        zu_getfile_idx(z64_icon_item_field_static,
                       z64_game.pause_ctxt.icon_item_s);
      if (z64_game.pause_ctxt.screen_idx == 1) {
        uint32_t vaddr = z64_ftab[z64_map_name_static].vrom_start;
        if (z64_file.language != 0)
          vaddr += 0x000C * 0x0400;
        vaddr += z64_game.pause_ctxt.item_id * 0x400;
        zu_getfile(vaddr, z64_game.pause_ctxt.name_texture, 0x0400);
      }
      else {
        uint32_t vaddr = z64_ftab[z64_item_name_static].vrom_start;
        if (z64_file.language != 0)
          vaddr += 0x007B * 0x0400;
        vaddr += z64_game.pause_ctxt.item_id * 0x400;
        zu_getfile(vaddr, z64_game.pause_ctxt.name_texture, 0x0400);
      }
    }
    if (z64_file.language == 0) {
      zu_getfile_idx(z64_icon_item_jpn_static,
                     z64_game.pause_ctxt.icon_item_lang);
    }
    else {
      zu_getfile_idx(z64_icon_item_nes_static,
                     z64_game.pause_ctxt.icon_item_lang);
    }
    zu_getfile_idx(z64_icon_item_static, z64_game.pause_ctxt.icon_item);
    zu_getfile_idx(z64_icon_item_24_static, z64_game.pause_ctxt.icon_item_24);
    /* gray out restricted items */
    char *p = z64_play_ovl_tab[0].ptr;
    p += ((uint32_t)&z64_item_highlight_vram - z64_play_ovl_tab[0].vram_start);
    uint8_t *item_highlight_tab = (void*)p;
    uint32_t *pixels = z64_game.pause_ctxt.icon_item;
    for (int i = 0; i < 0x56; ++i) {
      if (item_highlight_tab[i] != 0x09 &&
          item_highlight_tab[i] != z64_file.link_age)
      {
        grayscale_texture(&pixels[i * 0x0400], 0x0400);
      }
    }
  }
  else {
    /* load normal objects in the object context */
    for (int i = 0; i < 19; ++i) {
      struct alloc *p_obj = &obj_list[i];
      z64_mem_obj_t *c_obj = &z64_game.obj_ctxt.objects[i];
      int p_id = p_obj->id;
      int c_id = c_obj->id;
      if (c_id < 0)
        c_id = -c_id;
      void *p_ptr = p_obj->ptr;
      void *c_ptr = c_obj->data;
      /* if the object in the current slot is different, or loaded to
         a different address, then reload it. if the previous state had
         pause screen objects loaded, all objects need to be reloaded. */
      if (c_id != 0 && (p_pause_objects || c_id != p_id || c_ptr != p_ptr)) {
        uint32_t start = z64_object_table[c_id].vrom_start;
        uint32_t end = z64_object_table[c_id].vrom_end;
        zu_getfile(start, c_ptr, end - start);
      }

      for (int i = 0; i < sizeof(z64_game.col_ctxt.dyn_col) / sizeof(*z64_game.col_ctxt.dyn_col); i++) {
        z64_dyn_col_t *dyn_col = &z64_game.col_ctxt.dyn_col[i];
        if(!dyn_col->actor) {
          continue;
        }

        z64_mem_obj_t *obj = &z64_game.obj_ctxt.objects[dyn_col->actor->alloc_index];
        z64_stab.seg[Z64_SEG_OBJ] = MIPS_KSEG0_TO_PHYS(obj->data);

        uint32_t offset = (uint32_t)dyn_col->col_hdr - (uint32_t)obj->data;
        reloc_col_hdr(0x06000000 | offset);
      }
    }
  }

  /* load waterboxes */
  {
    z64_col_hdr_t *col_hdr = z64_game.col_ctxt.col_hdr;
    serial_read(&p, &col_hdr->n_water, sizeof(col_hdr->n_water));
    serial_read(&p, col_hdr->water,
                sizeof(*col_hdr->water) * col_hdr->n_water);
  }
  /* load dynamic collision */
  {
    z64_col_ctxt_t *col = &z64_game.col_ctxt;
    serial_read(&p, col->dyn_list,
                col->dyn_list_max * sizeof(*col->dyn_list));
    serial_read(&p, col->dyn_poly,
                col->dyn_poly_max * sizeof(*col->dyn_poly));
    serial_read(&p, col->dyn_vtx,
                col->dyn_vtx_max * sizeof(*col->dyn_vtx));
  }

  /* create skybox */
  if (z64_game.skybox_type != 0) {
    if (z64_game.sky_ctxt.mode == 0) {
      if (z64_game.skybox_type == 5)
        z64_CreateSkyVtx(&z64_game.sky_ctxt, 6);
      else
        z64_CreateSkyVtx(&z64_game.sky_ctxt, 5);
    }
    else
      z64_CreateSkyGfx(&z64_game.sky_ctxt, z64_game.skybox_type);
    load_sky_image();
  }

  if (z64_game.elf_message)
    serial_read(&p, z64_game.elf_message, 0x0070);

  /* minimap details */
  serial_read(&p, &z64_minimap_entrance_x, sizeof(z64_minimap_entrance_x));
  serial_read(&p, &z64_minimap_entrance_y, sizeof(z64_minimap_entrance_y));
  serial_read(&p, &z64_minimap_entrance_r, sizeof(z64_minimap_entrance_r));

  /* weather / daytime state */
  serial_read(&p, z64_weather_state, 0x0018);
  serial_read(&p, &z64_temp_day_speed, sizeof(z64_temp_day_speed));

  /* hazard state */
  serial_read(&p, z64_hazard_state, 0x0008);

  /* timer state */
  serial_read(&p, z64_timer_state, 0x0008);

  /* hud state */
  serial_read(&p, z64_hud_state, 0x0008);

  /* letterboxing */
  serial_read(&p, &z64_letterbox_target, sizeof(z64_letterbox_target));
  serial_read(&p, &z64_letterbox_current, sizeof(z64_letterbox_current));
  serial_read(&p, &z64_letterbox_time, sizeof(z64_letterbox_time));

  /* poly color filter state (sepia effect) */
  serial_read(&p, z64_poly_colorfilter_state, 0x001C);

  /* sound state */
  serial_read(&p, z64_sound_state, 0x004C);

  /* event state */
  serial_read(&p, z64_event_state_1, 0x0008);
  serial_read(&p, z64_event_state_2, 0x0004);
  /* event camera parameters */
  for (int i = 0; i < 24; ++i)
    serial_read(&p, &z64_event_camera[0x28 * i + 0x10], 0x0018);

  /* oob timer */
  serial_read(&p, &z64_oob_timer, sizeof(z64_oob_timer));

  /* countdown to gameover screen */
  serial_read(&p, &z64_gameover_countdown, sizeof(z64_gameover_countdown));

  /* rng */
  serial_read(&p, &z64_random, sizeof(z64_random));

  /* spell states */
  serial_read(&p, z64_dins_state_1, 0x0004);
  serial_read(&p, &z64_dins_state_2[0x0006], 0x0002);
  serial_read(&p, &z64_dins_state_2[0x0014], 0x0004);
  serial_read(&p, &z64_dins_state_2[0x0020], 0x0004);
  serial_read(&p, &z64_dins_state_2[0x003C], 0x0004);
  serial_read(&p, z64_fw_state_1, 0x0004);
  serial_read(&p, z64_fw_state_2, 0x0004);

  /* camera state */
  serial_read(&p, z64_camera_state, 0x0020);

  /* cutscene state */
  serial_read(&p, z64_cs_state, 0x0140);
  /* cutscene message id */
  serial_read(&p, z64_cs_message, 0x0008);

  /* message state */
  serial_read(&p, z64_message_state, 0x0028);

  /* load textures */
  zu_getfile_idx(z64_parameter_static, z64_game.if_ctxt.parameter);
  /* item button icons */
  for (int i = 0; i < 4; ++i)
    if (z64_file.button_items[i] != Z64_ITEM_NULL)
      z64_UpdateItemButton(&z64_game, i);
  /* action labels */
  z64_LoadActionLabel(&z64_game.if_ctxt, z64_game.if_ctxt.a_action,
                      Z64_ACTIONBTN_A);
  z64_LoadActionLabel(&z64_game.if_ctxt, z64_game.if_ctxt.b_label % 0x1D,
                      Z64_ACTIONBTN_B);
  z64_LoadActionLabel(&z64_game.if_ctxt, 0x0003, Z64_ACTIONBTN_START);
  /* minimap */
  if (!c_pause_objects || !dungeon_map)
    z64_LoadMinimap(&z64_game, z64_game.room_ctxt.rooms[0].index);
  /* message stuff */
  if (z64_game.message_state_1 != 0) {
    char *tex_buf = z64_game.message_texture;
    uint32_t message_static = z64_ftab[z64_message_static].vrom_start;
    /* load message background */
    switch (z64_game.message_type) {
      case 0:   zu_getfile(message_static + 0x0000, tex_buf, 0x1000); break;
      case 1:   zu_getfile(message_static + 0x1000, tex_buf, 0x1000); break;
      case 2:   zu_getfile(message_static + 0x3000, tex_buf, 0x1000); break;
      case 3:   zu_getfile(message_static + 0x2000, tex_buf, 0x1000); break;
      default:  break;
    }
    /* look for an icon or foreground image command;
       if one is found then load the texture */
    int icon_idx = -1;
    int foreground_image = -1;
    if (z64_file.language == 0) {
      /* japanese message format */
      for (int i = 0; i < 2; ++i) {
        if (z64_game.message_data_j[i] == 0x819A)
          icon_idx = z64_game.message_data_j[i + 1];
        else if (z64_game.message_data_j[i] == 0x86B3)
          foreground_image = 0;
        else
          continue;
        break;
      }
    }
    else {
      /* english message format */
      for (int i = 0; i < 2; ++i) {
        if (z64_game.message_data_e[i] == 0x13)
          icon_idx = z64_game.message_data_e[i + 1];
        else if (z64_game.message_data_e[i] == 0x15)
          foreground_image = 0;
        else
          continue;
        break;
      }
    }
    if (foreground_image == 0) {
      zu_getfile(z64_ftab[z64_message_texture_static].vrom_start,
                 tex_buf + 0x1000, 0x1200);
    }
    else if (icon_idx >= 0x66) {
      zu_getfile(z64_ftab[z64_icon_item_24_static].vrom_start
                 + (icon_idx - 0x66) * 0x900,
                 tex_buf + 0x1000, 0x900);
    }
    else if (icon_idx >= 0x00) {
      zu_getfile(z64_ftab[z64_icon_item_static].vrom_start + icon_idx * 0x1000,
                 tex_buf + 0x1000, 0x1000);
    }
  }
  /* clear unsaved textures */
  if (c_pause_objects && !p_pause_objects) {
    uint16_t (*img)[Z64_SCREEN_HEIGHT][Z64_SCREEN_WIDTH];
    img = (void*)&z64_zimg;
    for (int y = 0; y < Z64_SCREEN_HEIGHT; ++y)
      for (int x = 0; x < Z64_SCREEN_WIDTH; ++x)
        (*img)[y][x] = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
  }
  if (c_pause_objects && !c_gameover && (!p_pause_objects || p_gameover)) {
    uint16_t (*img)[112][64];
    img = z64_game.pause_ctxt.p_0x13C;
    for (int y = 0; y < 112; ++y)
      for (int x = 0; x < 64; ++x)
        (*img)[y][x] = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
  }

  /* load display lists */
  serial_read(&p, &next_ent, sizeof(next_ent));
  if (next_ent == 0) {
    z64_gfx_t *gfx = z64_ctxt.gfx;
    /* load pointers */
    struct zu_disp_p disp_p;
    serial_read(&p, &disp_p, sizeof(disp_p));
    zu_load_disp_p(&disp_p);
    /* load commands and data */
    z64_disp_buf_t *z_disp[4] =
    {
      &gfx->work,
      &gfx->poly_opa,
      &gfx->poly_xlu,
      &gfx->overlay,
    };
    for (int i = 0; i < 4; ++i) {
      z64_disp_buf_t *disp_buf = z_disp[i];
      size_t s = sizeof(Gfx);
      Gfx *e = disp_buf->buf + disp_buf->size / s;
      serial_read(&p, disp_buf->buf, (disp_buf->p - disp_buf->buf) * s);
      serial_read(&p, disp_buf->d, (e - disp_buf->d) * s);
    }
    /* relocate lists */
    uint32_t frame_count_1;
    uint32_t frame_count_2;
    serial_read(&p, &frame_count_1, sizeof(frame_count_1));
    serial_read(&p, &frame_count_2, sizeof(frame_count_2));
    zu_reloc_gfx(frame_count_1 & 1, frame_count_2 & 1);
  }
  else {
    /* no display lists saved, kill frame */
    z64_gfx_t *gfx = z64_ctxt.gfx;
    gfx->work.p = gfx->work.buf;
    gDPSetColorImage(gfx->work.p++,
                     G_IM_FMT_RGBA, G_IM_SIZ_16b, Z64_SCREEN_WIDTH,
                     ZU_MAKE_SEG(Z64_SEG_CIMG, 0));
    gDPSetCycleType(gfx->work.p++, G_CYC_FILL);
    gDPSetRenderMode(gfx->work.p++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetFillColor(gfx->work.p++,
                    (GPACK_RGBA5551(0x1F, 0x1F, 0x1F, 0x01) << 16) |
                    GPACK_RGBA5551(0x1F, 0x1F, 0x1F, 0x01));
    gDPFillRectangle(gfx->work.p++,
                     0, 0, Z64_SCREEN_WIDTH - 1, Z64_SCREEN_HEIGHT - 1);
    gDPPipeSync(gfx->work.p++);
    gDPFullSync(gfx->work.p++);
    gSPEndDisplayList(gfx->work.p++);
  }

  /* wait for afx config to finish */
  if (c_afx_cfg != p_afx_cfg) {
    while (z64_CheckAfxConfigBusy())
      ;
  }

  /* restore audio state */
  for (int i = 0; i < 4; ++i) {
    struct seq_info *si = &seq_info[i];
    z64_seq_ctl_t *sc = &z64_seq_ctl[i];
    char *seq = &z64_afx[0x3530 + i * 0x0160];
    _Bool c_active = *(uint8_t*)(seq) & 0x80;
    if (si->p_active) {
      memcpy(&sc->stop_cmd_timer, &si->stop_cmd_timer,
             sizeof(si->stop_cmd_timer));
      memcpy(&sc->stop_cmd_count, &si->stop_cmd_count,
             sizeof(si->stop_cmd_count));
      memcpy(&sc->stop_cmd_buf, &si->stop_cmd_buf,
             sizeof(*si->stop_cmd_buf) * si->stop_cmd_count);
      if (!c_active && sc->stop_cmd_timer == 0)
        sc->stop_cmd_timer = 1;
    }
    else {
      sc->stop_cmd_timer = 0;
      sc->stop_cmd_count = 0;
    }
    /* clear sequencer volume effects */
    memcpy(&sc->vp_factors, &si->vp_factors, sizeof(si->vp_factors));
    sc->vs_current = si->volume;
    sc->vs_time = 0;
    sc->vp_start = 0;
    /* play sequence */
    sc->seq_idx = si->seq_idx;
    sc->prev_seq_idx = si->seq_idx;
    uint8_t real_seq_idx = si->seq_idx;
    if (i == 0 && real_seq_idx == 0x01 && si->p_active)
      play_night_sfx();
    else if (si->seq_idx != 0xFFFF && si->p_active && !c_active)
      z64_AfxCmdW(0x82000000 | (i << 16) | (real_seq_idx << 8), 0x00000000);
    /* set sequencer volume */
    z64_AfxCmdF(0x41000000 | (i << 16), si->volume);
    /* clear channel volume effects and set channel mutes */
    for (int j = 0; j < 16; ++j) {
      z64_chan_ctl_t *cc = &sc->channels[j];
      _Bool ch_mute = si->ch_mute & (1 << j);
      cc->vs_current = 1.;
      cc->vs_time = 0;
      z64_AfxCmdF(0x01000000 | (i << 16) | (j << 8), 1.f);
      z64_AfxCmdW(0x08000000 | (i << 16) | (j << 8), ch_mute << 24);
    }
    sc->ch_volume_state = 0;
  }
  if (z64_game.pause_ctxt.state > 0 && z64_game.pause_ctxt.state < 8)
    z64_AfxCmdW(0xF1000000, 0x00000000);
  else
    z64_AfxCmdW(0xF2000000, 0x00000000);
  z64_FlushAfxCmd();

  /* load sfx mutes */
  serial_read(&p, z64_sfx_mute, 0x0008);
  /* restore pending audio commands */
  {
    uint8_t n_cmd;
    serial_read(&p, &n_cmd, sizeof(n_cmd));
    for (uint8_t i = 0; i != n_cmd; ++i) {
      serial_read(&p, &z64_audio_cmd_buf[z64_audio_cmd_write_pos++],
                  sizeof(*z64_audio_cmd_buf));
    }
  }
#if 0
  {
    uint8_t n_cmd;
    serial_read(&p, &n_cmd, sizeof(n_cmd));
    for (uint8_t i = 0; i != n_cmd; ++i) {
      serial_read(&p, &z64_afx_cmd_buf[z64_afx_cmd_write_pos++],
                  sizeof(*z64_afx_cmd_buf));
    }
  }
#endif

  /* load ocarina state */
  serial_read(&p, z64_ocarina_state, 0x0060);
  /* ocarina minigame parameters */
  serial_read(&p, &z64_ocarina_state[0x0068], 0x0001);
  serial_read(&p, &z64_ocarina_state[0x006C], 0x0001);
  /* load song state */
  serial_read(&p, z64_song_state, 0x00AC);
  serial_read(&p, z64_scarecrow_song, 0x0140);
  serial_read(&p, z64_song_ptr, 0x0004);
  serial_read(&p, z64_staff_notes, 0x001E);
  /* fix audio counters */
  {
    uint32_t delta = z64_song_counter - z64_ocarina_counter;
    z64_song_counter = z64_afx_counter;
    z64_ocarina_counter = z64_song_counter - delta;
  }

  //serial_read(&p, (void*)0x800E2FC0, 0x31E10);
  //serial_read(&p, (void*)0x8012143C, 0x41F4);
  //serial_read(&p, (void*)0x801DAA00, 0x1D4790);
}
