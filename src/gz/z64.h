#ifndef Z64_H
#define Z64_H
#include <stdint.h>
#include <n64.h>

#define Z64_OOT10             0x00
#define Z64_OOT11             0x01
#define Z64_OOT12             0x02
#define Z64_OOTDEBUG          0x03

#ifndef Z64_VERSION
#define Z64_VERSION           Z64_OOT10
#endif

#define Z64_SEG_PHYS          0x00
#define Z64_SEG_TITLE         0x01
#define Z64_SEG_SCENE         0x02
#define Z64_SEG_ROOM          0x03
#define Z64_SEG_KEEP          0x04
#define Z64_SEG_SKEEP         0x05
#define Z64_SEG_OBJ           0x06
#define Z64_SEG_ZIMG          0x0E
#define Z64_SEG_CIMG          0x0F

#define BUTTON_C_RIGHT        0x0001
#define BUTTON_C_LEFT         0x0002
#define BUTTON_C_DOWN         0x0004
#define BUTTON_C_UP           0x0008
#define BUTTON_R              0x0010
#define BUTTON_L              0x0020
#define BUTTON_D_RIGHT        0x0100
#define BUTTON_D_LEFT         0x0200
#define BUTTON_D_DOWN         0x0400
#define BUTTON_D_UP           0x0800
#define BUTTON_START          0x1000
#define BUTTON_Z              0x2000
#define BUTTON_B              0x4000
#define BUTTON_A              0x8000

#define BUTTON_INDEX_C_RIGHT  0
#define BUTTON_INDEX_C_LEFT   1
#define BUTTON_INDEX_C_DOWN   2
#define BUTTON_INDEX_C_UP     3
#define BUTTON_INDEX_R        4
#define BUTTON_INDEX_L        5
#define BUTTON_INDEX_D_RIGHT  8
#define BUTTON_INDEX_D_LEFT   9
#define BUTTON_INDEX_D_DOWN   10
#define BUTTON_INDEX_D_UP     11
#define BUTTON_INDEX_START    12
#define BUTTON_INDEX_Z        13
#define BUTTON_INDEX_B        14
#define BUTTON_INDEX_A        15

typedef struct
{
  float x;
  float y;
  float z;
} z64_xyz_t;

typedef uint16_t z64_angle_t;
typedef struct
{
  z64_angle_t x;
  z64_angle_t y;
  z64_angle_t z;
} z64_rot_t;

