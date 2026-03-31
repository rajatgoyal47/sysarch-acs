/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_common/val_smccc.h"

/**
 *   @brief    - Invoke FIRME_VERSION from the current world.
 *   @param    - out : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_version(firme_res_t *out)
{
    smccc_res_t res;
    val_smccc_smc_call(FIRME_FID_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_FEATURES from the current world.
 *   @param    - feature_id : FIRME feature register index (DEN0149).
 *   @param    - out        : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_features(uint64_t feature_id, firme_res_t *out)
{
    smccc_res_t res;
    val_smccc_smc_call(FIRME_FID_FEATURES, feature_id, 0, 0, 0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_GM_GPI_SET from the current world.
 *   @param    - base_address  : Base PA to transition.
 *   @param    - granule_count : Number of granules (FVP currently supports 1).
 *   @param    - attributes    : Encodes the target GPI (and any future attributes).
 *   @param    - out           : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_gm_gpi_set(uint64_t base_address, uint64_t granule_count, uint64_t attributes,
                          firme_res_t *out)
{
    smccc_res_t res;
    val_smccc_smc_call(FIRME_FID_GM_GPI_SET, base_address, granule_count, attributes,
                       0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}
