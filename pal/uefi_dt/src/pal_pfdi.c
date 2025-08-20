/** @file
 * Copyright (c) 2016-2018, 2021, 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
#include <Base.h>

#include "include/pal_uefi.h"

#define  REG_COUNT_X5_X17 13

VOID
AcsPfdiTest (
  IN OUT ARM_SMC_ARGS * Args,
  IN     INT32        Conduit,
  OUT    UINT64       pre_smc_regs[REG_COUNT_X5_X17],
  OUT    UINT64       post_smc_regs[REG_COUNT_X5_X17]
  );

/**
  @brief  Make the SMC call using AARCH64 Assembly code

  @param  Argumets to pass to the EL3 firmware
  @param  Conduit  SMC or HVC
  @param  PreSmcRegs  regs x5 to x17 before smc/hvc call
  @param  PostSmcRegs regs x5 to x17 after smc/hvc call

  @return  None
**/
VOID
pal_pfdi_verify_regs(ARM_SMC_ARGS *ArmSmcArgs, INT32 Conduit,
                     UINT64 PreSmcRegs[REG_COUNT_X5_X17],
                     UINT64 PostSmcRegs[REG_COUNT_X5_X17])
{

  if (ArmSmcArgs == NULL) {
    return;
  }

  AcsPfdiTest (ArmSmcArgs, Conduit, PreSmcRegs, PostSmcRegs);
}

