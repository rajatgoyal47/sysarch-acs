/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_rules.h"

#include "val_log.h"
#include "val_firme_suite.h"

/**
 *   @brief    - Dispatch a rule test implementation for Secure world execution.
 *   @param    - rule    : Rule ID.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
**/
int val_firme_rule_run_secure(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    const firme_test_case_t *tc = val_firme_suite_find(rule);
    if (tc && tc->run_secure)
        return tc->run_secure(base_pa, out);
    if (out) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
    return 0;
}

/**
 *   @brief    - Dispatch a rule test implementation for Realm world execution.
 *   @param    - rule    : Rule ID.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
**/
int val_firme_rule_run_realm(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    const firme_test_case_t *tc = val_firme_suite_find(rule);
    if (tc && tc->run_realm)
        return tc->run_realm(base_pa, out);
    if (out) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
    return 0;
}

/**
 *   @brief    - Dispatch a rule test implementation for NS world execution.
 *   @param    - rule    : Rule ID.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int val_firme_rule_run_ns(firme_rule_id_t rule, uint64_t base_pa, firme_rule_res_t *out)
{
    const firme_test_case_t *tc = val_firme_suite_find(rule);

    LOG_DEBUG(NULL, "dispatch ns rule=%u base=0x%llx",
              (unsigned int)rule, (unsigned long long)base_pa);
    if (tc && tc->run_ns)
        return tc->run_ns(base_pa, out);
    if (out) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
    return 0;
}
