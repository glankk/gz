#ifndef Z64_H
#define Z64_H
#include <stdint.h>
#include <n64.h>
#include "gu.h"

#ifndef Z64_VERSION
# error no z64 version specified
#endif

#define Z64_OOT10             0x00
#define Z64_OOT11             0x01
#define Z64_OOT12             0x02
#define Z64_OOTMQJ            0x03
#define Z64_OOTMQU            0x04
#define Z64_OOTGCJ            0x05
#define Z64_OOTGCU            0x06
#define Z64_OOTCEJ            0x07

#define Z64_SCREEN_WIDTH      320
#define Z64_SCREEN_HEIGHT     240

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

#define z64_disp_size         0x12410
#define z64_cimg_size         0x25800

typedef struct
{
  uint32_t          vrom_start;               /* 0x0000 */
  uint32_t          vrom_end;                 /* 0x0004 */
  uint32_t          prom_start;               /* 0x0008 */
  uint32_t          prom_end;                 /* 0x000C */
                                              /* 0x0010 */
} z64_ftab_t;

typedef struct
{
  uint32_t          vrom_start;               /* 0x0000 */
  uint32_t          vrom_end;                 /* 0x0004 */
                                              /* 0x0008 */
} z64_vrom_file_t;

typedef struct z64_arena      z64_arena_t;
typedef struct z64_arena_node z64_arena_node_t;

struct z64_arena
{
  z64_arena_node_t *first_node;               /* 0x0000 */
  void             *start;                    /* 0x0004 */
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
  uint32_t          size;                     /* 0x0008 */
  char              unk_0xC[0x0004];          /* 0x000C */
                                              /* 0x0010 */
#elif Z64_VERSION == Z64_OOTMQJ || \
      Z64_VERSION == Z64_OOTMQU || \
      Z64_VERSION == Z64_OOTGCJ || \
      Z64_VERSION == Z64_OOTGCU || \
      Z64_VERSION == Z64_OOTCEJ
                                              /* 0x0008 */
#endif
};

struct z64_arena_node
{
  uint16_t          magic;                    /* 0x0000 */
  uint16_t          free;                     /* 0x0002 */
  uint32_t          size;                     /* 0x0004 */
  z64_arena_node_t *next;                     /* 0x0008 */
  z64_arena_node_t *prev;                     /* 0x000C */
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
  char             *filename;                 /* 0x0010 */
  int32_t           line;                     /* 0x0014 */
  OSId              thread_id;                /* 0x0018 */
  z64_arena_t      *arena;                    /* 0x001C */
  uint32_t          count_hi;                 /* 0x0020 */
  uint32_t          count_lo;                 /* 0x0024 */
  char              pad_0x28[0x0008];         /* 0x0028 */
  char              data[];                   /* 0x0030 */
#elif Z64_VERSION == Z64_OOTMQJ || \
      Z64_VERSION == Z64_OOTMQU || \
      Z64_VERSION == Z64_OOTGCJ || \
      Z64_VERSION == Z64_OOTGCU || \
      Z64_VERSION == Z64_OOTCEJ
  char              data[];                   /* 0x0010 */
#endif
};

typedef struct
{
  uint32_t          section             : 2;  /* 0x0000 */
  uint32_t          type                : 6;  /* 0x0000 */
  uint32_t          offset              : 24; /* 0x0001 */
                                              /* 0x0004 */
} z64_reloc_t;

typedef struct
{
  uint32_t          text_size;                /* 0x0000 */
  uint32_t          data_size;                /* 0x0004 */
  uint32_t          rodata_size;              /* 0x0008 */
  uint32_t          bss_size;                 /* 0x000C */
  uint32_t          n_relocs;                 /* 0x0010 */
  z64_reloc_t       relocs[];                 /* 0x0014 */
} z64_ovl_hdr_t;

typedef struct
{
  void             *ptr;                      /* 0x0000 */
  uint32_t          vrom_start;               /* 0x0004 */
  uint32_t          vrom_end;                 /* 0x0008 */
  uint32_t          vram_start;               /* 0x000C */
  uint32_t          vram_end;                 /* 0x0010 */
  char              unk_0x14[0x0004];         /* 0x0014 */
  uint32_t          vram_ctor;                /* 0x0018 */
  uint32_t          vram_dtor;                /* 0x001C */
  char              unk_0x20[0x000C];         /* 0x0020 */
  char              ctxt_size;                /* 0x002C */
                                              /* 0x0030 */
} z64_state_ovl_t;

typedef struct
{
  uint32_t          vrom_start;               /* 0x0000 */
  uint32_t          vrom_end;                 /* 0x0004 */
  uint32_t          vram_start;               /* 0x0008 */
  uint32_t          vram_end;                 /* 0x000C */
  void             *ptr;                      /* 0x0010 */
  uint32_t          vram_info;                /* 0x0014 */
  uint32_t          unk_0x18;                 /* 0x0018 */
                                              /* 0x001C */
} z64_part_ovl_t;

typedef struct
{
  uint32_t          vrom_start;               /* 0x0000 */
  uint32_t          vrom_end;                 /* 0x0004 */
  uint32_t          vram_start;               /* 0x0008 */
  uint32_t          vram_end;                 /* 0x000C */
  void             *ptr;                      /* 0x0010 */
  uint32_t          vram_info;                /* 0x0014 */
  char             *filename;                 /* 0x0018 */
  int16_t           alloc_type;               /* 0x001C */
  uint8_t           n_inst;                   /* 0x001E */
  char              pad_0x1F[0x0001];         /* 0x001F */
                                              /* 0x0020 */
} z64_actor_ovl_t;

typedef struct
{
  void             *ptr;                      /* 0x0000 */
  uint32_t          vrom_start;               /* 0x0004 */
  uint32_t          vrom_end;                 /* 0x0008 */
  uint32_t          vram_start;               /* 0x000C */
  uint32_t          vram_end;                 /* 0x0010 */
  uint32_t          vram_data_tab;            /* 0x0014 */
                                              /* 0x0018 */
} z64_map_mark_ovl_t;

typedef struct
{
  void             *ptr;                      /* 0x0000 */
  uint32_t          vrom_start;               /* 0x0004 */
  uint32_t          vrom_end;                 /* 0x0008 */
  uint32_t          vram_start;               /* 0x000C */
  uint32_t          vram_end;                 /* 0x0010 */
  int32_t           reloc_offset;             /* 0x0014 */
  char             *filename;                 /* 0x0018 */
                                              /* 0x001C */
} z64_play_ovl_t;

typedef struct
{
  int16_t           x;                        /* 0x0000 */
  int16_t           y;                        /* 0x0002 */
  int16_t           z;                        /* 0x0004 */
                                              /* 0x0006 */
} z64_xyz_t;

typedef struct
{
  float             x;                        /* 0x0000 */
  float             y;                        /* 0x0004 */
  float             z;                        /* 0x0008 */
                                              /* 0x000C */
} z64_xyzf_t;

typedef uint16_t z64_angle_t;
typedef struct
{
  z64_angle_t       x;                        /* 0x0000 */
  z64_angle_t       y;                        /* 0x0002 */
  z64_angle_t       z;                        /* 0x0004 */
                                              /* 0x0006 */
} z64_rot_t;

typedef struct
{
  /* index of z64_col_type in col_hdr */
  uint16_t          type;                     /* 0x0000 */
  /* vertex indices, a and b are bitmasked for some reason */
  struct
  {
    uint16_t        unk_00              : 3;
    uint16_t        va                  : 13;
  };                                          /* 0x0002 */
  struct
  {
    uint16_t        unk_01              : 3;
    uint16_t        vb                  : 13;
  };                                          /* 0x0004 */
  uint16_t          vc;                       /* 0x0006 */
  /* normal vector */
  z64_xyz_t         norm;                     /* 0x0008 */
  /* plane distance from origin */
  int16_t           dist;                     /* 0x000E */
                                              /* 0x0010 */
} z64_col_poly_t;

typedef struct
{
  struct
  {
    uint32_t        unk_00              : 1;
    /* link drops one unit into the floor */
    uint32_t        drop                : 1;
    uint32_t        special             : 4;
    uint32_t        interaction         : 5;
    uint32_t        unk_01              : 3;
    uint32_t        behavior            : 5;
    uint32_t        exit                : 5;
    uint32_t        camera              : 8;
  } flags_1;                                  /* 0x0000 */
  struct
  {
    uint32_t        pad_00              : 4;
    uint32_t        wall_damage         : 1;
    uint32_t        unk_00              : 6;
    uint32_t        unk_01              : 3;
    uint32_t        hookshot            : 1;
    uint32_t        echo                : 6;
    uint32_t        unk_02              : 5;
    uint32_t        terrain             : 2;
    uint32_t        material            : 4;
  } flags_2;                                  /* 0x0004 */
                                              /* 0x0008 */
} z64_col_type_t;

typedef struct
{
  z64_xyz_t         pos;                      /* 0x0000 */
  z64_xyz_t         rot;                      /* 0x0006 */
  int16_t           fov;                      /* 0x000C */
  int16_t           unk_0xE;                  /* 0x000E */
                                              /* 0x0010 */
} z64_cam_params_t;

typedef struct
{
  uint16_t          mode;                     /* 0x0000 */
  uint16_t          unk_0x2;                  /* 0x0002 */
  z64_cam_params_t *params;                   /* 0x0004 */
  char              unk_0x8[0x012C];          /* 0x0008 */
  z64_xyz_t         input_dir;                /* 0x0134 */
  char              unk_0x13A[0x0032];        /* 0x013A */
} z64_camera_t;                               /* 0x016C */

typedef struct
{
  z64_xyz_t         pos;                      /* 0x0000 */
  int16_t           width;                    /* 0x0006 */
  int16_t           depth;                    /* 0x0008 */
  char              pad_0xA[0x0002];          /* 0x000A */
  struct
  {
    uint32_t        unk_00              : 12;
    uint32_t        active              : 1;
     /* ? */
    uint32_t        group               : 6;
    uint32_t        unk_01              : 5;
    uint32_t        camera              : 8;
  } flags;                                    /* 0x000C */
                                              /* 0x0010 */
} z64_col_water_t;

typedef struct
{
  z64_xyz_t         min;                      /* 0x0000 */
  z64_xyz_t         max;                      /* 0x0006 */
  uint16_t          n_vtx;                    /* 0x000C */
  char              pad_0xE[0x0002];          /* 0x000E */
  z64_xyz_t        *vtx;                      /* 0x0010 */
  uint16_t          n_poly;                   /* 0x0014 */
  char              pad_0x16[0x0002];         /* 0x0016 */
  z64_col_poly_t   *poly;                     /* 0x0018 */
  z64_col_type_t   *type;                     /* 0x001C */
  z64_camera_t     *camera;                   /* 0x0020 */
  uint16_t          n_water;                  /* 0x0024 */
  char              pad_0x26[0x0002];         /* 0x0026 */
  z64_col_water_t  *water;                    /* 0x0028 */
                                              /* 0x002C */
} z64_col_hdr_t;

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

typedef enum
{
  Z64_ACTIONBTN_A,
  Z64_ACTIONBTN_B,
  Z64_ACTIONBTN_START,
} z64_actionbtn_t;

typedef enum
{
  Z64_ITEMBTN_B,
  Z64_ITEMBTN_CL,
  Z64_ITEMBTN_CD,
  Z64_ITEMBTN_CR,
} z64_itembtn_t;

