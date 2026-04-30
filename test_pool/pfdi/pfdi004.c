/** @file
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "val_interface.h"
#include "acs_memory.h"

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 4)
#define TEST_RULE  "R0060"
#define TEST_DESC  "Check PFDI Feature function support       "

PFDI_RET_PARAMS *g_pfdi_feature_check_details;

void
check_feature()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  int64_t  status;
  PFDI_RET_PARAMS *status_buffer;

  status_buffer = g_pfdi_feature_check_details + index;

  /* Invoke PFDI Feature function for current PE index */
  status = val_pfdi_features(PFDI_FN_PFDI_FEATURES, &status_buffer->x1, &status_buffer->x2,
                                               &status_buffer->x3, &status_buffer->x4);

  /*Save the return status*/
  status_buffer->x0 = status;
  val_pfdi_invalidate_ret_params(status_buffer);

  val_set_status(index, RESULT_PASS);
  return;
}

static void payload_feature_check(void *arg)
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t timeout, i = 0, run_fail = 0;
  PFDI_RET_PARAMS *status_buffer;
  uint32_t num_pe = *(uint32_t *)arg;

  /* Allocate memory to save all PFDI function status for all PE's */
  g_pfdi_feature_check_details = (PFDI_RET_PARAMS *)
                    val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
  if (g_pfdi_feature_check_details == NULL) {
    val_print(ERROR,
                "\n       Allocation for PFDI Feature Check Function Failed");
    val_set_status(index, RESULT_FAIL(1));
    return;
  }

  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_feature_check_details + i;
    val_pfdi_invalidate_ret_params(status_buffer);
  }

  /* Invoke PFDI Feature function for current PE index */
  check_feature();

  /* Execute check_feature function in All PE's */
  for (i = 0; i < num_pe; i++) {
    if (i != index) {
      timeout = TIMEOUT_LARGE;
      val_execute_on_pe(i, check_feature, 0);

      while ((--timeout) && (IS_RESULT_PENDING(val_get_status(i))));

      if (timeout == 0) {
        val_print(ERROR, "\n       **Timed out** for PE index = %d", i);
        val_set_status(i, RESULT_FAIL(2));
        goto free_pfdi_details;
      }
    }
  }
  val_time_delay_ms(ONE_MILLISECOND);

  /* Check return status of function for all PE's */
  for (i = 0; i < num_pe; i++) {
    status_buffer = g_pfdi_feature_check_details + i;
    val_pfdi_invalidate_ret_params(status_buffer);
    run_fail = 0;

    if (status_buffer->x0 < PFDI_ACS_SUCCESS) {
      val_print(ERROR, "\n       PFDI Feature Check function failed err = %ld",
                                                        status_buffer->x0);
      val_print(ERROR, " on PE index = %d", i);
      run_fail++;
    }

    if ((status_buffer->x1 != 0) || (status_buffer->x2 != 0) ||
        (status_buffer->x3 != 0) || (status_buffer->x4 != 0)) {
      val_print(ERROR, "\n       Registers X1-X4 are not zero:");
      val_print(ERROR, " x1=0x%llx", status_buffer->x1);
      val_print(ERROR, " x2=0x%llx", status_buffer->x2);
      val_print(ERROR, " x3=0x%llx", status_buffer->x3);
      val_print(ERROR, " x4=0x%llx", status_buffer->x4);
      val_print(ERROR, "\n       Failed on PE = %d", i);
      run_fail++;
    }

    if (run_fail)
      val_set_status(i, RESULT_FAIL(3));
    else
      val_set_status(i, RESULT_PASS);
  }

free_pfdi_details:
  val_memory_free((void *) g_pfdi_feature_check_details);

  return;
}

uint32_t pfdi004_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
    val_run_test_configurable_payload(&num_pe, payload_feature_check);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
