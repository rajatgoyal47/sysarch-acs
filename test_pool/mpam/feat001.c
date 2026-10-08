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

#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 25
#define TEST_RULE  "VFJMZ"
#define TEST_DESC  "Check PARTID Disable functionality    "

/* Test algorithm -
1. Check if the MSC supports PARTID enable/disable control by confirming MPAMF_IDR.HAS_ENDIS == 1.
2. Select a valid PARTID (e.g., PARTID_X) and identify a cache resource that supports
   portion-based partitioning.
3. Preserve the effective partition controls for PARTID_X and PARTID_Y.
4. Give both PARTIDs unrestricted cache controls and use separate cold working sets.
5. Run PARTID_X traffic that occupies approximately one LLC capacity.
6. Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value A.
7. Disable PARTID_X by writing its ID to MPAMCFG_DIS.PARTID. Do not set the NFU bit.
8. Generate PARTID_Y traffic on a separate LLC-sized working set to evict PARTID_X lines.
9. Use the CSU monitor to measure the remaining PARTID_X usage as Value B.
10. Value B should be less than Value A and gradually decreasing.
11. Now, re-enable PARTID_X by writing it to MPAMCFG_EN.PARTID and Disable PARTID_Y
12. Re-run the same memory workload once more from the same PE.
13. Measure cache usage again using CSU. Record this as Value C.
Validate the results:
Value A ~= Value C: confirms that the original partition settings were preserved after re-enabling.
Value B < Value A: confirms that the disabled PARTID was deprioritized or blocked from allocation.
If both conditions hold, the test is passed.
*/

#define CACHE_PERCENTAGE 50U
#define NUM_SCENARIOS 3

