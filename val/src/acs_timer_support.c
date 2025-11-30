/** @file
 * Copyright (c) 2016-2018, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include "include/acs_timer_support.h"
#include "include/acs_common.h"
#include "include/acs_pe.h"

/**
  @brief This API is used to get the effective HCR_EL2.E2H
**/
uint8_t get_effective_e2h(void)
{
  uint32_t effective_e2h;

  /* if EL2 is not present, effective E2H will be 0 */
  if (val_pe_reg_read(CurrentEL) == AARCH64_EL1) {
    val_print(ACS_PRINT_DEBUG, "\n       CurrentEL: AARCH64_EL1", 0);
    return 0;
  }

  uint32_t hcr_e2h = VAL_EXTRACT_BITS(ArmReadHcrEl2(), 34, 34);
  uint32_t feat_vhe = VAL_EXTRACT_BITS(ArmReadAA64MMFR1EL1(), 8, 11);
  uint32_t e2h0 = VAL_EXTRACT_BITS(ArmReadAA64MMFR4EL1(), 24, 27);

  val_print(ACS_PRINT_DEBUG, "\n       hcr_e2h   : 0x%x", hcr_e2h);
  val_print(ACS_PRINT_DEBUG, "\n       feat_vhe  : 0x%x", feat_vhe);
  val_print(ACS_PRINT_DEBUG, "\n       e2h0 : 0x%x", e2h0);

  if (feat_vhe == 0x0) //ID_AA64MMFR1_EL1.VH
    effective_e2h = 0;
  else if (e2h0 != 0x0) //E2H0 = 0 means implemented
    effective_e2h = 1;
  else
    effective_e2h = hcr_e2h;

  val_print(ACS_PRINT_DEBUG, "\n       effective e2h : 0x%x\n", effective_e2h);
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
      return ArmReadCntFrq();

    case CntPct:
      return ArmReadCntPct();

    case CntPctSS:
      return ArmReadCntPctSS();

    case CntkCtl:
      return effective_e2h ? ArmReadCntkCtl12() : ArmReadCntkCtl();

    case CntpTval:
      /* Check For E2H, If EL2 Host then access to cntp_tval_el02 */
      return effective_e2h ? ArmReadCntpTval02() : ArmReadCntpTval();

    case CntpCtl:
      /* Check For E2H, If EL2 Host then access to cntp_ctl_el02 */
      return effective_e2h ? ArmReadCntpCtl02() : ArmReadCntpCtl();

    case CntvTval:
      return effective_e2h ? ArmReadCntvTval02() : ArmReadCntvTval();

    case CntvCtl:
      return effective_e2h ? ArmReadCntvCtl02() : ArmReadCntvCtl();

    case CntvCt:
      return ArmReadCntvCt();

    case CntVctSS:
      return ArmReadCntVctSS();

    case CntpCval:
      return effective_e2h ? ArmReadCntpCval02() : ArmReadCntpCval();

    case CntvCval:
      return effective_e2h ? ArmReadCntvCval02() : ArmReadCntvCval();

    case CntvOff:
      return ArmReadCntvOff();
    case CnthpCtl:
      return ArmReadCnthpCtl();
    case CnthpTval:
      return ArmReadCnthpTval();
    case CnthvCtl:
      return ArmReadCnthvCtl();
    case CnthvTval:
      return ArmReadCnthvTval();

    case CnthCtl:
    case CnthpCval:
      val_print(ACS_PRINT_TEST, "The register is related to Hypervisor Mode. \
      Can't perform requested operation\n ", 0);
      break;

    default:
      val_print(ACS_PRINT_TEST, "Unknown ARM Generic Timer register %x.\n ", Reg);
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
      val_print(ACS_PRINT_TEST, "Can't write to Read Only Register: CNTPCT\n", 0);
      break;

    case CntkCtl:
      if (effective_e2h)
        ArmWriteCntkCtl12(*data_buf);
      else
        ArmWriteCntkCtl(*data_buf);
      break;

    case CntpTval:
      if (effective_e2h)
        ArmWriteCntpTval02(*data_buf);
      else
        ArmWriteCntpTval(*data_buf);
      break;

    case CntpCtl:
      if (effective_e2h)
        ArmWriteCntpCtl02(*data_buf);
      else
        ArmWriteCntpCtl(*data_buf);
      break;

    case CntvTval:
      if (effective_e2h)
        ArmWriteCntvTval02(*data_buf);
      else
        ArmWriteCntvTval(*data_buf);
      break;

    case CntvCtl:
      if (effective_e2h)
        ArmWriteCntvCtl02(*data_buf);
      else
        ArmWriteCntvCtl(*data_buf);
      break;

    case CntvCt:
       val_print(ACS_PRINT_TEST, "Can't write to Read Only Register: CNTVCT\n", 0);
      break;

    case CntpCval:
      ArmWriteCntpCval(*data_buf);
      break;

    case CntvCval:
      ArmWriteCntvCval(*data_buf);
      break;

    case CntvOff:
      ArmWriteCntvOff(*data_buf);
      break;

    case CnthpTval:
      ArmWriteCnthpTval(*data_buf);
      break;
    case CnthpCtl:
      ArmWriteCnthpCtl(*data_buf);
      break;
    case CnthvTval:
      ArmWriteCnthvTval(*data_buf);
      break;
    case CnthvCtl:
      ArmWriteCnthvCtl(*data_buf);
      break;
    case CnthCtl:
    case CnthpCval:
      val_print(ACS_PRINT_TEST, "The register is related to Hypervisor Mode. \
      Can't perform requested operation\n", 0);
      break;

    default:
      val_print(ACS_PRINT_TEST, "Unknown ARM Generic Timer register %x.\n ", Reg);
    }
}
