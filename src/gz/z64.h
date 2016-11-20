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

#define Z64_ETAB_LENGTH       0x0614

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
  int32_t       entrance_index;     /* 0x0000 */
  int32_t       link_age;           /* 0x0004 */
  char          unk_00_[0x0002];    /* 0x0008 */
  int16_t       cutscene_index;     /* 0x000A */
  int16_t       day_time;           /* 0x000C */
  char          unk_01_[0x0002];    /* 0x000E */
  int32_t       night_flag;         /* 0x0010 */
  char          unk_02_[0x0008];    /* 0x0014 */
  char          id[6];              /* 0x001C */
  int16_t       deaths;             /* 0x0022 */
  char          file_name[0x08];    /* 0x0024 */
  int16_t       n64dd_flag;         /* 0x002C */
  int16_t       energy_capacity;    /* 0x002E */
  int16_t       energy;             /* 0x0030 */
  uint8_t       magic_capacity_set; /* 0x0032 */
  uint8_t       magic;              /* 0x0033 */
  uint16_t      rupees;             /* 0x0034 */
  uint16_t      bgs_hits_left;      /* 0x0036 */
  uint16_t      navi_timer;         /* 0x0038 */
  uint8_t       magic_acquired;     /* 0x003A */
  char          unk_03_;            /* 0x003B */
  uint8_t       magic_capacity;     /* 0x003C */
  char          unk_04_;            /* 0x003D */
  int8_t        bgs_flag;           /* 0x003E */
  char          unk_05_[0x0027];    /* 0x003F */
  int16_t       scene_index;        /* 0x0066 */
  int8_t        button_items[4];    /* 0x0068 */
  int8_t        c_button_slots[3];  /* 0x006C */
  char          unk_06_;            /* 0x006F */
  union
  {
    uint16_t    equips;             /* 0x0070 */
    struct
    {
      uint16_t  equip_boots         : 4;
      uint16_t  equip_tunic         : 4;
      uint16_t  equip_shield        : 4;
      uint16_t  equip_sword         : 4;
    };
  };
  char          unk_07_[0x0002];    /* 0x0072 */
  int8_t        items[24];          /* 0x0074 */
  int8_t        ammo[15];           /* 0x008C */
  uint8_t       magic_beans_sold;   /* 0x009B */
  union
  {
    uint16_t    equipment;          /* 0x009C */
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
  char          unk_08_[0x0002];    /* 0x009E */
  union
  {
    uint32_t    equipment_items;    /* 0x00A0 */
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
    uint32_t    quest_items;        /* 0x00A4 */
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
  }             dungeon_items[20];  /* 0x00A8 */
  int8_t        dungeon_keys[19];   /* 0x00BC */
  uint8_t       defense_hearts;     /* 0x00CF */
  int16_t       gs_tokens;          /* 0x00D0 */
  char          unk_09_[0x0002];    /* 0x00D2 */
  struct
  {
    uint32_t    chests;
    uint32_t    switches;
    uint32_t    rooms_cleared;
    uint32_t    collectibles;
    uint32_t    unk_00_;
    uint32_t    unk_01_;
    uint32_t    rooms_visited;
  }             scene_flags[101];   /* 0x00D4 */
  char          unk_0A_[0x0284];    /* 0x0BE0 */
  z64_xyz_t     fw_pos;             /* 0x0E64 */
  z64_angle_t   fw_yaw;             /* 0x0E70 */
  char          unk_0B_[0x0008];    /* 0x0E72 */
  uint16_t      fw_scene_index;     /* 0x0E7A */
  uint32_t      fw_room_index;      /* 0x0E7C */
  int32_t       fw_set;             /* 0x0E80 */
  char          unk_0C_[0x0018];    /* 0x0E84 */
  uint8_t       gs_flags[56];       /* 0x0E9C */
  uint16_t      event_chk_inf[14];  /* 0x0ED4 */
  uint16_t      item_get_inf[4];    /* 0x0EF0 */
  uint16_t      inf_table[30];      /* 0x0EF8 */
  char          unk_0D_[0x041E];    /* 0x0F34 */
  uint16_t      checksum;           /* 0x1352 */
  int32_t       file_index;         /* 0x1354 */
  char          unk_0E_[0x0007];    /* 0x1358 */
  uint8_t       interface_flags;    /* 0x135F */
  uint32_t      scene_setup_index;  /* 0x1360 */
  char          unk_0F_[0x0064];    /* 0x1364 */
  uint16_t      nayrus_love_timer;  /* 0x13C8 */
  char          unk_10_[0x0008];    /* 0x13CA */
  uint16_t      timer;              /* 0x13D2 */
  char          unk_11_[0x0035];    /* 0x13D4 */
  uint8_t       language;           /* 0x1409 */
  char          unk_12_[0x0046];    /* 0x140A */
                                    /* 0x1450 */
} z64_file_t;

