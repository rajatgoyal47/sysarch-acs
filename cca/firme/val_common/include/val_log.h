/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Print levels follow the sysarch-acs convention:
 * - lower value == more verbose
 * - default should be ACS_PRINT_TEST (normal regression flow)
 */
#define ACS_PRINT_ERR   5U
#define ACS_PRINT_WARN  4U
#define ACS_PRINT_TEST  3U
#define ACS_PRINT_DEBUG 2U
#define ACS_PRINT_INFO  1U

/* FIRME logger levels (matches ACS_PRINT_* ordering). */
typedef enum {
    TRACE = ACS_PRINT_INFO,
    DEBUG = ACS_PRINT_DEBUG,
    TEST = ACS_PRINT_TEST,
    WARN = ACS_PRINT_WARN,
    ERROR = ACS_PRINT_ERR,
    FATAL = 6,
} print_verbosity_t;

/* Initialize/retarget logging to the given UART base address. */
void val_log_init(uintptr_t uart_base);

/**
 *   @brief    - Get the UART base currently used for logging.
 *   @return   - UART base address, or 0 if not initialized yet.
**/
uintptr_t val_log_get_uart_base(void);

/*
 * World tag used for the "ACS(<tag>):" prefix.
 *
 * Provided as a weak symbol in the logger and overridden by each payload
 * (NS-EL2, S-EL2/S-EL1, R-EL2).
 */
const char *val_world_tag(void);

/* Set runtime verbosity threshold (default: INFO). */
void val_log_set_verbosity(print_verbosity_t verbosity);
print_verbosity_t val_log_get_verbosity(void);

/* printf-style logging, filtered by verbosity. */
uint32_t val_printf(print_verbosity_t verbosity, const char *fmt, ...);
uint32_t val_vprintf(print_verbosity_t verbosity, const char *fmt, va_list ap);

/*
 * Tagged logging helper.
 *
 * Prefix policy:
 * - TRACE/TEST: prints message as-is (no world tag, no level prefix).
 * - DEBUG/WARN/ERROR/FATAL: prints `ACS(<world>):` + indentation + level prefix, then message.
 */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *fmt, ...);
uint32_t val_vlogf(print_verbosity_t verbosity, const char *tag, const char *fmt, va_list ap);

/* Convenience macros for consistent callsites. */
#define LOG_TRACE(tag, fmt, ...) val_logf(TRACE, (tag), (fmt), ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt, ...) val_logf(DEBUG, (tag), (fmt), ##__VA_ARGS__)
#define LOG_TEST(tag, fmt, ...)  val_logf(TEST, (tag), (fmt), ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)  val_logf(WARN, (tag), (fmt), ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) val_logf(ERROR, (tag), (fmt), ##__VA_ARGS__)
#define LOG_FATAL(tag, fmt, ...) val_logf(FATAL, (tag), (fmt), ##__VA_ARGS__)

/* Backwards-compat aliases (avoid churn while code is being cleaned up). */
#define LOG_DBG(tag, fmt, ...)  LOG_DEBUG((tag), (fmt), ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...) LOG_TRACE((tag), (fmt), ##__VA_ARGS__)
#define LOG_ERR(tag, fmt, ...)  LOG_ERROR((tag), (fmt), ##__VA_ARGS__)
#define LOG_ALWAYS(tag, fmt, ...) LOG_TEST((tag), (fmt), ##__VA_ARGS__)
