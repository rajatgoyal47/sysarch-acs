/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_CXL_H__
#define __ACS_CXL_H__

#include "pal_interface.h"
/*
 * CXL + PCIe extended capability helpers
 */

/* PCIe ECAP/DVSEC */
#define CXL_DVSEC_VENDOR_ID             0x1E98  /* CXL Consortium Vendor ID */
#define CXL_68BVH_SHIFT                 21

#define CXL_COMPONENT_TABLE_MAX_ENTRIES 1024

/* ---- CXL DVSEC IDs you'll encounter on endpoints/ports ----
   NOTE: Leave as constants here; exact list evolves per CXL 3.x.
   You only need the Register Locator to find MMIO blocks.
*/
#define CXL_DVSEC_ID_DEVICE               0x0000
#define CXL_DVSEC_ID_NONCXL_FUNCTION_MAP  0x0002
#define CXL_DVSEC_ID_PORT_EXTENSIONS      0x0003
#define CXL_DVSEC_ID_GPF_PORT             0x0004
#define CXL_DVSEC_ID_GPF_DEVICE           0x0005
#define CXL_DVSEC_ID_PCIE_FLEXBUS_PORT    0x0007
#define CXL_DVSEC_ID_REGISTER_LOCATOR     0x0008

/* ---- Register Locator entry decode ---- */
/* ----   DVSEC len = 0x0C + n*8   ------- */
#define CXL_RL_ENTRY_SIZE               0x08    /* bytes per entry per spec */
#define CXL_RL_REG_BLK_ENTRIES          0x0C    /* Entries start after 12B header */

/* Per-entry fields (offsets from start of an entry)
   DW0 layout (little endian):
     [2:0]   BAR Indicator (BAR number in [2:0]; upper bits reserved)
     [15:8]  Block Identifier (8-bit)
     [31:16] Register Block offset Low
   DW1: Register Block Offset High
*/
#define CXL_RL_REG_OFF_LOW            0x00
#define CXL_RL_REG_OFF_HIGH           0x04    /* 32b offset */

/* Block IDs commonly seen (8-bit identifiers per RL DW0[15:8]) */
#define CXL_REG_BLOCK_COMPONENT         0x01    /* CXL Component Registers */
#define CXL_REG_BLOCK_DEVICE            0x03    /* CXL Device Registers */

/* BAR field helpers */
#define CXL_RL_BAR_NUM(b)               ((b) & 0x7)

/* CXL.cachemem Primary range offset (per CXL 3.1 Table 8-21): BAR + 4KB */
#define CXL_CACHEMEM_PRIMARY_OFFSET     0x1000

/* Size of the CXL.cachemem Primary window used for Component registers */
#define CXL_CACHEMEM_PRIMARY_SIZE       0x1000

/* ---- CXL Capability header (array form per 8.2.4/8.2.4.1) ----
   [15:0]  CapID
   [19:16] CapVersion (4 bits)
   [23:20] CXL.cachemem Version (only in the first header element at +0x00)
           Offset of next capability from the second header element
   [31:24] Size/Next (context-dependent)
*/
#define CXL_CAP_HDR_SIZE                0x04
#define CXL_CAP_HDR_CAPID(x)            ((uint16_t)((x) & 0xFFFF))
#define CXL_CAP_HDR_VER(x)              ((uint8_t)(((x) >> 16) & 0xF))
#define CXL_CAP_ARRAY_ENTRIES(x)        ((uint8_t)(((x) >> 24) & 0xFF))
#define CXL_CAP_HDR_PTR_SHIFT           20
#define CXL_CAP_HDR_PTR_MASK            0xFFFu
#define CXL_CAP_HDR_POINTER(x)          (((x) >> CXL_CAP_HDR_PTR_SHIFT) & CXL_CAP_HDR_PTR_MASK)

/* Cachemem version field present only in header element at +0x00 */
#define CXL_CAP_HDR_CACHEMEM_VER(x)     ((uint8_t)(((x) >> 20) & 0xF))

