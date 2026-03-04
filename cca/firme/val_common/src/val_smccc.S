/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text, "ax"
    .align 3

/*
	 * void val_smccc_smc_call(uint64_t fid,
 *                     uint64_t arg1, uint64_t arg2, uint64_t arg3,
 *                     uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
 *                     smccc_res_t *res);
 *
 * Notes:
 * - The 9th argument (res) is passed on the stack per AAPCS64.
 * - We preserve x19-x30 around the SMC so callers can safely keep live values
 *   in callee-saved registers across the call (robust vs. EL3 clobbering).
 */
	func val_smccc_smc_call
    /* Save original SP to fetch stack-passed 'res' pointer. */
    mov x9, sp
    ldr x10, [x9] /* res pointer */

    /* Save callee-saved registers. */
    stp x29, x30, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x19, x20, [sp, #-16]!

	    /* Save 'res' pointer across the SMC (x10 is caller-saved). */
	    sub sp, sp, #16
	    str x10, [sp]

	    smc #0

	    /*
	     * SMCCC return path:
	     * - Control returns here after EL3 services the SMC (or forwards it).
	     * - Per SMCCC, the return values are in x0-x3 (and some implementations
	     *   also return additional information in x4-x7).
	     * - Reload the caller-provided result pointer and store x0-x7 into it.
	     *
	     * Note:
	     * - EL3 is permitted to clobber all caller-saved registers. We therefore
	     *   keep the `res` pointer in memory (on our stack) across `smc #0`
	     *   instead of relying on it staying live in a register.
	     */
	    ldr x10, [sp]
	    add sp, sp, #16

    cbz x10, 1f
    str x0, [x10, #0x00]
    str x1, [x10, #0x08]
    str x2, [x10, #0x10]
    str x3, [x10, #0x18]
    str x4, [x10, #0x20]
    str x5, [x10, #0x28]
    str x6, [x10, #0x30]
    str x7, [x10, #0x38]
1:
    /* Restore callee-saved registers. */
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x29, x30, [sp], #16

    ret
	endfunc val_smccc_smc_call
