#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>
#include "menu.h"

#define input_sch_pad_addr    0x8000045E
#define input_sch_pad         (*(uint16_t*)input_sch_pad_addr)

#define INPUT_REPEAT_DELAY    8

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

void      input_update(void);
uint16_t  input_z_pad(void);
uint16_t  input_pad(void);
uint16_t  input_pressed_raw(void);
uint16_t  input_pressed(void);
uint16_t  input_released(void);
void      input_reservation_set(_Bool enabled);
void      input_reserve(uint16_t bitmask);
void      input_free(uint16_t bitmask);
uint16_t  input_bind_make(int length, ...);
void      input_bind_set_disable(int index, _Bool value);
void      input_bind_set_override(int index, _Bool value);
_Bool     input_bind_held(int index);
_Bool     input_bind_pressed_raw(int index);
_Bool     input_bind_pressed(int index);

struct menu_item *binder_create(struct menu *menu, int x, int y,
                                int bind_index);

extern const uint32_t input_button_color[];

#endif

