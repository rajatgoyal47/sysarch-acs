/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VAL_SYSREG_RAS_H
#define VAL_SYSREG_RAS_H

#include "val_sysreg.h"

RENAME_SYSREG_RW_FUNCS(errselr_el1, ERRSELR_EL1)

RENAME_SYSREG_READ_FUNC(erxfr_el1, ERXFR_EL1)
RENAME_SYSREG_RW_FUNCS(erxctlr_el1, ERXCTLR_EL1)
RENAME_SYSREG_RW_FUNCS(erxstatus_el1, ERXSTATUS_EL1)
RENAME_SYSREG_RW_FUNCS(erxaddr_el1, ERXADDR_EL1)

SYSREG_READ_FUNC(erxpfgf_el1)
SYSREG_RW_FUNCS(erxpfgctl_el1)
SYSREG_RW_FUNCS(erxpfgcdn_el1)

#endif /* VAL_SYSREG_RAS_H */
