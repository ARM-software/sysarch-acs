#/** @file
# Copyright (c) 2016-2020,2021,2024-2026, Arm Limited or its affiliates. All rights reserved.
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

//
// Private worker functions for ASM_PFX()
//
#define _CONCATENATE(a, b)  __CONCATENATE(a, b)
#define __CONCATENATE(a, b) a ## b

#define __USER_LABEL_PREFIX__
//
// The __USER_LABEL_PREFIX__ macro predefined by GNUC represents the prefix
// on symbols in assembly language.
//
#define ASM_PFX(name) _CONCATENATE (__USER_LABEL_PREFIX__, name)

#define GCC_ASM_EXPORT(func__)  \
       .global  _CONCATENATE (__USER_LABEL_PREFIX__, func__)    ;\
       .type ASM_PFX(func__), %function

#define GCC_ASM_IMPORT(func__)  \
       .extern  _CONCATENATE (__USER_LABEL_PREFIX__, func__)


.text
.align 3


GCC_ASM_IMPORT (exception_return_regs)
GCC_ASM_IMPORT (exception_return_regs_valid)

GCC_ASM_EXPORT (AA64ReadSp)
GCC_ASM_EXPORT (AA64SaveExceptionReturnContext)
GCC_ASM_EXPORT (AA64WriteSp)
GCC_ASM_EXPORT (ArmRdvl)
GCC_ASM_EXPORT (AA64SetupTraceAccess)
GCC_ASM_EXPORT (AA64EnableETETrace)
GCC_ASM_EXPORT (AA64EnableTRBUTrace)
GCC_ASM_EXPORT (AA64GenerateETETrace)
GCC_ASM_EXPORT (AA64DisableTRBUTrace)
GCC_ASM_EXPORT (AA64DisableETETrace)
GCC_ASM_EXPORT (AA64EnableTFO)
GCC_ASM_EXPORT (AA64DisableTFO)

ASM_PFX(AA64ReadSp):
  mov   x0, sp
  ret

