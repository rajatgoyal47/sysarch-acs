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
 * 1. Iterate over MSCs that implement resource selection and error reporting.
 * 2. Save the error-control and partition-selector registers, then clear ESR.
 * 3. Program RIS_MAX + 1 as an undefined resource selector.
 * 4. Read back the selector and continue only if the undefined RIS is retained.
 * 5. Clear any error caused by programming the selector.
 * 6. Read MPAMF_IDR and verify that all HAS_* fields are zero.
 * 7. Verify that the ID-register access does not record an error.
 * 8. Restore the selector, error status, and error-control register.
 */

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  15
#define TEST_DESC  "Check MPAM IDR Zero for Undefined RIS "
#define TEST_RULE  ""

static
void payload(void)
{

    uint32_t msc_index;
    uint32_t status;
    uint32_t pe_index;
    uint16_t partid;
    uint32_t total_nodes;
    uint32_t mpamf_ecr;
    uint32_t esr_errcode;
    uint32_t data;
    uint32_t data_original;
    uint32_t max_ris_index;
    uint32_t undefined_ris;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint64_t idr_value = 0;

    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    /* Validate ID-register behavior on every eligible MSC. */
    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

      val_print(DEBUG, "\n       MSC index : %d", msc_index);

      /* Resource selection is required to create an undefined RIS state. */
      if (!val_mpam_msc_supports_ris(msc_index)) {
        val_print(DEBUG, "\n       MSC index %d does not support RIS", msc_index);
        continue;
      }

      /* Error reporting is required to verify that the ID-register access is error-free. */
      if (!val_mpam_msc_supports_esr(msc_index)) {
        val_print(DEBUG, "\n       MSC index %d does not support ESR/ECR", msc_index);
        continue;
      }

      /* Preserve error control and clear stale errors. */
      mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);

      status    = val_mpam_msc_reset_errcode(msc_index);
      if (!status) {
        val_print(DEBUG, "\n       Error Code Reset Failed");
        val_set_status(pe_index, RESULT_FAIL(01));
        return;
      }

      /* Determine the first undefined RIS value representable by the selector. */
      partid = val_mpam_get_max_partid(msc_index);
      max_ris_index = val_mpam_get_max_ris_count(msc_index);
      if (max_ris_index == BITFIELD_READ(PART_SEL_RIS, ACS_UINT32_MAX)) {
        val_print(DEBUG, "\n       MSC index %d has no undefined RIS value", msc_index);
        val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
        continue;
      }

      undefined_ris = max_ris_index + 1U;

      data = BITFIELD_SET(PART_SEL_PARTID_SEL, partid) |
             BITFIELD_SET(PART_SEL_RIS, undefined_ris);

      /* Save the selector before programming the undefined RIS. */
      data_original = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);

      /* Program and read back the undefined resource selector. */
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);
      data = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);
      val_print(DEBUG, "\n       Value Read Back from MPAMCFG_PART_SEL is %llx", data);

      /* Skip when the implementation does not retain the undefined RIS. */
      if (BITFIELD_READ(PART_SEL_RIS, data) != undefined_ris) {
        val_print(DEBUG, "\n       Undefined RIS was not retained for MSC index %d", msc_index);
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data_original);
        val_mpam_msc_reset_errcode(msc_index);
        val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
        continue;
      }

      /* Allow selector-write error reporting to complete. */
      val_time_delay_ms(100 * ONE_MILLISECOND);

      /* Clear any error caused by writing the selector. */
      status    = val_mpam_msc_reset_errcode(msc_index);
      if (!status) {
        val_print(DEBUG, "\n       Error Code Reset Failed after RIS");
        val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data_original);
        val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
        val_set_status(pe_index, RESULT_FAIL(02));
        return;
      }

      test_skip = 0;

      /* Read the feature ID register under the retained undefined RIS. */
      idr_value = val_mpam_mmr_read64(msc_index, REG_MPAMF_IDR);

      /* All feature-presence fields must read as zero. */
      if (idr_value & MPAMF_IDR_HAS_MASK) {
        val_print(ERROR, "\n       ID Register HAS_* Field Non Zero : %llx", idr_value);
        test_fail++;
      }
      /* The ID-register access must not record an error. */
      esr_errcode = val_mpam_msc_get_errcode(msc_index);
      val_print(DEBUG, "\n       Error code read is %llx", esr_errcode);

      if (esr_errcode != ESR_ERRCODE_NO_ERROR) {
        val_print(ERROR, "\n       Expected errcode: %d", ESR_ERRCODE_NO_ERROR);
        val_print(ERROR, "\n       Actual errcode: %d", esr_errcode);
        test_fail++;
      }

      /* Restore selector and error state. */
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data_original);
      val_mpam_msc_reset_errcode(msc_index);

      /* Restore the original error-control configuration. */
      val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);

    }

    /* Report the aggregate result from all eligible MSCs. */
    if (test_skip)
      val_set_status(pe_index, RESULT_SKIP(01));
    else if (test_fail)
      val_set_status(pe_index, RESULT_FAIL(03));
    else
      val_set_status(pe_index, RESULT_PASS);
    return;
}

uint32_t
error012_entry(uint32_t num_pe)
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
