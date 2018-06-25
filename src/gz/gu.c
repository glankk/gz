#include <stdint.h>
#include <math.h>
#include <n64.h>
#include "gu.h"

void guMtxIdent(Mtx *m)
{
  *m = (Mtx)gdSPDefMtx(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       0, 0, 0, 1);
}

void guMtxIdentF(MtxF *mf)
{
  *mf = (MtxF)guDefMtxF(1.f, 0.f, 0.f, 0.f,
                        0.f, 1.f, 0.f, 0.f,
                        0.f, 0.f, 1.f, 0.f,
                        0.f, 0.f, 0.f, 1.f);
}

void guPerspectiveF(MtxF *mf, uint16_t *perspNorm, float fovy, float aspect,
                    float near, float far, float scale)
{
  float cot = cos(fovy / 2.f) / sin(fovy / 2.f);
  mf->xx = cot / aspect * scale;
  mf->xy = 0.f;
  mf->xz = 0.f;
  mf->xw = 0.f;
  mf->yx = 0.f;
  mf->yy = cot * scale;
  mf->yz = 0.f;
  mf->yw = 0.f;
  mf->zx = 0.f;
  mf->zy = 0.f;
  mf->zz = (near + far) / (near - far) * scale;
  mf->zw = -1.f * scale;
  mf->wx = 0.f;
  mf->wy = 0.f;
  mf->wz = 2.f * near * far / (near - far) * scale;
  mf->ww = 0.f;
}

void guMtxCatF(const MtxF *m, const MtxF *n, MtxF *r)
{
  MtxF t;
  t.xx = m->xx * n->xx + m->xy * n->yx + m->xz * n->zx + m->xw * n->wx;
  t.xy = m->xx * n->xy + m->xy * n->yy + m->xz * n->zy + m->xw * n->wy;
  t.xz = m->xx * n->xz + m->xy * n->yz + m->xz * n->zz + m->xw * n->wz;
  t.xw = m->xx * n->xw + m->xy * n->yw + m->xz * n->zw + m->xw * n->ww;
  t.yx = m->yx * n->xx + m->yy * n->yx + m->yz * n->zx + m->yw * n->wx;
  t.yy = m->yx * n->xy + m->yy * n->yy + m->yz * n->zy + m->yw * n->wy;
  t.yz = m->yx * n->xz + m->yy * n->yz + m->yz * n->zz + m->yw * n->wz;
  t.yw = m->yx * n->xw + m->yy * n->yw + m->yz * n->zw + m->yw * n->ww;
  t.zx = m->zx * n->xx + m->zy * n->yx + m->zz * n->zx + m->zw * n->wx;
  t.zy = m->zx * n->xy + m->zy * n->yy + m->zz * n->zy + m->zw * n->wy;
  t.zz = m->zx * n->xz + m->zy * n->yz + m->zz * n->zz + m->zw * n->wz;
  t.zw = m->zx * n->xw + m->zy * n->yw + m->zz * n->zw + m->zw * n->ww;
  t.wx = m->wx * n->xx + m->wy * n->yx + m->wz * n->zx + m->ww * n->wx;
  t.wy = m->wx * n->xy + m->wy * n->yy + m->wz * n->zy + m->ww * n->wy;
  t.wz = m->wx * n->xz + m->wy * n->yz + m->wz * n->zz + m->ww * n->wz;
  t.ww = m->wx * n->xw + m->wy * n->yw + m->wz * n->zw + m->ww * n->ww;
  *r = t;
}

void guRotateF(MtxF *mf, float a, float x, float y, float z)
{
  float s = sin(a);
  float c = cos(a);
  mf->xx = x * x + c * (1.f - x * x);
  mf->xy = x * y * (1.f - c) + z * s;
  mf->xz = x * z * (1.f - c) - y * s;
  mf->xw = 0.f;
  mf->yx = y * x * (1.f - c) - z * s;
  mf->yy = y * y + c * (1.f - y * y);
  mf->yz = y * z * (1.f - c) + x * s;
  mf->yw = 0.f;
  mf->zx = z * x * (1.f - c) + y * s;
  mf->zy = z * y * (1.f - c) - x * s;
  mf->zz = z * z + c * (1.f - z * z);
  mf->zw = 0.f;
  mf->wx = 0.f;
  mf->wy = 0.f;
  mf->wz = 0.f;
  mf->ww = 1.f;
}

void guRotateRPYF(MtxF *mf, float r, float p, float h)
{
  float sr = sin(r);
  float cr = cos(r);
  float sp = sin(p);
  float cp = cos(p);
  float sh = sin(h);
  float ch = cos(h);
  mf->xx = cp * ch;
  mf->xy = cp * sh;
  mf->xz = -sp;
  mf->xw = 0.f;
  mf->yx = sr * sp * ch - cr * sh;
  mf->yy = sr * sp * sh + cr * ch;
  mf->yz = sr * cp;
  mf->yw = 0.f;
  mf->zx = cr * sp * ch + sr * sh;
  mf->zy = cr * sp * sh - sp * sh;
  mf->zz = cr * cp;
  mf->zw = 0.f;
  mf->wx = 0.f;
  mf->wy = 0.f;
  mf->wz = 0.f;
  mf->ww = 1.f;
}

void guScaleF(MtxF *mf, float x, float y, float z)
{
  *mf = (MtxF)guDefMtxF(x,   0.f, 0.f, 0.f,
                        0.f, y,   0.f, 0.f,
                        0.f, 0.f, z,   0.f,
                        0.f, 0.f, 0.f, 1.f);
}

void guTranslateF(MtxF *mf, float x, float y, float z)
{
  *mf = (MtxF)guDefMtxF(1.f, 0.f, 0.f, 0.f,
                        0.f, 1.f, 0.f, 0.f,
                        0.f, 0.f, 1.f, 0.f,
                        x,   y,   z,   1.f);
}

void guMtxF2L(const MtxF *mf, Mtx *m)
{
  for (int i = 0; i < 16; ++i) {
    qs1616_t n = qs1616(mf->f[i]);
    m->i[i] = (n >> 16) & 0x0000FFFF;
    m->f[i] = (n >> 0)  & 0x0000FFFF;
  }
}
