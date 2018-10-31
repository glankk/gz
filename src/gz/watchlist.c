#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <vector/vector.h>
#include "adex.h"
#include "files.h"
#include "menu.h"
#include "resource.h"
#include "settings.h"

struct item_data
{
  struct menu      *menu_release;
  struct menu      *imenu;
  struct vector     members;
  struct menu_item *add_button;
#ifndef WIIVC
  struct menu_item *import_button;
#endif
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
  if (!member_data->position_set) {
    member_data->x = menu_item_screen_x(watch);
    member_data->y = menu_item_screen_y(watch);
  }
  watch->x = 0;
  watch->y = 0;
  watch->pxoffset = member_data->x;
  watch->pyoffset = member_data->y;
  menu_item_transfer(watch, member_data->data->menu_release);
}

static void anchor_member(struct member_data *member_data)
{
  if (member_data->anchored)
    return;
  member_data->anchored = 1;
  menu_item_disable(member_data->positioning);
  struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
  watch->x = 13;
  watch->y = 0;
  watch->pxoffset = 0;
  watch->pyoffset = 0;
  menu_item_transfer(watch, member_data->userwatch->imenu);
}

static int anchor_button_enter_proc(struct menu_item *item,
                                    enum menu_switch_reason reason)
{
  struct member_data *member_data = item->data;
  member_data->anchor_anim_state = 0;
  return 0;
}

