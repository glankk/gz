#include <mips.h>
#include <n64.h>
#include "ucode.h"
#include "z64.h"

_Alignas(8) __attribute__((weak))
char gspL3DEX2_fifoTextStart[0x1190] = "gspL3DEX2_fifoTextStart";
_Alignas(8) __attribute__((weak))
char gspL3DEX2_fifoDataStart[0x03F0] = "gspL3DEX2_fifoDataStart";

_Bool have_l3dex2(void)
{
  return gspL3DEX2_fifoTextStart[0] == 'J';
}

Gfx *load_l3dex2(Gfx **pgdl)
{
  if (have_l3dex2()) {
    gSPLoadUcode((*pgdl)++,
                 MIPS_KSEG0_TO_PHYS(gspL3DEX2_fifoTextStart),
                 MIPS_KSEG0_TO_PHYS(gspL3DEX2_fifoDataStart));
  }
  return *pgdl;
}

Gfx *unload_l3dex2(Gfx **pgdl, _Bool set_lights)
{
  if (have_l3dex2()) {
    gSPLoadUcode((*pgdl)++,
                 MIPS_KSEG0_TO_PHYS(gspF3DEX2_NoN_fifoTextStart),
                 MIPS_KSEG0_TO_PHYS(gspF3DEX2_NoN_fifoDataStart));
    if (set_lights) {
      Lights0 lites;
      lites.a.l.col[0] = lites.a.l.colc[0] = z64_game.lighting.ambient[0];
      lites.a.l.col[1] = lites.a.l.colc[1] = z64_game.lighting.ambient[1];
      lites.a.l.col[2] = lites.a.l.colc[2] = z64_game.lighting.ambient[2];
      gSPSetLights0((*pgdl)++, lites);
    }
  }
  return *pgdl;
}
