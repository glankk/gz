#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <n64.h>
#include "flags.h"
#include "gfx.h"
#include "gz.h"
#include "mem.h"
#include "menu.h"
#include "rdb.h"
#include "ucode.h"
#include "z64.h"

struct actor_debug_info
{
  struct menu_item     *edit_item;
  uint8_t               type;
  uint8_t               index;
};

struct actor_spawn_info
{
  uint16_t              actor_no;
  uint16_t              variable;
  int32_t               x;
  int32_t               y;
  int32_t               z;
  uint16_t              rx;
  uint16_t              ry;
  uint16_t              rz;
};

static int byte_optionmod_proc(struct menu_item *item,
                               enum menu_callback_reason reason,
                               void *data)
{
  uint8_t *v = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_option_get(item) != *v)
      menu_option_set(item, *v);
  }
  else if (reason == MENU_CALLBACK_DEACTIVATE)
    *v = menu_option_get(item);
  return 0;
}

static int halfword_mod_proc(struct menu_item *item,
                             enum menu_callback_reason reason,
                             void *data)
{
  uint16_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static int word_mod_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  uint32_t *p = data;
  if (reason == MENU_CALLBACK_THINK_INACTIVE) {
    if (menu_intinput_get(item) != *p)
      menu_intinput_set(item, *p);
  }
  else if (reason == MENU_CALLBACK_CHANGED)
    *p = menu_intinput_get(item);
  return 0;
}

static _Bool heap_node_validate(z64_arena_node_t *node)
{
  return node->magic == 0x7373 &&
         (!node->next ||
          ((uint32_t)node->next >= 0x80000000 &&
           (uint32_t)node->next < 0x80400000 &&
           (uint32_t)node->next > (uint32_t)node &&
           (uint32_t)node->next - (uint32_t)node == node->size + 0x30)) &&
         (!node->prev ||
          ((uint32_t)node->prev >= 0x80000000 &&
           (uint32_t)node->prev < 0x80400000 &&
           (uint32_t)node->prev < (uint32_t)node));
}

static int heap_draw_proc(struct menu_item *item,
                          struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int ch = menu_get_cell_height(item->owner, 1);
  /* collect heap data */
  const uint32_t heap_start = (uint32_t)&z64_link - 0x30;
  size_t max_alloc = 0;
  size_t min_alloc = 0;
  size_t total_alloc = 0;
  size_t max_avail = 0;
  size_t min_avail = 0;
  size_t total_avail = 0;
  size_t overhead = 0;
  size_t total = 0;
  z64_arena_node_t *p = (void*)heap_start;
  z64_arena_node_t *error_node = NULL;
  int node_count_total = 0;
  int node_count_free = 0;
  while (p) {
    ++node_count_total;
    if (!heap_node_validate(p)) {
      error_node = p;
      break;
    }
    if (p->free) {
      ++node_count_free;
      total_avail += p->size;
      if (p->size > max_avail)
        max_avail = p->size;
      if (min_avail == 0 || p->size < min_avail)
        min_avail = p->size;
    }
    else {
      total_alloc += p->size;
      if (p->size > max_alloc)
        max_alloc = p->size;
      if (min_alloc == 0 || p->size < min_alloc)
        min_alloc = p->size;
    }
    overhead += 0x30;
    total += p->size + 0x30;
    p = p->next;
  }
  /* show heap data */
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  gfx_printf(font, x, y + ch * 0, "node count     %i (%i free)",
             node_count_total, node_count_free);
  gfx_printf(font, x, y + ch * 1, "allocated      0x%8" PRIx32, total_alloc);
  gfx_printf(font, x, y + ch * 2, "available      0x%8" PRIx32, total_avail);
  gfx_printf(font, x, y + ch * 3, "max allocated  0x%8" PRIx32, max_alloc);
  gfx_printf(font, x, y + ch * 4, "min allocated  0x%8" PRIx32, min_alloc);
  gfx_printf(font, x, y + ch * 5, "max available  0x%8" PRIx32, max_avail);
  gfx_printf(font, x, y + ch * 6, "min available  0x%8" PRIx32, min_avail);
  gfx_printf(font, x, y + ch * 7, "overhead       0x%8" PRIx32, overhead);
  gfx_printf(font, x, y + ch * 8, "total          0x%8" PRIx32, total);
  if (error_node) {
    gfx_printf(font, x, y + ch * 9, "erroneous node 0x%08" PRIx32,
               (uint32_t)error_node);
  }
  /* plot graph */
  static struct gfx_texture *t = NULL;
  if (!t)
    t = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 1, 9, 1);
  size_t graph_width = t->tile_width * t->tiles_x;
  uint16_t *d = t->data;
  p = (void*)heap_start;
  while (p) {
    if (p == error_node)
      break;
    size_t b = (uint32_t)p - heap_start;
    size_t e = b + p->size + 0x30;
    b = b * graph_width / total;
    e = e * graph_width / total;
    for (size_t i = b; i < e; ++i) {
      uint16_t c;
      if (p->free)
        c = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
      else
        c = GPACK_RGBA5551(0x1F, 0x1F - p->size * 0x1F / max_alloc,
                           i == b ? 0x1F : 0x00, 0x01);
      d[i] = c;
    }
    p = p->next;
  }
  /* draw graph */
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, alpha));
  for (int i = 0; i < t->tiles_x; ++i) {
    struct gfx_sprite s =
    {
      t, i,
      x + t->tile_width * i,
      y + ch * 10,
      1.f, 16.f,
    };
    gfx_sprite_draw(&s);
  }
  return 1;
}