typedef struct
{
  char              unk_0x0[0x006E];          /* 0x0000 */
  int16_t           run_speed_limit;          /* 0x006E */
  char              unk_0x70[0x0004];         /* 0x0070 */
  int16_t           run_speed_max_anim;       /* 0x0074 */
  char              unk_0x76[0x0026];         /* 0x0076 */
  int16_t           gravity;                  /* 0x009C */
  char              unk_0x9E[0x0072];         /* 0x009E */
  uint16_t          update_rate;              /* 0x0110 */
  char              unk_0x112[0x0022];        /* 0x0112 */
  int16_t           override_aspect;          /* 0x0134 */
  uint16_t          aspect_width;             /* 0x0136 */
  uint16_t          aspect_height;            /* 0x0138 */
  char              unk_0x13A[0x0050];        /* 0x013A */
  int16_t           game_playing;             /* 0x018A */
  char              unk_0x18C[0x0004];        /* 0x018C */
  int16_t           screenshot_state;         /* 0x0190 */
  char              unk_0x192[0x03B2];        /* 0x0192 */
  uint16_t          c_up_icon_x;              /* 0x0544 */
  uint16_t          c_up_icon_y;              /* 0x0546 */
  char              unk_0x548[0x021C];        /* 0x0548 */
  uint16_t          game_freeze;              /* 0x0764 */
  char              unk_0x766[0x002E];        /* 0x0766 */
  uint16_t          magic_fill_r;             /* 0x0794 */
  uint16_t          magic_fill_g;             /* 0x0796 */
  uint16_t          magic_fill_b;             /* 0x0798 */
  char              unk_0x79A[0x004A];        /* 0x079A */
  uint16_t          c_button_r;               /* 0x07E4 */
  uint16_t          c_button_g;               /* 0x07E6 */
  uint16_t          c_button_b;               /* 0x07E8 */
  uint16_t          b_button_r;               /* 0x07EA */
  uint16_t          b_button_g;               /* 0x07EC */
  uint16_t          b_button_b;               /* 0x07EE */
  char              unk_0x7F0[0x0004];        /* 0x07F0 */
  qs510_t           start_icon_dd;            /* 0x07F4 */
  int16_t           start_icon_scale;         /* 0x07F6 */
  char              unk_0x7F8[0x0006];        /* 0x07F8 */
  uint16_t          start_icon_y;             /* 0x07FE */
  char              unk_0x800[0x0002];        /* 0x0800 */
  uint16_t          start_icon_x;             /* 0x0802 */
  char              unk_0x804[0x000C];        /* 0x0804 */
  uint16_t          c_up_button_x;            /* 0x0810 */
  uint16_t          c_up_button_y;            /* 0x0812 */
  char              unk_0x814[0x0008];        /* 0x0814 */
  uint16_t          start_button_x;           /* 0x081C */
  uint16_t          start_button_y;           /* 0x081E */
  uint16_t          item_button_x[4];         /* 0x0820 */
  uint16_t          item_button_y[4];         /* 0x0828 */
  qs510_t           item_button_dd[4];        /* 0x0830 */
  uint16_t          item_icon_x[4];           /* 0x0838 */
  uint16_t          item_icon_y[4];           /* 0x0840 */
  qs510_t           item_icon_dd[4];          /* 0x0848 */
  char              unk_0x850[0x0264];        /* 0x0850 */
  uint16_t          a_button_y;               /* 0x0AB4 */
  uint16_t          a_button_x;               /* 0x0AB6 */
  char              unk_0xAB8[0x0002];        /* 0x0AB8 */
  uint16_t          a_button_icon_y;          /* 0x0ABA */
  uint16_t          a_button_icon_x;          /* 0x0ABC */
  char              unk_0xABE[0x0002];        /* 0x0ABE */
  uint16_t          a_button_r;               /* 0x0AC0 */
  uint16_t          a_button_g;               /* 0x0AC2 */
  uint16_t          a_button_b;               /* 0x0AC4 */
  char              unk_0xAC6[0x0030];        /* 0x0AC6 */
  uint16_t          magic_bar_x;              /* 0x0AF6 */
  uint16_t          magic_bar_y;              /* 0x0AF8 */
  uint16_t          magic_fill_x;             /* 0x0AFA */
  char              unk_0xAFC[0x02D6];        /* 0x0AFC */
  int16_t           minimap_disabled;         /* 0x0DD2 */
  char              unk_0xDD4[0x015A];        /* 0x0DD4 */
  int16_t           dungeon_map_floor;        /* 0x0F2E */
  char              unk_0xF30[0x0064];        /* 0x0F30 */
  uint16_t          item_ammo_x[4];           /* 0x0F94 */
  uint16_t          item_ammo_y[4];           /* 0x0F9C */
  char              unk_0xFA4[0x0008];        /* 0x0FA4 */
  uint16_t          item_icon_space[4];       /* 0x0FAC */
  uint16_t          item_button_space[4];     /* 0x0FB4 */
  char              unk_0xFBC[0x0618];        /* 0x0FBC */
                                              /* 0x15D4 */
} z64_gameinfo_t;

typedef struct
{
  int32_t           entrance_index;           /* 0x0000 */
  int32_t           link_age;                 /* 0x0004 */
  char              unk_0x8[0x0002];          /* 0x0008 */
  uint16_t          cutscene_index;           /* 0x000A */
  uint16_t          day_time;                 /* 0x000C */
  char              unk_0xE[0x0002];          /* 0x000E */
  int32_t           night_flag;               /* 0x0010 */
  char              unk_0x14[0x0008];         /* 0x0014 */
  char              id[6];                    /* 0x001C */
  int16_t           deaths;                   /* 0x0022 */
  char              file_name[0x08];          /* 0x0024 */
  int16_t           n64dd_flag;               /* 0x002C */
  int16_t           energy_capacity;          /* 0x002E */
  int16_t           energy;                   /* 0x0030 */
  uint8_t           magic_capacity_set;       /* 0x0032 */
  uint8_t           magic;                    /* 0x0033 */
  uint16_t          rupees;                   /* 0x0034 */
  uint16_t          bgs_hits_left;            /* 0x0036 */
  uint16_t          navi_timer;               /* 0x0038 */
  uint8_t           magic_acquired;           /* 0x003A */
  char              unk_0x3B;                 /* 0x003B */
  uint8_t           magic_capacity;           /* 0x003C */
  int8_t            double_defense;           /* 0x003D */
  int8_t            bgs_flag;                 /* 0x003E */
  char              unk_0x3F;                 /* 0x003F */
  int8_t            child_button_items[4];    /* 0x0040 */
  int8_t            child_c_button_slots[3];  /* 0x0044 */
  union
  {
    uint16_t        child_equips;             /* 0x0048 */
    struct
    {
      uint16_t      child_equip_boots   : 4;
      uint16_t      child_equip_tunic   : 4;
      uint16_t      child_equip_shield  : 4;
      uint16_t      child_equip_sword   : 4;
    };
  };
  int8_t            adult_button_items[4];    /* 0x004A */
  int8_t            adult_c_button_slots[3];  /* 0x004E */
  union
  {
    uint16_t        adult_equips;             /* 0x0052 */
    struct
    {
      uint16_t      adult_equip_boots   : 4;
      uint16_t      adult_equip_tunic   : 4;
      uint16_t      adult_equip_shield  : 4;
      uint16_t      adult_equip_sword   : 4;
    };
  };
  char              unk_0x54[0x0012];         /* 0x0054 */
  int16_t           scene_index;              /* 0x0066 */
  int8_t            button_items[4];          /* 0x0068 */
  int8_t            c_button_slots[3];        /* 0x006C */
  union
  {
    uint16_t        equips;                   /* 0x0070 */
    struct
    {
      uint16_t      equip_boots         : 4;
      uint16_t      equip_tunic         : 4;
      uint16_t      equip_shield        : 4;
      uint16_t      equip_sword         : 4;
    };
  };
  char              unk_0x72[0x0002];         /* 0x0072 */
  int8_t            items[24];                /* 0x0074 */
  int8_t            ammo[15];                 /* 0x008C */
  uint8_t           magic_beans_sold;         /* 0x009B */
  union
  {
    uint16_t        equipment;                /* 0x009C */
    struct
    {
      uint16_t                          : 1;
      uint16_t      hover_boots         : 1;
      uint16_t      iron_boots          : 1;
      uint16_t      kokiri_boots        : 1;
      uint16_t                          : 1;
      uint16_t      zora_tunic          : 1;
      uint16_t      goron_tunic         : 1;
      uint16_t      kokiri_tunic        : 1;
      uint16_t                          : 1;
      uint16_t      mirror_shield       : 1;
      uint16_t      hylian_shield       : 1;
      uint16_t      deku_shield         : 1;
      uint16_t      broken_giants_knife : 1;
      uint16_t      giants_knife        : 1;
      uint16_t      master_sword        : 1;
      uint16_t      kokiri_sword        : 1;
    };
  };
  char              unk_0x9E[0x0002];         /* 0x009E */
  union
  {
    uint32_t        equipment_items;          /* 0x00A0 */
    struct
    {
      uint32_t                          : 9;
      uint32_t      nut_upgrade         : 3;
      uint32_t      stick_upgrade       : 3;
      uint32_t      bullet_bag          : 3;
      uint32_t      wallet              : 2;
      uint32_t      diving_upgrade      : 3;
      uint32_t      strength_upgrade    : 3;
      uint32_t      bomb_bag            : 3;
      uint32_t      quiver              : 3;
    };
  };
  union
  {
    uint32_t        quest_items;              /* 0x00A4 */
    struct
    {
      uint32_t      heart_pieces        : 8;
      uint32_t      gold_skulltula      : 1;
      uint32_t      gerudos_card        : 1;
      uint32_t      stone_of_agony      : 1;
      uint32_t      zoras_sapphire      : 1;
      uint32_t      gorons_ruby         : 1;
      uint32_t      kokiris_emerald     : 1;
      uint32_t      song_of_storms      : 1;
      uint32_t      song_of_time        : 1;
      uint32_t      suns_song           : 1;
      uint32_t      sarias_song         : 1;
      uint32_t      eponas_song         : 1;
      uint32_t      zeldas_lullaby      : 1;
      uint32_t      prelude_of_light    : 1;
      uint32_t      nocturne_of_shadow  : 1;
      uint32_t      requiem_of_spirit   : 1;
      uint32_t      serenade_of_water   : 1;
      uint32_t      bolero_of_fire      : 1;
      uint32_t      minuet_of_forest    : 1;
      uint32_t      light_medallion     : 1;
      uint32_t      shadow_medallion    : 1;
      uint32_t      spirit_medallion    : 1;
      uint32_t      water_medallion     : 1;
      uint32_t      fire_medallion      : 1;
      uint32_t      forest_medallion    : 1;
    };
  };
  union
  {
    uint8_t         items;
    struct
    {
      uint8_t                           : 5;
      uint8_t       map                 : 1;
      uint8_t       compass             : 1;
      uint8_t       boss_key            : 1;
    };
  }                 dungeon_items[20];        /* 0x00A8 */
  int8_t            dungeon_keys[19];         /* 0x00BC */
  uint8_t           defense_hearts;           /* 0x00CF */
  int16_t           gs_tokens;                /* 0x00D0 */
  char              unk_0xD2[0x0002];         /* 0x00D2 */
  struct
  {
    uint32_t        chest;
    uint32_t        swch;
    uint32_t        clear;
    uint32_t        collect;
    uint32_t        unk_0x10;
    uint32_t        rooms_1;
    uint32_t        rooms_2;
  }                 scene_flags[101];         /* 0x00D4 */
  char              unk_0xBE0[0x0284];        /* 0x0BE0 */
  z64_xyzf_t        fw_pos;                   /* 0x0E64 */
  z64_angle_t       fw_yaw;                   /* 0x0E70 */
  char              unk_0xE72[0x0008];        /* 0x0E72 */
  uint16_t          fw_scene_index;           /* 0x0E7A */
  uint32_t          fw_room_index;            /* 0x0E7C */
  int32_t           fw_set;                   /* 0x0E80 */
  char              unk_0xE84[0x0018];        /* 0x0E84 */
  uint32_t          gs_flags[6];              /* 0x0E9C */
  char              unk_EBC[0x0004];          /* 0x0EB4 */
  int32_t           high_scores[7];           /* 0x0EB8 */
  uint16_t          event_chk_inf[14];        /* 0x0ED4 */
  uint16_t          item_get_inf[4];          /* 0x0EF0 */
  uint16_t          inf_table[30];            /* 0x0EF8 */
  char              unk_0xF34[0x041E];        /* 0x0F34 */
  uint16_t          checksum;                 /* 0x1352 */
  char              unk_0x1354[0x0003];       /* 0x1354 */
  int8_t            file_index;               /* 0x1357 */
  char              unk_0x1358[0x0004];       /* 0x1358 */
  int32_t           interface_flag;           /* 0x135C */
  uint32_t          scene_setup_index;        /* 0x1360 */
  int32_t           void_flag;                /* 0x1364 */
  z64_xyzf_t        void_pos;                 /* 0x1368 */
  z64_angle_t       void_yaw;                 /* 0x1374 */
  int16_t           void_var;                 /* 0x1376 */
  int16_t           void_entrance;            /* 0x1378 */
  int8_t            void_room_index;          /* 0x137A */
  int8_t            unk_0x137B;               /* 0x137B */
  uint32_t          temp_swch_flags;          /* 0x137C */
  uint32_t          temp_collect_flags;       /* 0x1380 */
  char              unk_0x1384[0x0044];       /* 0x1384 */
  uint16_t          nayrus_love_timer;        /* 0x13C8 */
  char              unk_0x13CA[0x0004];       /* 0x13CA */
  int16_t           timer_1_state;            /* 0x13CE */
  int16_t           timer_1_value;            /* 0x13D0 */
  int16_t           timer_2_state;            /* 0x13D2 */
  int16_t           timer_2_value;            /* 0x13D4 */
  char              unk_0x13D6[0x000A];       /* 0x13D6 */
  int8_t            seq_index;                /* 0x13E0 */
  int8_t            night_sfx;                /* 0x13E1 */
  char              unk_0x13E2[0x0006];       /* 0x13E2 */
  uint16_t          hud_flag;                 /* 0x13E8 */
  char              unk_0x13EA[0x10];         /* 0x13EA */
  uint16_t          event_inf[4];             /* 0x13FA */
  char              unk_0x1402[0x0001];       /* 0x1402 */
  uint8_t           minimap_index;            /* 0x1403 */
  int16_t           minigame_state;           /* 0x1404 */
  char              unk_0x1406[0x0003];       /* 0x1406 */
  uint8_t           language;                 /* 0x1409 */
  char              unk_0x140A[0x0002];       /* 0x140A */
  uint8_t           z_targeting;              /* 0x140C */
  char              unk_0x140D[0x0001];       /* 0x140D */
  uint16_t          disable_music_flag;       /* 0x140E */
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
  char              unk_0x1410[0x0020];       /* 0x1410 */
  z64_gameinfo_t   *gameinfo;                 /* 0x1430 */
  char              unk_0x1434[0x001C];       /* 0x1434 */
#elif Z64_VERSION == Z64_OOTMQJ || \
      Z64_VERSION == Z64_OOTMQU || \
      Z64_VERSION == Z64_OOTGCJ || \
      Z64_VERSION == Z64_OOTGCU || \
      Z64_VERSION == Z64_OOTCEJ
  char              unk_0x1410[0x0018];       /* 0x1410 */
  z64_gameinfo_t   *gameinfo;                 /* 0x1428 */
  char              unk_0x142C[0x0024];       /* 0x142C */
#endif
                                              /* 0x1450 */
} z64_file_t;

