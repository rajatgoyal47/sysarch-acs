/** @file
 * Copyright (c) 2016-2025, Arm Limited or its affiliates. All rights reserved.
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

#include "include/acs_val.h"
#include "include/acs_common.h"
#include "include/acs_iovirt.h"
#include "include/acs_smmu.h"
#include "include/acs_mmu.h"

IOVIRT_INFO_TABLE *g_iovirt_info_table;
uint32_t g_num_smmus;

/**
  @brief   This API is a single point of entry to retrieve
           SMMU information stored in the IoVirt Info table
           1. Caller       -  val_smmu_get_info
           2. Prerequisite -  val_iovirt_create_info_table
  @param   type   the type of information being requested
  @param   index  smmu index in iovirt table
  @return  64-bit data
**/
uint64_t
val_iovirt_get_smmu_info(SMMU_INFO_e type, uint32_t index)
{
  uint32_t i,j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "GET_SMMU_INFO: iovirt info table is not created\n", 0);
      return 0;
  }

  if(type == SMMU_NUM_CTRL)
       return g_iovirt_info_table->num_smmus;

  /* Go through the table return the relevant field value for the SMMU block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];
  for(i = 0; i < g_iovirt_info_table->num_blocks; i++, block=IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if(block->type == IOVIRT_NODE_SMMU || block->type == IOVIRT_NODE_SMMU_V3)
      {
          if(j == index)
          {
              switch(type)
              {
                  case SMMU_CTRL_ARCH_MAJOR_REV:
                      return block->data.smmu.arch_major_rev;
                  case SMMU_CTRL_BASE:
                      return block->data.smmu.base;
                  case SMMU_IOVIRT_BLOCK:
                      return (uint64_t)block;
                  default:
                      val_print(ACS_PRINT_ERR, "This SMMU info option not supported %d\n", type);
                      return 0;
              }
          }
          j++;
      }
  }

  if (index > j-1)
  {
      val_print(ACS_PRINT_ERR, "GET_SMMU_INFO: Index (%d) is greater than num of SMMU\n", index);
      return 0;
  }
  return j;
}

/**
  @brief   This API is a single point of entry to retrieve
           PCIe Root Complex Node info stored in the iovirt Info table
           1. Caller       -  Test suite
           2. Prerequisite -  val_iovirt_create_info_table
  @param   type   the type of information being requested
  @param   index  PCIe RC index in iovirt table
  @return  64-bit data
**/
uint64_t
val_iovirt_get_pcie_rc_info(PCIE_RC_INFO_e type, uint32_t index)
{
  uint32_t i,j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "GET_PCIe_RC_INFO: iovirt info table is not created\n", 0);
      return 0;
  }

  if(type == NUM_PCIE_RC)
      return g_iovirt_info_table->num_pci_rcs;

  /* Go through the table return the relevant field value for the SMMU block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];
  for(i = 0; i < g_iovirt_info_table->num_blocks; i++, block=IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if(block->type == IOVIRT_NODE_PCI_ROOT_COMPLEX)
      {
          if(j == index)
          {
              switch(type)
              {
                  case RC_SEGMENT_NUM:
                      return block->data.rc.segment;
                  case RC_MEM_ATTRIBUTE:
                      return block->data.rc.cca;
                  case RC_ATS_ATTRIBUTE:
                      return block->data.rc.ats_attr;
                  case RC_IOVIRT_BLOCK:
                      return (uint64_t)block;
                  case RC_SMMU_BASE:
                      return block->data.rc.smmu_base;
                  default:
                      val_print(ACS_PRINT_ERR, "This PCIe RC info option not supported %d\n", type);
                      return 0;
              }
          }
          j++;
      }
  }
  if (index > j-1)
  {
      val_print(ACS_PRINT_ERR, "GET_PCIe_RC_INFO: Index (%d) is greater than num of PCIe-RC\n",
                                                                                            index);
      return 0;
  }
  return j;

}

/**
  @brief  Calculate the device id and stream id orresponding to the requestor id
  @param  rid          Requestor ID
  @param  segment      pci_segment_number
  @param  *device_id   Pointer to device id
  @param  *stream_id   Pointer to stream id
  @param  *its_id      Pointer to its id
  @return status
**/