typedef enum
{
  Z64_ITEM_NULL = -1,
  Z64_ITEM_STICK,
  Z64_ITEM_NUT,
  Z64_ITEM_BOMB,
  Z64_ITEM_BOW,
  Z64_ITEM_FIRE_ARROW,
  Z64_ITEM_DINS_FIRE,
  Z64_ITEM_SLINGSHOT,
  Z64_ITEM_FAIRY_OCARINA,
  Z64_ITEM_OCARINA_OF_TIME,
  Z64_ITEM_BOMBCHU,
  Z64_ITEM_HOOKSHOT,
  Z64_ITEM_LONGSHOT,
  Z64_ITEM_ICE_ARROW,
  Z64_ITEM_FARORES_WIND,
  Z64_ITEM_BOOMERANG,
  Z64_ITEM_LENS,
  Z64_ITEM_BEANS,
  Z64_ITEM_HAMMER,
  Z64_ITEM_LIGHT_ARROW,
  Z64_ITEM_NAYRUS_LOVE,
  Z64_ITEM_BOTTLE,
  Z64_ITEM_RED_POTION,
  Z64_ITEM_GREEN_POTION,
  Z64_ITEM_BLUE_POTION,
  Z64_ITEM_FAIRY,
  Z64_ITEM_FISH,
  Z64_ITEM_MILK,
  Z64_ITEM_LETTER,
  Z64_ITEM_BLUE_FIRE,
  Z64_ITEM_BUG,
  Z64_ITEM_BIG_POE,
  Z64_ITEM_HALF_MILK,
  Z64_ITEM_POE,
  Z64_ITEM_WEIRD_EGG,
  Z64_ITEM_CHICKEN,
  Z64_ITEM_ZELDAS_LETTER,
  Z64_ITEM_KEATON_MASK,
  Z64_ITEM_SKULL_MASK,
  Z64_ITEM_SPOOKY_MASK,
  Z64_ITEM_BUNNY_HOOD,
  Z64_ITEM_GORON_MASK,
  Z64_ITEM_ZORA_MASK,
  Z64_ITEM_GERUDO_MASK,
  Z64_ITEM_MASK_OF_TRUTH,
  Z64_ITEM_SOLD_OUT,
  Z64_ITEM_POCKET_EGG,
  Z64_ITEM_POCKET_CUCCO,
  Z64_ITEM_COJIRO,
  Z64_ITEM_ODD_MUSHROOM,
  Z64_ITEM_ODD_POTION,
  Z64_ITEM_POACHERS_SAW,
  Z64_ITEM_BROKEN_GORONS_SWORD,
  Z64_ITEM_PRESCRIPTION,
  Z64_ITEM_EYEBALL_FROG,
  Z64_ITEM_EYE_DROPS,
  Z64_ITEM_CLAIM_CHECK,
  Z64_ITEM_BOW_FIRE_ARROW,
  Z64_ITEM_BOW_ICE_ARROW,
  Z64_ITEM_BOW_LIGHT_ARROW,
  Z64_ITEM_KOKIRI_SWORD,
  Z64_ITEM_MASTER_SWORD,
  Z64_ITEM_BIGGORON_SWORD,
  Z64_ITEM_DEKU_SHIELD,
  Z64_ITEM_HYLIAN_SHIELD,
  Z64_ITEM_MIRROR_SHIELD,
  Z64_ITEM_KOKIRI_TUNIC,
  Z64_ITEM_GORON_TUNIC,
  Z64_ITEM_ZORA_TUNIC,
  Z64_ITEM_KOKIRI_BOOTS,
  Z64_ITEM_IRON_BOOTS,
  Z64_ITEM_HOVER_BOOTS,
  Z64_ITEM_BULLET_BAG_30,
  Z64_ITEM_BULLET_BAG_40,
  Z64_ITEM_BULLET_BAG_50,
  Z64_ITEM_QUIVER_30,
  Z64_ITEM_QUIVER_40,
  Z64_ITEM_QUIVER_50,
  Z64_ITEM_BOMB_BAG_20,
  Z64_ITEM_BOMB_BAG_30,
  Z64_ITEM_BOMB_BAG_40,
  Z64_ITEM_GORONS_BRACELET,
  Z64_ITEM_SILVER_GAUNTLETS,
  Z64_ITEM_GOLDEN_GAUNTLETS,
  Z64_ITEM_SILVER_SCALE,
  Z64_ITEM_GOLDEN_SCALE,
  Z64_ITEM_BROKEN_GIANTS_KNIFE,
  Z64_ITEM_ADULTS_WALLET,
  Z64_ITEM_GIANTS_WALLET,
  Z64_ITEM_DEKU_SEEDS,
  Z64_ITEM_FISHING_POLE,
  Z64_ITEM_MINUET,
  Z64_ITEM_BOLERO,
  Z64_ITEM_SERENADE,
  Z64_ITEM_REQUIEM,
  Z64_ITEM_NOCTURNE,
  Z64_ITEM_PRELUDE,
  Z64_ITEM_ZELDAS_LULLABY,
  Z64_ITEM_EPONAS_SONG,
  Z64_ITEM_SARIAS_SONG,
  Z64_ITEM_SUNS_SONG,
  Z64_ITEM_SONG_OF_TIME,
  Z64_ITEM_SONG_OF_STORMS,
  Z64_ITEM_FOREST_MEDALLION,
  Z64_ITEM_FIRE_MEDALLION,
  Z64_ITEM_WATER_MEDALLION,
  Z64_ITEM_SPIRIT_MEDALLION,
  Z64_ITEM_SHADOW_MEDALLION,
  Z64_ITEM_LIGHT_MEDALLION,
  Z64_ITEM_KOKIRIS_EMERALD,
  Z64_ITEM_GORONS_RUBY,
  Z64_ITEM_ZORAS_SAPPHIRE,
  Z64_ITEM_STONE_OF_AGONY,
  Z64_ITEM_GERUDOS_CARD,
  Z64_ITEM_GOLD_SKULLTULA,
  Z64_ITEM_HEART_CONTAINER,
  Z64_ITEM_PIECE_OF_HEART,
  Z64_ITEM_BOSS_KEY,
  Z64_ITEM_COMPASS,
  Z64_ITEM_DUNGEON_MAP,
  Z64_ITEM_SMALL_KEY,
} z64_item_t;

