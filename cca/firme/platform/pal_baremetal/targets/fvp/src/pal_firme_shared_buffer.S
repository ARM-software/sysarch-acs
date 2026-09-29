/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

    .section .text, "ax"

    .global pal_firme_shared_buffer_read_ttbr0_el2
    .type pal_firme_shared_buffer_read_ttbr0_el2, %function
/** @brief Read TTBR0_EL2 without changing the caller translation regime. */
pal_firme_shared_buffer_read_ttbr0_el2:
    mrs x0, ttbr0_el2
    ret
    .size pal_firme_shared_buffer_read_ttbr0_el2, .-pal_firme_shared_buffer_read_ttbr0_el2

    .global pal_firme_shared_buffer_read_mair_el2
    .type pal_firme_shared_buffer_read_mair_el2, %function
/** @brief Read MAIR_EL2 for the coherent-buffer precondition. */
pal_firme_shared_buffer_read_mair_el2:
    mrs x0, mair_el2
    ret
    .size pal_firme_shared_buffer_read_mair_el2, .-pal_firme_shared_buffer_read_mair_el2

    .global pal_firme_shared_buffer_read_hcr_el2
    .type pal_firme_shared_buffer_read_hcr_el2, %function
/** @brief Read HCR_EL2 without changing the caller translation regime. */
pal_firme_shared_buffer_read_hcr_el2:
    mrs x0, hcr_el2
    ret
    .size pal_firme_shared_buffer_read_hcr_el2, .-pal_firme_shared_buffer_read_hcr_el2
