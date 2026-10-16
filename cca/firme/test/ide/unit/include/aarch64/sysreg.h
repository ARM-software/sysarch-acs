/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

/** @brief Keep the native PAL fixture independent of AArch64 instructions. */
static inline void dsb_sy(void)
{
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}
