/** @file
 * Copyright (c) 2016-2018, 2021,2024-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "val/include/acs_val.h"
#include "val/include/acs_smmu.h"
#include "val/include/acs_pcie.h"

#define TEST_NUM   (ACS_SMMU_TEST_NUM_BASE + 7)
#define TEST_RULE  "B_SMMU_21, SMMU_01"
#define TEST_DESC  "SMMUv3 Integration compliance         "

static
void
payload()
{

  uint64_t data;
  uint32_t num_smmu;
  uint32_t index;

  index = val_pe_get_index_mpid(val_pe_get_mpid());

  data = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
  if (data == 0) {
      val_print(ACS_PRINT_WARN, "\n       PCIe Subsystem not  discovered   ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);

  if (num_smmu == 0) {
      val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  while (num_smmu--) {
      if (val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu) == 2) {
          val_print(ACS_PRINT_WARN, "\n       Not valid for SMMU v2           ", 0);
          val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
          return;
      }

      data = val_smmu_read_cfg(SMMUv3_IDR0, num_smmu);
      // Check I/O coherent access
      if (VAL_EXTRACT_BITS(data, 4, 4) == 0)      {
          val_print(ACS_PRINT_ERR, "\n\t IO-Coherent access not supported  ", 0);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
          return;
      }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));

}

uint32_t
i007_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
