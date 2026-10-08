/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "acs_val.h"
#include "acs_pe.h"
#include "acs_mpam.h"
#include "acs_mpam_reg.h"
#include "val_interface.h"

/*
 * Algorithm:
 * 1. Iterate over MSCs that implement ESR and RIS.
 * 2. Save MPAMF_ECR and MPAMCFG_PART_SEL, then clear MPAMF_ESR.ERRCODE.
 * 3. Write RIS_MAX + 1 to MPAMCFG_PART_SEL.RIS.
 * 4. Read back MPAMCFG_PART_SEL and MPAMF_ESR.ERRCODE.
 * 5. Accept either behavior permitted by VKKDP:
 *    - the undefined RIS is retained without an immediate error, or
 *    - the selector is not updated and undefined-RIS error 8 is reported.
 * 6. Restore the selector and error-control state and clear MPAMF_ESR.
 * 7. Pass if every eligible MSC follows one of the permitted behaviors.
 */

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  8
#define TEST_DESC  "Check Undef RIS MPAMCFG_PART_SEL error"
#define TEST_RULE  "VKKDP"

static
void payload(void)
{

    uint32_t msc_index;
    uint32_t status;
    uint32_t pe_index;
    uint32_t partid;
    uint32_t internal;
    uint32_t total_nodes;
    uint32_t mpamf_ecr;
    uint32_t selector_original;
    uint32_t selector_readback;
    uint32_t esr_errcode;
    uint32_t max_ris_index;
    uint32_t undefined_ris;
    uint32_t data;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    /* Validate undefined-RIS handling independently on every eligible MSC. */
    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

        /* ESR support is required to observe a reported undefined-RIS error. */
        if (!val_mpam_msc_supports_esr(msc_index)) {
            val_print(DEBUG, "\n       MSC index %d does not support ESR", msc_index);
            continue;
        }

        /* The rule applies only when resource instance selection is supported. */
        if (!val_mpam_msc_supports_ris(msc_index)) {
            val_print(DEBUG,
                        "\n       MSC index %d does not support RIS", msc_index);
            continue;
        }

        /* RIS_MAX + 1 must fit in the architectural selector field. */
        max_ris_index = val_mpam_get_max_ris_count(msc_index);
        if (max_ris_index == BITFIELD_READ(PART_SEL_RIS, ACS_UINT32_MAX)) {
            val_print(DEBUG, "\n       MSC index %d has no undefined RIS value", msc_index);
            continue;
        }

        /* Preserve selector and error state before provoking the error condition. */
        mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);
        selector_original = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
        status    = val_mpam_msc_reset_errcode(msc_index);

        if (!status) {
            val_set_status(pe_index, RESULT_FAIL(01));
            return;
        }

        test_skip = 0;

        /* Keep PARTID valid so the programmed RIS is the only invalid selector. */
        internal = val_mpam_msc_supports_partid_nrw(msc_index);
        partid = internal ? 0 : val_mpam_get_max_partid(msc_index);
        undefined_ris = max_ris_index + 1U;

        data = BITFIELD_SET(PART_SEL_PARTID_SEL, partid) |
               BITFIELD_SET(PART_SEL_RIS, undefined_ris) |
               BITFIELD_SET(PART_SEL_INTERNAL, internal);

        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);
        val_print(DEBUG, "\n       Value written to MPAMCFG_PART_SEL is %llx", data);

        /* Read back the selector to determine whether the write was accepted. */
        selector_readback = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);

        /* Wait for some time for the error to be reflected in MPAMF_ESR */
        val_time_delay_ms(100 * ONE_MILLISECOND);

        /* Read Error Status Register and check if the error code is recorded */
        esr_errcode = val_mpam_msc_get_errcode(msc_index);
        val_print(DEBUG, "\n       Error code read is %llx", esr_errcode);

        /* Classify the result against the two implementation-defined VKKDP behaviors. */
        if ((BITFIELD_READ(PART_SEL_RIS, selector_readback) == undefined_ris) &&
            (esr_errcode == ESR_ERRCODE_NO_ERROR)) {
            val_print(DEBUG, "\n       Undefined RIS accepted without checking");
        } else if ((selector_readback == selector_original) &&
                   (esr_errcode == ESR_ERRCODE_UNDEF_RIS_PART_SEL)) {
            val_print(DEBUG, "\n       Undefined RIS rejected and error reported");
        } else {
            val_print(ERROR, "\n       Undefined RIS handling does not match a permitted behavior");
            val_print(ERROR, "\n       Selector readback: 0x%x, errcode: %d",
                      selector_readback, esr_errcode);
            test_fail++;
        }

        /* Restore the selector and clear any error before testing the next MSC. */
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, selector_original);
        if (!val_mpam_msc_reset_errcode(msc_index)) {
            val_print(ERROR, "\n       Failed to clear MPAMF_ESR for MSC index %d", msc_index);
            test_fail++;
        }

        /* Restore the original error-control configuration. */
        val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
    }

    /* Report skip only when no MSC was eligible for this test. */
    if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(01));
    else if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(02));
    else
        val_set_status(pe_index, RESULT_PASS);
    return;
}

uint32_t error008_entry(uint32_t num_pe)
{

    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), NULL);

    return status;
}
