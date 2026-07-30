/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"
#include "val_firme_smccc_probe.h"

    .purgem isb

    .arch armv9-a+sme
    .section .text, "ax"
    .align 3

/**
 * @brief Save architecturally accessible vector state into a fixed-stride buffer.
 * @param x0 Buffer; x1 state flags.
 * @return No result; caller-saved general registers may change.
 */
func val_smccc_store_vectors
    mov x9, x0
    .irp number,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,\
16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    tbz x1, #0, 1f
    str z\number, [x9]
    b 2f
1:
    str q\number, [x9]
2:
    add x9, x9, #256
    .endr
    tbz x1, #0, 4f
    .irp number,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    str p\number, [x9]
    add x9, x9, #32
    .endr
    tbz x1, #1, 4f
    rdffr p0.b
    str p0, [x9]
    add x9, x0, #VAL_SMCCC_PREDICATE_OFFSET
    ldr p0, [x9]
4:
    add x9, x0, #0x2000
    add x9, x9, #0x220
    mrs x10, fpcr
    mrs x11, fpsr
    stp x10, x11, [x9]
    tbz x1, #2, 6f
    add x9, x9, #32
    rdsvl x10, #1
    mov w12, wzr
5:
    str za[w12, 0], [x9]
    add x9, x9, x10
    add w12, w12, #1
    cmp x12, x10
    b.lo 5b
6:
    ret
endfunc val_smccc_store_vectors

/**
 * @brief Restore vector state from the fixed-stride buffer.
 * @param x0 Buffer; x1 state flags.
 * @return No result; caller-saved general registers may change.
 */
func val_smccc_load_vectors
    mov x9, x0
    .irp number,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,\
16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    tbz x1, #0, 1f
    ldr z\number, [x9]
    b 2f
1:
    ldr q\number, [x9]
2:
    add x9, x9, #256
    .endr
    tbz x1, #0, 4f
    tbz x1, #1, 3f
    add x10, x9, #512
    ldr p0, [x10]
    wrffr p0.b
3:
    .irp number,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    ldr p\number, [x9]
    add x9, x9, #32
    .endr
4:
    add x9, x0, #0x2000
    add x9, x9, #0x220
    ldp x10, x11, [x9]
    msr fpcr, x10
    msr fpsr, x11
    tbz x1, #2, 6f
    add x9, x9, #32
    rdsvl x10, #1
    mov w12, wzr
5:
    ldr za[w12, 0], [x9]
    add x9, x9, x10
    add w12, w12, #1
    cmp x12, x10
    b.lo 5b
6:
    ret
endfunc val_smccc_load_vectors

/**
 * @brief Probe Base SMC preservation without masking returned register corruption.
 * @param x0 FID; x1 state flags; x2 probe context.
 * @return Zero on completion, minus one on a trapped state access.
 */
