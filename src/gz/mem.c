#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <vector/vector.h>
#include "input.h"
#include "mem.h"
#include "menu.h"
#include "resource.h"

#define MEM_VIEW_ROWS     16
#define MEM_VIEW_COLS     8
#define MEM_VIEW_SIZE     ((MEM_VIEW_COLS)*(MEM_VIEW_ROWS))

static int                view_domain_index;
static int                view_data_size;
static struct menu_item  *view_address;
static struct menu_item  *view_domain_name;
static struct menu_item  *view_pageup;
static struct menu_item  *view_pagedown;
static struct menu_item  *view_rows[MEM_VIEW_ROWS];
static struct menu_item  *view_cells[MEM_VIEW_SIZE];

struct mem_domain
{
  size_t      start;
  size_t      size;
  const char *name;
  int         view_offset;
};

static struct vector domains;

static void add_domain(size_t start, size_t size, const char *name)
{
  struct mem_domain *domain = vector_push_back(&domains, 1, NULL);
  domain->start = start;
  domain->size = size;
  domain->name = name;
  domain->view_offset = 0;
}

static void update_view(void)
{
  struct mem_domain *d = vector_at(&domains, view_domain_index);
  if (d->size <= MEM_VIEW_SIZE || d->view_offset < 0)
    d->view_offset = 0;
  else if (d->view_offset + MEM_VIEW_SIZE > d->size)
    d->view_offset = d->size - MEM_VIEW_SIZE;
  menu_intinput_set(view_address, d->start + d->view_offset);
  strcpy(view_domain_name->text, d->name);
  view_pageup->enabled = view_pagedown->enabled = (d->size > MEM_VIEW_SIZE);
  for (int y = 0; y < MEM_VIEW_ROWS; ++y) {
    struct menu_item *row = view_rows[y];
    row->enabled = (d->view_offset + y * MEM_VIEW_COLS < d->size);
    if (row->enabled)
      sprintf(view_rows[y]->text, "%08x",
              d->start + d->view_offset + y * MEM_VIEW_COLS);
    for (int x = 0; x < MEM_VIEW_COLS; ++x) {
      int n = y * MEM_VIEW_COLS + x;
      if (n % view_data_size != 0)
        continue;
      struct menu_item *cell = view_cells[n];
      cell->enabled = (d->view_offset + n < d->size);
      if (cell->enabled)
        cell->think_proc(cell);
    }
  }
}

static int cell_proc(struct menu_item *item,
                     enum menu_callback_reason reason,
                     void *data)
{
  int cell_index = (int)data;
  struct mem_domain *d = vector_at(&domains, view_domain_index);
  switch (view_data_size) {
    case 1: {
      uint8_t *p = (void*)(d->start + d->view_offset + cell_index);
      if (reason == MENU_CALLBACK_THINK_INACTIVE) {
        uint8_t v = *p;
        if (menu_intinput_get(item) != v)
          menu_intinput_set(item, v);
      }
      else if (reason == MENU_CALLBACK_CHANGED)
        *p = menu_intinput_get(item);
      break;
    }
    case 2: {
      uint16_t *p = (void*)(d->start + d->view_offset + cell_index);
      if (reason == MENU_CALLBACK_THINK_INACTIVE) {
        uint16_t v = *p;
        if (menu_intinput_get(item) != v)
          menu_intinput_set(item, v);
      }
      else if (reason == MENU_CALLBACK_CHANGED)
        *p = menu_intinput_get(item);
      break;
    }
    case 4: {
      uint32_t *p = (void*)(d->start + d->view_offset + cell_index);
      if (reason == MENU_CALLBACK_THINK_INACTIVE) {
        uint32_t v = *p;
        if (menu_intinput_get(item) != v)
          menu_intinput_set(item, v);
      }
      else if (reason == MENU_CALLBACK_CHANGED)
        *p = menu_intinput_get(item);
      break;
    }
  }
  return 0;
}

static void make_cells(struct menu *menu)
{
  int n = 0;
  for (int y = 0; y < MEM_VIEW_ROWS; ++y)
    for (int x = 0; x < MEM_VIEW_COLS; ++x) {
      if (view_cells[n])
        menu_item_remove(view_cells[n]);
      if (n % view_data_size == 0)
        view_cells[n] = menu_add_intinput(menu, 9 + x * 2, 3 + y,
                                          16, view_data_size * 2,
                                          cell_proc, (void*)n);
      else
        view_cells[n] = NULL;
      ++n;
    }
}

