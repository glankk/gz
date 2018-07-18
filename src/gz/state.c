#include <stdlib.h>
#include <stdint.h>
#include <mips.h>
#include <n64.h>
#include <set/set.h>
#include "sys.h"
#include "zu.h"
#include "z64.h"

char          *fbuf_;
unsigned int   fpos_;

#if 1

#define open  open_
#define write write_
#define read  read_
#define close close_

static int open_(const char *path, int oflags, ...)
{
  if (!fbuf_)
    fbuf_ = malloc(512 * 1024);
  fpos_ = 0;
  return 0;
}

static int write_(int fildes, void *buf, unsigned int nbyte)
{
  memcpy(&fbuf_[fpos_], buf, nbyte);
  fpos_ += nbyte;
  return nbyte;
}

static int read_(int fildes, void *buf, unsigned int nbyte)
{
  memcpy(buf, &fbuf_[fpos_], nbyte);
  fpos_ += nbyte;
  return nbyte;
}

static int close_(int fildes)
{
  return 0;
}

#endif


typedef void (*z64_CreateStaticCollision_proc)(z64_col_ctxt_t *col_ctxt, z64_game_t *game, z64_col_lut_t *col_lut);
typedef void (*z64_LoadMinimap_proc)(z64_game_t *game, int room_idx);
typedef void (*z64_LoadActionLabel_proc)(z64_if_ctxt_t *if_ctxt, uint16_t action_idx, int button_idx);
typedef void (*z64_InitPauseObjects_proc)(z64_game_t *game, void *addr, void *s72C);
typedef void (*z64_CreateSkyGfx_proc)(z64_sky_ctxt_t *sky_ctxt, int skybox_type);
typedef void (*z64_CreateSkyVtx_proc)(z64_sky_ctxt_t *sky_ctxt, int a1);
typedef void (*z64_StopSfx_proc)(void);
typedef uint32_t (*z64_LoadOverlay_proc)(uint32_t vrom_start, uint32_t vrom_end,
                                         uint32_t vram_start, uint32_t vram_end,
                                         void *dst);

#if Z64_VERSION == Z64_OOT10

#define z64_CreateStaticCollision_addr          0x8002E70C
#define z64_LoadMinimap_addr                    0x8006BF04
#define z64_LoadActionLabel_addr                0x80071D24
#define z64_InitPauseObjects_addr               0x8007C09C
#define z64_CreateSkyGfx_addr                   0x80095A9C
#define z64_CreateSkyVtx_addr                   0x80095C4C
#define z64_StopSfx_addr                        0x800A0290
#define z64_LoadOverlay_addr                    0x800CCBB8
#define z64_part_ovl_tab_addr                   0x800E7C40
#define z64_part_space_addr                     0x800E7B40
#define z64_part_max_addr                       0x800E7B48
#define z64_actor_ovl_tab_addr                  0x800E8530
#define z64_day_speed_addr                      0x800F1650
#define z64_sky_images_addr                     0x800F184C
#define z64_map_mark_ovl_addr                   0x800F1BF8
#define z64_minimap_entrance_x_addr             0x800F5530
#define z64_minimap_entrance_y_addr             0x800F5534
#define z64_minimap_entrance_r_addr             0x800F5538
#define z64_temp_day_speed_addr                 0x800F7638
#define z64_letterbox_target_addr               0x800FE474
#define z64_letterbox_current_addr              0x800FE478
#define z64_play_ovl_tab_addr                   0x800FE480
#define z64_play_ovl_ptr_addr                   0x800FE4BC
#define z64_sfx_write_pos_addr                  0x80104360
#define z64_sfx_read_pos_addr                   0x80104364
#define z64_light_queue_addr                    0x8011BD60
#define z64_game_arena_addr                     0x8011BEF0
#define z64_mtx_stack_addr                      0x80121200
#define z64_mtx_stack_top_addr                  0x80121204

#elif Z64_VERSION == Z64_OOT11

