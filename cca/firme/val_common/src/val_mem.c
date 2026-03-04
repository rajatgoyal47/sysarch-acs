/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_common/val_mem.h"

/**
 *   @brief    - Fill a buffer with a byte value.
 *   @param    - dst : Destination buffer.
 *   @param    - c   : Byte value.
 *   @param    - n   : Number of bytes.
 *   @return   - dst
**/
void *memset(void *dst, int c, size_t n)
{
    unsigned char *p = (unsigned char *)dst;
    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return dst;
}

/**
 *   @brief    - Copy a buffer.
 *   @param    - dst : Destination buffer.
 *   @param    - src : Source buffer.
 *   @param    - n   : Number of bytes.
 *   @return   - dst
**/
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

/**
 *   @brief    - Zero a BSS region using 16-byte stores.
 *   @param    - bss_start : Start address.
 *   @param    - bss_end   : End address (exclusive).
 *   @return   - void
**/
void val_zero_bss(uint64_t bss_start, uint64_t bss_end)
{
    for (uint64_t p = bss_start; p < bss_end; p += 16) {
        uint64_t *p64 = (uint64_t *)(uintptr_t)p;
        p64[0] = UINT64_C(0);
        p64[1] = UINT64_C(0);
    }
}
