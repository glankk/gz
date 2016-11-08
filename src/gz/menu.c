#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <list/list.h>
#include "z64.h"
#include "gz.h"
#include "menu.h"
#include "gfx.h"


void menu_init(struct menu *menu)
{
  list_init(&menu->items, sizeof(struct menu_item));
  menu->selector = NULL;
  menu->parent = NULL;
  menu->child = NULL;
  menu->animate_highlight = 0;
  menu->highlight_color_animated = 0x000000;
  menu->highlight_color_static = 0x5050FF;
  menu->highlight_state[0] = 17;
  menu->highlight_state[1] = 19;
  menu->highlight_state[2] = 23;
}

struct menu_item *menu_add_item(struct menu *menu, struct menu_item *item)
{
  return list_push_back(&menu->items, item);
}

void menu_draw(struct menu *menu)
{
  if (menu->child)
    return menu_draw(menu->child);
  for (int i = 0; i < 3; ++i) {
    int shift = i * 8;
    uint32_t mask = 0xFF << shift;
    int v = (menu->highlight_color_animated & mask) >> shift;
    v += menu->highlight_state[i];
    if (v < 0x00 || v > 0xFF) {
      v = -v + (v > 0xFF ? 2 * 0xFF : 0);
      menu->highlight_state[i] = -menu->highlight_state[i];
    }
    menu->highlight_color_animated &= ~mask;
    menu->highlight_color_animated |= (uint32_t)v << shift;
  }
  for (struct menu_item *item = menu->items.first;
       item; item = list_next(item))
  {
    if (item->think_proc && item->think_proc(menu, item))
      continue;
    if (!item->text)
      continue;
    uint32_t color = (item == menu->selector ?
                      (menu->animate_highlight ?
                       menu->highlight_color_animated :
                       menu->highlight_color_static) :
                      item->color);
    gfx_mode_color((color >> 16) & 0xFF,
                   (color >> 8)  & 0xFF,
                   (color >> 0)  & 0xFF,
                   0xFF);
    gfx_printf(menu_font, item->x * 6, item->y * 8, "%s", item->text);
  }
}

void menu_navigate(struct menu *menu, enum menu_navigation nav)
{
  if (menu->child)
    return menu_navigate(menu->child, nav);
  if (!menu->selector) {
    for (struct menu_item *item = menu->items.first;
         item; item = list_next(item))
    {
      if (item->priority >= 0) {
        menu->selector = item;
        break;
      }
    }
    return;
  }
  if (menu->selector->navigate_proc &&
      menu->selector->navigate_proc(menu, menu->selector, nav))
    return;
  int x = (nav == MENU_NAVIGATE_LEFT ?
           -1 : (nav == MENU_NAVIGATE_RIGHT ? 1 : 0));
  int y = (nav == MENU_NAVIGATE_UP ?
           -1 : (nav == MENU_NAVIGATE_DOWN ? 1 : 0));
  if (x == 0 && y == 0)
    return;
  int near_distance_pa = 0;
  int far_distance_pa = 0;
  int near_distance_pe = 0;
  int far_distance_pe = 0;
  struct menu_item *near_item = NULL;
  struct menu_item *far_item = NULL;
  for (struct menu_item *item = menu->items.first;
       item; item = list_next(item))
  {
    if (item == menu->selector || item->priority < menu->selector->priority)
      continue;
    int distance_x = item->x - menu->selector->x;
    int distance_y = item->y - menu->selector->y;
    int distance_pa = (x ? distance_x * x : distance_y * y);
    int distance_pe = (y ? distance_x : distance_y);
    if (distance_pe < 0)
      distance_pe = -distance_pe;
    if (distance_pa > 0 && (near_item == NULL ||
                            distance_pa < near_distance_pa ||
                            (distance_pa == near_distance_pa &&
                             distance_pe < near_distance_pe)))
    {
      near_distance_pa = distance_pa;
      near_distance_pe = distance_pe;
      near_item = item;
    }
    if (distance_pa < 0 && (far_item == NULL ||
                            -distance_pa > far_distance_pa ||
                            (-distance_pa == far_distance_pa &&
                             distance_pe < far_distance_pe)))
    {
      far_distance_pa = -distance_pa;
      far_distance_pe = distance_pe;
      far_item = item;
    }
  }
  if (near_item)
    menu->selector = near_item;
  else if (far_item)
    menu->selector = far_item;
}

void menu_activate(struct menu *menu)
{
  if (menu->child)
    return menu_activate(menu->child);
  if (menu->selector && menu->selector->activate_proc)
    menu->selector->activate_proc(menu, menu->selector);
}

void menu_enter(struct menu *menu, struct menu *submenu)
{
  if (menu->child)
    return menu_enter(menu->child, submenu);
  menu->child = submenu;
  submenu->parent = menu;
}

struct menu *menu_return(struct menu *menu)
{
  if (menu->child)
    return menu_return(menu->child);
  struct menu *parent = menu->parent;
  if (!parent)
    return NULL;
  menu->parent = NULL;
  parent->child = NULL;
  return parent;
}

void menu_item_init(struct menu_item *item, int x, int y,
                    const char *text, uint32_t color)
{
  item->x = x;
  item->y = y;
  if (text) {
    item->text = malloc(strlen(text) + 1);
    strcpy(item->text, text);
  }
  else
    item->text = NULL;
  item->color = color;
  item->priority = 0;
  item->data = NULL;
  item->think_proc = NULL;
  item->navigate_proc = NULL;
  item->activate_proc = NULL;
  item->move_proc = NULL;
  item->remove_proc = NULL;
}

void menu_item_move(struct menu *menu, struct menu_item *item, int x, int y)
{
  if (!item->move_proc || !item->move_proc(menu, item, x, y)) {
    item->x = x;
    item->y = y;
  }
}

void menu_item_remove(struct menu *menu, struct menu_item *item)
{
  if (!item->remove_proc || !item->remove_proc(menu, item)) {
    if (item->text)
      free(item->text);
    if (item->data)
      free(item->data);
    list_erase(&menu->items, item);
  }
  if (menu->selector == item)
    menu->selector = NULL;
}

struct menu_item *menu_add_static(struct menu *menu, int x, int y,
                                  const char *text, uint32_t color)
{
  struct menu_item *item = menu_add_item(menu, NULL);
  menu_item_init(item, x, y, text, color);
  item->priority = INT_MIN;
  return item;
}
