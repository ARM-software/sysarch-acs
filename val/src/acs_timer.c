/** @file
 * Copyright (c) 2016-2019, 2021-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "acs_timer.h"
#include "acs_common.h"
#include "acs_pe.h"
#include "acs_mmu.h"
#include "acs_timer_infra.h"

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

static uint8_t timer_reg_requires_el2(ARM_ARCH_TIMER_REGS Reg)
{
  switch (Reg) {
  case CntvOff:
  case CnthpCtl:
  case CnthpTval:
  case CnthvCtl:
  case CnthvTval:
  case CnthCtl:
  case CnthpCval:
  case CnthvCval:
    return 1;
  default:
    return 0;
  }
}

static uint8_t timer_reg_el2_access_allowed(ARM_ARCH_TIMER_REGS Reg)
{
  if (!timer_reg_requires_el2(Reg))
    return 1;

  if (val_pe_reg_read(CurrentEL) == AARCH64_EL1) {
    val_print(INFO, "The register is related to Hypervisor Mode. "
                    "Can't perform requested operation\n");
    return 0;
  }

  return 1;
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

    if (!timer_reg_el2_access_allowed(Reg))
      return 0xFFFFFFFF;

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
      return read_cnthctl_el2();
    case CnthpCval:
      return read_cnthp_cval_el2();
    case CnthvCval:
      return read_cnthv_cval_el2();

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

    if (!timer_reg_el2_access_allowed(Reg))
      return;

    switch (Reg) {

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
      write_cnthctl_el2(*data_buf);
      break;
    case CnthpCval:
      write_cnthp_cval_el2(*data_buf);
      break;
    case CnthvCval:
      write_cnthv_cval_el2(*data_buf);
      break;

    default:
      val_print(INFO, "Unknown ARM Generic Timer register %x.\n ", Reg);
    }
}

/**
  @brief   This API enables the Architecture timer whose register is given as the input parameter.
           1. Caller       -  VAL
           2. Prerequisite -  None
  @param   reg  - system register of the ELx Arch timer.

  @return  None
**/
void
ArmGenericTimerEnableTimer (
  ARM_ARCH_TIMER_REGS reg
 )
{
  uint64_t timer_ctrl_reg;

  timer_ctrl_reg = ArmArchTimerReadReg (reg);
  timer_ctrl_reg &= (~ARM_ARCH_TIMER_IMASK);
  timer_ctrl_reg |= ARM_ARCH_TIMER_ENABLE;
  ArmArchTimerWriteReg (reg, &timer_ctrl_reg);
}

/**
  @brief   This API disables the Architecture timer whose register is given as the input parameter.
           1. Caller       -  VAL
           2. Prerequisite -  None
  @param   reg  - system register of the ELx Arch timer.

  @return  None
**/
void
ArmGenericTimerDisableTimer (
  ARM_ARCH_TIMER_REGS reg
 )
{
  uint64_t timer_ctrl_reg;

  timer_ctrl_reg = ArmArchTimerReadReg (reg);
  timer_ctrl_reg |= ARM_ARCH_TIMER_IMASK;
  timer_ctrl_reg &= ~ARM_ARCH_TIMER_ENABLE;
  ArmArchTimerWriteReg (reg, &timer_ctrl_reg);
}

/**
  @brief   This API programs the el1 phy timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_phy_el1(uint32_t timeout)
{
  uint64_t cval;
  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CntpCtl);

    /* Program the timer */
    cval = syscounter_read();
    cval += (uint64_t)timeout;

    ArmArchTimerWriteReg(CntpCval, &cval);
    ArmGenericTimerEnableTimer(CntpCtl);
  } else {
    ArmGenericTimerDisableTimer(CntpCtl);
 }
}

/**
  @brief   This API programs the el1 Virtual timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_vir_el1(uint32_t timeout)
{
  uint64_t cval;
  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CntvCtl);

    /* Program the timer */
    cval = syscounter_read();
    cval += (uint64_t)timeout;

    ArmArchTimerWriteReg(CntvCval, &cval);
    ArmGenericTimerEnableTimer(CntvCtl);
  } else {
    ArmGenericTimerDisableTimer(CntvCtl);
 }
}

/**
  @brief   This API programs the el2 phy timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_phy_el2(uint32_t timeout)
{
  uint64_t cval;
  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CnthpCtl);

    /* Program the timer */
    cval = syscounter_read();
    cval += (uint64_t)timeout;

    ArmArchTimerWriteReg(CnthpCval, &cval);
    ArmGenericTimerEnableTimer(CnthpCtl);
  } else {
    ArmGenericTimerDisableTimer(CnthpCtl);
 }
}

