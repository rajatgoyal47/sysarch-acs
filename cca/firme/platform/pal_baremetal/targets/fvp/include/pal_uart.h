/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uintptr_t base;
} pl011_uart_t;

void pal_uart_pl011_init(pl011_uart_t *uart, uintptr_t base);
void pal_uart_pl011_putc(pl011_uart_t *uart, char c);
void pal_uart_pl011_puts(pl011_uart_t *uart, const char *s);
