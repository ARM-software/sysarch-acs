/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef enum {
    MMU_EL1 = 1,
    MMU_EL2 = 2,
} mmu_el_t;

/*
 * Build a simple identity map suitable for early bare-metal bring-up:
 * - 1GiB blocks, VA == PA, configured for a 39-bit VA space.
 * - Device memory for low 4GiB; normal WBWA for DRAM window.
 */
void val_mmu_setup_identity_1g(mmu_el_t el, uint64_t dram_base, uint64_t dram_size);
void val_mmu_enable(mmu_el_t el);
int val_mmu_map_device_block_el2(uint64_t base);
int val_mmu_unmap_device_block_el2(uint64_t base);

/**
 *   @brief    - Map one EL2 page through the alternate MECID path.
 *   @param    - va : Page-aligned virtual address.
 *   @param    - pa : Page-aligned Realm physical address.
 *   @return   - VAL_SUCCESS on success, otherwise VAL_ERROR.
 **/
int val_mmu_map_alternate_mecid_page_el2(uint64_t va, uint64_t pa);

/**
 *   @brief    - Remove the active EL2 alternate-MECID page mapping.
 *   @param    - va : Virtual address passed to the map operation.
 *   @return   - VAL_SUCCESS on success, otherwise VAL_ERROR.
 **/
int val_mmu_unmap_alternate_mecid_page_el2(uint64_t va);