typedef struct
{
  uint32_t          seg[16];                  /* 0x0000 */
                                              /* 0x0040 */
} z64_stab_t;

typedef struct
{
  uint8_t           scene_index;              /* 0x0000 */
  uint8_t           entrance_index;           /* 0x0001 */
  union
  {
    uint16_t        variable;                 /* 0x0002 */
    struct
    {
      uint16_t      transition_out      : 7;
      uint16_t      transition_in       : 7;
      uint16_t      unk_00              : 1;
      uint16_t      continue_music      : 1;
    };
  };
                                              /* 0x0004 */
} z64_entrance_table_t;

typedef struct
{
  uint32_t          scene_vrom_start;         /* 0x0000 */
  uint32_t          scene_vrom_end;           /* 0x0004 */
  uint32_t          title_vrom_start;         /* 0x0008 */
  uint32_t          title_vrom_end;           /* 0x000C */
  char              unk_0x10[0x0001];         /* 0x0010 */
  uint8_t           scene_config;             /* 0x0011 */
  char              unk_0x12[0x0001];         /* 0x0012 */
  char              pad_0x13[0x0001];         /* 0x0013 */
                                              /* 0x0014 */
} z64_scene_table_t;

typedef struct
{
  uint32_t          size;                     /* 0x0000 */
  Gfx              *buf;                      /* 0x0004 */
  Gfx              *p;                        /* 0x0008 */
  Gfx              *d;                        /* 0x000C */
                                              /* 0x0010 */
} z64_disp_buf_t;

typedef struct
{
  Gfx              *poly_opa_w;               /* 0x0000 */
  Gfx              *poly_xlu_w;               /* 0x0004 */
  char              unk_0x8[0x0008];          /* 0x0008 */
  Gfx              *overlay_w;                /* 0x0010 */
  char              unk_0x14[0x0024];         /* 0x0014 */
  OSMesg            task_msg[8];              /* 0x0038 */
  char              unk_0x58[0x0004];         /* 0x0058 */
  OSMesgQueue       task_mq;                  /* 0x005C */
  char              pad_0x74[0x0004];         /* 0x0074 */
  OSScTask          task;                     /* 0x0078 */
  char              unk_0xD0[0x00E0];         /* 0x00D0 */
  Gfx              *work_w;                   /* 0x01B0 */
  z64_disp_buf_t    work;                     /* 0x01B4 */
  char              unk_0x1C4[0x00E4];        /* 0x01C4 */
  z64_disp_buf_t    overlay;                  /* 0x02A8 */
  z64_disp_buf_t    poly_opa;                 /* 0x02B8 */
  z64_disp_buf_t    poly_xlu;                 /* 0x02C8 */
  uint32_t          frame_count_1;            /* 0x02D8 */
  void             *frame_buffer;             /* 0x02DC */
  char              unk_0x2E0[0x0008];        /* 0x02E0 */
  uint32_t          frame_count_2;            /* 0x02E8 */
                                              /* 0x02EC */
} z64_gfx_t;

typedef struct
{
  union
  {
    struct
    {
      uint16_t      a                   : 1;
      uint16_t      b                   : 1;
      uint16_t      z                   : 1;
      uint16_t      s                   : 1;
      uint16_t      du                  : 1;
      uint16_t      dd                  : 1;
      uint16_t      dl                  : 1;
      uint16_t      dr                  : 1;
      uint16_t                          : 2;
      uint16_t      l                   : 1;
      uint16_t      r                   : 1;
      uint16_t      cu                  : 1;
      uint16_t      cd                  : 1;
      uint16_t      cl                  : 1;
      uint16_t      cr                  : 1;
    };
    uint16_t        pad;                      /* 0x0000 */
  };
  int8_t            x;                        /* 0x0002 */
  int8_t            y;                        /* 0x0003 */
                                              /* 0x0004 */
} z64_controller_t;

enum z64_actor_id
{
  Z64_ACTOR_EN_HOLL = 0x0023
};

enum z64_actor_type
{
  Z64_ACTORTYPE_SWITCH,
  Z64_ACTORTYPE_BG,
  Z64_ACTORTYPE_PLAYER,
  Z64_ACTORTYPE_EXPLOSIVE,
  Z64_ACTORTYPE_NPC,
  Z64_ACTORTYPE_ENEMY,
  Z64_ACTORTYPE_PROP,
  Z64_ACTORTYPE_ITEMACTION,
  Z64_ACTORTYPE_MISC,
  Z64_ACTORTYPE_BOSS,
  Z64_ACTORTYPE_DOOR,
  Z64_ACTORTYPE_CHEST
};

typedef struct z64_actor_s z64_actor_t;
struct z64_actor_s
{
  int16_t           actor_id;                 /* 0x0000 */
  uint8_t           actor_type;               /* 0x0002 */
  int8_t            room_index;               /* 0x0003 */
  uint32_t          flags;                    /* 0x0004 */
  z64_xyzf_t        pos_1;                    /* 0x0008 */
  z64_rot_t         rot_init;                 /* 0x0014 */
  char              unk_0x1A[0x0002];         /* 0x001A */
  uint16_t          variable;                 /* 0x001C */
  uint8_t           alloc_index;              /* 0x001E */
  char              unk_0x1F;                 /* 0x001F */
  uint16_t          sound_effect;             /* 0x0020 */
  char              unk_0x22[0x0002];         /* 0x0022 */
  z64_xyzf_t        pos_2;                    /* 0x0024 */
  char              unk_0x30[0x0002];         /* 0x0030 */
  uint16_t          xz_dir;                   /* 0x0032 */
  char              unk_0x34[0x0004];         /* 0x0034 */
  z64_xyzf_t        pos_3;                    /* 0x0038 */
  z64_rot_t         rot_1;                    /* 0x0044 */
  char              unk_0x4A[0x0002];         /* 0x004A */
  float             unk_0x4C;                 /* 0x004C */
  z64_xyzf_t        scale;                    /* 0x0050 */
  z64_xyzf_t        vel_1;                    /* 0x005C */
  float             xz_speed;                 /* 0x0068 */
  float             gravity;                  /* 0x006C */
  float             min_vel_y;                /* 0x0070 */
  /* struct bgcheck common */
  z64_col_poly_t   *wall_poly;                /* 0x0074 */
  z64_col_poly_t   *floor_poly;               /* 0x0078 */
  uint8_t           wall_poly_source;         /* 0x007C */
  uint8_t           floor_poly_source;        /* 0x007D */
  int16_t           wall_rot;                 /* 0x007E */
  float             floor_height;             /* 0x0080 */
  float             water_surface_dist;       /* 0x0084 */
  uint16_t          bgcheck_flags;            /* 0x0088 */
  int16_t           unk_0x8A_rot;             /* 0x008A */
  float             unk_0x8C;                 /* 0x008C */
  float             dist_from_link_xz;        /* 0x0090 */
  float             dist_from_link_y;         /* 0x0094 */
  /* struct collision_check common */
  void             *damage_table;             /* 0x0098 */
  z64_xyzf_t        vel_2;                    /* 0x009C */
  char              unk_0xA8[0x0006];         /* 0x00A8 */
  uint8_t           mass;                     /* 0x00AE */
  uint8_t           health;                   /* 0x00AF */
  uint8_t           damage;                   /* 0x00B0 */
  uint8_t           damage_effect;            /* 0x00B1 */
  uint8_t           impact_effect;            /* 0x00B2 */
  char              unk_0D;                   /* 0x00B3 */
  /* end CollisionCheck common */
  /* struct start */
  z64_rot_t         rot_2;                    /* 0x00B4 */
  char              unk_0xBA[0x0002];         /* 0x00BA */
  float             unk_0xBC;                 /* 0x00BC */
  void             *draw_drop_shadow;         /* 0x00C0 */
  float             unk_0xC4;                 /* 0x00C4 */
  uint8_t           unk_0xC8;                 /* 0x00C8 */
  char              pad_0xC9[0x0003];         /* 0x00C9 */
  /* struct end */
  z64_xyzf_t        unk_0xCC;                 /* 0x00CC */
  z64_xyzf_t        unk_0xD8;                 /* 0x00D8 */
  z64_xyzf_t        projectedPos;             /* 0x00E4 */
  float             projectedW;               /* 0x00F0 */
  float             uncullZoneForward;        /* 0x00F4 */
  float             uncullZoneScale;          /* 0x00F8 */
  float             uncullZoneDownward;       /* 0x00FC */
  z64_xyzf_t        pos_4;                    /* 0x0100 */
  uint16_t          unk_0x10C;                /* 0x010C */
  uint16_t          text_id;                  /* 0x010E */
  int16_t           frozen;                   /* 0x0110 */
  char              unk_0x112[0x0003];        /* 0x0112 */
  uint8_t           active;                   /* 0x0115 */
  uint8_t           unk_0x116;                /* 0x0116 */
  uint8_t           navi_enemy_text_id;       /* 0x0117 */
  z64_actor_t      *attached_a;               /* 0x0118 */
  z64_actor_t      *attached_b;               /* 0x011C */
  z64_actor_t      *prev;                     /* 0x0120 */
  z64_actor_t      *next;                     /* 0x0124 */
  void             *ctor;                     /* 0x0128 */
  void             *dtor;                     /* 0x012C */
  void             *main_proc;                /* 0x0130 */
  void             *draw_proc;                /* 0x0134 */
  void             *code_entry;               /* 0x0138 */
                                              /* 0x013C */
};

typedef struct
{
  z64_actor_t       common;                   /* 0x0000 */
  char              unk_0x13C[0x0070];        /* 0x013C */
  uint32_t          current_animation;        /* 0x01AC */
  char              unk_0x1B0[0x000C];        /* 0x01B0 */
  float             animation_timer;          /* 0x01BC */
  char              unk_0x1C0[0x0274];        /* 0x01C0 */
  uint8_t           action;                   /* 0x0434 */
  char              unk_0x435[0x0237];        /* 0x0435 */
  uint32_t          state_flags_1;            /* 0x066C */
  uint32_t          state_flags_2;            /* 0x0670 */
  char              unk_0x674[0x01B4];        /* 0x0674 */
  float             linear_vel;               /* 0x0828 */
  char              unk_0x82C[0x0002];        /* 0x082C */
  uint16_t          target_yaw;               /* 0x082E */
  char              unk_0x830[0x0003];        /* 0x0830 */
  int8_t            sword_state;              /* 0x0833 */
  char              unk_0x834[0x0050];        /* 0x0834 */
  int16_t           drop_y;                   /* 0x0884 */
  int16_t           drop_distance;            /* 0x0886 */
                                              /* 0x0888 */
} z64_link_t;

