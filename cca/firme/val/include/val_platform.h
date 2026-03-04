/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/**
 *   @brief    - Return the UART base used for NS console logging.
 *   @return   - PL011 UART base address.
**/
uintptr_t val_get_ns_uart_base(void);
