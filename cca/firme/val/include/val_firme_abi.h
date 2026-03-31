/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

/*
 * FIRME ABI FIDs (DEN0149 FIRME specification).
 * These are invoked as SMC calls into EL3 (TF-A BL31).
 */

#define FIRME_FID_VERSION  0xC4000400ULL
#define FIRME_FID_FEATURES 0xC4000401ULL
#define FIRME_FID_GM_GPI_SET 0xC4000402ULL

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
} firme_res_t;

void val_firme_version(firme_res_t *out);
void val_firme_features(uint64_t feature_id, firme_res_t *out);
void val_firme_gm_gpi_set(uint64_t base_address, uint64_t granule_count, uint64_t attributes,
                          firme_res_t *out);
