/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/asm_macros.S"
#include "val_common/val_status.h"

/**
 * @brief Read a naturally aligned MMIO word with scoped synchronous recovery.
 * @param x0 Device-mapped address.
 * @param x1 Result pointer; not written if the access faults.
 * @return VAL_SUCCESS (0) or VAL_ERROR (-1).
 */
func val_mmio_read32_checked
    cbz x1, 3f
    tst x0, #3
    b.ne 3f
    stp x29, x30, [sp, #-48]!
    mov x29, sp
    stp x19, x20, [sp, #16]
    str x21, [sp, #32]
    mov x19, x0
    mov x20, x1
    adr x0, 1f
    bl val_fault_begin
    cbnz w0, 2f
    ldr w21, [x19]
    .inst 0xd5033f9f
    bl val_fault_end
    str w21, [x20]
    mov w0, #VAL_SUCCESS
    b 4f
1:
    bl val_fault_end
2:
    mov w0, #VAL_ERROR
4:
    ldr x21, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp x29, x30, [sp], #48
    ret
3:
    mov w0, #VAL_ERROR
    ret
endfunc val_mmio_read32_checked