typedef struct
{
  z64_controller_t  raw;                      /* 0x0000 */
  /* 0x0000: ok */
  /* 0x0800: device not present */
  /* 0x0400: transaction error */
  uint16_t          status;                   /* 0x0004 */
  z64_controller_t  raw_prev;                 /* 0x0006 */
  uint16_t          status_prev;              /* 0x000A */
  uint16_t          pad_pressed;              /* 0x000C */
  int8_t            x_diff;                   /* 0x000E */
  int8_t            y_diff;                   /* 0x000F */
  char              unk_0x10[0x0002];         /* 0x0010 */
  uint16_t          pad_released;             /* 0x0012 */
  int8_t            adjusted_x;               /* 0x0014 */
  int8_t            adjusted_y;               /* 0x0015 */
  char              unk_0x16[0x0002];         /* 0x0016 */
                                              /* 0x0018 */
} z64_input_t;

typedef struct {
    /* string literal "VIEW" / 0x56494557 */
    uint32_t        magic;                    /* 0x0000 */
    /* pointer to gfx ctx */
    z64_gfx_t      *gfx;                      /* 0x0004 */
    /* view properties */
    float           viewport[4];              /* 0x0008 */
    float           fovy;                     /* 0x0018 */
    float           zNear;                    /* 0x001C */
    float           zFar;                     /* 0x0020 */
    float           scale;                    /* 0x0024 */
    z64_xyzf_t      eye;                      /* 0x0028 */
    z64_xyzf_t      at;                       /* 0x0034 */
    z64_xyzf_t      up;                       /* 0x0040 */
    Vp              vp;                       /* 0x0050 */
    Mtx             projection;               /* 0x0060 */
    Mtx             viewing;                  /* 0x00A0 */
    Mtx            *projectionPtr;            /* 0x00E0 */
    Mtx            *viewingPtr;               /* 0x00E4 */
    /* unknown */
    z64_xyzf_t      unk_E8;                   /* 0x00E8 */
    z64_xyzf_t      unk_F4;                   /* 0x00F4 */
    float           unk_100;                  /* 0x0100 */
    z64_xyzf_t      unk_104;                  /* 0x0104 */
    z64_xyzf_t      unk_110;                  /* 0x0110 */
    uint16_t        normal;                   /* 0x011C */
    uint32_t        flags;                    /* 0x0120 */
    uint32_t        unk_124;                  /* 0x0124 */
                                              /* 0x0128 */
} z64_view_t;

/* state context base */
typedef struct z64_ctxt z64_ctxt_t;
typedef void (*z64_ctxt_proc_t)(z64_ctxt_t *ctxt);
struct z64_ctxt
{
  z64_gfx_t        *gfx;                      /* 0x0000 */
  z64_ctxt_proc_t   state_main;               /* 0x0004 */
  z64_ctxt_proc_t   state_dtor;               /* 0x0008 */
  uint32_t          next_ctor;                /* 0x000C */
  uint32_t          next_size;                /* 0x0010 */
  z64_input_t       input[4];                 /* 0x0014 */
  uint32_t          state_heap_size;          /* 0x0074 */
  void             *state_heap;               /* 0x0078 */
  void             *heap_start;               /* 0x007C */
  void             *heap_end;                 /* 0x0080 */
  void             *state_heap_node;          /* 0x0084 */
  char              unk_0x88[0x0010];         /* 0x0088 */
  int32_t           state_continue;           /* 0x0098 */
  int32_t           state_frames;             /* 0x009C */
  uint32_t          unk_0xA0;                 /* 0x00A0 */
                                              /* 0x00A4 */
};

typedef struct z64_part_s z64_part_t;
typedef int32_t (*z64_part_ctor_t)(z64_ctxt_t *ctxt, int part_index,
                                   z64_part_t *part, void *cdata);
typedef void    (*z64_part_proc_t)(z64_ctxt_t *ctxt, int part_index,
                                   z64_part_t *part);

struct z64_part_s
{
  z64_xyzf_t        pos;                      /* 0x0000 */
  z64_xyzf_t        speed;                    /* 0x000C */
  z64_xyzf_t        accel;                    /* 0x0018 */
  z64_part_proc_t  *main_proc;                /* 0x0024 */
  z64_part_proc_t  *draw_proc;                /* 0x0028 */
  /* begin particle-defined fields */
  z64_xyzf_t        data_xyz;                 /* 0x002C */
  Gfx              *disp;                     /* 0x0038 */
  int32_t           data_32;                  /* 0x003C */
  int16_t           data_16[14];              /* 0x0040 */
  /* end particle-defined fields */
  int16_t           time;                     /* 0x005C */
  /* lower value means more important */
  uint8_t           priority;                 /* 0x005E */
  uint8_t           part_id;                  /* 0x005F */
                                              /* 0x0060 */
};

typedef struct
{
  int16_t           poly_idx;                 /* 0x0000 */
  uint16_t          list_next;                /* 0x0002 */
                                              /* 0x0004 */
} z64_col_list_t;

typedef struct
{
  uint16_t          floor_list_idx;           /* 0x0000 */
  uint16_t          wall_list_idx;            /* 0x0002 */
  uint16_t          ceil_list_idx;            /* 0x0004 */
                                              /* 0x0006 */
} z64_col_lut_t;

typedef struct
{
  z64_actor_t      *actor;                    /* 0x0000 */
  z64_col_hdr_t    *col_hdr;                  /* 0x0004 */
  uint16_t          poly_idx;                 /* 0x0008 */
  uint16_t          ceil_list_idx;            /* 0x000A */
  uint16_t          wall_list_idx;            /* 0x000C */
  uint16_t          floor_list_idx;           /* 0x000E */
  uint16_t          vtx_idx;                  /* 0x0010 */
  char              pad_0x12[0x0002];         /* 0x0012 */
  z64_xyzf_t        scale_1;                  /* 0x0014 */
  z64_xyz_t         rot_1;                    /* 0x0020 */
  char              pad_0x26[0x0002];         /* 0x0026 */
  z64_xyzf_t        pos_1;                    /* 0x0028 */
  z64_xyzf_t        scale_2;                  /* 0x0034 */
  z64_xyz_t         rot_2;                    /* 0x0040 */
  char              pad_0x46[0x0002];         /* 0x0046 */
  z64_xyzf_t        pos_2;                    /* 0x0048 */
  int16_t           h_0x54;                   /* 0x0054 */
  int16_t           h_0x56;                   /* 0x0056 */
  int16_t           h_0x58;                   /* 0x0058 */
  int16_t           h_0x5A;                   /* 0x005A */
  float             f_0x5C;                   /* 0x005C */
  float             f_0x60;                   /* 0x0060 */
                                              /* 0x0064 */
} z64_dyn_col_t;

typedef struct
{
  /* static collision stuff */
  z64_col_hdr_t    *col_hdr;                  /* 0x0000 */
  z64_xyzf_t        bbox_min;                 /* 0x0004 */
  z64_xyzf_t        bbox_max;                 /* 0x0010 */
  int               n_sect_x;                 /* 0x001C */
  int               n_sect_y;                 /* 0x0020 */
  int               n_sect_z;                 /* 0x0024 */
  z64_xyzf_t        sect_size;                /* 0x0028 */
  z64_xyzf_t        sect_inv;                 /* 0x0034 */
  z64_col_lut_t    *stc_lut;                  /* 0x0040 */
  uint16_t          stc_list_max;             /* 0x0044 */
  uint16_t          stc_list_pos;             /* 0x0046 */
  z64_col_list_t   *stc_list;                 /* 0x0048 */
  uint8_t          *stc_check;                /* 0x004C */
  /* dynamic collision stuff */
  uint8_t           unk_flags_0x50;           /* 0x0050 */
  char              unk_0x51[0x0003];         /* 0x0051 */
  z64_dyn_col_t     dyn_col[50];              /* 0x0054 */
  union
  {
    uint16_t        data;
    struct
    {
      uint16_t      unk_00              : 14;
      uint16_t      destroy             : 1;
      uint16_t      active              : 1;
    };
  }                 dyn_flags[50];            /* 0x13DC */
  z64_col_poly_t   *dyn_poly;                 /* 0x1440 */
  z64_xyz_t        *dyn_vtx;                  /* 0x1444 */
  z64_col_list_t   *dyn_list;                 /* 0x1448 */
  uint32_t          n_dyn_list;               /* 0x144C */
  char              unk_0x1450[0x0004];       /* 0x1450 */
  uint32_t          dyn_list_max;             /* 0x1454 */
  uint32_t          dyn_poly_max;             /* 0x1458 */
  uint32_t          dyn_vtx_max;              /* 0x145C */
  char              unk_0x1460[0x0004];       /* 0x1460 */
                                              /* 0x1464 */
} z64_col_ctxt_t;

enum
{
  Z64_HIT_SPH_LIST,
  Z64_HIT_CYL,
  Z64_HIT_TRI_LIST,
  Z64_HIT_QUAD,
};

typedef struct
{
  z64_actor_t      *actor;                    /* 0x0000 */
  char              unk_0x4[0x0011];          /* 0x0004 */
  uint8_t           type;                     /* 0x0015 */
  char              unk_0x16[0x0002];         /* 0x0016 */
                                              /* 0x0018 */
} z64_hit_t;

typedef struct
{
  char              unk_0x0[0x0028];          /* 0x0000 */
  z64_xyz_t         xyz_0x28;                 /* 0x0028 */
  int16_t           h_0x2E;                   /* 0x002E */
  struct
  {
    z64_xyz_t       pos;                      /* 0x0030 */
    int16_t         radius;                   /* 0x0036 */
  };
  /* not used by hit tests */
  float             f_0x38;                   /* 0x0038 */
  uint8_t           b_0x3C;                   /* 0x003C */
  char              unk_0x3D[0x0003];         /* 0x003D */
                                              /* 0x0040 */
} z64_hit_sph_ent_t;

typedef struct
{
  z64_hit_t         base;                     /* 0x0000 */
  int32_t           n_ent;                    /* 0x0018 */
  z64_hit_sph_ent_t*ent_list;                 /* 0x001C */
                                              /* 0x0020 */
} z64_hit_sph_list_t;

typedef struct
{
  z64_hit_t         base;                     /* 0x0000 */
  char              unk_0x18[0x0028];         /* 0x0018 */
  int16_t           radius;                   /* 0x0040 */
  int16_t           height;                   /* 0x0042 */
  /* the y coordinate is offset by this during hit tests */
  int16_t           y_offset;                 /* 0x0044 */
  /* the origin is on the bottom center of the cylinder */
  z64_xyz_t         pos;                      /* 0x0046 */
                                              /* 0x004C */
} z64_hit_cyl_t;

typedef struct
{
  char              unk_0x0[0x0028];          /* 0x0000 */
  z64_xyzf_t        v[3];                     /* 0x0028 */
  char              unk_0x4C[0x0010];         /* 0x004C */
                                              /* 0x005C */
} z64_hit_tri_ent_t;

typedef struct
{
  z64_hit_t         base;                     /* 0x0000 */
  int32_t           n_ent;                    /* 0x0018 */
  z64_hit_tri_ent_t*ent_list;                 /* 0x001C */
                                              /* 0x0020 */
} z64_hit_tri_list_t;

typedef struct
{
  z64_hit_t         base;                     /* 0x0000 */
  char              unk_0x18[0x0028];         /* 0x0018 */
  z64_xyzf_t        v[4];                     /* 0x0040 */
  char              unk_0x70[0x000C];         /* 0x0070 */
                                              /* 0x007C */
} z64_hit_quad_t;

typedef struct
{
  int16_t           n_at;                     /* 0x0000 */
  uint16_t          hz_0x2;                   /* 0x0002 */
  z64_hit_t        *at_list[50];              /* 0x0004 */
  int32_t           n_ac;                     /* 0x00CC */
  z64_hit_t        *ac_list[60];              /* 0x00D0 */
  int32_t           n_oc;                     /* 0x01C0 */
  z64_hit_t        *oc_list[50];              /* 0x01C4 */
                                              /* 0x028C */
} z64_hit_ctxt_t;

typedef struct
{
  /* vrom addresses */
  uint32_t          tex_start;                /* 0x0000 */
  uint32_t          tex_end;                  /* 0x0004 */
  uint32_t          pal_start;                /* 0x0008 */
  uint32_t          pal_end;                  /* 0x000C */
                                              /* 0x0010 */
} z64_sky_image_t;

