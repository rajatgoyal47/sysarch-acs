/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_SYSREG_GIC_H
#define VAL_SYSREG_GIC_H

#include "val_sysreg.h"

SYSREG_RW_FUNCS(ich_hcr_el2)
SYSREG_READ_FUNC(ich_misr_el2)
RENAME_SYSREG_RW_FUNCS(icc_igrpen1_el1, ICC_IGRPEN1_EL1)
SYSREG_RW_FUNCS(icc_bpr1_el1)
RENAME_SYSREG_RW_FUNCS(icc_pmr_el1, ICC_PMR_EL1)

#endif /* VAL_SYSREG_GIC_H */
