#ifndef Z64_H
#define Z64_H
#include <stdint.h>
#include <n64.h>

#define Z64_OOT10     0x00
#define Z64_OOT11     0x01
#define Z64_OOT12     0x02
#define Z64_OOTDEBUG  0x03

#ifndef Z64_VERSION
#define Z64_VERSION   Z64_OOT10
#endif

typedef struct
{
  Gfx      *w_poly_opa_disp;
  Gfx      *w_poly_xlu_disp;
  uint32_t  unk_00_[2];
  Gfx      *w_overlay_disp;
  uint32_t  unk_01_[41];
  Gfx      *c_work_disp;
  size_t    c_work_disp_size;
  uint32_t  unk_02_[60];
  Gfx      *w_work_disp;
  size_t    work_disp_size;
  Gfx      *work_disp;
  Gfx      *work_disp_app;
  Gfx      *work_disp_end;
  uint32_t  unk_03_[57];
  size_t    overlay_disp_size;
  Gfx      *overlay_disp;
  Gfx      *overlay_disp_app;
  Gfx      *overlay_disp_end;
  size_t    poly_opa_disp_size;
  Gfx      *poly_opa_disp;
  Gfx      *poly_opa_disp_app;
  Gfx      *poly_opa_disp_end;
  size_t    poly_xlu_disp_size;
  Gfx      *poly_xlu_disp;
  Gfx      *poly_xlu_disp_app;
  Gfx      *poly_xlu_disp_end;
  size_t    frame_count_1;
  void     *frame_buffer;
  uint32_t  unk_04_[2];
  size_t    frame_count_2;
} z64_gctxt_t;

typedef union
{
  struct
  {
    unsigned a  : 1;
    unsigned b  : 1;
    unsigned z  : 1;
    unsigned s  : 1;
    unsigned du : 1;
    unsigned dd : 1;
    unsigned dl : 1;
    unsigned dr : 1;
    unsigned    : 2;
    unsigned l  : 1;
    unsigned r  : 1;
    unsigned cu : 1;
    unsigned cd : 1;
    unsigned cl : 1;
    unsigned cr : 1;
    signed   x  : 8;
    signed   y  : 8;
  };
  uint16_t pad;
  uint32_t data;
} z64_controller_t;

typedef struct
{
  z64_gctxt_t *gctxt;
} z64_ctxt_t;

typedef union
{
  struct
  {
    float x;
    float y;
    float z;
  };
  struct
  {
    uint32_t rx;
    uint32_t ry;
    uint32_t rz;
  };
} z64_xyz_t;

typedef struct
{
  uint16_t x;
  uint16_t y;
  uint16_t z;
} z64_rot_t;

typedef struct
{
  uint32_t  vrom_start;
  void     *ram_dest;
  uint32_t  size;
} z64_getfile_t;

enum
{
  BUTTON_C_RIGHT = 0x0001,
  BUTTON_C_LEFT  = 0x0002,
  BUTTON_C_DOWN  = 0x0004,
  BUTTON_C_UP    = 0x0008,
  BUTTON_R       = 0x0010,
  BUTTON_L       = 0x0020,
  BUTTON_D_RIGHT = 0x0100,
  BUTTON_D_LEFT  = 0x0200,
  BUTTON_D_DOWN  = 0x0400,
  BUTTON_D_UP    = 0x0800,
  BUTTON_START   = 0x1000,
  BUTTON_Z       = 0x2000,
  BUTTON_B       = 0x4000,
  BUTTON_A       = 0x8000,
};

