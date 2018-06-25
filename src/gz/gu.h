#ifndef GU_H
#define GU_H
#include <stdint.h>
#include <n64.h>

#define                           M_PI 3.14159265358979323846
#define guDefMtxF(xx,xy,xz,xw,  \
                  yx,yy,yz,yw,  \
                  zx,zy,zz,zw,  \
                  wx,wy,wz,ww)    {.f={xx,xy,xz,xw,                           \
                                       yx,yy,yz,yw,                           \
                                       zx,zy,zz,zw,                           \
                                       wx,wy,wz,ww}}

typedef float MtxF_t[4][4];
typedef union
{
  MtxF_t  mf;
  float   f[16];
  struct
  {
    float xx, xy, xz, xw,
          yx, yy, yz, yw,
          zx, zy, zz, zw,
          wx, wy, wz, ww;
  };
} MtxF;

void guMtxIdent(Mtx *m);
void guMtxIdentF(MtxF *mf);
void guPerspectiveF(MtxF *mf, uint16_t *perspNorm, float fovy, float aspect,
                    float near, float far, float scale);
void guMtxCatF(const MtxF *m, const MtxF *n, MtxF *r);
void guRotateF(MtxF *mf, float a, float x, float y, float z);
void guRotateRPYF(MtxF *mf, float r, float p, float h);
void guScaleF(MtxF *mf, float x, float y, float z);
void guTranslateF(MtxF *mf, float x, float y, float z);
void guMtxF2L(const MtxF *mf, Mtx *m);

#endif