static int disp_draw_proc(struct menu_item *item,
                          struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  for (int i = 0; i < 4; ++i) {
    /* plot graph */
    static struct gfx_texture *t[4];
    if (!t[i])
      t[i] = gfx_texture_create(G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 1, 2, 1);
    size_t graph_width = t[i]->tile_width * t[i]->tiles_x;
    uint16_t *d = t[i]->data;
    size_t b = gz.disp_hook_p[i] * graph_width / gz.disp_hook_size[i];
    size_t e = gz.disp_hook_d[i] * graph_width / gz.disp_hook_size[i];
    for (size_t j = 0; j < graph_width; ++j) {
      uint16_t c;
      if (j < b) {
        if (j >= e)
          c = GPACK_RGBA5551(0xFF, 0x00, 0x00, 0x01);
        else
          c = GPACK_RGBA5551(0x00, 0xFF, 0x00, 0x01);
      }
      else if (j >= e)
        c = GPACK_RGBA5551(0x00, 0x00, 0xFF, 0x01);
      else
        c = GPACK_RGBA5551(0x00, 0x00, 0x00, 0x00);
      d[j] = c;
    }
    /* display info */
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
    static const char *disp_name[4] =
    {
      "work",
      "poly_opa",
      "poly_xlu",
      "overlay",
    };
    gfx_printf(font, x, y + ch * i * 2, "%s", disp_name[i]);
    gfx_printf(font, x + cw * 2, y + ch * (i * 2 + 1),
               "0x%04" PRIx32 "  0x%04" PRIx32,
               gz.disp_hook_size[i], gz.disp_hook_p[i]);
    gfx_printf(font, x + cw * 2 + 192, y + ch * (i * 2 + 1),
               "0x%04" PRIx32, gz.disp_hook_size[i] - gz.disp_hook_d[i]);
    /* draw graph */
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGBA8888(0xFF, 0xFF, 0xFF, alpha));
    for (int j = 0; j < t[i]->tiles_x; ++j) {
      struct gfx_sprite s =
      {
        t[i], j,
        x + cw * 2 + 120 + t[i]->tile_width * j,
        y + ch * i * 2 + 1,
        1.f, ch - 2,
      };
      gfx_sprite_draw(&s);
    }
  }
  return 1;
}

