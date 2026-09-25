/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_common/val_mp.h"

/* Preserve entry arguments and LR; return the table index in x9. */
.macro val_find_pe
    mrs x10, mpidr_el1
    ldr x11, =PAL_PE_AFFINITY_MASK
    and x10, x10, x11
    ldr x11, =g_pal_pe_mpidrs
    mov x9, #0
1:
    ldr x12, [x11, x9, lsl #3]
    cmp x10, x12
    b.eq 2f
    add x9, x9, #1
    cmp x9, #PAL_PE_COUNT
    b.lo 1b
2:
.endm

/* Unknown affinities must never alias another PE's stack. */
.macro val_set_pe_stack
    val_find_pe
    cmp x9, #PAL_PE_COUNT
    b.lo 2f
1:
    wfe
    b 1b
2:
    ldr x10, =__stack_start__
    mov x11, #VAL_PE_STACK_SIZE
    add x9, x9, #1
    madd x10, x9, x11, x10
    mov sp, x10
    msr sp_el0, x10
.endm
