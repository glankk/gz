#ifndef IQUE_H
#define IQUE_H
#include "z64.h"

static inline int is_ique(void)
{
#if Z64_VERSION == Z64_OOTIQC
  return __osBbIsBb;
#else
  return 0;
#endif
}

#endif
