/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#define VAL_MMU_EL2_BOOT_TCR 0x3519
#define VAL_MMU_BOOT_MAIR 0xff04

#ifndef __ASSEMBLER__
#include <stdint.h>
#include "val_common/val_fault.h"

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
void val_mmu_publish_tables(void);
uint32_t val_mmu_is_controller_pe(void);

/**
 * @brief Read an owned PA through a temporary Secure EL2 Non-secure mapping.
 * @param pa Aligned physical address leased to the caller for observation.
 * @param image_start Start of this Secure payload's identity-mapped image.
 * @param image_end End of its image, including all PE stacks and probe tables.
 * @param value Output value, valid only when fault->faulted is clear.
 * @param fault Synchronous access-fault evidence, not a transport status.
 * @return VAL_SUCCESS if the probe completed or faulted, VAL_ERROR on setup failure.
 * @details Requires an MMU/cache-disabled non-VHE S-EL2 caller; restores its state.
 */
int val_mmu_probe_ns_read64_el2(uint64_t pa, uint64_t image_start,
                               uint64_t image_end, uint64_t *value,
                               val_fault_info_t *fault);
/** @brief Probe native-PAS RAM with a private mapping, preserving enabled caches. */
int val_mmu_probe_native_read64_el2(uint64_t pa, uint64_t image_start,
                                    uint64_t image_end, uint64_t *value,
                                    val_fault_info_t *fault);
uint32_t val_mmu_ns_probe_active(void);

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
#endif
