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
#include "pal_interface.h"
#include "acs_pe.h"
#include "acs_memory.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 10)
#define TEST_RULE  "CXL_10"
#define TEST_DESC  "Validate PCMO support for CXL                  "

static void
payload(void)
{
  uint32_t pe_index;
  uint32_t dpb_field;
  uint32_t num_cxl_hb, index;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

  num_cxl_hb = val_cxl_get_info(CXL_INFO_NUM_DEVICES, 0);
  if (num_cxl_hb == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       No CXL devices discovered", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  for (index = 0; index < num_cxl_hb; index++) {
      if (val_cxl_check_persistent_memory(index)) {
          val_print(ACS_PRINT_DEBUG, "\n       No Persistent memory supported", 0);
          val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 2));
          return;
      }
  }

  dpb_field = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64ISAR1_EL1), 0, 3);

  if ((dpb_field == 0x1) || (dpb_field == 0x2)) {
    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 1));
  } else {
    val_print(ACS_PRINT_ERR, "\n       ID_AA64ISAR1_EL1.DPB does not indicate PCMO support", 0);
    val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 1));
  }
}

uint32_t
cxl010_entry(uint32_t num_pe)
{
  uint32_t status;

  num_pe = 1;

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
    val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
