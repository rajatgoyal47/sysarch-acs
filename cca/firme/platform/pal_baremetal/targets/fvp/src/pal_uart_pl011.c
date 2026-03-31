/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_uart.h"

/* PL011 UART registers */
#define UARTDR 0x00
#define UARTFR 0x18

#define UARTFR_TXFF (1u << 5)

/**
 *   @brief    - Write a 32-bit value to a MMIO address.
 *   @param    - addr : MMIO address.
 *   @param    - v    : Value to write.
**/
static inline void mmio_write32(uintptr_t addr, uint32_t v)
{
    __atomic_store_n((uint32_t *)(uintptr_t)addr, v, __ATOMIC_RELAXED);
}

/**
 *   @brief    - Initialize a PL011 UART instance.
 *   @param    - uart : UART instance to initialize.
 *   @param    - base : PL011 base address.
 *   @return   - void
**/
void pal_uart_pl011_init(pl011_uart_t *uart, uintptr_t base)
{
    uart->base = base;
}

/**
 *   @brief    - Output a character on PL011 UART (best-effort).
 *   @param    - uart : UART instance.
 *   @param    - c    : Character to transmit.
 *   @return   - void
**/
void pal_uart_pl011_putc(pl011_uart_t *uart, char c)
{
    /*
     * Best-effort TX: avoid reading UARTFR. On some FVP/TF-A configurations
     * the status register access can become unreliable (or trap) after EL3
     * reconfiguration, which makes polling look like a hang.
     */
    mmio_write32(uart->base + UARTDR, (uint32_t)c);
}

/**
 *   @brief    - Output a null-terminated string on PL011 UART.
 *   @param    - uart : UART instance.
 *   @param    - s    : String to transmit.
 *   @return   - void
**/
void pal_uart_pl011_puts(pl011_uart_t *uart, const char *s)
{
    for (; *s; s++) {
        if (*s == '\n')
            pal_uart_pl011_putc(uart, '\r');
        pal_uart_pl011_putc(uart, *s);
    }
}