/* Known CXL Component Cap IDs (subset; print unknowns too) */
#define CXL_CAPID_COMPONENT_CAP         0x0001
#define CXL_CAPID_RAS                   0x0002
#define CXL_CAPID_SECURITY              0x0003
#define CXL_CAPID_LINK                  0x0004
#define CXL_CAPID_HDM_DECODER           0x0005
#define CXL_CAPID_EXT_SECURITY          0x0006
#define CXL_CAPID_IDE                   0x0007
#define CXL_CAPID_SNOOP_FILTER          0x0008
#define CXL_CAPID_TIMEOUT_ISOLATION     0x0009
#define CXL_CAPID_BI_DECODER            0x000C
#define CXL_CAPID_CACHE_ID_DECODER      0x000E

/* CXL Device Register Cap IDs */
#define CXL_DEVCAPID_DEVICE_STATUS      0x0001
#define CXL_DEVCAPID_PRI_MAILBOX        0x0002
#define CXL_DEVCAPID_SEC_MAILBOX        0x0003
#define CXL_DEVCAPID_MEMORY_DEVICE_STS  0x4000

/* ---- CXL helper status codes (for VAL CXL helpers) ---- */
/* Status codes returned by BAR resolution helpers */
#define VAL_CXL_BAR_SUCCESS            0x00000000
#define VAL_CXL_BAR_ERR_INVALID_INDEX  0x00000001
#define VAL_CXL_BAR_ERR_CFG_READ       0x00000002
#define VAL_CXL_BAR_ERR_NOT_MMIO       0x00000003
#define VAL_CXL_BAR_ERR_ZERO           0x00000004

/* ---- CXL DVSEC header offsets and fields ---- */
#define CXL_DVSEC_HDR1_OFFSET          0x04
#define CXL_DVSEC_HDR2_OFFSET          0x08
#define CXL_DVSEC_HDR1_VENDOR_ID_MASK  0xFFFF
#define CXL_DVSEC_HDR1_REV_SHIFT       16
#define CXL_DVSEC_HDR1_REV_MASK        0xF
#define CXL_DVSEC_HDR1_LEN_SHIFT       20
#define CXL_DVSEC_HDR1_LEN_MASK        0xFFF
#define CXL_DVSEC_HDR2_ID_MASK         0xFFFF

#define CXL_DVSEC_CXL_CAPABILITY_OFFSET 0x0A
#define CXL_DVSEC_CXL_CAPABILITY_SHIFT  16
#define CXL_DVSEC_CXL_CAPABILITY_MASK   0xFFFFu
#define CXL_DVSEC_CXL_CAP_TSP_CAPABLE   (1u << 12)
#define CXL_DVSEC_CXL_CAP_CACHE_CAPABLE (1u << 0)
#define CXL_DVSEC_CXL_CAP_IO_CAPABLE    (1u << 1)
#define CXL_DVSEC_CXL_CAP_MEM_CAPABLE   (1u << 2)

/* ---- CXL Register Locator entry fields (DW0) ---- */
#define CXL_RL_ENTRY_BIR_SHIFT         0
#define CXL_RL_ENTRY_BIR_MASK          0xFF
#define CXL_RL_ENTRY_BLOCKID_SHIFT     8
#define CXL_RL_ENTRY_BLOCKID_MASK      0xFF

/* ---- CXL Device Register Block: Capabilities Array ---- */
#define CXL_DEV_CAP_ARR_HDR_OFFSET     0x0
#define CXL_DEV_CAP_ARR_COUNT_SHIFT    32
#define CXL_DEV_CAP_ARR_COUNT_MASK     0xFFFF
#define CXL_DEV_CAP_ARR_BASE_OFFSET    0x10
#define CXL_DEV_CAP_ELEM_SIZE          16
#define CXL_DEV_CAP_ARR_HDR_SIZE       8