enum
{
  BUTTON_INDEX_C_RIGHT = 0,
  BUTTON_INDEX_C_LEFT  = 1,
  BUTTON_INDEX_C_DOWN  = 2,
  BUTTON_INDEX_C_UP    = 3,
  BUTTON_INDEX_R       = 4,
  BUTTON_INDEX_L       = 5,
  BUTTON_INDEX_D_RIGHT = 8,
  BUTTON_INDEX_D_LEFT  = 9,
  BUTTON_INDEX_D_DOWN  = 10,
  BUTTON_INDEX_D_UP    = 11,
  BUTTON_INDEX_START   = 12,
  BUTTON_INDEX_Z       = 13,
  BUTTON_INDEX_B       = 14,
  BUTTON_INDEX_A       = 15,
};

#if Z64_VERSION == Z64_OOT10
#define z64_ctxt          (*(z64_ctxt_t*)       0x801C84A0)
#define z64_controller_1  (*(z64_controller_t*) 0x801C84B4)
#define z64_link_pos      (*(z64_xyz_t*)        0x801DAA54)
#define z64_link_rot      (*(z64_rot_t*)        0x801DAAE4)

/* dram addresses */
#define z64_GetFile_addr                        0x80000B0C
#define z64_minimap_disable_1_addr              0x8006CD50
#define z64_minimap_disable_2_addr              0x8006D4E4
#define z64_frame_update_func_addr              0x8009AF1C
#define z64_frame_update_call_addr              0x8009CAE8
#define z64_frame_input_call_func_addr          0x800A0BA0
#define z64_frame_input_call_call_addr          0x800A16AC
#define z64_curr_day_time_addr                  0x8011A5DC
#define z64_max_hearts_addr                     0x8011A5FE
#define z64_hearts_addr                         0x8011A600
#define z64_flag_update_magic_addr              0x8011A602
#define z64_magic_addr                          0x8011A603
#define z64_rupees_addr                         0x8011A604
#define z64_has_magic_addr                      0x8011A60A
#define z64_magic_capacity_addr                 0x8011A60C
#define z64_equipment_3_addr                    0x8011A60E
#define z64_b_btn_itm_id_addr                   0x8011A638
#define z64_cl_btn_itm_id_addr                  0x8011A639
#define z64_cd_btn_itm_id_addr                  0x8011A63A
#define z64_cr_btn_itm_id_addr                  0x8011A63B
#define z64_equipment_ptr_addr                  0x8011A640
#define z64_deku_stick_addr                     0x8011A644
#define z64_deku_nut_addr                       0x8011A645
#define z64_bomb_addr                           0x8011A646
#define z64_bow_addr                            0x8011A647
#define z64_arrow_fire_addr                     0x8011A648
#define z64_dins_fire_addr                      0x8011A649
#define z64_slingshot_addr                      0x8011A64A
#define z64_ocarina_addr                        0x8011A64B
#define z64_bombchu_addr                        0x8011A64C
#define z64_hookshot_addr                       0x8011A64D
#define z64_arrow_ice_addr                      0x8011A64E
#define z64_farores_wind_addr                   0x8011A64F
#define z64_boomerang_addr                      0x8011A650
#define z64_lens_of_truth_addr                  0x8011A651
#define z64_magic_bean_addr                     0x8011A652
#define z64_megaton_hammer_addr                 0x8011A653
#define z64_arrow_light_addr                    0x8011A654
#define z64_nayrus_love_addr                    0x8011A655
#define z64_bottle_1_opt_addr                   0x8011A656
#define z64_bottle_2_opt_addr                   0x8011A657
#define z64_bottle_3_opt_addr                   0x8011A658
#define z64_bottle_4_opt_addr                   0x8011A659
#define z64_trade_itm_adult_addr                0x8011A65A
#define z64_trade_itm_child_addr                0x8011A65B
#define z64_qty_deku_stick_addr                 0x8011A65C
#define z64_qty_deku_nut_addr                   0x8011A65D
#define z64_qty_bomb_addr                       0x8011A65E
#define z64_qty_arrow_addr                      0x8011A65F
#define z64_qty_deku_seed_addr                  0x8011A662
#define z64_qty_bombchu_addr                    0x8011A664
#define z64_qty_magic_bean_addr                 0x8011A66A
#define z64_equipment_1_addr                    0x8011A66C
#define z64_equipment_2_addr                    0x8011A66D
#define z64_upgrades_addr                       0x8011A670
#define z64_quest_itm_1_addr                    0x8011A674
#define z64_quest_itm_2_addr                    0x8011A675
#define z64_quest_itm_3_addr                    0x8011A676
#define z64_quest_itm_4_addr                    0x8011A677
#define z64_dungeon_itm_addr                    0x8011A678
#define z64_dungeon_key_addr                    0x8011A68C
#define z64_defense_hearts_addr                 0x8011A69F
#define z64_gs_tokens_addr                      0x8011A6A1
#define z64_reset_gs_addr                       0x8011B46C
#define z64_flag_swordless_addr                 0x8011B503
#define z64_flag_interface_addr                 0x8011B92F
#define z64_flag_nayrus_addr                    0x8011B998
#define z64_countdown_tmr_addr                  0x8011B9A2
#define z64_opt_lang_addr                       0x8011B9D9
#define z64_control_input_ptr_addr              0x8011D730
#define z64_scene_num_addr                      0x801C8544
#define z64_flag_switch_addr                    0x801CA1C8
#define z64_cutscene_ptr_addr                   0x801CA208
#define z64_scene_age_addr                      0x801DA288
#define z64_flag_reload_scn_addr                0x801DA2B4
#define z64_scene_entrance_addr                 0x801DA2BA
#define z64_actor_pos_addr                      0x801DAA38
#define z64_levitate_addr                       0x801DAA90
#define z64_collision_detect_addr               0x801DAADE
#define z64_actor_rot_addr                      0x801DB25E