static void push_object_proc(struct menu_item *item, void *data)
{
  int16_t object_id = menu_intinput_get(data);
  if (object_id == 0)
    return;
  int n = z64_game.obj_ctxt.n_objects++;
  z64_mem_obj_t *obj = z64_game.obj_ctxt.objects;
  obj[n].id = -object_id;
  obj[n].getfile.vrom_addr = 0;
  size_t size = z64_object_table[object_id].vrom_end -
                z64_object_table[object_id].vrom_start;
  obj[n + 1].data = (char*)obj[n].data + size;
}

static void pop_object_proc(struct menu_item *item, void *data)
{
  if (z64_game.obj_ctxt.n_objects > 0)
    z64_game.obj_ctxt.objects[--z64_game.obj_ctxt.n_objects].id = 0;
}

static int objects_draw_proc(struct menu_item *item,
                             struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  char *s = z64_game.obj_ctxt.obj_space_start;
  char *e = z64_game.obj_ctxt.obj_space_end;
  char *p = z64_game.obj_ctxt.objects[z64_game.obj_ctxt.n_objects].data;
  size_t max_objects = sizeof(z64_game.obj_ctxt.objects) /
                       sizeof(*z64_game.obj_ctxt.objects) - 1;
  gfx_printf(font, x, y + ch * 0, "objects   %i / %i",
             z64_game.obj_ctxt.n_objects, max_objects);
  gfx_printf(font, x, y + ch * 1, "allocated %08" PRIx32, p - s);
  gfx_printf(font, x, y + ch * 2, "available %08" PRIx32, e - p);
  gfx_printf(font, x, y + ch * 3, "total     %08" PRIx32, e - s);
  for (int i = 0; i < z64_game.obj_ctxt.n_objects; ++i) {
    gfx_printf(font, x + cw * (i % 2) * 16, y + ch * (i / 2 + 5),
               "%08" PRIx32 " %04" PRIx16,
               z64_game.obj_ctxt.objects[i].data,
               z64_game.obj_ctxt.objects[i].id);
  }
  return 1;
}

static void actor_index_dec_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *adi = data;
  int type = adi->type;
  int index = adi->index;
  do {
    if (--index >= 0)
      break;
    type = (type + 12 - 1) % 12;
    index = z64_game.actor_list[type].length;
  } while (type != adi->type || index != adi->index);
  adi->type = type;
  adi->index = index;
}

static void actor_index_inc_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *adi = data;
  int type = adi->type;
  int index = adi->index + 1;
  do {
    if (index < z64_game.actor_list[type].length)
      break;
    type = (type + 1) % 12;
    index = 0;
  } while (type != adi->type || index != adi->index);
  adi->type = type;
  adi->index = index;
}

