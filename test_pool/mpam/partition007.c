/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "acs_val.h"
#include "acs_pe.h"
#include "acs_memory.h"
#include "acs_mpam.h"
#include "acs_mpam_reg.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_MPAM_CACHE_TEST_NUM_BASE + 7)
#define TEST_RULE  "GFYDF"
#define TEST_DESC  "Check PARTID narrowing and monitoring  "

#define CACHE_PERCENTAGE 50U
#define REQ_PARTID_A 1U
#define REQ_PARTID_B 2U
#define INT_PARTID_A 1U
#define INT_PARTID_B 2U

static uint32_t cpbm_a[MAX_CPBM_WORDS];
static uint32_t cpbm_b[MAX_CPBM_WORDS];

static void
invalidate_buffers(void *src_buf, void *dest_buf, uint32_t buffer_size)
{
    val_pe_cache_invalidate_range((uint64_t)src_buf, buffer_size);
    val_pe_cache_invalidate_range((uint64_t)dest_buf, buffer_size);
    val_mem_issue_dsb();
}

static uint32_t
run_csu_workload(uint32_t msc_index, uint16_t monitor_partid, uint16_t traffic_partid,
                 void *src_buf, void *dest_buf, uint32_t buffer_size, uint32_t *count)
{
    uint64_t saved_mpam2 = val_mpam_reg_read(MPAM2_EL2);
    uint32_t status = 1;

    /* Reuse the buffers, but evict them so every measurement starts cold. */
    invalidate_buffers(src_buf, dest_buf, buffer_size);

    val_mpam_configure_csu_mon(msc_index, monitor_partid, DEFAULT_PMG, 0);
    val_mpam_csumon_enable(msc_index);

    /* Apply the test PARTID only to the measured memory traffic. */
    if (val_mpam_program_el2(traffic_partid, DEFAULT_PMG))
        goto cleanup;
    val_memcpy(dest_buf, src_buf, buffer_size);
    val_mem_issue_dsb();
    val_mpam_reg_write(MPAM2_EL2, saved_mpam2);

    val_time_delay_ms(TIMEOUT_MEDIUM);
    *count = val_mpam_read_csumon(msc_index);
    status = 0;

cleanup:
    val_mpam_csumon_disable(msc_index);
    val_mpam_reg_write(MPAM2_EL2, saved_mpam2);
    return status;
}

