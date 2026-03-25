/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_common/val_mmu.h"

/* Minimal architectural setup for tests (FP/SIMD access, etc.). */
void val_arch_init_basic(mmu_el_t el);