static int actor_draw_proc(struct menu_item *item,
                           struct menu_draw_params *draw_params)
{
  struct actor_debug_info *adi = item->data;
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int cw = menu_get_cell_width(item->owner, 1);
  int ch = menu_get_cell_height(item->owner, 1);
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(color, alpha));
  uint32_t max = z64_game.actor_list[adi->type].length;
  if (adi->index >= max) {
    if (max > 0)
      adi->index = max - 1;
    else
      adi->index = 0;
  }
  gfx_printf(font, x + cw * 14, y, "%i / %i", adi->index, max);
  if (adi->index < max) {
    z64_actor_t *actor = z64_game.actor_list[adi->type].first;
    for (int i = 0; i < adi->index; ++i)
      actor = actor->next;
    sprintf(adi->edit_item->text, "%08" PRIx32, (uint32_t)actor);
    gfx_printf(font, x + cw * 10, y + ch * 1, "%04" PRIx16, actor->actor_id);
    gfx_printf(font, x, y + ch * 2, "variable  %04" PRIx16, actor->variable);

    {
      Mtx m;
      {
        MtxF mf;
        guTranslateF(&mf, actor->pos_2.x, actor->pos_2.y, actor->pos_2.z);
        MtxF mt;
        guRotateRPYF(&mt,
                     actor->rot_2.x * M_PI / 0x8000,
                     actor->rot_2.y * M_PI / 0x8000,
                     actor->rot_2.z * M_PI / 0x8000);
        guMtxCatF(&mt, &mf, &mf);
        guMtxF2L(&mf, &m);
      }
      Vtx v[6] =
      {
        gdSPDefVtxC(-8192, 0,      0,      0, 0, 0xFF, 0x00, 0x00, 0x80),
        gdSPDefVtxC(8192,  0,      0,      0, 0, 0xFF, 0x00, 0x00, 0x80),
        gdSPDefVtxC(0,      -8192, 0,      0, 0, 0x00, 0xFF, 0x00, 0x80),
        gdSPDefVtxC(0,      8192,  0,      0, 0, 0x00, 0xFF, 0x00, 0x80),
        gdSPDefVtxC(0,      0,      -8192, 0, 0, 0x00, 0x00, 0xFF, 0x80),
        gdSPDefVtxC(0,      0,      8192,  0, 0, 0x00, 0x00, 0xFF, 0x80),
      };
      load_l3dex2(&z64_ctxt.gfx->poly_xlu.p);
      gDPPipeSync(z64_ctxt.gfx->poly_xlu.p++);
      gDPSetCycleType(z64_ctxt.gfx->poly_xlu.p++, G_CYC_1CYCLE);
      gDPSetRenderMode(z64_ctxt.gfx->poly_xlu.p++,
                       G_RM_AA_ZB_XLU_LINE, G_RM_AA_ZB_XLU_LINE2);
      gDPSetCombineMode(z64_ctxt.gfx->poly_xlu.p++,
                        G_CC_PRIMITIVE, G_CC_PRIMITIVE);
      gSPLoadGeometryMode(z64_ctxt.gfx->poly_xlu.p++, G_ZBUFFER);
      gSPTexture(z64_ctxt.gfx->poly_xlu.p++, 0, 0, 0, 0, G_OFF);
      gSPMatrix(z64_ctxt.gfx->poly_xlu.p++,
                gDisplayListData(&z64_ctxt.gfx->poly_xlu.d, m),
                G_MTX_LOAD | G_MTX_NOPUSH | G_MTX_MODELVIEW);
      gSPVertex(z64_ctxt.gfx->poly_xlu.p++,
                gDisplayListData(&z64_ctxt.gfx->poly_xlu.d, v), 6, 0);
      gDPSetPrimColor(z64_ctxt.gfx->poly_xlu.p++,
                      0, 0, 0xFF, 0x00, 0x00, 0x80);
      gSPLine3D(z64_ctxt.gfx->poly_xlu.p++, 0, 1, 0);
      gDPSetPrimColor(z64_ctxt.gfx->poly_xlu.p++,
                      0, 0, 0x00, 0xFF, 0x00, 0x80);
      gSPLine3D(z64_ctxt.gfx->poly_xlu.p++, 2, 3, 0);
      gDPSetPrimColor(z64_ctxt.gfx->poly_xlu.p++,
                      0, 0, 0x00, 0x00, 0xFF, 0x80);
      gSPLine3D(z64_ctxt.gfx->poly_xlu.p++, 4, 5, 0);
      unload_l3dex2(&z64_ctxt.gfx->poly_xlu.p, 1);
    }
  }
  else
    strcpy(adi->edit_item->text, "<none>");
  return 1;
}

static void edit_actor_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *adi = data;
  if (adi->index < z64_game.actor_list[adi->type].length) {
    z64_actor_t *actor = z64_game.actor_list[adi->type].first;
    for (int i = 0; i < adi->index; ++i)
      actor = actor->next;
    menu_enter(gz.menu_main, gz.menu_mem);
    mem_goto((uint32_t)actor);
  }
}

