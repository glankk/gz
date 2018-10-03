#include <vector/vector.h>
#include "gz.h"
#include "z64.h"

void z_to_movie(int movie_frame, z64_input_t *zi, _Bool reset)
{
  struct movie_input *mi = vector_at(&gz.movie_inputs, movie_frame);
  z64_controller_t *raw_prev;
  if (movie_frame == 0) {
    raw_prev = &gz.movie_input_start;
    raw_prev->x = zi->raw.x - zi->x_diff;
    raw_prev->y = zi->raw.y - zi->y_diff;
    raw_prev->pad = (zi->raw.pad |
                     (~zi->pad_pressed & zi->pad_released)) &
                    ~(zi->pad_pressed & ~zi->pad_released);
  }
  else {
    struct movie_input *mi_prev = vector_at(&gz.movie_inputs, movie_frame - 1);
    raw_prev = &mi_prev->raw;
  }
  mi->raw = zi->raw;
  mi->pad_delta = (~mi->raw.pad & raw_prev->pad & zi->pad_pressed) |
                  (mi->raw.pad & ~raw_prev->pad & zi->pad_released) |
                  (zi->pad_pressed & zi->pad_released);
  mi->pad_delta |= reset << 7;
}

void movie_to_z(int movie_frame, z64_input_t *zi, _Bool *reset)
{
  struct movie_input *mi = vector_at(&gz.movie_inputs, movie_frame);
  z64_controller_t *raw_prev;
  if (movie_frame == 0)
    raw_prev = &gz.movie_input_start;
  else {
    struct movie_input *mi_prev = vector_at(&gz.movie_inputs, movie_frame - 1);
    raw_prev = &mi_prev->raw;
  }
  uint16_t delta = mi->pad_delta;
  if (reset)
    *reset = delta & 0x0080;
  delta &= ~0x0080;
  zi->raw = mi->raw;
  zi->raw_prev = *raw_prev;
  zi->pad_pressed = (mi->raw.pad & ~raw_prev->pad) | delta;
  zi->pad_released = (~mi->raw.pad & raw_prev->pad) | delta;
  zi->x_diff = mi->raw.x - raw_prev->x;
  zi->y_diff = mi->raw.y - raw_prev->y;
  if (zi->raw.x < 8) {
    if (zi->raw.x > -8)
      zi->adjusted_x = 0;
    else if (zi->raw.x < -66)
      zi->adjusted_x = -60;
    else
      zi->adjusted_x = zi->raw.x + 7;
  }
  else if (zi->raw.x > 66)
    zi->adjusted_x = 60;
  else
    zi->adjusted_x = zi->raw.x - 7;
  if (zi->raw.y < 8) {
    if (zi->raw.y > -8)
      zi->adjusted_y = 0;
    else if (zi->raw.y < -66)
      zi->adjusted_y = -60;
    else
      zi->adjusted_y = zi->raw.y + 7;
  }
  else if (zi->raw.y > 66)
    zi->adjusted_y = 60;
  else
    zi->adjusted_y = zi->raw.y - 7;
}

void gz_movie_rewind(void)
{
  gz.movie_frame = 0;
  gz.movie_seed_pos = 0;
}

void gz_movie_seek(int frame)
{
  if (frame > gz.movie_inputs.size)
    frame = gz.movie_inputs.size;
  gz.movie_frame = frame;
  int seed_pos = 0;
  while (seed_pos < gz.movie_seeds.size) {
    struct movie_seed *ms = vector_at(&gz.movie_seeds, seed_pos);
    if (ms->frame_idx >= gz.movie_frame)
      break;
    ++seed_pos;
  }
  gz.movie_seed_pos = seed_pos;
}
