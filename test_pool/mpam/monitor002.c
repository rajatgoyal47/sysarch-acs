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


#define TEST_NUM   ACS_MPAM_MONITOR_TEST_NUM_BASE + 2
#define TEST_RULE  ""
#define TEST_DESC  "Check PMG Storage by CCAP Nodes       "

#define PARTITION_PERCENTAGE 75
#define CACHE_PERCENTAGE 50

static uint64_t mpam2_el2_temp;

static void payload(void)
{
    uint32_t msc_node_cnt = val_mpam_get_msc_count();
    uint32_t rsrc_node_cnt;
    uint32_t msc_index;
    uint32_t rsrc_index;
    uint32_t llc_index;
    uint64_t cache_identifier;
    uint32_t cache_maxsize = 0;
    uint32_t max_pmg = DEFAULT_PMG_MAX;
    uint32_t test_partid = 8; //Selecting distinct partid
    uint32_t ccap_nodes = 0;
    uint32_t csumon_count = 0;
    uint8_t pmg1;
    uint8_t pmg2;
    void *src_buf = 0;
    void *dest_buf = 0;
    uint64_t buf_size;
    uint32_t page_size;
    uint32_t num_pages;
    uint64_t num_pages64;
    uint64_t mpam2_el2 = 0;
    uint64_t nrdy_timeout;
    uint32_t storage_value1;
    uint32_t storage_value2;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t test_run = 0;
    uint32_t test_fail = 0;
    uint32_t saved_part_sel = 0;
    uint32_t saved_mon_sel = 0;
    uint16_t saved_mapping = 0;
    uint32_t mapping_saved;
    uint32_t restore_status;
    bool saved_enable;
    bool enable_state_saved;
    MPAM_CSUMON_STATE saved_csumon;
    MPAM_PARTITION_STATE saved_partition;

   /* Get the Index for LLC */
    llc_index = val_cache_get_llc_index();
    if (llc_index == CACHE_TABLE_EMPTY) {
        val_print(ERROR, "\n       Cache info table empty");
        val_set_status(index, RESULT_SKIP(01));
        return;
    }

    /* Get the cache identifier for LLC */
    cache_identifier = val_cache_get_info(CACHE_ID, llc_index);
    if (cache_identifier == INVALID_CACHE_INFO) {
        val_print(ERROR, "\n       LLC invalid in PPTT");
        val_set_status(index, RESULT_SKIP(02));
        return;
    }

    /* Get MPAM related information for LLC */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {

      saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
      saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);

      rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
      for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

        if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) != MPAM_RSRC_TYPE_PE_CACHE)
          continue;

        if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) != cache_identifier)
          continue;

        /* Select resource instance if RIS feature implemented */
        if (val_mpam_msc_supports_ris(msc_index))
          val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

        if (val_mpam_supports_ccap(msc_index)) {
          cache_maxsize = GET_MAX_VALUE(cache_maxsize,
                              val_cache_get_info(CACHE_SIZE, llc_index));

          max_pmg = GET_MIN_VALUE(max_pmg,
                                  val_mpam_get_max_pmg(msc_index));

          if (val_mpam_supports_csumon(msc_index))
              csumon_count = val_mpam_get_csumon_count(msc_index);

          ccap_nodes++;
        }
        test_partid = GET_MIN_VALUE(test_partid, val_mpam_get_max_partid(msc_index));
      }
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
      val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
    }

    val_print(DEBUG, "\n       CCAP Nodes = %d", ccap_nodes);
    val_print(DEBUG, "\n       Max PMG = %d", max_pmg);
    val_print(DEBUG, "\n       Test PARTID = %d", test_partid);
    val_print(DEBUG, "\n       Cache Max Size = 0x%x", cache_maxsize);
    val_print(DEBUG, "\n       Number of CSU Monitors = %d", csumon_count);

    /* Skip the test if CSU monitors/ nodes supporting Cache Portion Partitoning are 0
    */
    if (csumon_count == 0 || ccap_nodes == 0) {
        val_set_status(index, RESULT_SKIP(03));
        return;
    }

    /* Create two PMG groups for PE traffic */
    pmg1 = DEFAULT_PMG;

    mpam2_el2 = val_mpam_reg_read(MPAM2_EL2);
    mpam2_el2_temp = mpam2_el2;

    /* Clear the PARTID_D & PMG_D bits in mpam2_el2 before writing to them */
    mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2, MPAMn_ELx_PARTID_D_SHIFT+15, MPAMn_ELx_PARTID_D_SHIFT);
    mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2, MPAMn_ELx_PMG_D_SHIFT+7, MPAMn_ELx_PMG_D_SHIFT);

    /* Write test_partid & pmg1 to mpam2_el2 to generate PE traffic */
    mpam2_el2 |= (((uint64_t)pmg1 << MPAMn_ELx_PMG_D_SHIFT) |
                  ((uint64_t)test_partid << MPAMn_ELx_PARTID_D_SHIFT));

     /* visit each MSC node and check for Cache resources */
    for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
      rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);

      for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

        /* Check whether resource node is a PE Cache */
        if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) !=
                                                    MPAM_RSRC_TYPE_PE_CACHE)
          continue;

        /*Check if the PE Cache ID matches LLC ID */
        if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) != cache_identifier)
          continue;

        buf_size = ((uint64_t)cache_maxsize * CACHE_PERCENTAGE) / 100U;
        page_size = val_memory_page_size();
        if (page_size == 0U) {
            val_print(ERROR, "\n       Invalid memory page size");
            val_set_status(index, RESULT_FAIL(01));
            return;
        }

        num_pages64 = (buf_size / page_size) + ((buf_size % page_size) != 0U);
        /* Validate the allocator page count before calculating its byte capacity. */
        if ((num_pages64 == 0U) || (num_pages64 > ACS_UINT32_MAX) ||
            (num_pages64 > (~(uint64_t)0 / page_size)) ||
            ((num_pages64 * page_size) < buf_size)) {
            val_print(ERROR, "\n       Invalid page count for workload buffers");
            val_set_status(index, RESULT_FAIL(01));
            return;
        }
        num_pages = (uint32_t)num_pages64;

        /*Allocate memory for source and destination buffers */
        src_buf = (void *)val_memory_alloc_pages(num_pages);
        dest_buf = (void *)val_memory_alloc_pages(num_pages);

        val_print(DEBUG, "\n       buf_size            = 0x%x", buf_size);

        if ((src_buf == NULL) || (dest_buf == NULL)) {
            val_print(ERROR, "\n       Mem allocation failed");
            val_set_status(index, RESULT_FAIL(01));
            if (src_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)src_buf, buf_size);
                val_mem_issue_dsb();
                val_memory_free_pages(src_buf, num_pages);
            }
            if (dest_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)dest_buf, buf_size);
                val_mem_issue_dsb();
                val_memory_free_pages(dest_buf, num_pages);
            }
            return;
        }

        /* Preserve both selectors before selecting this resource instance. */
        saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
        saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
        /* Select and validate this resource only after fallible allocation succeeds. */
        if (val_mpam_msc_supports_ris(msc_index))
          val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

        if (!val_mpam_supports_ccap(msc_index) ||
            !val_mpam_supports_csumon(msc_index) ||
            (val_mpam_get_csumon_count(msc_index) == 0U) ||
            (val_mpam_get_max_pmg(msc_index) == DEFAULT_PMG)) {
            val_memory_free_pages(src_buf, num_pages);
            val_memory_free_pages(dest_buf, num_pages);
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
            continue;
        }

        mapping_saved = 0;
        if (val_mpam_msc_supports_partid_nrw(msc_index)) {
            saved_mapping = val_mpam_get_partid_mapping(msc_index, test_partid);
            mapping_saved = 1;
        }

        /* Save the request PARTID enable state before preparation. */
        enable_state_saved = val_mpam_msc_supports_partid_endis(msc_index);
        saved_enable = enable_state_saved ?
                       val_mpam_msc_partid_is_enabled(msc_index, test_partid) : true;

        /* Prepare controls only after both workload buffers are available. */
        if (val_mpam_prepare_partid(msc_index, test_partid, 0) != ACS_STATUS_PASS) {

            if (mapping_saved)
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
            restore_status = 0;
            if (enable_state_saved)
                restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                             0, test_partid);
            if (restore_status)
                val_print(ERROR, "\n       Failed to restore test PARTID enable state");
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
            val_memory_free_pages(src_buf, num_pages);
            val_memory_free_pages(dest_buf, num_pages);
            test_fail++;
            continue;
        }

        /* Use a distinct supported PMG for the negative filter scenario. */
        pmg2 = val_mpam_get_max_pmg(msc_index);

        /* Preserve the effective partition controls before changing CCAP. */
        if (!val_mpam_save_partition_state(msc_index, test_partid, &saved_partition)) {
            test_fail++;
            if (mapping_saved)
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_mapping);
            if (enable_state_saved &&
                val_mpam_msc_endis_partid(msc_index, saved_enable, 0, test_partid))
                test_fail++;
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
            val_memory_free_pages(src_buf, num_pages);
            val_memory_free_pages(dest_buf, num_pages);
            continue;
        }
        test_run = 1;

        if (val_mpam_supports_ccap(msc_index))
          val_mpam_configure_ccap(msc_index, test_partid,
                                  SOFTLIMIT_DIS, PARTITION_PERCENTAGE);

        /* Apply the test PARTID only while running this resource's workload. */
        val_mpam_reg_write(MPAM2_EL2, mpam2_el2);

        /* Preserve monitor 0 before changing its filter, control, or VALUE. */
        val_mpam_save_csumon_state(msc_index, 0, &saved_csumon);

        /* Configure CSU monitors with PMG1 */
        if (val_mpam_supports_csumon(msc_index))
            val_mpam_configure_csu_mon(msc_index, test_partid, pmg1, 0);

        storage_value1 = val_mpam_read_csumon(msc_index);
        val_print(DEBUG, "\n       Initial Value 1 = 0x%x", storage_value1);

        /* Enable CSU monitoring */
        val_mpam_csumon_enable(msc_index);

        /* wait for MAX_NRDY_USEC after msc config change */
        nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
        while (nrdy_timeout) {
            --nrdy_timeout;
        };

        /*Perform first memory transaction */
        val_memcpy(src_buf, dest_buf, buf_size);
        /* Wait for some time before the memcpy settles and counters update */
        val_time_delay_ms(TIMEOUT_MEDIUM);

        /* Read Cache storage value */
        storage_value1 = val_mpam_read_csumon(msc_index);
        val_print(DEBUG, "\n       Storage Value 1 = 0x%x", storage_value1);

        val_pe_cache_invalidate_range((uint64_t)src_buf, buf_size);
        val_pe_cache_invalidate_range((uint64_t)dest_buf, buf_size);
        val_mem_issue_dsb();

        /* Configure CSU monitors with PMG2 */
        if (val_mpam_supports_csumon(msc_index))
            val_mpam_configure_csu_mon(msc_index, test_partid, pmg2, 0);

        /* Read Cache storage value */
        storage_value2 = val_mpam_read_csumon(msc_index);
        val_print(DEBUG, "\n       Initial Value 2 = 0x%x", storage_value2);

        /* Enable CSU monitoring */
        val_mpam_csumon_enable(msc_index);

        /* wait for MAX_NRDY_USEC after msc config change */
        nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
        while (nrdy_timeout) {
            --nrdy_timeout;
        };

        /*Perform second memory transaction */
        val_memcpy(src_buf, dest_buf, buf_size);
        /* Wait for some time before the memcpy settles and counters update */
        val_time_delay_ms(TIMEOUT_MEDIUM);

        /* Read Cache storage value for PMG2 */
        storage_value2 = val_mpam_read_csumon(msc_index);
        val_print(DEBUG, "\n       Storage Value 2 = 0x%x", storage_value2);

        /* Disable the monitor */
        val_mpam_csumon_disable(msc_index);

        /* Test fails if storage_value1 is zero or storage_value2 is non zero */
        if (!storage_value1 || storage_value2) {
            val_set_status(index, RESULT_FAIL(02));

            /*Restore MPAM2_EL2 settings */
            val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

            val_mpam_restore_csumon_state(msc_index, &saved_csumon);
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
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);

            /*Free the buffers */
            if (dest_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)dest_buf, buf_size);
                val_mem_issue_dsb();
                val_memory_free_pages(dest_buf, num_pages);
            }
            if (src_buf != NULL) {
                val_pe_cache_invalidate_range((uint64_t)src_buf, buf_size);
                val_mem_issue_dsb();
                val_memory_free_pages(src_buf, num_pages);
            }

            return;
        }

        /*Restore MPAM2_EL2 settings */
        val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

        val_mpam_restore_csumon_state(msc_index, &saved_csumon);
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
        val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);

        /*Free the buffers */
        if (dest_buf != NULL) {
            val_pe_cache_invalidate_range((uint64_t)dest_buf, buf_size);
            val_mem_issue_dsb();
            val_memory_free_pages(dest_buf, num_pages);
        }
        if (src_buf != NULL) {
            val_pe_cache_invalidate_range((uint64_t)src_buf, buf_size);
            val_mem_issue_dsb();
            val_memory_free_pages(src_buf, num_pages);
        }
      }
    }

    if (test_fail)
        val_set_status(index, RESULT_FAIL(03));
    else if (test_run)
        val_set_status(index, RESULT_PASS);
    else
        val_set_status(index, RESULT_SKIP(04));
    return;
}

uint32_t monitor002_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    num_pe = 1;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
