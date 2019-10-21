#include <mips.h>
#include <n64.h>
#include "ucode.h"
#include "z64.h"
#include "zu.h"

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

Gfx *unload_l3dex2(Gfx **pgdl)
{
  if (have_l3dex2()) {
    gSPLoadUcode((*pgdl)++,
                 MIPS_KSEG0_TO_PHYS(gspF3DEX2_NoN_fifoTextStart),
                 MIPS_KSEG0_TO_PHYS(gspF3DEX2_NoN_fifoDataStart));
  }
  return *pgdl;
}
