    .section    __TEXT,__text,regular,pure_instructions
    .globl _bar
    .p2align 2
_bar:
    mov w8, #5
    str w8, [sp]
    sub sp, sp, #16
    mov w8, w1
    ldr w9, [sp, #16]
    mul w8, w8, w9
    add sp, sp, #16
    mov w0, w8
    ret

    .globl _foo
    .p2align 2
_foo:
    mov w8, w1
    str w8, [sp]
    sub sp, sp, #16
    mov w8, w2
    str w8, [sp]
    sub sp, sp, #16
    mov w8, #5
    ldr w9, [sp, #16]
    mul w8, w8, w9
    add sp, sp, #16
    str w8, [sp]
    sub sp, sp, #16
    sub sp, sp, #32
    stp x29, x30, [sp, #16]
    str w8, [sp, #8]
    mov w8, #2
    mov w1, w8
    bl _bar
    ldp x29, x30, [sp, #16]
    ldr w8, [sp, #8]
    add sp, sp, #32
    str w8, [sp]
    sub sp, sp, #16
    mov w8, w3
    ldr w9, [sp, #16]
    sdiv w8, w8, w9
    add sp, sp, #16
    ldr w9, [sp, #16]
    add w8, w8, w9
    add sp, sp, #16
    ldr w9, [sp, #16]
    add w8, w8, w9
    add sp, sp, #16
    mov w0, w8
    ret

    .globl _main
    .p2align 2
_main:
    sub sp, sp, #32
    stp x29, x30, [sp, #16]
    str w8, [sp, #8]
    mov w8, #1
    mov w1, w8
    mov w8, #2
    mov w2, w8
    mov w8, #3
    mov w3, w8
    bl _foo
    ldp x29, x30, [sp, #16]
    ldr w8, [sp, #8]
    add sp, sp, #32
    mov w0, w8
    ret

