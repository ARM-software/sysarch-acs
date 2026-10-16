/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"

    .section .text

/* Read ID_AA64ISAR0_EL1.RNDR without executing an unsupported instruction. */
func val_realm_pat_has_rndr
    mrs x0, id_aa64isar0_el1
    lsr x0, x0, #60
    cmp x0, #1
    cset w0, eq
    ret
endfunc val_realm_pat_has_rndr

/* Store only successful RNDR output; the caller first checks FEAT_RNG. */
func val_realm_pat_read_rndr
    cbz x0, 1f
    mrs x1, S3_3_C2_C4_0
    b.eq 1f
    str x1, [x0]
    mov x1, xzr
    mov w0, #1
    ret
1:
    mov x1, xzr
    mov w0, #0
    ret
endfunc val_realm_pat_read_rndr
