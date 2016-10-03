#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "z64.h"
#include "sys.h"
#include "gz.h"

static char *b_ptr = 0;
static int b_width;
static int b_height;
static int b_size;
static int b_pos;
static int c_x = 0;
static int c_y = 0;
static int c_cx = 0;
static int c_cy = 0;
static int c_width = 40;
static int c_height = 30;
static int c_n = 0;


static int string_bufsize(const char *ptr, int size)
{
  int s_size = 0;
  int s_pos = 0;
  int line_pos = b_pos % b_width;
  while (size--) {
    int chr = *(unsigned char*)ptr++;
    int chr_size;
    switch (chr) {
    case '\t': chr_size = 8 - line_pos % 8; break;
    case '\n': chr_size = b_width - line_pos; break;
    case '\r': chr_size = -line_pos; break;
    default:
      chr_size = (chr >= 0x20 && chr < 0x7F) ||
                 (chr >= 0xA0 && chr < 0xE0);
      break;
    }
    line_pos = (line_pos + chr_size) % b_width;
    s_pos += chr_size;
    if (s_pos > s_size)
      s_size = s_pos;
  }
  return s_size;
}

static void put_string(const char *ptr, int size)
{
  int s_size = string_bufsize(ptr, size);
  int s_pos = 0;
  if (b_pos + s_size >= b_size) {
    int s_total = s_size + b_width - (b_pos + s_size) % b_width;
    int s_lines = ((b_pos + s_total) - (b_pos / b_width * b_width) +
                   b_width - 1) / b_width * b_width - b_width;
    if (s_lines > b_size)
      s_lines = b_size;
    if (s_total < b_size) {
      memmove(b_ptr, b_ptr + s_lines, b_size - s_lines);
      b_pos = b_size - s_total;
    }
    else {
      b_pos = 0;
      s_pos = b_size - s_total;
    }
    memset(b_ptr + b_size - s_lines, ' ', s_lines);
  }
  int line_pos = (b_pos + s_pos) % b_width;
  if (line_pos < 0)
    line_pos += b_width;
  while (size--) {
    int chr = *(unsigned char*)ptr++;
    int chr_size;
    switch (chr) {
    case '\t': chr_size = 8 - line_pos % 8; break;
    case '\n': chr_size = b_width - line_pos; break;
    case '\r': chr_size = -line_pos; break;
    default:
      if ((chr >= 0x20 && chr < 0x7F) || (chr >= 0xA0 && chr < 0xE0)) {
        chr_size = 1;
        if (b_pos + s_pos >= 0)
          b_ptr[b_pos + s_pos] = chr;
      }
      else
        chr_size = 0;
      break;
    }
    line_pos = (line_pos + chr_size) % b_width;
    s_pos += chr_size;
  }
  b_pos += s_pos;
}

static void scroll_to_cursor()
{
  int cursor_x = b_pos % b_width;
  int cursor_y = b_pos / b_width;
  if (c_cx > cursor_x)
    c_cx = cursor_x;
  if (c_cx + c_width <= cursor_x)
    c_cx = cursor_x - c_width + 1;
  if (c_cy > cursor_y)
    c_cy = cursor_y;
  if (c_cy + c_height <= cursor_y)
    c_cy = cursor_y - c_height + 1;
}

static void correct_coords()
{
  if (b_width < 1)
    b_width = 1;
  if (b_height < 1)
   b_height = 1;
  if (c_width < 0)
    c_width = 0;
  if (c_width > b_width)
    c_width = b_width;
  if (c_height < 0)
    c_height = 0;
  if (c_height > b_height)
    c_height = b_height;
  if (c_cx < 0)
    c_cx = 0;
  if (c_cx + c_width > b_width)
    c_cx = b_width - c_width;
  if (c_cy < 0)
    c_cy = 0;
  if (c_cy + c_height > b_height)
    c_cy = b_height - c_height;
}

static int stdout_handler(const char *ptr, int size)
{
  if (size > 0 && b_ptr) {
    put_string(ptr, size);
    scroll_to_cursor();
    c_n = 0;
  }
  return size;
}

void console_init(int width, int height)
{
  b_width = width;
  b_height = height;
  b_pos = 0;
  c_cx = 0;
  c_cy = 0;
  correct_coords();
  b_size = b_width * b_height;
  setvbuf(stdout, 0, _IOFBF, BUFSIZ);
  set_stdout(0);
  if (b_ptr)
    free(b_ptr);
  b_ptr = malloc(b_size);
  memset(b_ptr, ' ', b_size);
  fflush(stdout);
  set_stdout(stdout_handler);
}

void console_set_view(int x, int y, int width, int height)
{
  c_x = x;
  c_y = y;
  c_width = width;
  c_height = height;
  correct_coords();
}

void console_scroll(int cx, int cy)
{
  c_cx += cx;
  c_cy += cy;
  correct_coords();
}

void console_print()
{
  fflush(stdout);
  SetTextRGBA(g_text_ptr, 0xFF, 0xFF, 0xFF, 0xFF);
  for (int i = 0; i < c_height; ++i) {
    SetTextXY(g_text_ptr, c_x, c_y + i);
    SetTextString(g_text_ptr, "%.*s", c_width,
                  b_ptr + b_width * (c_cy + i) + c_cx);
  }
  int cursor_x = b_pos % b_width;
  int cursor_y = b_pos / b_width;
  if (c_n < 10 && c_cx <= cursor_x && c_cx + c_width > cursor_x &&
      c_cy <= cursor_y && c_cy + c_height > cursor_y)
  {
    SetTextXY(g_text_ptr, c_x + cursor_x - c_cx, c_y + cursor_y - c_cy);
    SetTextString(g_text_ptr, "_");
  }
  c_n = (c_n + 1) % 20;
}
