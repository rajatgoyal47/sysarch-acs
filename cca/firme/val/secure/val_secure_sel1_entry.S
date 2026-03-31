/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text, "ax"

    .extern secure_vectors_el1
    .extern secure_el1_main

    .extern g_el1_saved_regs
    .extern g_el1_ctx_ptr

/*
 *   @brief    - Enter S-EL1 to execute a Secure command body, then return to S-EL2.
 *               The S-EL1 code signals completion via `hvc #0`, which traps to S-EL2.
 *   @param    - x0: pointer to shared context structure.
 *   @param    - x1: S-EL1 stack top.
 *   @return   - returns to the caller after the S-EL1 handler completes.
 */
func secure_enter_el1
    /* Save AAPCS64 callee-saved regs across the EL transition. */
    ldr x9, =g_el1_saved_regs
    stp x19, x20, [x9, #0]
    stp x21, x22, [x9, #16]
    stp x23, x24, [x9, #32]
    stp x25, x26, [x9, #48]
    stp x27, x28, [x9, #64]
    stp x29, x30, [x9, #80]

    /* Record context pointer for the EL2 lower-sync handler. */
    ldr x9, =g_el1_ctx_ptr
    str x0, [x9]

    /* Program S-EL1 stack and vectors. */
    msr sp_el1, x1
    ldr x9, =secure_vectors_el1
    msr vbar_el1, x9
    .inst 0xd5033fdf /* isb */

    /* Return to S-EL1 (EL1h) at secure_el1_entry. */
    ldr x9, =secure_el1_entry
    msr elr_el2, x9
    mov x9, #0x3c5 /* D,A,I,F masked; EL1h */
    msr spsr_el2, x9
    .inst 0xd5033fdf /* isb */
    eret

    .global secure_enter_el1_return
secure_enter_el1_return:
    /* Restore callee-saved regs and return to the S-EL2 caller. */
    ldr x9, =g_el1_saved_regs
    ldp x19, x20, [x9, #0]
    ldp x21, x22, [x9, #16]
    ldp x23, x24, [x9, #32]
    ldp x25, x26, [x9, #48]
    ldp x27, x28, [x9, #64]
    ldp x29, x30, [x9, #80]
    ret
endfunc secure_enter_el1

/*
 *   @brief    - S-EL1 entry used by secure_enter_el1().
 *               Uses the already-programmed SP_EL1 as stack and invokes the C body.
 */
    .global secure_el1_entry
secure_el1_entry:
    mov x9, #1
    msr spsel, x9
    .inst 0xd5033fdf /* isb */
    bl  secure_el1_main
1:  wfe
    b   1b