typedef struct
{
  char              unk_0x0[0x0128];          /* 0x0000 */
  char             *textures[2];              /* 0x0128 */
  char             *palettes;                 /* 0x0130 */
  Gfx              *gfx;                      /* 0x0134 */
  char              unk_0x138[0x0004];        /* 0x0138 */
  char             *vtx;                      /* 0x013C */
  int16_t           mode;                     /* 0x0140 */
  char              unk_0x142[0x0006];        /* 0x0142 */
  float             f_0x148;                  /* 0x0148 */
  char              unk_0x14C[0x0004];        /* 0x014C */
                                              /* 0x0150 */
} z64_sky_ctxt_t;

typedef struct
{
  /* file loading params */
  uint32_t          vrom_addr;                /* 0x0000 */
  void             *dram_addr;                /* 0x0004 */
  uint32_t          size;                     /* 0x0008 */
  /* debug stuff */
  char             *filename;                 /* 0x000C */
  int32_t           line;                     /* 0x0010 */
  int32_t           unk_0x14;                 /* 0x0014 */
  /* completion notification params */
  OSMesgQueue      *notify_mq;                /* 0x0018 */
  OSMesg            notify_msg;               /* 0x001C */
                                              /* 0x0020 */
} z64_getfile_t;

typedef struct
{
  int16_t           id;                       /* 0x0000 */
  char              pad_0x2[0x0002];          /* 0x0002 */
  void             *data;                     /* 0x0004 */
  z64_getfile_t     getfile;                  /* 0x0008 */
  OSMesgQueue       load_mq;                  /* 0x0028 */
  OSMesg            load_m;                   /* 0x0040 */
                                              /* 0x0044 */
} z64_mem_obj_t;

typedef struct
{
  void             *obj_space_start;          /* 0x0000 */
  void             *obj_space_end;            /* 0x0004 */
  uint8_t           n_objects;                /* 0x0008 */
  uint8_t           n_special;                /* 0x0009 */
  uint8_t           keep_index;               /* 0x000A */
  uint8_t           skeep_index;              /* 0x000B */
  z64_mem_obj_t     objects[19];              /* 0x000C */
                                              /* 0x0518 */
} z64_obj_ctxt_t;

typedef struct
{
  /* decides which draw function to use (0-2) */
  uint8_t           mode;                     /* 0x0000 */
  int8_t            n_entries;                /* 0x0001 */
  char              pad_0x2[0x0002];          /* 0x0002 */
  uint32_t          seg_start;                /* 0x0004 */
  uint32_t          seg_end;                  /* 0x0008 */
                                              /* 0x000C */
} z64_sr_mesh_t;

typedef struct
{
  int8_t            room_idx_1;               /* 0x0000 */
  int8_t            effect_1;                 /* 0x0001 */
  int8_t            room_idx_2;               /* 0x0002 */
  int8_t            effect_2;                 /* 0x0003 */
  int16_t           actor_id;                 /* 0x0004 */
  z64_xyz_t         pos;                      /* 0x0006 */
  z64_angle_t       rot;                      /* 0x000C */
  int16_t           variable;                 /* 0x000E */
                                              /* 0x0010 */
} z64_tnsn_actor_t;

typedef struct
{
  int8_t            index;                    /* 0x0000 */
  char              unk_0x1[0x0001];          /* 0x0001 */
  uint8_t           bhv_z;                    /* 0x0002 */
  uint8_t           bhv_x;                    /* 0x0003 */
  uint8_t           echo;                     /* 0x0004 */
  int8_t            show_invis_act;           /* 0x0005 */
  char              pad_0x6[0x0002];          /* 0x0006 */
  z64_sr_mesh_t    *mesh_hdr;                 /* 0x0008 */
  void             *file;                     /* 0x000C */
  char              unk_0x10[0x0004];         /* 0x0010 */
                                              /* 0x0014 */
} z64_room_t;

typedef struct
{
  /* room info */
  z64_room_t        rooms[2];                 /* 0x0000 */
  void             *room_space_start;         /* 0x0028 */
  void             *room_space_end;           /* 0x002C */
  /* loading info */
  uint8_t           load_slot;                /* 0x0030 */
  int8_t            load_active;              /* 0x0031 */
  char              pad_0x32[0x0002];         /* 0x0032 */
  void             *load_ptr;                 /* 0x0034 */
  z64_getfile_t     load_getfile;             /* 0x0038 */
  OSMesgQueue       load_notify_mq;           /* 0x0058 */
  OSMesg            load_notify_msg;          /* 0x0070 */
  /* may also be used for other things, not sure */
  /* handled by the scene config for lost woods */
  int16_t           cojiro_sfx_played;        /* 0x0074 */
  int16_t           cojiro_sfx_timer;         /* 0x0076 */
  /* transition actor list */
  uint8_t           n_tnsn;                   /* 0x0078 */
  char              pad_0x79[0x0003];         /* 0x0079 */
  z64_tnsn_actor_t *tnsn_list;                /* 0x007C */
                                              /* 0x0080 */
} z64_room_ctxt_t;

/* interface context */
typedef struct
{
  char              unk_0x0[0x0130];          /* 0x0000 */
  char             *parameter;                /* 0x0130 */
  char             *action_texture;           /* 0x0134 */
  char             *item_texture;             /* 0x0138 */
  char             *minimap_texture;          /* 0x013C */
  char              unk_0x140[0x00AC];        /* 0x0140 */
  uint16_t          h_0x1EC;                  /* 0x01EC */
  char              unk_0x1EE[0x0002];        /* 0x01EE */
  uint16_t          a_action;                 /* 0x01F0 */
  char              unk_0x1F2[0x0002];        /* 0x01F2 */
  float             f_0x1F4;                  /* 0x01F4 */
  char              unk_0x1F8[0x0004];        /* 0x01F8 */
  int16_t           b_label;                  /* 0x01FC */
  char              unk_0x1FE[0x0064];        /* 0x01FE */
  struct
  {
    uint8_t         unk_0x0;
    uint8_t         b_button;
    uint8_t         unk_0x2;
    uint8_t         bottles;
    uint8_t         trade_items;
    uint8_t         hookshot;
    uint8_t         ocarina;
    uint8_t         warp_songs;
    uint8_t         suns_song;
    uint8_t         farores_wind;
    uint8_t         dfnl;
    uint8_t         all;
  }                 restriction_flags;        /* 0x0262 */
  char              unk_0x26E[0x0002];        /* 0x026E */
                                              /* 0x0270 */
} z64_if_ctxt_t;

typedef struct
{
  char              unk_0x0[0x0128];          /* 0x0000 */
  void             *icon_item;                /* 0x0128 */
  void             *icon_item_24;             /* 0x012C */
  void             *icon_item_s;              /* 0x0130 */
  void             *icon_item_lang;           /* 0x0134 */
  void             *name_texture;             /* 0x0138 */
  void             *p_0x13C;                  /* 0x013C */
  char              unk_0x0140[0x0094];       /* 0x0140 */
  uint16_t          state;                    /* 0x01D4 */
  char              unk_0x1D6[0x0012];        /* 0x01D6 */
  uint16_t          screen_idx;               /* 0x01E8 */
  char              unk_0x1EA[0x002E];        /* 0x01EA */
  int16_t           item_cursor;              /* 0x0218 */
  char              unk_0x21A[0x0002];        /* 0x021A */
  int16_t           quest_cursor;             /* 0x021C */
  int16_t           equip_cursor;             /* 0x021E */
  int16_t           map_cursor;               /* 0x0220 */
  int16_t           item_x;                   /* 0x0222 */
  char              unk_0x224[0x0004];        /* 0x0224 */
  int16_t           equipment_x;              /* 0x0228 */
  char              unk_0x22A[0x0002];        /* 0x022A */
  int16_t           item_y;                   /* 0x022C */
  char              unk_0x22E[0x0004];        /* 0x022E */
  int16_t           equipment_y;              /* 0x0232 */
  char              unk_0x234[0x0004];        /* 0x0234 */
  int16_t           cursor_pos;               /* 0x0238 */
  char              unk_0x23A[0x0002];        /* 0x023A */
  int16_t           item_id;                  /* 0x023C */
  int16_t           item_item;                /* 0x023E */
  int16_t           map_item;                 /* 0x0240 */
  int16_t           quest_item;               /* 0x0242 */
  int16_t           equip_item;               /* 0x0244 */
  char              unk_0x246[0x0004];        /* 0x0246 */
  int16_t           quest_hilite;             /* 0x024A */
  char              unk_0x24C[0x0018];        /* 0x024C */
  int16_t           quest_song;               /* 0x0264 */
  char              unk_0x266[0x0016];        /* 0x0266 */
  /* unknown structure */
  char              s_0x27C[0x0038];          /* 0x027C */
                                              /* 0x02B4 */
} z64_pause_ctxt_t;

/* lighting structs */
typedef struct
{
  int8_t            dir[3];                   /* 0x0000 */
  uint8_t           col[3];                   /* 0x0003 */
                                              /* 0x0006 */
} z64_light1_t;

typedef struct
{
  int16_t           x;                        /* 0x0000 */
  int16_t           y;                        /* 0x0002 */
  int16_t           z;                        /* 0x0004 */
  uint8_t           col[3];                   /* 0x0006 */
  char              pad_0x9[0x0001];          /* 0x0009 */
  int16_t           intensity;                /* 0x000A */
                                              /* 0x000C */
} z64_light2_t;

typedef union
{
  z64_light1_t      light1;                   /* 0x0000 */
  z64_light2_t      light2;                   /* 0x0000 */
                                              /* 0x000C */
} z64_lightn_t;

typedef struct
{
  uint8_t           type;                     /* 0x0000 */
  char              pad_0x1[0x0001];          /* 0x0001 */
  z64_lightn_t      lightn;                   /* 0x0002 */
                                              /* 0x000E */
} z64_light_t;

typedef struct z64_light_node_s z64_light_node_t;
struct z64_light_node_s
{
  z64_light_t      *light;                    /* 0x0000 */
  z64_light_node_t *prev;                     /* 0x0004 */
  z64_light_node_t *next;                     /* 0x0008 */
                                              /* 0x000C */
};

typedef struct
{
  uint32_t          n_nodes;                  /* 0x0000 */
  uint32_t          current_node;             /* 0x0004 */
  z64_light_node_t  nodes[32];                /* 0x0008 */
                                              /* 0x0188 */
} z64_light_queue_t;

typedef struct
{
  z64_light_node_t *light_list;               /* 0x0000 */
  uint8_t           ambient[3];               /* 0x0004 */
  uint8_t           fog[3];                   /* 0x0007 */
  int16_t           fog_position;             /* 0x000A */
  int16_t           draw_distance;            /* 0x000C */
  char              pad_0xE[0x0002];          /* 0x000E */
                                              /* 0x0010 */
} z64_lighting_t;

typedef struct
{
  int8_t            numlights;                /* 0x0000 */
  char              pad_0x1[0x0007];          /* 0x0001 */
  Lightsn           lites;                    /* 0x0008 */
                                              /* 0x0080 */
} z64_gbi_lights_t;

typedef void (*z64_light_handler_t)(z64_gbi_lights_t *, z64_lightn_t *,
                                    z64_actor_t *);

typedef struct
{
  int8_t            numpoints;
  z64_xyz_t        *points;
} z64_path_t;