typedef struct
{
  uint32_t seg[16];
} z64_stab_t;

typedef struct
{
  uint8_t       scene_index;
  uint8_t       entrance_index;
  union
  {
    uint16_t    variable;
    struct
    {
      uint16_t  transition_out  : 7;
      uint16_t  transition_in   : 7;
      uint16_t  unk_00_         : 1;
      uint16_t  continue_music  : 1;
    };
  };
} z64_entrance_table_t;

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
  Gfx      *poly_opa_disp_w;      /* 0x0000 */
  Gfx      *poly_xlu_disp_w;      /* 0x0004 */
  char      unk_00_[0x0008];      /* 0x0008 */
  Gfx      *overlay_disp_w;       /* 0x0010 */
  char      unk_01_[0x00A4];      /* 0x0014 */
  Gfx      *work_disp_c;          /* 0x00B8 */
  size_t    work_disp_c_size;     /* 0x00BC */
  char      unk_02_[0x00F0];      /* 0x00C0 */
  Gfx      *work_disp_w;          /* 0x01B0 */
  size_t    work_disp_size;       /* 0x01B4 */
  Gfx      *work_disp;            /* 0x01B8 */
  Gfx      *work_disp_p;          /* 0x01BC */
  Gfx      *work_disp_e;          /* 0x01C0 */
  char      unk_03_[0x00E4];      /* 0x01C4 */
  size_t    overlay_disp_size;    /* 0x02A8 */
  Gfx      *overlay_disp;         /* 0x02AC */
  Gfx      *overlay_disp_p;       /* 0x02B0 */
  Gfx      *overlay_disp_e;       /* 0x02B4 */
  size_t    poly_opa_disp_size;   /* 0x02B8 */
  Gfx      *poly_opa_disp;        /* 0x02BC */
  Gfx      *poly_opa_disp_p;      /* 0x02C0 */
  Gfx      *poly_opa_disp_e;      /* 0x02C4 */
  size_t    poly_xlu_disp_size;   /* 0x02C8 */
  Gfx      *poly_xlu_disp;        /* 0x02CC */
  Gfx      *poly_xlu_disp_p;      /* 0x02D0 */
  Gfx      *poly_xlu_disp_e;      /* 0x02D4 */
  size_t    frame_count_1;        /* 0x02D8 */
  void     *frame_buffer;         /* 0x02DC */
  char      unk_04_[0x0008];      /* 0x02E0 */
  size_t    frame_count_2;        /* 0x02E8 */
                                  /* 0x02EC */
} z64_gfx_t;

typedef union
{
  struct
  {
    uint32_t a  : 1;
    uint32_t b  : 1;
    uint32_t z  : 1;
    uint32_t s  : 1;
    uint32_t du : 1;
    uint32_t dd : 1;
    uint32_t dl : 1;
    uint32_t dr : 1;
    uint32_t    : 2;
    uint32_t l  : 1;
    uint32_t r  : 1;
    uint32_t cu : 1;
    uint32_t cd : 1;
    uint32_t cl : 1;
    uint32_t cr : 1;
    int32_t  x  : 8;
    int32_t  y  : 8;
  };
  uint16_t pad;
  uint32_t data;
} z64_controller_t;

