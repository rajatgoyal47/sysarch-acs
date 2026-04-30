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
#include "acs_pcie.h"
#include "acs_cxl.h"

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 1)
#define TEST_RULE  "CXL_01"
#define TEST_DESC  "Check CXL Version                    "

static void
payload(void)
{
  uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t num_cxl_hb;
  uint32_t num_cxl_comp;
  uint32_t cxl_ver;
  uint32_t pciecs_base;
  uint32_t vh_capable;
  uint32_t bdf;
  uint32_t index;
  uint32_t reg_value;
  uint32_t test_fails = 0;

  num_cxl_hb = val_cxl_get_info(CXL_INFO_NUM_DEVICES, 0);
  if (num_cxl_hb == 0) {
      val_print(DEBUG, "\n       No CXL devices discovered");
      val_set_status(pe_index, RESULT_SKIP(1));
      return;
  }

  for (index = 0; index < num_cxl_hb; index++) {
      cxl_ver = val_cxl_get_info(CXL_INFO_VERSION, index);
      if (!cxl_ver)
          test_fails++;
  }

  num_cxl_comp = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  for (index = 0; index < num_cxl_comp; index++) {
      vh_capable = 0;
      bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, index);
      if (val_cxl_find_capability(bdf, CXL_DVSEC_ID_PCIE_FLEXBUS_PORT, &pciecs_base)) {
          val_print(ERROR, "\n CXL Flexbus port capability absent for bdf 0x%0x", bdf);
          test_fails++;
          continue;
      }

      val_pcie_read_cfg(bdf, pciecs_base + CXL_DVSEC_HDR2_OFFSET, &reg_value);
      if ((reg_value & CXL_DVSEC_HDR2_ID_MASK) != CXL_DVSEC_ID_PCIE_FLEXBUS_PORT) {
          val_print(ERROR, "\n CXL Flexbus port ID mismatch for bdf 0x%0x", bdf);
          test_fails++;
          continue;
      }

      vh_capable = ((reg_value >> CXL_68BVH_SHIFT) & 1);
      if (!vh_capable)
          test_fails++;

  }

  if (test_fails)
      val_set_status(pe_index, RESULT_FAIL(1));
  else
      val_set_status(pe_index, RESULT_PASS);
}

uint32_t
cxl001_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
