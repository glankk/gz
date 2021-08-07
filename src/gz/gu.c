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

  if (perspNorm) {
    float n = 65536.f * 2.f / (near + far);
    if (n > 65535.f)
      n = 65535.f;
    else if (n < 1.f)
      n = 1.f;
    *perspNorm = n;
  }
}

void guLookAtF(MtxF *mf,
               float xEye, float yEye, float zEye,
               float xAt, float yAt, float zAt,
               float xUp, float yUp, float zUp)
{
  /* compute forward vector */
  float xFwd = xAt - xEye;
  float yFwd = yAt - yEye;
  float zFwd = zAt - zEye;
  float dFwd = sqrtf(xFwd * xFwd + yFwd * yFwd + zFwd * zFwd);
  xFwd /= dFwd;
  yFwd /= dFwd;
  zFwd /= dFwd;

  /* compute right vector */
  float xRight = yFwd * zUp - zFwd * yUp;
  float yRight = zFwd * xUp - xFwd * zUp;
  float zRight = xFwd * yUp - yFwd * xUp;
  float dRight = sqrtf(xRight * xRight + yRight * yRight + zRight * zRight);
  xRight /= dRight;
  yRight /= dRight;
  zRight /= dRight;

  /* compute up vector */
  xUp = yRight * zFwd - zRight * yFwd;
  yUp = zRight * xFwd - xRight * zFwd;
  zUp = xRight * yFwd - yRight * xFwd;
  float dUp = sqrtf(xUp * xUp + yUp * yUp + zUp * zUp);
  xUp /= dUp;
  yUp /= dUp;
  zUp /= dUp;

  /* compute translation vector */
  float xTrans = -(xEye * xRight + yEye * yRight + zEye * zRight);
  float yTrans = -(xEye * xUp    + yEye * yUp    + zEye * zUp);
  float zTrans = -(xEye * xFwd   + yEye * yFwd   + zEye * zFwd);

  *mf = (MtxF)guDefMtxF
  (
    xRight, xUp,    -xFwd,   0.f,
    yRight, yUp,    -yFwd,   0.f,
    zRight, zUp,    -zFwd,   0.f,
    xTrans, yTrans, -zTrans, 1.f,
  );
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
  mf->zy = cr * sp * sh - sr * ch;
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
    m->i[i] = n >> 16;
    m->f[i] = n;
  }
}

void guMtxL2F(MtxF *mf, const Mtx *m)
{
  for (int i = 0; i < 16; ++i) {
    qs1616_t v = (m->i[i] << 16) | m->f[i];
    mf->f[i] = v * (1.f / 0x10000);
  }
}

float guMtxDetF(const MtxF *mf)
{
  return mf->xw * mf->yz * mf->zy * mf->wx -
         mf->xz * mf->yw * mf->zy * mf->wx -
         mf->xw * mf->yy * mf->zz * mf->wx +
         mf->xy * mf->yw * mf->zz * mf->wx +
         mf->xz * mf->yy * mf->zw * mf->wx -
         mf->xy * mf->yz * mf->zw * mf->wx -
         mf->xw * mf->yz * mf->zx * mf->wy +
         mf->xz * mf->yw * mf->zx * mf->wy +
         mf->xw * mf->yx * mf->zz * mf->wy -
         mf->xx * mf->yw * mf->zz * mf->wy -
         mf->xz * mf->yx * mf->zw * mf->wy +
         mf->xx * mf->yz * mf->zw * mf->wy +
         mf->xw * mf->yy * mf->zx * mf->wz -
         mf->xy * mf->yw * mf->zx * mf->wz -
         mf->xw * mf->yx * mf->zy * mf->wz +
         mf->xx * mf->yw * mf->zy * mf->wz +
         mf->xy * mf->yx * mf->zw * mf->wz -
         mf->xx * mf->yy * mf->zw * mf->wz -
         mf->xz * mf->yy * mf->zx * mf->ww +
         mf->xy * mf->yz * mf->zx * mf->ww +
         mf->xz * mf->yx * mf->zy * mf->ww -
         mf->xx * mf->yz * mf->zy * mf->ww -
         mf->xy * mf->yx * mf->zz * mf->ww +
         mf->xx * mf->yy * mf->zz * mf->ww;
}