int
val_iovirt_get_device_info(uint32_t rid, uint32_t segment, uint32_t *device_id,
                           uint32_t *stream_id, uint32_t *its_id)
{
  uint32_t i, j, id = 0;
  uint32_t sid, did, oref;
  uint32_t itsid = 0;
  uint32_t mapping_found;
  IOVIRT_BLOCK *block;
  NODE_DATA_MAP *map;
  if (g_iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "\n       GET_DEVICE_ID: iovirt info table is not created", 0);
      return ACS_STATUS_ERR;
  }
  if (!device_id) {
      val_print(ACS_PRINT_ERR, "\n       GET_DEVICE_ID: Invalid parameters", 0);
      return ACS_STATUS_ERR;
  }

  /* Search for root complex block with same segment number, and in whose id */
  /* mapping range 'rid' falls. Calculate the output id */
  block = &g_iovirt_info_table->blocks[0];
  mapping_found = 0;
  for (i = 0; i < g_iovirt_info_table->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if (block->type == IOVIRT_NODE_PCI_ROOT_COMPLEX
          && block->data.rc.segment == segment)
      {
          for (j = 0, map = &block->data_map[0]; j < block->num_data_map; j++, map++)
          {
              if(rid >= (*map).map.input_base
                      && rid <= ((*map).map.input_base + (*map).map.id_count))
              {
                  id =  (rid - (*map).map.input_base) + (*map).map.output_base;
                  oref = (*map).map.output_ref;
                  mapping_found = 1;
                  break;
              }
          }
      }
  }
  if (!mapping_found) {
      val_print(ACS_PRINT_ERR,
             "\n       RID to Stream/Dev ID map not found ", 0);
      return ACS_STATUS_ERR;
  }
  /* If output reference node is to ITS group, 'id' is device id */
  block = (IOVIRT_BLOCK*)((uint8_t*)g_iovirt_info_table + oref);
  if(block->type == IOVIRT_NODE_ITS_GROUP)
  {
      did = id;
      sid = ~((uint32_t)0);
      itsid = block->data_map[0].id[0];
  }
  /* If output reference is to SMMU block, 'id' is stream id */
  /* Go through id mappings of this block and find corresponding device id */
  else if(block->type == IOVIRT_NODE_SMMU || block->type == IOVIRT_NODE_SMMU_V3)
  {
      sid = id;
      id = 0;
      mapping_found = 0;
      for(i = 0, map = &block->data_map[0]; i < block->num_data_map; i++, map++)
      {
          if(sid >= (*map).map.input_base && sid <= ((*map).map.input_base +
                                                    (*map).map.id_count))
          {
              did =  (sid - (*map).map.input_base) + (*map).map.output_base;
              oref = (*map).map.output_ref;
              mapping_found = 1;
              break;
          }
      }
      /* If output reference node is to ITS group */
      block = (IOVIRT_BLOCK*)((uint8_t*)g_iovirt_info_table + oref);
      if(block->type == IOVIRT_NODE_ITS_GROUP)
          itsid = block->data_map[0].id[0];
  }
  else
  {
    val_print(ACS_PRINT_ERR, "\n       GET_DEVICE_ID: Invalid mapping for RC in IORT", 0);
    return ACS_STATUS_ERR;
  }
  if (!mapping_found)
  {
    val_print(ACS_PRINT_ERR,
                        "\n       GET_DEVICE_ID: Stream ID to Device ID mapping not found", 0);
    return ACS_STATUS_ERR;
  }

  if (its_id)
      *its_id = itsid;
  if (stream_id)
      *stream_id = sid;
  *device_id = did;
  return 0;
}

