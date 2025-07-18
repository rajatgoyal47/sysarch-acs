/** @file
 * Copyright (c) 2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/val_interface.h"
#include "val/include/acs_gic.h"

#define TEST_NUM   (ACS_GIC_V2M_TEST_NUM_BASE + 3)
#define TEST_RULE  "Appendix I.6"
#define TEST_DESC  "Check GICv2m MSI to SPI Generation    "

static uint32_t int_id;

static
void
isr()
{
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  val_print(ACS_PRINT_INFO, "\n Received SPI ", 0);
  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
  val_gic_end_of_interrupt(int_id);

  return;
}

static
void
payload()
{

  uint32_t num_spi;
  uint32_t instance;
  uint32_t timeout = TIMEOUT_MEDIUM;
  uint32_t msi_frame, min_spi_id;
  uint64_t frame_base;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

  msi_frame = val_gic_get_info(GIC_INFO_NUM_MSI_FRAME);
  if (msi_frame == 0) {
      val_print(ACS_PRINT_DEBUG, "\n       No MSI frame, Skipping               ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  for (instance = 0; instance < msi_frame; instance++) {
    frame_base = val_gic_v2m_get_info(V2M_MSI_FRAME_BASE, instance);

    min_spi_id = val_gic_v2m_get_info(V2M_MSI_SPI_BASE, instance);
    num_spi  = val_gic_v2m_get_info(V2M_MSI_SPI_NUM, instance);

    int_id = min_spi_id + num_spi - 1;

    /* Register an interrupt handler to verify */
    if (val_gic_install_isr(int_id, isr)) {
      val_print(ACS_PRINT_ERR, "\n       GIC Install Handler Failed...", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
      return;
    }

    /* Part 1 : SPI Generation using 32 Bit Write */
    /* Generate the Interrupt by writing the int_id to SETSPI_NS Register */
    val_mmio_write(frame_base + GICv2m_MSI_SETSPI, int_id);

    while ((timeout > 0) && (IS_RESULT_PENDING(val_get_status(index)))) {
      timeout--;
    }

    if (timeout == 0) {
      val_print(ACS_PRINT_ERR, "\n       Interrupt not received within timeout", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
      return;
    }

    /* Part 2 : SETSPI must Support 16 Bit Access. */
    /* Generate the Interrupt by writing the int_id to SETSPI_NS Register */
    val_mmio_write16(frame_base + GICv2m_MSI_SETSPI, int_id);

    while ((timeout > 0) && (IS_RESULT_PENDING(val_get_status(index)))) {
      timeout--;
    }

    if (timeout == 0) {
      val_print(ACS_PRINT_ERR, "\n       Interrupt not received within timeout", 0);
      val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
      return;
    }
  }
}

uint32_t
v2m003_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This GIC test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);

  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
