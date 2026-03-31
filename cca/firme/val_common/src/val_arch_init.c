/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_common/val_arch_init.h"

#include "aarch64/sysreg.h"

/**
 *   @brief    - Perform minimal architectural initialization for a payload.
 *   @param    - el : Execution level used for FP/SIMD enablement (EL2 or EL1).
 *   @return   - void
**/
void val_arch_init_basic(mmu_el_t el)
{
    /*
     * Enable FP/SIMD for payload code (avoid traps).
     * - EL2: clear CPTR_EL2.TFP (bit 10).
     * - EL1: set CPACR_EL1.FPEN=0b11 (bits 21:20).
     */
    if (el == MMU_EL2) {
        uint64_t cptr = read_cptr_el2();
        cptr &= ~(UINT64_C(1) << 10);
        write_cptr_el2(cptr);
    } else {
        uint64_t cpacr = read_cpacr_el1();
        cpacr |= (UINT64_C(3) << 20);
        write_cpacr_el1(cpacr);
    }
}
