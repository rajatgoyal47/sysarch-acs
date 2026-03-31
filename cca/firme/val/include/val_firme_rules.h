/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_common/val_status.h"

/*
 * FIRME rule IDs map 1:1 to tests (DEN0149).
 * Keep IDs stable so logs and automation can key off them.
 */
typedef enum {
    FIRME_RULE_R0085 = 85,
    FIRME_RULE_R0086 = 86,
    FIRME_RULE_R0089 = 89,
} firme_rule_id_t;

typedef struct {
    uint64_t status;     /* ACS_STATUS_PASS/FAIL/SKIP */
    uint64_t fail_step;  /* step index for debugging */
    uint64_t last_x0;    /* last FIRME x0 (status) observed */
    uint64_t last_x1;    /* last FIRME x1 (count) observed */
} firme_rule_res_t;

int val_firme_rule_run_secure(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out);
int val_firme_rule_run_realm(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out);
int val_firme_rule_run_ns(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out);
