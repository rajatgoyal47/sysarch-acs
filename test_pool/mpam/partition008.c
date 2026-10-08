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

#define TEST_NUM   (ACS_MPAM_CACHE_TEST_NUM_BASE + 8)
#define TEST_RULE  "CSLHH"
#define TEST_DESC  "Check narrowed request PARTID disable "

#define CACHE_PERCENTAGE 50U
#define REQ_PARTID_A 1U
#define REQ_PARTID_B 2U
#define INT_PARTID_A 1U

static uint32_t cpbm_saved[MAX_CPBM_WORDS];
static uint32_t cpbm_check[MAX_CPBM_WORDS];

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
    uint32_t saved_cmax;
    uint16_t saved_map_a;
    uint16_t saved_map_b;
    uint32_t saved_mon_sel;
    bool saved_enable_a;
    bool saved_enable_b;
    MPAM_CSUMON_STATE saved_csumon;
    uint64_t saved_mpam2;
    uint32_t count_enabled;
    uint32_t count_disabled;
    uint32_t count_peer;
    uint32_t count_reenabled;
    uint32_t status;
    uint32_t word;
    uint32_t last_bits;
    uint32_t expected;
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

    /* CSLHH requires both narrowing and per-request enable control. */
    for (msc_index = 0; msc_index < msc_count; msc_index++) {
        if (val_mpam_msc_supports_partid_nrw(msc_index) &&
            val_mpam_msc_supports_partid_endis(msc_index))
            break;
    }
    if (msc_index == msc_count) {
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

    /* Find an LLC resource supporting narrowing and request-PARTID enable control. */
    for (msc_index = 0; msc_index < msc_count; msc_index++) {
        if (!val_mpam_msc_supports_partid_nrw(msc_index) ||
            !val_mpam_msc_supports_partid_endis(msc_index) ||
            (val_mpam_get_max_partid(msc_index) < REQ_PARTID_B) ||
            (val_mpam_get_max_intpartid(msc_index) < INT_PARTID_A))
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
            saved_enable_a = val_mpam_msc_partid_is_enabled(msc_index, REQ_PARTID_A);
            saved_enable_b = val_mpam_msc_partid_is_enabled(msc_index, REQ_PARTID_B);
            val_mpam_read_cpbm(msc_index, INT_PARTID_A, cpbm_saved, cpbm_words);
            if (val_mpam_supports_ccap(msc_index)) {
                val_mpam_select_partid(msc_index, INT_PARTID_A, true);
                saved_cmax = val_mpam_mmr_read(msc_index, REG_MPAMCFG_CMAX);
            }
            val_mpam_save_csumon_state(msc_index, 0, &saved_csumon);

            /*
             * Both requests share an unrestricted internal control. Disabling
             * one request must not modify that shared control or affect its peer.
             */
            val_mpam_configure_intpartid_cpor(msc_index, INT_PARTID_A, 100);
            if (val_mpam_supports_ccap(msc_index))
                val_mpam_configure_intpartid_ccap(msc_index, INT_PARTID_A, 0, 100);
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_A, INT_PARTID_A);
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_B, INT_PARTID_A);
            status = val_mpam_msc_endis_partid(msc_index, 1, 0, REQ_PARTID_A);
            status |= val_mpam_msc_endis_partid(msc_index, 1, 0, REQ_PARTID_B);
            if (status) {
                test_fail++;
                goto restore;
            }

            /* Establish normal enabled occupancy for request A as the baseline. */
            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_A,
                                 src_buf, dest_buf, buffer_size, &count_enabled)) {
                test_fail++;
                goto restore;
            }
            if (count_enabled == 0U) {
                val_print(ERROR, "\n       Enabled request PARTID has no cache occupancy");
                test_fail++;
                goto restore;
            }

            /* CSLHH: disable request A while request B remains enabled on the same intPARTID. */
            status = val_mpam_msc_endis_partid(msc_index, 0, 0, REQ_PARTID_A);
            if (status) {
                test_fail++;
                goto restore;
            }
            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_A,
                                 src_buf, dest_buf, buffer_size, &count_disabled)) {
                test_fail++;
                goto restore;
            }
            if (count_disabled >= count_enabled) {
                val_print(ERROR, "\n       Disabled request PARTID was not deprioritized");
                test_fail++;
                goto restore;
            }

            /* The enabled peer must retain use of the shared internal control. */
            if (run_csu_workload(msc_index, REQ_PARTID_B, REQ_PARTID_B,
                                 src_buf, dest_buf, buffer_size, &count_peer)) {
                test_fail++;
                goto restore;
            }
            if (count_peer == 0U) {
                val_print(ERROR, "\n       Enabled peer request has no cache occupancy");
                test_fail++;
                goto restore;
            }

            /* Disabling a request PARTID must not clear the shared internal CPBM. */
            val_mpam_read_cpbm(msc_index, INT_PARTID_A, cpbm_check, cpbm_words);
            last_bits = cpbm_width & 31U;
            for (word = 0; word < cpbm_words; word++) {
                expected = ACS_UINT32_MAX;
                if ((word == (cpbm_words - 1U)) && (last_bits != 0U))
                    expected = (1U << last_bits) - 1U;
                if (cpbm_check[word] != expected) {
                    val_print(ERROR, "\n       Disabling request changed internal CPBM");
                    test_fail++;
                    goto restore;
                }
            }

            /* Re-enable request A and verify that its normal cache behavior recovers. */
            status = val_mpam_msc_endis_partid(msc_index, 1, 0, REQ_PARTID_A);
            if (status) {
                test_fail++;
                goto restore;
            }
            if (run_csu_workload(msc_index, REQ_PARTID_A, REQ_PARTID_A,
                                 src_buf, dest_buf, buffer_size, &count_reenabled)) {
                test_fail++;
                goto restore;
            }
            if (count_reenabled <= count_disabled) {
                val_print(ERROR, "\n       Re-enabled request did not recover cache occupancy");
                test_fail++;
            }

restore:
            /* Restore mappings, request enable states, controls, and monitor state. */
            val_mpam_csumon_disable(msc_index);
            val_mpam_write_cpbm(msc_index, INT_PARTID_A, cpbm_saved, cpbm_words);
            if (val_mpam_supports_ccap(msc_index)) {
                val_mpam_select_partid(msc_index, INT_PARTID_A, true);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_CMAX, saved_cmax);
            }
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_A, saved_map_a);
            val_mpam_set_partid_mapping(msc_index, REQ_PARTID_B, saved_map_b);
            status = val_mpam_msc_endis_partid(msc_index, saved_enable_a,
                                               0, REQ_PARTID_A);
            status |= val_mpam_msc_endis_partid(msc_index, saved_enable_b,
                                                0, REQ_PARTID_B);
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
partition008_entry(uint32_t num_pe)
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
