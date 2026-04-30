/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_SYSREG_PMU_REG_H
#define VAL_SYSREG_PMU_REG_H

#include "val_sysreg.h"

SYSREG_RW_FUNCS(pmcntenset_el0)
SYSREG_RW_FUNCS(pmccntr_el0)
SYSREG_RW_FUNCS(pmccfiltr_el0)

#endif /* VAL_SYSREG_PMU_REG_H */
