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
 * 1. Iterate over MSCs that implement error reporting.
 * 2. Find an MPAMCFG_* register that can be accessed with a valid selector.
 * 3. Save the selector, error-control register, and selected register value.
 * 4. Program PARTID_MAX + 1 into MPAMCFG_PART_SEL.PARTID_SEL.
 * 5. Continue only if the out-of-range PARTID selector is retained.
 * 6. Clear any selector-write error and write the saved MPAMCFG_* value.
 * 7. Accept either no error or an out-of-range PARTID error.
 * 8. Restore the selector and error state.
 */

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE + 3
#define TEST_DESC  "Check MPAMCFG write with out-of-range PARTID"
#define TEST_RULE  "ZWRQW"

static uint32_t
find_config_register(uint32_t msc_index, uint32_t valid_partid,
                     uint32_t selector_original, uint32_t *config_reg,
                     uint32_t *config_value, uint32_t *selector_valid)
{
    uint32_t data;
    uint32_t has_ris;
    uint32_t ris;
    uint32_t rsrc_count;
    uint32_t rsrc_index;

    /* Narrowing mappings are configured using a request PARTID selector. */
    if (val_mpam_msc_supports_partid_nrw(msc_index)) {
        data = BITFIELD_WRITE(selector_original, PART_SEL_PARTID_SEL, valid_partid);
        data = BITFIELD_WRITE(data, PART_SEL_INTERNAL, 0);
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);

        *config_reg = REG_MPAMCFG_INTPARTID;
        *config_value = val_mpam_mmr_read(msc_index, *config_reg);
        *selector_valid = data;
        return ACS_STATUS_PASS;
    }

    /* Otherwise, choose one implemented partition-control register. */
    has_ris = val_mpam_msc_supports_ris(msc_index);
    rsrc_count = has_ris ? val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0) : 1;
    if ((rsrc_count == 0) || (rsrc_count == MPAM_INVALID_INFO))
        return ACS_STATUS_SKIP;

    for (rsrc_index = 0; rsrc_index < rsrc_count; rsrc_index++) {
        ris = has_ris ? val_mpam_get_info(MPAM_MSC_RSRC_RIS, msc_index, rsrc_index) :
                        BITFIELD_READ(PART_SEL_RIS, selector_original);
        data = BITFIELD_WRITE(selector_original, PART_SEL_PARTID_SEL, valid_partid);
        data = BITFIELD_WRITE(data, PART_SEL_INTERNAL, 0);
        data = BITFIELD_WRITE(data, PART_SEL_RIS, ris);
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);

        if (val_mpam_supports_ccap(msc_index))
            *config_reg = REG_MPAMCFG_CMAX;
        else if (val_mpam_supports_cpor(msc_index))
            *config_reg = REG_MPAMCFG_CPBM;
        else if (val_mpam_msc_supports_mbw_max(msc_index))
            *config_reg = REG_MPAMCFG_MBW_MAX;
        else
            continue;

        *config_value = val_mpam_mmr_read(msc_index, *config_reg);
        *selector_valid = data;
        return ACS_STATUS_PASS;
    }

    return ACS_STATUS_SKIP;
}

static void
payload(void)
{
    uint32_t msc_index;
    uint32_t status;
    uint32_t pe_index;
    uint32_t total_nodes;
    uint32_t mpamf_ecr;
    uint32_t esr_errcode;
    uint32_t selector_original;
    uint32_t selector_valid;
    uint32_t selector_test;
    uint32_t selector_readback;
    uint32_t config_reg;
    uint32_t config_value;
    uint32_t max_partid;
    uint32_t out_of_range_partid;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    /* Validate the out-of-range configuration-write behavior on each MSC. */
    for (msc_index = 0; msc_index < total_nodes; msc_index++) {
        if (!val_mpam_msc_supports_esr(msc_index)) {
            val_print(DEBUG, "\n       MSC index %d does not support ESR", msc_index);
            continue;
        }

        max_partid = val_mpam_get_max_partid(msc_index);
        if (max_partid == BITFIELD_READ(PART_SEL_PARTID_SEL, ACS_UINT32_MAX)) {
            val_print(DEBUG, "\n       MSC index %d has no out-of-range PARTID value",
                      msc_index);
            continue;
        }
        out_of_range_partid = max_partid + 1U;

        /* Preserve state before selecting a register used by the test. */
        mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);
        selector_original = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
        status = find_config_register(msc_index, max_partid, selector_original,
                                      &config_reg, &config_value, &selector_valid);
        if (status == ACS_STATUS_SKIP) {
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, selector_original);
            val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
            continue;
        }

        /* Clear stale errors before programming the out-of-range PARTID. */
        if (!val_mpam_msc_reset_errcode(msc_index)) {
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, selector_original);
            val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
            test_skip = 0;
            test_fail++;
            continue;
        }

        selector_test = BITFIELD_WRITE(selector_valid, PART_SEL_PARTID_SEL,
                                       out_of_range_partid);
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, selector_test);
        selector_readback = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);

        /* Skip when the implementation rejects or truncates the selector. */
        if (BITFIELD_READ(PART_SEL_PARTID_SEL, selector_readback) !=
            out_of_range_partid) {
            val_print(DEBUG, "\n       Out-of-range PARTID was not retained for MSC %d",
                      msc_index);
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, selector_original);
            val_mpam_msc_reset_errcode(msc_index);
            val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
            continue;
        }

        /* Isolate the error, if any, raised by the MPAMCFG_* write. */
        if (!val_mpam_msc_reset_errcode(msc_index)) {
            test_fail++;
            goto cleanup;
        }

        test_skip = 0;
        val_mpam_mmr_write(msc_index, config_reg, config_value);
        val_time_delay_ms(100 * ONE_MILLISECOND);
        esr_errcode = val_mpam_msc_get_errcode(msc_index);
        val_print(DEBUG, "\n       Error code read is %d", esr_errcode);

        /* Detection is optional, but a detected out-of-range PARTID reports error 1. */
        if ((esr_errcode != ESR_ERRCODE_NO_ERROR) &&
            (esr_errcode != ESR_ERRCODE_PARTID_SEL_RANGE)) {
            val_print(ERROR, "\n       Expected errcode: %d or no error",
                      ESR_ERRCODE_PARTID_SEL_RANGE);
            val_print(ERROR, "\n       Actual errcode: %d", esr_errcode);
            test_fail++;
        }

cleanup:
        /* The target PARTID is unknown, so only selector and error state can be restored. */
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, selector_original);
        if (!val_mpam_msc_reset_errcode(msc_index))
            test_fail++;
        val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
    }

    if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(02));
    else if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(01));
    else
        val_set_status(pe_index, RESULT_PASS);
}

uint32_t
error003_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    num_pe = 1;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), NULL);

    return status;
}