/* rom addresses */
#define z64_icon_item_static_vaddr              0x007BD000
#define z64_icon_item_static_vsize              0x000888A0
#define z64_nes_font_static_vaddr               0x00928000
#define z64_nes_font_static_vsize               0x00004580
#endif

#if Z64_VERSION == Z64_OOT12
#define z64_ctxt          (*(z64_ctxt_t*)       0x801C8D60)
#define z64_controller_1  (*(z64_controller_t*) 0x801C8D74)
#define z64_link_pos      (*(z64_xyz_t*)        0x801DB2F8)
#define z64_link_rot      (*(z64_rot_t*)        0x801DB3A4)

/* dram ddresses */
#define z64_GetFile_addr                        0x80000C9C
#define z64_minimap_disable_1_addr              0x8006D3B0
#define z64_minimap_disable_2_addr              0x8006DB40
#define z64_frame_update_func_addr              0x8009B60C
#define z64_frame_update_call_addr              0x8009D1D8
#define z64_frame_input_call_func_addr          0x800A1290
#define z64_frame_input_call_call_addr          0x800A1D8C
#define z64_curr_day_time_addr                  0x8011AC8C
#define z64_max_hearts_addr                     0x8011ACAE
#define z64_hearts_addr                         0x8011ACB0
#define z64_flag_update_magic_addr              0x8011ACB2
#define z64_magic_addr                          0x8011ACB3
#define z64_rupees_addr                         0x8011ACB4
#define z64_has_magic_addr                      0x8011ACBA
#define z64_magic_capacity_addr                 0x8011ACBC
#define z64_equipment_3_addr                    0x8011ACBE
#define z64_b_btn_itm_id_addr                   0x8011ACE8
#define z64_cl_btn_itm_id_addr                  0x8011ACE9
#define z64_cd_btn_itm_id_addr                  0x8011ACEA
#define z64_cr_btn_itm_id_addr                  0x8011ACEB
#define z64_equipment_ptr_addr                  0x8011ACF0
#define z64_deku_stick_addr                     0x8011ACF4
#define z64_deku_nut_addr                       0x8011ACF5
#define z64_bomb_addr                           0x8011ACF6
#define z64_bow_addr                            0x8011ACF7
#define z64_arrow_fire_addr                     0x8011ACF8
#define z64_dins_fire_addr                      0x8011ACF9
#define z64_slingshot_addr                      0x8011ACFA
#define z64_ocarina_addr                        0x8011ACFB
#define z64_bombchu_addr                        0x8011ACFC
#define z64_hookshot_addr                       0x8011ACFD
#define z64_arrow_ice_addr                      0x8011ACFE
#define z64_farores_wind_addr                   0x8011ACFF
#define z64_boomerang_addr                      0x8011AD00
#define z64_lens_of_truth_addr                  0x8011AD01
#define z64_magic_bean_addr                     0x8011AD02
#define z64_megaton_hammer_addr                 0x8011AD03
#define z64_arrow_light_addr                    0x8011AD04
#define z64_nayrus_love_addr                    0x8011AD05
#define z64_bottle_1_opt_addr                   0x8011AD06
#define z64_bottle_2_opt_addr                   0x8011AD07
#define z64_bottle_3_opt_addr                   0x8011AD08
#define z64_bottle_4_opt_addr                   0x8011AD09
#define z64_trade_itm_adult_addr                0x8011AD0A
#define z64_trade_itm_child_addr                0x8011AD0B
#define z64_qty_deku_stick_addr                 0x8011AD0C
#define z64_qty_deku_nut_addr                   0x8011AD0D
#define z64_qty_bomb_addr                       0x8011AD0E
#define z64_qty_arrow_addr                      0x8011AD0F
#define z64_qty_deku_seed_addr                  0x8011AD12
#define z64_qty_bombchu_addr                    0x8011AD14
#define z64_qty_magic_bean_addr                 0x8011AD1A
#define z64_equipment_1_addr                    0x8011AD1C
#define z64_equipment_2_addr                    0x8011AD1D
#define z64_upgrades_addr                       0x8011AD20
#define z64_quest_itm_1_addr                    0x8011AD24
#define z64_quest_itm_2_addr                    0x8011AD25
#define z64_quest_itm_3_addr                    0x8011AD26
#define z64_quest_itm_4_addr                    0x8011AD27
#define z64_dungeon_itm_addr                    0x8011AD28
#define z64_dungeon_key_addr                    0x8011AD3C
#define z64_defense_hearts_addr                 0x8011AD4F
#define z64_gs_tokens_addr                      0x8011AD51
#define z64_reset_gs_addr                       0x8011BB2F
#define z64_flag_swordless_addr                 0x8011BBB3
#define z64_flag_interface_addr                 0x8011BFDF
#define z64_flag_nayrus_addr                    0x8011C048
#define z64_countdown_tmr_addr                  0x8011C051
#define z64_opt_lang_addr                       0x8011C089
#define z64_control_input_ptr_addr              0x8011DE00
#define z64_scene_num_addr                      0x801C8E04
#define z64_flag_switch_addr                    0x801CAA88
#define z64_cutscene_ptr_addr                   0x801CAAC8
#define z64_scene_age_addr                      0x801DAB48
#define z64_flag_reload_scn_addr                0x801DAB74
#define z64_scene_entrance_addr                 0x801DAB7A
#define z64_actor_pos_addr                      0x801DB2F8
#define z64_levitate_addr                       0x801DB350
#define z64_collision_detect_addr               0x801DB39E
#define z64_actor_rot_addr                      0x801DBB1E

/* rom addresses */
#define z64_icon_item_static_vaddr              0x007BD000
#define z64_icon_item_static_vsize              0x000888A0
#define z64_nes_font_static_vaddr               0x008ED000
#define z64_nes_font_static_vsize               0x00004580
#endif

/* function prototypes */
typedef void (*z64_GetFile_proc)(z64_getfile_t*);

/* functions */
#define z64_GetFile       ((z64_GetFile_proc)   z64_GetFile_addr)

#endif