/* game context */
typedef struct
{
  z64_ctxt_t        common;                   /* 0x00000 */
  uint16_t          scene_index;              /* 0x000A4 */
  char              unk_0xA6[0x000A];         /* 0x000A6 */
  void             *scene_file;               /* 0x000B0 */
  char              unk_0xB4[0x0004];         /* 0x000B4 */
  z64_view_t        view;                     /* 0x000B8 */
  char              unk_0xE0[0x0090];         /* 0x001E0 */
  z64_actor_t      *camera_focus;             /* 0x00270 */
  char              unk_0x274[0x00AE];        /* 0x00274 */
  uint16_t          camera_mode;              /* 0x00322 */
  char              unk_0x324[0x001A];        /* 0x00324 */
  uint16_t          camera_flag_1;            /* 0x0033E */
  char              unk_0x340[0x016C];        /* 0x00340 */
  int16_t           event_flag;               /* 0x004AC */
  char              unk_0x4AE[0x02E2];        /* 0x004AE */
  z64_camera_t     *camera_ptrs[4];           /* 0x00790 */
  uint16_t          active_camera;            /* 0x007A0 */
  uint16_t          next_camera;              /* 0x007A2 */
  uint8_t           seq_idx;                  /* 0x007A4 */
  uint8_t           night_sfx;                /* 0x007A5 */
  char              unk_0x7A6[0x0002];        /* 0x007A6 */
  z64_lighting_t    lighting;                 /* 0x007A8 */
  char              unk_0x7B8[0x0008];        /* 0x007B8 */
  z64_col_ctxt_t    col_ctxt;                 /* 0x007C0 */
  char              actor_ctxt[0x0008];       /* 0x01C24 */
  uint8_t           n_actors_loaded;          /* 0x01C2C */
  char              unk_0x1C2D[0x0003];       /* 0x01C2D */
  struct
  {
    uint32_t        length;
    z64_actor_t    *first;
  }                 actor_list[12];           /* 0x01C30 */
  char              unk_0x1C90[0x0038];       /* 0x01C90 */
  z64_actor_t      *arrow_actor;              /* 0x01CC8 */
  z64_actor_t      *target_actor;             /* 0x01CCC */
  char              unk_0x1CD0[0x0058];       /* 0x01CD0 */
  uint32_t          swch_flags;               /* 0x01D28 */
  uint32_t          temp_swch_flags;          /* 0x01D2C */
  uint32_t          unk_flags_0;              /* 0x01D30 */
  uint32_t          unk_flags_1;              /* 0x01D34 */
  uint32_t          chest_flags;              /* 0x01D38 */
  uint32_t          clear_flags;              /* 0x01D3C */
  uint32_t          temp_clear_flags;         /* 0x01D40 */
  uint32_t          collect_flags;            /* 0x01D44 */
  uint32_t          temp_collect_flags;       /* 0x01D48 */
  void             *title_card_texture;       /* 0x01D4C */
  char              unk_0x1D50[0x0007];       /* 0x01D50 */
  uint8_t           title_card_delay;         /* 0x01D57 */
  char              unk_0x1D58[0x0010];       /* 0x01D58 */
  void             *cutscene_ptr;             /* 0x01D68 */
  int8_t            cutscene_state;           /* 0x01D6C */
  char              unk_0x1D6D[0x020B];       /* 0x01D6D */
  z64_sky_ctxt_t    sky_ctxt;                 /* 0x01F78 */
  char              unk_0x20C8[0xE2C0];       /* 0x020C8 */
  char             *message_texture;          /* 0x10388 */
  char              unk_0x1038C[0x0049];      /* 0x1038C */
  uint8_t           message_type;             /* 0x103D5 */
  char              unk_0x103D6[0x0006];      /* 0x103D6 */
  uint8_t           message_state_1;          /* 0x103DC */
  char              pad_0x103DD[0x0001];      /* 0x103DD */
  union
  {
    uint8_t         message_data_e[200];
    uint16_t        message_data_j[100];
  };                                          /* 0x103DE */
  char              unk_0x104A6[0x0016];      /* 0x104A6 */
  uint8_t           message_state_2;          /* 0x104BC */
  char              unk_0x104BD[0x0002];      /* 0x104BD */
  uint8_t           message_state_3;          /* 0x104BF */
  char              unk_0x104C0[0x0030];      /* 0x104C0 */
  z64_if_ctxt_t     if_ctxt;                  /* 0x104F0 */
  z64_pause_ctxt_t  pause_ctxt;               /* 0x10760 */
  char              unk_0x10A14[0x000C];      /* 0x10A14 */
  uint16_t          death_state;              /* 0x10A20 */
  char              unk_0x10A22[0x0012];      /* 0x10A22 */
  uint8_t           sky_image_idx[2];         /* 0x10A34 */
  char              unk_0x10A36[0x00CE];      /* 0x10A36 */
  uint8_t           day_phase;                /* 0x10B04 */
  char              unk_0x10B05[0x000D];      /* 0x10B05 */
  uint8_t           rain_effect_1;            /* 0x10B12 */
  uint8_t           rain_level;               /* 0x10B13 */
  char              unk_0x10B14[0x0002];      /* 0x10B14 */
  uint8_t           rain_effect_2;            /* 0x10B16 */
  char              unk_0x10B17[0x0C8D];      /* 0x10B17 */
  z64_obj_ctxt_t    obj_ctxt;                 /* 0x117A4 */
  z64_room_ctxt_t   room_ctxt;                /* 0x11CBC */
  char              unk_0x11D3C[0x0024];      /* 0x11D3C */
  MtxF              mf_11D60;                 /* 0x11D60 */
  MtxF              mf_11DA0;                 /* 0x11DA0 */
  char              unk_0x11DE0[0x0004];      /* 0x11DE0 */
  uint32_t          gameplay_frames;          /* 0x11DE4 */
  uint8_t           link_age;                 /* 0x11DE8 */
  char              unk_0x11DE9;              /* 0x11DE9 */
  uint8_t           spawn_index;              /* 0x11DEA */
  uint8_t           n_map_actors;             /* 0x11DEB */
  uint8_t           n_rooms;                  /* 0x11DEC */
  char              unk_0x11DED[0x0003];      /* 0x11DED */
  z64_vrom_file_t  *room_list;                /* 0x11DF0 */
  char              unk_0x11DF4[0x0004];      /* 0x11DF4 */
  void             *map_actor_list;           /* 0x11DF8 */
  char              unk_0x11DFC[0x0008];      /* 0x11DFC */
  void             *scene_exit_list;          /* 0x11E04 */
  z64_path_t       *path_list;                /* 0x11E08 */
  void             *elf_message;              /* 0x11E0C */
  char              unk_0x11E10[0x0004];      /* 0x11E10 */
  uint8_t           skybox_type;              /* 0x11E14 */
  int8_t            scene_load_flag;          /* 0x11E15 */
  char              unk_0x11E16[0x0004];      /* 0x11E16 */
  int16_t           entrance_index;           /* 0x11E1A */
  char              unk_0x11E1C[0x0042];      /* 0x11E1C */
  uint8_t           fadeout_transition;       /* 0x11E5E */
  char              unk_0x11E5F[0x0001];      /* 0x11E5F */
  z64_hit_ctxt_t    hit_ctxt;                 /* 0x11E60 */
  char              unk_0x120EC[0x042C];      /* 0x120EC */
                                              /* 0x12518 */
} z64_game_t;

typedef void (*z64_scene_config_t)(z64_game_t *game);

/* static particle effects */
typedef struct
{
  /* velocity */
  float             vel_x;                    /* 0x0000 */
  float             vel_y;                    /* 0x0004 */
  float             vel_z;                    /* 0x0008 */
  /* position */
  float             x;                        /* 0x000C */
  float             y;                        /* 0x0010 */
  float             z;                        /* 0x0014 */
  /* integer xyz velocity-position pair, unused ? */
  int16_t           h_0x18;                   /* 0x0018 */
  int16_t           h_0x1A;                   /* 0x001A */
  int16_t           h_0x1C;                   /* 0x001C */
  int16_t           h_0x1E;                   /* 0x001E */
  int16_t           h_0x20;                   /* 0x0020 */
  int16_t           h_0x22;                   /* 0x0022 */
                                              /* 0x0024 */
} z64_dot_cp_t;

typedef struct
{
  /* initial position */
  int16_t           x;                        /* 0x0000 */
  int16_t           y;                        /* 0x0002 */
  int16_t           z;                        /* 0x0004 */
  char              pad_0x6[0x0002];          /* 0x0006 */
  /* number of active control points */
  uint32_t          n_cp;                     /* 0x0008 */
  /* control points */
  z64_dot_cp_t      cp[32];                   /* 0x000C */
  /* initial linear velocity */
  float             vel;                      /* 0x048C */
  /* vertical acceleration */
  float             accel_y;                  /* 0x0490 */
  /* initial number of controls points (n_cp_a * n_cp_b + 2) */
  uint32_t          n_cp_a;                   /* 0x0494 */
  uint32_t          n_cp_b;                   /* 0x0498 */
  /* primary color 1 */
  uint8_t           c1r1;                     /* 0x049C */
  uint8_t           c1g1;                     /* 0x049D */
  uint8_t           c1b1;                     /* 0x049E */
  uint8_t           c1a1;                     /* 0x049F */
  /* secondary color 1 */
  uint8_t           c2r1;                     /* 0x04A0 */
  uint8_t           c2g1;                     /* 0x04A1 */
  uint8_t           c2b1;                     /* 0x04A2 */
  uint8_t           c2a1;                     /* 0x04A3 */
  /* tertiary color 1 */
  uint8_t           c3r1;                     /* 0x04A4 */
  uint8_t           c3g1;                     /* 0x04A5 */
  uint8_t           c3b1;                     /* 0x04A6 */
  uint8_t           c3a1;                     /* 0x04A7 */
  /* quaternary color 1 */
  uint8_t           c4r1;                     /* 0x04A8 */
  uint8_t           c4g1;                     /* 0x04A9 */
  uint8_t           c4b1;                     /* 0x04AA */
  uint8_t           c4a1;                     /* 0x04AB */
  /* primary color 2 */
  uint8_t           c1r2;                     /* 0x04AC */
  uint8_t           c1g2;                     /* 0x04AD */
  uint8_t           c1b2;                     /* 0x04AE */
  uint8_t           c1a2;                     /* 0x04AF */
  /* secondary color 2 */
  uint8_t           c2r2;                     /* 0x04B0 */
  uint8_t           c2g2;                     /* 0x04B1 */
  uint8_t           c2b2;                     /* 0x04B2 */
  uint8_t           c2a2;                     /* 0x04B3 */
  /* tertiary color 2 */
  uint8_t           c3r2;                     /* 0x04B4 */
  uint8_t           c3g2;                     /* 0x04B5 */
  uint8_t           c3b2;                     /* 0x04B6 */
  uint8_t           c3a2;                     /* 0x04B7 */
  /* quaternary color 2 */
  uint8_t           c4r2;                     /* 0x04B8 */
  uint8_t           c4g2;                     /* 0x04B9 */
  uint8_t           c4b2;                     /* 0x04BA */
  uint8_t           c4a2;                     /* 0x04BB */
  /* elapsed time and total duration */
  int32_t           time;                     /* 0x04BC */
  int32_t           duration;                 /* 0x04C0 */
                                              /* 0x04C4 */
} z64_dot_fx_t;

typedef struct
{
  uint8_t           active;                   /* 0x0000 */
  uint8_t           b_0x1;                    /* 0x0001 */
  uint8_t           b_0x2;                    /* 0x0002 */
  char              pad_0x3[0x0001];          /* 0x0003 */
  z64_dot_fx_t      fx;                       /* 0x0004 */
                                              /* 0x04C8 */
} z64_dot_t;

typedef struct
{
  /* state ? */
  int32_t           state;                    /* 0x0000 */
  /* elapsed time */
  int32_t           time;                     /* 0x0004 */
  /* point 1 */
  int16_t           p1x;                      /* 0x0008 */
  int16_t           p1y;                      /* 0x000A */
  int16_t           p1z;                      /* 0x000C */
  /* point 2 */
  int16_t           p2x;                      /* 0x000E */
  int16_t           p2y;                      /* 0x0010 */
  int16_t           p2z;                      /* 0x0012 */
  /* flags ? */
  uint16_t          flags;                    /* 0x0014 */
  char              pad_0x16[0x0002];         /* 0x0016 */
                                              /* 0x0018 */
} z64_trail_cp_t;

typedef struct
{
  /* control points */
  z64_trail_cp_t    cp[16];                   /* 0x0000 */
  /* interpolation mode ? */
  uint32_t          ipn_mode;                 /* 0x0180 */
  /* parameter for interpolation mode 4 */
  float             f_0x184;                  /* 0x0184 */
  /* flags ? */
  uint16_t          h_0x188;                  /* 0x0188 */
  /* counter increment, counter, what for ? */
  int16_t           h_0x18A;                  /* 0x018A */
  int16_t           h_0x18C;                  /* 0x018C */
  /* point 1 starting color */
  uint8_t           p1r1;                     /* 0x018E */
  uint8_t           p1g1;                     /* 0x018F */
  uint8_t           p1b1;                     /* 0x0190 */
  uint8_t           p1a1;                     /* 0x0191 */
  /* point 2 starting color */
  uint8_t           p2r1;                     /* 0x0192 */
  uint8_t           p2g1;                     /* 0x0193 */
  uint8_t           p2b1;                     /* 0x0194 */
  uint8_t           p2a1;                     /* 0x0195 */
  /* point 1 ending color */
  uint8_t           p1r2;                     /* 0x0196 */
  uint8_t           p1g2;                     /* 0x0197 */
  uint8_t           p1b2;                     /* 0x0198 */
  uint8_t           p1a2;                     /* 0x0199 */
  /* point 2 ending color */
  uint8_t           p2r2;                     /* 0x019A */
  uint8_t           p2g2;                     /* 0x019B */
  uint8_t           p2b2;                     /* 0x019C */
  uint8_t           p2a2;                     /* 0x019D */
  /* number of active control points */
  uint8_t           n_cp;                     /* 0x019E */
  /* control point duration */
  uint8_t           duration;                 /* 0x019F */
  /* unknown */
  uint8_t           b_0x1A0;                  /* 0x01A0 */
  /* render mode */
  /* 0:   simple */
  /* 1:   simple with alternate colors */
  /* 2+:  smooth */
  uint8_t           mode;                     /* 0x01A1 */
  /* alternate colors */
  /* inner color */
  uint8_t           m1r1;                     /* 0x01A2 */
  uint8_t           m1g1;                     /* 0x01A3 */
  uint8_t           m1b1;                     /* 0x01A4 */
  uint8_t           m1a1;                     /* 0x01A5 */
  /* outer color */
  uint8_t           m1r2;                     /* 0x01A6 */
  uint8_t           m1g2;                     /* 0x01A7 */
  uint8_t           m1b2;                     /* 0x01A8 */
  uint8_t           m1a2;                     /* 0x01A9 */
  char              pad_0x1AA[0x0002];        /* 0x01AA */
                                              /* 0x01AC */
} z64_trail_fx_t;