static void delete_actor_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *adi = data;
  if (adi->index < z64_game.actor_list[adi->type].length) {
    z64_actor_t *actor = z64_game.actor_list[adi->type].first;
    for (int i = 0; i < adi->index; ++i)
      actor = actor->next;
    z64_DeleteActor(&z64_game, &z64_game.actor_ctxt, actor);
  }
}

static void goto_actor_proc(struct menu_item *item, void *data)
{
  struct actor_debug_info *adi = data;
  if (adi->index < z64_game.actor_list[adi->type].length) {
    z64_actor_t *actor = z64_game.actor_list[adi->type].first;
    for (int i = 0; i < adi->index; ++i)
      actor = actor->next;
    z64_link.common.pos_1 = actor->pos_2;
    z64_link.common.pos_2 = actor->pos_2;
  }
}

static void spawn_actor_proc(struct menu_item *item, void *data)
{
  struct actor_spawn_info *asi = data;
  z64_SpawnActor(&z64_game.actor_ctxt, &z64_game, asi->actor_no,
                 asi->x, asi->y, asi->z, asi->rx, asi->ry, asi->rz,
                 asi->variable);
}

static void fetch_actor_info_proc(struct menu_item *item, void *data)
{
  struct actor_spawn_info *asi = data;
  asi->x = floorf(z64_link.common.pos_2.x + 0.5f);
  asi->y = floorf(z64_link.common.pos_2.y + 0.5f);
  asi->z = floorf(z64_link.common.pos_2.z + 0.5f);
  asi->rx = z64_link.common.rot_2.x;
  asi->ry = z64_link.common.rot_2.y;
  asi->rz = z64_link.common.rot_2.z;
}

#ifndef WIIVC
static void start_rdb_proc(struct menu_item *item, void *data)
{
  rdb_start();
}

static void stop_rdb_proc(struct menu_item *item, void *data)
{
  rdb_stop();
}

static void break_proc(struct menu_item *item, void *data)
{
  rdb_interrupt();
}

static int rdb_draw_proc(struct menu_item *item,
                         struct menu_draw_params *draw_params)
{
  const char *status = rdb_check() ? "running" : "not running";
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  gfx_printf(draw_params->font, draw_params->x, draw_params->y,
             "rdb status: %s", status);
  return 1;
}
#endif

struct menu *gz_debug_menu(void)
{
  static struct menu menu;
  static struct menu heap;
  static struct menu disp;
  static struct menu objects;
  static struct menu actors;
  static struct menu flags;
  static struct menu mem;
#ifndef WIIVC
  static struct menu rdb;
#endif
  struct menu_item *item;

