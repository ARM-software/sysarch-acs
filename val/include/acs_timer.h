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

#ifndef __ACS_TIMER_H__
#define __ACS_TIMER_H__

#include "val_sysreg_timer.h"
#include "acs_timer_infra.h"

#define ARM_ARCH_TIMER_ENABLE           (1 << 0)
#define ARM_ARCH_TIMER_IMASK            (1 << 1)
#define ARM_ARCH_TIMER_ISTATUS          (1 << 2)

typedef enum {
  CntFrq = 0,
  CntPct,
  CntPctSS,
  CntkCtl,
  CntpTval,
  CntpCtl,
  CntvTval,
  CntvCtl,
  CntvCt,
  CntVctSS,
  CntpCval,
  CntvCval,
  CntvOff,
  CnthCtl,
  CnthpTval,
  CnthpCtl,
  CnthpCval,
  CnthvTval,
  CnthvCtl,
  CnthvCval,
  RegMaximum
} ARM_ARCH_TIMER_REGS;


uint64_t
ArmArchTimerReadReg (
  ARM_ARCH_TIMER_REGS   Reg
  );


void
ArmArchTimerWriteReg (
    ARM_ARCH_TIMER_REGS   Reg,
    uint64_t              *data_buf
  );

void
ArmWriteCntFrq (
  uint64_t   FreqInHz
  );

void ArmGenericTimerEnableTimer (ARM_ARCH_TIMER_REGS reg);
void ArmGenericTimerDisableTimer (ARM_ARCH_TIMER_REGS reg);

void val_timer_set_phy_el1(uint32_t timeout);
void val_timer_set_vir_el1(uint32_t timeout);
void val_timer_set_phy_el2(uint32_t timeout);
void val_timer_set_vir_el2(uint32_t timeout);

#endif // __ACS_TIMER_H__
