#/** @file
# Copyright (c) 2016-2018,2024-2025, Arm Limited or its affiliates. All rights reserved.
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

GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(PalGetSecondaryStackBase)
GCC_ASM_IMPORT(PalGetMaxMpidr)
GCC_ASM_IMPORT(PalGetMmuConfigAddr)
GCC_ASM_EXPORT(ModuleEntryPoint)

StartupAddr:         .8byte ASM_PFX(val_test_entry)
ASM_PFX(StackSize):  .8byte 0x100

// PE_MMU_CONFIG structure offsets
.equ MMU_CFG_TTBR0,     0
.equ MMU_CFG_TCR,       8
.equ MMU_CFG_MAIR,      16
.equ MMU_CFG_SCTLR,     24
.equ MMU_CFG_EL,        32

// SCTLR bits
.equ SCTLR_M_BIT,       (1 << 0)   // MMU enable
.equ SCTLR_C_BIT,       (1 << 2)   // Data cache enable
.equ SCTLR_I_BIT,       (1 << 12)  // Instruction cache enable

ASM_PFX(ModuleEntryPoint):
  //
  // Enable MMU and caches using primary PE's configuration
  //
_EnableMmu:
  // Get the MMU configuration address from primary PE
  // Note: This is a C function call, but we haven't set up stack yet
  // so we use a special calling convention (just branch and link)
  adr   x9, MmuConfigAddr
  ldr   x9, [x9]
  blr   x9

  // x0 now contains the address of PE_MMU_CONFIG structure
  mov   x11, x0  // Save config address in x11

  // Load all config values from the structure
  ldr   x12, [x11, #MMU_CFG_TTBR0]  // TTBR0
  ldr   x13, [x11, #MMU_CFG_TCR]    // TCR
  ldr   x14, [x11, #MMU_CFG_MAIR]   // MAIR
  ldr   x15, [x11, #MMU_CFG_SCTLR]  // SCTLR
  ldr   w16, [x11, #MMU_CFG_EL]     // Current EL (32-bit)

  // Determine current exception level
  mrs   x17, CurrentEL
  lsr   x17, x17, 2
  and   x17, x17, 0x3

  // Check if we're at EL2
  cmp   x17, #2
  b.eq  _SetupMmuEl2

_SetupMmuEl1:
  // Invalidate TLB for EL1
  tlbi  vmalle1
  dsb   sy
  isb

  // Set MAIR, TCR, TTBR0 for EL1
  msr   mair_el1, x14
  isb
  msr   tcr_el1, x13
  isb
  msr   ttbr0_el1, x12
  isb

  // Enable MMU and caches for EL1
  // Use the same SCTLR value as primary PE (which has M, C, I bits set)
  msr   sctlr_el1, x15
  isb
  b     _MmuEnabled

_SetupMmuEl2:
  // Invalidate TLB for EL2
  tlbi  alle2
  dsb   sy
  isb

  // Set MAIR, TCR, TTBR0 for EL2
  msr   mair_el2, x14
  isb
  msr   tcr_el2, x13
  isb
  msr   ttbr0_el2, x12
  isb

  // Enable MMU and caches for EL2
  // Use the same SCTLR value as primary PE (which has M, C, I bits set)
  msr   sctlr_el2, x15
  isb

_MmuEnabled:
  //
  // MMU and caches are now enabled, proceed with original entry code
  //

  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x10, x0

  // With this function: CorePos = (Aff3 x maxAff2 x maxAff1 x maxAff0) +
  //                                 (Aff2 x maxAff1 x maxAff0) + (Aff1 x maxAff0) + Aff0
  and   x1, x0, 0xFF
  and   x2, x0, 0xFF00
  lsr   x2, x2, 8
  and   x3, x0, 0xFF0000
  lsr   x3, x3, 16
  and   x4, x0, 0xFF00000000
  lsr   x4, x4, 32

  bl    ASM_PFX(PalGetMaxMpidr)
  and   x5, x0, 0xFF
  add   x5, x5, 1
  and   x6, x0, 0xFF00
  lsr   x6, x6, 8
  add   x6, x6, 1
  and   x7, x0, 0xFF0000
  lsr   x7, x7, 16
  add   x7, x7, 1
  and   x8, x0, 0xFF00000000
  lsr   x8, x8, 32
  add   x8, x8, 1    // x8 has maxAff3, which is reserved for future use

  madd  x0, x2, x5, x1
  mul   x5, x5, x6
  madd  x0, x3, x5, x0
  mul   x5, x5, x7
  madd  x0, x4, x5, x0

  ldr   x3, StackSize
  mul   x3, x3, x0
_GetStackBase:
  mov   x0, 0
  //ldr   x4, GetStackMem
  //blr   x4
  //ASM_PFX(pal_allocate_stack)
  bl ASM_PFX(PalGetSecondaryStackBase)
  add   x0, x0, x3
  mov   sp, x0
_PrepareArguments:

  ldr   x4, StartupAddr

  blr   x4

_NeverReturn:
  b _NeverReturn

// Function pointer for PalGetMmuConfigAddr
.align 3
MmuConfigAddr:   .8byte ASM_PFX(PalGetMmuConfigAddr)