typedef struct
{
  uint8_t           active;                   /* 0x0000 */
  char              pad_0x1[0x0003];          /* 0x0001 */
  z64_trail_fx_t    fx;                       /* 0x0004 */
                                              /* 0x01B0 */
} z64_trail_t;

typedef struct
{
  /* initial velocity */
  float             vel;                      /* 0x0000 */
  /* point 1 velocity and distance */
  float             p1v;                      /* 0x0004 */
  float             p1d;                      /* 0x0008 */
  /* point 2 velocity and distance */
  float             p2v;                      /* 0x000C */
  float             p2d;                      /* 0x0010 */
  /* orientation */
  int16_t           yaw;                      /* 0x0014 */
  int16_t           pitch;                    /* 0x0016 */
                                              /* 0x0018 */
} z64_spark_cp_t;

typedef struct
{
  /* control points */
  z64_spark_cp_t    cp[16];                   /* 0x0000 */
  /* number of active control points */
  uint8_t           n_cp;                     /* 0x0180 */
  char              pad_0x181[0x0001];        /* 0x0181 */
  /* position */
  int16_t           x;                        /* 0x0182 */
  int16_t           y;                        /* 0x0184 */
  int16_t           z;                        /* 0x0186 */
  /* primary color 1 */
  uint8_t           c1r1;                     /* 0x0188 */
  uint8_t           c1g1;                     /* 0x0189 */
  uint8_t           c1b1;                     /* 0x018A */
  uint8_t           c1a1;                     /* 0x018B */
  /* secondary color 1 */
  uint8_t           c2r1;                     /* 0x018C */
  uint8_t           c2g1;                     /* 0x018D */
  uint8_t           c2b1;                     /* 0x018E */
  uint8_t           c2a1;                     /* 0x018F */
  /* primary color 2 */
  uint8_t           c1r2;                     /* 0x0190 */
  uint8_t           c1g2;                     /* 0x0191 */
  uint8_t           c1b2;                     /* 0x0192 */
  uint8_t           c1a2;                     /* 0x0193 */
  /* secondary color 2 */
  uint8_t           c2r2;                     /* 0x0194 */
  uint8_t           c2g2;                     /* 0x0195 */
  uint8_t           c2b2;                     /* 0x0196 */
  uint8_t           c2a2;                     /* 0x0197 */
  /* primary color 3 */
  uint8_t           c1r3;                     /* 0x0198 */
  uint8_t           c1g3;                     /* 0x0199 */
  uint8_t           c1b3;                     /* 0x019A */
  uint8_t           c1a3;                     /* 0x019B */
  /* secondary color 3 */
  uint8_t           c2r3;                     /* 0x019C */
  uint8_t           c2g3;                     /* 0x019D */
  uint8_t           c2b3;                     /* 0x019E */
  uint8_t           c2a3;                     /* 0x019F */
  /* deceleration of point velocities */
  float             decel;                    /* 0x01A0 */
  char              unk_0x1A4[0x0004];        /* 0x01A4 */
  /* initial velocity range */
  float             vel_max;                  /* 0x01A8 */
  float             vel_min;                  /* 0x01AC */
  /* total duration and elapsed time */
  uint8_t           duration;                 /* 0x01B0 */
  uint8_t           time;                     /* 0x01B1 */
  /* light */
  z64_light_t       light;                    /* 0x01B2 */
  z64_light_node_t *light_node;               /* 0x01C0 */
  /* reduces light intensity by half each frame when set to 1 */
  int32_t           decay;                    /* 0x01C4 */
                                              /* 0x01C8 */
} z64_spark_fx_t;

typedef struct
{
  uint8_t           active;                   /* 0x0000 */
  uint8_t           b_0x1;                    /* 0x0001 */
  uint8_t           b_0x2;                    /* 0x0002 */
  char              pad_0x3[0x0001];          /* 0x0003 */
  z64_spark_fx_t    fx;                       /* 0x0004 */
                                              /* 0x01CC */
} z64_spark_t;

typedef struct
{
  z64_game_t       *game;                     /* 0x0000 */
  z64_dot_t         dots[3];                  /* 0x0004 */
  z64_trail_t       trails[25];               /* 0x0E5C */
  z64_spark_t       sparks[3];                /* 0x388C */
                                              /* 0x3DF0 */
} z64_pfx_t;

/* high-level audio control structures (separate from afx) */
typedef struct
{
  /* volume effect state */
  float             vs_current;               /* 0x0000 */
  float             vs_target;                /* 0x0004 */
  float             vs_delta;                 /* 0x0008 */
  uint16_t          vs_time;                  /* 0x000C */
  char              pad_0xE[0x0002];          /* 0x000E */
  /* pitch effect state */
  float             ps_current;               /* 0x0010 */
  float             ps_target;                /* 0x0014 */
  float             ps_delta;                 /* 0x0018 */
  uint16_t          ps_time;                  /* 0x001C */
  char              pad_0x1E[0x0002];         /* 0x001E */
                                              /* 0x0020 */
} z64_chan_ctl_t;

typedef struct
{
  /* volume effect state */
  float             vs_current;               /* 0x0000 */
  float             vs_target;                /* 0x0004 */
  float             vs_delta;                 /* 0x0008 */
  uint16_t          vs_time;                  /* 0x000C */
  /* volume effect parameters (for starting a volume effect) */
  uint8_t           vp_factors[4];            /* 0x000E */
  uint8_t           vp_time;                  /* 0x0012 */
  uint8_t           vp_start;                 /* 0x0013 */
  /* pitch effect parameters (ditto) */
  uint32_t          pp_bits;                  /* 0x0014 */
  uint16_t          pp_unk;                   /* 0x0018 */
  char              pad_0x1A[0x0002];         /* 0x001A */
  /* pitch effect state */
  float             ps_current;               /* 0x001C */
  float             ps_target;                /* 0x0020 */
  float             ps_delta;                 /* 0x0024 */
  uint16_t          ps_time;                  /* 0x0028 */
  char              pad_0x2A[0x0002];         /* 0x002A */
  /* on-sequence-stop commands */
  uint32_t          stop_cmd_buf[8];          /* 0x002C */
  uint8_t           stop_cmd_timer;           /* 0x004C */
  uint8_t           stop_cmd_count;           /* 0x004D */
  uint8_t           b_0x4E;                   /* 0x004E */
  char              pad_0x4F[0x0001];         /* 0x004F */
  /* channel control stuff */
  z64_chan_ctl_t    channels[0x10];           /* 0x0050 */
  /* bitmask of channels with active effects */
  uint16_t          ch_pitch_state;           /* 0x0250 */
  uint16_t          ch_volume_state;          /* 0x0252 */
  /* sequence info */
  uint16_t          seq_idx;                  /* 0x0254 */
  uint16_t          prev_seq_idx;             /* 0x0256 */
  /* unknown */
  uint16_t          h_0x258;                  /* 0x0258 */
  char              unk_0x25A[0x0006];        /* 0x025A */
  uint8_t           b_0x260;                  /* 0x0260 */
  char              pad_0x261[0x0003];        /* 0x0261 */
                                              /* 0x0264 */
} z64_seq_ctl_t;

typedef struct
{
  /* which channels to use */
  uint16_t          channel_enable;           /* 0x0000 */
  /* which channels to disable initially */
  uint16_t          channel_mask;             /* 0x0002 */
  /* channel parameter command list */
  /* 3 bytes per command: ccppvv */
  /* c: channel (0xFF to terminate command list) */
  /* p: parameter id */
  /* v: parameter value */
  uint8_t           params[0x64];             /* 0x0004 */
                                              /* 0x0068 */
} z64_night_sfx_t;

typedef struct
{
  uint32_t          hi;                       /* 0x0000 */
  uint32_t          lo;                       /* 0x0004 */
                                              /* 0x0008 */
} z64_afx_cmd_t;

/* file indices */
#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12
# define z64_icon_item_static                   8
# define z64_icon_item_24_static                9
# define z64_icon_item_field_static             10
# define z64_icon_item_dungeon_static           11
# define z64_icon_item_gameover_static          12
# define z64_icon_item_jpn_static               13
# define z64_icon_item_nes_static               14
# define z64_item_name_static                   15
# define z64_map_name_static                    16
# define z64_message_static                     18
# define z64_message_texture_static             20
# define z64_nes_font_static                    21
# define z64_map_48x85_static                   26
# define z64_parameter_static                   940
# define z64_vr_cloud2_static                   953
# define z64_vr_cloud2_pal_static               954
# define z64_vr_holy0_static                    957
# define z64_vr_holy0_pal_static                958
# define z64_vr_holy1_static                    959
# define z64_vr_holy1_pal_static                960
# define z64_vr_MDVR_static                     961
# define z64_vr_MDVR_pal_static                 962
# define z64_vr_MNVR_static                     963
# define z64_vr_MNVR_pal_static                 964
# define z64_vr_RUVR_static                     965
# define z64_vr_RUVR_pal_static                 966
# define z64_vr_LHVR_static                     967
# define z64_vr_LHVR_pal_static                 968
# define z64_vr_KHVR_static                     969
# define z64_vr_KHVR_pal_static                 970
# define z64_vr_K3VR_static                     971
# define z64_vr_K3VR_pal_static                 972
# define z64_vr_K4VR_static                     973
# define z64_vr_K4VR_pal_static                 974
# define z64_vr_K5VR_static                     975
# define z64_vr_K5VR_pal_static                 976
# define z64_vr_SP1a_static                     977
# define z64_vr_SP1a_pal_static                 978
# define z64_vr_MLVR_static                     979
# define z64_vr_MLVR_pal_static                 980
# define z64_vr_KKRVR_static                    981
# define z64_vr_KKRVR_pal_static                982
# define z64_vr_KR3VR_static                    983
# define z64_vr_KR3VR_pal_static                984
# define z64_vr_IPVR_static                     985
# define z64_vr_IPVR_pal_static                 986
# define z64_vr_KSVR_static                     987
# define z64_vr_KSVR_pal_static                 988
# define z64_vr_GLVR_static                     989
# define z64_vr_GLVR_pal_static                 990
# define z64_vr_ZRVR_static                     991
# define z64_vr_ZRVR_pal_static                 992
# define z64_vr_DGVR_static                     993
# define z64_vr_DGVR_pal_static                 994
# define z64_vr_ALVR_static                     995
# define z64_vr_ALVR_pal_static                 996
# define z64_vr_NSVR_static                     997
# define z64_vr_NSVR_pal_static                 998
# define z64_vr_LBVR_static                     999
# define z64_vr_LBVR_pal_static                 1000
# define z64_vr_TTVR_static                     1001
# define z64_vr_TTVR_pal_static                 1002
# define z64_vr_FCVR_static                     1003
# define z64_vr_FCVR_pal_static                 1004
#elif Z64_VERSION == Z64_OOTMQJ || \
      Z64_VERSION == Z64_OOTMQU || \
      Z64_VERSION == Z64_OOTGCJ || \
      Z64_VERSION == Z64_OOTGCU || \
      Z64_VERSION == Z64_OOTCEJ
