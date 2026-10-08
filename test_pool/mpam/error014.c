/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#define TEST_NUM   (ACS_MPAM_ERROR_TEST_NUM_BASE + 17)
#define TEST_DESC  "Check MPAM error overwrite behavior   "
#define TEST_RULE  ""

/*
 * Test algorithm:
 * 1. Save MPAM2_EL2 and program valid default PARTID and PMG labels.
 * 2. For each MSC with ESR support, save ECR and clear stale error status.
 * 3. Generate a PARTID selector range error and verify ERRCODE=1 and OVRWR=0.
 * 4. Select one implemented monitor type and generate a monitor configuration error.
 * 5. Without clearing ESR, verify ERRCODE changes to the second error and OVRWR is set.
 * 6. Restore MPAM2_EL2, clear ESR, restore ECR, and report the aggregate result.
 */

static void
payload(void)
{
    uint32_t msc_index;
    uint32_t mon_count;
    uint32_t status;
    uint32_t mpamf_ecr;
    uint32_t esr_errcode;
    uint32_t first_errcode;
    uint32_t part_sel;
    uint32_t mon_sel;
    uint32_t mon_test_sel;
    uint32_t mon_filter;
    uint32_t use_csumon;
    /* Track which per-MSC state must be restored on every exit path. */
    uint32_t part_sel_saved;
    uint32_t mon_state_saved;
    uint64_t mpam2_el2_temp;

    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint32_t pe_index  = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t msc_cnt   = val_mpam_get_msc_count();

    /* Save MPAM2_EL2 and prevent unrelated request-label errors during the test. */
    mpam2_el2_temp = val_mpam_reg_read(MPAM2_EL2);
    status = val_mpam_program_el2(0, DEFAULT_PMG);
    if (status) {
        val_print(ERROR, "\n       Failed to program valid MPAM2_EL2 labels");
        val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);
        val_set_status(pe_index, RESULT_FAIL(01));
        return;
    }

    for (msc_index = 0; msc_index < msc_cnt; msc_index++)
    {
        /* No selector state has been saved for this MSC yet. */
        part_sel_saved = 0;
        mon_state_saved = 0;

        /* Check if Error Status Register Supported */
        if (!val_mpam_msc_supports_esr(msc_index)) {
            val_print(DEBUG, "\n       MSC index %d does not support ESR", msc_index);
            continue;
        }

        /* Read MPAMF_ECR before generating error. This will be used to restore to default later */
        mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);
        status    = val_mpam_msc_reset_errcode(msc_index);

        if (!status) {
            test_fail++;
            goto restore_msc_state;
        }

        mon_count = 0;
        use_csumon = 0;
        /* Select one monitor type because CSU and MBWU instance ranges are independent. */
        if (val_mpam_msc_supports_mon(msc_index)) {
            if (val_mpam_supports_csumon(msc_index)) {
                mon_count = val_mpam_get_csumon_count(msc_index);
                use_csumon = 1;
                val_print(DEBUG,
                            "\n       MSC implements %d CSU Monitors", mon_count);
            } else if (val_mpam_msc_supports_mbwumon(msc_index)) {
                mon_count = val_mpam_get_mbwumon_count(msc_index);
                val_print(DEBUG,
                            "\n       MSC implements %d MBWU Monitors", mon_count);
            }
        }

        /* CSU/MBWU monitors are required to generate the second error.
           Skip MSC if mon not present */
        if (mon_count == 0) {
            val_print(WARN, "\n       Cannot generate second error. Skipping MSC");
            goto restore_msc_state;
        }

        /* Generate PARTID selection range error (program MPAMCFG_PART_SEL with out-of-range PID) */
        part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
        part_sel_saved = 1;
        status = val_mpam_msc_generate_psr_error(msc_index);
        if (status == ACS_STATUS_SKIP)
            goto restore_msc_state;

        /* Wait for some time for the error to be reflected in MPAMF_ESR */
        val_time_delay_ms(100 * ONE_MILLISECOND);

        /* Read Error Status Register and check if the error code is recorded */
        esr_errcode = val_mpam_msc_get_errcode(msc_index);
        first_errcode = esr_errcode;
        val_print(DEBUG, "\n       Scenario 1 - Error code read is %d", esr_errcode);

        if (esr_errcode != ESR_ERRCODE_PARTID_SEL_RANGE)
        {
            val_print(ERROR, "\n       Expected errcode: %d", ESR_ERRCODE_PARTID_SEL_RANGE);
            val_print(ERROR, "\n       Actual errcode: %d", esr_errcode);
            test_fail++;
        }

        /* Check OVRWR bit - should be 0 for first error */
        if (val_mpam_msc_get_esr_ovrwr(msc_index))
        {
            val_print(ERROR, "\n       OVRWR bit is set for first error");
            test_fail++;
        }

        /* Preserve the filter of the monitor that the error helper will modify. */
        mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
        mon_test_sel = BITFIELD_WRITE(mon_sel, MON_SEL_MON_SEL, (mon_count - 1));
        val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, mon_test_sel);
        mon_filter = val_mpam_mmr_read(msc_index, use_csumon ?
                                       REG_MSMON_CFG_CSU_FLT : REG_MSMON_CFG_MBWU_FLT);
        mon_state_saved = 1;

        /* Generate MSMON config ID out-of-range error. */
        status = val_mpam_msc_generate_msmon_config_error(msc_index, mon_count);
        if (status == ACS_STATUS_SKIP)
            goto restore_msc_state;

        /* Test runs on at least one MSC only when both errors can be generated. */
        test_skip = 0;

        /* Wait for some time for the error to be reflected in MPAMF_ESR */
        val_time_delay_ms(100 * ONE_MILLISECOND);

        /* Read Error Status Register and check if the error code is recorded */
        esr_errcode = val_mpam_msc_get_errcode(msc_index);
        val_print(DEBUG, "\n       Scenario 2 - Error code read is %d", esr_errcode);

        if ((esr_errcode == ESR_ERRCODE_NO_ERROR) || (esr_errcode == first_errcode))
        {
            val_print(ERROR, "\n       ERRCODE was not updated by the second error");
            test_fail++;
        }

        /* Error classification is validated separately from overwrite behavior. */
        if (esr_errcode != ESR_ERRCODE_MSMONCFG_ID_RANGE)
            val_print(WARN, "\n       Monitor configuration stimulus reported errcode: %d",
                              esr_errcode);

        /* Check OVRWR bit - should be 1 for second error */
        if (val_mpam_msc_get_esr_ovrwr(msc_index) == 0)
        {
            val_print(ERROR, "\n       MPAMF_ESR.OVRWR bit is not set for second error");
            test_fail++;
        }

restore_msc_state:
        /* Restore every register saved before an error or skip path. */
        if (part_sel_saved)
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, part_sel);
        if (mon_state_saved) {
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, mon_test_sel);
            val_mpam_mmr_write(msc_index, use_csumon ?
                               REG_MSMON_CFG_CSU_FLT : REG_MSMON_CFG_MBWU_FLT, mon_filter);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, mon_sel);
        }

        /* Clear error state so it cannot affect a later test. */
        if (!val_mpam_msc_reset_errcode(msc_index)) {
            val_print(ERROR, "\n       Failed to clear MPAMF_ESR during cleanup");
            test_fail++;
        }
        val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
    }

    /* Restore the request labels after all MSCs have been tested. */
    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

    if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(02));
    else if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(01));
    else
        val_set_status(pe_index, RESULT_PASS);

    return;
}

uint32_t
error014_entry(uint32_t num_pe)
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
