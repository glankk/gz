#include "input.h"
#include "menu.h"
#include "osk.h"
#include "resource.h"

static char               osk_buf[32];
static osk_callback_t     osk_callback_proc;
static void              *osk_callback_data;
static struct menu        osk_menu;
static struct menu_item  *osk_text;
static struct menu_item  *osk_letters[26];
static struct menu_item  *osk_minus;
static int                osk_cursor_pos;
static _Bool              osk_shift_state;

static void insert_char(char c)
{
  if (osk_cursor_pos < sizeof(osk_buf) - 1) {
    memmove(&osk_buf[osk_cursor_pos + 1], &osk_buf[osk_cursor_pos],
            sizeof(osk_buf) - osk_cursor_pos - 1);
    osk_buf[sizeof(osk_buf) - 1] = 0;
    osk_buf[osk_cursor_pos++] = c;
  }
}

static void key_proc(struct menu_item *item, void *data)
{
  insert_char(item->text[0]);
}

static void space_proc(struct menu_item *item, void *data)
{
  insert_char(' ');
}

static int navigate_proc(struct menu_item *item, enum menu_navigation nav)
{
  if (input_pad() & BUTTON_Z) {
    switch (nav) {
      case MENU_NAVIGATE_UP: {
        osk_shift_state = !osk_shift_state;
        for (int i = 0; i < 26; ++i)
          osk_letters[i]->text[0] += (osk_shift_state ? 'A' - 'a' : 'a' - 'A' );
        osk_minus->text[0] = (osk_shift_state ? '_' : '-');
        break;
      }
      case MENU_NAVIGATE_DOWN: {
        insert_char(' ');
        break;
      }
      case MENU_NAVIGATE_LEFT: {
        if (osk_cursor_pos > 0) {
          memmove(&osk_buf[osk_cursor_pos - 1], &osk_buf[osk_cursor_pos],
                  sizeof(osk_buf) - osk_cursor_pos);
          --osk_cursor_pos;
        }
        break;
      }
      case MENU_NAVIGATE_RIGHT: {
        if (osk_cursor_pos < sizeof(osk_buf)) {
          memmove(&osk_buf[osk_cursor_pos], &osk_buf[osk_cursor_pos + 1],
                  sizeof(osk_buf) - osk_cursor_pos - 1);
        }
        break;
      }
    }
    return 1;
  }
  else
    return 0;
}

static int activate_text_proc(struct menu_item *item)
{
  if (!osk_callback_proc || !osk_callback_proc(osk_buf, osk_callback_data))
    menu_return(&osk_menu);
  return 1;
}

static int navigate_text_proc(struct menu_item *item, enum menu_navigation nav)
{
  if (navigate_proc(item, nav))
    return 1;
  else {
    switch (nav) {
      case MENU_NAVIGATE_LEFT: {
        if (osk_cursor_pos > 0)
          --osk_cursor_pos;
        return 1;
      }
      case MENU_NAVIGATE_RIGHT: {
        if (osk_cursor_pos < strlen(osk_buf))
          ++osk_cursor_pos;
        return 1;
      }
      default:
        return 0;
    }
  }
}

static int draw_text_proc(struct menu_item *item,
                          struct menu_draw_params *draw_params)
{
  int cw = menu_get_cell_width(item->owner, 1);
  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  gfx_printf(draw_params->font, draw_params->x, draw_params->y, "%s", osk_buf);
  gfx_printf(draw_params->font, draw_params->x + osk_cursor_pos * cw,
             draw_params->y + 3, "_");
  return 0;
}

static void draw_shortcut(struct menu_item *item,
                          struct menu_draw_params *draw_params,
                          uint16_t bind, const char *desc)
{
  struct gfx_texture *texture = resource_get(RES_ICON_BUTTONS);
  int cw = menu_get_cell_width(item->owner, 1);
  int x = draw_params->x + (cw - texture->tile_width) / 2;
  int y = draw_params->y - (gfx_font_xheight(draw_params->font) +
                            texture->tile_height + 1) / 2;

  int n;
  for (n = 0; n < 4; ++n) {
    uint16_t c = bind_get_component(bind, n);
    if (c == BIND_END)
      break;
    struct gfx_sprite sprite =
    {
      texture, c,
      x + n * 10, y,
      1.f, 1.f,
    };
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(input_button_color[c],
                                               draw_params->alpha));
    gfx_sprite_draw(&sprite);
  }

  gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                             draw_params->alpha));
  gfx_printf(draw_params->font, draw_params->x + (n + 1) * 10, draw_params->y,
             "%s", desc);
}

static int draw_tooltip_proc(struct menu_item *item,
                             struct menu_draw_params *draw_params)
{
  int ch = menu_get_cell_height(item->owner, 1);

  draw_shortcut(item, draw_params, bind_make(2, BUTTON_Z, BUTTON_D_UP),
                "shift");
  draw_params->y += ch;
  draw_shortcut(item, draw_params, bind_make(2, BUTTON_Z, BUTTON_D_LEFT),
                "backspace");
  draw_params->y += ch;
  draw_shortcut(item, draw_params, bind_make(2, BUTTON_Z, BUTTON_D_DOWN),
                "space");
  draw_params->y += ch;
  draw_shortcut(item, draw_params, bind_make(2, BUTTON_Z, BUTTON_D_RIGHT),
                "delete");

