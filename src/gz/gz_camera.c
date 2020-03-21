#include <stdint.h>
#include <math.h>
#include <n64.h>
#include "geometry.h"
#include "gu.h"
#include "gz.h"

static const float  joy_mspeed    = 0.5f;
static const float  joy_rspeed    = 0.0025f;
static const int    joy_max       = 60.f;
static const float  pitch_lim     = M_PI / 2.f - joy_rspeed;
static const float  fol_mspeed    = 1.f / 3.f;
static const float  fol_rspeed    = 1.f / 3.f;

static void get_target_point(z64_xyzf_t *v)
{
  *v = z64_link.common.pos_2;
  if (z64_file.link_age == 0)
    v->y += 55.f;
  else
    v->y += 35.f;
}

static void cam_manual(void)
{
  if (!gz.lock_cam) {
    int x = zu_adjust_joystick(input_x());
    int y = zu_adjust_joystick(input_y());

    z64_xyzf_t vf;
    z64_xyzf_t vr;
    z64_xyzf_t move;
    vec3f_py(&vf, gz.cam_pitch, gz.cam_yaw);
    vec3f_py(&vr, 0.f, gz.cam_yaw - M_PI / 2.f);

    if (input_pad() & BUTTON_Z) {
      vec3f_scale(&move, &vf, y * joy_mspeed);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);

      vec3f_scale(&move, &vr, x * joy_mspeed);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);

      if (input_pad() & BUTTON_C_UP)
        gz.cam_pos.y += joy_max * joy_mspeed;
      if (input_pad() & BUTTON_C_DOWN)
        gz.cam_pos.y += -joy_max * joy_mspeed;
      if (input_pad() & BUTTON_C_RIGHT)
        gz.cam_yaw -= joy_max * joy_rspeed;
      if (input_pad() & BUTTON_C_LEFT)
        gz.cam_yaw -= -joy_max * joy_rspeed;
    }
    else {
      gz.cam_pitch += y * joy_rspeed;
      gz.cam_yaw -= x * joy_rspeed;

      if (input_pad() & BUTTON_C_UP) {
        vec3f_scale(&move, &vf, joy_max * joy_mspeed);
        vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
      }
      if (input_pad() & BUTTON_C_DOWN) {
        vec3f_scale(&move, &vf, -joy_max * joy_mspeed);
        vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
      }
      if (input_pad() & BUTTON_C_RIGHT) {
        vec3f_scale(&move, &vr, joy_max * joy_mspeed);
        vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
      }
      if (input_pad() & BUTTON_C_LEFT) {
        vec3f_scale(&move, &vr, -joy_max * joy_mspeed);
        vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
      }
    }

    if (gz.cam_pitch > pitch_lim)
      gz.cam_pitch = pitch_lim;
    else if (gz.cam_pitch < -pitch_lim)
      gz.cam_pitch = -pitch_lim;
  }
}

static void cam_birdseye(void)
{
  if (zu_in_game()) {
    z64_xyzf_t vt;
    get_target_point(&vt);

    z64_xyzf_t vd;
    vec3f_sub(&vd, &vt, &gz.cam_pos);

    float pitch, yaw;
    vec3f_pyangles(&vd, &pitch, &yaw);

    float d_pitch = angle_dif(pitch, gz.cam_pitch);
    if (fabsf(d_pitch) < .001f)
      gz.cam_pitch = pitch;
    else
      gz.cam_pitch += d_pitch * fol_rspeed;

    float d_yaw = angle_dif(yaw, gz.cam_yaw);
    if (fabsf(d_yaw) < .001f)
      gz.cam_yaw = yaw;
    else
      gz.cam_yaw += d_yaw * fol_rspeed;

    float dist = vec3f_mag(&vd);
    if (dist < gz.cam_dist_min) {
      z64_xyzf_t move;
      vec3f_py(&move, gz.cam_pitch, gz.cam_yaw);
      vec3f_scale(&move, &move, (dist - gz.cam_dist_min) * fol_mspeed);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
    }
    else if (dist > gz.cam_dist_max) {
      z64_xyzf_t move;
      vec3f_py(&move, gz.cam_pitch, gz.cam_yaw);
      vec3f_scale(&move, &move, (dist - gz.cam_dist_max) * fol_mspeed);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
    }
  }

  cam_manual();
}

