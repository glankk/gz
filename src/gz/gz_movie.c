#include <vector/vector.h>
#include "gz.h"
#include "z64.h"

void z_to_movie(int movie_frame, z64_input_t *zi, _Bool reset)
{
  struct movie_input *mi = vector_at(&gz.movie_input, movie_frame);
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
    struct movie_input *mi_prev = vector_at(&gz.movie_input, movie_frame - 1);
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
  struct movie_input *mi = vector_at(&gz.movie_input, movie_frame);
  z64_controller_t *raw_prev;
  if (movie_frame == 0)
    raw_prev = &gz.movie_input_start;
  else {
    struct movie_input *mi_prev = vector_at(&gz.movie_input, movie_frame - 1);
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
  zi->adjusted_x = zu_adjust_joystick(zi->raw.x);
  zi->adjusted_y = zu_adjust_joystick(zi->raw.y);
}

void gz_movie_rewind(void)
{
  gz.movie_frame = 0;
  gz.movie_seed_pos = 0;
  gz.movie_oca_input_pos = 0;
  gz.movie_oca_sync_pos = 0;
  gz.movie_room_load_pos = 0;
}

void gz_movie_seek(int frame)
{
  int pos;
  /* set frame number */
  if (frame > gz.movie_input.size)
    frame = gz.movie_input.size;
  gz.movie_frame = frame;
  /* seek seed pos */
  pos = 0;
  while (pos < gz.movie_seed.size) {
    struct movie_seed *ms = vector_at(&gz.movie_seed, pos);
    if (ms->frame_idx > gz.movie_frame)
      break;
    ++pos;
  }
  gz.movie_seed_pos = pos;
  /* seek ocarina input pos */
  pos = 0;
  while (pos < gz.movie_oca_input.size) {
    struct movie_oca_input *oi = vector_at(&gz.movie_oca_input, pos);
    if (oi->frame_idx > gz.movie_frame)
      break;
    ++pos;
  }
  gz.movie_oca_input_pos = pos;
  /* seek ocarina sync pos */
  pos = 0;
  while (pos < gz.movie_oca_sync.size) {
    struct movie_oca_sync *os = vector_at(&gz.movie_oca_sync, pos);
    if (os->frame_idx > gz.movie_frame)
      break;
    ++pos;
  }
  gz.movie_oca_sync_pos = pos;
  /* seek room load pos */
  pos = 0;
  while (pos < gz.movie_room_load.size) {
    struct movie_room_load *rl = vector_at(&gz.movie_room_load, pos);
    if (rl->frame_idx > gz.movie_frame)
      break;
    ++pos;
  }
  gz.movie_room_load_pos = pos;
}
