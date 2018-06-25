#ifndef UCODE_H
#define UCODE_H
#include <n64.h>

extern char gspL3DEX2_fifoTextStart[0x1190];
extern char gspL3DEX2_fifoDataStart[0x03F0];

_Bool   have_l3dex2(void);
Gfx    *load_l3dex2(Gfx **pgdl);
Gfx    *unload_l3dex2(Gfx **pgdl, _Bool set_lights);

#endif
