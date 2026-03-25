/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text, "ax"
    .align 11
    .global nsel2_vectors
nsel2_vectors:
    /*
     * Vector routing notes:
     * - Payload code forces `SPSel=1` in its entry path, but some early/edge
     *   paths may still take exceptions on SP0 (e.g. during bring-up or after
     *   unexpected state changes).
     * - Route both SP0 and SPx synchronous exceptions to the same handler so
     *   probe faults are always recoverable and the suite does not hang.
     */
    /*
     * AArch64 vector table layout requires each entry at a fixed 0x80-byte
     * offset. Use `.balign 0x80` between entries to guarantee correct offsets.
     */

    /* Current EL with SP0 */
    b nsel2_sync_spx
    .balign 0x80
    b nsel2_irq_spx
    .balign 0x80
    b nsel2_fiq_mask
    .balign 0x80
    b nsel2_serror_spx
    .balign 0x80

    /* Current EL with SPx */
    b nsel2_sync_spx
    .balign 0x80
    b nsel2_irq_spx
    .balign 0x80
    b nsel2_fiq_mask
    .balign 0x80
    b nsel2_serror_spx
    .balign 0x80

    /* Lower EL using AArch64 */
    b nsel2_unhandled
    .balign 0x80
    b nsel2_unhandled
    .balign 0x80
    b nsel2_unhandled
    .balign 0x80
    b nsel2_unhandled
    .balign 0x80

    /* Lower EL using AArch32 */
    b nsel2_unhandled
    .balign 0x80
    b nsel2_unhandled
    .balign 0x80
    b nsel2_unhandled
    .balign 0x80
    b nsel2_unhandled
    .balign 0x80

    .extern nsel2_sync_handler
    .extern nsel2_irq_handler

func nsel2_sync_spx
    /* Save full context so probe recovery doesn't corrupt caller registers. */
    sub sp, sp, #0x100
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]
    stp x8, x9, [sp, #0x40]
    stp x10, x11, [sp, #0x50]
    stp x12, x13, [sp, #0x60]
    stp x14, x15, [sp, #0x70]
    stp x16, x17, [sp, #0x80]
    stp x18, x19, [sp, #0x90]
    stp x20, x21, [sp, #0xA0]
    stp x22, x23, [sp, #0xB0]
    stp x24, x25, [sp, #0xC0]
    stp x26, x27, [sp, #0xD0]
    stp x28, x29, [sp, #0xE0]
    str x30, [sp, #0xF0]
    mrs x0, esr_el2
    mrs x1, elr_el2
    mrs x2, far_el2
    bl  nsel2_sync_handler
    ldr x30, [sp, #0xF0]
    ldp x28, x29, [sp, #0xE0]
    ldp x26, x27, [sp, #0xD0]
    ldp x24, x25, [sp, #0xC0]
    ldp x22, x23, [sp, #0xB0]
    ldp x20, x21, [sp, #0xA0]
    ldp x18, x19, [sp, #0x90]
    ldp x16, x17, [sp, #0x80]
    ldp x14, x15, [sp, #0x70]
    ldp x12, x13, [sp, #0x60]
    ldp x10, x11, [sp, #0x50]
    ldp x8, x9, [sp, #0x40]
    ldp x6, x7, [sp, #0x30]
    ldp x4, x5, [sp, #0x20]
    ldp x2, x3, [sp, #0x10]
    ldp x0, x1, [sp, #0x00]
    add sp, sp, #0x100
    eret
endfunc nsel2_sync_spx

func nsel2_irq_spx
    /* Save full context: interrupt handlers are C and may clobber registers. */
    sub sp, sp, #0x100
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]
    stp x8, x9, [sp, #0x40]
    stp x10, x11, [sp, #0x50]
    stp x12, x13, [sp, #0x60]
    stp x14, x15, [sp, #0x70]
    stp x16, x17, [sp, #0x80]
    stp x18, x19, [sp, #0x90]
    stp x20, x21, [sp, #0xA0]
    stp x22, x23, [sp, #0xB0]
    stp x24, x25, [sp, #0xC0]
    stp x26, x27, [sp, #0xD0]
    stp x28, x29, [sp, #0xE0]
    str x30, [sp, #0xF0]
    bl  nsel2_irq_handler
    ldr x30, [sp, #0xF0]
    ldp x28, x29, [sp, #0xE0]
    ldp x26, x27, [sp, #0xD0]
    ldp x24, x25, [sp, #0xC0]
    ldp x22, x23, [sp, #0xB0]
    ldp x20, x21, [sp, #0xA0]
    ldp x18, x19, [sp, #0x90]
    ldp x16, x17, [sp, #0x80]
    ldp x14, x15, [sp, #0x70]
    ldp x12, x13, [sp, #0x60]
    ldp x10, x11, [sp, #0x50]
    ldp x8, x9, [sp, #0x40]
    ldp x6, x7, [sp, #0x30]
    ldp x4, x5, [sp, #0x20]
    ldp x2, x3, [sp, #0x10]
    ldp x0, x1, [sp, #0x00]
    add sp, sp, #0x100
    eret
endfunc nsel2_irq_spx

func nsel2_serror_spx
    /* Save full context so probe recovery doesn't corrupt caller registers. */
    sub sp, sp, #0x100
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]
    stp x8, x9, [sp, #0x40]
    stp x10, x11, [sp, #0x50]
    stp x12, x13, [sp, #0x60]
    stp x14, x15, [sp, #0x70]
    stp x16, x17, [sp, #0x80]
    stp x18, x19, [sp, #0x90]
    stp x20, x21, [sp, #0xA0]
    stp x22, x23, [sp, #0xB0]
    stp x24, x25, [sp, #0xC0]
    stp x26, x27, [sp, #0xD0]
    stp x28, x29, [sp, #0xE0]
    str x30, [sp, #0xF0]
    mrs x0, esr_el2
    mrs x1, elr_el2
    mrs x2, far_el2
    bl  nsel2_sync_handler
    ldr x30, [sp, #0xF0]
    ldp x28, x29, [sp, #0xE0]
    ldp x26, x27, [sp, #0xD0]
    ldp x24, x25, [sp, #0xC0]
    ldp x22, x23, [sp, #0xB0]
    ldp x20, x21, [sp, #0xA0]
    ldp x18, x19, [sp, #0x90]
    ldp x16, x17, [sp, #0x80]
    ldp x14, x15, [sp, #0x70]
    ldp x12, x13, [sp, #0x60]
    ldp x10, x11, [sp, #0x50]
    ldp x8, x9, [sp, #0x40]
    ldp x6, x7, [sp, #0x30]
    ldp x4, x5, [sp, #0x20]
    ldp x2, x3, [sp, #0x10]
    ldp x0, x1, [sp, #0x00]
    add sp, sp, #0x100
    eret
endfunc nsel2_serror_spx

func nsel2_fiq_mask
    /* Mask FIQ and return to avoid hard hangs on unexpected Group0/FIQ routing. */
    msr daifset, #1
    eret
endfunc nsel2_fiq_mask

func nsel2_unhandled
1:  wfe
    b 1b
endfunc nsel2_unhandled