typedef enum
{
  Z64_SLOT_STICK,
  Z64_SLOT_NUT,
  Z64_SLOT_BOMB,
  Z64_SLOT_BOW,
  Z64_SLOT_FIRE_ARROW,
  Z64_SLOT_DINS_FIRE,
  Z64_SLOT_SLINGSHOT,
  Z64_SLOT_OCARINA,
  Z64_SLOT_BOMBCHU,
  Z64_SLOT_HOOKSHOT,
  Z64_SLOT_ICE_ARROW,
  Z64_SLOT_FARORES_WIND,
  Z64_SLOT_BOOMERANG,
  Z64_SLOT_LENS,
  Z64_SLOT_BEANS,
  Z64_SLOT_HAMMER,
  Z64_SLOT_LIGHT_ARROW,
  Z64_SLOT_NAYRUS_LOVE,
  Z64_SLOT_BOTTLE_1,
  Z64_SLOT_BOTTLE_2,
  Z64_SLOT_BOTTLE_3,
  Z64_SLOT_BOTTLE_4,
  Z64_SLOT_ADULT_TRADE,
  Z64_SLOT_CHILD_TRADE,
} z64_slot_t;

typedef struct
{
  int32_t       entrance_index;
  int32_t       link_age;
  char          unk_00_[0x0002];
  int16_t       cutscene_index;
  int16_t       day_time;
  char          unk_01_[0x0002];
  int32_t       night_flag;
  char          unk_02_[0x0008];
  char          id[6];
  int16_t       deaths;
  char          file_name[0x08];
  int16_t       n64dd_flag;
  int16_t       energy_capacity;
  int16_t       energy;
  uint8_t       magic_capacity_set;
  uint8_t       magic;
  uint16_t      rupees;
  uint16_t      bgs_hits_left;
  uint16_t      navi_timer;
  uint8_t       magic_acquired;
  char          unk_03_;
  uint8_t       magic_capacity;
  char          unk_04_;
  int8_t        bgs_flag;
  char          unk_05_[0x0027];
  int16_t       scene_index;
  int8_t        button_items[4];
  int8_t        c_button_slots[3];
  char          unk_06_;
  union
  {
    uint16_t    equips;
    struct
    {
      uint16_t  equip_boots         : 4;
      uint16_t  equip_tunic         : 4;
      uint16_t  equip_shield        : 4;
      uint16_t  equip_sword         : 4;
    };
  };
  char          unk_07_[0x0002];
  int8_t        items[24];
  int8_t        ammo[15];
  uint8_t       magic_beans_sold;
  union
  {
    uint16_t    equipment;
    struct
    {
      uint16_t                      : 1;
      uint16_t  hover_boots         : 1;
      uint16_t  iron_boots          : 1;
      uint16_t  kokiri_boots        : 1;
      uint16_t                      : 1;
      uint16_t  zora_tunic          : 1;
      uint16_t  goron_tunic         : 1;
      uint16_t  kokiri_tunic        : 1;
      uint16_t                      : 1;
      uint16_t  mirror_shield       : 1;
      uint16_t  hylian_shield       : 1;
      uint16_t  deku_shield         : 1;
      uint16_t  broken_giants_knife : 1;
      uint16_t  giants_knife        : 1;
      uint16_t  master_sword        : 1;
      uint16_t  kokiri_sword        : 1;
    };
  };
  union
  {
    uint32_t    equipment_items;
    struct
    {
      uint32_t                      : 9;
      uint32_t  nut_upgrade         : 3;
      uint32_t  stick_upgrade       : 3;
      uint32_t  bullet_bag          : 3;
      uint32_t  wallet              : 2;
      uint32_t  diving_upgrade      : 3;
      uint32_t  strength_upgrade    : 3;
      uint32_t  bomb_bag            : 3;
      uint32_t  quiver              : 3;
    };
  };
  union
  {
    uint32_t    quest_items;
    struct
    {
      uint32_t  heart_pieces        : 8;
      uint32_t  gold_skulltula      : 1;
      uint32_t  gerudos_card        : 1;
      uint32_t  stone_of_agony      : 1;
      uint32_t  zoras_sapphire      : 1;
      uint32_t  gorons_ruby         : 1;
      uint32_t  kokiris_emerald     : 1;
      uint32_t  song_of_storms      : 1;
      uint32_t  song_of_time        : 1;
      uint32_t  suns_song           : 1;
      uint32_t  sarias_song         : 1;
      uint32_t  eponas_song         : 1;
      uint32_t  zeldas_lullaby      : 1;
      uint32_t  prelude_of_light    : 1;
      uint32_t  nocturne_of_shadow  : 1;
      uint32_t  requiem_of_spirit   : 1;
      uint32_t  serenade_of_water   : 1;
      uint32_t  bolero_of_fire      : 1;
      uint32_t  minuet_of_forest    : 1;
      uint32_t  light_medallion     : 1;
      uint32_t  shadow_medallion    : 1;
      uint32_t  spirit_medallion    : 1;
      uint32_t  water_medallion     : 1;
      uint32_t  fire_medallion      : 1;
      uint32_t  forest_medallion    : 1;
    };
  };
  union
  {
    uint8_t     items;
    struct
    {
      uint8_t                       : 5;
      uint8_t   map                 : 1;
      uint8_t   compass             : 1;
      uint8_t   boss_key            : 1;
    };
  }             dungeon_items[20];
  int8_t        dungeon_keys[19];
  uint8_t       defense_hearts;
  int16_t       gs_tokens;
  char          unk_08_[0x0002];
  struct
  {
    uint32_t    chests;
    uint32_t    switches;
    uint32_t    rooms_cleared;
    uint32_t    collectibles;
    uint32_t    unk_00_;
    uint32_t    unk_01_;
    uint32_t    rooms_visited;
  }             scene_flags[101];
  char          unk_09_[0x0284];
  z64_xyz_t     fw_pos;
  z64_angle_t   fw_yaw;
  char          unk_0A_[0x0008];
  uint16_t      fw_scene_index;
  uint32_t      fw_room_index;
  int32_t       fw_set;
  char          unk_0B_[0x0018];
  uint8_t       gs_flags[56];
  uint16_t      event_chk_inf[14];
  uint16_t      item_get_inf[4];
  uint16_t      inf_table[30];
  char          unk_0C_[0x041E];
  uint16_t      checksum;
  int32_t       file_index;
  char          unk_0D_[0x0007];
  uint8_t       interface_flags;
  uint32_t      scene_setup_index;
  char          unk_0E_[0x0064];
  uint16_t      nayrus_love_timer;
  char          unk_0F_[0x0008];
  uint16_t      timer;
  char          unk_10_[0x0035];
  uint8_t       language;
  char          unk_11_[0x0046];
} z64_file_t;