static void
payload(void)
{

    uint32_t msc_cnt  = val_mpam_get_msc_count();
    uint32_t llc_idx  = val_cache_get_llc_index();
    uint32_t index    = val_pe_get_index_mpid(val_pe_get_mpid());

    void     *src_buf;
    void     *dest_buf;
    void     *src_buf_y;
    void     *dest_buf_y;
    uint32_t page_size;
    uint32_t num_pages;
    uint64_t num_pages64;
    uint64_t cache_size;
    uint64_t buffer_size64;
    uint32_t buffer_size;
    uint32_t end_count    = 0;
    uint32_t test_fail    = 0;
    uint16_t partid_x     = 1;
    uint16_t partid_y     = 2;
    uint32_t msc_index, rsrc_index, rsrc_cnt;
    uint32_t nrdy_timeout;
    uint64_t cache_identifier;
    uint64_t counter[NUM_SCENARIOS];
    uint64_t saved_el2;
    uint32_t cpart_flag  = 0;
    bool     test_skip   = 1;
    uint32_t status      = 0;
    uint32_t num_mon     = 0;
    uint32_t saved_part_sel;
    uint32_t saved_mon_sel;
    uint16_t saved_map_x;
    uint16_t saved_map_y;
    bool mappings_saved;
    bool saved_enable_x;
    bool saved_enable_y;
    MPAM_CSUMON_STATE saved_csumon;
    bool csumon_state_saved;
    MPAM_PARTITION_STATE saved_partition_x;
    MPAM_PARTITION_STATE saved_partition_y;
    bool partition_state_saved;

    /* Check if LLC is valid */
    if (llc_idx == CACHE_TABLE_EMPTY) {
        val_print(DEBUG, "\n       LLC not present, Skipping test");
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

    cache_size = val_cache_get_info(CACHE_SIZE, llc_idx);
    buffer_size64 = ((cache_size / 100U) * CACHE_PERCENTAGE) +
                    (((cache_size % 100U) * CACHE_PERCENTAGE) / 100U);
    if ((cache_size == INVALID_CACHE_INFO) || (buffer_size64 == 0U) ||
        (buffer_size64 > ACS_UINT32_MAX)) {
        val_print(DEBUG, "\n       Invalid LLC size, skipping test");
        val_set_status(index, RESULT_SKIP(2));
        return;
    }
    buffer_size = (uint32_t)buffer_size64;

    /* Iterate through all available LLC MSCs and their resources */
    for (msc_index = 0; msc_index < msc_cnt; msc_index++)
    {

        /* Check if the MSC supports PARTID Disable feature (MPAMCFG.HAS_ENDIS bit) */
        if (!val_mpam_msc_supports_partid_endis(msc_index)) {
            val_print(DEBUG,
                      "\n       MSC index %d does not support PARTID Enable/Disable, skipping...",
                      msc_index);
            continue;
        }

        /* Iterate through resources of MSC to find a resource which implements CPOR or CCAP */
        rsrc_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        for (rsrc_index = 0; rsrc_index < rsrc_cnt; rsrc_index++)
        {

            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index)
                != MPAM_RSRC_TYPE_PE_CACHE)
                continue;

            if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index)
                != cache_identifier)
                continue;

            /* Preserve selectors before the test selects a resource or PARTID. */
            saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
            saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);

            /* Select resource instance if RIS feature implemented */
            if (val_mpam_msc_supports_ris(msc_index))
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            if (val_mpam_get_max_partid(msc_index) < partid_y) {
                val_print(DEBUG,
                    "\n       MSC %d does not support required PARTIDs, skipping MSC", msc_index);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }

            num_mon     = 0;
            cpart_flag  = 0;
            end_count   = 0;
            csumon_state_saved = false;
            partition_state_saved = false;

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

            /* Preserve request-to-internal mappings before preparing test PARTIDs. */
            mappings_saved = false;
            if (val_mpam_msc_supports_partid_nrw(msc_index)) {
                saved_map_x = val_mpam_get_partid_mapping(msc_index, partid_x);
                saved_map_y = val_mpam_get_partid_mapping(msc_index, partid_y);
                mappings_saved = true;
            }
            /* Preserve request enable states before preparing the test PARTIDs. */
            saved_enable_x = val_mpam_msc_partid_is_enabled(msc_index, partid_x);
            saved_enable_y = val_mpam_msc_partid_is_enabled(msc_index, partid_y);

            /* Give each request PARTID a distinct control PARTID when narrowing is supported. */
            if ((val_mpam_prepare_partid(msc_index, partid_x, 0) != ACS_STATUS_PASS) ||
                (val_mpam_prepare_partid(msc_index, partid_y, 1) != ACS_STATUS_PASS)) {
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, partid_x, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, partid_y, saved_map_y);
                }
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_x, 0, partid_x);
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_y, 0, partid_y);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }

            /* Check if MSC supports CPOR or CCAP.
               Select Cache partitioning scheme based on the support */
            if (val_mpam_supports_cpor(msc_index)) {
                cpart_flag = 1;
                val_print(DEBUG, "\n       MSC %d supports CPOR partitioning", msc_index);

            } else
            if (val_mpam_supports_ccap(msc_index)) {
                cpart_flag = 2;
                val_print(DEBUG, "\n       MSC %d supports CCAP partitioning", msc_index);

            } else {
                val_print(DEBUG,
                    "\n       MSC %d does not support CPOR/CCAP partitioning, skipping...",
                    msc_index);
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, partid_x, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, partid_y, saved_map_y);
                }
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_x, 0, partid_x);
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_y, 0, partid_y);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }

            /* Allocate page-aligned memory for source and destination buffers. */
            page_size = val_memory_page_size();
            if (page_size == 0U) {
                val_print(ERROR, "\n       Invalid memory page size");
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, partid_x, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, partid_y, saved_map_y);
                }
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_x, 0, partid_x);
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_y, 0, partid_y);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(01));
                return;
            }

            num_pages64 = (buffer_size / page_size) +
                          ((buffer_size % page_size) != 0U);
            if ((num_pages64 == 0U) || (num_pages64 > ACS_UINT32_MAX) ||
                (num_pages64 > (~(uint64_t)0 / page_size)) ||
                ((num_pages64 * page_size) < buffer_size)) {
                val_print(ERROR, "\n       Invalid page count for workload buffers");
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, partid_x, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, partid_y, saved_map_y);
                }
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_x, 0, partid_x);
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_y, 0, partid_y);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                val_set_status(index, RESULT_FAIL(01));
                return;
            }
            num_pages = (uint32_t)num_pages64;
            src_buf = (void *)val_memory_alloc_pages(num_pages);
            dest_buf = (void *)val_memory_alloc_pages(num_pages);
            src_buf_y = (void *)val_memory_alloc_pages(num_pages);
            dest_buf_y = (void *)val_memory_alloc_pages(num_pages);

            if ((src_buf == NULL) || (dest_buf == NULL) ||
                (src_buf_y == NULL) || (dest_buf_y == NULL)) {
                val_print(ERROR, "\n       Mem allocation failed");
                val_set_status(index, RESULT_FAIL(01));
                if (dest_buf_y != NULL)
                    val_memory_free_pages(dest_buf_y, num_pages);
                if (src_buf_y != NULL)
                    val_memory_free_pages(src_buf_y, num_pages);
                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                if (mappings_saved) {
                    val_mpam_set_partid_mapping(msc_index, partid_x, saved_map_x);
                    val_mpam_set_partid_mapping(msc_index, partid_y, saved_map_y);
                }
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_x, 0, partid_x);
                (void)val_mpam_msc_endis_partid(msc_index, saved_enable_y, 0, partid_y);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                return;
            }

            /* Preserve the PE labels and partition controls before changing them. */
            saved_el2 = val_mpam_reg_read(MPAM2_EL2);
            if (!val_mpam_save_partition_state(msc_index, partid_x,
                                                &saved_partition_x)) {
                test_fail++;
                goto cleanup;
            }
            if (!val_mpam_save_partition_state(msc_index, partid_y,
                                                &saved_partition_y)) {
                val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                test_fail++;
                goto cleanup;
            }
            partition_state_saved = true;

            /* Keep the unused partitioning mechanism unrestricted. */
            if (cpart_flag == 1) {
                val_mpam_configure_ccap(msc_index, partid_x, 0, 100);
                val_mpam_configure_ccap(msc_index, partid_y, 0, 100);
            } else {
                val_mpam_configure_cpor(msc_index, partid_x, 100);
                val_mpam_configure_cpor(msc_index, partid_y, 100);
            }

            /* Initialize, then evict both working sets so measured traffic starts cold. */
            val_pe_cache_invalidate_range((uint64_t)src_buf, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)src_buf_y, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)dest_buf_y, buffer_size);
            val_mem_issue_dsb();

            /* Test runs on at least one MSC. */
            test_skip = 0;

            /* Step 3: Program PARTID_X with 100% Cache partition control */
            if (cpart_flag == 1)
                val_mpam_configure_cpor(msc_index, partid_x, 100);
            else
                val_mpam_configure_ccap(msc_index, partid_x, 0, 100);

            /* Preserve CSU monitor 0 before changing its programmable state. */
            val_mpam_save_csumon_state(msc_index, 0, &saved_csumon);
            csumon_state_saved = true;

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, partid_x, DEFAULT_PMG, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            /* Step 4: Set MPAM2_EL2 to assign PARTID_X to the active PE or thread */
            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                test_fail++;
                goto cleanup;
            }

            /* Step 5: Start mem copy */
            val_memcpy(src_buf, dest_buf, buffer_size);
            /* Complete the workload before restoring the PE labels used by ACS. */
            val_mem_issue_dsb();
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            /* Read CSU monitor cnt - number of cache lines filled by PARTID_X at this point */
            end_count = val_mpam_read_csumon(msc_index);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Step 6 */
            counter[0] = end_count;
            val_print(DEBUG, "\n       PARTID_X Counter (Scenario 1)= 0x%lx", counter[0]);

            /* Counter should be non-zero */
            if (counter[0] == 0) {
                val_print(ERROR,
                "\n       Scenario 1: PARTID_X counter is zero, test failed for MSC %d", msc_index);
                test_fail++;
                goto cleanup;
            }

            /* Keep PARTID_X resident so PARTID_Y creates real cache pressure. */

            /* Step 7: Disable PARTID_X. MPAMCFG_DIS.PARTID = PARTID_X.
               Do not set NFU. We need the PARTID later */
            status = val_mpam_msc_endis_partid(msc_index, 0, 0, partid_x);
            if (status) {
                test_fail++;
                goto cleanup;
            }

            /* Step 8: Program PARTID_Y with 100% Cache partitioning control */
            if (cpart_flag == 1)
                val_mpam_configure_cpor(msc_index, partid_y, 100);
            else
                val_mpam_configure_ccap(msc_index, partid_y, 0, 100);

            /* Reset CSU monitor counting PARTID_X, only a fail safe.
               It should not impact the next read anyway */
            val_mpam_reset_csumon(msc_index, 0);

            status = val_mpam_program_el2(partid_y, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                test_fail++;
                goto cleanup;
            }

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            val_memcpy(src_buf_y, dest_buf_y, buffer_size);
            /* Complete the workload before restoring the PE labels used by ACS. */
            val_mem_issue_dsb();
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            /* Step 9 */
            end_count = val_mpam_read_csumon(msc_index);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            counter[1] = end_count;
            val_print(DEBUG, "\n       PARTID_X Counter (Scenario 2)= 0x%lx", counter[1]);

            /* Step 10: Validate Value B < Value A */
            /* Txn tagged with PARTID_Y but monitor counts PARTID X. PARTID_X is disabled, so
            cache lines occupied previously by PARTID_X should have highest eviction priority.
            Buffer tagged with PARTID_Y should evict PARTID_X cache lines */
            if (counter[1] >= counter[0]) {
                val_print(ERROR,
                "\n       Scenario 2: Counter more than S1, test failed for MSC %d", msc_index);
                test_fail++;
                goto cleanup;
            }

            val_pe_cache_invalidate_range((uint64_t)src_buf, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, buffer_size);
            val_mem_issue_dsb();

            /* Step 11: Enable PARTID_X and Disable PARTID_Y. H/w must remember PARTID_X config */
            status = val_mpam_msc_endis_partid(msc_index, 1, 0, partid_x);
            status |= val_mpam_msc_endis_partid(msc_index, 0, 0, partid_y);

            if (status) {
                test_fail++;
                goto cleanup;
            }

            /* Step 12: Program MPAM2_EL2 with PARTID_X and perform bufcopy */
            status = val_mpam_program_el2(partid_x, DEFAULT_PMG);
            if (status) {
                val_print(ERROR, "\n       MPAM2_EL2 programming failed");
                test_fail++;
                goto cleanup;
            }

            /* Reset CSU monitor counting PARTID_X */
            val_mpam_reset_csumon(msc_index, 0);

            /* Enable CSU Monitor */
            val_mpam_csumon_enable(msc_index);

            /* Wait for MAX_NRDY_USEC after monitor config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            val_memcpy(src_buf, dest_buf, buffer_size);
            /* Complete the workload before restoring the PE labels used by ACS. */
            val_mem_issue_dsb();
            val_mpam_reg_write(MPAM2_EL2, saved_el2);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            end_count = val_mpam_read_csumon(msc_index);

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            counter[2] = end_count;
            val_print(DEBUG, "\n       PARTID_X Counter (Scenario 3)= 0x%lx", counter[2]);

            /* With PARTID_Y disabled and PARTID_X enabled, CPOR settings of PARTID_X must be
               restored by the H/W. PARTID_Y cache lines should be evicted by txn tagged with
               PARTID_X */
            if (counter[1] >= counter[2]) {
                val_print(ERROR,
                    "\n       Scenario 3: Counter less than S2, test failed for MSC %d", msc_index);
                test_fail++;
                goto cleanup;
            }

