/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text.entry, "ax"

    .extern __bss_start__
    .extern __bss_end__
    .extern __stack_end__
    .extern val_zero_bss
    .extern nsel2_main

func nsel2_entry
    /* Ensure we use SP_EL2 (SPx) so EL2 exceptions hit the SPx vector table. */
    mov x1, #1
    msr spsel, x1
    /* ISB (avoid recursive macro from asm_macros.S). */
    .inst 0xd5033fdf
    /* Avoid hard hangs on unexpected Group0/FIQ routing during bring-up. */
    msr daifset, #1

    /* Set up stack */
    ldr x0, =__stack_end__
    mov sp, x0
    /* Also initialize SP_EL0 so accidental SPSel=0 does not crash. */
    msr sp_el0, x0

    /* Zero BSS */
    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    bl  val_zero_bss

    /* Jump to C */
    bl  nsel2_main

1:  wfe
    b   1b
endfunc nsel2_entry