typedef struct
{
  uint32_t seg[16];
} z64_stab_t;

typedef struct
{
  uint32_t scene_vrom_start;
  uint32_t scene_vrom_end;
  uint32_t title_vrom_start;
  uint32_t title_vrom_end;
  char     unk_00_;
  uint8_t  scene_config;
  char     unk_01_;
  char     padding_00_;
} z64_scene_table_t;

typedef struct
{
  Gfx      *poly_opa_disp_w;
  Gfx      *poly_xlu_disp_w;
  char      unk_00_[0x0008];
  Gfx      *overlay_disp_w;
  char      unk_01_[0x00A4];
  Gfx      *work_disp_c;
  size_t    work_disp_c_size;
  char      unk_02_[0x00F0];
  Gfx      *work_disp_w;
  size_t    work_disp_size;
  Gfx      *work_disp;
  Gfx      *work_disp_p;
  Gfx      *work_disp_e;
  char      unk_03_[0x00E4];
  size_t    overlay_disp_size;
  Gfx      *overlay_disp;
  Gfx      *overlay_disp_p;
  Gfx      *overlay_disp_e;
  size_t    poly_opa_disp_size;
  Gfx      *poly_opa_disp;
  Gfx      *poly_opa_disp_p;
  Gfx      *poly_opa_disp_e;
  size_t    poly_xlu_disp_size;
  Gfx      *poly_xlu_disp;
  Gfx      *poly_xlu_disp_p;
  Gfx      *poly_xlu_disp_e;
  size_t    frame_count_1;
  void     *frame_buffer;
  char      unk_04_[0x0008];
  size_t    frame_count_2;
} z64_gfx_t;

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

