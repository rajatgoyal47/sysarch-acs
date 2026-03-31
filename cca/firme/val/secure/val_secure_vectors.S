/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text, "ax"

    .align 11
    .global secure_vectors_el2
secure_vectors_el2:
    /*
     * Vector routing notes:
     * - Secure entry code forces `SPSel=1`, but we route SP0 entries to the
     *   same handlers for robustness (no silent hangs).
     * - Route SError to the synchronous handler so probe-related SErrors can
     *   be reported and recovered (instead of trapping into an unhandled loop).
     */
    /* Current EL with SP0 */
    b secure_sync_el2_spx
    .balign 0x80
    b secure_irq_el2_spx
    .balign 0x80
    b secure_fiq_mask_el2
    .balign 0x80
    b secure_sync_el2_spx
    .balign 0x80

    /* Current EL with SPx */
    b secure_sync_el2_spx
    .balign 0x80
    b secure_irq_el2_spx
    .balign 0x80
    b secure_fiq_mask_el2
    .balign 0x80
    b secure_sync_el2_spx
    .balign 0x80

    /* Lower EL using AArch64 */
    b secure_lower_sync_el2_spx
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80

    /* Lower EL using AArch32 */
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80

    .align 11
    .global secure_vectors_el1
secure_vectors_el1:
    /* Current EL with SP0 */
    b secure_sync_el1_spx
    .balign 0x80
    b secure_irq_el1_spx
    .balign 0x80
    b secure_fiq_mask_el1
    .balign 0x80
    b secure_sync_el1_spx
    .balign 0x80

    /* Current EL with SPx */
    b secure_sync_el1_spx
    .balign 0x80
    b secure_irq_el1_spx
    .balign 0x80
    b secure_fiq_mask_el1
    .balign 0x80
    b secure_sync_el1_spx
    .balign 0x80

    /* Lower EL using AArch64 */
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80

    /* Lower EL using AArch32 */
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80
    b secure_unhandled
    .balign 0x80

    .extern secure_sync_handler
    .extern secure_irq_handler
    .extern secure_lower_sync_handler

func secure_sync_el2_spx
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
    bl  secure_sync_handler
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
endfunc secure_sync_el2_spx

func secure_irq_el2_spx
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
    bl  secure_irq_handler
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
endfunc secure_irq_el2_spx

func secure_fiq_mask_el2
    /* Mask FIQ and return to avoid hard hangs on unexpected Group0/FIQ routing. */
    msr daifset, #1
    eret
endfunc secure_fiq_mask_el2

func secure_lower_sync_el2_spx
    /* Save full context: lower EL traps (e.g. HVC) are handled in C. */
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
    bl  secure_lower_sync_handler
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
endfunc secure_lower_sync_el2_spx

func secure_sync_el1_spx
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
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    bl  secure_sync_handler
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
endfunc secure_sync_el1_spx

func secure_irq_el1_spx
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
    bl  secure_irq_handler
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
endfunc secure_irq_el1_spx

func secure_fiq_mask_el1
    /* Mask FIQ and return to avoid hard hangs on unexpected Group0/FIQ routing. */
    msr daifset, #1
    eret
endfunc secure_fiq_mask_el1

func secure_unhandled
1:  wfe
    b 1b
endfunc secure_unhandled
