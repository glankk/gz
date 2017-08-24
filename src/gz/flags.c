#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <vector/vector.h>
#include "flags.h"
#include "gfx.h"
#include "menu.h"
#include "z64.h"

#define FLAG_LOG_LENGTH 16

struct flag_record
{
  int         word_size;
  int         length;
  void       *data;
  void       *comp;
  int         index_length;
  const char *name;
};

struct flag_event
{
  int   record_index;
  int   flag_index;
  _Bool value;
  char  description[32];
};

static struct vector records;
static struct vector events;

static void add_record(size_t word_size, size_t length, void *data,
                       const char *name)
{
  struct flag_record *record = vector_push_back(&records, 1, NULL);
  record->word_size = word_size;
  record->length = length;
  record->data = data;
  record->name = name;
  record->comp = calloc(length, word_size);
  int no_flags = word_size * 8 * length;
  record->index_length = ((int)(ceilf(log2f(no_flags))) + 3) / 4;
}

static void add_event(int record_index, int flag_index, _Bool value)
{
  if (events.size >= FLAG_LOG_LENGTH)
    vector_erase(&events, 0, 1);
  struct flag_record *r = vector_at(&records, record_index);
  struct flag_event *e = vector_push_back(&events, 1, NULL);
  e->record_index = record_index;
  e->flag_index = flag_index;
  e->value = value;
  sprintf(e->description, "%s[0x%0*x] := %i",
          r->name, r->index_length, flag_index, value);
}

static uint32_t get_flag_word(void *data, size_t word_size, int index)
{
  if (word_size == 1)
    return ((uint8_t*)data)[index];
  else if (word_size == 2)
    return ((uint16_t*)data)[index];
  else if (word_size == 4)
    return ((uint32_t*)data)[index];
  return 0;
}

static void modify_flag(void *data, size_t word_size, int flag_index,
                        _Bool value)
{
  int word = flag_index / (word_size * 8);
  int bit = flag_index % (word_size * 8);
  if (word_size == 1) {
    uint8_t *p = data;
    if (value)
      p[word] |= (1 << bit);
    else
      p[word] &= ~(1 << bit);
  }
  else if (word_size == 2) {
    uint16_t *p = data;
    if (value)
      p[word] |= (1 << bit);
    else
      p[word] &= ~(1 << bit);
  }
  else if (word_size == 4) {
    uint32_t *p = data;
    if (value)
      p[word] |= (1 << bit);
    else
      p[word] &= ~(1 << bit);
  }
}

static void check_flags(void)
{
  for (int i = 0; i < records.size; ++i) {
    struct flag_record *r = vector_at(&records, i);
    for (int j = 0; j < r->length; ++j) {
      uint32_t wd = get_flag_word(r->data, r->word_size, j);
      uint32_t wc = get_flag_word(r->comp, r->word_size, j);
      uint32_t d = wd ^ wc;
      if (d != 0)
        for (int k = 0; k < r->word_size * 8; ++k)
          if ((d >> k) & 1)
            add_event(i, r->word_size * 8 * j + k, (wd >> k) & 1);
    }
  }
  update_flag_records();
}

static int draw_proc(struct menu_item *item,
                     struct menu_draw_params *draw_params)
{
  int x = draw_params->x;
  int y = draw_params->y;
  struct gfx_font *font = draw_params->font;
  uint32_t color = draw_params->color;
  uint8_t alpha = draw_params->alpha;
  int ch = menu_get_cell_height(item->owner, 1);
  gfx_mode_set(GFX_MODE_COLOR, (color << 8) | alpha);

  check_flags();
  for (int i = 0; i < events.size && i < FLAG_LOG_LENGTH; ++i) {
    struct flag_event *e = vector_at(&events, events.size - i - 1);
    gfx_printf(font, x, y + ch * i, "%s", e->description);
  }

  return 1;
}

static void undo_proc(struct menu_item *item, void *data)
{
  if (events.size == 0)
    return;
  struct flag_event *e = vector_at(&events, events.size - 1);
  struct flag_record *r = vector_at(&records, e->record_index);
  modify_flag(r->data, r->word_size, e->flag_index, !e->value);
  modify_flag(r->comp, r->word_size, e->flag_index, !e->value);
  vector_erase(&events, events.size - 1, 1);
}

void flag_log_create(struct menu *menu)
{
  menu_init(menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);
  menu->selector = menu_add_submenu(menu, 0, 0, NULL, "return");
  menu_add_static_custom(menu, 0, 2, draw_proc, NULL, 0xC0C0C0);
  menu_add_button(menu, 0, 1, "undo", undo_proc, NULL);
  vector_init(&records, sizeof(struct flag_record));
  vector_init(&events, sizeof(struct flag_event));
  add_record(1, 56, z64_file.gs_flags, "gs");
  add_record(2, 14, z64_file.event_chk_inf, "event_chk_inf");
  add_record(2, 4, z64_file.item_get_inf, "item_get_inf");
  add_record(2, 30, z64_file.inf_table, "inf_table");
  add_record(4, 1, &z64_game.switch_flags, "switch");
  add_record(4, 1, &z64_game.temp_switch_flags, "temp_switch");
  add_record(4, 1, &z64_game.chest_flags, "chest");
  add_record(4, 1, &z64_game.room_clear_flags, "clear");
  add_record(4, 1, &z64_game.collectible_flags, "collect");
}

void update_flag_records(void)
{
  for (int i = 0; i < records.size; ++i) {
    struct flag_record *r = vector_at(&records, i);
    memcpy(r->comp, r->data, r->word_size * r->length);
  }
}
