/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

void secure_vectors_el2(void);
void secure_vectors_el1(void);
void secure_enter_el1(void *ctx, uint64_t sp_el1_top);
void secure_enter_el1_return(void);

