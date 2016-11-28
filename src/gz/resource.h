#ifndef RESOURCE_H
#define RESOURCE_H

enum resource_id
{
  RES_ZICON_ITEM,
  RES_ZFONT_NES,
  RES_FONT_ORIGAMIMOMMY10,
  RES_TEXTURE_CROSSHAIR,
  RES_MAX,
};

void *resource_get(enum resource_id res);
void resource_free(enum resource_id res);

#endif
