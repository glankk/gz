#include <vector/vector.h>
#include "gz.h"
#include "z64.h"

void z_to_movie(z64_input_t *zi, struct movie_input *mi)
{
  mi->raw = zi->raw;
  mi->raw_prev = zi->raw_prev;
  mi->pad_pressed = zi->pad_pressed;
  mi->pad_released = zi->pad_released;
  mi->x_diff = zi->x_diff;
  mi->y_diff = zi->y_diff;
}

void movie_to_z(z64_input_t *zi, struct movie_input *mi)
{
  zi->raw = mi->raw;
  zi->raw_prev = mi->raw_prev;
  zi->pad_pressed = mi->pad_pressed;
  zi->pad_released = mi->pad_released;
  zi->x_diff = mi->x_diff;
  zi->y_diff = mi->y_diff;
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
  gz.movie_seed_pos = 0;
  for (int i = 0; i < gz.movie_seeds.size; ++i) {
    struct movie_seed *ms = vector_at(&gz.movie_seeds, i);
    if (ms->frame_idx >= gz.movie_frame) {
      gz.movie_seed_pos = i;
      break;
    }
  }
}