typedef struct z64_actor_s z64_actor_t;
struct z64_actor_s
{
  uint16_t      actor_index;
  uint8_t       actor_type;
  int8_t        room_index;
  char          unk_00_[0x0004];
  z64_xyz_t     pos_1;
  z64_rot_t     rot_init;
  char          unk_01_[0x0002];
  uint16_t      actor_variable;
  uint8_t       alloc_index;
  char          unk_02_;
  uint16_t      sound_effect;
  char          unk_03_[0x0002];
  z64_xyz_t     pos_2;
  char          unk_04_[0x0002];
  uint16_t      xz_dir;
  char          unk_05_[0x0004];
  z64_xyz_t     pos_3;
  z64_rot_t     rot_1;
  char          unk_06_[0x0002];
  float         unk_07_;
  z64_xyz_t     scale;
  z64_xyz_t     vel_1;
  float         xz_speed;
  float         gravity;
  float         min_vel_y;
  void         *unk_08_;
  void         *unk_09_;
  char          unk_0A_[0x001C];
  void         *damage_table;
  z64_xyz_t     vel_2;
  char          unk_0B_[0x0006];
  int16_t       health;
  char          unk_0C_;
  uint8_t       damage_effect;
  char          unk_0D_[0x0002];
  z64_rot_t     rot_2;
  char          unk_0E_[0x0046];
  z64_xyz_t     pos_4;
  uint16_t      unk_0F_;
  uint16_t      text_id;
  int16_t       actor_frozen;
  char          unk_10_[0x0003];
  uint8_t       actor_active;
  char          unk_11_[0x0002];
  z64_actor_t  *unk_12_;
  char          unk_13_[0x0004];
  z64_actor_t  *actor_prev;
  z64_actor_t  *actor_next;
  void         *unk_14_;
  void         *actor_init_proc;
  void         *actor_main_proc;
  void         *actor_draw_proc;
  void         *code_entry;
};

typedef struct
{
  z64_actor_t common;
  char        unk_00_[0x6F2];
  uint16_t    yaw;
} z64_link_t;

typedef struct
{
  z64_controller_t  ctrl;
  char              unk_00_[0x0014];
} z64_input_t;

typedef struct
{
  z64_gfx_t            *gfx;
  void                 *state_main_proc;
  void                 *state_dtor_proc;
  void                 *state_next_ctor_proc;
  size_t               *ctxt_size;
  z64_input_t           input[4];
  size_t                state_heap_size;
  void                 *state_heap;
  void                 *heap_start;
  void                 *heap_end;
  void                 *state_heap_node;
  char                  unk_00_[0x0010];
  int32_t               state_continue;
  int32_t               state_frames;
  uint32_t              unk_01_;
} z64_ctxt_t;

