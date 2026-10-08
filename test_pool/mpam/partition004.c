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
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 4
#define TEST_RULE  "BDRCM"
#define TEST_DESC  "Check CASSOC Partitioning             "

/* Test algorithm -
1.  Identify an MSC with cache resources that support associativity partitioning by checking
    MPAMF_IDR.HAS_CCAP_PART is set and MPAMF_CCAP_IDR.HAS_CASSOC is also set.
2.  Select a valid PARTID, for example PARTID_X.
3.  Configure MPAMCFG_CASSOC for PARTID_X to 100% cache associativity
4.  Assign PARTID_X to the running PE using MPAM2_EL2.
5.  Generate a memory workload from the PE to populate the cache and observe full usage of
    available ways.
6.  Measure the cache usage using the CSU monitor and record the value.
7.  Reconfigure MPAMCFG_CASSOC for PARTID_Y to restrict associativity to 50%
    (and later to 25% using PARTID_Z).
8.  Repeat the same memory workload under the same conditions.
9.  Observe whether cache lines allocated to new PARTID are now limited to fewer ways
10. Use the CSU monitor to confirm the reduced cache allocation compared to the
    full associativity setting.
11. If the observed cache usage aligns with the restricted CASSOC setting, confirming enforcement
    of associativity limits, mark the test as passed.
12. If the allocation behavior remains unchanged despite the restriction, indicating
    that the control is not being applied, mark the test as failed.
*/

