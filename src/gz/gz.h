#ifndef GZ_H
#define GZ_H
#include <stdint.h>
#include "z64.h"

struct memory_file
{
  z64_file_t  z_file;
  uint16_t    scene_index;
  uint32_t    scene_flags[9];
};

void save_memfile(struct memory_file *memfile);
void load_memfile(struct memory_file *memfile);

#endif
