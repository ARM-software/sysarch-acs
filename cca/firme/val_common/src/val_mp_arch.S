/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"
#include "aarch64/val_mp_asm.inc"

    .section .text, "ax"

/** @brief Look up the current physical PE; PAL_PE_COUNT means unknown. */
func val_pe_index
    val_find_pe
    mov x0, x9
    ret
endfunc val_pe_index

/** @brief Read the physical counter after synchronizing instruction execution. */
func val_read_counter
    .inst 0xd5033fdf
    mrs x0, cntpct_el0
    ret
endfunc val_read_counter

/** @brief Return the architected counter frequency. */
func val_read_counter_frequency
    mrs x0, cntfrq_el0
    ret
endfunc val_read_counter_frequency

/** @brief Yield without an unbounded WFE that could miss a timeout. */
func val_mp_relax
    yield
    ret
endfunc val_mp_relax

/** @brief Clean [start, start + size) to PoC using this PE's cache-line size. */
func val_cache_clean_poc
    cbz x1, 2f
    add x1, x0, x1
    mrs x2, ctr_el0
    ubfx x2, x2, #16, #4
    mov x3, #4
    lsl x2, x3, x2
    sub x3, x2, #1
    bic x0, x0, x3
1:
    dc cvac, x0
    add x0, x0, x2
    cmp x0, x1
    b.lo 1b
2:
    .inst 0xd5033f9f /* DSB SY */
    ret
endfunc val_cache_clean_poc

/* Loadable storage ensures firmware assigns every stack the payload's PAS. */
    .section .mp_stacks, "aw", %progbits
    .balign 4096
    .space PAL_PE_COUNT * VAL_PE_STACK_SIZE
