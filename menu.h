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

enum menu_callback_reason
{
  MENU_CALLBACK_THINK,
  MENU_CALLBACK_SWITCH_ON,
  MENU_CALLBACK_SWITCH_OFF,
};

typedef int  (*menu_switch_callback)(struct menu_item *item,
                                     enum menu_callback_reason reason,
                                     void *data);
typedef void (*menu_button_callback)(struct menu_item *item, void *data);

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
  struct menu        *parent;
  struct menu        *child;
  int                 animate_highlight;
  uint32_t            highlight_color_static;
  uint32_t            highlight_color_animated;
  int                 highlight_state[3];
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
void                menu_enter(struct menu *menu, struct menu *submenu);
struct menu        *menu_return(struct menu *menu);
void                menu_item_init(struct menu_item *item, int x, int y,
                                   const char *text, uint32_t color);
void                menu_item_move(struct menu *menu, struct menu_item *item,
                                   int x, int y);
void                menu_item_remove(struct menu *menu, struct menu_item *item);
struct menu_item   *menu_add_static(struct menu *menu, int x, int y,
                                    const char *text, uint32_t color);

struct menu_item   *menu_add_intinput(struct menu *menu, int x, int y,
                                      int base, int length, int priority);
uint32_t            menu_intinput_get(struct menu_item *item);
void                menu_intinput_set(struct menu_item *item, uint32_t value);
struct menu_item   *menu_add_option(struct menu *menu, int x, int y,
                                    const char *options, int priority);
int                 menu_option_get(struct menu_item *item);
void                menu_option_set(struct menu_item *item,
                                    int value);
struct menu_item   *menu_add_watch(struct menu *menu, int x, int y,
                                   int priority);
struct menu_item   *menu_watch_address(struct menu_item *item);
struct menu_item   *menu_watch_type(struct menu_item *item);
struct menu_item   *menu_add_watchlist(struct menu *menu, int x, int y,
                                       int priority);
struct menu_item   *menu_add_submenu(struct menu *menu, int x, int y,
                                     struct menu *submenu, const char *name,
                                     int priority);
struct menu_item   *menu_add_switch(struct menu *menu, int x, int y,
                                    const char *name,
                                    menu_switch_callback callback_proc,
                                    void *callback_data, int priority);
void                menu_switch_set(struct menu_item *item, int state);
int                 menu_switch_get(struct menu_item *item);
void                menu_switch_toggle(struct menu_item *item);
struct menu_item   *menu_add_button(struct menu *menu, int x, int y,
                                    const char *name,
                                    menu_button_callback callback_proc,
                                    void *callback_data, int priority);
#ifdef __cplusplus
}
#endif

#endif
