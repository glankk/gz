.section  .text, "ax", @progbits
.set      nomips16
.set      nomicromips

.global   _start
.ent      _start
.type     _start, @function
_start:
  /* begin dma transfer */
  la      $v0, 0xA4600000
  la      $v1, ((DMA_RAM) & 0x1FFFFFFF)
  sw      $v1, 0x0000($v0)
  la      $v1, ((DMA_ROM) | 0x10000000)
  sw      $v1, 0x0004($v0)
  la      $v1, ((DMA_SIZE) - 1)
  sw      $v1, 0x000C($v0)
  /* wait for completion */    
  dma_busy_loop:
    lw    $v1, 0x0010($v0)
    andi  $v1, $v1, 0x0001
    bnez  $v1, dma_busy_loop
  la      $v1, 0x00000002
  sw      $v1, 0x0010($v0)
  /* jump to preamble */
  j       (DMA_RAM)
.end      _start
.size     _start, . - _start

/* input side channel */
.fill     0x5E - (. - _start)
.global   input_sch_pad
.type     input_sch_pad, @object
.size     input_sch_pad, 2
input_sch_pad:
  .hword  0x0000