#define z64_CreateStaticCollision_addr          0x8002E70C
#define z64_LoadMinimap_addr                    0x8006BF04
#define z64_LoadActionLabel_addr                0x80071D24
#define z64_InitPauseObjects_addr               0x8007C09C
#define z64_CreateSkyGfx_addr                   0x80095AAC
#define z64_CreateSkyVtx_addr                   0x80095C5C
#define z64_StopSfx_addr                        0x800A02A0
#define z64_LoadOverlay_addr                    0x800CCD78
#define z64_part_ovl_tab_addr                   0x800E7E00
#define z64_part_space_addr                     0x800E7D00
#define z64_part_max_addr                       0x800E7D08
#define z64_actor_ovl_tab_addr                  0x800E86F0
#define z64_day_speed_addr                      0x800F1810
#define z64_sky_images_addr                     0x800F1A0C
#define z64_map_mark_ovl_addr                   0x800F1DB8
#define z64_minimap_entrance_x_addr             0x800F56F0
#define z64_minimap_entrance_y_addr             0x800F56F4
#define z64_minimap_entrance_r_addr             0x800F56F8
#define z64_temp_day_speed_addr                 0x800F77F8
#define z64_letterbox_target_addr               0x800FE634
#define z64_letterbox_current_addr              0x800FE638
#define z64_play_ovl_tab_addr                   0x800FE640
#define z64_play_ovl_ptr_addr                   0x800FE67C
#define z64_sfx_write_pos_addr                  0x80104520
#define z64_sfx_read_pos_addr                   0x80104524
#define z64_light_queue_addr                    0x8011BF20
#define z64_game_arena_addr                     0x8011C0B0
#define z64_mtx_stack_addr                      0x801213C0
#define z64_mtx_stack_top_addr                  0x801213C4

#elif Z64_VERSION == Z64_OOT12

#define z64_CreateStaticCollision_addr          0x8002ED4C
#define z64_LoadMinimap_addr                    0x8006C564
#define z64_LoadActionLabel_addr                0x800723AC
#define z64_InitPauseObjects_addr               0x8007C72C
#define z64_CreateSkyGfx_addr                   0x8009618C
#define z64_CreateSkyVtx_addr                   0x8009633C
#define z64_StopSfx_addr                        0x800A0980
#define z64_LoadOverlay_addr                    0x800CD3F8
#define z64_part_ovl_tab_addr                   0x800E8280
#define z64_part_space_addr                     0x800E8180
#define z64_part_max_addr                       0x800E8188
#define z64_actor_ovl_tab_addr                  0x800E8B70
#define z64_day_speed_addr                      0x800F1C90
#define z64_sky_images_addr                     0x800F1E8C
#define z64_map_mark_ovl_addr                   0x800F2238
#define z64_minimap_entrance_x_addr             0x800F5B70
#define z64_minimap_entrance_y_addr             0x800F5B74
#define z64_minimap_entrance_r_addr             0x800F5B78
#define z64_temp_day_speed_addr                 0x800F7C80
#define z64_letterbox_target_addr               0x800FEAC4
#define z64_letterbox_current_addr              0x800FEAC8
#define z64_play_ovl_tab_addr                   0x800FEAD0
#define z64_play_ovl_ptr_addr                   0x800FEB0C
#define z64_sfx_write_pos_addr                  0x801049A0
#define z64_sfx_read_pos_addr                   0x801049A4
#define z64_light_queue_addr                    0x8011C410
#define z64_game_arena_addr                     0x8011C5A0
#define z64_mtx_stack_addr                      0x80121AD0
#define z64_mtx_stack_top_addr                  0x80121AD4

#endif

#define z64_part_ovl_tab        (*(z64_part_ovl_t(*)[37])     z64_part_ovl_tab_addr)
#define z64_part_space          (*(z64_part_t**)              z64_part_space_addr)
#define z64_part_max            (*(int32_t*)                  z64_part_max_addr)
#define z64_actor_ovl_tab       (*(z64_actor_ovl_t(*)[471])   z64_actor_ovl_tab_addr)
#define z64_day_speed           (*(uint16_t*)                 z64_day_speed_addr)
#define z64_sky_images          (*(z64_sky_image_t(*)[9])     z64_sky_images_addr)
#define z64_map_mark_ovl        (*(z64_map_mark_ovl_t*)       z64_map_mark_ovl_addr)
#define z64_minimap_entrance_x  (*(int16_t*)                  z64_minimap_entrance_x_addr)
#define z64_minimap_entrance_y  (*(int16_t*)                  z64_minimap_entrance_y_addr)
#define z64_minimap_entrance_r  (*(int16_t*)                  z64_minimap_entrance_r_addr)
#define z64_temp_day_speed      (*(uint16_t*)                 z64_temp_day_speed_addr)
#define z64_letterbox_target    (*(int32_t*)                  z64_letterbox_target_addr)
#define z64_letterbox_current   (*(int32_t*)                  z64_letterbox_current_addr)
#define z64_play_ovl_tab        (*(z64_play_ovl_t(*)[2])      z64_play_ovl_tab_addr)
#define z64_play_ovl_ptr        (*(z64_play_ovl_t*)           z64_play_ovl_ptr_addr)
#define z64_sfx_write_pos       (*(uint8_t*)                  z64_sfx_write_pos_addr)
#define z64_sfx_read_pos        (*(uint8_t*)                  z64_sfx_read_pos_addr)
#define z64_light_queue         (*(z64_light_queue_t*)        z64_light_queue_addr)
#define z64_game_arena          (*(z64_arena_t*)              z64_game_arena_addr)
#define z64_mtx_stack           (*(MtxF(**)[20])              z64_mtx_stack_addr)
#define z64_mtx_stack_top       (*(MtxF**)                    z64_mtx_stack_top_addr)

