/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"
#include "val_common/val_mmu.h"

    .section .text, "ax"

/**
 * @brief Enable existing EL2 tables without touching a cache-off stack.
 * Entry arguments x0-x7 and LR are preserved. Call only from boot assembly,
 * before any C frame is created; cached and uncached stack accesses must not
 * span this transition. The primary has already published the tables to PoC.
 */
func val_mmu_attach_secondary_el2
    .inst 0xd5033f9f
    tlbi alle2
    ic iallu
    mov x9, #VAL_MMU_BOOT_MAIR
    msr mair_el2, x9
    mov x9, #VAL_MMU_EL2_BOOT_TCR
    msr tcr_el2, x9
    ldr x9, =g_val_mmu_l1
    msr ttbr0_el2, x9
    .inst 0xd5033f9f
    .inst 0xd5033fdf
    mrs x9, sctlr_el2
    orr x9, x9, #1
    orr x9, x9, #4
    orr x9, x9, #0x1000
    msr sctlr_el2, x9
    .inst 0xd5033fdf
    ret
endfunc val_mmu_attach_secondary_el2