typedef struct z64_actor_s z64_actor_t;
struct z64_actor_s
{
  uint16_t      actor_index;      /* 0x0000 */
  uint8_t       actor_type;       /* 0x0002 */
  int8_t        room_index;       /* 0x0003 */
  char          unk_00_[0x0004];  /* 0x0004 */
  z64_xyz_t     pos_1;            /* 0x0008 */
  z64_rot_t     rot_init;         /* 0x0014 */
  char          unk_01_[0x0002];  /* 0x001A */
  uint16_t      actor_variable;   /* 0x001C */
  uint8_t       alloc_index;      /* 0x001E */
  char          unk_02_;          /* 0x001F */
  uint16_t      sound_effect;     /* 0x0020 */
  char          unk_03_[0x0002];  /* 0x0022 */
  z64_xyz_t     pos_2;            /* 0x0024 */
  char          unk_04_[0x0002];  /* 0x0030 */
  uint16_t      xz_dir;           /* 0x0032 */
  char          unk_05_[0x0004];  /* 0x0034 */
  z64_xyz_t     pos_3;            /* 0x0038 */
  z64_rot_t     rot_1;            /* 0x0044 */
  char          unk_06_[0x0002];  /* 0x004A */
  float         unk_07_;          /* 0x004C */
  z64_xyz_t     scale;            /* 0x0050 */
  z64_xyz_t     vel_1;            /* 0x005C */
  float         xz_speed;         /* 0x0068 */
  float         gravity;          /* 0x006C */
  float         min_vel_y;        /* 0x0070 */
  void         *unk_08_;          /* 0x0074 */
  void         *unk_09_;          /* 0x0078 */
  char          unk_0A_[0x001C];  /* 0x007C */
  void         *damage_table;     /* 0x0098 */
  z64_xyz_t     vel_2;            /* 0x009C */
  char          unk_0B_[0x0006];  /* 0x00A8 */
  int16_t       health;           /* 0x00AE */
  char          unk_0C_;          /* 0x00B0 */
  uint8_t       damage_effect;    /* 0x00B1 */
  char          unk_0D_[0x0002];  /* 0x00B2 */
  z64_rot_t     rot_2;            /* 0x00B4 */
  char          unk_0E_[0x0046];  /* 0x00BA */
  z64_xyz_t     pos_4;            /* 0x0100 */
  uint16_t      unk_0F_;          /* 0x010C */
  uint16_t      text_id;          /* 0x010E */
  int16_t       actor_frozen;     /* 0x0110 */
  char          unk_10_[0x0003];  /* 0x0112 */
  uint8_t       actor_active;     /* 0x0115 */
  char          unk_11_[0x0002];  /* 0x0116 */
  z64_actor_t  *unk_12_;          /* 0x0118 */
  char          unk_13_[0x0004];  /* 0x011C */
  z64_actor_t  *actor_prev;       /* 0x0120 */
  z64_actor_t  *actor_next;       /* 0x0124 */
  void         *unk_14_;          /* 0x0128 */
  void         *actor_init_proc;  /* 0x012C */
  void         *actor_main_proc;  /* 0x0130 */
  void         *actor_draw_proc;  /* 0x0134 */
  void         *code_entry;       /* 0x0138 */
                                  /* 0x013C */
};

typedef struct
{
  z64_actor_t common;             /* 0x0000 */
  char        unk_00_[0x6F2];     /* 0x013C */
  uint16_t    yaw;                /* 0x082E */
                                  /* 0x0830 */
} z64_link_t;

typedef struct
{
  z64_controller_t  ctrl;
  char              unk_00_[0x0014];
} z64_input_t;

typedef struct
{
  z64_gfx_t      *gfx;                    /* 0x0000 */
  void           *state_main_proc;        /* 0x0004 */
  void           *state_dtor_proc;        /* 0x0008 */
  void           *state_next_ctor_proc;   /* 0x000C */
  uint32_t        ctxt_size;              /* 0x0010 */
  z64_input_t     input[4];               /* 0x0014 */
  size_t          state_heap_size;        /* 0x0074 */
  void           *state_heap;             /* 0x0078 */
  void           *heap_start;             /* 0x007C */
  void           *heap_end;               /* 0x0080 */
  void           *state_heap_node;        /* 0x0084 */
  char            unk_00_[0x0010];        /* 0x0088 */
  int32_t         state_continue;         /* 0x0098 */
  int32_t         state_frames;           /* 0x009C */
  uint32_t        unk_01_;                /* 0x00A0 */
                                          /* 0x00A4 */
} z64_ctxt_t;