#define z64_CreateStaticCollision ( (z64_CreateStaticCollision_proc)  z64_CreateStaticCollision_addr)
#define z64_LoadMinimap           ( (z64_LoadMinimap_proc)            z64_LoadMinimap_addr)
#define z64_LoadActionLabel       ( (z64_LoadActionLabel_proc)        z64_LoadActionLabel_addr)
#define z64_InitPauseObjects      ( (z64_InitPauseObjects_proc)       z64_InitPauseObjects_addr)
#define z64_CreateSkyGfx          ( (z64_CreateSkyGfx_proc)           z64_CreateSkyGfx_addr)
#define z64_CreateSkyVtx          ( (z64_CreateSkyVtx_proc)           z64_CreateSkyVtx_addr)
#define z64_StopSfx               ( (z64_StopSfx_proc)                z64_StopSfx_addr)
#define z64_LoadOverlay           ( (z64_LoadOverlay_proc)            z64_LoadOverlay_addr)


static void (*z64_osStopThread)(OSThread *t) = (void*)0x80001F70;
static void (*z64_osStartThread)(OSThread *t) = (void*)0x80005EC0;


static void stop(void)
{
  /* disable interrupts */
#if 0
  __asm__ volatile ("mfc0 $t0, $12  \n"
                    "and  $t0, %0   \n"
                    "mtc0 $t0, $12  \n" :: "r"(~MIPS_STATUS_IE));
#elif 1
  //z64_osStopThread((void*)0x80007DD8); /* dmamgr */
  z64_osStopThread((void*)0x8011D318); /* sched */
  z64_osStopThread((void*)0x8011D580); /* padmgr */
  z64_osStopThread((void*)0x80120D60); /* audio */
#endif
#if 1
  volatile uint32_t *sp_status = (void*)0xA4040010;
  volatile uint32_t *pi_status = (void*)0xA4600010;
  /* wait for rsp to halt */
  while (!(*sp_status & 3))
    ;
  /* wait for dma busy */
  while (*pi_status & 3)
    ;
#endif
}

static void go(void)
{
  /* enable interrupts */
#if 0
  __asm__ volatile ("mfc0 $t0, $12  \n"
                    "or   $t0, %0   \n"
                    "mtc0 $t0, $12  \n" :: "r"(MIPS_STATUS_IE));
#elif 1
  //z64_osStartThread((void*)0x80007DD8); /* dmamgr */
  z64_osStartThread((void*)0x8011D318); /* sched */
  z64_osStartThread((void*)0x8011D580); /* padmgr */
  z64_osStartThread((void*)0x80120D60); /* audio */
#endif
}

static void save_ovl(int file, void *addr, size_t size)
{
  write(file, &addr, sizeof(addr));
  char *start = addr;
  char *end = start + size;
  uint32_t *hdr_off = (void*)(end - sizeof(*hdr_off));
  if (*hdr_off == 0)
    return;
  z64_ovl_hdr_t *hdr = (void*)(end - *hdr_off);
  void *data = start + hdr->text_size;
  void *bss = end;
  write(file, data, hdr->data_size);
  write(file, bss, hdr->bss_size);
}

