.equ      stack_size, 0x2000

.section  .text, "ax", @progbits
.set      nomips16
.set      nomicromips
.global   _start
.ent      _start
.type     _start, @function
_start:
  la      $t0, _startup_data
  sw      $sp, 0x0000($t0)
  sw      $ra, 0x0004($t0)
  la      $sp, _stack + stack_size - 0x10
  jal     main
  la      $t0, _startup_data
  lw      $sp, 0x0000($t0)
  lw      $ra, 0x0004($t0)
  jr      $ra
.end      _start
.size     _start, . - _start

.section  .text, "ax", @progbits
.set      nomips16
.set      nomicromips
.global   gz_leave
.ent      gz_leave
.type     gz_leave, @function
gz_leave:
  addiu   $sp, $sp, -0x0008
  sw      $ra, 0x0004($sp)
  la      $t0, _startup_data
  lw      $t1, 0x0000($t0)
  sw      $sp, 0x0000($t0)
  move    $sp, $t1
  lw      $t9, 0x0008($t0)
  jalr    $t9
  la      $t0, _startup_data
  lw      $t1, 0x0000($t0)
  sw      $sp, 0x0000($t0)
  move    $sp, $t1
  lw      $ra, 0x0004($sp)
  addiu   $sp, $sp, 0x0008
  jr      $ra
.end      gz_leave
.size     gz_leave, . - gz_leave

.section  .stack, "aw", @nobits
.align    3
.type     _stack, @object
.local    _stack
_stack:
.space    stack_size
.size     _stack, . - _stack

.section  .data, "aw", @progbits
.align    2
.type     _startup_data, @object
.local    _startup_data
_startup_data:
.space    0x8
.type     gz_leave_func, @object
.global   gz_leave_func
gz_leave_func:
.space    0x4
.size     gz_leave_func, . - gz_leave_func
.size     _startup_data, . - _startup_data
