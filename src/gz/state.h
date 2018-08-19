#ifndef STATE_H
#define STATE_H
#include <stdint.h>

struct state_meta
{
  uint32_t              size;
  uint16_t              scene_idx;
  int                   movie_frame;
};

uint32_t      save_state(void *state);
void          load_state(void *state);

#endif
