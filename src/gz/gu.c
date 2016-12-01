#include <stdint.h>
#include <math.h>
#include <n64.h>
#include "gu.h"

void guMtxIdent(Mtx *m)
{
  m->i[0]   = 1; m->f[0]  = 0;
  m->i[1]   = 0; m->f[1]  = 0;
  m->i[2]   = 0; m->f[2]  = 0;
  m->i[3]   = 0; m->f[3]  = 0;
  m->i[4]   = 0; m->f[4]  = 0;
  m->i[5]   = 1; m->f[5]  = 0;
  m->i[6]   = 0; m->f[6]  = 0;
  m->i[7]   = 0; m->f[7]  = 0;
  m->i[8]   = 0; m->f[8]  = 0;
  m->i[9]   = 0; m->f[9]  = 0;
  m->i[10]  = 1; m->f[10] = 0;
  m->i[11]  = 0; m->f[11] = 0;
  m->i[12]  = 0; m->f[12] = 0;
  m->i[13]  = 0; m->f[13] = 0;
  m->i[14]  = 0; m->f[14] = 0;
  m->i[15]  = 1; m->f[15] = 0;
}

void guMtxIdentF(MtxF *mf)
{
  mf->xx = 1;
  mf->xy = 0;
  mf->xz = 0;
  mf->xw = 0;
  mf->yx = 0;
  mf->yy = 1;
  mf->yz = 0;
  mf->yw = 0;
  mf->zx = 0;
  mf->zy = 0;
  mf->zz = 1;
  mf->zw = 0;
  mf->wx = 0;
  mf->wy = 0;
  mf->wz = 0;
  mf->ww = 1;
}

void guPerspectiveF(MtxF *mf, uint16_t *perspNorm, float fovy, float aspect,
                    float near, float far, float scale)
{
  float cot = cos(fovy/2)/sin(fovy/2);
  mf->xx = cot/aspect*scale;
  mf->xy = 0;
  mf->xz = 0;
  mf->xw = 0;
  mf->yx = 0;
  mf->yy = cot*scale;
  mf->yz = 0;
  mf->yw = 0;
  mf->zx = 0;
  mf->zy = 0;
  mf->zz = (near+far)/(near-far)*scale;
  mf->zw = -1*scale;
  mf->wx = 0;
  mf->wy = 0;
  mf->wz = 2*near*far/(near-far)*scale;
  mf->ww = 0;
}

void guMtxCatF(const MtxF *m, const MtxF *n, MtxF *r)
{
  MtxF t;
  t.xx = m->xx*n->xx+m->xy*n->yx+m->xz*n->zx+m->xw*n->wx;
  t.xy = m->xx*n->xy+m->xy*n->yy+m->xz*n->zy+m->xw*n->wy;
  t.xz = m->xx*n->xz+m->xy*n->yz+m->xz*n->zz+m->xw*n->wz;
  t.xw = m->xx*n->xw+m->xy*n->yw+m->xz*n->zw+m->xw*n->ww;
  t.yx = m->yx*n->xx+m->yy*n->yx+m->yz*n->zx+m->yw*n->wx;
  t.yy = m->yx*n->xy+m->yy*n->yy+m->yz*n->zy+m->yw*n->wy;
  t.yz = m->yx*n->xz+m->yy*n->yz+m->yz*n->zz+m->yw*n->wz;
  t.yw = m->yx*n->xw+m->yy*n->yw+m->yz*n->zw+m->yw*n->ww;
  t.zx = m->zx*n->xx+m->zy*n->yx+m->zz*n->zx+m->zw*n->wx;
  t.zy = m->zx*n->xy+m->zy*n->yy+m->zz*n->zy+m->zw*n->wy;
  t.zz = m->zx*n->xz+m->zy*n->yz+m->zz*n->zz+m->zw*n->wz;
  t.zw = m->zx*n->xw+m->zy*n->yw+m->zz*n->zw+m->zw*n->ww;
  t.wx = m->wx*n->xx+m->wy*n->yx+m->wz*n->zx+m->ww*n->wx;
  t.wy = m->wx*n->xy+m->wy*n->yy+m->wz*n->zy+m->ww*n->wy;
  t.wz = m->wx*n->xz+m->wy*n->yz+m->wz*n->zz+m->ww*n->wz;
  t.ww = m->wx*n->xw+m->wy*n->yw+m->wz*n->zw+m->ww*n->ww;
  *r = t;
}

void guRotateF(MtxF *mf, float a, float x, float y, float z)
{
  float s = sin(a);
  float c = cos(a);
  mf->xx = x*x+c*(1-x*x);
  mf->xy = x*y*(1-c)+z*s;
  mf->xz = x*z*(1-c)-y*s;
  mf->xw = 0;
  mf->yx = y*x*(1-c)-z*s;
  mf->yy = y*y+c*(1-y*y);
  mf->yz = y*z*(1-c)+x*s;
  mf->yw = 0;
  mf->zx = z*x*(1-c)+y*s;
  mf->zy = z*y*(1-c)-x*s;
  mf->zz = z*z+c*(1-z*z);
  mf->zw = 0;
  mf->wx = 0;
  mf->wy = 0;
  mf->wz = 0;
  mf->ww = 1;
}

void guScaleF(MtxF *mf, float x, float y, float z)
{
  mf->xx = x;
  mf->xy = 0;
  mf->xz = 0;
  mf->xw = 0;
  mf->yx = 0;
  mf->yy = y;
  mf->yz = 0;
  mf->yw = 0;
  mf->zx = 0;
  mf->zy = 0;
  mf->zz = z;
  mf->zw = 0;
  mf->wx = 0;
  mf->wy = 0;
  mf->wz = 0;
  mf->ww = 1;
}

void guTranslateF(MtxF *mf, float x, float y, float z)
{
  mf->xx = 1;
  mf->xy = 0;
  mf->xz = 0;
  mf->xw = 0;
  mf->yx = 0;
  mf->yy = 1;
  mf->yz = 0;
  mf->yw = 0;
  mf->zx = 0;
  mf->zy = 0;
  mf->zz = 1;
  mf->zw = 0;
  mf->wx = x;
  mf->wy = y;
  mf->wz = z;
  mf->ww = 1;
}

void guMtxF2L(const MtxF *mf, Mtx *m)
{
  for (int i = 0; i < 16; ++i) {
    qs1616_t n = qs1616(mf->f[i]);
    m->i[i] = (n >> 16) & 0xFFFF;
    m->f[i] = (n >> 0)  & 0xFFFF;
  }
}
