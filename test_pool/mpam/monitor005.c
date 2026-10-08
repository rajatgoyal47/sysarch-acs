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
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   ACS_MPAM_MONITOR_TEST_NUM_BASE + 5
#define TEST_RULE  ""
#define TEST_DESC  "Check MSMON CTL Disable Behavior      "

#define BUFFER_SIZE           0x101000   /* 1MB */

static void
payload(void)
{
    uint32_t msc_cnt  = val_mpam_get_msc_count();
    uint32_t llc_idx  = val_cache_get_llc_index();
    uint32_t index    = val_pe_get_index_mpid(val_pe_get_mpid());

    uint32_t start_count = 0;
    uint32_t end_count   = 0;
    uint32_t test_fail   = 0;
    uint16_t test_partid = 0;
    uint32_t max_partid  = 0;
    void     *src_buf    = 0;
    void     *dest_buf   = 0;
    uint32_t msc_index, rsrc_index, rsrc_cnt;
    uint64_t nrdy_timeout;
    uint64_t cache_identifier;
    uint64_t saved_el2;
    uint32_t test_skip   = 1;
    uint32_t status      = 0;
    uint32_t num_mon     = 0;
    uint32_t page_size;
    uint32_t num_pages;
    uint32_t saved_csu_ctl;
    uint32_t saved_csu_flt;
    uint32_t saved_mon_sel;
    uint32_t mon_sel;
    uint32_t csu_ctl;
    uint32_t csu_flt;
    uint16_t test_flt_partid;
    uint32_t saved_part_sel;
    uint32_t saved_resource_mon_sel;
    uint16_t saved_mapping;
    uint32_t mapping_saved;
    uint32_t restore_status;
    bool saved_enable;
    bool enable_state_saved;
    MPAM_PARTITION_STATE saved_partition;

    /* Check if LLC is valid */
    if (llc_idx == CACHE_TABLE_EMPTY) {
        val_print(DEBUG, "\n       No LLC found, skipping test");
        val_set_status(index, RESULT_SKIP(1));
        return;
    }

    /* Get the LLC Cache ID */
    cache_identifier = val_cache_get_info(CACHE_ID, llc_idx);
    if (cache_identifier == INVALID_CACHE_INFO) {
        val_print(DEBUG, "\n       Invalid LLC ID, skipping test");
        val_set_status(index, RESULT_SKIP(2));
        return;
    }

    page_size = val_memory_page_size();
    if (page_size == 0U) {
        val_print(ERROR, "\n       Invalid memory page size");
        val_set_status(index, RESULT_FAIL(1));
        return;
    }
    num_pages = (BUFFER_SIZE / page_size) +
                ((BUFFER_SIZE % page_size) != 0U);

    /* Iterate through all MSCs and get the minmax PARTID -> the test PARTID */
    for (msc_index = 0; msc_index < msc_cnt; msc_index++) {
        max_partid = val_mpam_get_max_partid(msc_index);
        if (max_partid == 0) {
            val_print(DEBUG, "\n       MSC %d has no PARTID support", msc_index);
            continue;
        }

        test_partid = (test_partid == 0) ?
                      (max_partid - 1) :
                      GET_MIN_VALUE(test_partid, max_partid - 1);
    }

    val_print(DEBUG, "\n       Selected PARTID = %d", test_partid);

    /* Iterate through all available LLC MSCs and their resources */
    for (msc_index = 0; msc_index < msc_cnt; msc_index++) {

        rsrc_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        for (rsrc_index = 0; rsrc_index < rsrc_cnt; rsrc_index++) {

            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index)
                != MPAM_RSRC_TYPE_PE_CACHE)
                continue;

            if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index)
                != cache_identifier)
                continue;

            num_mon     = 0;
            start_count = 0;
            end_count   = 0;

            /* Preserve selectors before selecting this resource instance. */
            saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
            saved_resource_mon_sel = val_mpam_mmr_read(msc_index,
                                                       REG_MSMON_CFG_MON_SEL);

            /* Select resource instance if RIS feature implemented */
            if (val_mpam_msc_supports_ris(msc_index))
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            /* Check if MSC implements CSU monitors */
            if (val_mpam_supports_csumon(msc_index))
                num_mon = val_mpam_get_csumon_count(msc_index);

            if (num_mon == 0) {
                val_print(DEBUG,
                    "\n       MSC %d does not have CSU monitors, skipping MSC", msc_index);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL,
                                   saved_resource_mon_sel);
                continue;
            }

            mapping_saved = 0;
            if (val_mpam_msc_supports_partid_nrw(msc_index)) {
                saved_mapping = val_mpam_get_partid_mapping(msc_index, test_partid);
                mapping_saved = 1;
            }

            /* Preserve the request PARTID enable state before preparation. */
            enable_state_saved = val_mpam_msc_supports_partid_endis(msc_index);
            saved_enable = enable_state_saved ?
                           val_mpam_msc_partid_is_enabled(msc_index, test_partid) : true;

            /* Prepare the request PARTID for narrowed or non-narrowed controls. */
            if (val_mpam_prepare_partid(msc_index, test_partid, 0) != ACS_STATUS_PASS) {
                if (mapping_saved)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
                if (enable_state_saved)
                    (void)val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                     0, test_partid);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL,
                                   saved_resource_mon_sel);
                continue;
            }

            /* Preserve the effective partition controls before changing them. */
            if (!val_mpam_save_partition_state(msc_index, test_partid,
                                                &saved_partition)) {
                if (mapping_saved)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
                if (enable_state_saved)
                    (void)val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                     0, test_partid);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL,
                                   saved_resource_mon_sel);
                test_fail++;
                continue;
            }

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid, 0, 100);

            /* Allocate memory for source and destination buffers */
            src_buf  = (void *)val_memory_alloc_pages(num_pages);
            dest_buf = (void *)val_memory_alloc_pages(num_pages);

            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ERROR, "\n       Mem allocation failed");
                val_set_status(index, RESULT_FAIL(01));
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                val_mpam_restore_partition_state(msc_index, &saved_partition);
                if (mapping_saved)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
                if (enable_state_saved)
                    (void)val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                     0, test_partid);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL,
                                   saved_resource_mon_sel);
                return;
            }

            /* Test runs on atleast one MSC */
            test_skip = 0;

            /* Select monitor 0 and save its configuration. */
            saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
            mon_sel = BITFIELD_WRITE(saved_mon_sel, MON_SEL_MON_SEL, 0);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, mon_sel);
            saved_csu_ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_CSU_CTL);
            saved_csu_flt = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_CSU_FLT);

            /* Configure CSU Monitor. This leaves the selected monitor disabled. */
            val_mpam_configure_csu_mon(msc_index, test_partid, DEFAULT_PMG, 0);

            /* Change CTL fields and verify that CTL is writable while EN is zero. */
            csu_ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_CSU_CTL);
            csu_ctl = BITFIELD_WRITE(csu_ctl, CSU_CTL_EN, 0);
            csu_ctl = BITFIELD_WRITE(csu_ctl, CSU_CTL_MATCH_PARTID, 0);
            csu_ctl = BITFIELD_WRITE(csu_ctl, CSU_CTL_MATCH_PMG, 0);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_CSU_CTL, csu_ctl);
            csu_ctl = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_CSU_CTL);
            if ((BITFIELD_READ(CSU_CTL_EN, csu_ctl) != 0) ||
                (BITFIELD_READ(CSU_CTL_MATCH_PARTID, csu_ctl) != 0) ||
                (BITFIELD_READ(CSU_CTL_MATCH_PMG, csu_ctl) != 0)) {
                val_print(ERROR, "\n       CSU CTL update failed while monitor disabled");
                test_fail++;
            }

            /* Change FLT fields and verify that FLT is writable while EN is zero. */
            max_partid = val_mpam_get_max_partid(msc_index);
            test_flt_partid = (max_partid == 0U) ? test_partid :
                              ((test_partid == 0U) ? 1U : (test_partid - 1U));
            csu_flt = BITFIELD_SET(CSU_FLT_PARTID, test_flt_partid) |
                      BITFIELD_SET(CSU_FLT_PMG, DEFAULT_PMG);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_CSU_FLT, csu_flt);
            csu_flt = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_CSU_FLT);
            if ((BITFIELD_READ(CSU_FLT_PARTID, csu_flt) != test_flt_partid) ||
                (BITFIELD_READ(CSU_FLT_PMG, csu_flt) != DEFAULT_PMG)) {
                val_print(ERROR, "\n       CSU FLT update failed while monitor disabled");
                test_fail++;
            }

            /* Restore the disabled monitor setup used by the workload check. */
            val_mpam_configure_csu_mon(msc_index, test_partid, DEFAULT_PMG, 0);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Save the current MPAM2_EL2 settings */
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);

            /* Program MPAM2_EL2 with test_partid and default PMG */
            status = val_mpam_program_el2(test_partid, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                /* Restore the selected monitor before leaving on setup failure. */
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_CSU_FLT, saved_csu_flt);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_CSU_CTL, saved_csu_ctl);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_mpam_reg_write(MPAM2_EL2, saved_el2);
                val_mpam_restore_partition_state(msc_index, &saved_partition);
                if (mapping_saved)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
                restore_status = 0;
                if (enable_state_saved)
                    restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                                 0, test_partid);
                if (restore_status)
                    val_print(ERROR, "\n       Failed to restore test PARTID enable state");
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL,
                                   saved_resource_mon_sel);
                /* Free the buffers to the heap manager */
                if (dest_buf != NULL) {
                    val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
                    val_mem_issue_dsb();
                    val_memory_free_pages(dest_buf, num_pages);
                }
                if (src_buf != NULL) {
                    val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
                    val_mem_issue_dsb();
                    val_memory_free_pages(src_buf, num_pages);
                }
                val_set_status(index, RESULT_FAIL(02));
                return;
            }

            /* Add Delay */
            val_time_delay_ms(100 * ONE_MILLISECOND);

            start_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);
            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            /* Add Delay */
            val_time_delay_ms(100 * ONE_MILLISECOND);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       End Count = 0x%lx", end_count);

            if (start_count != end_count) {
                val_print(ERROR, "\n       Unexpected cache usage behavior observed");
                val_print(ERROR, "\n       Start count = 0x%lx", start_count);
                val_print(ERROR, "\n       End count  = 0x%lx", end_count);
                test_fail++;
            }

            /* Restore the selected monitor configuration. */
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_CSU_FLT, saved_csu_flt);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_CSU_CTL, saved_csu_ctl);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);

            /* Restore MPAM2_EL2 settings */
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Restore partition controls, mapping, enable state, and selectors. */
            val_mpam_restore_partition_state(msc_index, &saved_partition);
            if (mapping_saved)
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
            restore_status = 0;
            if (enable_state_saved)
                restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                             0, test_partid);
            if (restore_status)
                test_fail++;
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL,
                               saved_resource_mon_sel);

            /* Free the buffers to the heap manager */
            if (dest_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
                val_mem_issue_dsb();
                val_memory_free_pages(dest_buf, num_pages);
            }
            if (src_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
                val_mem_issue_dsb();
                val_memory_free_pages(src_buf, num_pages);
            }
        }
    }

    if (test_fail)
        val_set_status(index, RESULT_FAIL(3));
    else if (test_skip)
        val_set_status(index, RESULT_SKIP(3));
    else
        val_set_status(index, RESULT_PASS);

    return;
}

uint32_t
monitor005_entry(uint32_t num_pe)
{
    uint32_t status  = ACS_STATUS_FAIL;

    num_pe = 1;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    /* Check if test needs to be skipped - based on user configuration */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* Get the result from PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