/**
  @brief   This API will call PAL layer to fill in the IO Virt information
           into the g_iovirt_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   iovirt_info_table  pre-allocated memory pointer for iovirt_info
  @return  Error if Input param is NULL
**/
void
val_iovirt_create_info_table(uint64_t *iovirt_info_table)
{
  uint32_t i, smmu_ver;
  uint32_t smmu_minor;
  uint64_t smmu_base = 0;

  if (iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "\n   Input for Create Info table cannot be NULL\n", 0);
      return;
  }
  val_print(ACS_PRINT_INFO, " Creating SMMU INFO table\n", 0);

  g_iovirt_info_table = (IOVIRT_INFO_TABLE *)iovirt_info_table;

  pal_iovirt_create_info_table(g_iovirt_info_table);

  g_num_smmus = (uint32_t)val_iovirt_get_smmu_info(SMMU_NUM_CTRL, 0);
  val_print(ACS_PRINT_TEST,
            " SMMU_INFO: Number of SMMU CTRL       :    %d\n", g_num_smmus);
  for (i = 0; i < g_num_smmus; i++) {

    smmu_base = val_smmu_get_info(SMMU_CTRL_BASE, i);
    val_print(ACS_PRINT_DEBUG, "\n   SMMU index %x, ", i);
    val_print(ACS_PRINT_DEBUG, "Base : 0x%llx", smmu_base);

#ifndef TARGET_LINUX
    val_print(ACS_PRINT_DEBUG, "\n   Check for SMMU index %x entry in memmap", i);
    if (val_mmu_update_entry(smmu_base, SMMU_MAP_SIZE))
      val_print(ACS_PRINT_WARN, "\n   Adding SMMU index %x entry failed", i);
#endif

    smmu_ver = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, i);
    val_print(ACS_PRINT_TEST,
            " SMMU_INFO: SMMU index %.2d ", i);
    val_print(ACS_PRINT_TEST, "version     :    v%d", smmu_ver);

    if (smmu_ver == 3) {
      smmu_minor = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, i), 0, 3);
      val_print(ACS_PRINT_TEST, ".%d", smmu_minor);
    }
    val_print(ACS_PRINT_TEST, "\n", smmu_ver);
  }
}

/**
  @brief This API deletes IO virt info table pointed by g_iovirt_info_table pointer

  @param None

  @return None
**/
void
val_iovirt_free_info_table(void)
{
    if (g_iovirt_info_table != NULL) {
        pal_mem_free_aligned((void *)g_iovirt_info_table);
        g_iovirt_info_table = NULL;
    }
    else {
      val_print(ACS_PRINT_ERR,
                  "\n WARNING: g_iovirt_info_table pointer is already NULL",
        0);
    }
}

/**
  @brief This API returns the SMMU index of root complex node requested

  @param rc_seg_num Root complex segment number
  @param rid Unique requester ID

  @return SMMU index of root complex node requested
**/
uint32_t
val_iovirt_get_rc_smmu_index(uint32_t rc_seg_num, uint32_t rid)
{

  uint32_t num_smmu;
  uint64_t smmu_base;

  smmu_base = pal_iovirt_get_rc_smmu_base(g_iovirt_info_table, rc_seg_num, rid);
  if (smmu_base) {
      num_smmu = (uint32_t)val_smmu_get_info(SMMU_NUM_CTRL, 0);
      while (num_smmu--) {
          if (smmu_base == val_smmu_get_info(SMMU_CTRL_BASE, num_smmu))
              return num_smmu;
      }
  }

  val_print(ACS_PRINT_INFO, "\n       RC with segment number %d is not behind SMMU", rc_seg_num);
  return ACS_INVALID_INDEX;
}

#if defined(TARGET_LINUX) || defined(TARGET_EMULATION)
/**
  @brief   This API will call PAL layer to fill in the path of the hid passed in the
           hid parameter.
           1. Caller       -  Application layer.
  @param   hid      hardware ID of the device to which the path is filled
  @param   hid_path 2D array in which the path is stored
  @return  Error if not able to find the path of given hid
**/
uint32_t
val_get_device_path(const char *hid, char hid_path[][MAX_NAMED_COMP_LENGTH])
{
  uint32_t status = 0;

  status = pal_get_device_path(hid, hid_path);

  return status;
}

/**
  @brief   This API will call PAL layer to check if etr is behind the catu.
           1. Caller       -  Application layer.
  @param   etr_path  path of ETR
  @return  Error if CATU is not behind ETR device
**/

uint32_t
val_smmu_is_etr_behind_catu(char *etr_path)
{
  uint32_t status = 0;

  status = pal_smmu_is_etr_behind_catu(etr_path);

  return status;
}
#endif

