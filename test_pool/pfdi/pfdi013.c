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

#define TEST_NUM   (ACS_PFDI_TEST_NUM_BASE + 13)
#define TEST_RULE  "R0164"
#define TEST_DESC  "Check PE Run with Start exceeds End       "

static PFDI_RET_PARAMS *g_pfdi_status;

/* Execute the invalid parameter scenario: start index > end index */
static void
check_pe_test_run_start_exceeds_end(void)
{
    uint32_t index;
    PFDI_RET_PARAMS *pfdi_buffer;

    index = val_pe_get_index_mpid(val_pe_get_mpid());
    pfdi_buffer = g_pfdi_status + index;

    /* Execute invalid start > end case */
    pfdi_buffer->x0 = val_pfdi_pe_test_run(1, 0,
                             &pfdi_buffer->x1,
                             &pfdi_buffer->x2,
                             &pfdi_buffer->x3,
                             &pfdi_buffer->x4);

    val_pfdi_invalidate_ret_params(pfdi_buffer);

    val_set_status(index, RESULT_PASS);
}

/* Validate that PFDI_PE_TEST_RUN returns INVALID_PARAMETERS for start > end */
static void
payload_check_pe_test_run_start_exceeds_end(void *arg)
{
    uint32_t  num_pe = *((uint32_t *)arg);
    uint32_t  index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint32_t  i = 0, timeout, test_fail = 0;
    PFDI_RET_PARAMS *pfdi_buffer;

    g_pfdi_status = (PFDI_RET_PARAMS *)
        val_memory_calloc(num_pe, sizeof(PFDI_RET_PARAMS));
    if (g_pfdi_status == NULL) {
        val_print(ERROR, "\n       Allocation for PFDI Run Function Failed");
        val_set_status(index, RESULT_FAIL(1));
        return;
    }

    check_pe_test_run_start_exceeds_end();

    for (i = 0; i < num_pe; i++) {
        if (i != index) {
            timeout = TIMEOUT_LARGE;
            val_execute_on_pe(i, check_pe_test_run_start_exceeds_end, 0);

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
        pfdi_buffer = g_pfdi_status + i;

        val_pfdi_invalidate_ret_params(pfdi_buffer);

        test_fail = 0;

        if (pfdi_buffer->x0 != PFDI_ACS_INVALID_PARAMETERS) {
            val_print(ERROR, "\n       Invalid parameter test failed on PE %d", i);
            val_print(ERROR, " (expected -3, got %ld)", pfdi_buffer->x0);
            test_fail++;
        }

        if ((pfdi_buffer->x1 != 0) || (pfdi_buffer->x2 != 0) ||
            (pfdi_buffer->x3 != 0) || (pfdi_buffer->x4 != 0)) {
            val_print(ERROR, "\n       Registers X1-X4 are not zero:");
            val_print(ERROR, " x1=0x%llx", pfdi_buffer->x1);
            val_print(ERROR, " x2=0x%llx", pfdi_buffer->x2);
            val_print(ERROR, " x3=0x%llx", pfdi_buffer->x3);
            val_print(ERROR, " x4=0x%llx", pfdi_buffer->x4);
            val_print(ERROR, "\n       Failed on PE = %d", i);
            test_fail++;
        }

        if (test_fail)
            val_set_status(i, RESULT_FAIL(3));
        else
            val_set_status(i, RESULT_PASS);
    }

free_pfdi_details:
    val_memory_free((void *)g_pfdi_status);
}

/* Entry point for test PFDI013 */
uint32_t pfdi013_entry(uint32_t num_pe)
{
    uint32_t status;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    if (status != ACS_STATUS_SKIP)
        val_run_test_configurable_payload(&num_pe, payload_check_pe_test_run_start_exceeds_end);

    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), NULL);
    return status;
}
