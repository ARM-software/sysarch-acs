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
    .extern rel2_platform_init
    .extern rel2_handle_smc
    .extern g_rel2_cold_boot

/* Entry point for TF-A RMMD: runs in Realm EL2. */
func rel2_entry
    /* Ensure we use SP_EL2 (SPx) so EL2 exceptions hit the SPx vector table. */
    mov x1, #1
    msr spsel, x1
    /* ISB (avoid recursive macro from asm_macros.S). */
    .inst 0xd5033fdf
    /* Avoid hard hangs on unexpected Group0/FIQ routing during bring-up. */
    msr daifset, #1

    /* Set up stack */
    val_set_pe_stack

    /* Clear BSS exactly once across cold boot (shared image). */
    ldr x9, =g_rel2_cold_boot
    ldr w10, [x9]
    mov w19, w10
    cbz w10, 1f
    mov w10, #0
    str w10, [x9]

    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    bl  val_zero_bss
    b 0f
1:
    /* Never carry an uncached C stack frame into coherent execution. */
    bl val_mmu_attach_secondary_el2
0:

    /* Per-CPU init (VBAR/MMU/FP). */
    mov w0, w19
    bl  rel2_platform_init

    /* Signal boot complete to EL3 (RMMD). x1=status, x2=activation token. */
    ldr x0, =0xC40001CF /* RMM_BOOT_COMPLETE */
    mov x1, xzr
    mov x2, xzr
    smc #0

    /*
     * The SMC above exits to EL3 (RMMD in BL31). Execution continues here only
     * when EL3 forwards a subsequent call from the Non-secure Host to this
     * Realm payload.
     */
    b rel2_dispatch_loop
endfunc rel2_entry

func rel2_dispatch_loop
    /* Save incoming x0-x7 as smc_args_t at SP. */
    sub sp, sp, #0x40
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]

    mov x0, sp
    bl  rel2_handle_smc

    ldp x0, x1, [sp, #0x00]
    ldp x2, x3, [sp, #0x10]
    ldp x4, x5, [sp, #0x20]
    ldp x6, x7, [sp, #0x30]
    add sp, sp, #0x40

    /* Return to EL3 (RMMD) to forward results. */
    smc #0

    /*
     * Similar to the Secure shim, this SMC exits the Realm payload back to EL3.
     * EL3 resumes execution at this label when the next Host request is
     * forwarded to Realm EL2.
     */
    b rel2_dispatch_loop
endfunc rel2_dispatch_loop
