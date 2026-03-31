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

#define TEST_NUM   (ACS_CXL_TEST_NUM_BASE + 4)
#define TEST_RULE  "CXL_04"
#define TEST_DESC  "Validate CHBCR capability registers"

typedef struct {
  uint32_t cap_id;
  uint8_t requirement;
} capability_check_t;

#define CAP_REQ_MANDATORY     0x1
#define CAP_REQ_NOT_PERMITTED 0x2

static const capability_check_t g_capability_checks[] = {
  {CXL_CAPID_COMPONENT_CAP,     CAP_REQ_MANDATORY},
  {CXL_CAPID_HDM_DECODER,       CAP_REQ_MANDATORY},
  {CXL_CAPID_EXT_SECURITY,      CAP_REQ_MANDATORY},
  {CXL_CAPID_RAS,               CAP_REQ_NOT_PERMITTED},
  {CXL_CAPID_SECURITY,          CAP_REQ_NOT_PERMITTED},
  {CXL_CAPID_LINK,              CAP_REQ_NOT_PERMITTED},
  {CXL_CAPID_SNOOP_FILTER,      CAP_REQ_NOT_PERMITTED},
  {CXL_CAPID_TIMEOUT_ISOLATION, CAP_REQ_NOT_PERMITTED},
  {CXL_CAPID_BI_DECODER,        CAP_REQ_NOT_PERMITTED},
  {CXL_CAPID_CACHE_ID_DECODER,  CAP_REQ_NOT_PERMITTED}
};

static void
log_capability_issue(uint32_t index, uint32_t cap_id, uint8_t requirement)
{
  const char *cap_name = val_cxl_cap_name((uint16_t)cap_id);

  if (cap_name == NULL)
    return;

  if (requirement == CAP_REQ_MANDATORY) {
    val_print(ACS_PRINT_ERR, "\n       %a is not supported", (uint64_t)cap_name);
  }
  else if (requirement == CAP_REQ_NOT_PERMITTED) {
    val_print(ACS_PRINT_ERR, "\n       %a is supported", (uint64_t)cap_name);
  }
  val_print(ACS_PRINT_ERR, " for host index 0x%x", index);
}

static void
payload(void)
{
  uint32_t pe_index;
  uint32_t num_cxl_hb;
  uint32_t index;
  uint64_t cxl_hb_base, cxl_hb_len;
  uint32_t status;
  uint32_t present;
  uint32_t cap_id_size;
  uint32_t cap_check;
  uint32_t test_fail = 0;

  pe_index = val_pe_get_index_mpid(val_pe_get_mpid());
  num_cxl_hb = val_cxl_get_info(CXL_INFO_NUM_DEVICES, 0);

  if (num_cxl_hb == 0) {
      val_print(ACS_PRINT_INFO, "\n       No CXL Host Bridges discovered via CEDT", 0);
      val_set_status(pe_index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  for (index = 0; index < num_cxl_hb; index++) {
      cxl_hb_base = val_cxl_get_info(CXL_INFO_COMPONENT_BASE, index);
      cxl_hb_len  = val_cxl_get_info(CXL_INFO_COMPONENT_LENGTH, index);

      val_print(ACS_PRINT_INFO, "\n       HB[%x] CHBCR cxl_hb_base", index);
      val_print(ACS_PRINT_INFO, " = 0x%llx", cxl_hb_base);
      val_print(ACS_PRINT_INFO, " cxl_hb_len = 0x%llx", cxl_hb_len);

      cap_id_size = (uint32_t)(sizeof(g_capability_checks) / sizeof(g_capability_checks[0]));
      for (cap_check = 0; cap_check < cap_id_size; cap_check++) {
          const capability_check_t *entry = &g_capability_checks[cap_check];
          status = val_cxl_find_comp_capability(index, entry->cap_id);
          present = (status == 0) ? 1 : 0;

          if ((entry->requirement == CAP_REQ_MANDATORY && present == 0) ||
              (entry->requirement == CAP_REQ_NOT_PERMITTED && present == 1)) {
              log_capability_issue(index, entry->cap_id, entry->requirement);
              test_fail++;
          }

      }
  }

  if (test_fail) {
    val_set_status(pe_index, RESULT_FAIL(TEST_NUM, 1));
  } else {
    val_set_status(pe_index, RESULT_PASS(TEST_NUM, 1));
  }

}

uint32_t
cxl004_entry(uint32_t num_pe)
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