# define z64_icon_item_static                   8
# define z64_icon_item_24_static                9
# define z64_icon_item_field_static             10
# define z64_icon_item_dungeon_static           11
# define z64_icon_item_gameover_static          12
# define z64_icon_item_jpn_static               13
# define z64_icon_item_nes_static               14
# define z64_item_name_static                   15
# define z64_map_name_static                    16
# define z64_message_static                     18
# define z64_message_texture_static             19
# define z64_nes_font_static                    20
# define z64_map_48x85_static                   25
# define z64_parameter_static                   939
# define z64_vr_cloud2_static                   952
# define z64_vr_cloud2_pal_static               953
# define z64_vr_holy0_static                    956
# define z64_vr_holy0_pal_static                957
# define z64_vr_holy1_static                    958
# define z64_vr_holy1_pal_static                959
# define z64_vr_MDVR_static                     960
# define z64_vr_MDVR_pal_static                 961
# define z64_vr_MNVR_static                     962
# define z64_vr_MNVR_pal_static                 963
# define z64_vr_RUVR_static                     964
# define z64_vr_RUVR_pal_static                 965
# define z64_vr_LHVR_static                     966
# define z64_vr_LHVR_pal_static                 967
# define z64_vr_KHVR_static                     968
# define z64_vr_KHVR_pal_static                 969
# define z64_vr_K3VR_static                     970
# define z64_vr_K3VR_pal_static                 971
# define z64_vr_K4VR_static                     972
# define z64_vr_K4VR_pal_static                 973
# define z64_vr_K5VR_static                     974
# define z64_vr_K5VR_pal_static                 975
# define z64_vr_SP1a_static                     976
# define z64_vr_SP1a_pal_static                 977
# define z64_vr_MLVR_static                     978
# define z64_vr_MLVR_pal_static                 979
# define z64_vr_KKRVR_static                    980
# define z64_vr_KKRVR_pal_static                981
# define z64_vr_KR3VR_static                    982
# define z64_vr_KR3VR_pal_static                983
# define z64_vr_IPVR_static                     984
# define z64_vr_IPVR_pal_static                 985
# define z64_vr_KSVR_static                     986
# define z64_vr_KSVR_pal_static                 987
# define z64_vr_GLVR_static                     988
# define z64_vr_GLVR_pal_static                 989
# define z64_vr_ZRVR_static                     990
# define z64_vr_ZRVR_pal_static                 991
# define z64_vr_DGVR_static                     992
# define z64_vr_DGVR_pal_static                 993
# define z64_vr_ALVR_static                     994
# define z64_vr_ALVR_pal_static                 995
# define z64_vr_NSVR_static                     996
# define z64_vr_NSVR_pal_static                 997
# define z64_vr_LBVR_static                     998
# define z64_vr_LBVR_pal_static                 999
# define z64_vr_TTVR_static                     1000
# define z64_vr_TTVR_pal_static                 1001
# define z64_vr_FCVR_static                     1002
# define z64_vr_FCVR_pal_static                 1003
#endif

/* data */
#define     z64_extern            extern __attribute__ ((section(".data")))
z64_extern  OSThread              z64_thread_idle;
z64_extern  OSThread              z64_thread_main;
z64_extern  OSThread              z64_thread_dmamgr;
z64_extern  OSMesgQueue           z64_file_mq;
z64_extern  z64_ftab_t            z64_ftab[];
z64_extern  z64_part_t           *z64_part_space;
z64_extern  int32_t               z64_part_pos;
z64_extern  int32_t               z64_part_max;
z64_extern  z64_part_ovl_t        z64_part_ovl_tab[37];
z64_extern  z64_actor_ovl_t       z64_actor_ovl_tab[471];
z64_extern  char                  z_camera_c_data[];
z64_extern  char                  z64_hud_state[];
z64_extern  char                  z64_event_state_1[];
z64_extern  uint32_t              z64_letterbox_time;
z64_extern  char                  z64_event_state_2[];
z64_extern  char                  z64_event_camera[];
z64_extern  int32_t               z64_oob_timer;
z64_extern  char                  z64_cs_message[];
z64_extern  z64_state_ovl_t       z64_state_ovl_tab[6];
z64_extern  char                  z64_weather_state[];
z64_extern  uint32_t              z64_audio_cmd_buf[0x100];
z64_extern  z64_scene_table_t     z64_scene_table[];
z64_extern  uint16_t              z64_day_speed;
z64_extern  z64_sky_image_t       z64_sky_images[9];
z64_extern  z64_light_handler_t   z64_light_handlers[];
z64_extern  char                  z_onepointdemo_c_data[];
z64_extern  z64_map_mark_ovl_t    z64_map_mark_ovl;
z64_extern  char                  z64_dins_state_1[];
z64_extern  char                  z64_dins_state_2[];
z64_extern  int16_t               z64_minimap_entrance_x;
z64_extern  int16_t               z64_minimap_entrance_y;
z64_extern  int16_t               z64_minimap_entrance_r;
z64_extern  char                  z64_hazard_state[];
z64_extern  uint16_t              z64_temp_day_speed;
z64_extern  uint16_t              z64_n_camera_shake;
z64_extern  z64_vrom_file_t       z64_object_table[];
z64_extern  z64_entrance_table_t  z64_entrance_table[];
z64_extern  z64_scene_config_t    z64_scene_config_table[];
z64_extern  int32_t               z64_letterbox_target;
z64_extern  int32_t               z64_letterbox_current;
z64_extern  z64_play_ovl_t        z64_play_ovl_tab[2];
z64_extern  z64_play_ovl_t        z64_play_ovl_ptr;
z64_extern  char                  code_800EC960_c_data[];
z64_extern  char                  z64_sound_state[];
z64_extern  z64_night_sfx_t       z64_night_sfx[20];
z64_extern  char                  z64_ocarina_state[];
z64_extern  int32_t               z64_song_play_counter;
z64_extern  uint8_t               z64_ocarina_song_length;
z64_extern  char                  z64_scarecrow_song[];
z64_extern  char                  z64_song_ptr[];
z64_extern  uint32_t              z64_song_rec_counter;
z64_extern  uint8_t               z64_ocarina_button_state;
z64_extern  uint8_t               z64_sfx_write_pos;
z64_extern  uint8_t               z64_sfx_read_pos;
z64_extern  uint8_t               z64_audio_cmd_write_pos;
z64_extern  uint8_t               z64_audio_cmd_read_pos;
z64_extern  uint8_t               z64_afx_cfg;
z64_extern  uint8_t               z64_afx_config_busy;
z64_extern  uint32_t              z64_random;
z64_extern  char                  z64_message_state[];
z64_extern  char                  z64_staff_notes[];
z64_extern  int16_t               z64_message_select_state;
z64_extern  char                  z64_message_icon_state[];
z64_extern  char                  z64_message_note_icon_state[];
z64_extern  int16_t               z64_gameover_countdown;
z64_extern  z64_pfx_t             z64_pfx;
z64_extern  char                  z64_fw_state_1[];
z64_extern  char                  z64_fw_state_2[];
z64_extern  char                  z64_camera_state[];
z64_extern  z64_file_t            z64_file;
z64_extern  char                  z64_cs_state[];
z64_extern  z64_light_queue_t     z64_light_queue;
z64_extern  z64_arena_t           z64_game_arena;
z64_extern  void                 *z64_map_mark_data_tab;
z64_extern  char                  z64_timer_state[];
z64_extern  char                  z64_camera_shake[];
z64_extern  char                  z64_poly_colorfilter_state[];
z64_extern  OSThread              z64_thread_sched;
z64_extern  OSThread              z64_thread_padmgr;
z64_extern  z64_input_t           z64_input_direct;
z64_extern  OSThread              z64_thread_irqmgr;
z64_extern  OSThread              z64_thread_graph;
z64_extern  z64_stab_t            z64_stab;
z64_extern  OSThread              z64_thread_audio;
z64_extern  MtxF                (*z64_mtx_stack)[20];
z64_extern  MtxF                 *z64_mtx_stack_top;
z64_extern  OSThread              z64_thread_fault;
z64_extern  char                  code_800EC960_c_bss[];
z64_extern  char                  z64_song_state[];
z64_extern  uint32_t              z64_ocarina_counter;
z64_extern  char                  z64_sfx_mute[];
z64_extern  z64_seq_ctl_t         z64_seq_ctl[4];
z64_extern  char                  z64_afx[];
z64_extern  uint32_t              z64_afx_counter;
z64_extern  uint8_t               z64_afx_cmd_write_pos;
z64_extern  uint8_t               z64_afx_cmd_read_pos;
z64_extern  z64_afx_cmd_t         z64_afx_cmd_buf[0x100];
z64_extern  char                  z_message_c_bss[];
z64_extern  char                  z64_zimg[];
z64_extern  char                  z64_disp[];
z64_extern  z64_ctxt_t            z64_ctxt;
z64_extern  z64_game_t            z64_game;
z64_extern  z64_link_t            z64_link;
z64_extern  char                  z64_cimg[];
z64_extern  char                  z64_item_highlight_vram[];

/* functions */
void      z64_Actor_UpdateBgCheckInfo (z64_game_t *game, z64_actor_t *actor,
                                       float wall_check_height,
                                       float wall_check_radius,
                                       float ceiling_check_height,
                                       int32_t flags);
void      z64_DrawActors              (z64_game_t *game, void *actor_ctxt);
void      z64_SpawnActor              (void *actor_ctxt, z64_game_t *game,
                                       uint16_t actor_id, float x, float y,
                                       float z, uint16_t rx, uint16_t ry,
                                       uint16_t rz, uint16_t variable);
z64_actor_t *
          z64_SpawnActorAttachedB     (void *actor_ctxt, z64_actor_t *actor,
                                       z64_game_t *game, uint16_t actor_id,
                                       float x, float y, float z, uint16_t rx,
                                       uint16_t ry, uint16_t rz,
                                       uint16_t variable);
void      z64_CreateStaticCollision   (z64_col_ctxt_t *col_ctxt,
                                       z64_game_t *game,
                                       z64_col_lut_t *col_lut);
void      z64_Camera_ChangeMode       (z64_camera_t *camera, int16_t mode);
float     z64_Math_SinS               (int16_t angle);
float     z64_Math_CosS               (int16_t angle);
void      z64_LoadMinimap             (z64_game_t *game, int room_idx);
void      z64_SwitchAgeEquips         (void);
void      z64_UpdateItemButton        (z64_game_t *game, int button_index);
void      z64_LoadActionLabel         (z64_if_ctxt_t *if_ctxt,
                                       uint16_t action_idx, int button_idx);
void      z64_UpdateEquipment         (z64_game_t *game, z64_link_t *link);
void      z64_InitPauseObjects        (z64_game_t *game, void *addr,
                                       void *s72C);
void      z64_LoadRoom                (z64_game_t *game,
                                       z64_room_ctxt_t *room_ctxt,
                                       uint8_t room_index);
void      z64_DrawRoom                (z64_game_t *game, z64_room_t *room,
                                       int unk_a2);
void      z64_UnloadRoom              (z64_game_t *game,
                                       z64_room_ctxt_t *room_ctxt);
void      z64_Sram_LoadDebugSave      (void);
void      z64_Io                      (uint32_t dev_addr, void *dram_addr,
                                       uint32_t size, int32_t direction);
void      z64_CreateSkyGfx            (z64_sky_ctxt_t *sky_ctxt,
                                       int skybox_type);
void      z64_CreateSkyVtx            (z64_sky_ctxt_t *sky_ctxt, int a1);
void      z64_StopSfx                 (void);
void      z64_UpdateCtxtInput         (z64_ctxt_t *ctxt);
void      z64_GetInput                (void *input_ctxt, z64_input_t *input,
                                       int a2);
void      z64_AfxCmdF                 (uint32_t hi, float lo);
void      z64_AfxCmdW                 (uint32_t hi, uint32_t lo);
void      z64_FlushAfxCmd             (void);
void      z64_ConfigureAfx            (uint8_t cfg);
uint32_t  z64_AfxRand                 (void);
void      z64_OcarinaUpdate           (void);
void      z64_ResetAudio              (uint8_t cfg);
void      z64_Audio_PlaySfxGeneral    (uint16_t sfx_id, z64_xyzf_t *pos,
                                       uint8_t token, float *freq_scale,
                                       float *vol, int8_t *reverb_add);
uint8_t   z64_Audio_IsSfxPlaying      (uint32_t sfx_id);
int       z64_CheckAfxConfigBusy      (void);
uint32_t  z64_LoadOverlay             (uint32_t vrom_start, uint32_t vrom_end,
                                       uint32_t vram_start, uint32_t vram_end,
                                       void *dst);
void      z64_SeedRandom              (uint32_t seed);

#endif
