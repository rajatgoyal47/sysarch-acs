/** @file
 * Copyright (c) 2019-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_PCIE_SPEC_H__
#define __ACS_PCIE_SPEC_H__

/* TYPE 0/1 Cmn Cfg reg offsets */
#define TYPE01_VIDR   0x0
#define TYPE01_CR     0x4
#define TYPE01_RIDR   0x8
#define TYPE01_CLSR   0xc
#define TYPE01_BAR    0x10
#define TYPE01_CPR    0x34
#define TYPE01_ILR    0x3c

/* TYPE 0/1 Cmn Cfg reg shifts and masks */
#define TYPE01_VIDR_SHIFT   0
#define TYPE01_VIDR_MASK    0xffff
#define TYPE01_DIDR_SHIFT   16
#define TYPE01_DIDR_MASK    0xffff
#define TYPE01_CCR_SHIFT    8
#define TYPE01_CCR_MASK     0xffffff
#define TYPE01_CPR_SHIFT    0
#define TYPE01_CPR_MASK     0xff
#define TYPE01_HTR_SHIFT    16
#define TYPE01_HTR_MASK     0xff
#define TYPE01_IPR_SHIFT    8
#define TYPE01_IPR_MASK     0xFF
#define TYPE01_ILR_SHIFT    0
#define TYPE01_ILR_MASK     0xFF
#define TYPE01_BCC_SHIFT    24
#define VIDR_RRS_MASK       0x0001
#define DIDR_RRS_MASK       0xFFFF

#define TYPE0_HEADER 0
#define TYPE1_HEADER 1
#define MAS_CC       0x1
#define CNTRL_CC     0x2
#define DP_CNTRL_CC  0x3
#define RES_CC       0x13

/* Command register shifts */
#define CR_MSE_SHIFT   1
#define CR_BME_SHIFT   2
#define CR_SCE_SHIFT   3
#define CR_MWI_SHIFT   4
#define CR_VPS_SHIFT   5
#define CR_IDSEL_SHIFT 7
#define CR_SERRE_SHIFT 8
#define CR_FBTE_SHIFT  9
#define CR_ID_SHIFT    10

/* Command register masks */
#define CR_MSE_MASK    0x1
#define CR_BME_MASK    0x1
#define CR_SCE_MASK    0x1
#define CR_MWI_MASK    0x1
#define CR_VPS_MASK    0x1
#define CR_IDSEL_MASK  0x1
#define CR_SERRE_MASK  0x1
#define CR_FBTE_MASK   0x1
#define CR_ID_MASK     0x1

/* Status Register */
#define SR_IS_SHIFT    19
#define SR_IS_MASK     0x1
#define SR_STA_SHIFT   27
#define SR_STA_MASK    0x1

/* Secondary Status Register */
#define SSR_STA_SHIFT  27
#define SSR_STA_MASK   0x1

/* Class Code Masks */
#define CC_SUB_MASK     0xFF   /* Sub Class */
#define CC_BASE_MASK    0xFF   /* Base Class */
#define CC_PGM_IF_MASK  0xFF   /* Programming Interface */

/* Class Code Shifts */
#define CC_PGM_IF_SHIFT 8
#define CC_SUB_SHIFT    16
#define CC_BASE_SHIFT   24

#define HB_BASE_CLASS   0x06
#define HB_SUB_CLASS    0x00

#define RCEC_BASE_CLASS 0x08
#define RCEC_SUB_CLASS  0x07
#define RCEC_PGMING_IF  0x00

/* BIST register masks */
#define BIST_REG_START  24
#define BIST_REG_END    31
#define BIST_BC_MASK    0x80
#define BIST_SB_MASK    0x40
#define BIST_CC_MASK    0x07

/* Header type reg shifts and masks */
#define HTR_HL_SHIFT   0x0
#define HTR_HL_MASK    0x7f
#define HTR_MFD_SHIFT  7
#define HTR_MFD_MASK   0x1

/* BAR register shifts */
#define BAR_MIT_SHIFT   0
#define BAR_MDT_SHIFT   1
#define BAR_MT_SHIFT    3
#define BAR_BASE_SHIFT  4

