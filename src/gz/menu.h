#ifndef MENU_H
#define MENU_H
#include <stdint.h>
#include <list/list.h>
#include "gfx.h"

#define MENU_NOVALUE 0

struct menu;
struct menu_item;

enum menu_navigation
{
  MENU_NAVIGATE_UP,
  MENU_NAVIGATE_DOWN,
  MENU_NAVIGATE_LEFT,
  MENU_NAVIGATE_RIGHT,
};

enum menu_callback_reason
{
  MENU_CALLBACK_THINK,
  MENU_CALLBACK_THINK_ACTIVE,
  MENU_CALLBACK_THINK_INACTIVE,
  MENU_CALLBACK_ACTIVATE,
  MENU_CALLBACK_DEACTIVATE,
  MENU_CALLBACK_SWITCH_ON,
  MENU_CALLBACK_SWITCH_OFF,
  MENU_CALLBACK_NAV_UP,
  MENU_CALLBACK_NAV_DOWN,
  MENU_CALLBACK_NAV_LEFT,
  MENU_CALLBACK_NAV_RIGHT,
  MENU_CALLBACK_CHANGED,
};

typedef int  (*menu_generic_callback)(struct menu_item *item,
                                      enum menu_callback_reason reason,
                                      void *data);
typedef void (*menu_action_callback) (struct menu_item *item, void *data);

struct menu_item
{
  struct menu      *owner;
  int               x;
  int               y;
  char             *text;
  uint32_t          color;
  _Bool             animate_highlight;
  void             *data;
  _Bool             selectable;
  struct menu      *imenu;
  int             (*think_proc)   (struct menu_item *item);
  int             (*draw_proc)    (struct menu_item *item);
  int             (*navigate_proc)(struct menu_item *item,
                                   enum menu_navigation nav);
  int             (*activate_proc)(struct menu_item *item);
  int             (*destroy_proc) (struct menu_item *item);
};

struct menu
{
  int               cxoffset;
  int               cyoffset;
  int               pxoffset;
  int               pyoffset;
  int               cell_width;
  int               cell_height;
  struct gfx_font  *font;
  float             alpha;
  struct list       items;
  struct menu_item *selector;
  struct menu      *parent;
  struct menu      *child;
  uint32_t          highlight_color_static;
  uint32_t          highlight_color_animated;
  int               highlight_state[3];
};

enum watch_type
{
  WATCH_TYPE_U8,
  WATCH_TYPE_S8,
  WATCH_TYPE_X8,
  WATCH_TYPE_U16,
  WATCH_TYPE_S16,
  WATCH_TYPE_X16,
  WATCH_TYPE_U32,
  WATCH_TYPE_S32,
  WATCH_TYPE_X32,
  WATCH_TYPE_F32,
  WATCH_TYPE_MAX,
};

#ifdef __cplusplus
extern "C"
{
#endif

void                menu_init(struct menu *menu,
                              int cell_width, int cell_height,
                              struct gfx_font *font);
void                menu_destroy(struct menu *menu);
int                 menu_get_cxoffset(struct menu *menu, _Bool inherit);
void                menu_set_cxoffset(struct menu *menu, int cxoffset);
int                 menu_get_cyoffset(struct menu *menu, _Bool inherit);
void                menu_set_cyoffset(struct menu *menu, int cyoffset);
int                 menu_get_pxoffset(struct menu *menu, _Bool inherit);
void                menu_set_pxoffset(struct menu *menu, int pxoffset);
int                 menu_get_pyoffset(struct menu *menu, _Bool inherit);
void                menu_set_pyoffset(struct menu *menu, int pyoffset);
int                 menu_get_cell_width(struct menu *menu, _Bool inherit);
void                menu_set_cell_width(struct menu *menu, int cell_width);
int                 menu_get_cell_height(struct menu *menu, _Bool inherit);
void                menu_set_cell_height(struct menu *menu, int cell_height);
struct gfx_font    *menu_get_font(struct menu *menu, _Bool inherit);
void                menu_set_font(struct menu *menu, struct gfx_font *font);
float               menu_get_alpha(struct menu *menu, _Bool inherit);
uint8_t             menu_get_alpha_i(struct menu *menu, _Bool inherit);
void                menu_set_alpha(struct menu *menu, float alpha);
int                 menu_cell_screen_x(struct menu *menu, int cell_x);
int                 menu_cell_screen_y(struct menu *menu, int cell_y);
int                 menu_think(struct menu *menu);
void                menu_draw(struct menu *menu);
void                menu_navigate(struct menu *menu, enum menu_navigation nav);
void                menu_activate(struct menu *menu);
void                menu_enter(struct menu *menu, struct menu *submenu);
struct menu        *menu_return(struct menu *menu);
void                menu_navigate_top(struct menu *menu,
                                      enum menu_navigation nav);
void                menu_activate_top(struct menu *menu);
void                menu_enter_top(struct menu *menu, struct menu *submenu);
struct menu        *menu_return_top(struct menu *menu);

struct menu_item   *menu_item_add(struct menu *menu, int x, int y,
                                  const char *text, uint32_t color);
void                menu_item_remove(struct menu_item *item);
int                 menu_item_screen_x(struct menu_item *item);
int                 menu_item_screen_y(struct menu_item *item);
struct menu_item   *menu_add_static(struct menu *menu, int x, int y,
                                    const char *text, uint32_t color);
struct menu_item   *menu_add_imenu(struct menu *menu, int x, int y,
                                   struct menu **p_imenu);

struct menu_item   *menu_add_intinput(struct menu *menu, int x, int y,
                                      int base, int length,
                                      menu_generic_callback callback_proc,
                                      void *callback_data);
uint32_t            menu_intinput_get(struct menu_item *item);
void                menu_intinput_set(struct menu_item *item, uint32_t value);
struct menu_item   *menu_add_option(struct menu *menu, int x, int y,
                                    const char *options,
                                    menu_generic_callback callback_proc,
                                    void *callback_data);
int                 menu_option_get(struct menu_item *item);
void                menu_option_set(struct menu_item *item,
                                    int value);
struct menu_item   *menu_add_watch(struct menu *menu, int x, int y,
                                   uint32_t address, enum watch_type type);
uint32_t            menu_watch_get_address(struct menu_item *item);
void                menu_watch_set_address(struct menu_item *item,
                                           uint32_t address);
enum watch_type     menu_watch_get_type(struct menu_item *item);
void                menu_watch_set_type(struct menu_item *item,
                                        enum watch_type type);
struct menu_item   *menu_add_userwatch(struct menu *menu, int x, int y,
                                       uint32_t address, enum watch_type type);
struct menu_item   *menu_userwatch_address(struct menu_item *item);
struct menu_item   *menu_userwatch_type(struct menu_item *item);
struct menu_item   *menu_userwatch_watch(struct menu_item *item);
struct menu_item   *menu_add_watchlist(struct menu *menu, int x, int y);
struct menu_item   *menu_add_submenu(struct menu *menu, int x, int y,
                                     struct menu *submenu, const char *name);
struct menu_item   *menu_add_switch(struct menu *menu, int x, int y,
                                    const char *name,
                                    menu_generic_callback callback_proc,
                                    void *callback_data);
void                menu_switch_set(struct menu_item *item, _Bool state);
_Bool               menu_switch_get(struct menu_item *item);
void                menu_switch_toggle(struct menu_item *item);
struct menu_item   *menu_add_button(struct menu *menu, int x, int y,
                                    const char *name,
                                    menu_action_callback callback_proc,
                                    void *callback_data);
void               *menu_button_callback_data(struct menu_item *item);

#ifdef __cplusplus
}
#endif

#endif