/**
  @brief   This API programs the el2 Virt timer with the input timeout value.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   timeout - clock ticks after which an interrupt is generated.

  @return  None
**/
void
val_timer_set_vir_el2(uint32_t timeout)
{
  uint64_t cval;
  if (timeout != 0) {
    ArmGenericTimerDisableTimer(CnthvCtl);

    /* Program the timer */
    cval = syscounter_read();
    cval += (uint64_t)timeout;

    ArmArchTimerWriteReg(CnthvCval, &cval);
    ArmGenericTimerEnableTimer(CnthvCtl);
  } else {
    ArmGenericTimerDisableTimer(CnthvCtl);
 }
}

/**
  @brief   This API to get the el2 phy timer count.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   None

  @return  Current timer count
**/
uint64_t
val_get_phy_el2_timer_count(void)
{
  return  ArmArchTimerReadReg(CnthpTval);
}

/**
  @brief   This API to get the el1 phy timer count.
           1. Caller       -  Test Suite
           2. Prerequisite -  None
  @param   None

  @return  Current timer count
**/
uint64_t
val_get_phy_el1_timer_count(void)
{
  return  ArmArchTimerReadReg(CntpTval);
}

/**
  @brief  This API will program and start the counter

  @param  cnt_base_n  Counter base address
  @param  timeout     Timeout value

  @return None
**/
void
val_timer_set_system_timer(addr_t cnt_base_n, uint32_t timeout)
{
  /* Start the System timer */
  val_mmio_write(cnt_base_n + CNTP_TVAL, timeout);

  /* enable System timer */
  val_mmio_write(cnt_base_n + CNTP_CTL, 1);

}

/**
  @brief  This API will stop the counter

  @param  cnt_base_n  Counter base address

  @return None
**/
void
val_timer_disable_system_timer(addr_t cnt_base_n)
{

  /* stop System timer */
  val_mmio_write(cnt_base_n + CNTP_CTL, 0);
}

/**
  @brief  This API will read CNTACR (from CNTCTLBase) to determine whether
          access permission from NS state is permitted

  @param  index  index of SYS counter in timer table

  @return Status 0 if success
**/
uint32_t
val_timer_skip_if_cntbase_access_not_allowed(uint64_t index)
{
  uint64_t cnt_ctl_base;
  uint32_t data;
  uint32_t frame_num = 0;

  cnt_ctl_base = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, index);
  frame_num = val_timer_get_info(TIMER_INFO_FRAME_NUM, index);

  if (cnt_ctl_base) {
      data = val_mmio_read(cnt_ctl_base + CNTACR + frame_num * 4);
      if ((data & 0x1) == 0x1)
          return 0;
      else {
          data |= 0x1;
          val_mmio_write(cnt_ctl_base + CNTACR + frame_num * 4, data);
          data = val_mmio_read(cnt_ctl_base + CNTACR + frame_num * 4);
          if ((data & 0x1) == 1)
              return 0;
          else
              return ACS_STATUS_SKIP;
      }
  }
  else
      return ACS_STATUS_SKIP;

}

/**
  @brief  Get a safe timeout value in timer ticks.

  @param  None

  @return uint32_t  Timeout value computed as (counter frequency / MAX_WAKEUP_TIMEOUT),
                    guaranteed to fit in 32-bit timer registers.
**/
uint32_t
val_get_safe_timeout_ticks(void)
{
    /*
     * Compute a safe timer tick value based on the maximum supported
     * wakeup timeout. This ensures the returned value always fits in
     * a 32-bit timer register even on high-frequency systems.
     */
    uint64_t freq = val_get_counter_frequency();
    uint64_t ticks = freq / MAX_WAKEUP_TIMEOUT;

    if (ticks > 0xFFFFFFFF)
        ticks = 0xFFFFFFFF;

    return (uint32_t)ticks;
}

/**
  @brief  convert timeout in terms of us to ticks.

  @param  timeout_us input timeout in terms of us

  @return uint64_t  timeout in terms of ticks.
**/
uint64_t
val_get_timeout_to_ticks(uint32_t timeout_us)
{

    uint64_t freq = val_get_counter_frequency();
    uint64_t ticks = (timeout_us * freq)/MICRO_SECONDS;

    return ticks;
}
