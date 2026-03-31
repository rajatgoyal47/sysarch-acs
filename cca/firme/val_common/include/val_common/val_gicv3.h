/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/*
 * Minimal GICv3 CPU interface bring-up using system registers.
 * This assumes the platform firmware has already configured the GICD/GICR.
 */
void val_gicv3_cpuif_enable(void);
uint32_t val_gicv3_acknowledge_irq(void);
void val_gicv3_end_of_irq(uint32_t intid);
