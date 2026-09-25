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
    .extern spmc_handle_smc
    .extern g_spmc_cold_boot

func secure_entry
    /* Ensure we use SP_EL2 (SPx) so EL2 exceptions hit the SPx vector table. */
    /*
     * Do not clobber incoming x0-x7: this entrypoint is invoked with
     * direct-messaging arguments in x0-x7.
     */
    mov x9, #1
    msr spsel, x9
    /* ISB (avoid recursive macro from asm_macros.S). */
    .inst 0xd5033fdf
    /* Avoid hard hangs on unexpected Group0/FIQ routing during bring-up. */
    msr daifset, #1

    /* Set up stack without clobbering incoming x0-x7 (SMC args). */
    val_set_pe_stack

    /* Save incoming x0-x7 as smc_args_t at SP. */
    sub sp, sp, #0x40
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]

    /* Clear BSS exactly once (guarded by a .data flag). */
    ldr x9, =g_spmc_cold_boot
    ldr w10, [x9]
    cbz w10, 0f
    mov w10, #0
    str w10, [x9]

    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    bl  val_zero_bss
0:

    mov x0, sp
    bl  spmc_handle_smc

    ldp x0, x1, [sp, #0x00]
    ldp x2, x3, [sp, #0x10]
    ldp x4, x5, [sp, #0x20]
    ldp x6, x7, [sp, #0x30]
    add sp, sp, #0x40

    /* Return to EL3/NS via SMC with x0-x7 set. */
    smc #0

    /*
     * SPMC "main loop":
     * - The SMC above does not behave like a normal function call. It exits
     *   this payload to EL3 (SPMD/BL31).
     * - When EL3 routes the next Secure-world direct message/SMC to this
     *   payload, execution starts again at the entry point.
     * - We branch back to reuse the same common prologue and handle the next
     *   request in a stable way.
     */
    b secure_entry
endfunc secure_entry