static void load_ovl(int file, void **p_addr,
                     uint32_t vrom_start, uint32_t vrom_end,
                     uint32_t vram_start, uint32_t vram_end)
{
  void *addr = *p_addr;
  size_t size = vrom_end - vrom_start;
  void *load_addr;
  read(file, &load_addr, sizeof(load_addr));
  if (addr != load_addr) {
    addr = load_addr;
    *p_addr = addr;
    z64_LoadOverlay(vrom_start, vrom_end, vram_start, vram_end, addr);
  }
  char *start = addr;
  char *end = start + size;
  uint32_t *hdr_off = (void*)(end - sizeof(*hdr_off));
  if (*hdr_off == 0)
    return;
  z64_ovl_hdr_t *hdr = (void*)(end - *hdr_off);
  void *data = start + hdr->text_size;
  void *bss = end;
  read(file, data, hdr->data_size);
  read(file, bss, hdr->bss_size);
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
      z64_sky_image_t *image[2];
      uint8_t *image_idx = z64_game.sky_image_idx;
      uint32_t file_size;

      image[0] = &z64_sky_images[image_idx[0]];
      file_size = image[0]->tex_end - image[0]->tex_start;
      zu_getfile(image[0]->tex_start, sky_ctxt->textures[0], file_size);

      image[1] = &z64_sky_images[image_idx[1]];
      file_size = image[1]->tex_end - image[1]->tex_start;
      zu_getfile(image[1]->tex_start, sky_ctxt->textures[1], file_size);

      if ((image_idx[0] & 1) == (image_idx[0] & 4) >> 2) {
        file_size = image[0]->pal_end - image[0]->pal_start;
        zu_getfile(image[1]->pal_start,
                   sky_ctxt->palettes, file_size);
        zu_getfile(image[0]->pal_start,
                   sky_ctxt->palettes + file_size, file_size);
      }
      else {
        file_size = image[0]->pal_end - image[0]->pal_start;
        zu_getfile(image[0]->pal_start,
                   sky_ctxt->palettes, file_size);
        zu_getfile(image[1]->pal_start,
                   sky_ctxt->palettes + file_size, file_size);
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

static _Bool addr_comp(void *a, void *b)
{
  uint32_t *a_u32 = a;
  uint32_t *b_u32 = b;
  return *a_u32 < *b_u32;
}

void save_state(void)
{
  stop();
  int file = open("state.gzs", O_WRONLY | O_CREAT | O_TRUNC, 0777);

  int16_t sot = 0;
  int16_t eot = -1;
  /* save context */
  write(file, &z64_game, sizeof(z64_game));
  write(file, &z64_file, sizeof(z64_file));
  write(file, z64_file.gameinfo, sizeof(*z64_file.gameinfo));
  /* save overlays */
  int16_t n_ovl;
  struct set ovl_nodes;
  set_init(&ovl_nodes, sizeof(uint32_t), addr_comp);
  /* actor overlays */
  n_ovl = sizeof(z64_actor_ovl_tab) / sizeof(*z64_actor_ovl_tab);
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_actor_ovl_t *ovl = &z64_actor_ovl_tab[i];
    if (ovl->ptr) {
      write(file, &i, sizeof(i));
      write(file, &ovl->n_inst, sizeof(ovl->n_inst));
      save_ovl(file, ovl->ptr, ovl->vrom_end - ovl->vrom_start);
      set_insert(&ovl_nodes, &ovl->ptr);
    }
  }
  write(file, &eot, sizeof(eot));
  /* play overlays */
  n_ovl = sizeof(z64_play_ovl_tab) / sizeof(*z64_play_ovl_tab);
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_play_ovl_t *ovl = &z64_play_ovl_tab[i];
    if (ovl->ptr) {
      write(file, &i, sizeof(i));
      save_ovl(file, ovl->ptr, ovl->vrom_end - ovl->vrom_start);
      set_insert(&ovl_nodes, &ovl->ptr);
    }
  }
  write(file, &eot, sizeof(eot));
  write(file, &z64_play_ovl_ptr, sizeof(z64_play_ovl_ptr));
  /* particle overlays */
  n_ovl = sizeof(z64_part_ovl_tab) / sizeof(*z64_part_ovl_tab);
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_part_ovl_t *ovl = &z64_part_ovl_tab[i];
    if (ovl->ptr) {
      write(file, &i, sizeof(i));
      save_ovl(file, ovl->ptr, ovl->vrom_end - ovl->vrom_start);
      set_insert(&ovl_nodes, &ovl->ptr);
    }
  }
  write(file, &eot, sizeof(eot));
  /* map mark overlay */
  if (z64_map_mark_ovl.ptr) {
    z64_map_mark_ovl_t *ovl = &z64_map_mark_ovl;
    write(file, &sot, sizeof(sot));
    save_ovl(file, ovl->ptr, ovl->vrom_end - ovl->vrom_start);
    set_insert(&ovl_nodes, &ovl->ptr);
  }
  write(file, &eot, sizeof(eot));

  /* save arena nodes */
  write(file, &z64_game_arena, sizeof(z64_game_arena));
  for (z64_arena_node_t *node = z64_game_arena.first_node;
       node; node = node->next)
  {
    write(file, &sot, sizeof(sot));
    write(file, &node->free, sizeof(node->free));
    write(file, &node->size, sizeof(node->size));
    char *data = node->data;
    if (!set_get(&ovl_nodes, &data) && !node->free)
      write(file, data, node->size);
  }
  write(file, &eot, sizeof(eot));
  set_destroy(&ovl_nodes);

  /* save light queue */
  write(file, &z64_light_queue, sizeof(z64_light_queue));
  /* save particle space info */
  write(file, &z64_part_space, sizeof(z64_part_space));
  write(file, &z64_part_max, sizeof(z64_part_max));
  /* save matrix stack info */
  write(file, &z64_mtx_stack, sizeof(z64_mtx_stack));
  write(file, &z64_mtx_stack_top, sizeof(z64_mtx_stack_top));
  /* save segment table */
  write(file, &z64_stab, sizeof(z64_stab));

  /* save dynamic collision */
  write(file, z64_game.col_ctxt.dyn_list,
        z64_game.col_ctxt.dyn_list_max * sizeof(*z64_game.col_ctxt.dyn_list));
  write(file, z64_game.col_ctxt.dyn_poly,
        z64_game.col_ctxt.dyn_poly_max * sizeof(*z64_game.col_ctxt.dyn_poly));
  write(file, z64_game.col_ctxt.dyn_vtx,
        z64_game.col_ctxt.dyn_vtx_max * sizeof(*z64_game.col_ctxt.dyn_vtx));

  if (z64_game.elf_message)
    write(file, z64_game.elf_message, 0x0070);

  /* minimap details */
  write(file, &z64_minimap_entrance_x, sizeof(z64_minimap_entrance_x));
  write(file, &z64_minimap_entrance_y, sizeof(z64_minimap_entrance_y));
  write(file, &z64_minimap_entrance_r, sizeof(z64_minimap_entrance_r));

  /* day speed */
  write(file, &z64_day_speed, sizeof(z64_day_speed));
  write(file, &z64_temp_day_speed, sizeof(z64_temp_day_speed));

  /* letterboxing */
  write(file, &z64_letterbox_target, sizeof(z64_letterbox_target));
  write(file, &z64_letterbox_current, sizeof(z64_letterbox_current));