#define CASSOC_SCENARIO_MAX   3
#define CASSOC_PARTITION_100  100
#define CASSOC_PARTITION_50   50
#define CASSOC_PARTITION_25   25
#define BUFFER_SIZE           0x100000  /* 1MB */

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
    uint64_t counter[CASSOC_SCENARIO_MAX];
    uint64_t saved_el2;
    uint32_t test_skip   = 1;
    uint32_t status      = 0;
    uint32_t num_mon     = 0;
    uint32_t page_size;
    uint32_t num_pages;
    uint64_t num_pages64;
    uint32_t saved_part_sel = 0;
    uint32_t saved_mon_sel = 0;
    uint16_t saved_map_x = 0;
    uint16_t saved_map_y = 0;
    uint16_t saved_map_z = 0;
    uint32_t mappings_saved;
    uint32_t restore_status;
    bool saved_enable_x;
    bool saved_enable_y;
    bool saved_enable_z;
    bool enable_state_saved;
    MPAM_CSUMON_STATE saved_csumon;
    bool csumon_state_saved;
    MPAM_PARTITION_STATE saved_partition_x;
    MPAM_PARTITION_STATE saved_partition_y;
    MPAM_PARTITION_STATE saved_partition_z;
    bool partition_state_saved;

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

    if (test_partid < 2) {
        val_print(DEBUG, "\n       Three consecutive test PARTIDs are required, skipping test");
        val_set_status(index, RESULT_SKIP(3));
        return;
    }
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

            /* Check if the LLC is atleast 4-way set associative.
               Else restricting the cache to 1/4 (25 percent) might result
               in unexpected behavior in the test
            */
            if (val_cache_get_associativity(cache_identifier) < 4) {
                val_print(DEBUG,
                          "\n       LLC is less than 4-way, skipping MSC");
                continue;
            }

            num_mon     = 0;
            start_count = 0;
            end_count   = 0;
            mappings_saved = 0;

            /* Preserve both selectors before selecting a resource instance. */
            saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
            saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
            if (val_mpam_msc_supports_ris(msc_index)) {
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);
            }

            /* Check if MSC supports CASSOC */
            if (!val_mpam_supports_cassoc(msc_index)) {
                val_print(DEBUG,
                    "\n       MSC %d does not support CASSOC partitioning, skipping", msc_index);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }

            /* Check if MSC implements CSU monitors */
            if (val_mpam_supports_csumon(msc_index))
                num_mon = val_mpam_get_csumon_count(msc_index);

            if (num_mon == 0) {
                val_print(DEBUG,
                    "\n       MSC %d does not have CSU monitors, skipping MSC", msc_index);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }

            /* Allocate memory for source and destination buffers */
            page_size = val_memory_page_size();
            if (page_size == 0U) {
                val_print(ERROR, "\n       Invalid memory page size");
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(01));
                return;
            }

            num_pages64 = (BUFFER_SIZE / page_size) +
                          ((BUFFER_SIZE % page_size) != 0U);
            /* Validate the allocator page count before calculating its byte capacity. */
            if ((num_pages64 == 0U) || (num_pages64 > ACS_UINT32_MAX) ||
                (num_pages64 > (~(uint64_t)0 / page_size)) ||
                ((num_pages64 * page_size) < BUFFER_SIZE)) {
                val_print(ERROR, "\n       Invalid page count for workload buffers");
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(01));
                return;
            }
            num_pages = (uint32_t)num_pages64;
            src_buf  = (void *)val_memory_alloc_pages(num_pages);
            dest_buf = (void *)val_memory_alloc_pages(num_pages);

            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ERROR, "\n       Mem allocation failed");
                val_set_status(index, RESULT_FAIL(01));
                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                return;
            }

            if (val_mpam_msc_supports_partid_nrw(msc_index)) {
                if (val_mpam_get_max_intpartid(msc_index) < 2U) {
                    val_memory_free_pages(src_buf, num_pages);
                    val_memory_free_pages(dest_buf, num_pages);
                    val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                    val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                    continue;
                }
                saved_map_x = val_mpam_get_partid_mapping(msc_index, test_partid);
                saved_map_y = val_mpam_get_partid_mapping(msc_index, test_partid - 1);
                saved_map_z = val_mpam_get_partid_mapping(msc_index, test_partid - 2);
                mappings_saved = 1;
            }

            /* Save all request PARTID enable states before preparation. */
            enable_state_saved = val_mpam_msc_supports_partid_endis(msc_index);
            saved_enable_x = enable_state_saved ?
                             val_mpam_msc_partid_is_enabled(msc_index, test_partid) : true;
            saved_enable_y = enable_state_saved ?
                             val_mpam_msc_partid_is_enabled(msc_index, test_partid - 1) : true;
            saved_enable_z = enable_state_saved ?
                             val_mpam_msc_partid_is_enabled(msc_index, test_partid - 2) : true;

            csumon_state_saved = false;
            partition_state_saved = false;
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);

            /* Give each request PARTID a distinct control PARTID when narrowing is supported. */
            if ((val_mpam_prepare_partid(msc_index, test_partid, 0) != ACS_STATUS_PASS) ||
                (val_mpam_prepare_partid(msc_index, test_partid - 1, 1) != ACS_STATUS_PASS) ||
                (val_mpam_prepare_partid(msc_index, test_partid - 2, 2) != ACS_STATUS_PASS)) {
                test_fail++;
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 1, saved_map_y);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 2, saved_map_z);
                }
                restore_status = 0;
                if (enable_state_saved) {
                    restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable_x,
                                                                 0, test_partid);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_y,
                                                                  0, test_partid - 1);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_z,
                                                                  0, test_partid - 2);
                }
                if (restore_status)
                    val_print(ERROR, "\n       Failed to restore PARTID enable state");
                if (csumon_state_saved)
                    val_mpam_restore_csumon_state(msc_index, &saved_csumon);

                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_memory_free_pages(src_buf, num_pages);
                val_memory_free_pages(dest_buf, num_pages);
                continue;
            }

            /* Preserve all effective controls before applying CASSOC scenarios. */
            if (!val_mpam_save_partition_state(msc_index, test_partid,
                                                &saved_partition_x)) {
                test_fail++;
                goto cleanup;
            }
            if (!val_mpam_save_partition_state(msc_index, test_partid - 1,
                                                &saved_partition_y)) {
                test_fail++;
                val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                goto cleanup;
            }
            if (!val_mpam_save_partition_state(msc_index, test_partid - 2,
                                                &saved_partition_z)) {
                test_fail++;
                val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                val_mpam_restore_partition_state(msc_index, &saved_partition_y);
                goto cleanup;
            }
            partition_state_saved = true;

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid, 0, 100);

            /* Start the first scenario with the same cold-buffer state as later scenarios. */
            val_memory_set(dest_buf, BUFFER_SIZE, 0xA5U);
            val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
            val_mem_issue_dsb();

            /* Test runs on atleast one MSC */
            test_skip = 0;

            /* Configure PARTID with 100% cassoc settings */
            val_mpam_configure_cassoc(msc_index, test_partid, CASSOC_PARTITION_100);

            /* Preserve monitor 0 before changing its filter, control, or VALUE. */
            val_mpam_save_csumon_state(msc_index, 0, &saved_csumon);
            csumon_state_saved = true;

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /***  Scenario 1: Buffer copy with 100% CASSOC settings  ***/
            val_print(DEBUG, "\n       Scenario 1: 100 percent CASSOC setting");

            /* Program MPAM2_EL2 with test_partid and default PMG */
            status = val_mpam_program_el2(test_partid, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                val_mpam_csumon_disable(msc_index);
                val_mpam_reg_write(MPAM2_EL2, saved_el2);
                /* Free the buffers to the heap manager */
                val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
                val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
                val_mem_issue_dsb();

                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 1, saved_map_y);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 2, saved_map_z);
                }
                restore_status = 0;
                if (enable_state_saved) {
                    restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable_x,
                                                                 0, test_partid);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_y,
                                                                  0, test_partid - 1);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_z,
                                                                  0, test_partid - 2);
                }
                if (restore_status)
                    val_print(ERROR, "\n       Failed to restore PARTID enable state");
                if (csumon_state_saved)
                    val_mpam_restore_csumon_state(msc_index, &saved_csumon);
                if (partition_state_saved) {
                    val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                    val_mpam_restore_partition_state(msc_index, &saved_partition_y);
                    val_mpam_restore_partition_state(msc_index, &saved_partition_z);
                }

                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(02));
                return;
            }

            start_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);
            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       End Count = 0x%lx", end_count);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Read CSU MON */
            counter[0] = end_count;

            val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
            val_mem_issue_dsb();

            /***  Scenario 2: Buffer copy with 50% CASSOC settings  ***/
            val_print(DEBUG, "\n       Scenario 2: 50 percent CASSOC setting");

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid - 1, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid - 1, 0, 100);

            /* Configure different PARTID with 50% cassoc settings */
            val_mpam_configure_cassoc(msc_index, test_partid - 1, CASSOC_PARTITION_50);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid - 1, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Program MPAM2_EL2 with test_partid-1 and default PMG */
            status = val_mpam_program_el2(test_partid - 1, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                val_mpam_csumon_disable(msc_index);
                val_mpam_reg_write(MPAM2_EL2, saved_el2);
                /* Free the buffers to the heap manager */
                val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
                val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
                val_mem_issue_dsb();

                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 1, saved_map_y);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 2, saved_map_z);
                }
                restore_status = 0;
                if (enable_state_saved) {
                    restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable_x,
                                                                 0, test_partid);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_y,
                                                                  0, test_partid - 1);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_z,
                                                                  0, test_partid - 2);
                }
                if (restore_status)
                    val_print(ERROR, "\n       Failed to restore PARTID enable state");
                if (csumon_state_saved)
                    val_mpam_restore_csumon_state(msc_index, &saved_csumon);
                if (partition_state_saved) {
                    val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                    val_mpam_restore_partition_state(msc_index, &saved_partition_y);
                    val_mpam_restore_partition_state(msc_index, &saved_partition_z);
                }

                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(03));
                return;
            }

            start_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);
            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       End Count = 0x%lx", end_count);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Read CSU MON */
            counter[1] = end_count;

            val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
            val_mem_issue_dsb();

            /***  Scenario 3: Buffer copy with 25% CASSOC settings  ***/
            val_print(DEBUG, "\n       Scenario 3: 25 percent CASSOC setting");

            /* Disable CCAP and CPOR settings -> Set them to max */
            if (val_mpam_supports_cpor(msc_index))
                val_mpam_configure_cpor(msc_index, test_partid - 2, 100);

            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_ccap(msc_index, test_partid - 2, 0, 100);

            /* Configure different PARTID with 25% cassoc settings */
            val_mpam_configure_cassoc(msc_index, test_partid - 2, CASSOC_PARTITION_25);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid - 2, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Program MPAM2_EL2 with test_partid-2 and default PMG */
            status = val_mpam_program_el2(test_partid - 2, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                val_mpam_csumon_disable(msc_index);
                val_mpam_reg_write(MPAM2_EL2, saved_el2);
                /* Free the buffers to the heap manager */
                val_pe_cache_invalidate_range((uint64_t)src_buf, BUFFER_SIZE);
                val_pe_cache_invalidate_range((uint64_t)dest_buf, BUFFER_SIZE);
                val_mem_issue_dsb();

                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 1, saved_map_y);
                    val_mpam_set_partid_mapping(msc_index, test_partid - 2, saved_map_z);
                }
                restore_status = 0;
                if (enable_state_saved) {
                    restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable_x,
                                                                 0, test_partid);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_y,
                                                                  0, test_partid - 1);
                    restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_z,
                                                                  0, test_partid - 2);
                }
                if (restore_status)
                    val_print(ERROR, "\n       Failed to restore PARTID enable state");
                if (csumon_state_saved)
                    val_mpam_restore_csumon_state(msc_index, &saved_csumon);
                if (partition_state_saved) {
                    val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                    val_mpam_restore_partition_state(msc_index, &saved_partition_y);
                    val_mpam_restore_partition_state(msc_index, &saved_partition_z);
                }

                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(04));
                return;
            }

            start_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Start mem copy */
            val_memcpy(src_buf, dest_buf, BUFFER_SIZE);
            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       End Count = 0x%lx", end_count);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Read CSU MON */
            counter[2] = end_count;

            /* Compare CSU mon counter across all 3 scenarios.
               As the cache associativity allocated to an MSC is restricted further,
               the number of cache lines in a set that the MSC can occupy is
               expected to decrease in each scenario, resulting in reduced CSU MON counter */

            if (!((counter[0] > counter[1]) && (counter[1] > counter[2]))) {
                val_print(ERROR, "\n       Unexpected cache usage behavior observed");
                val_print(ERROR, "\n       CASSOC 100 percent count = 0x%lx", counter[0]);
                val_print(ERROR, "\n       CASSOC 50 percent count  = 0x%lx", counter[1]);
                val_print(ERROR, "\n       CASSOC 25 percent count  = 0x%lx", counter[2]);
                test_fail++;
            }

cleanup:
            /* Restore MPAM2_EL2 settings and all state modified by this resource. */
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            if (mappings_saved) {
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_map_x);
                val_mpam_set_partid_mapping(msc_index, test_partid - 1, saved_map_y);
                val_mpam_set_partid_mapping(msc_index, test_partid - 2, saved_map_z);
            }
            restore_status = 0;
            if (enable_state_saved) {
                restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable_x,
                                                             0, test_partid);
                restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_y,
                                                              0, test_partid - 1);
                restore_status |= val_mpam_msc_endis_partid(msc_index, saved_enable_z,
                                                              0, test_partid - 2);
            }
            if (restore_status)
                test_fail++;
            if (csumon_state_saved)
                val_mpam_restore_csumon_state(msc_index, &saved_csumon);
            if (partition_state_saved) {
                val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                val_mpam_restore_partition_state(msc_index, &saved_partition_y);
                val_mpam_restore_partition_state(msc_index, &saved_partition_z);
            }

            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);

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
        val_set_status(index, RESULT_FAIL(5));
    else if (test_skip)
        val_set_status(index, RESULT_SKIP(3));
    else
        val_set_status(index, RESULT_PASS);

    return;
}

uint32_t partition004_entry(uint32_t num_pe)
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
