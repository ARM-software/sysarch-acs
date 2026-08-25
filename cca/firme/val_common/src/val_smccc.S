/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text, "ax"
    .align 3

/*
 * void val_smccc_smc_call(uint64_t fid,
 *                     uint64_t arg1, uint64_t arg2, uint64_t arg3,
 *                     uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
 *                     smccc_res_t *res);
 *
 * Notes:
 * - The 9th argument (res) is passed on the stack per AAPCS64.
 * - We preserve x19-x30 around the SMC so callers can safely keep live values
 *   in callee-saved registers across the call (robust vs. EL3 clobbering).
 */
    func val_smccc_smc_call
    /* Save original SP to fetch stack-passed 'res' pointer. */
    mov x9, sp
    ldr x10, [x9]

    /* Save callee-saved registers. */
    stp x29, x30, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x19, x20, [sp, #-16]!

    /* Save 'res' pointer across the SMC (x10 is caller-saved). */
    sub sp, sp, #16
    str x10, [sp]

    /* The helper accepts x0-x7 only; higher FIRME input registers are SBZ. */
    mov x8, xzr
    mov x9, xzr
    mov x10, xzr
    mov x11, xzr
    mov x12, xzr
    mov x13, xzr
    mov x14, xzr
    mov x15, xzr
    mov x16, xzr
    mov x17, xzr

    /* Capture the final input frame and bypass the SMC while auditing callers. */
    adrp x16, g_val_smccc_input_capture_enabled
    ldr x17, [x16, :lo12:g_val_smccc_input_capture_enabled]
    cbz x17, 2f

    adrp x16, g_val_smccc_input_capture_registers
    add x16, x16, :lo12:g_val_smccc_input_capture_registers
    stp x0, x1, [x16, #0x00]
    stp x2, x3, [x16, #0x10]
    stp x4, x5, [x16, #0x20]
    stp x6, x7, [x16, #0x30]
    stp xzr, xzr, [x16, #0x40]
    stp xzr, xzr, [x16, #0x50]
    stp xzr, xzr, [x16, #0x60]
    stp xzr, xzr, [x16, #0x70]
    stp xzr, xzr, [x16, #0x80]

    adrp x16, g_val_smccc_input_capture_valid
    mov x17, #1
    str x17, [x16, :lo12:g_val_smccc_input_capture_valid]
    mov x0, xzr
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    b 3f
2:
    mov x16, xzr
    mov x17, xzr
    smc #0
3:

    /*
     * SMCCC return path:
     * - Control returns here after EL3 services the SMC (or forwards it).
     * - Per SMCCC, the return values are in x0-x3 (and some implementations
     *   also return additional information in x4-x7).
     * - Reload the caller-provided result pointer and store x0-x7 into it.
     *
     * Note:
     * - Registers defined as results may change. We therefore
     *   keep the `res` pointer in memory (on our stack) across `smc #0`
     *   instead of relying on it staying live in a register.
     */
    ldr x10, [sp]
    add sp, sp, #16

    cbz x10, 1f
    str x0, [x10, #0x00]
    str x1, [x10, #0x08]
    str x2, [x10, #0x10]
    str x3, [x10, #0x18]
    str x4, [x10, #0x20]
    str x5, [x10, #0x28]
    str x6, [x10, #0x30]
    str x7, [x10, #0x38]
1:
    /* Restore callee-saved registers. */
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x29, x30, [sp], #16

    ret
    endfunc val_smccc_smc_call

/*
 * void val_smccc_smc_call_x8(uint64_t fid,
 *                     uint64_t arg1, uint64_t arg2, uint64_t arg3,
 *                     uint64_t arg4, uint64_t arg5, uint64_t arg6, uint64_t arg7,
 *                     uint64_t arg8, smccc_res_t *res);
 *
 * Notes:
 * - The 9th argument (arg8) and 10th argument (res) are stack-passed by
 *   AAPCS64. The helper loads arg8 into x8 before issuing SMC.
 * - This is used by FIRME_IDE_KEYSET_PROG, whose Handle parameter is x8.
 */
    func val_smccc_smc_call_x8
    /* Save original SP to fetch stack-passed 'arg8' and 'res'. */
    mov x9, sp
    ldr x8, [x9]
    ldr x10, [x9, #8]

    /* Save callee-saved registers. */
    stp x29, x30, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x19, x20, [sp, #-16]!

    /* Save 'res' pointer across the SMC because x10 is caller-saved. */
    sub sp, sp, #16
    str x10, [sp]

    /* Keep the IDE handle in x8; all remaining input registers are SBZ. */
    mov x9, xzr
    mov x10, xzr
    mov x11, xzr
    mov x12, xzr
    mov x13, xzr
    mov x14, xzr
    mov x15, xzr
    mov x16, xzr
    mov x17, xzr

    /* Capture the x0-x8 input frame and bypass the audited SMC. */
    adrp x16, g_val_smccc_input_capture_enabled
    ldr x17, [x16, :lo12:g_val_smccc_input_capture_enabled]
    cbz x17, 2f

    adrp x16, g_val_smccc_input_capture_registers
    add x16, x16, :lo12:g_val_smccc_input_capture_registers
    stp x0, x1, [x16, #0x00]
    stp x2, x3, [x16, #0x10]
    stp x4, x5, [x16, #0x20]
    stp x6, x7, [x16, #0x30]
    str x8, [x16, #0x40]
    str xzr, [x16, #0x48]
    stp xzr, xzr, [x16, #0x50]
    stp xzr, xzr, [x16, #0x60]
    stp xzr, xzr, [x16, #0x70]
    stp xzr, xzr, [x16, #0x80]

    adrp x16, g_val_smccc_input_capture_valid
    mov x17, #1
    str x17, [x16, :lo12:g_val_smccc_input_capture_valid]
    mov x0, xzr
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    b 3f
2:
    mov x16, xzr
    mov x17, xzr
    smc #0
3:

    /*
     * SMCCC return path:
     * - Control returns here after EL3 handles the SMC.
     * - Reload the caller-provided result pointer and store x0-x7 into it.
     */
    ldr x10, [sp]
    add sp, sp, #16

    cbz x10, 1f
    str x0, [x10, #0x00]
    str x1, [x10, #0x08]
    str x2, [x10, #0x10]
    str x3, [x10, #0x18]
    str x4, [x10, #0x20]
    str x5, [x10, #0x28]
    str x6, [x10, #0x30]
    str x7, [x10, #0x38]
1:
    /* Restore callee-saved registers. */
    ldp x19, x20, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x29, x30, [sp], #16

    ret
    endfunc val_smccc_smc_call_x8
