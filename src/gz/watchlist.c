#include <stdlib.h>
#include <vector/vector.h>
#include "menu.h"
#include "resource.h"
#include "settings.h"
#include "z64.h"

struct item_data
{
  struct menu      *menu_global_watches;
  struct menu      *imenu;
  struct vector     members;
  struct menu_item *add_button;
};

struct member_data
{
  struct item_data *data;
  int               index;
  struct menu_item *member;
  struct menu_item *anchor_button;
  struct menu_item *positioning;
  struct menu_item *userwatch;
  _Bool             anchored;
  int               anchor_anim_state;
  int               x;
  int               y;
  _Bool             position_set;
};

static struct gfx_texture *list_icons = NULL;

static struct member_data *get_member(struct item_data *data, int index)
{
  if (index < 0 || index >= data->members.size)
    return NULL;
  struct member_data **member_data = vector_at(&data->members, index);
  return *member_data;
}

static void release_member(struct member_data *member_data)
{
  if (!member_data->anchored)
    return;
  member_data->anchored = 0;
  menu_item_enable(member_data->positioning);
  struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
  watch->pxoffset = member_data->position_set ?
                    member_data->x : menu_item_screen_x(watch);
  watch->pyoffset = member_data->position_set ?
                    member_data->y : menu_item_screen_y(watch);
  watch->x = 0;
  watch->y = 0;
  menu_item_transfer(watch, member_data->data->menu_global_watches);
}

static void anchor_member(struct member_data *member_data)
{
  if (member_data->anchored)
    return;
  member_data->anchored = 1;
  menu_item_disable(member_data->positioning);
  struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
  watch->pxoffset = 0;
  watch->pyoffset = 0;
  watch->x = 13;
  watch->y = 0;
  menu_item_transfer(watch, member_data->userwatch->imenu);
}

static int anchor_button_enter_proc(struct menu_item *item)
{
  struct member_data *member_data = item->data;
  member_data->anchor_anim_state = 0;
  return 0;
}

static int anchor_button_draw_proc(struct menu_item *item,
                                   struct menu_draw_params *draw_params)
{
  struct member_data *member_data = item->data;
  gfx_mode_color((draw_params->color >> 16) & 0xFF,
                 (draw_params->color >> 8)  & 0xFF,
                 (draw_params->color >> 0)  & 0xFF,
                 draw_params->alpha);
  static struct gfx_texture *texture = NULL;
  if (!texture)
    texture = resource_load_grc_texture("anchor");
  int cw = menu_get_cell_width(item->owner, 1);
  if (member_data->anchor_anim_state > 0) {
    ++draw_params->x;
    ++draw_params->y;
  }
  struct gfx_sprite sprite =
  {
    texture,
    (member_data->anchor_anim_state > 0) != member_data->anchored ? 0 : 1,
    draw_params->x +
    (cw - texture->tile_width) / 2,
    draw_params->y +
    (draw_params->font->median - draw_params->font->baseline -
     texture->tile_height) / 2,
    1.f, 1.f,
  };
  gfx_sprite_draw(&sprite);
  if (member_data->anchor_anim_state > 0)
    member_data->anchor_anim_state = (member_data->anchor_anim_state + 1) % 3;
  return 1;
}

static int anchor_button_activate_proc(struct menu_item *item)
{
  struct member_data *member_data = item->data;
  if (member_data->anchored)
    release_member(member_data);
  else
    anchor_member(member_data);
  member_data->anchor_anim_state = 1;
  return 1;
}

static int position_proc(struct menu_item *item,
                         enum menu_callback_reason reason,
                         void *data)
{
  struct member_data *member_data = data;
  struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
  if (!member_data->position_set) {
    member_data->position_set = 1;
    member_data->x = watch->pxoffset;
    member_data->y = watch->pyoffset;
  }
  int dist = 2;
  if (z64_input_direct.pad & BUTTON_Z)
    dist *= 2;
  switch (reason) {
    case MENU_CALLBACK_NAV_UP:    member_data->y -= dist; break;
    case MENU_CALLBACK_NAV_DOWN:  member_data->y += dist; break;
    case MENU_CALLBACK_NAV_LEFT:  member_data->x -= dist; break;
    case MENU_CALLBACK_NAV_RIGHT: member_data->x += dist; break;
    default:
      break;
  }
  watch->pxoffset = member_data->x;
  watch->pyoffset = member_data->y;
  return 0;
}

