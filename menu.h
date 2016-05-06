#ifndef MENU_H
#define MENU_H
#include <stdint.h>
#include <list/list.h>

struct menu;
struct menu_item;

enum menu_navigation
{
  MENU_NAVIGATE_UP,
  MENU_NAVIGATE_DOWN,
  MENU_NAVIGATE_LEFT,
  MENU_NAVIGATE_RIGHT,
};

struct menu_item
{
  int         x;
  int         y;
  char       *text;
  uint32_t    color;
  int         priority;
  void       *data;
  int       (*think_proc)   (struct menu *menu, struct menu_item *item);
  int       (*navigate_proc)(struct menu *menu, struct menu_item *item,
                             enum menu_navigation nav);
  int       (*activate_proc)(struct menu *menu, struct menu_item *item);
  int       (*move_proc)    (struct menu *menu, struct menu_item *item,
                             int x, int y);
  int       (*remove_proc)  (struct menu *menu, struct menu_item *item);
};

struct menu
{
  struct list         items;
  struct menu_item   *selector;
  int                 animate_highlight;
  uint32_t            highlight_color_static;
  uint32_t            highlight_color_animated;
};

enum data_type
{
  DATA_TYPE_U8,
  DATA_TYPE_S8,
  DATA_TYPE_X8,
  DATA_TYPE_U16,
  DATA_TYPE_S16,
  DATA_TYPE_X16,
  DATA_TYPE_U32,
  DATA_TYPE_S32,
  DATA_TYPE_X32,
  DATA_TYPE_F32,
  DATA_TYPE_MAX,
};

#ifdef __cplusplus
extern "C"
{
#endif

void                menu_init(struct menu *menu);
struct menu_item   *menu_add_item(struct menu *menu, struct menu_item *item);
void                menu_draw(struct menu *menu);
void                menu_navigate(struct menu *menu, enum menu_navigation nav);
void                menu_activate(struct menu *menu);
void                menu_item_init(struct menu_item *item, int x, int y,
                                   const char *text, uint32_t color);
void                menu_item_move(struct menu *menu, struct menu_item *item,
                                   int x, int y);
void                menu_item_remove(struct menu *menu, struct menu_item *item);

struct menu_item   *menu_add_address(struct menu *menu, int x, int y,
                                     int priority);
uint32_t            menu_address_get(struct menu_item *item);
void                menu_address_set(struct menu_item *item, uint32_t address);
struct menu_item   *menu_add_datatype(struct menu *menu, int x, int y,
                                      int priority);
enum data_type      menu_datatype_get(struct menu_item *item);
void                menu_datatype_set(struct menu_item *item,
                                      enum data_type type);
struct menu_item   *menu_add_watch(struct menu *menu, int x, int y,
                                   int priority);
struct menu_item   *menu_watch_address(struct menu_item *item);
struct menu_item   *menu_watch_type(struct menu_item *item);
struct menu_item   *menu_add_watchlist(struct menu *menu, int x, int y,
                                       int priority);

#ifdef __cplusplus
}
#endif

#endif