  /* initialize menus */
  menu_init(&menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&heap, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&disp, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&objects, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu_init(&actors, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
#ifndef WIIVC
  menu_init(&rdb, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
#endif

  /* populate debug top menu */
  menu.selector = menu_add_submenu(&menu, 0, 0, NULL, "return");
  menu_add_submenu(&menu, 0, 1, &heap, "heap");
  menu_add_submenu(&menu, 0, 2, &disp, "display lists");
  menu_add_submenu(&menu, 0, 3, &objects, "objects");
  menu_add_submenu(&menu, 0, 4, &actors, "actors");
  menu_add_submenu(&menu, 0, 5, &flags, "flags");
  menu_add_submenu(&menu, 0, 6, &mem, "memory");
#ifndef WIIVC
  menu_add_submenu(&menu, 0, 7, &rdb, "rdb");
#endif

  /* populate heap menu */
  heap.selector = menu_add_submenu(&heap, 0, 0, NULL, "return");
  menu_add_static_custom(&heap, 0, 1, heap_draw_proc, NULL, 0xC0C0C0);

  /* populate display lists menu */
  disp.selector = menu_add_submenu(&disp, 0, 0, NULL, "return");
  menu_add_static_custom(&disp, 0, 1, disp_draw_proc, NULL, 0xC0C0C0);

  /* populate objects menu */
  objects.selector = menu_add_submenu(&objects, 0, 0, NULL, "return");
  menu_add_static(&objects, 0, 1, "object id", 0xC0C0C0);
  item = menu_add_intinput(&objects, 10, 1, 16, 4, NULL, NULL);
  menu_add_button(&objects, 16, 1, "push", push_object_proc, item);
  menu_add_button(&objects, 22, 1, "pop", pop_object_proc, NULL);
  menu_add_static_custom(&objects, 0, 3, objects_draw_proc, NULL, 0xC0C0C0);

  /* populate actors menu */
  actors.selector = menu_add_submenu(&actors, 0, 0, NULL, "return");
  /* actor debug controls */
  static struct actor_debug_info adi;
  menu_add_static(&actors, 0, 1, "type", 0xC0C0C0);
  menu_add_option(&actors, 10, 1,
                  "switch\0""prop (1)\0""player\0""bomb\0""npc\0"
                  "enemy\0""prop (2)\0""item/action\0""misc\0""boss\0"
                  "door\0""chest\0",
                  byte_optionmod_proc, &adi.type);
  item = menu_add_static_custom(&actors, 0, 2, actor_draw_proc,
                                NULL, 0xC0C0C0);
  item->data = &adi;
  menu_add_static(&actors, 0, 2, "index", 0xC0C0C0);
  menu_add_button(&actors, 10, 2, "<", actor_index_dec_proc, &adi);
  menu_add_button(&actors, 12, 2, ">", actor_index_inc_proc, &adi);
  item = menu_add_button(&actors, 0, 3, NULL, &edit_actor_proc, &adi);
  item->text = malloc(9);
  item->text[0] = 0;
  adi.edit_item = item;
  menu_add_button(&actors, 0, 5, "delete", &delete_actor_proc, &adi);
  menu_add_button(&actors, 10, 5, "go to", &goto_actor_proc, &adi);
  /* actor spawn controls */
  static struct actor_spawn_info asi;
  menu_add_static(&actors, 0, 7, "actor id", 0xC0C0C0);
  menu_add_intinput(&actors, 10, 7, 16, 4, halfword_mod_proc, &asi.actor_no);
  menu_add_static(&actors, 0, 8, "variable", 0xC0C0C0);
  menu_add_intinput(&actors, 10, 8, 16, 4, halfword_mod_proc, &asi.variable);
  menu_add_static(&actors, 0, 9, "position", 0xC0C0C0);
  menu_add_intinput(&actors, 10, 9, -10, 6, word_mod_proc, &asi.x);
  menu_add_intinput(&actors, 17, 9, -10, 6, word_mod_proc, &asi.y);
  menu_add_intinput(&actors, 24, 9, -10, 6, word_mod_proc, &asi.z);
  menu_add_static(&actors, 0, 10, "rotation", 0xC0C0C0);
  menu_add_intinput(&actors, 10, 10, 10, 5, halfword_mod_proc, &asi.rx);
  menu_add_intinput(&actors, 17, 10, 10, 5, halfword_mod_proc, &asi.ry);
  menu_add_intinput(&actors, 24, 10, 10, 5, halfword_mod_proc, &asi.rz);
  menu_add_button(&actors, 0, 11, "spawn", spawn_actor_proc, &asi);
  menu_add_button(&actors, 10, 11,"fetch from link",
                  &fetch_actor_info_proc, &asi);

  /* create flags menu */
  flag_menu_create(&flags);

  /* create memory menu */
  mem_menu_create(&mem);
  gz.menu_mem = &mem;

#ifndef WIIVC
  /* populate rdb menu */
  rdb.selector = menu_add_submenu(&rdb, 0, 0, NULL, "return");
  menu_add_button(&rdb, 0, 1, "start rdb", start_rdb_proc, NULL);
  menu_add_button(&rdb, 0, 2, "stop rdb", stop_rdb_proc, NULL);
  menu_add_button(&rdb, 0, 3, "break", break_proc, NULL);
  menu_add_static_custom(&rdb, 0, 5, rdb_draw_proc, NULL, 0xC0C0C0);
#endif

  return &menu;
}
