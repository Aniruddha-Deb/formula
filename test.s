	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
        ; sub sp, sp, #16 ; byte-align sp
        mov w1, #5 ; const 5
        str w1, [sp]
        sub sp, sp, #16 ; not allowed, as sp has to be 16-byte aligned?
        mov w1, #10 ; const 10
        ldr w2, [sp, #16]
        add w1, w1, w2
        add sp, sp, #16

        mov w0, #0
        ret
                                        ; -- End function
