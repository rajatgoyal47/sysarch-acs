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
#include "acs_gic.h"
#include "gic.h"

#ifdef PC_BSA
#define TEST_RULE  "TTLFJ"
#else
#define TEST_RULE  "S_L8GI_01"
#endif
#define TEST_NUM   (ACS_GIC_TEST_NUM_BASE + 15)
#define TEST_DESC  "Check GICv4.1 or higher compliant     "

static bool check_all_controllers;

static
void
payload(void)
{

  uint32_t gic_version;
  uint32_t num_gicr_rd = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t gicrd_base, gicrd_rvpeid;
  uint32_t gicrd_length, i;

  gic_version = val_gic_get_info(GIC_INFO_VERSION);
  val_print(TRACE, "\n       Received GIC Major version = %4d      ", gic_version);

  /* Check the Major Version of GIC */
  if (gic_version < 4) {
      val_print(ERROR, "\n       Expected GICv4 or higher, received GICv%d", gic_version);
      val_set_status(index, RESULT_FAIL(01));
      return;
  }

  num_gicr_rd = val_gic_get_info(GIC_INFO_NUM_GICR_GICRD);
  val_print(TRACE, "\n       Redistributor count: %d", num_gicr_rd);
  if (num_gicr_rd == 0) {
      val_print(ERROR, "\n       No GIC Redistributors are presented");
      val_set_status(index, RESULT_FAIL(02));
      return;
  }


  for (i = 0; i < num_gicr_rd; i++)
  {
      gicrd_base = val_get_gicr_base(&gicrd_length, i);
      if (gicrd_base == 0) {
          val_print(ERROR, "\n       Invalid GICR base for controller %d", i);
          if (check_all_controllers) {
              val_set_status(index, RESULT_FAIL(03));
              return;
          }
          continue;
      }

      /* GICR_TYPER.RVPEID[7] == 0x1 indicates gic is v4.1 compliant */
      gicrd_rvpeid = (val_mmio_read64(gicrd_base + GICR_TYPER) >> 7) & 0x1;

      if (gicrd_rvpeid == 0) {
          val_print(ERROR,
                    "\n       Interrupt controller %d is not GICv4.1 or higher", i);
          if (check_all_controllers) {
              val_set_status(index, RESULT_FAIL(04));
              return;
          }
          continue;
      }

      val_print(TRACE, "\n       Interrupt controller %d is GICv4.1 or higher", i);

      if (!check_all_controllers) {
          val_set_status(index, RESULT_PASS);
          return;
      }
  }

  if (check_all_controllers)
      val_set_status(index, RESULT_PASS);
  else
      val_set_status(index, RESULT_FAIL(04));
  return;
}

uint32_t
g015_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;
  check_all_controllers = false;

  num_pe = 1;  //This GIC test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}

uint32_t
g020_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;
  check_all_controllers = true;

  num_pe = 1;  //This GIC test is run on single processor

  val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

  return status;
}