#if 0
  /* ocarina state */
  write(file, (void*)0x80102208, 0x60);
  write(file, (void*)0x80121F0C, 0xA8);

  /* cutscene state */
  write(file, (void*)0x8011BC20, 0x0140);
  /* cutscene text id */
  write(file, (void*)0x800EFCD0, 0x0002);
#endif

  _Bool save_gfx = 1;
  if (save_gfx) {
    write(file, &sot, sizeof(sot));
    int disp_idx = z64_ctxt.gfx->frame_count_1 & 1;
    void *disp = (void*)(z64_disp_addr + disp_idx * z64_disp_size);
    write(file, disp, z64_disp_size);
    struct zu_disp_p disp_p;
    zu_save_disp_p(&disp_p);
    write(file, &disp_p, sizeof(disp_p));
    write(file, &z64_ctxt.gfx->frame_count_1, sizeof(z64_ctxt.gfx->frame_count_1));
    write(file, &z64_ctxt.gfx->frame_count_2, sizeof(z64_ctxt.gfx->frame_count_2));
  }
  else
    write(file, &eot, sizeof(eot));

  //write(file, (void*)0x800E2FC0, 0x31E10);
  //write(file, (void*)0x8012143C, 0x41F4);
  //write(file, (void*)0x801DAA00, 0x1D4790);

  close(file);
  go();
}