static void
payload(void)
{
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t msc_count = val_mpam_get_msc_count();
    uint32_t llc_index = val_cache_get_llc_index();
    uint64_t cache_id;
    uint64_t cache_size;
    uint64_t buffer_size64;
    uint64_t num_pages64;
    uint32_t buffer_size;
    uint32_t page_size;
    uint32_t num_pages;
    void *src_buf;
    void *dest_buf;
    uint32_t msc_index;
    uint32_t rsrc_index;
    uint32_t rsrc_count;
    uint32_t cpbm_width;
    uint32_t cpbm_words;
    uint32_t saved_part_sel;
    uint32_t saved_cmax_a;
    uint32_t saved_cmax_b;
    uint16_t saved_map_a;
    uint16_t saved_map_b;
    uint32_t saved_mon_sel;
    uint32_t status;
    bool endis_supported;
    bool saved_enable_a;
    bool saved_enable_b;
    MPAM_CSUMON_STATE saved_csumon;
    uint64_t saved_mpam2;
    uint32_t count_allowed;
    uint32_t count_blocked;
    uint32_t count_nonmatch;
    uint32_t count_match;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint64_t pe_max_partid;

    /* The PE must be able to generate both fixed request PARTID labels. */
    pe_max_partid = (val_mpam_reg_read(MPAMIDR_EL1) >> MPAMIDR_PARTID_MAX_SHIFT) &
                    MPAMIDR_PARTID_MAX_MASK;
    if (pe_max_partid < REQ_PARTID_B) {
        val_set_status(pe_index, RESULT_SKIP(03));
        return;
    }

    if (llc_index == CACHE_TABLE_EMPTY) {
        val_set_status(pe_index, RESULT_SKIP(01));
        return;
    }

    cache_id = val_cache_get_info(CACHE_ID, llc_index);
    cache_size = val_cache_get_info(CACHE_SIZE, llc_index);
    if ((cache_id == INVALID_CACHE_INFO) ||
        (cache_size == INVALID_CACHE_INFO) ||
        (cache_size < 2U)) {
        val_set_status(pe_index, RESULT_SKIP(02));
        return;
    }

    /* Calculate the percentage without overflowing cache_size. */
    buffer_size64 = ((cache_size / 100U) * CACHE_PERCENTAGE) +
                    (((cache_size % 100U) * CACHE_PERCENTAGE) / 100U);
    if ((buffer_size64 == 0U) || (buffer_size64 > ACS_UINT32_MAX)) {
        val_set_status(pe_index, RESULT_SKIP(02));
        return;
    }
    buffer_size = (uint32_t)buffer_size64;

    page_size = val_memory_page_size();
    if (page_size == 0U) {
        val_set_status(pe_index, RESULT_SKIP(02));
        return;
    }

    num_pages64 = (buffer_size64 + page_size - 1U) / page_size;
    /* Validate the allocator page count before calculating its byte capacity. */
    if ((num_pages64 == 0U) || (num_pages64 > ACS_UINT32_MAX) ||
        (num_pages64 > (~(uint64_t)0 / page_size)) ||
        ((num_pages64 * page_size) < buffer_size64)) {
        val_set_status(pe_index, RESULT_SKIP(02));
        return;
    }
    num_pages = (uint32_t)num_pages64;

    /* One allocation is sufficient; cache invalidation makes each workload cold. */
    src_buf = val_memory_alloc_pages(num_pages);
    dest_buf = val_memory_alloc_pages(num_pages);
    if ((src_buf == NULL) || (dest_buf == NULL)) {
        val_print(ERROR, "\n       Mem allocation failed");
        if (src_buf != NULL)
            val_memory_free_pages(src_buf, num_pages);
        if (dest_buf != NULL)
            val_memory_free_pages(dest_buf, num_pages);
        val_set_status(pe_index, RESULT_FAIL(01));
        return;
    }
    val_memory_set(src_buf, buffer_size, 0xA5U);

    /* Find an LLC resource that can exercise narrowing through CPOR and CSUMON. */
    for (msc_index = 0; msc_index < msc_count; msc_index++) {
        if (!val_mpam_msc_supports_partid_nrw(msc_index) ||
            (val_mpam_get_max_partid(msc_index) < REQ_PARTID_B) ||
            (val_mpam_get_max_intpartid(msc_index) < INT_PARTID_B))
            continue;

        rsrc_count = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
        for (rsrc_index = 0; rsrc_index < rsrc_count; rsrc_index++) {
            if ((val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) !=
                 MPAM_RSRC_TYPE_PE_CACHE) ||
                (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) != cache_id))
                continue;

            /* Save selectors before changing the resource instance. */
            saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
            saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);

            /* Select the same resource instance for partition controls and monitoring. */
            if (val_mpam_msc_supports_ris(msc_index))
                val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            /* Partitioning and monitoring capabilities are specific to the selected RIS. */
            if (!val_mpam_supports_cpor(msc_index) ||
                !val_mpam_supports_csumon(msc_index) ||
                (val_mpam_get_csumon_count(msc_index) == 0U)) {
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }

            cpbm_width = val_mpam_get_cpbm_width(msc_index);
            if ((cpbm_width == 0U) || (cpbm_width > (MAX_CPBM_WORDS * 32U))) {
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                continue;
            }
            cpbm_words = (cpbm_width + 31U) / 32U;

            test_skip = 0;
            /* Preserve all architectural state changed by the test. */
            saved_mpam2 = val_mpam_reg_read(MPAM2_EL2);
            saved_map_a = val_mpam_get_partid_mapping(msc_index, REQ_PARTID_A);
            saved_map_b = val_mpam_get_partid_mapping(msc_index, REQ_PARTID_B);
            endis_supported = val_mpam_msc_supports_partid_endis(msc_index);
            saved_enable_a = endis_supported ?
                             val_mpam_msc_partid_is_enabled(msc_index, REQ_PARTID_A) : true;
            saved_enable_b = endis_supported ?
                             val_mpam_msc_partid_is_enabled(msc_index, REQ_PARTID_B) : true;
            val_mpam_read_cpbm(msc_index, INT_PARTID_A, cpbm_a, cpbm_words);
            val_mpam_read_cpbm(msc_index, INT_PARTID_B, cpbm_b, cpbm_words);
            if (val_mpam_supports_ccap(msc_index)) {
                val_mpam_select_partid(msc_index, INT_PARTID_A, true);
                saved_cmax_a = val_mpam_mmr_read(msc_index, REG_MPAMCFG_CMAX);
                val_mpam_select_partid(msc_index, INT_PARTID_B, true);
                saved_cmax_b = val_mpam_mmr_read(msc_index, REG_MPAMCFG_CMAX);
            }
            val_mpam_save_csumon_state(msc_index, 0, &saved_csumon);

            /*
             * Give the two internal PARTIDs opposite CPBM settings. Keep CMAX
             * unrestricted, when present, so CPBM is the differentiating control.
             */
            val_mpam_configure_intpartid_cpor(msc_index, INT_PARTID_A, 100);
            val_mpam_configure_intpartid_cpor(msc_index, INT_PARTID_B, 0);
            if (val_mpam_supports_ccap(msc_index)) {
                val_mpam_configure_intpartid_ccap(msc_index, INT_PARTID_A, 0, 100);
                val_mpam_configure_intpartid_ccap(msc_index, INT_PARTID_B, 0, 100);
            }
            /* GFYDF: first map request A to the internal PARTID with full CPBM. */
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_A, INT_PARTID_A);
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_B, INT_PARTID_B);
            status = 0;
            if (endis_supported) {
                status = val_mpam_msc_endis_partid(msc_index, 1, 0, REQ_PARTID_A);
                status |= val_mpam_msc_endis_partid(msc_index, 1, 0, REQ_PARTID_B);
            }
            if (status) {
                test_fail++;
                goto restore;
            }

            if (val_mpam_get_partid_mapping(msc_index, REQ_PARTID_A) != INT_PARTID_A) {
                val_print(ERROR, "\n       Request PARTID mapping readback failed");
                test_fail++;
                goto restore;
            }

            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_A,
                                 src_buf, dest_buf, buffer_size, &count_allowed)) {
                test_fail++;
                goto restore;
            }
            if (count_allowed == 0U) {
                val_print(ERROR, "\n       No occupancy with enabled internal CPBM");
                test_fail++;
                goto restore;
            }

            /* Remap only request A; lower occupancy proves the new internal control applies. */
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_A, INT_PARTID_B);
            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_A,
                                 src_buf, dest_buf, buffer_size, &count_blocked)) {
                test_fail++;
                goto restore;
            }
            if (count_blocked >= count_allowed) {
                val_print(ERROR, "\n       Remapping did not change cache occupancy");
                test_fail++;
                goto restore;
            }

            /*
             * ZGGMX: requests A and B share one internal PARTID, but CSUMON must
             * continue to distinguish their original request PARTID labels.
             */
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_A, INT_PARTID_A);
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_B, INT_PARTID_A);
            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_B,
                                 src_buf, dest_buf, buffer_size, &count_nonmatch)) {
                test_fail++;
                goto restore;
            }

            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_A,
                                 src_buf, dest_buf, buffer_size, &count_match)) {
                test_fail++;
                goto restore;
            }
            if (count_match <= count_nonmatch) {
                val_print(ERROR, "\n       Monitor did not distinguish request PARTIDs");
                test_fail++;
            }

