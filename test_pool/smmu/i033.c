/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

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
#include "val_interface.h"
#include "acs_smmu.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 33)
#define TEST_RULE  "SMMU_03"
#define TEST_DESC  "Check SMMU 128-bit atomicity support     "

#define SMMU_IDR5_D128_BIT  8

static
void
payload()
{
  uint32_t index;
  uint32_t num_smmu;
  uint32_t smmu_index;
  uint32_t lse2_feat;
  uint32_t d128;
  uint32_t relevant_smmu = 0;

  index = val_pe_get_index_mpid(val_pe_get_mpid());

  /* ID_AA64MMFR2_EL1.AT != 0 indicates that the PE implements FEAT_LSE2. */
  lse2_feat = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64MMFR2_EL1), 32, 35);

  if (lse2_feat == 0) {
    val_print(DEBUG, "\n       SMMU_03 not applicable: FEAT_LSE2 not supported");
    val_set_status(index, RESULT_SKIP(1));
    return;
  }

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
    val_print(DEBUG, "\n       No SMMU Controllers are discovered");
    val_set_status(index, RESULT_SKIP(2));
    return;
  }

  for (smmu_index = 0; smmu_index < num_smmu; smmu_index++) {
    /* SMMU_IDR5.D128 is defined for SMMUv3 controllers. */
    if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, smmu_index) < 3) {
      val_print(DEBUG, "\n       SMMU %x is not an SMMUv3 controller", smmu_index);
      continue;
    }

    relevant_smmu++;
    /* IDR5.D128 indicates support for the required 128-bit atomicity. */
    d128 = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR5, smmu_index),
                            SMMU_IDR5_D128_BIT, SMMU_IDR5_D128_BIT);
    val_print(DEBUG, "\n       SMMU %x IDR5.D128 = 0x%x", smmu_index, d128);

    if (d128 == 0) {
      val_print(ERROR, "\n       SMMU does not support 128-bit single-copy atomicity");
      val_set_status(index, RESULT_FAIL(1));
      return;
    }
  }

  if (relevant_smmu == 0) {
    val_print(DEBUG, "\n       SMMU_03 not applicable: No SMMUv3 controllers");
    val_set_status(index, RESULT_SKIP(3));
    return;
  }

  val_set_status(index, RESULT_PASS);
}

uint32_t
i033_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  // This test is run on a single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
