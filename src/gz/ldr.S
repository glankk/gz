#include "z64_version.h"

.section  .text.startup, "ax", @progbits
.set      nomips16
.set      nomicromips

#if Z64_VERSION == Z64_OOT10 || \
    Z64_VERSION == Z64_OOT11 || \
    Z64_VERSION == Z64_OOT12 || \
    Z64_VERSION == Z64_OOTMQJ || \
    Z64_VERSION == Z64_OOTMQU || \
    Z64_VERSION == Z64_OOTGCJ || \
    Z64_VERSION == Z64_OOTGCU || \
    Z64_VERSION == Z64_OOTCEJ
.equ      entry_size, 0x60
#elif Z64_VERSION == Z64_OOTIQC
.equ      entry_size, 0x50
#endif
.equ      image_rom_ptr_addr, entrypoint + entry_size - 4

.local    ldr
.ent      ldr
.type     ldr, @function
ldr:
  lui     $v0, 0xA460
#if Z64_VERSION != Z64_OOTIQC
  /* check for homeboy reset */
  lui     $v1, 0xA805
  lw      $v1, 0x0020($v1)
  beq     $v1, 0x6864, no_dma
#endif
  /* begin dma transfer */
  lui     $v1, %hi(__image_start)
  addiu   $v1, $v1, %lo(__image_start)
  sw      $v1, 0x0000($v0)
  lui     $v1, %hi(image_rom_ptr_addr)
  lw      $v1, %lo(image_rom_ptr_addr)($v1)
  sw      $v1, 0x0004($v0)
  lui     $v1, %hi(__image_size - 1)
  addiu   $v1, $v1, %lo(__image_size - 1)
  sw      $v1, 0x000C($v0)
  /* wait for completion */
  dma_busy_loop:
    lw    $v1, 0x0010($v0)
    andi  $v1, $v1, 1
    bnez  $v1, dma_busy_loop
  li      $v1, 2
  sw      $v1, 0x0010($v0)
  no_dma:
  /* jump to preamble, moved here by patcher */
  j       ldr
.end      ldr
.size     ldr, . - ldr

.fill     entry_size - 4 - (. - ldr)
.local    image_rom_ptr
.type     image_rom_ptr, @object
image_rom_ptr:
  /* magic placeholder number, to be filled in by patcher */
  .word   0x01234567
