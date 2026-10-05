/** @file
 * Copyright (c) 2021,2024-2026, Arm Limited or its affiliates. All rights reserved.
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

#define EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS    0
#define EXCEPT_AARCH64_IRQ                       1
#define EXCEPT_AARCH64_FIQ                       2
#define EXCEPT_AARCH64_SERROR                    3

#define EXCEPTION_RETURN_SP_ALIGN_MASK           0xf
#define EXCEPTION_RETURN_SP_MAX_DELTA            0x100000

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

#define GCC_ASM_IMPORT(func__)  \
       .extern  _CONCATENATE (__USER_LABEL_PREFIX__, func__)

GCC_ASM_IMPORT(common_exception_handler)
GCC_ASM_EXPORT(bsa_gic_set_el2_vector_table)
GCC_ASM_EXPORT(bsa_gic_update_elr)
GCC_ASM_EXPORT(bsa_gic_set_exception_return_sp)
GCC_ASM_EXPORT(bsa_gic_get_far)
GCC_ASM_EXPORT(bsa_gic_get_esr)
GCC_ASM_EXPORT(bsa_gic_get_elr)


#define CHECK_EL_1_OR_2_OR_3(REG) \
        mrs    REG, CurrentEL ;\
        cmp    REG, #0x8      ;\
        b.gt   3f             ;\
        b.eq   2f             ;\
        cbnz   REG, 1f        ;\
        b      .              ;// We should never get here

/**
 * This is a generic handler for exceptions taken at the current EL while using
 * SPx. It saves volatile registers, calls the C handler, restores volatile
 * registers, then returns.
 *
 * Saving state and jumping to C handler takes 15 instructions. We add an extra
 * branch to a common exit path. So each handler takes up one unique cache line
 * and one shared cache line (assuming 16-byte cache lines).
 */
.macro current_exception_spx elx:req handler:req
    save_volatile_to_stack \elx
    bl \handler
    b restore_from_stack_and_return
.endm

.macro current_el_with_spx elx:req val:req
    save_volatile_to_stack \elx
    mov x0, #\val
    bl  ASM_PFX(common_exception_handler)
    b restore_from_stack_and_return
.endm

/**
 * Saves the volatile registers onto the stack. This currently takes 14
 * instructions, so it can be used in exception handlers with 18 instructions
 * left, 2 of which in the same cache line (assuming a 16-byte cache line).
 *
 * On return, x0 and x1 are initialised to elr_el2 and spsr_el2 respectively,
 * which can be used as the first and second arguments of a subsequent call.
 */
