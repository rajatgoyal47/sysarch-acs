/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);

/* Called from assembly before C runtime is usable. */
void val_zero_bss(uint64_t bss_start, uint64_t bss_end);