static void remove_button_proc(struct menu_item *item, void *data);
static int add_member(struct item_data *data,
                      uint32_t address, enum watch_type type, int position,
                      _Bool anchored, int x, int y, _Bool position_set)
{
  if (data->members.size >= SETTINGS_WATCHES_MAX ||
      position < 0 || position > data->members.size)
    return 0;
  ++data->add_button->y;
  for (int i = position; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    ++member_data->index;
    ++member_data->member->y;
  }
  struct menu *imenu;
  struct member_data *member_data = malloc(sizeof(*member_data));
  member_data->data = data;
  member_data->index = position;
  member_data->member = menu_add_imenu(data->imenu, 0, position, &imenu);
  member_data->anchor_button = menu_item_add(imenu, 2, 0, NULL, 0xFFFFFF);
  member_data->anchor_button->enter_proc = anchor_button_enter_proc;
  member_data->anchor_button->draw_proc = anchor_button_draw_proc;
  member_data->anchor_button->activate_proc = anchor_button_activate_proc;
  member_data->anchor_button->data = member_data;
  member_data->positioning = menu_add_positioning(imenu, 4, 0,
                                                  position_proc, member_data);
  member_data->userwatch = menu_add_userwatch(imenu, 6, 0, address, type);
  member_data->anchored = 1;
  member_data->anchor_anim_state = 0;
  member_data->x = x;
  member_data->y = y;
  member_data->position_set = position_set;
  menu_add_button_icon(imenu, 0, 0, list_icons, 1, 0xFF0000,
                       remove_button_proc, member_data);
  if (anchored)
    menu_item_disable(member_data->positioning);
  else
    release_member(member_data);
  vector_insert(&data->members, position, 1, &member_data);
  return 1;
}

static int remove_member(struct item_data *data, int position)
{
  if (position < 0 || position >= data->members.size)
    return 0;
  menu_navigate_top(data->imenu, MENU_NAVIGATE_DOWN);
  --data->add_button->y;
  for (int i = position + 1; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    --member_data->index;
    --member_data->member->y;
  }
  struct member_data *member_data = get_member(data, position);
  struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
  menu_item_remove(watch);
  member_data->anchor_button->data = NULL;
  menu_item_remove(member_data->member);
  vector_erase(&data->members, position, 1);
  free(member_data);
  return 1;
}

static void add_button_proc(struct menu_item *item, void *data)
{
  struct item_data *item_data = data;
  uint32_t address = 0x00000000;
  enum watch_type type = WATCH_TYPE_U8;
  if (item_data->members.size > 0) {
    struct member_data *member_data = get_member(item_data,
                                                 item_data->members.size - 1);
    struct menu_item *last_watch = menu_userwatch_watch(member_data->userwatch);
    address = menu_watch_get_address(last_watch);
    type = menu_watch_get_type(last_watch);
  }
  add_member(item_data, address, type, item_data->members.size, 1, 0, 0, 0);
}

static void remove_button_proc(struct menu_item *item, void *data)
{
  struct member_data *member_data = data;
  remove_member(member_data->data, member_data->index);
}

static int destroy_proc(struct menu_item *item)
{
  struct item_data *data = item->data;
  vector_destroy(&data->members);
  return 0;
}

struct menu_item *watchlist_create(struct menu *menu,
                                   struct menu *menu_global_watches,
                                   int x, int y)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  data->menu_global_watches = menu_global_watches;
  data->imenu = imenu;
  vector_init(&data->members, sizeof(struct member_data*));
  if (!list_icons)
    list_icons = resource_load_grc_texture("list_icons");
  data->add_button = menu_add_button_icon(imenu, 0, 0, list_icons, 0, 0x00FF00,
                                          add_button_proc, data);
  item->data = data;
  item->destroy_proc = destroy_proc;
  return item;
}

void watchlist_store(struct menu_item *item)
{
  struct item_data *data = item->data;
  settings->no_watches = data->members.size;
  for (int i = 0; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
    settings->watch_address[i] = menu_watch_get_address(watch);
    settings->watch_type[i] = menu_watch_get_type(watch);
    settings->watch_anchored[i] = member_data->anchored;
    settings->watch_x[i] = member_data->x;
    settings->watch_y[i] = member_data->y;
    settings->watch_position_set[i] = member_data->position_set;
  }
}

void watchlist_fetch(struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = data->members.size - 1; i >= 0; --i)
    remove_member(data, i);
  for (int i = 0; i < settings->no_watches; ++i)
    add_member(data, settings->watch_address[i], settings->watch_type[i], i,
               settings->watch_anchored[i],
               settings->watch_x[i], settings->watch_y[i],
               settings->watch_position_set[i]);
}
