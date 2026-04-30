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
#include "acs_common.h"
#include "acs_cxl.h"
#include "acs_pcie.h"
#include "acs_iovirt.h"
#include "acs_smmu.h"

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 2)
#define TEST_RULE  "CXL_02"
#define TEST_DESC  "Check SMMU & ATS for CXL Type1/Type2  "

static
void
payload(void)
{
  uint32_t pe_index;
  uint32_t comp_count;
  uint32_t comp_idx;
  uint32_t test_fail = 0;
  uint32_t cxl_type;
  uint32_t bdf;
  uint32_t seg_num;
  uint32_t rid;
  uint32_t smmu_idx;
  uint64_t smmu_rev;
  uint32_t idr0;
  uint32_t ats_supported;
  uint32_t test_skip = 0;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  comp_count = val_cxl_get_component_info(CXL_COMPONENT_INFO_COUNT, 0);
  if (comp_count == 0) {
    val_print(TRACE, "\n       No CXL components discovered");
    val_set_status(pe_index, RESULT_SKIP(1));
    return;
  }

  for (comp_idx = 0; comp_idx < comp_count; comp_idx++) {

    cxl_type = val_cxl_get_component_info(CXL_COMPONENT_INFO_DEVICE_TYPE, comp_idx);
    if ((cxl_type != CXL_DEVICE_TYPE_TYPE1) && (cxl_type != CXL_DEVICE_TYPE_TYPE2))
      continue;

    test_skip = 1;
    bdf = val_cxl_get_component_info(CXL_COMPONENT_INFO_BDF_INDEX, comp_idx);
    seg_num = PCIE_EXTRACT_BDF_SEG(bdf);
    rid = PCIE_CREATE_BDF_PACKED(bdf);

    smmu_idx = val_iovirt_get_rc_smmu_index(seg_num, rid);
    if (smmu_idx == ACS_INVALID_INDEX) {
      val_print(ERROR,
                "\n       CXL Type1/Type2 component not behind SMMU, BDF: 0x%x",
                bdf);
      test_fail++;
      continue;
    }

    smmu_rev = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, smmu_idx);
    if (smmu_rev < 3u) {
      val_print(ERROR, "\n       CXL path SMMU is not SMMUv3, index: %u", smmu_idx);
      test_fail++;
      continue;
    }

    idr0 = val_smmu_read_cfg(SMMUv3_IDR0, smmu_idx);
    ats_supported = VAL_EXTRACT_BITS(idr0, SMMUV3_ATS_BIT, SMMUV3_ATS_BIT);
    if (ats_supported == 0) {
      val_print(ERROR, "\n       SMMUv3 ATS unsupported, index: %u", smmu_idx);
      test_fail++;
    }
  }

  if (test_skip == 0) {
    val_print(TRACE, "\n       No CXL Type1/Type2 components discovered");
    val_set_status(pe_index, RESULT_SKIP(2));
  } else if (test_fail) {
    val_set_status(pe_index, RESULT_FAIL(1));
  } else {
    val_set_status(pe_index, RESULT_PASS);
  }
}

uint32_t
cxl002_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
