#/** @file
# Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
# SPDX-License-Identifier : Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#**/

.text
.align 3

GCC_ASM_IMPORT(gPsciConduit)
.equ CONDUIT_SMC,        0
.equ CONDUIT_HVC,        1
.equ INVALID_PARAMETER, -3

GCC_ASM_EXPORT(AcsPfdiTest)

ASM_PFX(AcsPfdiTest):
  //save x19-x23 and x30 in stack
  stp x19, x20, [sp, #-48]!
  stp x21, x22, [sp, #16]
  stp x23, x30, [sp, #32]

  //save input params in x19-x23
  mov x19, x0
  mov x20, x1
  mov x21, x2
  mov x22, x3

  //Load SMC arguments into x0-x7 registers
  ldp   x6, x7, [x19, #48]
  ldp   x4, x5, [x19, #32]
  ldp   x2, x3, [x19, #16]
  ldp   x0, x1, [x19, #0]

  //Before sending SMC call, save x5-x17 registers
  stp  x5,  x6,  [x21], #16
  stp  x7,  x8,  [x21], #16
  stp  x9,  x10, [x21], #16
  stp  x11, x12, [x21], #16
  stp  x13, x14, [x21], #16
  stp  x15, x16, [x21], #16
  str  x17, [x21]

  // Check whether the conduit is SMC or HVC
  cmp   x20, CONDUIT_SMC
  beq   conduit_smc
  cmp   x20, CONDUIT_HVC
  beq   conduit_hvc

conduit_invalid:
  mov   x0, INVALID_PARAMETER
  b     finish

conduit_smc:
  smc   #0
  b     finish

conduit_hvc:
  hvc   #0
  b     finish

finish:
  //After sending SMC call, save x5-x17 registers
  stp  x5,  x6,  [x22], #16
  stp  x7,  x8,  [x22], #16
  stp  x9,  x10, [x22], #16
  stp  x11, x12, [x22], #16
  stp  x13, x14, [x22], #16
  stp  x15, x16, [x22], #16
  str  x17, [x21]


  // Store the SMC returned values x0-x4 into the ARM_SMC_ARGS structure.
  stp   x0, x1, [x19, #0]
  stp   x2, x3, [x19, #16]
  str   x4, [x19, #32]

  // Pop x19-x23 and x30 registers from stack
  ldp   x23, x30, [sp, #32]
  ldp   x21, x22, [sp, #16]
  ldp   x19, x20, [sp], #48

  ret