typedef struct
{
  z64_ctxt_t      common;                 /* 0x00000 */
  uint16_t        scene_index;            /* 0x000A4 */
  char            unk_00_[0x001A];        /* 0x000A6 */
  uint32_t        screen_top;             /* 0x000C0 */
  uint32_t        screen_bottom;          /* 0x000C4 */
  uint32_t        screen_left;            /* 0x000C8 */
  uint32_t        screen_right;           /* 0x000CC */
  float           camera_distance;        /* 0x000D0 */
  float           fog_distance;           /* 0x000D4 */
  float           z_distance;             /* 0x000D8 */
  float           unk_01_;                /* 0x000DC */
  char            unk_02_[0x0190];        /* 0x000E0 */
  z64_actor_t    *camera_focus;           /* 0x00270 */
  char            unk_03_[0x19B8];        /* 0x00274 */
  uint8_t         no_actors_loaded;       /* 0x01C2C */
  char            unk_04_[0x0003];        /* 0x01C2D */
  struct
  {
    uint32_t      length;
    z64_actor_t  *first;
  }               actor_list[12];         /* 0x01C30 */
  char            unk_05_[0x0038];        /* 0x01C90 */
  z64_actor_t    *arrow_actor;            /* 0x01CC8 */
  z64_actor_t    *target_actor;           /* 0x01CCC */
  char            unk_06_[0x0058];        /* 0x01CD0 */
  uint32_t        switch_flags;           /* 0x01D28 */
  uint32_t        temp_switch_flags;      /* 0x01D2C */
  uint32_t        unk_07_;                /* 0x01D30 */
  uint32_t        unk_08_;                /* 0x01D34 */
  uint32_t        chest_flags;            /* 0x01D38 */
  uint32_t        room_clear_flags;       /* 0x01D3C */
  uint32_t        unk_09_;                /* 0x01D40 */
  uint32_t        unk_0A_;                /* 0x01D44 */
  uint32_t        unk_0B_;                /* 0x01D48 */
  void           *title_card_texture;     /* 0x01D4C */
  char            unk_0C_[0x0007];        /* 0x01D50 */
  uint8_t         title_card_delay;       /* 0x01D57 */
  char            unk_0D_[0x0010];        /* 0x01D58 */
  void           *cutscene_ptr;           /* 0x01D68 */
  char            unk_0E_[0xFF50];        /* 0x01D6C */
  int8_t          room_index;             /* 0x11CBC */
  char            unk_0F_[0x000B];        /* 0x11CBD */
  void           *room_ptr;               /* 0x11CC8 */
  char            unk_10_[0x0118];        /* 0x11CCC */
  uint32_t        gameplay_frames;        /* 0x11DE4 */
  uint8_t         link_age;               /* 0x11DE8 */
  char            unk_11_;                /* 0x11DE9 */
  uint8_t         spawn_index;            /* 0x11DEA */
  uint8_t         no_map_actors;          /* 0x11DEB */
  uint8_t         no_rooms;               /* 0x11DEC */
  char            unk_12_[0x000B];        /* 0x11DED */
  void           *map_actor_list;         /* 0x11DF8 */
  char            unk_13_[0x0008];        /* 0x11DFC */
  void           *scene_exit_list;        /* 0x11E04 */
  char            unk_14_[0x000C];        /* 0x11E08 */
  uint16_t        scene_load_flags;       /* 0x11E14 */
  char            unk_15_[0x0004];        /* 0x11E16 */
  uint16_t        entrance_index;         /* 0x11E1A */
                                          /* 0x11E1C */
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
#define z64_entrance_table_addr                 0x800F9C90
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
#define z64_entrance_table_addr                 0x800F9E50
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
#define z64_entrance_table_addr                 0x800FA2E0
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
#define z64_entrance_table      ( (z64_entrance_table_t*)                     \
                                   z64_entrance_table_addr)
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
