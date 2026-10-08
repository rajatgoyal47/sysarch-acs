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
#include "val_interface.h"

#define TEST_NUM   ACS_MPAM_ERROR_TEST_NUM_BASE  +  16
#define TEST_DESC  "Check MPAMF_ESR.RIS field correctness "
#define TEST_RULE  "QBKDG"

/*
 * Test algorithm:
 * 1. Iterate over MSCs that implement RIS selection, ESR, and extended ESR fields.
 * 2. Save MPAMF_ECR, clear stale error status, and choose RIS_MAX + 1 as undefined RIS.
 * 3. For MPAMCFG access, use a request PARTID without narrowing or an internal PARTID with it.
 * 4. Program undefined RIS, access MPAMCFG, and read MPAMF_ESR.
 * 5. For error 8 or 9, verify ESR.RIS. Accept no error, but reject unrelated errors.
 * 6. Restore MPAMCFG_PART_SEL and clear the error status.
 * 7. Program undefined RIS in MSMON_CFG_MON_SEL and access MSMON when monitors exist.
 * 8. For error 10 or 11, verify ESR.RIS. Accept no error, but reject unrelated errors.
 * 9. Restore all modified state and report the aggregate result.
 */

static
void payload(void)
{
    uint32_t status;
    uint64_t data;
    uint32_t msc_index;
    uint32_t pe_index;
    uint32_t total_nodes;
    uint32_t mpamf_ecr;
    uint32_t data_original;
    uint64_t esr_value;
    uint32_t esr_errcode;
    uint32_t programmed_ris;
    uint32_t max_ris_index;
    uint32_t internal;
    uint32_t test_fail = 0;
    uint32_t test_skip = 1;
    uint32_t partid;

    /* Get the executing PE and number of MSCs to inspect. */
    pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
    total_nodes = val_mpam_get_msc_count();

    /* Validate ESR.RIS behavior independently on every eligible MSC. */
    for (msc_index = 0; msc_index < total_nodes; msc_index++) {

      /* RIS selection is required to create a misconfigured RIS value. */
      if (!val_mpam_msc_supports_ris(msc_index)) {
        val_print(DEBUG, "\n       MSC index %d does not support RIS", msc_index);
        continue;
      }

      /* ESR support is required to observe the generated error. */
      if (!val_mpam_msc_supports_esr(msc_index)) {
        val_print(DEBUG, "\n       MSC index %d does not support ESR", msc_index);
        continue;
      }

      /* The RIS syndrome field requires extended ESR support. */
      if (!val_mpam_msc_supports_extd_esr(msc_index)) {
        val_print(DEBUG, "\n       MSC index %d does not support EXTD_ESR", msc_index);
        continue;
      }

      /* Preserve error control and clear stale error status. */
      mpamf_ecr = val_mpam_mmr_read(msc_index, REG_MPAMF_ECR);

      status = val_mpam_msc_reset_errcode(msc_index);
      if (!status) {
        val_set_status(pe_index, RESULT_FAIL(01));
        return;
      }

      /* RIS_MAX + 1 must fit in the architectural RIS selector field. */
      max_ris_index = val_mpam_get_max_ris_count(msc_index);
      if (max_ris_index == BITFIELD_READ(PART_SEL_RIS, ACS_UINT32_MAX)) {
        /* All selector values are in range, so this MSC cannot be tested. */
        val_print(DEBUG, "\n       MSC index %d supports all RIS", msc_index);
        continue;
      }

      test_skip = 0;

      /********* Case 1: MPAMCFG access *********/
      /* Preserve MPAMCFG_PART_SEL before programming the invalid selector. */
      data_original = val_mpam_mmr_read(msc_index, REG_MPAMCFG_PART_SEL);

      /* Use an internal PARTID for configuration only when narrowing is implemented. */
      programmed_ris = max_ris_index + 1;
      internal = val_mpam_msc_supports_partid_nrw(msc_index);
      partid = internal ? 0 : val_mpam_get_max_partid(msc_index);

      /* Keep PARTID valid so undefined RIS is the only invalid selector component. */
      data = BITFIELD_SET(PART_SEL_PARTID_SEL, partid) |
             BITFIELD_SET(PART_SEL_RIS, programmed_ris) |
             BITFIELD_SET(PART_SEL_INTERNAL, internal);

      /* Program the undefined RIS for the MPAMCFG access. */
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data);
      val_print(DEBUG, "\n       Value written to MPAMCFG_PART_SEL is %llx", data);

      /* Access MPAMCFG to exercise optional undefined-RIS detection. */
      val_mpam_mmr_read(msc_index, REG_MPAMCFG_CMAX);

      /* Allow a detected error to be reflected in MPAMF_ESR. */
      val_time_delay_ms(100 * ONE_MILLISECOND);

      /* Validate ESR.RIS only for errors caused by a misconfigured MPAMCFG RIS. */
      esr_value = val_mpam_mmr_read64(msc_index, REG_MPAMF_ESR);
      esr_errcode = (esr_value >> ESR_ERRCODE_SHIFT) & ESR_ERRCODE_MASK;

      if ((esr_errcode == ESR_ERRCODE_UNDEF_RIS_PART_SEL) ||
          (esr_errcode == ESR_ERRCODE_RIS_NO_CTRL)) {
        /* A reported RIS error must identify the programmed MPAMCFG RIS. */
        if (((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK) != programmed_ris) {
          val_print(ERROR, "\n       ESR.RIS mismatch MPAMCFG access: expected 0x%x",
                              programmed_ris);
          val_print(ERROR, ", Received 0x%x",
                              ((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK));
          test_fail++;
        }
      } else if (esr_errcode == ESR_ERRCODE_NO_ERROR) {
        /* Error detection is optional for this access. */
        val_print(DEBUG, "\n       Undefined RIS error not reported for MPAMCFG access");
      } else {
        /* Reject unrelated errors instead of interpreting their RIS field. */
        val_print(ERROR, "\n       Unexpected ERRCODE for MPAMCFG access: Received 0x%x",
                                    esr_errcode);
        test_fail++;
      }

      /* Restore the configuration selector and clear this case error. */
      val_mpam_mmr_write(msc_index, REG_MPAMCFG_PART_SEL, data_original);
      val_mpam_msc_reset_errcode(msc_index);

      /********* Case 2: MSMON access *********/
      /* Test monitor RIS reporting only when monitoring is implemented. */
      if (val_mpam_msc_supports_mon(msc_index)) {
        /* Preserve the monitor selector before programming the invalid RIS. */
        data_original = val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MON_SEL);
        programmed_ris = max_ris_index + 1;

        /* Select monitor zero and the same undefined RIS. */
        data = BITFIELD_SET(MON_SEL_MON_SEL, 0) |
               BITFIELD_SET(MON_SEL_RIS, programmed_ris);

        /* Program the undefined RIS for the MSMON access. */
        val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, data);
        val_print(DEBUG, "\n       Value written to MSMON_CFG_MON_SEL is %llx", data);

        /* Access MSMON to exercise optional undefined-RIS detection. */
        val_mpam_mmr_read(msc_index, REG_MSMON_CFG_MBWU_FLT);

        /* Allow a detected error to be reflected in MPAMF_ESR. */
        val_time_delay_ms(100 * ONE_MILLISECOND);

        /* Validate ESR.RIS only for errors caused by a misconfigured monitor RIS. */
        esr_value = val_mpam_mmr_read64(msc_index, REG_MPAMF_ESR);
        esr_errcode = (esr_value >> ESR_ERRCODE_SHIFT) & ESR_ERRCODE_MASK;

        if ((esr_errcode == ESR_ERRCODE_UNDEF_RIS_MON_SEL) ||
            (esr_errcode == ESR_ERRCODE_RIS_NO_MON)) {
          /* A reported RIS error must identify the programmed monitor RIS. */
          if (((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK) != programmed_ris) {
            val_print(ERROR, "\n       ESR.RIS mismatch MSMON_CFG access: expected 0x%x",
                                programmed_ris);
            val_print(ERROR, ", Received 0x%x",
                                ((esr_value >> ESR_RIS_SHIFT) & ESR_RIS_MASK));
            test_fail++;
          }
        } else if (esr_errcode == ESR_ERRCODE_NO_ERROR) {
          /* Error detection is optional for this access. */
          val_print(DEBUG, "\n       Undefined RIS error not reported for MSMON access");
        } else {
          /* Reject unrelated errors instead of interpreting their RIS field. */
          val_print(ERROR, "\n       Unexpected ERRCODE for MSMON access: Received 0x%x",
                                      esr_errcode);
          test_fail++;
        }

        /* Restore the monitor selector and clear this case error. */
        val_mpam_mmr_write(msc_index, REG_MSMON_CFG_MON_SEL, data_original);
        val_mpam_msc_reset_errcode(msc_index);
      }

      /* Restore the original error-control configuration. */
      val_mpam_mmr_write(msc_index, REG_MPAMF_ECR, mpamf_ecr);
    }

    /* Report one aggregate result for all eligible MSCs. */
    if (test_skip)
        val_set_status(pe_index, RESULT_SKIP(01));
    else if (test_fail)
        val_set_status(pe_index, RESULT_FAIL(02));
    else
        val_set_status(pe_index, RESULT_PASS);
}

uint32_t error013_entry(uint32_t num_pe)
{

    uint32_t status = ACS_STATUS_FAIL;

    /* This test executes on one PE. */
    num_pe = 1;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

    /* Run the payload unless execution policy skips this rule. */
    if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* Collect and report the test result. */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

    val_report_status(0, ACS_END(TEST_NUM), NULL);

    return status;
}
