/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_SYSREG_MPAM_H
#define VAL_SYSREG_MPAM_H

#include "val_sysreg.h"

SYSREG_RW_FUNCS(mpam1_el1)
RENAME_SYSREG_RW_FUNCS(mpam2_el2, MPAM2_EL2)
RENAME_SYSREG_READ_FUNC(mpamidr_el1, MPAMIDR_EL1)

#endif /* VAL_SYSREG_MPAM_H */
