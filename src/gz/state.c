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
  z64_ovl_hdr_t *hdr = (void*)(end - *hdr_off);
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
  z64_ovl_hdr_t *hdr = (void*)(end - *hdr_off);
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
    for (uint16_t j = 0; j < n_copy; ++j)
      data[i++] = yaz0_get_byte();
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
      zu_getfile_idx(977, sky_ctxt->textures[0]);
      zu_getfile_idx(978, sky_ctxt->palettes);
      break;

    case 0x0003:
      /* overcast sunset */
      zu_getfile_idx(953, sky_ctxt->textures[0]);
      zu_getfile_idx(953, sky_ctxt->textures[1]);
      zu_getfile_idx(954, sky_ctxt->palettes);
      zu_getfile_idx(954, sky_ctxt->palettes + 0x100);
      break;

    case 0x0004:
      /* market ruins */
      zu_getfile_idx(965, sky_ctxt->textures[0]);
      zu_getfile_idx(966, sky_ctxt->palettes);
      break;

    case 0x0005:
      /* cutscene map */
      zu_getfile_idx(957, sky_ctxt->textures[0]);
      zu_getfile_idx(959, sky_ctxt->textures[1]);
      zu_getfile_idx(958, sky_ctxt->palettes);
      zu_getfile_idx(960, sky_ctxt->palettes + 0x100);
      break;

    case 0x0007:
      /* link's house */
      zu_getfile_idx(967, sky_ctxt->textures[0]);
      zu_getfile_idx(968, sky_ctxt->palettes);
      break;

    case 0x0009:
      /* market day */
      zu_getfile_idx(961, sky_ctxt->textures[0]);
      zu_getfile_idx(962, sky_ctxt->palettes);
      break;

    case 0x000A:
      /* market night */
      zu_getfile_idx(963, sky_ctxt->textures[0]);
      zu_getfile_idx(964, sky_ctxt->palettes);
      break;

    case 0x000B:
      /* happy mask shop */
      zu_getfile_idx(1003, sky_ctxt->textures[0]);
      zu_getfile_idx(1004, sky_ctxt->palettes);
      break;

    case 0x000C:
      /* know-it-all brothers' house */
      zu_getfile_idx(969, sky_ctxt->textures[0]);
      zu_getfile_idx(970, sky_ctxt->palettes);
      break;

    case 0x000E:
      /* house of twins */
      zu_getfile_idx(971, sky_ctxt->textures[0]);
      zu_getfile_idx(972, sky_ctxt->palettes);
      break;

    case 0x000F:
      /* stable */
      zu_getfile_idx(979, sky_ctxt->textures[0]);
      zu_getfile_idx(980, sky_ctxt->palettes);
      break;

    case 0x0010:
      /* carpenter's house */
      zu_getfile_idx(981, sky_ctxt->textures[0]);
      zu_getfile_idx(982, sky_ctxt->palettes);
      break;

    case 0x0011:
      /* kokiri shop */
      zu_getfile_idx(987, sky_ctxt->textures[0]);
      zu_getfile_idx(988, sky_ctxt->palettes);
      break;

    case 0x0013:
      /* goron shop */
      zu_getfile_idx(989, sky_ctxt->textures[0]);
      zu_getfile_idx(990, sky_ctxt->palettes);
      break;

    case 0x0014:
      /* zora shop */
      zu_getfile_idx(991, sky_ctxt->textures[0]);
      zu_getfile_idx(992, sky_ctxt->palettes);
      break;

    case 0x0016:
      /* kakariko potion shop */
      zu_getfile_idx(993, sky_ctxt->textures[0]);
      zu_getfile_idx(994, sky_ctxt->palettes);
      break;

    case 0x0017:
      /* market potion shop */
      zu_getfile_idx(995, sky_ctxt->textures[0]);
      zu_getfile_idx(996, sky_ctxt->palettes);
      break;

    case 0x0018:
      /* bombchu shop */
      zu_getfile_idx(997, sky_ctxt->textures[0]);
      zu_getfile_idx(998, sky_ctxt->palettes);
      break;

    case 0x001A:
      /* richard's house */
      zu_getfile_idx(985, sky_ctxt->textures[0]);
      zu_getfile_idx(986, sky_ctxt->palettes);
      break;

    case 0x001B:
      /* impa's house */
      zu_getfile_idx(999, sky_ctxt->textures[0]);
      zu_getfile_idx(1000, sky_ctxt->palettes);
      break;

    case 0x001C:
      /* carpenter's tent */
      zu_getfile_idx(1001, sky_ctxt->textures[0]);
      zu_getfile_idx(1002, sky_ctxt->palettes);
      break;

    case 0x0020:
      /* mido's house */
      zu_getfile_idx(973, sky_ctxt->textures[0]);
      zu_getfile_idx(974, sky_ctxt->palettes);
      break;

    case 0x0021:
      /* saria's house */
      zu_getfile_idx(975, sky_ctxt->textures[0]);
      zu_getfile_idx(976, sky_ctxt->palettes);
      break;

    case 0x0022:
      /* guy's house */
      zu_getfile_idx(983, sky_ctxt->textures[0]);
      zu_getfile_idx(984, sky_ctxt->palettes);
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
    char *seq = (void*)(z64_afx_addr + 0x3530 + i * 0x0160);
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
  serial_write(&p, (void*)z64_n_camera_shake_addr, 0x0002);
  serial_write(&p, (void*)z64_camera_shake_addr, 0x0090);

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
  serial_write(&p, (void*)z64_weather_state_addr, 0x0018);
  serial_write(&p, &z64_temp_day_speed, sizeof(z64_temp_day_speed));

  /* hazard state */
  serial_write(&p, (void*)z64_hazard_state_addr, 0x0008);

  /* timer state */
  serial_write(&p, (void*)z64_timer_state_addr, 0x0008);

  /* hud state */
  serial_write(&p, (void*)z64_hud_state_addr, 0x0008);

  /* letterboxing */
  serial_write(&p, &z64_letterbox_target, sizeof(z64_letterbox_target));
  serial_write(&p, &z64_letterbox_current, sizeof(z64_letterbox_current));
  serial_write(&p, &z64_letterbox_time, sizeof(z64_letterbox_time));

  /* sound state */
  serial_write(&p, (void*)z64_sound_state_addr, 0x004C);

  /* event state */
  serial_write(&p, (void*)z64_event_state_1_addr, 0x0008);
  serial_write(&p, (void*)z64_event_state_2_addr, 0x0004);
  /* event camera parameters */
  for (int i = 0; i < 24; ++i)
    serial_write(&p, (void*)(z64_event_camera_addr + 0x28 * i + 0x10), 0x0018);

  /* oob timer */
  serial_write(&p, &z64_oob_timer, sizeof(z64_oob_timer));

  /* countdown to gameover screen */
  serial_write(&p, &z64_gameover_countdown, sizeof(z64_gameover_countdown));

  /* rng */
  serial_write(&p, &z64_random, sizeof(z64_random));

  /* camera state */
  serial_write(&p, (void*)z64_camera_state_addr, 0x0020);

  /* cutscene state */
  serial_write(&p, (void*)z64_cs_state_addr, 0x0140);
  /* cutscene message id */
  serial_write(&p, (void*)z64_cs_message_addr, 0x0008);

  /* message state */
  serial_write(&p, (void*)z64_message_state_addr, 0x0028);

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
  serial_write(&p, (void*)z64_sfx_mute_addr, 0x0008);
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
  serial_write(&p, (void*)z64_ocarina_state_addr, 0x0060);
  /* save song state */
  serial_write(&p, (void*)z64_song_state_addr, 0x00AC);
  serial_write(&p, (void*)z64_scarecrow_song_addr, 0x0140);
  serial_write(&p, (void*)z64_song_ptr_addr, 0x0004);
  serial_write(&p, (void*)z64_staff_notes_addr, 0x001E);

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
    node->arena = &z64_game_arena;
    node->filename = NULL,
    node->line = 0;
    node->thread_id = 4;
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
  serial_read(&p, (void*)z64_n_camera_shake_addr, 0x0002);
  serial_read(&p, (void*)z64_camera_shake_addr, 0x0090);

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
    if (c_gameover)
      zu_getfile_idx(12, z64_game.pause_ctxt.icon_item_s);
    else {
      z64_InitPauseObjects(&z64_game, z64_game.pause_ctxt.p13C,
                           &z64_game.pause_ctxt.s27C);
      if (dungeon_map) {
        zu_getfile_idx(11, z64_game.pause_ctxt.icon_item_s);
        uint32_t vaddr = z64_ftab[26].vrom_start;
        vaddr += z64_file.gameinfo->dungeon_map_floor * 0x07F8;
        zu_getfile(vaddr, z64_game.if_ctxt.minimap_texture, 0x07F8);
        vaddr += 0x07F8;
        zu_getfile(vaddr, z64_game.if_ctxt.minimap_texture + 0x0800, 0x07F8);
      }
      else
        zu_getfile_idx(10, z64_game.pause_ctxt.icon_item_s);
      if (z64_game.pause_ctxt.screen_idx == 1) {
        uint32_t vaddr = z64_ftab[16].vrom_start;
        if (z64_file.language != 0)
          vaddr += 0x000C * 0x0400;
        vaddr += z64_game.pause_ctxt.item_id * 0x400;
        zu_getfile(vaddr, z64_game.pause_ctxt.name_texture, 0x0400);
      }
      else {
        uint32_t vaddr = z64_ftab[15].vrom_start;
        if (z64_file.language != 0)
          vaddr += 0x007B * 0x0400;
        vaddr += z64_game.pause_ctxt.item_id * 0x400;
        zu_getfile(vaddr, z64_game.pause_ctxt.name_texture, 0x0400);
      }
    }
    if (z64_file.language == 0)
      zu_getfile_idx(13, z64_game.pause_ctxt.icon_item_lang);
    else
      zu_getfile_idx(14, z64_game.pause_ctxt.icon_item_lang);
    zu_getfile_idx(8, z64_game.pause_ctxt.icon_item);
    zu_getfile_idx(9, z64_game.pause_ctxt.icon_item_24);
    /* gray out restricted items */
    char *p = z64_play_ovl_tab[0].ptr;
    p += (z64_item_highlight_vram_addr - z64_play_ovl_tab[0].vram_start);
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
      /* object-specific initialization monkaS */
      z64_stab.seg[Z64_SEG_OBJ] = MIPS_KSEG0_TO_PHYS(c_ptr);
      switch (c_id) {
      /* some object files contain collision data. this collision data has
         an associated collision header (z64_col_hdr_t), with segment
         addresses that must be relocated to prevent crashes.
         this relocation usually happens within the constructor of the actor
         that uses said collision data. this means that the locations of
         these collision headers are hardcoded in the actor overlays.
         thus, there's no convenient list of all addresses that may need
         relocation. instead, they have to be painstakingly tracked down
         by hand. hopefully i've managed to find them all. */
        case 0x0001:  reloc_col_hdr(0x06039CE0);
                      reloc_col_hdr(0x0603A950);
                      reloc_col_hdr(0x0603ACB0);
                      reloc_col_hdr(0x0603B020); break;
        case 0x0002:  reloc_col_hdr(0x060041B0); break;
        case 0x0003:  reloc_col_hdr(0x06004E98);
                      reloc_col_hdr(0x06005FB8); break;
        case 0x000E:  reloc_col_hdr(0x06005FC8); break;
        case 0x0019:  reloc_col_hdr(0x06024764);
                      reloc_col_hdr(0x060250A8); break;
        case 0x001C:  reloc_col_hdr(0x0601D9D0); break;
        case 0x002A:  reloc_col_hdr(0x06000E94); break;
        case 0x002B:  reloc_col_hdr(0x06001DDC);
                      reloc_col_hdr(0x06003CE0);
                      reloc_col_hdr(0x06004F30); break;
        case 0x002C:  reloc_col_hdr(0x0600CB80);
                      reloc_col_hdr(0x0600CC90);
                      reloc_col_hdr(0x0600CDA0);
                      reloc_col_hdr(0x0600D054);
                      reloc_col_hdr(0x0600D188);
                      reloc_col_hdr(0x0600D5C0);
                      reloc_col_hdr(0x0600D800);
                      reloc_col_hdr(0x0600D878);
                      reloc_col_hdr(0x0600D8F8);
                      reloc_col_hdr(0x0600DA10);
                      reloc_col_hdr(0x0600DD1C);
                      reloc_col_hdr(0x0600DE44);
                      reloc_col_hdr(0x0600DF78);
                      reloc_col_hdr(0x0600E1E8);
                      reloc_col_hdr(0x0600E2CC);
                      reloc_col_hdr(0x0600E380);
                      reloc_col_hdr(0x0600E430);
                      reloc_col_hdr(0x0600E568);
                      reloc_col_hdr(0x0600FAE8);
                      reloc_col_hdr(0x060120E8); break;
        case 0x002F:  reloc_col_hdr(0x06000280);
                      reloc_col_hdr(0x060005E0); break;
        case 0x0036:  reloc_col_hdr(0x06005780);
                      reloc_col_hdr(0x06006050);
                      reloc_col_hdr(0x06006460);
                      reloc_col_hdr(0x060066A8);
                      reloc_col_hdr(0x06007798); break;
        case 0x0037:  reloc_col_hdr(0x06012FD0); break;
        case 0x0038:  reloc_col_hdr(0x06000118); break;
        case 0x0040:  reloc_col_hdr(0x06000A1C);
                      reloc_col_hdr(0x06001830);
                      reloc_col_hdr(0x0600BA8C); break;
        case 0x004D:  reloc_col_hdr(0x060042D8); break;
        case 0x0059:  reloc_col_hdr(0x060003F0);
                      reloc_col_hdr(0x06000998);
                      reloc_col_hdr(0x06000ED0);
                      reloc_col_hdr(0x060015F8);
                      reloc_col_hdr(0x06001C58);
                      reloc_col_hdr(0x06001DE8);
                      reloc_col_hdr(0x060025A4);
                      reloc_col_hdr(0x06003590);
                      reloc_col_hdr(0x06007250);
                      reloc_col_hdr(0x060073F0);
                      reloc_col_hdr(0x060074EC); break;
        case 0x005C:  reloc_col_hdr(0x060054B8); break;
        case 0x005E:  reloc_col_hdr(0x06007888); break;
        case 0x0061:  reloc_col_hdr(0x06000658); break;
        case 0x0068:  reloc_col_hdr(0x06000330); break;
        case 0x0069:  reloc_col_hdr(0x06000118);
                      reloc_col_hdr(0x06004330);
                      reloc_col_hdr(0x060044D0);
                      reloc_col_hdr(0x06004780);
                      reloc_col_hdr(0x06004940);
                      reloc_col_hdr(0x06004B00);
                      reloc_col_hdr(0x06004CC0);
                      reloc_col_hdr(0x06005334);
                      reloc_col_hdr(0x06005E30);
                      reloc_col_hdr(0x06006F70);
                      reloc_col_hdr(0x060081D0);
                      reloc_col_hdr(0x06008D10);
                      reloc_col_hdr(0x06009168);
                      reloc_col_hdr(0x06009CD0);
                      reloc_col_hdr(0x0600A7F4);
                      reloc_col_hdr(0x0600A938);
                      reloc_col_hdr(0x0600E408);
                      reloc_col_hdr(0x0600ED7C);
                      reloc_col_hdr(0x060108B8);
                      reloc_col_hdr(0x06010E10);
                      reloc_col_hdr(0x060131C4); break;
        case 0x006A:  reloc_col_hdr(0x06000EE8);
                      reloc_col_hdr(0x06001238); break;
        case 0x006B:  reloc_col_hdr(0x060003F0);
                      reloc_col_hdr(0x06001C1C);
                      reloc_col_hdr(0x06002594);
                      reloc_col_hdr(0x06002854);
                      reloc_col_hdr(0x06002920); break;
        case 0x006C:  reloc_col_hdr(0x060003C4);
                      reloc_col_hdr(0x060025FC); break;
        case 0x006F:  reloc_col_hdr(0x06003490); break;
        case 0x0070:  reloc_col_hdr(0x060043D0); break;
        case 0x0071:  reloc_col_hdr(0x06006078); break;
        case 0x0072:  reloc_col_hdr(0x06001AF8);
                      reloc_col_hdr(0x0600221C);
                      reloc_col_hdr(0x060035F8);
                      reloc_col_hdr(0x060037D8);
                      reloc_col_hdr(0x060063B8);
                      reloc_col_hdr(0x060087AC);
                      reloc_col_hdr(0x060089E0); break;
        case 0x0074:  reloc_col_hdr(0x06001904);
                      reloc_col_hdr(0x06002FD8);
                      reloc_col_hdr(0x060039D4); break;
        case 0x0076:  reloc_col_hdr(0x060000C0); break;
        case 0x0081:  reloc_col_hdr(0x06001F10); break;
        case 0x0082:  reloc_col_hdr(0x06000350);
                      reloc_col_hdr(0x060006D0); break;
        case 0x008D:  reloc_col_hdr(0x06000870);
                      reloc_col_hdr(0x06000C2C);
                      reloc_col_hdr(0x06001830);
                      reloc_col_hdr(0x06001AB8); break;
        case 0x0096:  reloc_col_hdr(0x06005048);
                      reloc_col_hdr(0x06005580);
                      reloc_col_hdr(0x06005CF8);
                      reloc_col_hdr(0x06008CE0); break;
        case 0x0099:  reloc_col_hdr(0x06007860); break;
        case 0x009A:  reloc_col_hdr(0x0600169C); break;
        case 0x009C:  reloc_col_hdr(0x06000D68); break;
        case 0x00A1:  reloc_col_hdr(0x060128D8);
                      reloc_col_hdr(0x06012BA4);
                      reloc_col_hdr(0x060133EC); break;
        case 0x00A2:  reloc_col_hdr(0x06000428); break;
        case 0x00AC:  reloc_col_hdr(0x06001A70); break;
        case 0x00AE:  reloc_col_hdr(0x0600283C);
                      reloc_col_hdr(0x06005520);
                      reloc_col_hdr(0x06007580);
                      reloc_col_hdr(0x06008458); break;
        case 0x00AF:  reloc_col_hdr(0x06000368);
                      reloc_col_hdr(0x06000534);
                      reloc_col_hdr(0x06002154);
                      reloc_col_hdr(0x0600261C);
                      reloc_col_hdr(0x06002FE4); break;
        case 0x00B1:  reloc_col_hdr(0x06000A38); break;
        case 0x00E2:  reloc_col_hdr(0x060180F8); break;
        case 0x00F0:  reloc_col_hdr(0x06000348);
                      reloc_col_hdr(0x060004D0); break;
        case 0x00F1:  reloc_col_hdr(0x060004A8);
                      reloc_col_hdr(0x06005C4C);
                      reloc_col_hdr(0x0600C4C8);
                      reloc_col_hdr(0x0600D7E8);
                      reloc_col_hdr(0x0600E710);
                      reloc_col_hdr(0x0600F208);
                      reloc_col_hdr(0x0601167C);
                      reloc_col_hdr(0x06012508); break;
        case 0x00F9:  reloc_col_hdr(0x0600075C); break;
        case 0x0100:  reloc_col_hdr(0x06001438); break;
        case 0x0112:  reloc_col_hdr(0x06000C98); break;
        case 0x0113:  reloc_col_hdr(0x06002590);
                      reloc_col_hdr(0x060038FC); break;
        case 0x011B:  reloc_col_hdr(0x06000360); break;
        case 0x011C:  reloc_col_hdr(0x06000578);
                      reloc_col_hdr(0x06000730); break;
        case 0x011D:  reloc_col_hdr(0x060003D0); break;
        case 0x011E:  reloc_col_hdr(0x060005DC); break;
        case 0x0125:  reloc_col_hdr(0x06007564); break;
        case 0x0129:  reloc_col_hdr(0x060011B8); break;
        case 0x0130:  reloc_col_hdr(0x06000DB8); break;
        case 0x013A:  reloc_col_hdr(0x06000D78); break;
        case 0x014B:  reloc_col_hdr(0x06000170); break;
        case 0x014C:  reloc_col_hdr(0x06005CB8);
                      reloc_col_hdr(0x060091E4); break;
        case 0x0156:  reloc_col_hdr(0x060011D4); break;
        case 0x0161:  reloc_col_hdr(0x06000918);
                      reloc_col_hdr(0x060012C0); break;
        case 0x0162:  reloc_col_hdr(0x060011EC);
                      reloc_col_hdr(0x0600238C); break;
        case 0x0166:  reloc_col_hdr(0x06000908);
                      reloc_col_hdr(0x06000AF0); break;
        case 0x016C:  reloc_col_hdr(0x06000D48);
                      reloc_col_hdr(0x06001430); break;
        case 0x016F:  reloc_col_hdr(0x06001A58); break;
        case 0x0170:  reloc_col_hdr(0x06000B70); break;
        case 0x0178:  reloc_col_hdr(0x06000CB8);
                      reloc_col_hdr(0x06001B00);
                      reloc_col_hdr(0x06001C40); break;
        case 0x0179:  reloc_col_hdr(0x06004618);
                      reloc_col_hdr(0x0600C080);
                      reloc_col_hdr(0x0600ECD8); break;
        case 0x0180:  reloc_col_hdr(0x06001A38);
                      reloc_col_hdr(0x06003C64); break;
        case 0x0181:  reloc_col_hdr(0x06001C58);
                      reloc_col_hdr(0x06001DA8); break;
        case 0x0185:  reloc_col_hdr(0x06001B70);
                      reloc_col_hdr(0x06001F70);
                      reloc_col_hdr(0x06002448);
                      reloc_col_hdr(0x06002850);
                      reloc_col_hdr(0x06002D28);
                      reloc_col_hdr(0x06002FE4);
                      reloc_col_hdr(0x060033E0);
                      reloc_col_hdr(0x06003AF0); break;
        case 0x0189:  reloc_col_hdr(0x0600C2D0); break;
        case 0x018A:  reloc_col_hdr(0x06000118); break;
        case 0x0190:  reloc_col_hdr(0x06000B30); break;
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
  serial_read(&p, (void*)z64_weather_state_addr, 0x0018);
  serial_read(&p, &z64_temp_day_speed, sizeof(z64_temp_day_speed));

  /* hazard state */
  serial_read(&p, (void*)z64_hazard_state_addr, 0x0008);

  /* timer state */
  serial_read(&p, (void*)z64_timer_state_addr, 0x0008);

  /* hud state */
  serial_read(&p, (void*)z64_hud_state_addr, 0x0008);

  /* letterboxing */
  serial_read(&p, &z64_letterbox_target, sizeof(z64_letterbox_target));
  serial_read(&p, &z64_letterbox_current, sizeof(z64_letterbox_current));
  serial_read(&p, &z64_letterbox_time, sizeof(z64_letterbox_time));

  /* sound state */
  serial_read(&p, (void*)z64_sound_state_addr, 0x004C);

  /* event state */
  serial_read(&p, (void*)z64_event_state_1_addr, 0x0008);
  serial_read(&p, (void*)z64_event_state_2_addr, 0x0004);
  /* event camera parameters */
  for (int i = 0; i < 24; ++i)
    serial_read(&p, (void*)(z64_event_camera_addr + 0x28 * i + 0x10), 0x0018);

  /* oob timer */
  serial_read(&p, &z64_oob_timer, sizeof(z64_oob_timer));

  /* countdown to gameover screen */
  serial_read(&p, &z64_gameover_countdown, sizeof(z64_gameover_countdown));

  /* rng */
  serial_read(&p, &z64_random, sizeof(z64_random));

  /* camera state */
  serial_read(&p, (void*)z64_camera_state_addr, 0x0020);

  /* cutscene state */
  serial_read(&p, (void*)z64_cs_state_addr, 0x0140);
  /* cutscene message id */
  serial_read(&p, (void*)z64_cs_message_addr, 0x0008);

  /* message state */
  serial_read(&p, (void*)z64_message_state_addr, 0x0028);

  /* load textures */
  zu_getfile_idx(940, z64_game.if_ctxt.parameter);
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
    uint32_t message_static = z64_ftab[18].vrom_start;
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
    if (foreground_image == 0)
      zu_getfile(z64_ftab[20].vrom_start, tex_buf + 0x1000, 0x1200);
    else if (icon_idx >= 0x66) {
      zu_getfile(z64_ftab[9].vrom_start + (icon_idx - 0x66) * 0x900,
                 tex_buf + 0x1000, 0x900);
    }
    else if (icon_idx >= 0x00) {
      zu_getfile(z64_ftab[8].vrom_start + icon_idx * 0x1000,
                 tex_buf + 0x1000, 0x1000);
    }
  }
  /* clear unsaved textures */
  if (c_pause_objects && !p_pause_objects) {
    uint16_t (*img)[Z64_SCREEN_HEIGHT][Z64_SCREEN_WIDTH];
    img = (void*)z64_zimg_addr;
    for (int y = 0; y < Z64_SCREEN_HEIGHT; ++y)
      for (int x = 0; x < Z64_SCREEN_WIDTH; ++x)
        (*img)[y][x] = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
  }
  if (c_pause_objects && !c_gameover && (!p_pause_objects || p_gameover)) {
    uint16_t (*img)[112][64];
    img = z64_game.pause_ctxt.p13C;
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
    char *seq = (void*)(z64_afx_addr + 0x3530 + i * 0x0160);
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
  serial_read(&p, (void*)z64_sfx_mute_addr, 0x0008);
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
  serial_read(&p, (void*)z64_ocarina_state_addr, 0x0060);
  /* load song state */
  serial_read(&p, (void*)z64_song_state_addr, 0x00AC);
  serial_read(&p, (void*)z64_scarecrow_song_addr, 0x0140);
  serial_read(&p, (void*)z64_song_ptr_addr, 0x0004);
  serial_read(&p, (void*)z64_staff_notes_addr, 0x001E);
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