/* BAR register masks */
#define BAR_MIT_MASK    0x1
#define BAR_MDT_MASK    0x3
#define BAR_MT_MASK     0x1
#define BAR_BASE_MASK   0xfffffff

/*BAR offset */
#define BAR0_OFFSET               0x10
#define BAR_TYPE_0_MAX_OFFSET     0x24
#define BAR_TYPE_1_MAX_OFFSET     0x14
#define BAR_NP_TYPE               0x0
#define BAR_P_TYPE                0x1
#define BAR_64_BIT                0x1
#define BAR_32_BIT                0x0
#define BAR_REG(bar_reg_value)    ((bar_reg_value >> 2) & 0x1)

#define TYPE0_MAX_BARS  6
#define TYPE1_MAX_BARS  2

/* Type 1 Cfg reg offsets */
#define TYPE1_PBN       0x18
#define TYPE1_SEC_STA   0x1C
#define TYPE1_NP_MEM    0x20
#define TYPE1_P_MEM     0x24
#define TYPE1_P_MEM_BU  0x28    /* Prefetchable Base Upper Offset */
#define TYPE1_P_MEM_LU  0x2C    /* Prefetchable Limit Upper Offset */

/* Type 1 Bridge Control Register */
#define BRIDGE_CTRL_SBR_SET     0x400000

/* Memory Base Reg Shifts */
#define MEM_BA_SHIFT        16
#define MEM_BA_MASK         0xFFF0
#define MEM_LIM_LOWER_BITS  ((1 << 20) - 1)
#define MEM_LIM_MASK        0xFFF00000

#define P_MEM_PAC_MASK  0xF
#define P_MEM_BU_SHIFT  32      /* Base Upper Shift */
#define P_MEM_LU_SHIFT  32      /* Limit Upper Shift */

/* Bus Number reg shifts */
#define SECBN_SHIFT 8
#define SUBBN_SHIFT 16

/* Bus Number reg masks */
#define PRIBN_MASK  0xff
#define SECBN_MASK  0xff
#define SUBBN_MASK  0xff
#define SECBN_EXTRACT 0xffff00ff

/* Capability header reg shifts */
#define PCIE_CIDR_SHIFT      0
#define PCIE_NCPR_SHIFT      8
#define PCIE_ECAP_CIDR_SHIFT  0
#define PCIE_ECAP_NCPR_SHIFT  20

/* Capability header reg masks */
#define PCIE_CIDR_MASK      0xff
#define PCIE_NCPR_MASK      0xff
#define PCIE_ECAP_CIDR_MASK  0xffff
#define PCIE_ECAP_NCPR_MASK  0xfff

#define PCIE_CAP_START   0x40
#define PCIE_CAP_END     0xFC
#define PCIE_ECAP_START  0x100
#define PCIE_ECAP_END    0xFFC

/* Capability Structure IDs */
#define CID_PCIECS     0x10
#define CID_MSI        0x05
#define CID_MSIX       0x11
#define CID_PMC        0x01
#define CID_EA         0x14
#define ECID_AER       0x0001
#define ECID_RCECEA    0x0007
#define ECID_ACS       0x000D
#define ECID_ARICS     0x000E
#define ECID_ATS       0x000F
#define ECID_PRI       0x0013
#define ECID_SPCIECS   0x0019
#define ECID_PASID     0x001B
#define ECID_DPC       0x001D
#define ECID_DVSEC     0x0023
#define ECID_DLFECS    0x0025
#define ECID_PL16ECS   0x0026
#define ECID_LMREC     0x0027

/* PCI Express capability struct offsets */
#define CIDR_OFFSET    0x0
#define PCIECR_OFFSET  0x2
#define DCAPR_OFFSET   0x4
#define ACSCR_OFFSET   0x4
#define DCTLR_OFFSET   0x8
#define LCAPR_OFFSET   0xC
#define LCTRLR_OFFSET  0x10
#define DCAP2R_OFFSET  0x24
#define DCTL2R_OFFSET  0x28
#define LCAP2R_OFFSET  0x2C
#define LCTL2R_OFFSET  0x30
#define DCTL2R_MASK    0xFFFF
#define DSTS_SHIFT     16
#define DS_UNCORR_MASK 0x6
#define DS_CORR_MASK   0x1
#define ACSCTRL_SHIFT  0x0F