typedef struct
{
  z64_ctxt_t      common;
  uint16_t        scene_index;
  char            unk_00_[0x001A];
  uint32_t        screen_top;
  uint32_t        screen_bottom;
  uint32_t        screen_left;
  uint32_t        screen_right;
  float           camera_distance;
  float           fog_distance;
  float           z_distance;
  float           unk_01_;
  char            unk_02_[0x0190];
  z64_actor_t    *camera_focus;
  char            unk_03_[0x19B8];
  uint8_t         no_actors_loaded;
  char            unk_04_[0x0003];
  struct
  {
    uint32_t      length;
    z64_actor_t  *first;
  }               actor_list[12];
  char            unk_05_[0x0038];
  z64_actor_t    *arrow_actor;
  z64_actor_t    *target_actor;
  char            unk_06_[0x0058];
  uint32_t        switch_flags;
  uint32_t        temp_switch_flags;
  uint32_t        unk_07_;
  uint32_t        unk_08_;
  uint32_t        chest_flags;
  uint32_t        room_clear_flags;
  uint32_t        unk_09_;
  uint32_t        unk_0A_;
  uint32_t        unk_0B_;
  void           *title_card_texture;
  char            unk_0C_[0x0007];
  uint8_t         title_card_delay;
  char            unk_0D_[0x0010];
  void           *cutscene_ptr;
  char            unk_0E_[0xFF50];
  int8_t          room_index;
  char            unk_0F_[0x000B];
  void           *room_ptr;
  char            unk_10_[0x0118];
  uint32_t        gameplay_frames;
  uint8_t         link_age;
  char            unk_11_;
  uint8_t         spawn_index;
  uint8_t         no_map_actors;
  uint8_t         no_rooms;
  char            unk_12_[0x000B];
  void           *map_actor_list;
  char            unk_13_[0x0008];
  void           *scene_exit_list;
  char            unk_14_[0x000C];
  uint16_t        scene_load_flags;
  char            unk_15_[0x0004];
  uint16_t        entrance_index;
} z64_game_t;

typedef struct
{
  /* file loading params */
  uint32_t      vrom_addr;
  void         *dram_addr;
  size_t        size;
  /* unknown, seem to be unused */
  void         *unk_00_;
  uint32_t      unk_01_;
  uint32_t      unk_02_;
  /* completion notification params */
  OSMesgQueue  *notify_queue;
  OSMesg        notify_message;
} z64_getfile_t;

#if Z64_VERSION == Z64_OOT10

/* dram addresses */
#define z64_GetFile_addr                        0x80000B0C
#define z64_osSendMesg_addr                     0x80001E20
#define z64_osRecvMesg_addr                     0x80002030
#define z64_osCreateMesgQueue_addr              0x80004220
#define z64_file_mq_addr                        0x80007D40
#define z64_minimap_disable_1_addr              0x8006CD50
#define z64_minimap_disable_2_addr              0x8006D4E4
#define z64_LoadRoom_addr                       0x80080A3C
#define z64_UnloadRoom_addr                     0x80080C98
#define z64_frame_update_func_addr              0x8009AF1C
#define z64_frame_update_call_addr              0x8009CAE8
#define z64_frame_input_func_addr               0x800A0BA0
#define z64_frame_input_call_addr               0x800A16AC
#define z64_scene_table_addr                    0x800FB4E0
#define z64_scene_config_table_addr             0x800FBD18
#define z64_file_addr                           0x8011A5D0
#define z64_input_direct_addr                   0x8011D730
#define z64_stab_addr                           0x80120C38
#define z64_ctxt_addr                           0x801C84A0
#define z64_link_addr                           0x801DAA30

/* rom addresses */
#define z64_icon_item_static_vaddr              0x007BD000
#define z64_icon_item_static_vsize              0x000888A0
#define z64_nes_font_static_vaddr               0x00928000
#define z64_nes_font_static_vsize               0x00004580

#elif Z64_VERSION == Z64_OOT11

/* dram ddresses */
#define z64_GetFile_addr                        0x80000B0C
#define z64_osSendMesg_addr                     0x80001E20
#define z64_osRecvMesg_addr                     0x80002030
#define z64_osCreateMesgQueue_addr              0x80004220
#define z64_file_mq_addr                        0x80007D40
#define z64_minimap_disable_1_addr              0x8006CD50
#define z64_minimap_disable_2_addr              0x8006D4E4
#define z64_LoadRoom_addr                       0x80080A3C
#define z64_UnloadRoom_addr                     0x80080C98
#define z64_frame_update_func_addr              0x8009AF2C
#define z64_frame_update_call_addr              0x8009CAF8
#define z64_frame_input_func_addr               0x800A0BB0
#define z64_frame_input_call_addr               0x800A16BC
#define z64_scene_table_addr                    0x800FB6A0
#define z64_scene_config_table_addr             0x800FBED8
#define z64_file_addr                           0x8011A790
#define z64_input_direct_addr                   0x8011D8F0
#define z64_stab_addr                           0x80120DF8
#define z64_ctxt_addr                           0x801C8660
#define z64_link_addr                           0x801DABF0