cleanup:
            /* Restore the PE labels before cleanup accesses memory or MPAM controls. */
            val_mpam_reg_write(MPAM2_EL2, saved_el2);
            /* Stop and restore the monitor only after its original state was saved. */
            if (csumon_state_saved) {
                val_mpam_csumon_disable(msc_index);
                val_mpam_restore_csumon_state(msc_index, &saved_csumon);
            }

            if (partition_state_saved) {
                val_mpam_restore_partition_state(msc_index, &saved_partition_x);
                val_mpam_restore_partition_state(msc_index, &saved_partition_y);
            }

            /* Restore original narrowing mappings before restoring selectors. */
            if (mappings_saved) {
                val_mpam_set_partid_mapping(msc_index, partid_x, saved_map_x);
                val_mpam_set_partid_mapping(msc_index, partid_y, saved_map_y);
            }
            /* Restore each request PARTID to its original enable state. */
            status = val_mpam_msc_endis_partid(msc_index, saved_enable_x, 0, partid_x);
            status |= val_mpam_msc_endis_partid(msc_index, saved_enable_y, 0, partid_y);
            if (status) {
                val_print(ERROR, "\n       Failed to restore test PARTID enable state");
                test_fail++;
            }

            /* Restore the selectors changed by test configuration. */
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);

            /* Evict and release both independent working sets. */
            val_pe_cache_invalidate_range((uint64_t)src_buf, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)src_buf_y, buffer_size);
            val_pe_cache_invalidate_range((uint64_t)dest_buf_y, buffer_size);
            val_mem_issue_dsb();

            val_memory_free_pages(src_buf, num_pages);
            val_memory_free_pages(dest_buf, num_pages);
            val_memory_free_pages(src_buf_y, num_pages);
            val_memory_free_pages(dest_buf_y, num_pages);
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

uint32_t feat001_entry(uint32_t num_pe)
{
    uint32_t status  = ACS_STATUS_FAIL;

    /* Legacy MPAM execution runs this test on one PE. */
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
