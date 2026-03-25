/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_common/val_gicv3.h"

#include "aarch64/sysreg.h"

/**
 *   @brief    - Enable the GICv3 CPU interface using system registers.
 *   @return   - void
**/
void val_gicv3_cpuif_enable(void)
{
    const uint64_t el = (read_currentel() >> 2) & 0x3U;

    /*
     * Enable system register interface at EL2 (SRE=1). Many platforms already
     * do this in firmware; re-programming is typically harmless.
     */
    if (el == 2U) {
        write_icc_sre_el2(1ULL);
    } else {
        write_icc_sre_el1(1ULL);
    }

    /* Allow all priorities. */
    write_icc_pmr_el1(0xFFULL);

    /* Enable Group1 interrupts. */
    write_icc_igrpen1_el1(1ULL);
}

/**
 *   @brief    - Acknowledge an interrupt and return the INTID.
 *   @return   - Interrupt ID.
**/
uint32_t val_gicv3_acknowledge_irq(void)
{
    return (uint32_t)read_icc_iar1_el1();
}

/**
 *   @brief    - Signal end-of-interrupt for the given INTID.
 *   @param    - intid : Interrupt ID.
 *   @return   - void
**/
void val_gicv3_end_of_irq(uint32_t intid)
{
    write_icc_eoir1_el1((uint64_t)intid);
}
