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


#define TEST_NUM   ACS_MPAM_CACHE_TEST_NUM_BASE + 1
#define TEST_RULE  "XPPGG"
#define TEST_DESC  "Check CPOR Partitioning               "

#define CPOR_SCENARIO_MAX 10
static uint64_t mpam2_el2_temp;

typedef struct {
    char8_t description[64];
    uint8_t partition_percent;  //as function of max cpor cache size
    uint8_t cache_percent;      //as function of max cpor cache size
    uint8_t config_enable;
} cpor_config_t;

static cpor_config_t cpor_config_data[] = {
    {"CPOR check for 75% partition size", 75, 75, true},
    {"CPOR check for 25% partition size", 25, 25, true}
};

static void free_counter_buffer(uint64_t **counter, uint32_t row_count)
{
    uint32_t row;

    if (counter == NULL)
        return;

    for (row = 0; row < row_count; row++) {
        if (counter[row] != NULL)
            val_memory_free(counter[row]);
    }
    val_memory_free(counter);
}

static void payload(void)
{
    uint8_t enabled_scenarios = 0;
    uint32_t cpor_nodes = 0;
    uint32_t cfg_index = 0;
    uint32_t start_count = 0;
    uint32_t end_count = 0;
    void *src_buf = 0;
    void *dest_buf = 0;
    uint64_t buf_size;
    uint32_t page_size;
    uint32_t num_pages;
    uint64_t num_pages64;
    uint64_t nrdy_timeout;
    uint64_t **counter;
    uint64_t it;
    uint64_t mpam2_el2 = 0;
    uint32_t msc_node_cnt = val_mpam_get_msc_count();
    uint32_t rsrc_node_cnt;
    uint32_t msc_index;
    uint32_t rsrc_index;
    uint32_t llc_index;
    uint64_t cache_identifier;
    uint32_t cache_maxsize = 0;
    uint32_t test_partid = 1; //Selecting distinct partid
    uint32_t max_scenario_index = 0;
    uint32_t required_max_partid;
    uint32_t counter_row_size;
    uint64_t counter_row_size64;
    uint32_t saved_int_partid;
    uint32_t saved_part_sel;
    uint32_t saved_mon_sel;
    uint32_t narrowing_active;
    uint32_t restore_status;
    bool saved_enable;
    bool enable_state_saved;
    MPAM_PARTITION_STATE saved_partition;
    MPAM_CSUMON_STATE saved_csumon;
    uint32_t csumon_count = 0;
    uint32_t resource_slot;
    uint32_t current_slot;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t test_fail = 0;

    /* Determine the largest request and internal PARTID needed by enabled scenarios. */
    for (cfg_index = 0;
         cfg_index < sizeof(cpor_config_data) / sizeof(cpor_config_t);
         cfg_index++) {
      if (cpor_config_data[cfg_index].config_enable)
        max_scenario_index = cfg_index;
    }
    required_max_partid = test_partid + max_scenario_index;

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

        /* Select the resource before checking its capabilities. */
        if (val_mpam_msc_supports_ris(msc_index))
          val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

        if ((val_mpam_get_max_partid(msc_index) < required_max_partid) ||
            (val_mpam_msc_supports_partid_nrw(msc_index) &&
             (val_mpam_get_max_intpartid(msc_index) < max_scenario_index)))
          continue;

        if (val_mpam_supports_cpor(msc_index) &&
            val_mpam_supports_csumon(msc_index) &&
            (val_mpam_get_csumon_count(msc_index) != 0U)) {
          cache_maxsize = GET_MAX_VALUE(cache_maxsize,
                              val_cache_get_info(CACHE_SIZE, llc_index));
          csumon_count = val_mpam_get_csumon_count(msc_index);
          cpor_nodes++;
        }
      }

      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
      val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
    }

    val_print(DEBUG, "\n       CPOR Nodes = %d", cpor_nodes);
    val_print(DEBUG, "\n       Test PARTID = %d", test_partid);
    val_print(DEBUG, "\n       Cache Max Size = 0x%x", cache_maxsize);
    val_print(DEBUG, "\n       Number of CSU Monitors = %d", csumon_count);

    /* Skip the test if CSU monitors/ nodes supporting Cache Portion Partitoning are 0 */
    if ((cpor_nodes == 0) || (csumon_count == 0)) {
        val_set_status(index, RESULT_SKIP(03));
        return;
    }

    /* Both allocation and initialization APIs accept a 32-bit byte count. */
    counter_row_size64 = (uint64_t)cpor_nodes * sizeof(uint64_t);
    if ((counter_row_size64 == 0U) || (counter_row_size64 > ACS_UINT32_MAX)) {
        val_print(ERROR, "\n       Counter row size exceeds allocator limit");
        val_set_status(index, RESULT_FAIL(01));
        return;
    }
    counter_row_size = (uint32_t)counter_row_size64;

    /* Dynamically create the CSU Mon Counter buffer
     * uint64_t counter[CPOR_SCENARIO_MAX][cpor_nodes]
     */
    counter = (uint64_t **) val_memory_alloc(CPOR_SCENARIO_MAX * sizeof (uint64_t *));
    if (counter == NULL) {
        val_set_status(index, RESULT_FAIL(01));
        return;
    }
    val_memory_set(counter, CPOR_SCENARIO_MAX * sizeof(uint64_t *), 0);
    for (it = 0; it < CPOR_SCENARIO_MAX; it++) {
        counter[it] = val_memory_alloc(counter_row_size);
        if (counter[it] == NULL) {
            free_counter_buffer(counter, CPOR_SCENARIO_MAX);
            val_set_status(index, RESULT_FAIL(01));
            return;
        }
        val_memory_set(counter[it], counter_row_size, 0);
    }

    mpam2_el2 = val_mpam_reg_read(MPAM2_EL2);
    mpam2_el2_temp = mpam2_el2;

    for (cfg_index = 0; cfg_index < sizeof(cpor_config_data)/sizeof(cpor_config_t); cfg_index++) {

      /* Use one distinct request PARTID per enabled scenario. */
      test_partid = 1U + cfg_index;
      /* Clear the PARTID_D & PMG_D bits in mpam2_el2 before writing to them */
      mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2,
                                    MPAMn_ELx_PARTID_D_SHIFT+15, MPAMn_ELx_PARTID_D_SHIFT);
      mpam2_el2 = CLEAR_BITS_M_TO_N(mpam2_el2,
                                    MPAMn_ELx_PMG_D_SHIFT+7, MPAMn_ELx_PMG_D_SHIFT);

      /* Write test_partid & DEFAULT PMG to mpam2_el2 to generate PE traffic */
      mpam2_el2 |= (((uint64_t)DEFAULT_PMG << MPAMn_ELx_PMG_D_SHIFT) |
                    ((uint64_t)test_partid << MPAMn_ELx_PARTID_D_SHIFT));

      if (cpor_config_data[cfg_index].config_enable) {
        if (enabled_scenarios >= CPOR_SCENARIO_MAX) {
          val_print(ERROR, "\n       Enabled CPOR scenarios exceed allocated counter rows");
          test_fail++;
          goto scenarios_done;
        }

        resource_slot = 0;

        /* Configure CPOR settings for nodes supporting CPOR */
        for (msc_index = 0; msc_index < msc_node_cnt; msc_index++) {
          val_print(DEBUG, "\n       Running for cfg_index = %d", cfg_index);
          val_print(DEBUG, ", msc_index = %d", msc_index);

          rsrc_node_cnt = val_mpam_get_info(MPAM_MSC_RSRC_COUNT, msc_index, 0);
          for (rsrc_index = 0; rsrc_index < rsrc_node_cnt; rsrc_index++) {

            if (val_mpam_get_info(MPAM_MSC_RSRC_TYPE, msc_index, rsrc_index) !=
                                                                MPAM_RSRC_TYPE_PE_CACHE)
              continue;

            if (val_mpam_get_info(MPAM_MSC_RSRC_DESC1, msc_index, rsrc_index) != cache_identifier)
              continue;

            start_count = 0;
            end_count = 0;

            /* Preserve selectors before selecting this resource instance. */
            saved_part_sel = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
            saved_mon_sel = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);

            /* Select resource instance if RIS feature implemented */
            if (val_mpam_msc_supports_ris(msc_index))
              val_mpam_memory_configure_ris_sel(msc_index, rsrc_index);

            if (!val_mpam_supports_cpor(msc_index) ||
                !val_mpam_supports_csumon(msc_index) ||
                (val_mpam_get_csumon_count(msc_index) == 0U) ||
                (val_mpam_get_max_partid(msc_index) < required_max_partid) ||
                (val_mpam_msc_supports_partid_nrw(msc_index) &&
                 (val_mpam_get_max_intpartid(msc_index) < max_scenario_index))) {
              val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
              val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
              continue;
            }

            if (resource_slot >= cpor_nodes) {
              val_print(ERROR, "\n       Resource count changed during test");
              val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
              val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
              test_fail++;
              continue;
            }
            current_slot = resource_slot++;

            narrowing_active = val_mpam_msc_supports_partid_nrw(msc_index);
            if (narrowing_active)
              saved_int_partid = val_mpam_get_partid_mapping(msc_index, test_partid);

            enable_state_saved = val_mpam_msc_supports_partid_endis(msc_index);
            saved_enable = enable_state_saved ?
                           val_mpam_msc_partid_is_enabled(msc_index, test_partid) : true;

            /* Prepare a distinct internal control when narrowing is implemented. */
            if (val_mpam_prepare_partid(msc_index, test_partid,
                                        (uint16_t)cfg_index) != ACS_STATUS_PASS) {
              test_fail++;
              if (narrowing_active)
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_int_partid);
              restore_status = 0;
              if (enable_state_saved)
                restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                            0, test_partid);
              if (restore_status)
                test_fail++;
              val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
              val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
              continue;
            }

            /* Preserve controls for both narrowed and non-narrowed operation. */
            if (!val_mpam_save_partition_state(msc_index, test_partid, &saved_partition)) {
              test_fail++;
              if (narrowing_active)
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_int_partid);
              restore_status = 0;
              if (enable_state_saved)
                restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                            0, test_partid);
              if (restore_status)
                test_fail++;
              val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
              val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
              continue;
            }

            val_mpam_configure_cpor(msc_index, test_partid,
                          cpor_config_data[cfg_index].partition_percent);
            if (val_mpam_supports_ccap(msc_index))
              val_mpam_configure_ccap(msc_index, test_partid, 0, 100);

            /* Use the same two-LLC working set to create sustained cache pressure. */
            buf_size = cache_maxsize;
            page_size = val_memory_page_size();
            if (page_size == 0U) {
                val_print(ERROR, "\n       Invalid memory page size");
                test_fail++;
                val_mpam_restore_partition_state(msc_index, &saved_partition);
                if (narrowing_active)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_int_partid);
                if (enable_state_saved)
                    val_mpam_msc_endis_partid(msc_index, saved_enable, 0, test_partid);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                goto scenarios_done;
            }

            num_pages64 = (buf_size / page_size) + ((buf_size % page_size) != 0U);
            /* Validate the allocator page count before calculating its byte capacity. */
            if ((num_pages64 == 0U) || (num_pages64 > ACS_UINT32_MAX) ||
                (num_pages64 > (~(uint64_t)0 / page_size)) ||
                ((num_pages64 * page_size) < buf_size)) {
                val_print(ERROR, "\n       Invalid page count for workload buffers");
                test_fail++;
                val_mpam_restore_partition_state(msc_index, &saved_partition);
                if (narrowing_active)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_int_partid);
                if (enable_state_saved)
                    val_mpam_msc_endis_partid(msc_index, saved_enable, 0, test_partid);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                goto scenarios_done;
            }
            num_pages = (uint32_t)num_pages64;

            /*Allocate memory for source and destination buffers */
            src_buf = (void *)val_memory_alloc_pages(num_pages);
            dest_buf = (void *)val_memory_alloc_pages(num_pages);

            if ((src_buf == NULL) || (dest_buf == NULL)) {
                val_print(ERROR, "\n       Mem allocation failed");
                val_set_status(index, RESULT_FAIL(01));
                /* Cleanup must run with the original PE labels. */
                val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);
                if (dest_buf != NULL)
                    val_memory_free_pages(dest_buf, num_pages);
                if (src_buf != NULL)
                    val_memory_free_pages(src_buf, num_pages);
                /* Restore controls, mapping, enable state, and selectors. */
                val_mpam_restore_partition_state(msc_index, &saved_partition);
                if (narrowing_active)
                    val_mpam_set_partid_mapping(msc_index, test_partid, saved_int_partid);
                if (enable_state_saved)
                    val_mpam_msc_endis_partid(msc_index, saved_enable, 0, test_partid);
                val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
                val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
                free_counter_buffer(counter, CPOR_SCENARIO_MAX);
                return;

            }

            /* val_memcpy() reads from dest_buf. Initialize it and make both
             * buffers cold so each CPOR scenario starts with the same workload.
             */
            val_memory_set(dest_buf, (uint32_t)buf_size, 0xA5U);
            val_pe_cache_invalidate_range((uint64_t)src_buf, buf_size);
            val_pe_cache_invalidate_range((uint64_t)dest_buf, buf_size);
            val_mem_issue_dsb();

            /* Preserve monitor 0 before changing its selector, filter, or control. */
            val_mpam_save_csumon_state(msc_index, 0, &saved_csumon);

            /* Configure CSU Monitor */
            val_mpam_configure_csu_mon(msc_index, test_partid, DEFAULT_PMG, 0);

            /* Enable monitoring */
            val_mpam_csumon_enable(msc_index);

            /* wait for MAX_NRDY_USEC after msc config change */
            nrdy_timeout = val_mpam_get_info(MPAM_MSC_NRDY, msc_index, 0);
            while (nrdy_timeout) {
                --nrdy_timeout;
            };

            start_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       Start Count = 0x%lx", start_count);

            /* Attribute only the measured workload to the test PARTID. */
            val_mpam_reg_write(MPAM2_EL2, mpam2_el2);
            val_memcpy(src_buf, dest_buf, buf_size);
            val_mem_issue_dsb();
            val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

            /* Wait for some time before the memcpy settles and counters update */
            val_time_delay_ms(TIMEOUT_MEDIUM);

            end_count = val_mpam_read_csumon(msc_index);
            val_print(DEBUG, "\n       End Count = 0x%lx", end_count);

            /* A partitioning result is not valid unless the monitor observed traffic. */
            if (end_count <= start_count) {
              val_print(ERROR, "\n       CSU monitor did not advance for msc_index : %d",
                        msc_index);
              test_fail++;
              counter[enabled_scenarios][current_slot] = 0;
            } else {
              /* Compare only the cache usage added by this workload. */
              counter[enabled_scenarios][current_slot] = end_count - start_count;
            }

            /* Disable CSU MON */
            val_mpam_csumon_disable(msc_index);

            /* Restore the monitor and partition controls modified by this scenario. */
            val_mpam_restore_csumon_state(msc_index, &saved_csumon);
            val_mpam_restore_partition_state(msc_index, &saved_partition);

            /* Free the buffers to the heap manager */
            val_pe_cache_invalidate_range((uint64_t)dest_buf, buf_size);
            val_mem_issue_dsb();
            val_memory_free_pages(dest_buf, num_pages);
            val_pe_cache_invalidate_range((uint64_t)src_buf, buf_size);
            val_mem_issue_dsb();
            val_memory_free_pages(src_buf, num_pages);

            /* Restore the original mapping, enable state, and selectors. */
            if (narrowing_active)
                val_mpam_set_partid_mapping(msc_index, test_partid, saved_int_partid);
            restore_status = 0;
            if (enable_state_saved)
                restore_status = val_mpam_msc_endis_partid(msc_index, saved_enable,
                                                            0, test_partid);
            if (restore_status)
                test_fail++;
            val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, saved_part_sel);
            val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, saved_mon_sel);
          }
        }
        enabled_scenarios++;
      }
    }

