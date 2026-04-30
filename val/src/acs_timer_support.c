/** @file
 * Copyright (c) 2016-2018, 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_timer_support.h"
#include "acs_common.h"
#include "acs_pe.h"

/**
  @brief This API is used to get the effective HCR_EL2.E2H
**/
uint8_t get_effective_e2h(void)
{
  uint32_t effective_e2h;

  /* if EL2 is not present, effective E2H will be 0 */
  if (val_pe_reg_read(CurrentEL) == AARCH64_EL1) {
    val_print(DEBUG, "\n       CurrentEL: AARCH64_EL1");
    return 0;
  }

  uint32_t hcr_e2h = VAL_EXTRACT_BITS(read_hcr_el2(), 34, 34);
  uint32_t feat_vhe = VAL_EXTRACT_BITS(read_id_aa64mmfr1_el1(), 8, 11);
  uint32_t e2h0 = VAL_EXTRACT_BITS(read_s3_0_c0_c7_4(), 24, 27);

  val_print(DEBUG, "\n       hcr_e2h   : 0x%x", hcr_e2h);
  val_print(DEBUG, "\n       feat_vhe  : 0x%x", feat_vhe);
  val_print(DEBUG, "\n       e2h0 : 0x%x", e2h0);

  if (feat_vhe == 0x0) //ID_AA64MMFR1_EL1.VH
    effective_e2h = 0;
  else if (e2h0 != 0x0) //E2H0 = 0 means implemented
    effective_e2h = 1;
  else
    effective_e2h = hcr_e2h;

  val_print(DEBUG, "\n       effective e2h : 0x%x\n", effective_e2h);
  return effective_e2h;
}

/**
  @brief   This API is used to read Timer related registers

  @param   Reg  Register to be read

  @return  Register value
**/
uint64_t
ArmArchTimerReadReg (
    ARM_ARCH_TIMER_REGS   Reg
  )
{
    static uint8_t effective_e2h = 0xFF;
    if (effective_e2h == 0xFF)
      effective_e2h = get_effective_e2h();

    switch (Reg) {

    case CntFrq:
      return read_cntfrq_el0();

    case CntPct:
      return read_cntpct_el0();

    case CntPctSS:
      return read_cntpctss_el0();

    case CntkCtl:
      return effective_e2h ? read_cntkctl_el12() : read_cntkctl_el1();

    case CntpTval:
      /* Check For E2H, If EL2 Host then access to cntp_tval_el02 */
      return effective_e2h ? read_cntp_tval_el02() : read_cntp_tval_el0();

    case CntpCtl:
      /* Check For E2H, If EL2 Host then access to cntp_ctl_el02 */
      return effective_e2h ? read_cntp_ctl_el02() : read_cntp_ctl_el0();

    case CntvTval:
      return effective_e2h ? read_cntv_tval_el02() : read_cntv_tval_el0();

    case CntvCtl:
      return effective_e2h ? read_cntv_ctl_el02() : read_cntv_ctl_el0();

    case CntvCt:
      return read_cntvct_el0();

    case CntVctSS:
      return read_cntvctss_el0();

    case CntpCval:
      return effective_e2h ? read_cntp_cval_el02() : read_cntp_cval_el0();

    case CntvCval:
      return effective_e2h ? read_cntv_cval_el02() : read_cntv_cval_el0();

    case CntvOff:
      return read_cntvoff_el2();
    case CnthpCtl:
      return read_cnthp_ctl_el2();
    case CnthpTval:
      return read_cnthp_tval_el2();
    case CnthvCtl:
      return read_cnthv_ctl_el2();
    case CnthvTval:
      return read_cnthv_tval_el2();

    case CnthCtl:
    case CnthpCval:
      val_print(INFO, "The register is related to Hypervisor Mode. \
      Can't perform requested operation\n ");
      break;

    default:
      val_print(INFO, "Unknown ARM Generic Timer register %x.\n ", Reg);
    }

    return 0xFFFFFFFF;
}

/**
  @brief   This API is used to write Timer related registers

  @param   Reg  Register to be read
  @param   data_buf Data to write in register

  @return  None
**/
void
ArmArchTimerWriteReg (
    ARM_ARCH_TIMER_REGS   Reg,
    uint64_t              *data_buf
  )
{

    static uint8_t effective_e2h = 0xFF;
    if (effective_e2h == 0xFF)
      effective_e2h = get_effective_e2h();

    switch(Reg) {

    case CntPct:
      val_print(INFO, "Can't write to Read Only Register: CNTPCT\n");
      break;

    case CntkCtl:
      if (effective_e2h)
        write_cntkctl_el12(*data_buf);
      else
        write_cntkctl_el1(*data_buf);
      break;

    case CntpTval:
      if (effective_e2h)
        write_cntp_tval_el02(*data_buf);
      else
        write_cntp_tval_el0(*data_buf);
      break;

    case CntpCtl:
      if (effective_e2h)
        write_cntp_ctl_el02(*data_buf);
      else
        write_cntp_ctl_el0(*data_buf);
      break;

    case CntvTval:
      if (effective_e2h)
        write_cntv_tval_el02(*data_buf);
      else
        write_cntv_tval_el0(*data_buf);
      break;

    case CntvCtl:
      if (effective_e2h)
        write_cntv_ctl_el02(*data_buf);
      else
        write_cntv_ctl_el0(*data_buf);
      break;

    case CntvCt:
       val_print(INFO, "Can't write to Read Only Register: CNTVCT\n");
      break;

    case CntpCval:
      write_cntp_cval_el0(*data_buf);
      break;

    case CntvCval:
      write_cntv_cval_el0(*data_buf);
      break;

    case CntvOff:
      write_cntvoff_el2(*data_buf);
      break;

    case CnthpTval:
      write_cnthp_tval_el2(*data_buf);
      break;
    case CnthpCtl:
      write_cnthp_ctl_el2(*data_buf);
      break;
    case CnthvTval:
      write_cnthv_tval_el2(*data_buf);
      break;
    case CnthvCtl:
      write_cnthv_ctl_el2(*data_buf);
      break;
    case CnthCtl:
    case CnthpCval:
      val_print(INFO, "The register is related to Hypervisor Mode. \
      Can't perform requested operation\n");
      break;

    default:
      val_print(INFO, "Unknown ARM Generic Timer register %x.\n ", Reg);
    }
}
