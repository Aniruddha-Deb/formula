    .section    __TEXT,__text,regular,pure_instructions
    .globl  _main
    .p2align    2
_main:
    mov w8, #5
    str w8, [sp]
    sub sp, sp, #16
    mov w8, #2
    ldr w9, [sp, #16]
    add w8, w8, w9
    add sp, sp, #16
    ret