scenarios_done:
    /* Restore MPAM2_EL2 settings */
    val_mpam_reg_write(MPAM2_EL2, mpam2_el2_temp);

    /* Compare each resource instance independently across scenarios. */
    for (resource_slot = 0; resource_slot < cpor_nodes; resource_slot++) {
      for (cfg_index = 1; cfg_index < enabled_scenarios; cfg_index++) {
        if (counter[cfg_index][resource_slot] >
            counter[cfg_index - 1][resource_slot]) {
          val_print(ERROR, "\n       Failed for resource slot : %d", resource_slot);
          val_print(ERROR, "\n       cfg_index : %d", cfg_index-1);
          val_print(ERROR, ", CSU Mon Count :0x%lx",
                    counter[cfg_index - 1][resource_slot]);
          val_print(ERROR, "\n       cfg_index : %d", cfg_index);
          val_print(ERROR, ", CSU Mon Count :0x%lx",
                    counter[cfg_index][resource_slot]);
          test_fail++;
        }
      }
    }

    if (test_fail)
      val_set_status(index, RESULT_FAIL(02));
    else
      val_set_status(index, RESULT_PASS);

    free_counter_buffer(counter, CPOR_SCENARIO_MAX);
    return;
}

uint32_t partition001_entry(uint32_t num_pe)
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