/**
  @brief   This API is a single point of entry to retrieve
           Named component info stored in the iovirt info table.
           1. Caller       -  Test suite
           2. Prerequisite -  val_iovirt_create_info_table
  @param   type   the type of information being requested.
  @param   index  the index of named component info instance.
  @return  64-bit data
**/
uint64_t
val_iovirt_get_named_comp_info(NAMED_COMP_INFO_e type, uint32_t index)
{
  uint32_t i, j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "GET_NAMED_COMP_INFO: iovirt info table is not created\n", 0);
      return 0; /* imply no named components parsed */
  }

  if (type == NUM_NAMED_COMP)
      return g_iovirt_info_table->num_named_components;

  /* check if index in range */
  if (index > g_iovirt_info_table->num_named_components - 1) {
      val_print(ACS_PRINT_ERR,
                "GET_NAMED_COMP_INFO: Index (%d) is greater than num of Named components\n",
                 index);
      return INVALID_NAMED_COMP_INFO;
  }

  /* Go through the table return the relevant field value for the Named component block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];
  for (i = 0; i < g_iovirt_info_table->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if (block->type == IOVIRT_NODE_NAMED_COMPONENT)
      {
          if (j == index)
          {
              switch (type)
              {
                  case NAMED_COMP_CCA_ATTR:
                      return block->data.named_comp.cca;
                  case NAMED_COMP_DEV_OBJ_NAME:
                      /* caller needs to typecast data to (char *) to retrieve full path
                        to named component in namespace */
                      return (uint64_t)block->data.named_comp.name;
                  case NAMED_COMP_SMMU_BASE:
                      return block->data.named_comp.smmu_base;
                  default:
                      val_print(ACS_PRINT_ERR,
                                "This Named component info option not supported %d\n", type);
                      return INVALID_NAMED_COMP_INFO;
              }
          }
          j++;
      }
  }

  return INVALID_NAMED_COMP_INFO;
}

/**
  @brief   This API is a single point of entry to retrieve
           PMCG information stored in the IoVirt Info table
           1. Caller       -  Test suite
           2. Prerequisite -  val_iovirt_create_info_table
  @param   type   the type of information being requested
  @param   index  the index of pmcg info instance.
  @return  64-bit data
**/
uint64_t
val_iovirt_get_pmcg_info(PMCG_INFO_e type, uint32_t index)
{
  uint32_t i, j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "GET_PMCG_INFO: iovirt info table is not created\n", 0);
      return 0;
  }

  if (type == PMCG_NUM_CTRL)
       return g_iovirt_info_table->num_pmcgs;

  /* Go through the table return the relevant field value for the SMMU block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];
  for (i = 0; i < g_iovirt_info_table->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if (block->type == IOVIRT_NODE_PMCG)
      {
          if (j == index)
          {
              switch (type)
              {
                  case PMCG_CTRL_BASE:
                      return block->data.pmcg.base;
                  case PMCG_IOVIRT_BLOCK:
                      return (uint64_t)block;
                  case PMCG_NODE_REF:
                      return block->data.pmcg.node_ref;
                  case PMCG_NODE_SMMU_BASE:
                      return block->data.pmcg.smmu_base;
                  default:
                      val_print(ACS_PRINT_ERR, "This PMCG info option not supported %d\n", type);
                      return 0;
              }
          }
          j++;
      }
  }

  if (index > j-1)
  {
      val_print(ACS_PRINT_ERR, "GET_PMCG_INFO: Index (%d) is greater than num of PMCG\n", index);
      return 0;
  }
  return j;
}

uint32_t
val_iovirt_get_rc_index(uint32_t rc_seg_num)
{
  uint32_t i, j = 0;
  IOVIRT_BLOCK *block;

  if (g_iovirt_info_table == NULL)
  {
      val_print(ACS_PRINT_ERR, "GET_PCIe_RC_INFO: iovirt info table is not created\n", 0);
      return 0;
  }

  /* Go through the table to reach a RC with the segment number */
  block = &g_iovirt_info_table->blocks[0];
  for (i = 0; i < g_iovirt_info_table->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if (block->type == IOVIRT_NODE_PCI_ROOT_COMPLEX)
      {
          if (block->data.rc.segment == rc_seg_num)
          {
             break;
          }
          j++;
      }
  }
  if (i >=  g_iovirt_info_table->num_blocks)
  {
      val_print(ACS_PRINT_ERR, "GET_PCIe_RC_INFO: segemnt (%d) is not valid\n", rc_seg_num);
      return ACS_INVALID_INDEX;
  }
  return j;

}

