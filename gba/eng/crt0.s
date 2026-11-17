@ Arranque mínimo de GBA (sin devkitPro). gbafix rellena logo, título y checksum.
    .section .crt0,"ax",%progbits
    .arm
    .global _start
_start:
    b start_vector
    .fill 188,1,0
start_vector:
    mov r0, #0x12
    msr cpsr_c, r0
    ldr sp, =0x03007FA0
    mov r0, #0x1F
    msr cpsr_c, r0
    ldr sp, =0x03007F00
    ldr r0, =__iwram_lma
    ldr r1, =__iwram_start
    ldr r2, =__iwram_end
1:  cmp r1, r2
    ldrlt r3, [r0], #4
    strlt r3, [r1], #4
    blt 1b
    mov r3, #0
    ldr r1, =__bss_start
    ldr r2, =__bss_end
2:  cmp r1, r2
    strlt r3, [r1], #4
    blt 2b
    ldr r1, =__ewram_bss_start
    ldr r2, =__ewram_bss_end
3:  cmp r1, r2
    strlt r3, [r1], #4
    blt 3b
    ldr r0, =0x03007FFC
    ldr r1, =irq_handler
    str r1, [r0]
    ldr r0, =main
    mov lr, pc
    bx r0
4:  b 4b
    .pool

    .section .iwram,"ax",%progbits
    .arm
    .align 2
    .global irq_handler
irq_handler:
    mov r0, #0x04000000
    add r2, r0, #0x200
    ldr r1, [r2]
    and r1, r1, r1, lsr #16
    strh r1, [r2, #2]
    ldr r2, =0x03007FF8
    ldrh r3, [r2]
    orr r3, r3, r1
    strh r3, [r2]
    ldr r2, =vbl_count
    ldr r3, [r2]
    add r3, r3, #1
    str r3, [r2]
    @ botones: acumula pulsaciones nuevas entre frames (para no perder toques cortos)
    ldr r2, =0x04000130
    ldrh r3, [r2]
    mvn r3, r3
    ldr r0, =0x3FF
    and r3, r3, r0
    ldr r2, =key_irq_prev
    ldrh r1, [r2]
    strh r3, [r2]
    bic r3, r3, r1
    ldr r2, =key_irq_hit
    ldrh r1, [r2]
    orr r1, r1, r3
    strh r1, [r2]
    bx lr
    .pool
