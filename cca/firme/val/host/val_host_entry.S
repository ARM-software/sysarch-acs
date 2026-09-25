/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"
#include "aarch64/val_mp_asm.inc"

    .section .text.entry, "ax"

    .extern __bss_start__
    .extern __bss_end__
    .extern __stack_end__
    .extern val_zero_bss
    .extern nsel2_main

func nsel2_entry
    /* Ensure we use SP_EL2 (SPx) so EL2 exceptions hit the SPx vector table. */
    mov x1, #1
    msr spsel, x1
    /* ISB (avoid recursive macro from asm_macros.S). */
    .inst 0xd5033fdf
    /* Avoid hard hangs on unexpected Group0/FIQ routing during bring-up. */
    msr daifset, #1

    /* Set up stack */
    val_set_pe_stack

    /* Zero BSS */
    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    bl  val_zero_bss

    /* Jump to C */
    bl  nsel2_main

1:  wfe
    b   1b
endfunc nsel2_entry

/** @brief PSCI CPU_ON entry; never clear another PE's BSS or stack. */
func val_host_secondary_entry
    msr daifset, #15
    mov x9, #1
    msr spsel, x9
    .inst 0xd5033fdf
    val_set_pe_stack
    bl val_mmu_attach_secondary_el2
    bl val_host_secondary_main
1:
    wfe
    b 1b
endfunc val_host_secondary_entry
