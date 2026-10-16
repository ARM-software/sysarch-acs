/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"
#include "val_common/val_status.h"

/**
 * @brief Temporarily install S-EL2 tables for a single NS-output load.
 * @param x0 Private root table PA, already published by DSB.
 * @param x1 Temporary TCR_EL2 with 4-KiB granules and a 39-bit VA space.
 * @param x2 NS alias VA; x3 and x4 are mapped Secure result/fault pointers.
 * @return Probe status after restoring translation registers and DAIF exactly.
 */
func val_mmu_ns_read_scope_el2
    mrs x5, CurrentEL
    cmp x5, #8
    b.ne 2f
    mrs x5, SCTLR_EL2
    tbnz x5, #0, 2f
    tbnz x5, #2, 2f
    mrs x5, HCR_EL2
    tbnz x5, #34, 2f
    stp x29, x30, [sp, #-64]!
    mov x29, sp
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    mrs x19, SCTLR_EL2
    mrs x20, TCR_EL2
    mrs x21, TTBR0_EL2
    mrs x22, MAIR_EL2
    mrs x23, DAIF
    msr DAIFSet, #15
    .inst 0xd5033f9f
    tlbi alle2
    .inst 0xd5033f9f
    mov x5, #0x44
    msr MAIR_EL2, x5
    msr TCR_EL2, x1
    msr TTBR0_EL2, x0
    .inst 0xd5033fdf
    orr x5, x19, #1
    msr SCTLR_EL2, x5
    .inst 0xd5033fdf
    mov x0, x2
    mov x1, x3
    mov x2, x4
    bl val_probe_read64
    mov w24, w0
    msr DAIFSet, #15
    .inst 0xd5033f9f
    msr SCTLR_EL2, x19
    .inst 0xd5033fdf
    tlbi alle2
    .inst 0xd5033f9f
    msr TTBR0_EL2, x21
    msr TCR_EL2, x20
    msr MAIR_EL2, x22
    .inst 0xd5033fdf
    msr DAIF, x23
    .inst 0xd5033fdf
    mov w0, w24
    ldp x23, x24, [sp, #48]
    ldp x21, x22, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #64
    ret
2:
    mov w0, #VAL_ERROR
    ret
endfunc val_mmu_ns_read_scope_el2

/**
 * @brief Switch only EL2 translation context for an owned native-PAS page load.
 * @param x0 Private root; x1 TCR; x2 alias; x3 value; x4 fault; x5 expected boot root.
 * @return Probe status after restoring TTBR, TCR, MAIR, local TLB context and DAIF.
 * @details Never toggles SCTLR.M/C, so cached stack frames keep their memory attributes.
 */
func val_mmu_native_read_scope_el2
    mrs x6, TTBR0_EL2
    cmp x6, x5
    b.ne 2f
    mrs x6, HCR_EL2
    tbnz x6, #34, 2f
    stp x29, x30, [sp, #-64]!
    mov x29, sp
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    str x23, [sp, #48]
    mrs x19, TCR_EL2
    mrs x20, TTBR0_EL2
    mrs x21, MAIR_EL2
    mrs x22, DAIF
    msr DAIFSet, #15
    .inst 0xd5033f9f
    mov x6, #0x44
    mov x7, x21
    bfi x7, x6, #56, #8
    msr MAIR_EL2, x7
    msr TCR_EL2, x1
    msr TTBR0_EL2, x0
    .inst 0xd5033fdf
    tlbi alle2
    .inst 0xd5033f9f
    .inst 0xd5033fdf
    mov x0, x2
    mov x1, x3
    mov x2, x4
    bl val_probe_read64
    mov w23, w0
    msr DAIFSet, #15
    .inst 0xd5033f9f
    msr TTBR0_EL2, x20
    msr TCR_EL2, x19
    msr MAIR_EL2, x21
    .inst 0xd5033fdf
    tlbi alle2
    .inst 0xd5033f9f
    .inst 0xd5033fdf
    msr DAIF, x22
    .inst 0xd5033fdf
    mov w0, w23
    ldr x23, [sp, #48]
    ldp x21, x22, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #64
    ret
2:
    mov w0, #VAL_ERROR
    ret
endfunc val_mmu_native_read_scope_el2
