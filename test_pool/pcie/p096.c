/** @file
 * Copyright (c) 2016-2018, 2021-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_memory.h"
#include "val/include/acs_pcie.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 96)
#define TEST_RULE  "PCI_LI_02"
#define TEST_DESC  "PCI legacy intr SPI ID unique         "

static inline char pin_name(int pin)
{
    return 'A' + pin;
}

static
void
payload (void)
{

  uint32_t index;
  uint32_t count;
  PERIPHERAL_IRQ_MAP *irq_map;
  uint32_t status;
  uint32_t current_irq_pin;
  uint32_t next_irq_pin;
  uint64_t dev_bdf;
  uint32_t ccnt;
  uint32_t ncnt;
  uint32_t test_skip;

  current_irq_pin = 0;
  status = 0;
  test_skip = 1;
  next_irq_pin = current_irq_pin + 1;
  index = val_pe_get_index_mpid (val_pe_get_mpid());
  count = val_peripheral_get_info (NUM_ALL, 0);

  if (!count) {
     val_set_status(index, RESULT_SKIP (TEST_NUM, 1));
     return;
  }

  irq_map = val_aligned_alloc(MEM_ALIGN_4K, sizeof(PERIPHERAL_IRQ_MAP));
  if (!irq_map) {
    val_print(ACS_PRINT_ERR, "\n       Memory allocation error", 0);
    val_set_status(index, RESULT_FAIL (TEST_NUM, 1));
    return;
  }

  /* get legacy IRQ info from PCI devices */
  while (count > 0 && status == 0) {
    count--;
    if (val_peripheral_get_info (ANY_GSIV, count)) {
      dev_bdf = val_peripheral_get_info (ANY_BDF, count);
      status = val_pci_get_legacy_irq_map (dev_bdf, irq_map);

      switch (status) {
      case 0:
        break;
      case 1:
        val_print(ACS_PRINT_WARN, "\n       Unable to access PCI bridge device", 0);
        break;
      case 2:
        val_print(ACS_PRINT_WARN, "\n       Unable to fetch _PRT ACPI handle", 0);
        /* Not a fatal error, just skip this device */
        status = 0;
        continue;
      case 3:
        val_print(ACS_PRINT_WARN, "\n       Unable to access _PRT ACPI object", 0);
        /* Not a fatal error, just skip this device */
        status = 0;
        continue;
      case 4:
        val_print(ACS_PRINT_WARN, "\n       Interrupt hard-wire error", 0);
        /* Not a fatal error, just skip this device */
        status = 0;
        continue;
      case 5:
        val_print(ACS_PRINT_ERR, "\n       Legacy interrupt out of range", 0);
        break;
      case 6:
        val_print(ACS_PRINT_ERR, "\n       Maximum number of interrupts has been reached", 0);
        break;
      default:
        if (status == NOT_IMPLEMENTED)
            val_print (ACS_PRINT_ERR, "\n       API not implemented", 0);
        else
            val_print (ACS_PRINT_ERR, "\n       Unknown error", 0);
        break;
      }

      /* Compare IRQ routings */
      if (!status) {
        while (current_irq_pin < LEGACY_PCI_IRQ_CNT && status == 0) {
          while (next_irq_pin < LEGACY_PCI_IRQ_CNT && status == 0) {

            for (ccnt = 0; (ccnt < irq_map->legacy_irq_map[current_irq_pin].irq_count)
                 && (status == 0); ccnt++) {
              for (ncnt = 0; (ncnt < irq_map->legacy_irq_map[next_irq_pin].irq_count)
                 && (status == 0); ncnt++) {
                test_skip = 0;
                if (irq_map->legacy_irq_map[current_irq_pin].irq_list[ccnt] ==
                    irq_map->legacy_irq_map[next_irq_pin].irq_list[ncnt]) {
                  status = 7;
                  val_print(ACS_PRINT_ERR, "\n        BDF : %x", dev_bdf);
                  val_print(ACS_PRINT_ERR, " Legacy interrupt %c",
                                             pin_name(current_irq_pin));
                  val_print(ACS_PRINT_ERR, "(%d) routing", current_irq_pin);

                  val_print(ACS_PRINT_ERR, "\n        is same as the %c",
                                             pin_name(next_irq_pin));
                  val_print(ACS_PRINT_ERR, "(%d) routing", next_irq_pin);
                }
              }
            }

            next_irq_pin++;
          }
          current_irq_pin++;
          next_irq_pin = current_irq_pin + 1;
        }
      }
    }
  }

  val_memory_free_aligned(irq_map);

  if (test_skip) {
    val_set_status(index, RESULT_SKIP (TEST_NUM, 2));
    return;
  }

  if (!status) {
    val_set_status(index, RESULT_PASS (TEST_NUM, 1));
  } else {
    val_set_status(index, RESULT_FAIL (TEST_NUM, status));
  }
}

uint32_t
p096_entry(uint32_t num_pe)
{
  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test (TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP) {
      val_run_test_payload (TEST_NUM, num_pe, payload, 0);
  }

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