func val_smccc_probe_state
    sub sp, sp, #512
    stp x19, x20, [sp]
    stp x21, x22, [sp, #16]
    stp x23, x24, [sp, #32]
    stp x25, x26, [sp, #48]
    stp x27, x28, [sp, #64]
    stp x29, x30, [sp, #80]
    str x18, [sp, #184]
    str x2, [sp, #96]
    stp x0, x1, [sp, #104]
    and x9, x1, #(VAL_SMCCC_STATE_SVE | VAL_SMCCC_STATE_FFR)
    str x9, [sp, #120]
    mrs x9, cptr_el2
    mrs x10, cpacr_el1
    stp x9, x10, [sp, #128]
    mrs x9, daif
    str x9, [sp, #144]
    msr daifset, #15
    stp xzr, xzr, [sp, #152]
    adr x0, .Lprobe_fault
    bl val_fault_begin
    cbnz x0, .Lprobe_unarmed

    /* All FIRME world bodies execute at EL2; handle both CPTR_EL2 layouts. */
    mrs x9, hcr_el2
    ldr x10, [sp, #128]
    tbnz x9, #34, .Lprobe_vhe
    bic x10, x10, #(1 << 10)
    bic x10, x10, #(1 << 8)
    bic x10, x10, #(1 << 12)
    b .Lprobe_access
.Lprobe_vhe:
    orr x10, x10, #(3 << 20)
    orr x10, x10, #(3 << 16)
    orr x10, x10, #(3 << 24)
.Lprobe_access:
    msr cptr_el2, x10
    ldr x10, [sp, #136]
    orr x10, x10, #(3 << 20)
    orr x10, x10, #(3 << 16)
    orr x10, x10, #(3 << 24)
    msr cpacr_el1, x10
    isb
    ldr x9, [sp, #112]
    tbz x9, #2, .Lprobe_save
    mrs x10, SVCR
    cbnz x10, .Lprobe_fault
    mrs x10, SMCR_EL2
    str x10, [sp, #176]
    mov x11, #4
    str x11, [sp, #152]
    /* Exercise streaming mode without FA64; non-streaming FFR is checked separately. */
    bic x10, x10, #(1 << 31)
    msr SMCR_EL2, x10
    isb
.Lprobe_save:
    ldr x9, [sp, #96]
    ldr x0, [x9]
    ldr x1, [sp, #120]
    bl val_smccc_store_vectors
    ldr x9, [sp, #152]
    orr x9, x9, #1
    str x9, [sp, #152]
    ldr x9, [sp, #112]
    tbz x9, #2, .Lprobe_seed
    mov x10, #7
    str x10, [sp, #152]
    smstart za
    tbz x9, #3, .Lprobe_seed
    smstart sm
.Lprobe_seed:
    ldr x1, [sp, #112]
    tbz x1, #3, .Lprobe_mode
    orr x1, x1, #VAL_SMCCC_STATE_SVE
    bic x1, x1, #VAL_SMCCC_STATE_FFR
.Lprobe_mode:
    str x1, [sp, #168]
    ldr x9, [sp, #96]
    mov x10, #16
    tbz x1, #0, .Lprobe_length
    ldr x11, [sp, #112]
    tbz x11, #3, .Lprobe_non_streaming_length
    rdsvl x10, #1
    b .Lprobe_length
.Lprobe_non_streaming_length:
    rdvl x10, #1
.Lprobe_length:
    str x10, [x9, #272]
    tbz x1, #2, .Lprobe_no_za_length
    rdsvl x10, #1
    str x10, [x9, #288]
.Lprobe_no_za_length:
    ldr x0, [x9, #16]
    bl val_smccc_load_vectors
    ldr x0, [sp, #104]
    ldr x9, [sp, #112]
    tbz x9, #4, .Lprobe_args
    orr x0, x0, #(1 << 16)
.Lprobe_args:
    /* Load ABI-specific x1-x17 inputs; the C catalogue keeps unused inputs SBZ. */
    ldr x18, [sp, #96]
    ldp x1, x2, [x18, #320]
    ldp x3, x4, [x18, #336]
    ldp x5, x6, [x18, #352]
    ldp x7, x8, [x18, #368]
    ldp x9, x10, [x18, #384]
    ldp x11, x12, [x18, #400]
    ldp x13, x14, [x18, #416]
    ldp x15, x16, [x18, #432]
    ldr x17, [x18, #448]
    mov x18, #18
    mov x19, #19
    mov x20, #20
    mov x21, #21
    mov x22, #22
    mov x23, #23
    mov x24, #24
    mov x25, #25
    mov x26, #26
    mov x27, #27
    mov x28, #28
    mov x29, #29
    mov x30, #30
    smc #0
    /* Capture before using scratch registers or restoring the AAPCS64 frame. */
    stp x0, x1, [sp, #256]
    stp x2, x3, [sp, #272]
    stp x4, x5, [sp, #288]
    stp x6, x7, [sp, #304]
    stp x8, x9, [sp, #320]
    stp x10, x11, [sp, #336]
    stp x12, x13, [sp, #352]
    stp x14, x15, [sp, #368]
    stp x16, x17, [sp, #384]
    stp x18, x19, [sp, #400]
    stp x20, x21, [sp, #416]
    stp x22, x23, [sp, #432]
    stp x24, x25, [sp, #448]
    stp x26, x27, [sp, #464]
    stp x28, x29, [sp, #480]
    str x30, [sp, #496]
    ldr x9, [sp, #96]
    add x9, x9, #24
    add x10, sp, #256
    mov x13, #31
.Lprobe_copy:
    ldr x11, [x10], #8
    str x11, [x9], #8
    subs x13, x13, #1
    b.ne .Lprobe_copy
    ldr x9, [sp, #96]
    ldr x1, [sp, #168]
    mov x10, #16
    tbz x1, #0, .Lprobe_returned_length
    ldr x11, [sp, #112]
    tbz x11, #3, .Lprobe_returned_non_streaming_length
    rdsvl x10, #1
    b .Lprobe_returned_length
.Lprobe_returned_non_streaming_length:
    rdvl x10, #1
.Lprobe_returned_length:
    str x10, [x9, #304]
    tbz x1, #2, .Lprobe_returned_vectors
    mrs x10, SVCR
    str x10, [x9, #296]
.Lprobe_returned_vectors:
    ldr x0, [x9, #8]
    bl val_smccc_store_vectors
    b .Lprobe_cleanup
.Lprobe_fault:
    mov x9, #-1
    str x9, [sp, #160]
.Lprobe_cleanup:
    /* Clear each recovery stage first, so a cleanup trap cannot loop forever. */
    ldr x9, [sp, #152]
    tbz x9, #1, .Lprobe_restore
    bic x9, x9, #2
    str x9, [sp, #152]
    smstop
.Lprobe_restore:
    ldr x9, [sp, #152]
    tbz x9, #0, .Lprobe_controls
    bic x9, x9, #1
    str x9, [sp, #152]
    ldr x9, [sp, #96]
    ldr x0, [x9]
    ldr x1, [sp, #120]
    bl val_smccc_load_vectors
.Lprobe_controls:
    ldr x9, [sp, #152]
    tbz x9, #2, .Lprobe_end
    str xzr, [sp, #152]
    ldr x10, [sp, #176]
    msr SMCR_EL2, x10
    isb
.Lprobe_end:
    bl val_fault_end
    ldr x9, [sp, #96]
    str x0, [x9, #280]
    ldp x9, x10, [sp, #128]
    msr cptr_el2, x9
    msr cpacr_el1, x10
    isb
    ldr x9, [sp, #144]
    msr daif, x9
    b .Lprobe_return
.Lprobe_unarmed:
    mov x0, #-1
    str x0, [sp, #160]
    ldr x9, [sp, #144]
    msr daif, x9
.Lprobe_return:
    ldr x0, [sp, #160]
    ldr x18, [sp, #184]
    ldp x19, x20, [sp]
    ldp x21, x22, [sp, #16]
    ldp x23, x24, [sp, #32]
    ldp x25, x26, [sp, #48]
    ldp x27, x28, [sp, #64]
    ldp x29, x30, [sp, #80]
    add sp, sp, #512
    ret
endfunc val_smccc_probe_state
