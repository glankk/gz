#ifndef STATE_H
#define STATE_H
#include <stdint.h>

struct state_meta
{
  uint16_t              z64_version;
  uint16_t              state_version;
  uint32_t              size;
  uint16_t              scene_idx;
  int                   movie_frame;
};

uint32_t      save_state(void *state);
void          load_state(void *state);

#endif