/**
  @brief   This API is a single point of entry to retrieve
           ITS information stored in the IoVirt Info table
           1. Prerequisite -  val_iovirt_create_info_table
  @param   type           : The type of information being requested
  @param   group_index    : ITS group index
  @param   param          : value to be passed based on use-case
  @param   return_value   : return data
  @return  Status
**/
int
val_iovirt_get_its_info(
  uint32_t type, uint32_t group_index, uint32_t param, uint32_t *return_value)
{
  uint32_t i = 0;
  uint32_t j = 0;
  uint32_t it = 0;
  IOVIRT_BLOCK *block;


  if (g_iovirt_info_table == NULL) {
    val_print(ACS_PRINT_ERR, "GET_ITS_INFO: iovirt info table is not created\n", 0);
    return ACS_STATUS_ERR;
  }
  if (!return_value) {
      val_print(ACS_PRINT_ERR, "GET_ITS_INFO: Return pointer is NULL\n", 0);
      return ACS_STATUS_ERR;
  }

  if (type == ITS_NUM_GROUPS) {
      *return_value = g_iovirt_info_table->num_its_groups;
      return ACS_STATUS_PASS;
   }

  /* Go through the table return the relevant field value for the ITS block */
  /* at the index position */
  block = &g_iovirt_info_table->blocks[0];

  for (i = 0; i < g_iovirt_info_table->num_blocks; i++, block = IOVIRT_NEXT_BLOCK(block))
  {
      block = ALIGN_MEMORY_ACCESS(block);
      if (block->type == IOVIRT_NODE_ITS_GROUP) {
          if (type == ITS_GET_GRP_INDEX_FOR_ID) {
              /* Return the ITS Group Index for ITS_ID = param */
              for (it = 0; it < block->data.its_count; it++) {
                if (block->data_map[0].id[it] == param) {
                  *return_value = j;
                  return ACS_STATUS_PASS;
                }
              }
              j++;
              continue;
          }
          if (group_index == j) {
              switch (type)
              {
                case ITS_GROUP_NUM_BLOCKS: /* Return Number of ITS blocks in this group */
                  *return_value = block->data.its_count;
                  return ACS_STATUS_PASS;
                case ITS_GET_ID_FOR_BLK_INDEX:
                  /* Get ITS Block ID for index = param in this current ITS Group */
                  for (it = 0; it < block->data.its_count; it++) {
                      if (it == param) {
                          *return_value = block->data_map[0].id[it];
                          return ACS_STATUS_PASS;
                      }
                  }
                  /* Return Error if block not found */
                  return ACS_INVALID_INDEX;
                case ITS_GET_BLK_INDEX_FOR_ID:
                  /* Get ITS Block index for ITS_ID = param in ITS Group = group_index */
                  for (it = 0; it < block->data.its_count; it++) {
                      if (block->data_map[0].id[it] == param) {
                        *return_value = it;
                        return ACS_STATUS_PASS;
                      }
                  }
                  /* ITS_ID not found in current group, return error */
                  return ACS_INVALID_INDEX;
                default:
                  val_print(ACS_PRINT_ERR, "This ITS info option not supported %d\n", type);
                  return ACS_STATUS_ERR;
              }
              break;
          }
          j++;
      }
  }

  val_print(ACS_PRINT_ERR, "GET_ITS_INFO: ITS Group not found %d\n", group_index);
  return ACS_INVALID_INDEX;
}

/**
  @brief Check if given SMMU node has unique context bank interrupt ids

  @param smmu_index smmu index in iovirt table

  @return 0 if test fail ; 1 if test pass
**/
uint32_t
val_iovirt_check_unique_ctx_intid(uint32_t smmu_index)
{
  uint64_t smmu_block = val_iovirt_get_smmu_info(SMMU_IOVIRT_BLOCK, smmu_index);
  return pal_iovirt_check_unique_ctx_intid(smmu_block);
}