/* DPC Capability struct offsets and shifts */
#define DPC_CTRL_OFFSET        0x4
#define DPC_STATUS_OFFSET      0x8
#define DPC_STATUS_RESET       0xFFFFFFFF
#define DPC_STATUS_MASK        0x1
#define DPC_TRIGGER_MASK       0x6
#define DPC_TRIGGER_FATAL      0x2
#define DPC_TRIGGER_NON_FATAL  0x1
#define DPC_CTRL_TRG_EN_SHIFT  16
#define DPC_CTRL_TRG_EN_MASK   0x3
#define DPC_SOURCE_ID_SHIFT    16
#define DPC_TRIGGER_SHIFT      0x1
#define DPC_DISABLE_MASK       0xFFFCFFFF
#define DPC_INTR_ENABLE        0x80000
#define DPC_RP_EXT_OFFSET      0x5
#define DPC_RP_EXT_MASK        0x1

/* AER Capability struct offsets and shifts */
#define ERR_CNT                  0x18
#define AER_UNCORR_STATUS_OFFSET 0x4
#define AER_UNCORR_MASK_OFFSET   0x8
#define AER_UNCORR_SEVR_OFFSET   0xC
#define AER_UNCORR_SEVR_FATAL    0xFFFFFFFF
#define AER_UNCORR_SEVR_NONFATAL 0x0
#define AER_CORR_STATUS_OFFSET   0x10
#define AER_CORR_MASK_OFFSET     0x14
#define AER_ROOT_ERR_CMD_OFFSET  0x2C
#define AER_ROOT_ERR_OFFSET      0x30
#define AER_ROOT_ERR_SOURCE_ID   0x34
#define AER_SOURCE_ID_SHIFT      16
#define AER_SOURCE_ID_MASK       0xFFFF
#define AER_ERROR_MASK           0xFFFFFFFF

/* EA Capability struct offsets */
#define EA_NUM_ENTRY_SHIFT         16
#define EA_NUM_ENTRY_MASK          0x3F
#define EA_ENTRY_TYPE_SIZE_MASK    0x7
#define EA_ENTRY_TYPE_ENABLE_SHIFT 31
#define EA_ENTRY_TYPE_ENABLE_MASK  1

/* ACS Capability Register */
#define ACS_CTRL_SVE_SHIFT  16
#define ACS_CTRL_TBE_SHIFT  17
#define ACS_CTRL_RRE_SHIFT  18
#define ACS_CTRL_UFE_SHIFT  20

/* PCIe capabilities reg shifts and masks */
#define PCIECR_DPT_SHIFT 4
#define PCIECR_DPT_MASK  0xf

/* Link Capabilities register shifts and masks */
#define LCAPR_DLLLARC_SHIFT 20
#define LCAPR_DLLLARC_MASK  0x100000

/* Link Status register shifts and masks */
#define LSTAT_DLLLA_SHIFT   29
#define LSTAT_DLLLA_MASK    0x20000000

/* Device Capabilities register */
#define DCAPR_MPSS_SHIFT 0
#define DCAPR_FLRC_SHIFT 28

/* Device Capabilities reg mask */
#define DCAPR_MPSS_MASK   0x07
#define DCAPR_FLRC_MASK  0x1

/* Device Control reg shifts */
#define DCTLR_SHIFT       0
#define DCTLR_CERE_SHIFT  0
#define DCTLR_NFERE_SHIFT 1
#define DCTLR_FERE_SHIFT  2
#define DCTLR_URRE_SHIFT  3
#define DCTLR_ERE_SHIFT   4
#define DCTLR_PFE_SHIFT   9
#define DCTLR_APE_SHIFT   10
#define DCTLR_ENS_SHIFT   11
#define DCTLR_IFLR_SHIFT  15
#define DCTLR_DSR_SHIFT   16

/* Device Control reg masks */
#define DCTLR_MASK       0xffff
#define DCTLR_CERE_MASK  0x1
#define DCTLR_NFERE_MASK 0x1
#define DCTLR_FERE_MASK  0x1
#define DCTLR_URRE_MASK  0x1
#define DCTLR_PFE_MASK   0x1
#define DCTLR_APE_MASK   0x1
#define DCTLR_ENS_MASK   0x1
#define DCTLR_IFLR_MASK  0x1
#define DCTLR_DSR_MASK   0xffff
#define DCTLR_FLR_SET    0x8000

