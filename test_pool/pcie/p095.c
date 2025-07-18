/** @file
 * Copyright (c) 2016-2018,2021,2024-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "val/include/acs_smmu.h"
#include "val/include/acs_dma.h"
#include "val/include/acs_pcie.h"
#include "val/include/val_interface.h"

#define TEST_NUM   (ACS_PCIE_TEST_NUM_BASE + 95)
#define TEST_RULE  "PCI_MM_05, PCI_MM_06, PCI_MM_07"
#define TEST_DESC  "No extra address translation          "


/* For all DMA masters populated in the Info table, which are behind an SMMU,
   verify there are no additional translations before address is given to SMMU */
static
void
payload(void)
{

  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint32_t target_dev_index;
  addr_t   dma_addr = 0;
  uint32_t dma_len = 0;
  void *buffer;
  uint32_t status;
  uint32_t iommu_flag = 0;

  target_dev_index = val_dma_get_info(DMA_NUM_CTRL, 0);

  if (!target_dev_index) {
      val_print(ACS_PRINT_DEBUG, "\n       No DMA controllers detected...    ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
      return;
  }

  while (target_dev_index)
  {
      target_dev_index--; //index is zero based

      /* Check there were no additional translations between Device and SMMU */
      if (val_dma_get_info(DMA_HOST_IOMMU_ATTACHED, target_dev_index)) {
          iommu_flag++;
          val_dma_device_get_dma_addr(target_dev_index, &dma_addr, &dma_len);
          status = val_smmu_ops(SMMU_CHECK_DEVICE_IOVA, &target_dev_index, &dma_addr);
          if (status) {
              val_print(ACS_PRINT_ERR, "\n       The DMA address %lx used by device ", dma_addr);
              val_print(ACS_PRINT_ERR, "\n       is not present in the SMMU IOVA table\n", 0);
              val_set_status(index, RESULT_FAIL(TEST_NUM, target_dev_index));
              return;
          }
      }
  }

  target_dev_index = val_dma_get_info(DMA_NUM_CTRL, 0);

  /* Check if IOMMU ops is properly integrated for this device by making the standard OS
     DMA API call and verifying the DMA address is part of the IOVA translation table */
  while (target_dev_index)
  {
      target_dev_index--;
      if (val_dma_get_info(DMA_HOST_IOMMU_ATTACHED, target_dev_index)) {
          /* Allocate DMA-able memory region in DDR */
          dma_addr = val_dma_mem_alloc(&buffer, 512, target_dev_index, DMA_COHERENT);
          status = val_smmu_ops(SMMU_CHECK_DEVICE_IOVA, &target_dev_index, &dma_addr);
          if (status) {
              val_print(ACS_PRINT_ERR, "\n       The DMA addr allocated to device %d ",
                        target_dev_index);
              val_print(ACS_PRINT_ERR, "\n       is not present in the SMMU IOVA table\n", 0);
              val_set_status(index, RESULT_FAIL(TEST_NUM, target_dev_index));
              return;
          }
          /* Free the allocated memory here */
          val_dma_mem_free(buffer, dma_addr, 512, target_dev_index, DMA_COHERENT);
      }
  }

  if (iommu_flag)
      val_set_status(index, RESULT_PASS(TEST_NUM, 2));
  else
      val_set_status(index, RESULT_SKIP(TEST_NUM, 2));

}


uint32_t
p095_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, ACS_END(TEST_NUM), NULL);

  return status;
}