.macro save_volatile_to_stack elx:req
    /* Reserve stack space and save registers x0-x18, x29 & x30. */
    stp x0, x1, [sp, #-(8 * 24)]!
    stp x2, x3, [sp, #8 * 2]
    stp x4, x5, [sp, #8 * 4]
    stp x6, x7, [sp, #8 * 6]
    stp x8, x9, [sp, #8 * 8]
    stp x10, x11, [sp, #8 * 10]
    stp x12, x13, [sp, #8 * 12]
    stp x14, x15, [sp, #8 * 14]
    stp x16, x17, [sp, #8 * 16]
    str x18, [sp, #8 * 18]
    stp x29, x30, [sp, #8 * 20]

    /*
     * Save elr_elx & spsr_elx. This such that we can take nested exception
     * and still be able to unwind.
     */
    CHECK_EL_1_OR_2_OR_3(x0)
1:  mrs   x0, elr_el1
    mrs   x1, spsr_el1
    b     4f
2:  mrs   x0, elr_el2
    mrs   x1, spsr_el2
    b     4f
3:  mrs   x0, elr_el3
    mrs   x1, spsr_el3
4:  stp x0, x1, [sp, #8 * 22]
.endm

  .section .text.vtable, "ax"
  .align 12

.global vector_table
vector_table:

// ------------------------------------------------------------
// Current EL with SP0
// ------------------------------------------------------------
  .balign 128
sync_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
fiq_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
serror_current_el_sp0:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

// ------------------------------------------------------------
// Current EL with SPx
// ------------------------------------------------------------

  .balign 128
sync_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_IRQ

  .balign 128
fiq_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_FIQ

  .balign 128
serror_current_el_spx:
  current_el_with_spx el2 EXCEPT_AARCH64_SERROR

// ------------------------------------------------------------
// Lower EL using AArch64
// ------------------------------------------------------------

  .balign 128
sync_lower_el_aarch64:
  B        .                   //       Synchronous exception
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_lower_el_aarch64:
  B        .                    //        IRQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
fiq_lower_el_aarch64:
  B        .                    //        FIQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
serror_lower_el_aarch64:
  B        .                    //        SError
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

// ------------------------------------------------------------
// Lower EL using AArch32
// ------------------------------------------------------------

  .balign 128
sync_lower_el_aarch32:
  B        .
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
irq_lower_el_aarch32:
  B        .                    //        IRQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
fiq_lower_el_aarch32:
  B        .                    //        FIQ
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  .balign 128
serror_lower_el_aarch32:
  B        .                    //        SError
  //current_el_with_spx el2 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

.balign 0x40
/**
 * Restores the volatile registers from the stack.

 * Register x0: if false restores elr_el2, if true retains the value of elr_el2.
 * This enables exception handlers to indicate whether they have changed the
 * value of elr_el2 (e.g., to skip the faulting instruction).
 */
restore_from_stack_and_return:
    /* Restore registers x2-x18, x29 & x30. */
    ldp x2, x3, [sp, #8 * 2]
    ldp x4, x5, [sp, #8 * 4]
    ldp x6, x7, [sp, #8 * 6]
    ldp x8, x9, [sp, #8 * 8]
    ldp x10, x11, [sp, #8 * 10]
    ldp x12, x13, [sp, #8 * 12]
    ldp x14, x15, [sp, #8 * 14]
    ldp x16, x17, [sp, #8 * 16]
    ldr x18, [sp, #8 * 18]
    ldp x29, x30, [sp, #8 * 20]

    cbnz x0, skip_elr

    /* Restore register elr_el2 using x1 as scratch. */
    ldr x1, [sp, #8 * 22]
    CHECK_EL_1_OR_2_OR_3(x0)
1:  msr   elr_el1, x1
    b     4f
2:  msr   elr_el2, x1
    b     4f
3:  msr   elr_el3, x1
4:  isb
    mov x16, xzr
    b restore_spsr

skip_elr:
    mov x16, #1

restore_spsr:
    /* Restore register spsr_el2 using x1 as scratch. */
    ldr x1, [sp, #8 * 23]
    CHECK_EL_1_OR_2_OR_3(x0)
1:  msr   spsr_el1, x1
    b     4f
2:  msr   spsr_el2, x1
    b     4f
3:  msr   spsr_el3, x1
4:  isb

    /* Restore x0 & x1, and release stack space. */
    cbz x16, no_sp_update
    adrp x0, exception_return_sp
    add x0, x0, :lo12:exception_return_sp
    ldr x16, [x0]
    cbz x16, no_sp_update
    str xzr, [x0]

    tst x16, #EXCEPTION_RETURN_SP_ALIGN_MASK
    b.ne no_sp_update
    add x17, sp, #8 * 24
    cmp x16, x17
    b.lo no_sp_update
    sub x17, x16, x17
    mov x0, #EXCEPTION_RETURN_SP_MAX_DELTA
    cmp x17, x0
    b.hi no_sp_update

    adrp x17, exception_return_regs_valid
    add x17, x17, :lo12:exception_return_regs_valid
    ldr x0, [x17]
    cbz x0, no_sp_update
    str xzr, [x17]

    mov sp, x16
    adrp x18, exception_return_regs
    add x18, x18, :lo12:exception_return_regs
    ldp x2, x3, [x18, #8 * 2]
    ldp x4, x5, [x18, #8 * 4]
    ldp x6, x7, [x18, #8 * 6]
    ldp x8, x9, [x18, #8 * 8]
    ldp x10, x11, [x18, #8 * 10]
    ldp x12, x13, [x18, #8 * 12]
    ldp x14, x15, [x18, #8 * 14]
    ldp x16, x17, [x18, #8 * 16]
    ldr x19, [x18, #8 * 19]
    ldp x20, x21, [x18, #8 * 20]
    ldp x22, x23, [x18, #8 * 22]
    ldp x24, x25, [x18, #8 * 24]
    ldp x26, x27, [x18, #8 * 26]
    ldp x28, x29, [x18, #8 * 28]
    ldr x30, [x18, #8 * 30]
    ldp x0, x1, [x18]
    ldr x18, [x18, #8 * 18]
    eret

no_sp_update:
    ldp x16, x17, [sp, #8 * 16]
    ldp x0, x1, [sp], #8 * 24

    eret


// ------------------------------------------------------------
// Set Vector Table
// ------------------------------------------------------------

ASM_PFX(bsa_gic_set_el2_vector_table):
    adr x0, vector_table
    CHECK_EL_1_OR_2_OR_3(x1)
1:  msr   vbar_el1, x0
    b     4f
2:  msr   vbar_el2, x0
    b     4f
3:  msr   vbar_el3, x0
4:  isb
    ret

ASM_PFX(bsa_gic_update_elr):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  msr   elr_el1, x0
    b     4f
2:  msr   elr_el2, x0
    b     4f
3:  msr   elr_el3, x0
4:  isb
    ret

ASM_PFX(bsa_gic_set_exception_return_sp):
    adrp x1, exception_return_sp
    add x1, x1, :lo12:exception_return_sp

    cbz x0, 1f
    tst x0, #EXCEPTION_RETURN_SP_ALIGN_MASK
    b.ne 2f
    mov x2, sp
    cmp x0, x2
    b.lo 2f
    sub x2, x0, x2
    mov x3, #EXCEPTION_RETURN_SP_MAX_DELTA
    cmp x2, x3
    b.hi 2f

1:  str x0, [x1]
    ret
2:  str xzr, [x1]
    ret

ASM_PFX(bsa_gic_get_elr):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  mrs   x0, elr_el1
    b     4f
2:  mrs   x0, elr_el2
    b     4f
3:  mrs   x0, elr_el3
4:  ret

ASM_PFX(bsa_gic_get_esr):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  mrs   x0, esr_el1
    b     4f
2:  mrs   x0, esr_el2
    b     4f
3:  mrs   x0, esr_el3
4:  ret

ASM_PFX(bsa_gic_get_far):
    CHECK_EL_1_OR_2_OR_3(x1)
1:  mrs   x0, far_el1
    b     4f
2:  mrs   x0, far_el2
    b     4f
3:  mrs   x0, far_el3
4:  ret

.section .data, "aw"
.balign 8
.global exception_return_sp
exception_return_sp:
    .quad 0
.global exception_return_regs_valid
exception_return_regs_valid:
    .quad 0
.global exception_return_regs
exception_return_regs:
    .skip 8 * 32