ASM_PFX(AA64SaveExceptionReturnContext):
  stp   x0, x1, [sp, #-16]!
  adrp  x0, exception_return_regs
  add   x0, x0, :lo12:exception_return_regs
  ldr   x1, [sp]
  str   x1, [x0]
  ldr   x1, [sp, #8]
  str   x1, [x0, #8]
  add   sp, sp, #16
  stp   x2, x3, [x0, #8 * 2]
  stp   x4, x5, [x0, #8 * 4]
  stp   x6, x7, [x0, #8 * 6]
  stp   x8, x9, [x0, #8 * 8]
  stp   x10, x11, [x0, #8 * 10]
  stp   x12, x13, [x0, #8 * 12]
  stp   x14, x15, [x0, #8 * 14]
  stp   x16, x17, [x0, #8 * 16]
  stp   x18, x19, [x0, #8 * 18]
  stp   x20, x21, [x0, #8 * 20]
  stp   x22, x23, [x0, #8 * 22]
  stp   x24, x25, [x0, #8 * 24]
  stp   x26, x27, [x0, #8 * 26]
  stp   x28, x29, [x0, #8 * 28]
  str   x30, [x0, #8 * 30]
  mov   x1, sp
  str   x1, [x0, #8 * 31]
  adrp  x0, exception_return_regs_valid
  add   x0, x0, :lo12:exception_return_regs_valid
  mov   x2, #1
  str   x2, [x0]
  mov   x0, x1
  ret

ASM_PFX(AA64WriteSp):
  mov   sp, x0
  ret

ASM_PFX(ArmRdvl):
  //RDVL   x0, #8   // TODO once instruction supports Read Vector Length
  .inst 0x04BF5100
  ret

ASM_PFX(AA64SetupTraceAccess):
  // Allows TRBU reg writes from EL1 and EL2
  // Programming of MDCR_EL3.NSTB = 0b11 is done in arm-tf
  mrs x1, MDCR_EL2
  bic x1, x1, #(3<<24) // MDCR_EL2.E2TB = 0b00 for EL2 tracing
  msr MDCR_EL2, x1
  isb
  msr OSLAR_EL1, xzr
  isb
  ret

ASM_PFX(AA64EnableTFO):
  /* Set OSLK */
  mov x1, 0x1
  msr OSLAR_EL1, x1
  isb
  mrs x1, MDSCR_EL1
  orr x1, x1, #(1<<31)
  msr MDSCR_EL1, x1
  isb
  mrs x1, TRBMAR_EL1
  orr x1, x1, #(1<<10)
  msr TRBMAR_EL1, x1
  isb
  ret

ASM_PFX(AA64DisableTFO):
  /* Set OSLK */
  mov x1, 0x1
  msr OSLAR_EL1, x1
  isb
  mrs x1, MDSCR_EL1
  bic x1, x1, #(1<<31) // MDSCR_EL1.TFO=0b0
  msr MDSCR_EL1, x1
  isb
  ret

ASM_PFX(AA64EnableETETrace):
  /* Set TRCRSR.TA as active */
  mov x0, #(1<<12)
  MSR TRCRSR, x0
  isb

  /* Enable Trace Unit */
  mov x0, #(1<<0)
  msr TRCPRGCTLR, x0
  isb
  ret

ASM_PFX(AA64EnableTRBUTrace):
    /* Add an offset for different PE */
    mov x0, x0, lsl #12   // x0 has Current PE Index
    add x1, x1, x0        // Updated Base Address for Curent PE

    /* Update TRBBASER_EL1, TRBPTR_EL1 */
    msr TRBBASER_EL1, x1
    isb
    msr TRBPTR_EL1, x1
    isb

    /* Program TRBLIMITR_EL1 */
    add x1, x1, #0x1000  // Set 4KB trace buffer limit for Current PE
    add x1, x1, x2
    orr x1, x1, #(3<<1)  // TRBLIMITR_EL1.FM_CIRCULAR_BUFFER_MODE
    orr x1, x1, #(3<<3)  // TRBLIMITR_EL1_TM_IGNORE_TRIGGER
    msr TRBLIMITR_EL1, x1
    isb

    /* Update TRFCR_EL2 to enable trace at EL2 */
    mrs x1, S3_4_C1_C2_1
    orr x1, x1, #(1<<1) // TRFCR_EL2.E2TRE = 0b1
    orr x1, x1, #(1<<3) // TRFCR_EL2.CX = 0b1
    orr x1, x1, #(3<<5) // TRFCR_EL2.TS = 0b11
    msr S3_4_C1_C2_1, x1
    isb

    /* TRCCONFIGR.TS = 1 */
    mrs x1, trcconfigr
    orr x1, x1, #(1<<11)
    msr trcconfigr, x1
    isb

    /* Set TRBU as Running, TRBSR_EL1.S = 0 */
    msr TRBSR_EL1, xzr
    isb

    /* Set TRCVICTLR.SSSTATUS = 1 (started), all exception levels enabled */
    mov x1, 0x201
    msr TRCVICTLR, x1
    isb
    ret

ASM_PFX(AA64DisableTRBUTrace):
    /* Flush trace unit before disabling TRBU */
    tsb CSYNC

    /* TRBSR_EL1.S Set TRBU as Stopped */
    mov x0, 0x20000
    msr TRBSR_EL1, x0
    isb

    /* TRBLIMITR_EL1 Disable TRBU */
    mrs x1, TRBLIMITR_EL1
    bic x1, x1, #(1<<0) // TRBLIMITR_EL1.E = 0b0
    msr TRBLIMITR_EL1, x1
    isb
    ret

ASM_PFX(AA64GenerateETETrace):
  /* Generate trace for sample instructions */
  MOV x0, #1000
  MOV x2, x0
  CMP x2, xzr
  B branch
  return:
    ret

  branch:
    SUB x0, x0, #1
    CMP x0, xzr
    BNE branch
    B return


ASM_PFX(AA64DisableETETrace):
  /* Disable trace */
  ISB
  TSB CSYNC
  ISB

  MSR TRCPRGCTLR, xzr
  ISB

  B branch1
  return1:
    ret

  branch1:
    MOV x2, x0
    CMP x2, xzr
    B return1

// MMU register write functions for secondary PE initialization

GCC_ASM_EXPORT (AA64WriteMair1)
GCC_ASM_EXPORT (AA64WriteMair2)
GCC_ASM_EXPORT (AA64WriteTcr1)
GCC_ASM_EXPORT (AA64WriteTcr2)
GCC_ASM_EXPORT (AA64WriteTtbr0El1)
GCC_ASM_EXPORT (AA64WriteTtbr0El2)
GCC_ASM_EXPORT (AA64WriteTtbr1El1)
GCC_ASM_EXPORT (AA64WriteTtbr1El2)
GCC_ASM_EXPORT (AA64WriteSctlr1)
GCC_ASM_EXPORT (AA64WriteSctlr2)
GCC_ASM_EXPORT (AA64TlbiVmalle1)
GCC_ASM_EXPORT (AA64TlbiAlle2)

ASM_PFX(AA64WriteMair1):
  msr   mair_el1, x0           // write EL1 MAIR
  isb
  ret

ASM_PFX(AA64WriteMair2):
  msr   mair_el2, x0           // write EL2 MAIR
  isb
  ret

ASM_PFX(AA64WriteTcr1):
  msr   tcr_el1, x0            // write EL1 TCR
  isb
  ret

ASM_PFX(AA64WriteTcr2):
  msr   tcr_el2, x0            // write EL2 TCR
  isb
  ret

ASM_PFX(AA64WriteTtbr0El1):
  msr   ttbr0_el1, x0          // write EL1 TTBR0
  isb
  ret

ASM_PFX(AA64WriteTtbr0El2):
  msr   ttbr0_el2, x0          // write EL2 TTBR0
  isb
  ret

ASM_PFX(AA64WriteTtbr1El1):
  msr   ttbr1_el1, x0          // write EL1 TTBR1
  isb
  ret

ASM_PFX(AA64WriteTtbr1El2):
  msr   ttbr1_el2, x0          // write EL2 TTBR1
  isb
  ret

ASM_PFX(AA64WriteSctlr1):
  msr   sctlr_el1, x0          // write EL1 SCTLR
  isb
  ret

ASM_PFX(AA64WriteSctlr2):
  msr   sctlr_el2, x0          // write EL2 SCTLR
  isb
  ret

ASM_PFX(AA64TlbiVmalle1):
  tlbi  vmalle1                // Invalidate all EL1 TLB entries
  dsb   sy
  isb
  ret

ASM_PFX(AA64TlbiAlle2):
  tlbi  alle2                  // Invalidate all EL2 TLB entries
  dsb   sy
  isb
  ret
