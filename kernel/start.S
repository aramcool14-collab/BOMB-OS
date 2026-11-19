    .syntax unified
    .cpu arm1176jzf-s
    .arm

    .section .text.boot, "ax"
    .global _start
    .extern kernel_main
    .extern __bss_start
    .extern __bss_end

_start:
    cpsid if
    ldr sp, =_stack_top

    /* Clear BSS before entering C. */
    mov r2, #0
    ldr r0, =__bss_start
    ldr r1, =__bss_end
1:
    cmp r0, r1
    strlo r2, [r0], #4
    blo 1b

    bl kernel_main
2:
    b 2b
