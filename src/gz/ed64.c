#include <n64.h>
#include "util.h"

static int lat;
static int pwd;
static int irqf;

void ed64_get_access(void)
{
  __osPiGetAccess();

  irqf = set_irqf(0);

  lat = pi_regs.dom2_lat;
  pwd = pi_regs.dom2_pwd;

  pi_regs.dom2_lat = 4;
  pi_regs.dom2_pwd = 9;
}

void ed64_rel_access(void)
{
  pi_regs.dom2_lat = lat;
  pi_regs.dom2_pwd = pwd;

  __osPiRelAccess();

  set_irqf(irqf);
}
