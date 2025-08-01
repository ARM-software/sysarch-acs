/** @file
 * Copyright (c) 2021,2024-2025, Arm Limited or its affiliates. All rights reserved.
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

#ifndef TARGET_EMULATION
#include "include/acs_pe.h"
#include "driver/gic/acs_exception.h"
#else
#define ICC_IAR1_EL1    S3_0_C12_C12_0
#define ICC_EOIR1_EL1   S3_0_C12_C12_1
#endif


/* Private worker functions for ASM_PFX() */
#define _CONCATENATE(a, b)  __CONCATENATE(a, b)
#define __CONCATENATE(a, b) a ## b

/* The __USER_LABEL_PREFIX__ macro predefined by GNUC represents
   the prefix on symbols in assembly language.*/
#define __USER_LABEL_PREFIX__

#define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)

#define GCC_ASM_EXPORT(func__)  \
       .global  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    ;\
       .type ASM_PFX(func__), %function

GCC_ASM_EXPORT(bsa_gic_ack_intr)
GCC_ASM_EXPORT(bsa_gic_end_intr)


ASM_PFX(bsa_gic_ack_intr):
   mrs x0, ICC_IAR1_EL1
   ret

ASM_PFX(bsa_gic_end_intr):
   msr ICC_EOIR1_EL1, x0
   ret