/* Device Capabilities element (first 64-bit dword) fields */
#define CXL_DEV_CAP_ELEM_W0_ID_MASK       0xFFFF
#define CXL_DEV_CAP_ELEM_W0_VER_SHIFT     16
#define CXL_DEV_CAP_ELEM_W0_VER_MASK      0xFF
#define CXL_DEV_CAP_ELEM_W0_OFF_SHIFT     32
#define CXL_DEV_CAP_ELEM_W0_OFF_MASK      0xFFFFFFFFu
#define CXL_DEV_CAP_MAX_GUARD             64

/* ---- HDM Decoder capability offsets ---- */
#define CXL_HDM_CAP_REG_OFFSET         0x00
#define CXL_HDM_GLOBAL_CTRL_OFFSET     0x04
#define CXL_HDM_DECODER_STRIDE         0x20
#define CXL_HDM_ALIGNMENT_SHIFT        28
#define CXL_HDM_ALIGNMENT_MASK         ((1ULL << CXL_HDM_ALIGNMENT_SHIFT) - 1ULL)
#define CXL_HDM_DECODER_BASE_LOW(n)    (0x10 + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_BASE_HIGH(n)   (0x14 + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_SIZE_LOW(n)    (0x18 + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_SIZE_HIGH(n)   (0x1C + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_CTRL(n)        (0x20 + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_TARGET_LOW(n)  (0x24 + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_TARGET_HIGH(n) (0x28 + (n) * CXL_HDM_DECODER_STRIDE)
#define CXL_HDM_DECODER_COUNT_MASK     0xF
#define CXL_HDM_DECODER_COUNT_SHIFT    0

/* ---- CXL Component Register Primary Array ---- */
#define CXL_COMPONENT_CAP_ARRAY_OFFSET 0x0

/* Other Register Block IDs */
#define CXL_REG_BLOCK_VENDOR_SPECIFIC   0xFF
#define CXL_COMPONENT_RCRB     0x1
#define CXL_COMPONENT_DVSEC    0x2
#define CXL_COMPONENT_HDM      0x4
#define CXL_COMPONENT_MAILBOX  0x8
#define CXL_COMPONENT_TSP      0x10
#define CXL_COMPONENT_IDE      0x20
#define CXL_MAX_DECODER_SLOTS  2
#define CXL_COMPONENT_INVALID_INDEX  0xFFFFFFFFu

#define PERSISTENT_MASK       0x8
#define PERSISTENT_SHIFT      3

#define PGT_SHAREABLITY_SHIFT 6
#define PGT_ENTRY_ACCESS      (0x1 << 8)
#define PGT_ENTRY_AP_RW       (0x1ull << 4)
#define NON_SHAREABLE         0x0ULL
#define NON_CACHEABLE         0x44ULL
#define MAIR_REG_VAL_EL3      0x00000000004404ff

#define LOWER_ATTRS(x)         (((x) & (0xFFF)) << 2)
#define EXTRACT_ATTR_IND(x)    ((MAIR_REG_VAL_EL3 >> (x * 8)) & 0xFF)
#define AARCH64_TTBR_ADDR_MASK (((0x1ull << 47) - 1) << 1)

#define SHAREABLE_ATTR(x) (x << PGT_SHAREABLITY_SHIFT)
#define GET_ATTR_INDEX(x)           \
  ({                                \
    int index;                      \
    for (int i = 0; i < 8; ++i)     \
    {                               \
      if (EXTRACT_ATTR_IND(i) == x) \
      {                             \
        index = i;                  \
        break;                      \
      }                             \
    }                               \
    index;                          \
  })

typedef enum {
  CXL_COMPONENT_ROLE_UNKNOWN = 0,
  CXL_COMPONENT_ROLE_ROOT_PORT,
  CXL_COMPONENT_ROLE_SWITCH_UPSTREAM,
  CXL_COMPONENT_ROLE_SWITCH_DOWNSTREAM,
  CXL_COMPONENT_ROLE_ENDPOINT,
} CXL_COMPONENT_ROLE;

