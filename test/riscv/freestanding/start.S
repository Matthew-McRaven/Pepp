         .section .text.startup,"ax",@progbits
         .globl _start
         .type  _start, @function
_start:
         la   sp, __stack_top
         call main       # a0 = return value
         li   a7, 93     # __NR_exit
         wfi