/* Device Status reg shifts */
#define DSR_CED_SHIFT  0
#define DSR_NFED_SHIFT 1
#define DSR_FED_SHIFT  2
#define DSR_URD_SHIFT  3
#define DSR_TP_SHIFT   5

/* Device Status reg masks */
#define DSR_CED_MASK  0x1
#define DSR_NFED_MASK 0x1
#define DSR_FED_MASK  0x1
#define DSR_URD_MASK  0x1
#define DSR_TP_MASK   0x1

/* Device Capabilities 2 reg shift */
#define DCAP2R_AFS_SHIFT  5
#define DCAP2R_OBFF_SHIFT 18
#define DCAP2R_CTRS_SHIFT 0
#define DCAP2R_CTDS_SHIFT 4
#define DCAP2R_A32C_SHIFT 7
#define DCAP2R_A64C_SHIFT 8
#define DCAP2R_A128C_SHIFT 9
#define DCAP2R_ARS_SHIFT 6

/* Device Capabilities 2 reg mask */
#define DCAP2R_AFS_MASK  0x1
#define DCAP2R_OBFF_MASK 0x3
#define DCAP2R_CTRS_MASK 0xf
#define DCAP2R_CTDS_MASK 0x1
#define DCAP2R_A32C_MASK 0x1
#define DCAP2R_A64C_MASK 0x1
#define DCAP2R_A128C_MASK 0x1
#define DCAP2R_ARS_MASK 0x01

/* Device Control 2 reg shift */
#define DCTL2R_AFE_SHIFT  5

/* Device Control 2 reg mask */
#define DCTL2R_AFE_MASK  0x1
#define DCTL2R_AFE_NORMAL 0xFFDF

#define LCAP2R_SLSV_MASK 0xFF
#define LCAP2R_SLSV_SHIFT 1

/* Device bitmask definitions */
#define RCiEP    (1 << 0b1001)
#define RCEC     (1 << 0b1010)
#define EP       (1 << 0b0000)
#define RP       (1 << 0b0100)
#define UP       (1 << 0b0101)
#define DP       (1 << 0b0110)
#define iEP_EP   (1 << 0b1100)
#define iEP_RP   (1 << 0b1011)
#define PCI_PCIE (1 << 0b1000)
#define PCIE_PCI (1 << 0b0111)
#define PCIe_ALL (iEP_RP | iEP_EP | RP | EP | RCEC | RCiEP)

/* PCIe DWord definition */
#define PCIE_DWORD_SIZE             0x04

/*MSI Capabilities */
#define MSI_ENABLE_SHIFT                16
#define MSI_ADDR_SHIFT                  32
#define MSI_MSG_TBL_LOWER_ADDR_OFFSET   0x4
#define MSI_MSG_TBL_HIGHER_ADDR_OFFSET  0x8
#define MSI_MSG_TBL_DATA_OFFSET         0xC

/* MSI-X Capabilities */
#define MSI_X_ENABLE_SHIFT          31

#define MSI_X_TOR_OFFSET            0x4

#define MSI_X_MSG_TBL_LOWER_ADDR_OFFSET   0x0
#define MSI_X_MSG_TBL_HIGHER_ADDR_OFFSET  0x4
#define MSI_X_MSG_TBL_DATA_OFFSET         0x8
#define MSI_X_MSG_TBL_MVC_OFFSET          0xC

#define MSI_X_TABLE_BIR_MASK        0x7
#define MSI_X_ENTRY_SIZE            16 /* Size of Single MSI Entry in MSI Table */
#define MSI_X_ADDR_SHIFT            32

/* PASID Capabilities */
#define PASID_CAPABILITY_OFFSET     0x4
#define MAX_PASID_MASK              0x1F00
#define MAX_PASID_SHIFT             0x8

/* ATS Capabilities */
#define ATS_CTRL                    0x4
#define ATS_CACHING_EN              (1 << 31)
#define ATS_CACHING_DIS             0x7FFFFFFF

#define MPCAPR_DS_MASK              0x1
#define MPCAPR_DS_SHIFT             0x0

#endif
