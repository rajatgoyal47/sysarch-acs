/** @file
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "pal_pcie_enum.h"
#include "pal_common_support.h"
#include "platform_override_struct.h"

extern PLATFORM_OVERRIDE_UART_INFO_TABLE platform_uart_cfg;
extern PLATFORM_OVERRIDE_MEMORY_INFO_TABLE  platform_mem_cfg;
extern PCIE_INFO_TABLE platform_pcie_cfg;

#define USB_CLASSCODE   0x0C0300
#define SATA_CLASSCODE  0x010600
#define BAR0            0
#define BAR1            1
#define BAR2            2
#define BAR3            3
#define BAR4            4
#define BAR5            5

uint32_t spcr_baudrate_id[] = {0, 0, 0, 9600, 19200, 0, 57600, 115200};

/**
  @brief  This API fills in the PERIPHERAL_INFO_TABLE with information about peripherals
          in the system.

  @param  peripheralInfoTable  - Address where the Peripheral information needs to be filled.

  @return  None
**/
void
pal_peripheral_create_info_table(PERIPHERAL_INFO_TABLE *peripheralInfoTable)
{

  uint32_t   DeviceBdf = 0;
  uint32_t   StartBdf  = 0;
  uint32_t   bar_index = 0;
  uint32_t   i = 0;
  uint64_t   uart;
  PERIPHERAL_INFO_BLOCK *per_info = NULL;

  if (peripheralInfoTable == NULL) {
    print(ACS_PRINT_ERR, "Input Peripheral Table Pointer is NULL. Cannot create Peripheral INFO\n");
    return;
  }

  per_info = peripheralInfoTable->info;

  peripheralInfoTable->header.num_usb = 0;
  peripheralInfoTable->header.num_sata = 0;
  peripheralInfoTable->header.num_uart = 0;
  peripheralInfoTable->header.num_all = 0;
  per_info->base0 = 0;

  // If there is no ECAM region found in the system, skip USB and SATA controllers.
  if (platform_pcie_cfg.num_entries == 0)
    goto UART_CONFIG;

  /* check for any USB Controllers */
  do {

       print(ACS_PRINT_INFO, "Entered USB loop\n");
       DeviceBdf = pal_pcie_get_bdf(USB_CLASSCODE, StartBdf);
       if (DeviceBdf != 0) {
          per_info->type  = PERIPHERAL_TYPE_USB;
          for (bar_index = 0; bar_index < TYPE0_MAX_BARS; bar_index++)
          {
              per_info->base0 = pal_pcie_get_base(DeviceBdf, bar_index);
              if (per_info->base0 != 0)
                  break;
          }
          per_info->bdf   = DeviceBdf;
          per_info->platform_type = 0;

          print(ACS_PRINT_INFO, "Found a USB controller %4x\n", per_info->base0);
          peripheralInfoTable->header.num_usb++;
          peripheralInfoTable->header.num_all++;
          per_info++;
          i++;
       }
       StartBdf = pal_increment_bus_dev(DeviceBdf);

  } while (DeviceBdf != 0);

  StartBdf = 0;
  /* check for any SATA Controllers */
  do {

       print(ACS_PRINT_INFO, "Entered SATA loop\n");
       DeviceBdf = pal_pcie_get_bdf(SATA_CLASSCODE, StartBdf);
       if (DeviceBdf != 0) {
          per_info->type  = PERIPHERAL_TYPE_SATA;
          for (bar_index = 0; bar_index < TYPE0_MAX_BARS; bar_index++)
          {
              per_info->base0 = pal_pcie_get_base(DeviceBdf, bar_index);
              if (per_info->base0 != 0)
                  break;
          }
          per_info->platform_type = 0;
          per_info->bdf   = DeviceBdf;

          print(ACS_PRINT_INFO, "Found a SATA controller %4x\n", per_info->base0);
          peripheralInfoTable->header.num_sata++;
          peripheralInfoTable->header.num_all++;
          per_info++;
          i++;
       }
       //Increment and check if we have more controllers
       StartBdf = pal_increment_bus_dev(DeviceBdf);

  } while (DeviceBdf != 0);

UART_CONFIG:
  /* UART details
   * Assumption that UART bdf details is
   * known to platform owner */
  uart = platform_uart_cfg.Address;

  if (uart) {
    per_info->base0 = platform_uart_cfg.BaseAddress.Address;
    per_info->width = 1;
    per_info->irq   = platform_uart_cfg.GlobalSystemInterrupt;
    per_info->type  = PERIPHERAL_TYPE_UART;
    per_info->bdf   = 0;

    if(platform_uart_cfg.PciVendorId != 0xFFFF)
    {
       DeviceBdf = PCIE_CREATE_BDF(platform_uart_cfg.PciSegment, platform_uart_cfg.PciBusNumber,
                                         platform_uart_cfg.PciDeviceNumber, platform_uart_cfg.PciFunctionNumber);
       per_info->bdf = DeviceBdf;
    }

    if (platform_uart_cfg.BaudRate < 8)
        per_info->baud_rate = spcr_baudrate_id[platform_uart_cfg.BaudRate];
    else
         per_info->baud_rate = 0;

    per_info->interface_type = platform_uart_cfg.InterfaceType;
    peripheralInfoTable->header.num_uart++;
    peripheralInfoTable->header.num_all++;
    per_info++;
  }

  per_info->type = 0xFF; //indicate end of table
}

