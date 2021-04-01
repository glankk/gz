#ifndef GEOMETRY_H
#define GEOMETRY_H
#include "z64.h"

z64_xyzf_t *vec3f_py      (z64_xyzf_t *r, float p, float y);
void        vec3f_pyangles(z64_xyzf_t *a, float *p, float *y);
z64_xyzf_t *vec3f_add     (z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b);
z64_xyzf_t *vec3f_sub     (z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b);
z64_xyzf_t *vec3f_mul     (z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b);
z64_xyzf_t *vec3f_scale   (z64_xyzf_t *r, z64_xyzf_t *a, float s);
z64_xyzf_t *vec3f_cross   (z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b);
float       vec3f_dot     (z64_xyzf_t *a, z64_xyzf_t *b);
float       vec3f_mag     (z64_xyzf_t *a);
float       vec3f_cos     (z64_xyzf_t *a, z64_xyzf_t *b);
float       vec3f_angle   (z64_xyzf_t *a, z64_xyzf_t *b);
z64_xyzf_t *vec3f_norm    (z64_xyzf_t *r, z64_xyzf_t *a);
z64_xyzf_t *vec3f_proj    (z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b);
z64_xyzf_t *vec3f_rej     (z64_xyzf_t *r, z64_xyzf_t *a, z64_xyzf_t *b);
float       angle_dif     (float a, float b);
z64_xyzf_t *vec3f_xfmw    (z64_xyzf_t *r, z64_xyzf_t *a, float w, MtxF *b);

#endif