  if (item->owner->selector == osk_text) {
    draw_params->y += ch;
    draw_shortcut(item, draw_params, bind_make(1, BUTTON_D_LEFT),
                  "cursor left");
    draw_params->y += ch;
    draw_shortcut(item, draw_params, bind_make(1, BUTTON_D_RIGHT),
                  "cursor right");
    gfx_mode_set(GFX_MODE_COLOR, GPACK_RGB24A8(draw_params->color,
                                               draw_params->alpha));
    draw_params->y += ch;
    gfx_printf(draw_params->font, draw_params->x, draw_params->y,
               "press to confirm");
  }

  return 1;
}

static void create_osk_menu(void)
{
  static _Bool ready = 0;
  if (!ready) {
    menu_init(&osk_menu, MENU_NOVALUE, MENU_NOVALUE, MENU_NOVALUE);

    osk_text = menu_item_add(&osk_menu, 0, 0, NULL, 0xFFFFFF);
    osk_menu.selector = osk_text;
    osk_text->activate_proc = activate_text_proc;
    osk_text->draw_proc = draw_text_proc;
    osk_text->navigate_proc = navigate_text_proc;

    struct menu_item *item;
    item = menu_add_button(&osk_menu, 0, 2, "1", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 2, 2, "2", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 4, 2, "3", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 6, 2, "4", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 8, 2, "5", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 10, 2, "6", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 12, 2, "7", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 14, 2, "8", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 16, 2, "9", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 18, 2, "0", key_proc, NULL);
    item->navigate_proc = navigate_proc;

    osk_letters[0] = menu_add_button(&osk_menu, 0, 4, "q", key_proc, NULL);
    osk_letters[1] = menu_add_button(&osk_menu, 2, 4, "w", key_proc, NULL);
    osk_letters[2] = menu_add_button(&osk_menu, 4, 4, "e", key_proc, NULL);
    osk_letters[3] = menu_add_button(&osk_menu, 6, 4, "r", key_proc, NULL);
    osk_letters[4] = menu_add_button(&osk_menu, 8, 4, "t", key_proc, NULL);
    osk_letters[5] = menu_add_button(&osk_menu, 10, 4, "y", key_proc, NULL);
    osk_letters[6] = menu_add_button(&osk_menu, 12, 4, "u", key_proc, NULL);
    osk_letters[7] = menu_add_button(&osk_menu, 14, 4, "i", key_proc, NULL);
    osk_letters[8] = menu_add_button(&osk_menu, 16, 4, "o", key_proc, NULL);
    osk_letters[9] = menu_add_button(&osk_menu, 18, 4, "p", key_proc, NULL);

    osk_letters[10] = menu_add_button(&osk_menu, 0, 6, "a", key_proc, NULL);
    osk_letters[11] = menu_add_button(&osk_menu, 2, 6, "s", key_proc, NULL);
    osk_letters[12] = menu_add_button(&osk_menu, 4, 6, "d", key_proc, NULL);
    osk_letters[13] = menu_add_button(&osk_menu, 6, 6, "f", key_proc, NULL);
    osk_letters[14] = menu_add_button(&osk_menu, 8, 6, "g", key_proc, NULL);
    osk_letters[15] = menu_add_button(&osk_menu, 10, 6, "h", key_proc, NULL);
    osk_letters[16] = menu_add_button(&osk_menu, 12, 6, "j", key_proc, NULL);
    osk_letters[17] = menu_add_button(&osk_menu, 14, 6, "k", key_proc, NULL);
    osk_letters[18] = menu_add_button(&osk_menu, 16, 6, "l", key_proc, NULL);
    osk_minus = menu_add_button(&osk_menu, 18, 6, "-", key_proc, NULL);
    osk_minus->navigate_proc = navigate_proc;

    osk_letters[19] = menu_add_button(&osk_menu, 2, 8, "z", key_proc, NULL);
    osk_letters[20] = menu_add_button(&osk_menu, 4, 8, "x", key_proc, NULL);
    osk_letters[21] = menu_add_button(&osk_menu, 6, 8, "c", key_proc, NULL);
    osk_letters[22] = menu_add_button(&osk_menu, 8, 8, "v", key_proc, NULL);
    osk_letters[23] = menu_add_button(&osk_menu, 10, 8, "b", key_proc, NULL);
    osk_letters[24] = menu_add_button(&osk_menu, 12, 8, "n", key_proc, NULL);
    osk_letters[25] = menu_add_button(&osk_menu, 14, 8, "m", key_proc, NULL);
    item = menu_add_button(&osk_menu, 16, 8, ",", key_proc, NULL);
    item->navigate_proc = navigate_proc;
    item = menu_add_button(&osk_menu, 18, 8, ".", key_proc, NULL);
    item->navigate_proc = navigate_proc;

    for (int i = 0; i < 26; ++i)
      osk_letters[i]->navigate_proc = navigate_proc;

    struct gfx_texture *osk_icons = resource_get(RES_ICON_OSK);
    item = menu_add_button_icon(&osk_menu, 0, 8, osk_icons, 0, 0xFFFFFF,
                                space_proc, NULL);
    item->navigate_proc = navigate_proc;

    item = menu_item_add(&osk_menu, 0, 10, NULL, 0xC0C0C0);
    item->draw_proc = draw_tooltip_proc;
    item->selectable = 0;

    ready = 1;
  }
}

void menu_get_osk_string(struct menu *menu, const char *dflt,
                         osk_callback_t callback_proc, void *callback_data)
{
  memset(osk_buf, 0, sizeof(osk_buf));
  if (dflt)
    strncpy(osk_buf, dflt, sizeof(osk_buf) - 1);
  osk_callback_proc = callback_proc;
  osk_callback_data = callback_data;
  osk_cursor_pos = 0;
  create_osk_menu();
  menu_enter(menu, &osk_menu);
}