restore:
            /* Restore mappings, partition controls, monitor state, and MPAM2_EL2. */
            val_mpam_csumon_disable(msc_index);
            val_mpam_write_cpbm(msc_index, INT_PARTID_A, cpbm_a, cpbm_words);
            val_mpam_write_cpbm(msc_index, INT_PARTID_B, cpbm_b, cpbm_words);
            if (val_mpam_supports_ccap(msc_index)) {
                val_mpam_select_partid(msc_index, INT_PARTID_A, true);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_CMAX, saved_cmax_a);
                val_mpam_select_partid(msc_index, INT_PARTID_B, true);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_CMAX, saved_cmax_b);
            }
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_A, saved_map_a);
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_B, saved_map_b);
            status = 0;
            if (endis_supported) {
                status = val_mpam_msc_endis_partid(msc_index, saved_enable_a,
                                                   0, REQ_PARTID_A);
                status |= val_mpam_msc_endis_partid(msc_index, saved_enable_b,
                                                    0, REQ_PARTID_B);
            }
            if (status)
                test_fail++;
            val_mpam_restore_csumon_state(msc_index, &saved_csumon);
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
            val_mpam_reg_write(MPAM2_EL2, saved_mpam2);
        }
    }

    invalidate_buffers(src_buf, dest_buf, buffer_size);
    val_memory_free_pages(src_buf, num_pages);
    val_memory_free_pages(dest_buf, num_pages);

    if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(03));
    else if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(01));
    else
        val_set_status(pe_index, RESULT_PASS);
}

uint32_t
partition007_entry(uint32_t num_pe)
{
    uint32_t status;

    num_pe = 1;
    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
    return status;
}