void guMtxInvertF(const MtxF *mf, MtxF *r)
{
  float d = guMtxDetF(mf);
  *r = (MtxF)guDefMtxF
  (
    (mf->yz * mf->zw * mf->wy -
     mf->yw * mf->zz * mf->wy +
     mf->yw * mf->zy * mf->wz -
     mf->yy * mf->zw * mf->wz -
     mf->yz * mf->zy * mf->ww +
     mf->yy * mf->zz * mf->ww) / d,
    (mf->xw * mf->zz * mf->wy -
     mf->xz * mf->zw * mf->wy -
     mf->xw * mf->zy * mf->wz +
     mf->xy * mf->zw * mf->wz +
     mf->xz * mf->zy * mf->ww -
     mf->xy * mf->zz * mf->ww) / d,
    (mf->xz * mf->yw * mf->wy -
     mf->xw * mf->yz * mf->wy +
     mf->xw * mf->yy * mf->wz -
     mf->xy * mf->yw * mf->wz -
     mf->xz * mf->yy * mf->ww +
     mf->xy * mf->yz * mf->ww) / d,
    (mf->xw * mf->yz * mf->zy -
     mf->xz * mf->yw * mf->zy -
     mf->xw * mf->yy * mf->zz +
     mf->xy * mf->yw * mf->zz +
     mf->xz * mf->yy * mf->zw -
     mf->xy * mf->yz * mf->zw) / d,
    (mf->yw * mf->zz * mf->wx -
     mf->yz * mf->zw * mf->wx -
     mf->yw * mf->zx * mf->wz +
     mf->yx * mf->zw * mf->wz +
     mf->yz * mf->zx * mf->ww -
     mf->yx * mf->zz * mf->ww) / d,
    (mf->xz * mf->zw * mf->wx -
     mf->xw * mf->zz * mf->wx +
     mf->xw * mf->zx * mf->wz -
     mf->xx * mf->zw * mf->wz -
     mf->xz * mf->zx * mf->ww +
     mf->xx * mf->zz * mf->ww) / d,
    (mf->xw * mf->yz * mf->wx -
     mf->xz * mf->yw * mf->wx -
     mf->xw * mf->yx * mf->wz +
     mf->xx * mf->yw * mf->wz +
     mf->xz * mf->yx * mf->ww -
     mf->xx * mf->yz * mf->ww) / d,
    (mf->xz * mf->yw * mf->zx -
     mf->xw * mf->yz * mf->zx +
     mf->xw * mf->yx * mf->zz -
     mf->xx * mf->yw * mf->zz -
     mf->xz * mf->yx * mf->zw +
     mf->xx * mf->yz * mf->zw) / d,
    (mf->yy * mf->zw * mf->wx -
     mf->yw * mf->zy * mf->wx +
     mf->yw * mf->zx * mf->wy -
     mf->yx * mf->zw * mf->wy -
     mf->yy * mf->zx * mf->ww +
     mf->yx * mf->zy * mf->ww) / d,
    (mf->xw * mf->zy * mf->wx -
     mf->xy * mf->zw * mf->wx -
     mf->xw * mf->zx * mf->wy +
     mf->xx * mf->zw * mf->wy +
     mf->xy * mf->zx * mf->ww -
     mf->xx * mf->zy * mf->ww) / d,
    (mf->xy * mf->yw * mf->wx -
     mf->xw * mf->yy * mf->wx +
     mf->xw * mf->yx * mf->wy -
     mf->xx * mf->yw * mf->wy -
     mf->xy * mf->yx * mf->ww +
     mf->xx * mf->yy * mf->ww) / d,
    (mf->xw * mf->yy * mf->zx -
     mf->xy * mf->yw * mf->zx -
     mf->xw * mf->yx * mf->zy +
     mf->xx * mf->yw * mf->zy +
     mf->xy * mf->yx * mf->zw -
     mf->xx * mf->yy * mf->zw) / d,
    (mf->yz * mf->zy * mf->wx -
     mf->yy * mf->zz * mf->wx -
     mf->yz * mf->zx * mf->wy +
     mf->yx * mf->zz * mf->wy +
     mf->yy * mf->zx * mf->wz -
     mf->yx * mf->zy * mf->wz) / d,
    (mf->xy * mf->zz * mf->wx -
     mf->xz * mf->zy * mf->wx +
     mf->xz * mf->zx * mf->wy -
     mf->xx * mf->zz * mf->wy -
     mf->xy * mf->zx * mf->wz +
     mf->xx * mf->zy * mf->wz) / d,
    (mf->xz * mf->yy * mf->wx -
     mf->xy * mf->yz * mf->wx -
     mf->xz * mf->yx * mf->wy +
     mf->xx * mf->yz * mf->wy +
     mf->xy * mf->yx * mf->wz -
     mf->xx * mf->yy * mf->wz) / d,
    (mf->xy * mf->yz * mf->zx -
     mf->xz * mf->yy * mf->zx +
     mf->xz * mf->yx * mf->zy -
     mf->xx * mf->yz * mf->zy -
     mf->xy * mf->yx * mf->zz +
     mf->xx * mf->yy * mf->zz) / d
  );
}