static void cam_radial(void)
{
  z64_xyzf_t vf;
  vec3f_py(&vf, gz.cam_pitch, gz.cam_yaw);

  z64_xyzf_t vt;
  get_target_point(&vt);

  z64_xyzf_t vd;
  vec3f_sub(&vd, &vt, &gz.cam_pos);

  float dist = vec3f_dot(&vd, &vf);
  z64_xyzf_t vp;
  vec3f_scale(&vp, &vf, dist);

  if (zu_in_game()) {
    z64_xyzf_t vr;
    vec3f_sub(&vr, &vd, &vp);
    {
      z64_xyzf_t move;
      vec3f_scale(&move, &vr, fol_mspeed);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
    }

    if (dist < gz.cam_dist_min) {
      float norm = 1.f / dist;
      z64_xyzf_t move;
      vec3f_scale(&move, &vp, (dist - gz.cam_dist_min) * fol_mspeed * norm);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
    }
    else if (dist > gz.cam_dist_max) {
      float norm = 1.f / dist;
      z64_xyzf_t move;
      vec3f_scale(&move, &vp, (dist - gz.cam_dist_max) * fol_mspeed * norm);
      vec3f_add(&gz.cam_pos, &gz.cam_pos, &move);
    }
  }

  if (!gz.lock_cam) {
    int x = zu_adjust_joystick(input_x());
    int y = zu_adjust_joystick(input_y());

    if (input_pad() & BUTTON_Z) {
      dist -= y * joy_mspeed;
      if (input_pad() & BUTTON_C_UP)
        gz.cam_pitch += joy_max * joy_rspeed;
      if (input_pad() & BUTTON_C_DOWN)
        gz.cam_pitch += -joy_max * joy_rspeed;
    }
    else {
      gz.cam_pitch += y * joy_rspeed;
      if (input_pad() & BUTTON_C_UP)
        dist -= joy_max * joy_mspeed;
      if (input_pad() & BUTTON_C_DOWN)
        dist -= -joy_max * joy_mspeed;
    }
    gz.cam_yaw -= x * joy_rspeed;
    if (input_pad() & BUTTON_C_LEFT)
      gz.cam_yaw -= joy_max * joy_rspeed;
    if (input_pad() & BUTTON_C_RIGHT)
      gz.cam_yaw -= -joy_max * joy_rspeed;

    if (gz.cam_pitch > pitch_lim)
      gz.cam_pitch = pitch_lim;
    else if (gz.cam_pitch < -pitch_lim)
      gz.cam_pitch = -pitch_lim;

    z64_xyzf_t vfoc;
    vec3f_add(&vfoc, &gz.cam_pos, &vp);

    z64_xyzf_t move;
    vec3f_py(&move, gz.cam_pitch, gz.cam_yaw);
    vec3f_scale(&move, &move, -dist);
    vec3f_add(&gz.cam_pos, &vfoc, &move);
  }
}

void gz_update_cam(void)
{
  switch (gz.cam_bhv) {
    case CAMBHV_MANUAL    : cam_manual();   break;
    case CAMBHV_BIRDSEYE  : cam_birdseye(); break;
    case CAMBHV_RADIAL    : cam_radial();   break;
  }
}

void gz_free_view(void)
{
  if (!gz.free_cam || gz.cam_mode != CAMMODE_VIEW)
    return;

  gz_update_cam();

  Mtx *m_persp = NULL;
  Mtx *m_lookat = NULL;
  z64_gfx_t *gfx = z64_ctxt.gfx;
  for (Gfx *p = gfx->poly_opa.buf; p != gfx->poly_opa.p; ++p) {
    const Gfx g_persp = gsSPMatrix(0, G_MTX_PROJECTION | G_MTX_LOAD);
    const Gfx g_lookat = gsSPMatrix(0, G_MTX_PROJECTION | G_MTX_MUL);
    if (m_persp == NULL && p->hi == g_persp.hi)
      m_persp = (void *)p->lo;
    if (m_lookat == NULL && p->hi == g_lookat.hi)
      m_lookat = (void *)p->lo;
  }

  if (m_persp != NULL) {
    const float aspect = (float)Z64_SCREEN_WIDTH / (float)Z64_SCREEN_HEIGHT;
    MtxF mf_persp;
    guPerspectiveF(&mf_persp, NULL, M_PI / 3.f, aspect, 10.f, 12800.f, 1.f);
    guMtxF2L(&mf_persp, m_persp);
  }

  if (m_lookat != NULL) {
    z64_xyzf_t at;
    z64_xyzf_t up;
    vec3f_py(&at, gz.cam_pitch, gz.cam_yaw);
    vec3f_add(&at, &gz.cam_pos, &at);
    vec3f_py(&up, gz.cam_pitch - M_PI / 2.f, gz.cam_yaw);
    MtxF mf_lookat;
    guLookAtF(&mf_lookat, gz.cam_pos.x, gz.cam_pos.y, gz.cam_pos.z,
              at.x, at.y, at.z, up.x, up.y, up.z);
    guMtxF2L(&mf_lookat, m_lookat);
  }
}