static int address_proc(struct menu_item *item,
                        enum menu_callback_reason reason,
                        void *data)
{
  if (reason == MENU_CALLBACK_CHANGED)
    mem_goto(menu_intinput_get(item));
  return 0;
}

static int data_size_proc(struct menu_item *item,
                          enum menu_callback_reason reason,
                          void *data)
{
  if (reason == MENU_CALLBACK_DEACTIVATE) {
    int v = (1 << menu_option_get(item));
    if (view_data_size != v) {
      view_data_size = v;
      make_cells(item->owner);
      update_view();
    }
  }
  return 0;
}

static void goto_domain(int domain_index)
{
  view_domain_index = domain_index;
  update_view();
}

static void prev_domain_proc(struct menu_item *item, void *data)
{
  goto_domain((view_domain_index + domains.size - 1) % domains.size);
}

static void next_domain_proc(struct menu_item *item, void *data)
{
  goto_domain((view_domain_index + 1) % domains.size);
}

static void page_up_proc(struct menu_item *item, void *data)
{
  struct mem_domain *d = vector_at(&domains, view_domain_index);
  d->view_offset -= ((input_pad() & BUTTON_Z) ? MEM_VIEW_SIZE : MEM_VIEW_COLS);
  update_view();
}

static void page_down_proc(struct menu_item *item, void *data)
{
  struct mem_domain *d = vector_at(&domains, view_domain_index);
  d->view_offset += ((input_pad() & BUTTON_Z) ? MEM_VIEW_SIZE : MEM_VIEW_COLS);
  update_view();
}

void mem_menu_create(struct menu *menu)
{
  /* initialize data */
  vector_init(&domains, sizeof(struct mem_domain));
  add_domain(0x80000000, 0x00C00000, "k0 rdram");
  add_domain(0xA0000000, 0x00C00000, "k1 rdram");
  add_domain(0xA3F00000, 0x00100000, "rdram regs");
  add_domain(0xA4000000, 0x00100000, "sp regs");
  add_domain(0xA4100000, 0x00100000, "dp com");
  add_domain(0xA4200000, 0x00100000, "dp span");
  add_domain(0xA4300000, 0x00100000, "mi regs");
  add_domain(0xA4400000, 0x00100000, "vi regs");
  add_domain(0xA4500000, 0x00100000, "ai regs");
  add_domain(0xA4600000, 0x00100000, "pi regs");
  add_domain(0xA4800000, 0x00100000, "si regs");
  add_domain(0xA8000000, 0x08000000, "cart dom2");
  add_domain(0xB0000000, 0x0FC00000, "cart dom1");
  add_domain(0xBFC00000, 0x000007C0, "pif rom");
  add_domain(0xBFC007C0, 0x00000040, "pif ram");
  /* initialize menus */
  menu_init(menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu->selector = menu_add_submenu(menu, 0, 0, NULL, "return");
  {
    view_address = menu_add_intinput(menu, 0, 1, 16, 8, address_proc, NULL);
    struct menu_item *data_size = menu_add_option(menu, 9, 1,
                                                  "byte\0""halfword\0""word\0",
                                                  data_size_proc, NULL);
    menu_option_set(data_size, 2);
    view_data_size = 4;
    menu_add_button(menu, 18, 1, "<", prev_domain_proc, NULL);
    menu_add_button(menu, 20, 1, ">", next_domain_proc, NULL);
    view_domain_name = menu_add_static(menu, 22, 1, NULL, 0xC0C0C0);
    view_domain_name->text = malloc(32);
    struct gfx_texture *t_arrow = resource_get(RES_ICON_ARROW);
    view_pageup = menu_add_button_icon(menu, 0, 2, t_arrow, 0, 0xFFFFFF,
                                       page_up_proc, NULL);
    view_pagedown = menu_add_button_icon(menu, 2, 2, t_arrow, 1, 0xFFFFFF,
                                         page_down_proc, NULL);
    menu_add_static(menu, 9, 2, "0 1 2 3 4 5 6 7", 0xC0C0C0);
    for (int y = 0; y < MEM_VIEW_ROWS; ++y) {
      view_rows[y] = menu_add_static(menu, 0, 3 + y, NULL, 0xC0C0C0);
      view_rows[y]->text = malloc(9);
    }
    make_cells(menu);
    goto_domain(0);
  }
}

void mem_goto(uint32_t address)
{
  for (int i = 0; i < domains.size; ++i) {
    struct mem_domain *d = vector_at(&domains, i);
    if (address >= d->start && address < d->start + d->size) {
      view_domain_index = i;
      d->view_offset = address - d->start;
      break;
    }
  }
  update_view();
}