typedef enum {
  CXL_DEVICE_TYPE_UNKNOWN = 0,
  CXL_DEVICE_TYPE_TYPE1,
  CXL_DEVICE_TYPE_TYPE2,
  CXL_DEVICE_TYPE_TYPE3,
} CXL_DEVICE_TYPE;

typedef struct {
  uint32_t bdf;
  uint32_t host_bridge_index;
  uint32_t role;
  uint32_t device_type;
  uint64_t component_reg_base;
  uint64_t component_reg_length;
  uint64_t device_reg_base;
  uint32_t device_reg_length;
  uint32_t hdm_decoder_count;
} CXL_COMPONENT_ENTRY;

typedef struct {
  uint32_t             num_entries;
  CXL_COMPONENT_ENTRY  component[];
} CXL_COMPONENT_TABLE;

#define CXL_COMPONENT_TABLE_SZ \
  (sizeof(CXL_COMPONENT_TABLE) + \
   (CXL_COMPONENT_TABLE_MAX_ENTRIES * sizeof(CXL_COMPONENT_ENTRY)))

typedef enum {
  CXL_COMPONENT_INFO_COUNT = 1,
  CXL_COMPONENT_INFO_FLAGS,
  CXL_COMPONENT_INFO_ROLE,
  CXL_COMPONENT_INFO_DEVICE_TYPE,
  CXL_COMPONENT_INFO_HOST_UID,
  CXL_COMPONENT_INFO_HOST_BRIDGE_INDEX,
  CXL_COMPONENT_INFO_BDF_INDEX,
  CXL_COMPONENT_INFO_COMPONENT_BASE,
  CXL_COMPONENT_INFO_COMPONENT_LENGTH,
  CXL_COMPONENT_INFO_DVSEC_OFFSET,
  CXL_COMPONENT_INFO_HDM_COUNT
} CXL_COMPONENT_INFO_e;

typedef enum {
  CXL_INFO_NUM_DEVICES = 1,
  CXL_INFO_COMPONENT_BASE,
  CXL_INFO_COMPONENT_LENGTH,
  CXL_INFO_COMPONENT_TYPE,
  CXL_INFO_DEVICE_TYPE,
  CXL_INFO_VERSION,
  CXL_INFO_HDM_COUNT,
  CXL_INFO_CAPABILITY_FLAGS,
  CXL_INFO_UID
} CXL_INFO_e;

void     val_cxl_create_info_table(uint64_t *cxl_info_table);
void     val_cxl_free_info_table(void);
void     val_cxl_free_component_table(void);
uint32_t val_cxl_create_table(void);
uint64_t val_cxl_get_info(CXL_INFO_e type, uint32_t index);
uint64_t val_cxl_get_component_info(CXL_COMPONENT_INFO_e type, uint32_t index);
uint32_t val_cxl_get_cfmws_count(uint32_t host_index);
uint32_t val_cxl_get_cfmws_window(uint32_t host_index, uint64_t *base, uint64_t *length);
uint32_t val_cxl_find_capability(uint32_t bdf, uint32_t cid, uint32_t *cid_offset);
uint32_t val_cxl_find_comp_capability(uint32_t index, uint32_t cap_id);
uint32_t val_cxl_device_cache_capable(uint32_t bdf);
uint32_t val_cxl_device_is_cxl(uint32_t bdf);
const char *val_cxl_cap_name(uint16_t id);
uint32_t val_cxl_check_persistent_memory(uint32_t index);
uint32_t val_cxl_map_hdm_address(uint64_t base, uint64_t length, volatile uint64_t **virt_out);

uint32_t cxl001_entry(uint32_t num_pe);
uint32_t cxl002_entry(uint32_t num_pe);
uint32_t cxl003_entry(uint32_t num_pe);
uint32_t cxl004_entry(uint32_t num_pe);
uint32_t cxl010_entry(uint32_t num_pe);
uint32_t cxl011_entry(uint32_t num_pe);
uint32_t cxl013_entry(uint32_t num_pe);

#endif
