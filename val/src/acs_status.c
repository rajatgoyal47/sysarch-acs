/** @file
 * Copyright (c) 2016-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_common.h"

extern uint32_t g_override_skip;

/**
  @brief  Parse the input status and print the appropriate information to console
          1. Caller       - Application layer
          2. Prerequisite - None
  @param  index   - index of the PE who is reporting this status.
  @param  status  - 32-bit value concatenated from state, level, error value
  @param  *ruleid - Rule ID in the test to print

  @return  none
 **/
void
val_report_status(uint32_t index, uint32_t status, char8_t *ruleid)
{
/* Rule based execution orchestrator reports status, hence muting if COMPILE_RB_EXE defined */
#ifdef COMPILE_RB_EXE
  return;
#endif

  /* Test stays quiet if it is overridden by any of the user options */
  if (!g_override_skip)
    return;

  if (IS_TEST_FAIL(status)) {
      val_print(ERROR, "\nFailed on PE - %4d\n", index);
  }

  if (IS_TEST_SKIP(status)) {
      val_print(ERROR, "\nSkipped on PE - %4d\n", index);
  }

  if (IS_TEST_PASS(status)) {
      val_print(DEBUG, "\n       ");
      val_print(DEBUG, ruleid);
      val_print(DEBUG, "\n                                  ");
      val_print(INFO, "     : Result:  PASS\n", status);
  }
  else if (IS_TEST_WARNING(status)) {
      val_print(INFO, "     : Result:  WARN\n", 0);
  }
  else
    if (IS_TEST_FAIL(status)) {
        if (ruleid) {
            val_print(INFO, "\n       ");
            val_print(INFO, ruleid);
            val_print(ERROR, "\n       Checkpoint -- %2d    ",
                                                         status & STATUS_MASK);
        }
        val_print(ERROR, "     : Result:  FAIL\n");
    }
    else
      if (IS_TEST_SKIP(status)) {
          if (ruleid) {
              val_print(INFO, "\n       ");
              val_print(INFO, ruleid);
              val_print(WARN, "\n       Checkpoint -- %2d    ",
                                                         status & STATUS_MASK);
          }
          val_print(WARN, "     : Result:  SKIPPED\n");
      }
      else
        if (IS_TEST_START(status))
          val_print(INFO, "\n       START", status);
        else
          if (IS_TEST_END(status))
            val_print(INFO, "       END\n\n", status);
          else
            val_print(INFO, ": Result:  %8x\n", status);

}

