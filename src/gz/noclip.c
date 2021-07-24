#include <stdint.h>
#include <math.h>
#include "gz.h"

static void *saved_player_update_func;

/* Link will use this as his update function while noclip is active */
static void noclip_update(z64_link_t *player, z64_game_t *global_ctx)
{
    uint16_t input_held = input_pad();
    float speed = (input_held & BUTTON_R) ? 100.f : 20.f;

    if (input_held & BUTTON_B)
      player->common.pos_2.y += speed;
    else if (input_held & BUTTON_A)
      player->common.pos_2.y -= speed;

    if (input_held & (BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT |
                      BUTTON_D_RIGHT))
    {
      int cam_idx = global_ctx->active_camera;
      z64_camera_t *active_cam = global_ctx->camera_ptrs[cam_idx];
      int16_t move_angle = active_cam->input_dir.y;

      if (input_held & BUTTON_D_DOWN)
        move_angle += 0x8000;
      else if (input_held & BUTTON_D_LEFT)
        move_angle += 0x4000;
      else if (input_held & BUTTON_D_RIGHT)
        move_angle -= 0x4000;

      player->common.pos_2.x += speed * z64_Math_SinS(move_angle);
      player->common.pos_2.z += speed * z64_Math_CosS(move_angle);
    }

    player->common.xz_speed = 0.f;
    player->linear_vel = 0.f;
    player->common.gravity = 0.f;
    player->common.vel_1.x = 0.f;
    player->common.vel_1.y = 0.f;
    player->common.vel_1.z = 0.f;
    player->common.pos_1 = player->common.pos_2;
}

void gz_noclip_start(void)
{
  saved_player_update_func = z64_link.common.main_proc;
  z64_link.common.main_proc = noclip_update;
  /* CAM_MODE_BOWARROWZ, first person */
  z64_Camera_ChangeMode(z64_game.camera_ptrs[0], 8);
  input_reserve(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
                BUTTON_A | BUTTON_B | BUTTON_R);
  input_bind_set_override(COMMAND_NOCLIP, 1);
  gz.noclip_on = 1;
}

void gz_noclip_stop(void)
{
  z64_link.common.main_proc = saved_player_update_func;
  saved_player_update_func = NULL;
  input_free(BUTTON_D_UP | BUTTON_D_DOWN | BUTTON_D_LEFT | BUTTON_D_RIGHT |
             BUTTON_A | BUTTON_B | BUTTON_R);
  input_bind_set_override(COMMAND_NOCLIP, 0);
  gz.noclip_on = 0;
}
