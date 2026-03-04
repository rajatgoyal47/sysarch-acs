/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_platform.h"

#include "pal_platform.h"

/**
 *   @brief    - Return the UART base used for NS console logging.
 *   @return   - PL011 UART base address.
**/
uintptr_t val_get_ns_uart_base(void)
{
    return (uintptr_t)PLAT_NS_UART_BASE;
}
