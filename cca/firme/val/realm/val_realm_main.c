/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/sysreg.h"
#include "pal_platform.h"
#include "val_common/val_arch_init.h"
#include "val_common/val_fault.h"
#include "val_common/val_mmu.h"

extern void rel2_vectors_el2(void);

/*
 * Non-BSS cold-boot flag: used to safely clear BSS exactly once.
 * See `val/realm/val_realm_entry.S`.
 */
uint32_t g_rel2_cold_boot = 1;

const char *val_world_tag(void)
{
    return "rel2";
}

/**
 *   @brief    - Realm world synchronous exception handler.
 *   @param    - esr : ESR_EL2 value for the exception.
 *   @param    - elr : ELR_EL2 value for the exception.
 *   @param    - far : FAR_EL2 value for the exception (if applicable).
 *   @return   - void
**/
void rel2_sync_handler(uint64_t esr, uint64_t elr, uint64_t far)
{
    (void)elr;
    if (val_fault_handle_sync(esr, far) != 0)
        return;
    for (;;)
        val_wfe();
}

/**
 *   @brief    - Minimal Realm payload initialization (vectors + MMU + FP/SIMD).
 *   @param    - cold_boot : Non-zero only for initial table construction.
 *   @return   - void
**/
void rel2_platform_init(uint32_t cold_boot)
{
    /*
     * Realm payloads should avoid direct device access (e.g. UART) because
     * devices are typically not accessible from Realm state on RME platforms.
     *
     * Keep init minimal: exception vectors + MMU + FP/SIMD.
     */
    write_vbar_el2((uint64_t)(uintptr_t)&rel2_vectors_el2);

    if (cold_boot != 0U) {
        val_mmu_setup_identity_1g(MMU_EL2, PLAT_DRAM_BASE, PLAT_DRAM_SIZE);
        val_mmu_enable(MMU_EL2);
        val_mmu_publish_tables();
    }

    val_arch_init_basic(MMU_EL2);
    val_fault_reset();
}
