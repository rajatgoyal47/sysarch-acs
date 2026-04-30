/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#ifndef PAL_SYSREG_H
#define PAL_SYSREG_H

#if defined(__ASSEMBLY__) || defined(__ASSEMBLER__)

/* Assembly/C preprocessor macro for .S files */
#define READ_MPIDR_EL1(reg) mrs reg, mpidr_el1

#else

typedef unsigned long u_register_t;

#ifndef _SYSREG_READ_FUNC
#define _SYSREG_READ_FUNC(_name, _reg_name)                \
static inline u_register_t read_ ## _name(void)            \
{                                                          \
    u_register_t v;                                        \
    __asm__ volatile ("mrs %0, " #_reg_name : "=r" (v));   \
    return v;                                              \
}

/* Define read function for system register */
#define SYSREG_READ_FUNC(_name)             \
    _SYSREG_READ_FUNC(_name, _name)
#endif

SYSREG_READ_FUNC(mpidr_el1)
SYSREG_READ_FUNC(CurrentEL)
SYSREG_READ_FUNC(ttbr0_el1)
SYSREG_READ_FUNC(ttbr0_el2)
SYSREG_READ_FUNC(tcr_el1)
SYSREG_READ_FUNC(tcr_el2)
SYSREG_READ_FUNC(ttbr1_el1)
SYSREG_READ_FUNC(ttbr1_el2)
SYSREG_READ_FUNC(mair_el1)
SYSREG_READ_FUNC(mair_el2)
SYSREG_READ_FUNC(sctlr_el1)
SYSREG_READ_FUNC(sctlr_el2)
#endif

#endif  /* PAL_SYSREG_H */