/**
    @brief   Check if PCI device is PCI Express capable

    @param   seg        PCI segment number
    @param   bus        PCI bus address
    @param   dev        PCI device address
    @param   fn         PCI function number

    @return  staus code:
             1: PCIe capable,  0: No PCIe capable
**/
uint32_t pal_peripheral_is_pcie(uint32_t seg, uint32_t bus, uint32_t dev, uint32_t fn)
{

  uint32_t reg_value;
  uint32_t next_cap_offset;
  pal_pcie_read_cfg(seg, bus, dev, fn, TYPE01_CPR, &reg_value);
  next_cap_offset = (reg_value & TYPE01_CPR_MASK);
  while (next_cap_offset)
  {
     pal_pcie_read_cfg(seg, bus, dev, fn, next_cap_offset, &reg_value);
     if ((reg_value & PCIE_CIDR_MASK) == CID_PCIECS)
     {
         print(ACS_PRINT_INFO, "PCIe Capable", 0);
         return 1;
     }
     next_cap_offset = ((reg_value >> PCIE_NCPR_SHIFT) & PCIE_NCPR_MASK);
  }

  return 0;
}

/**
  @brief  This API fills in the MEMORY_INFO_TABLE with information about memory in the
          system.

  @param  peripheralInfoTable  - Address where the Peripheral information needs to be filled.

  @return  None
**/
void
pal_memory_create_info_table(MEMORY_INFO_TABLE *memoryInfoTable)
{
    uint32_t index = 0;

    if (memoryInfoTable == NULL) {
        print(ACS_PRINT_ERR, "\nInput Memory Table Pointer is NULL", 0);
        return;
    }

    for (index = 0; index < platform_mem_cfg.count; index++)
    {
        memoryInfoTable->info[index].phy_addr = platform_mem_cfg.info[index].phy_addr;
        memoryInfoTable->info[index].virt_addr = platform_mem_cfg.info[index].virt_addr;
        memoryInfoTable->info[index].size = platform_mem_cfg.info[index].size;
        memoryInfoTable->info[index].type = platform_mem_cfg.info[index].type;
    }

    memoryInfoTable->info[index].type      = MEMORY_TYPE_LAST_ENTRY;

}

/**
  @brief  Return the address of unpopulated memory of requested
          instance from the GCD memory map.

  @param  addr      - Address of the unpopulated memory
          instance  - Instance of memory

  @return 0 - SUCCESS
          1 - No unpopulated memory present
          2 - FAILURE
**/
uint64_t
pal_memory_get_unpopulated_addr(uint64_t *addr, uint32_t instance)
{
  uint32_t index = 0;
  uint32_t memory_instance = 0;

  for (index = 0; index < platform_mem_cfg.count; index++)
  {
      if (platform_mem_cfg.info[index].type == MEMORY_TYPE_NOT_POPULATED)
      {
          if (memory_instance == instance)
          {
              *addr =  platform_mem_cfg.info[index].virt_addr;
              print(ACS_PRINT_INFO, "Unpopulated region with base address 0x%lX found\n", *addr);
              return MEM_MAP_SUCCESS;
          }

          memory_instance++;
      }
  }

  return MEM_MAP_NO_MEM;
}
