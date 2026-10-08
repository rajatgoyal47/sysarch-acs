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
#include "acs_memory.h"
#include "acs_mpam.h"
#include "acs_mpam_reg.h"
#include "val_interface.h"

/*
 * Algorithm:
 * 1. Iterate over MSCs that implement error reporting and resource monitors.
 * 2. Save the partition and monitor selectors.
 * 3. Iterate over each resource instance supported by the MSC.
 * 4. For each supported monitor type, obtain the implemented monitor count.
 * 5. Select monitor 0, clear the error status, and write the monitor count as
 *    the first out-of-range monitor selector.
 * 6. Read back the selector and error status.
 * 7. Accept the selector being retained without an error, rejected with a
 *    monitor-range error, or truncated at the implemented selector width.
 * 8. Restore the selectors and clear the error status.
 */

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  5
#define TEST_DESC  "Check MSC MONITOR selection range Err "
#define TEST_RULE  "VMTBP"

static uint32_t
check_monitor_selector(uint32_t msc_index, uint32_t mon_count)
{
    uint32_t selector_original;
    uint32_t selector_base;
    uint32_t selector_test;
    uint32_t selector_readback;
    uint32_t esr_errcode;
    uint32_t status = ACS_STATUS_PASS;

    /* The monitor count must identify an out-of-range value representable by the field. */
    if ((mon_count == 0) || (mon_count > BITFIELD_READ(MON_SEL_MON_SEL, ACS_UINT32_MAX)))
        return ACS_STATUS_SKIP;

    /* Save the selector before selecting a known valid monitor instance. */
    selector_original = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
    val_print(DEBUG, "\n       RIS: %d, monitor count: %d",
              BITFIELD_READ(MON_SEL_RIS, selector_original), mon_count);
    selector_base = BITFIELD_WRITE(selector_original, MON_SEL_MON_SEL, 0);
    val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, selector_base);

    /* Clear stale errors before programming the out-of-range selector. */
    if (!val_mpam_msc_reset_errcode(msc_index)) {
        status = ACS_STATUS_FAIL;
        goto cleanup;
    }

    /* Program the first selector beyond the implemented monitor range. */
    selector_test = BITFIELD_WRITE(selector_base, MON_SEL_MON_SEL, mon_count);
    val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, selector_test);
    val_mem_issue_dsb();

    /* Read back the selector and any error reported by the MSC. */
    selector_readback = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
    val_time_delay_ms(100 * ONE_MILLISECOND);
    esr_errcode = val_mpam_msc_get_errcode(msc_index);

    /* Classify the observed selector and error behavior. */
    if ((selector_readback == selector_test) && (esr_errcode == ESR_ERRCODE_NO_ERROR)) {
        val_print(DEBUG, "\n       Out-of-range monitor selector retained for MSC %d", msc_index);
    } else if ((selector_readback == selector_base) &&
               (esr_errcode == ESR_ERRCODE_MON_RANGE)) {
        val_print(DEBUG, "\n       Out-of-range monitor selector rejected for MSC %d", msc_index);
    } else if (((mon_count & (mon_count - 1U)) == 0) &&
               (selector_readback == selector_base) && (esr_errcode == ESR_ERRCODE_NO_ERROR)) {
        val_print(DEBUG, "\n       Monitor selector boundary truncated for MSC %d", msc_index);
        status = ACS_STATUS_SKIP;
    } else {
        val_print(ERROR, "\n       Invalid monitor selector handling for MSC %d", msc_index);
        val_print(ERROR, "\n       Selector readback: 0x%x, errcode: %d",
                  selector_readback, esr_errcode);
        status = ACS_STATUS_FAIL;
    }

cleanup:
    /* Restore the original selector and leave the error status clear. */
    val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, selector_original);
    if (!val_mpam_msc_reset_errcode(msc_index))
        status = ACS_STATUS_FAIL;

    return status;
}

static
void payload(void)
{
    uint32_t index;
    uint32_t status;
    uint32_t pe_index;
    uint32_t total_nodes;
    uint32_t rsrc_index;
    uint32_t rsrc_count;
    uint32_t has_ris;
    uint32_t part_sel_original;
    uint32_t mon_sel_original;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    /* Validate monitor selector handling on every eligible MSC. */
    for (index = 0; index < total_nodes; index++) {

        /* Error reporting is required to observe a rejected selector. */
        if (!val_mpam_msc_supports_esr(index)) {
            val_print(DEBUG, "\n       MSC index %d does not support ESR", index);
            continue;
        }

        /* At least one monitor type is required to obtain a monitor count. */
        if (!val_mpam_msc_supports_mon(index)) {
            val_print(DEBUG,
                "\n       MSC %d does not implement any MSMON. Skipping MSC..", index);
            continue;
        }

        /* Preserve selector state and determine the resources to examine. */
        part_sel_original = val_mpam_mmr_read(index, REG_MPAMCFG_PART_SEL);
        mon_sel_original = val_mpam_mmr_read(index, REG_MSMON_CFG_MON_SEL);
        has_ris = val_mpam_msc_supports_ris(index);
        rsrc_count = has_ris ? val_mpam_get_info(MPAM_MSC_RSRC_COUNT, index, 0) : 1;

        for (rsrc_index = 0; rsrc_index < rsrc_count; rsrc_index++) {
            /* Select the current resource when resource selection is implemented. */
            if (has_ris)
                val_mpam_memory_configure_ris_sel(index, rsrc_index);

            /* Check the selector boundary for cache-storage monitors. */
            if (val_mpam_supports_csumon(index)) {
                status = check_monitor_selector(index, val_mpam_get_csumon_count(index));
                if (status != ACS_STATUS_SKIP)
                    test_skip = 0;
                if (status == ACS_STATUS_FAIL)
                    test_fail++;
            }

            /* Check the selector boundary for memory-bandwidth monitors. */
            if (val_mpam_msc_supports_mbwumon(index)) {
                status = check_monitor_selector(index, val_mpam_get_mbwumon_count(index));
                if (status != ACS_STATUS_SKIP)
                    test_skip = 0;
                if (status == ACS_STATUS_FAIL)
                    test_fail++;
            }
        }

        /* Restore the selectors before moving to the next MSC. */
        val_mpam_mmr_write(index, REG_MSMON_CFG_MON_SEL, mon_sel_original);
        val_mpam_mmr_write(index, REG_MPAMCFG_PART_SEL, part_sel_original);
    }

    /* Report the aggregate result from all eligible resources and monitor types. */
    if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(01));
    else if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(02));
    else
        val_set_status(pe_index, RESULT_PASS);
    return;
}

uint32_t error005_entry(uint32_t num_pe)
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