/* rom addresses */
#define z64_icon_item_static_vaddr              0x007BD000
#define z64_icon_item_static_vsize              0x000888A0
#define z64_nes_font_static_vaddr               0x008ED000
#define z64_nes_font_static_vsize               0x00004580

#elif Z64_VERSION == Z64_OOT12

/* dram ddresses */
#define z64_GetFile_addr                        0x80000C9C
#define z64_osSendMesg_addr                     0x80001FD0
#define z64_osRecvMesg_addr                     0x800021F0
#define z64_osCreateMesgQueue_addr              0x800043E0
#define z64_file_mq_addr                        0x80008A30
#define z64_minimap_disable_1_addr              0x8006D3B0
#define z64_minimap_disable_2_addr              0x8006DB40
#define z64_LoadRoom_addr                       0x80081064
#define z64_UnloadRoom_addr                     0x800812C0
#define z64_frame_update_func_addr              0x8009B60C
#define z64_frame_update_call_addr              0x8009D1D8
#define z64_frame_input_func_addr               0x800A1290
#define z64_frame_input_call_addr               0x800A1D8C
#define z64_scene_table_addr                    0x800FBB30
#define z64_scene_config_table_addr             0x800FC368
#define z64_file_addr                           0x8011AC80
#define z64_input_direct_addr                   0x8011DE00
#define z64_stab_addr                           0x80121508
#define z64_ctxt_addr                           0x801C8D60
#define z64_link_addr                           0x801DB2F0

/* rom addresses */
#define z64_icon_item_static_vaddr              0x007BD000
#define z64_icon_item_static_vsize              0x000888A0
#define z64_nes_font_static_vaddr               0x008ED000
#define z64_nes_font_static_vsize               0x00004580

#endif

/* function prototypes */
typedef void (*z64_GetFile_proc)(z64_getfile_t *file);
typedef void (*z64_LoadRoom_proc)(z64_ctxt_t *ctxt, void *p_ctxt_room_index,
                                  uint8_t room_index);
typedef void (*z64_UnloadRoom_proc)(z64_ctxt_t *ctxt, void *p_ctxt_room_index);
typedef void (*z64_SceneConfig_proc)(z64_ctxt_t *ctxt);

/* data */
#define z64_stab                (*(z64_stab_t*)       z64_stab_addr)
#define z64_scene_table         ( (z64_scene_table_t*)z64_scene_table_addr)
#define z64_scene_config_table  ( (z64_SceneConfig_proc*)                     \
                                   z64_scene_config_table_addr)
#define z64_file                (*(z64_file_t*)       z64_file_addr)
#define z64_input_direct        (*(z64_controller_t*) z64_input_direct_addr)
#define z64_ctxt                (*(z64_ctxt_t*)       z64_ctxt_addr)
#define z64_game                (*(z64_game_t*)      &z64_ctxt)
#define z64_link                (*(z64_link_t*)       z64_link_addr)

/* functions */
#define z64_GetFile             ((z64_GetFile_proc)     z64_GetFile_addr)
#define z64_LoadRoom            ((z64_LoadRoom_proc)    z64_LoadRoom_addr)
#define z64_UnloadRoom          ((z64_UnloadRoom_proc)  z64_UnloadRoom_addr)
#define z64_osSendMesg          ((osSendMesg_t)         z64_osSendMesg_addr)
#define z64_osRecvMesg          ((osRecvMesg_t)         z64_osRecvMesg_addr)
#define z64_osCreateMesgQueue   ((osCreateMesgQueue_t)                        \
                                 z64_osCreateMesgQueue_addr)

#endif