static int anchor_button_draw_proc(struct menu_item *item,
                                   struct menu_draw_params *draw_params)
{
  struct member_data *member_data = item->data;
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
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
    draw_params->y -
    (gfx_font_xheight(draw_params->font) + texture->tile_height + 1) / 2,
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
  if (input_pad() & BUTTON_Z)
    dist *= 2;
  switch (reason) {
    case MENU_CALLBACK_ACTIVATE:    input_reserve(BUTTON_Z);  break;
    case MENU_CALLBACK_DEACTIVATE:  input_free(BUTTON_Z);     break;
    case MENU_CALLBACK_NAV_UP:      member_data->y -= dist;   break;
    case MENU_CALLBACK_NAV_DOWN:    member_data->y += dist;   break;
    case MENU_CALLBACK_NAV_LEFT:    member_data->x -= dist;   break;
    case MENU_CALLBACK_NAV_RIGHT:   member_data->x += dist;   break;
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
#ifndef WIIVC
  ++data->import_button->y;
#endif
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
  member_data->position_set = 1;
  menu_add_button_icon(imenu, 0, 0, list_icons, 1, 0xFF0000,
                       remove_button_proc, member_data);
  if (anchored)
    menu_item_disable(member_data->positioning);
  else
    release_member(member_data);
  member_data->position_set = position_set;
  vector_insert(&data->members, position, 1, &member_data);
  return 1;
}

static int remove_member(struct item_data *data, int position)
{
  if (position < 0 || position >= data->members.size)
    return 0;
  menu_navigate_top(data->imenu, MENU_NAVIGATE_DOWN);
  --data->add_button->y;
#ifndef WIIVC
  --data->import_button->y;
#endif
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
  uint32_t address = 0x80000000;
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

#ifndef WIIVC
static int import_callback(const char *path, void *data);
static void import_button_proc(struct menu_item *item, void *data)
{
  struct item_data *item_data = data;
  menu_get_file(menu_get_top(item_data->imenu), GETFILE_LOAD, NULL, ".txt",
                import_callback, item_data);
}
#endif

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
                                   struct menu *menu_release,
                                   int x, int y)
{
  struct menu *imenu;
  struct menu_item *item = menu_add_imenu(menu, x, y, &imenu);
  struct item_data *data = malloc(sizeof(*data));
  data->menu_release = menu_release;
  data->imenu = imenu;
  vector_init(&data->members, sizeof(struct member_data*));
  if (!list_icons)
    list_icons = resource_load_grc_texture("list_icons");
  data->add_button = menu_add_button_icon(imenu, 0, 0,
                                          list_icons, 0, 0x00FF00,
                                          add_button_proc, data);
#ifndef WIIVC
  struct gfx_texture *file_icons = resource_get(RES_ICON_FILE);
  data->import_button = menu_add_button_icon(imenu, 2, 0,
                                             file_icons, 1, 0xFFFFFF,
                                             import_button_proc, data);
#endif
  item->data = data;
  item->destroy_proc = destroy_proc;
  return item;
}

void watchlist_store(struct menu_item *item)
{
  struct item_data *data = item->data;
  settings->n_watches = data->members.size;
  for (int i = 0; i < data->members.size; ++i) {
    struct member_data *member_data = get_member(data, i);
    struct menu_item *watch = menu_userwatch_watch(member_data->userwatch);
    settings->watch_address[i] = menu_watch_get_address(watch);
    settings->watch_x[i] = member_data->x;
    settings->watch_y[i] = member_data->y;
    settings->watch_info[i].type = menu_watch_get_type(watch);
    settings->watch_info[i].anchored = member_data->anchored;
    settings->watch_info[i].position_set = member_data->position_set;
  }
}

void watchlist_fetch(struct menu_item *item)
{
  struct item_data *data = item->data;
  for (int i = data->members.size - 1; i >= 0; --i)
    remove_member(data, i);
  for (int i = 0; i < settings->n_watches; ++i)
    add_member(data, settings->watch_address[i], settings->watch_info[i].type,
               i, settings->watch_info[i].anchored,
               settings->watch_x[i], settings->watch_y[i],
               settings->watch_info[i].position_set);
}

/*
   import menu
*/

#ifndef WIIVC

#define WATCHFILE_VIEW_ROWS   16

static struct item_data      *watchfile_list_data;
static struct vector          watchfile_entries;
static struct menu            watchfile_menu;
static struct menu_item      *watchfile_return;
static struct menu_item      *watchfile_items[WATCHFILE_VIEW_ROWS];
static int                    watchfile_scroll;

struct watchfile_entry
{
  char             *name;
  enum watch_type   type;
  struct adex       adex;
  int               anim_state;
};

static const char *watch_type_name[] =
{
  "u8", "s8", "x8",
  "u16", "s16", "x16",
  "u32", "s32", "x32",
  "f32",
};

static int watch_type_size[] =
{
  1, 1, 1,
  2, 2, 2,
  4, 4, 4,
  4,
};

static void watchfile_destroy(void)
{
  for (int i = 0; i < watchfile_entries.size; ++i) {
    struct watchfile_entry *entry = vector_at(&watchfile_entries, i);
    free(entry->name);
    adex_destroy(&entry->adex);
  }
  vector_destroy(&watchfile_entries);
}

static int watchfile_leave_proc(struct menu_item *item,
                                enum menu_switch_reason reason)
{
  if (reason == MENU_SWITCH_RETURN)
    watchfile_destroy();
  return 0;
}

static int entry_enter_proc(struct menu_item *item,
                            enum menu_switch_reason reason)
{
  int row = (int)item->data;
  int index = watchfile_scroll + row;
  if (index < watchfile_entries.size) {
    struct watchfile_entry *entry = vector_at(&watchfile_entries, index);
    entry->anim_state = 0;
  }
  return 0;
}

static int entry_draw_proc(struct menu_item *item,
                           struct menu_draw_params *draw_params)
{
  int row = (int)item->data;
  struct watchfile_entry *entry = vector_at(&watchfile_entries,
                                            watchfile_scroll + row);
  if (entry->anim_state > 0) {
    ++draw_params->x;
    ++draw_params->y;
    entry->anim_state = (entry->anim_state + 1) % 3;
  }
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  gfx_printf(draw_params->font, draw_params->x, draw_params->y,
             "%s", entry->name);
  return 1;
}

static int entry_activate_proc(struct menu_item *item)
{
  int row = (int)item->data;
  struct watchfile_entry *entry = vector_at(&watchfile_entries,
                                            watchfile_scroll + row);
  entry->anim_state = 1;
  uint32_t address;
  enum adex_error e = adex_eval(&entry->adex, &address);
  if (!e && (address < 0x80000000 || address >= 0x80800000 ||
             address % watch_type_size[entry->type] != 0))
  {
    e = ADEX_ERROR_ADDRESS;
  }
  if (e) {
    struct menu *menu_top = menu_get_top(watchfile_list_data->imenu);
    menu_prompt(menu_top, adex_error_name[e], "return\0", 0, NULL, NULL);
  }
  else {
    add_member(watchfile_list_data, address, entry->type,
               watchfile_list_data->members.size, 1, 0, 0, 0);
  }
  return 1;
}

static void scroll_up_proc(struct menu_item *item, void *data)
{
  --watchfile_scroll;
  if (watchfile_scroll < 0)
    watchfile_scroll = 0;
}

static void scroll_down_proc(struct menu_item *item, void *data)
{
  ++watchfile_scroll;
  int n_entries = watchfile_entries.size;
  if (watchfile_scroll + WATCHFILE_VIEW_ROWS > n_entries)
    watchfile_scroll = n_entries - WATCHFILE_VIEW_ROWS;
  if (watchfile_scroll < 0)
    watchfile_scroll = 0;
}

static void watchfile_menu_init(void)
{
  static _Bool ready = 0;
  if (!ready) {
    ready = 1;
    struct menu *menu = &watchfile_menu;
    menu_init(menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
    watchfile_return = menu_add_submenu(menu, 0, 0, NULL, "return");
    watchfile_return->leave_proc = watchfile_leave_proc;
    struct gfx_texture *t_arrow = resource_get(RES_ICON_ARROW);
    menu_add_button_icon(menu, 0, 1,
                         t_arrow, 0, 0xFFFFFF,
                         scroll_up_proc, NULL);
    menu_add_button_icon(menu, 0, 1 + WATCHFILE_VIEW_ROWS - 1,
                         t_arrow, 1, 0xFFFFFF,
                         scroll_down_proc, NULL);
    for (int i = 0; i < WATCHFILE_VIEW_ROWS; ++i) {
      struct menu_item *item = menu_item_add(menu, 2, 1 + i, NULL, 0xFFFFFF);
      item->data = (void*)i;
      item->enter_proc = entry_enter_proc;
      item->draw_proc = entry_draw_proc;
      item->activate_proc = entry_activate_proc;
      watchfile_items[i] = item;
    }
  }
}

static void watchfile_view(struct menu *menu)
{
  /* initialize menus */
  watchfile_menu_init();
  watchfile_scroll = 0;
  /* configure menus */
  for (int i = 0; i < WATCHFILE_VIEW_ROWS; ++i) {
    if (i < watchfile_entries.size)
      menu_item_enable(watchfile_items[i]);
    else
      menu_item_disable(watchfile_items[i]);
  }
  if (watchfile_entries.size > 0)
    menu_select(&watchfile_menu, watchfile_items[0]);
  else
    menu_select(&watchfile_menu, watchfile_return);
  menu_enter(menu, &watchfile_menu);
}

static _Bool parse_line(const char *line, const char **err_str)
{
  const char *p = line;
  /* skip whitespace, check for comment or empty line */
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    ++p;
  if (*p == '#' || *p == 0)
    return 1;
  /* read name part */
  if (*p++ != '"')
    goto syntax_err;
  const char *name_s = p;
  const char *name_e = strchr(p, '"');
  if (!name_e || name_e == name_s)
    goto syntax_err;
  int name_l = name_e - name_s;
  p = name_e + 1;
  while (*p == ' ' || *p == '\t')
    ++p;
  /* read type part */
  const char *type_s = p;
  while (*p && *p != ' ' && *p != '\t')
    ++p;
  while (*p == ' ' || *p == '\t')
    ++p;
  const char *expr_s = p;
  /* construct entry */
  struct watchfile_entry entry;
  entry.type = -1;
  for (int i = 0; i < sizeof(watch_type_name) / sizeof(*watch_type_name);
       ++i)
  {
    int l = strlen(watch_type_name[i]);
    if (strncmp(type_s, watch_type_name[i], l) == 0) {
      entry.type = i;
      break;
    }
  }
  if (entry.type == -1)
    goto syntax_err;
  enum adex_error e = adex_parse(&entry.adex, expr_s);
  if (e) {
    *err_str = adex_error_name[e];
    goto err;
  }
  entry.name = malloc(name_l + 1);
  memcpy(entry.name, name_s, name_l);
  entry.name[name_l] = 0;
  entry.anim_state = 0;
  /* insert entry */
  vector_push_back(&watchfile_entries, 1, &entry);
  return 1;
syntax_err:
  *err_str = adex_error_name[ADEX_ERROR_SYNTAX];
  goto err;
err:
  return 0;
}

static int import_callback(const char *path, void *data)
{
  /* initialize watchfile data */
  watchfile_list_data = data;
  vector_init(&watchfile_entries, sizeof(struct watchfile_entry));
  /* parse lines */
  const char *err_str = NULL;
  FILE *f = fopen(path, "r");
  char *line = malloc(1024);
  if (f) {
    while (1) {
      if (fgets(line, 1024, f)) {
        if (strchr(line, '\n') || feof(f)) {
          if (!parse_line(line, &err_str))
            break;
        }
        else {
          err_str = "line overflow";
          break;
        }
      }
      else {
        if (!feof(f))
          err_str = strerror(ferror(f));
        break;
      }
    }
  }
  else
    err_str = strerror(errno);
  if (f)
    fclose(f);
  if (line)
    free(line);
  /* show error message or view file */
  struct menu *menu_top = menu_get_top(watchfile_list_data->imenu);
  if (err_str) {
    watchfile_destroy();
    menu_prompt(menu_top, err_str, "return\0", 0, NULL, NULL);
  }
  else {
    menu_return(menu_top);
    watchfile_view(menu_top);
  }
  return 1;
}

#endif