void load_state(void)
{
  stop();
  int file = open("state.gzs", O_RDONLY);

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
  _Bool p_pause_objects = z64_game.pause_ctxt.state > 3;
  /* load context */
  read(file, &z64_game, sizeof(z64_game));
  read(file, &z64_file, sizeof(z64_file));
  read(file, z64_file.gameinfo, sizeof(*z64_file.gameinfo));
  /* load overlays */
  int16_t n_ovl;
  int16_t next_ovl;
  struct set ovl_nodes;
  set_init(&ovl_nodes, sizeof(uint32_t), addr_comp);
  /* actor overlays */
  n_ovl = sizeof(z64_actor_ovl_tab) / sizeof(*z64_actor_ovl_tab);
  read(file, &next_ovl, sizeof(next_ovl));
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_actor_ovl_t *ovl = &z64_actor_ovl_tab[i];
    if (i == next_ovl) {
      read(file, &ovl->n_inst, sizeof(ovl->n_inst));
      load_ovl(file, &ovl->ptr,
               ovl->vrom_start, ovl->vrom_end,
               ovl->vram_start, ovl->vram_end);
      set_insert(&ovl_nodes, &ovl->ptr);
      read(file, &next_ovl, sizeof(next_ovl));
    }
    else {
      ovl->n_inst = 0;
      ovl->ptr = NULL;
    }
  }
  /* play overlays */
  n_ovl = sizeof(z64_play_ovl_tab) / sizeof(*z64_play_ovl_tab);
  read(file, &next_ovl, sizeof(next_ovl));
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_play_ovl_t *ovl = &z64_play_ovl_tab[i];
    if (i == next_ovl) {
      load_ovl(file, &ovl->ptr,
               ovl->vrom_start, ovl->vrom_end,
               ovl->vram_start, ovl->vram_end);
      ovl->reloc_offset = (uint32_t)ovl->ptr - ovl->vram_start;
      set_insert(&ovl_nodes, &ovl->ptr);
      read(file, &next_ovl, sizeof(next_ovl));
    }
    else {
      ovl->ptr = NULL;
      ovl->reloc_offset = 0;
    }
  }
  read(file, &z64_play_ovl_ptr, sizeof(z64_play_ovl_ptr));
  /* particle overlays */
  n_ovl = sizeof(z64_part_ovl_tab) / sizeof(*z64_part_ovl_tab);
  read(file, &next_ovl, sizeof(next_ovl));
  for (int16_t i = 0; i < n_ovl; ++i) {
    z64_part_ovl_t *ovl = &z64_part_ovl_tab[i];
    if (i == next_ovl) {
      load_ovl(file, &ovl->ptr,
               ovl->vrom_start, ovl->vrom_end,
               ovl->vram_start, ovl->vram_end);
      set_insert(&ovl_nodes, &ovl->ptr);
      read(file, &next_ovl, sizeof(next_ovl));
    }
    else
      ovl->ptr = NULL;
  }
  /* map mark overlay */
  read(file, &next_ovl, sizeof(next_ovl));
  if (next_ovl == 0) {
    z64_map_mark_ovl_t *ovl = &z64_map_mark_ovl;
    load_ovl(file, &ovl->ptr,
             ovl->vrom_start, ovl->vrom_end,
             ovl->vram_start, ovl->vram_end);
    set_insert(&ovl_nodes, &ovl->ptr);
    read(file, &next_ovl, sizeof(next_ovl));
  }
  else
    z64_map_mark_ovl.ptr = NULL;

  /* load arena nodes */
  read(file, &z64_game_arena, sizeof(z64_game_arena));
  z64_arena_node_t *node = z64_game_arena.first_node;
  int16_t tag;
  read(file, &tag, sizeof(tag));
  while (node) {
    node->magic = 0x7373;
    node->arena = &z64_game_arena;
    node->filename = NULL,
    node->line = 0;
    node->thread_id = 4;
    read(file, &node->free, sizeof(node->free));
    read(file, &node->size, sizeof(node->size));
    char *data = node->data;
    if (!set_get(&ovl_nodes, &data) && !node->free)
      read(file, data, node->size);
    if (node == z64_game_arena.first_node)
      node->prev = NULL;
    read(file, &tag, sizeof(tag));
    if (tag == 0) {
      node->next = (void*)&node->data[node->size];
      node->next->prev = node;
    }
    else
      node->next = NULL;
    node = node->next;
  }
  set_destroy(&ovl_nodes);

  /* load light queue */
  read(file, &z64_light_queue, sizeof(z64_light_queue));
  /* load particle space info */
  read(file, &z64_part_space, sizeof(z64_part_space));
  read(file, &z64_part_max, sizeof(z64_part_max));
  /* load matrix stack info */
  read(file, &z64_mtx_stack, sizeof(z64_mtx_stack));
  read(file, &z64_mtx_stack_top, sizeof(z64_mtx_stack_top));
  /* load segment table */
  read(file, &z64_stab, sizeof(z64_stab));

  /* load scene */
  if (z64_game.scene_index != scene_index) {
    z64_scene_table_t *scene = &z64_scene_table[z64_game.scene_index];
    uint32_t size = scene->scene_vrom_end - scene->scene_vrom_start;
    zu_getfile(scene->scene_vrom_start, z64_game.scene_file, size);
    reloc_col_hdr((uint32_t)z64_game.col_ctxt.col_hdr);
  }
  /* load rooms */
  for (int i = 0; i < 2; ++i) {
    struct alloc *p_room = &room_list[i];
    z64_room_t *c_room = &z64_game.room_ctxt.rooms[i];
    int p_id = p_room->id;
    int c_id = c_room->index;
    void *p_ptr = p_room->ptr;
    void *c_ptr = c_room->file;
    if (c_id != -1 && (c_id != p_id || c_ptr != p_ptr)) {
      uint32_t start = z64_game.room_list[c_id].vrom_start;
      uint32_t end = z64_game.room_list[c_id].vrom_end;
      zu_getfile(start, c_ptr, end - start);
    }
  }

  /* load objects */
  _Bool c_pause_objects = z64_game.pause_ctxt.state > 3;
  _Bool dungeon_map = z64_game.scene_index < 0x000A ||
                      (z64_game.scene_index > 0x0010 &&
                       z64_game.scene_index < 0x0019);
  if (c_pause_objects) {
    z64_InitPauseObjects(&z64_game, z64_game.pause_ctxt.p13C,
                         &z64_game.pause_ctxt.s27C);
    /* ovl_kaleido_scope modifies icons of restricted items to be grayscale */
    /* relevant function: 8039AFBC */
    zu_getfile_idx(8, z64_game.pause_ctxt.icon_item);
    zu_getfile_idx(9, z64_game.pause_ctxt.icon_item_24);
    if (z64_file.language == 0)
      zu_getfile_idx(13, z64_game.pause_ctxt.icon_item_lang);
    else
      zu_getfile_idx(14, z64_game.pause_ctxt.icon_item_lang);
    if (dungeon_map) {
      zu_getfile_idx(11, z64_game.pause_ctxt.icon_item_s);
      uint32_t vaddr = z64_ftab[26].vrom_start;
      vaddr += z64_file.gameinfo->dungeon_map_floor * 0x07F8;
      zu_getfile(vaddr, z64_game.if_ctxt.minimap_tex, 0x07F8);
      vaddr += 0x07F8;
      zu_getfile(vaddr, z64_game.if_ctxt.minimap_tex + 0x0800, 0x07F8);
    }
    else
      zu_getfile_idx(10, z64_game.pause_ctxt.icon_item_s);
  }
  else {
    /* load normal objects in the object context */
    for (int i = 0; i < 19; ++i) {
      struct alloc *p_obj = &obj_list[i];
      z64_mem_obj_t *c_obj = &z64_game.obj_ctxt.objects[i];
      int p_id = p_obj->id;
      int c_id = c_obj->id;
      void *p_ptr = p_obj->ptr;
      void *c_ptr = c_obj->data;
      /* if the object in the current slot is different, or loaded to
         a different address, then reload it. if the previous state had
         pause screen objects loaded, all objects need to be reloaded. */
      if (c_id != 0 && (p_pause_objects || c_id != p_id || c_ptr != p_ptr)) {
        uint32_t start = z64_object_table[c_id].vrom_start;
        uint32_t end = z64_object_table[c_id].vrom_end;
        zu_getfile(start, c_ptr, end - start);
        /* object-specific initialization monkaS */
        z64_stab.seg[Z64_SEG_OBJ] = MIPS_KSEG0_TO_PHYS(c_ptr);
        switch(c_id) {
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
  }

  /* remove particles */
  for (int i = 0; i < z64_part_max; ++i) {
    z64_part_t *part = &z64_part_space[i];
    memset(part, 0, sizeof(*part));
    part->time = -1;
    part->priority = 0x80;
    part->part_id = 0x25;
  }

  /* dynamic collision */
  read(file, z64_game.col_ctxt.dyn_list,
       z64_game.col_ctxt.dyn_list_max * sizeof(*z64_game.col_ctxt.dyn_list));
  read(file, z64_game.col_ctxt.dyn_poly,
       z64_game.col_ctxt.dyn_poly_max * sizeof(*z64_game.col_ctxt.dyn_poly));
  read(file, z64_game.col_ctxt.dyn_vtx,
       z64_game.col_ctxt.dyn_vtx_max * sizeof(*z64_game.col_ctxt.dyn_vtx));

  /* static collision */
  z64_game.col_ctxt.stc_list_pos = 0;
  z64_CreateStaticCollision(&z64_game.col_ctxt, &z64_game,
                            z64_game.col_ctxt.stc_lut);

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
    read(file, z64_game.elf_message, 0x0070);

  /* minimap details */
  read(file, &z64_minimap_entrance_x, sizeof(z64_minimap_entrance_x));
  read(file, &z64_minimap_entrance_y, sizeof(z64_minimap_entrance_y));
  read(file, &z64_minimap_entrance_r, sizeof(z64_minimap_entrance_r));

  /* day speed */
  read(file, &z64_day_speed, sizeof(z64_day_speed));
  read(file, &z64_temp_day_speed, sizeof(z64_temp_day_speed));

  /* letterboxing */
  read(file, &z64_letterbox_target, sizeof(z64_letterbox_target));
  read(file, &z64_letterbox_current, sizeof(z64_letterbox_current));

#if 0
  /* ocarina state */
  read(file, (void*)0x80102208, 0x60);
  read(file, (void*)0x80121F0C, 0xA8);

  /* cutscene state */
  read(file, (void*)0x8011BC20, 0x0140);
  /* cutscene text id */
  read(file, (void*)0x800EFCD0, 0x0002);
#endif

  /* stop sound effects */
  /* importantly, this removes all sound effect control points, which prevents
     floating-point exception crashes due to dangling pointers in the cp's. */
  z64_StopSfx();
  /* cancel pending sound effects */
  z64_sfx_read_pos = z64_sfx_write_pos;

  /* load textures */
  zu_getfile_idx(940, z64_game.if_ctxt.parameter);
  for (int i = 0; i < 4; ++i)
    if (z64_file.button_items[i] != Z64_ITEM_NULL)
      z64_UpdateItemButton(&z64_game, i);
  z64_LoadActionLabel(&z64_game.if_ctxt, z64_game.if_ctxt.a_action,
                      Z64_ACTIONBTN_A);
  z64_LoadActionLabel(&z64_game.if_ctxt, z64_game.if_ctxt.b_label % 0x1D,
                      Z64_ACTIONBTN_B);
  z64_LoadActionLabel(&z64_game.if_ctxt, 0x0003, Z64_ACTIONBTN_START);
  if (!c_pause_objects || !dungeon_map)
    z64_LoadMinimap(&z64_game, z64_game.room_ctxt.rooms[0].index);

#if 0
  if (c_pause_objects && !p_pause_objects) {
    uint16_t (*p)[240][320] = (void*)0x8012BE40;
    for (int y = 0; y < 240; ++y) {
      for (int x = 0; x < 320; ++x) {
        (*p)[y][x] = GPACK_RGBA5551(y * 0x1F / 239, 0x00, x * 0x1F / 319, 0x00);
      }
    }
  }
#endif

  //read(file, (void*)0x800E2FC0, 0x31E10);
  //read(file, (void*)0x8012143C, 0x41F4);
  //read(file, (void*)0x801DAA00, 0x1D4790);

  int16_t next_gfx;
  read(file, &next_gfx, sizeof(next_gfx));
  if (next_gfx == 0) {
    int disp_idx = z64_ctxt.gfx->frame_count_1 & 1;
    void *disp = (void*)(z64_disp_addr + disp_idx * z64_disp_size);
    read(file, disp, z64_disp_size);
    struct zu_disp_p disp_p;
    read(file, &disp_p, sizeof(disp_p));
    zu_load_disp_p(&disp_p);
    uint32_t frame_count_1;
    uint32_t frame_count_2;
    read(file, &frame_count_1, sizeof(frame_count_1));
    read(file, &frame_count_2, sizeof(frame_count_2));
    zu_reloc_gfx(frame_count_1 & 1, frame_count_2 & 1);
  }
  else {
    /* kill frame */
    z64_ctxt.gfx->work.p = z64_ctxt.gfx->work.buf;
    gDPFullSync(z64_ctxt.gfx->work.p++);
    gSPEndDisplayList(z64_ctxt.gfx->work.p++);
  }

  gDPFullSync(z64_ctxt.gfx->work_c);
  gSPEndDisplayList(z64_ctxt.gfx->work_c + 1);
  z64_ctxt.gfx->work_c_size = 0x10;

  close(file);
  go();
}
