/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/sysreg.h"
#include "val_common/val_arch_init.h"
#include "val_common/val_fault.h"
#include "val_common/val_gicv3.h"
#include "val_common/val_mmu.h"
#include "val_log.h"
#include "val_platform.h"
#include "val_firme_dispatch.h"
#include "val_firme_suite.h"
#include "pal_platform.h"
#include "val_common/val_status.h"

#if FIRME_ENABLE_CXL_ENDPOINT_PROBE && !FIRME_ENABLE_CXL_SPDM
#include "val_host_doe.h"
#endif

extern void nsel2_vectors(void);

const char *val_world_tag(void)
{
    return "nsel2";
}

/**
 *   @brief    - Initialize UART, vectors and basic architectural state for NS-EL2.
 *   @return   - void
**/
static void nsel2_platform_init(void)
{
    val_log_init(val_get_ns_uart_base());

    LOG_TEST(NULL, "");
    LOG_TEST(NULL, "*******************************************************");
    LOG_TEST(NULL, "  FIRME Architecture Compliance Suite (FIRME-ACS)");
    LOG_TEST(NULL, "  %s", FIRME_ACS_VERSION_STR);
    LOG_TEST(NULL, "*******************************************************");

    write_vbar_el2((uint64_t)(uintptr_t)&nsel2_vectors);

    /*
     * NS-EL2 bring-up:
     * - set vectors
     * - enable a minimal identity-mapped MMU (safe for BL33 in DRAM)
     * - enable FP/SIMD access
     */
    val_mmu_setup_identity_1g(MMU_EL2, PLAT_DRAM_BASE, PLAT_DRAM_SIZE);
    val_mmu_enable(MMU_EL2);
    val_arch_init_basic(MMU_EL2);
}

/**
 *   @brief    - NS-EL2 synchronous exception handler.
 *   @param    - esr : ESR_EL2 value for the exception.
 *   @param    - elr : ELR_EL2 value for the exception.
 *   @param    - far : FAR_EL2 value for the exception (if applicable).
 *   @return   - void
**/
void nsel2_sync_handler(uint64_t esr, uint64_t elr, uint64_t far)
{
    (void)elr;
    if (val_fault_handle_sync(esr, far) != 0)
        return;
    LOG_ERROR(NULL, "sync exception esr=%llx elr=%llx far=%llx",
              (unsigned long long)esr, (unsigned long long)elr,
              (unsigned long long)far);
    for (;;)
        val_wfe();
}

/**
 *   @brief    - NS-EL2 IRQ handler (GICv3).
 *   @return   - void
**/
void nsel2_irq_handler(void)
{
    uint32_t intid = val_gicv3_acknowledge_irq();

    LOG_DEBUG(NULL, "irq intid=%u", (unsigned int)intid);
    val_gicv3_end_of_irq(intid);
}

extern void dispatcher_main(void);

/**
 *   @brief    - NS-EL2 entrypoint called by TF-A when BL33 is handed off.
 *   @return   - void
**/
void nsel2_main(void)
{
#if FIRME_ENABLE_CXL_ENDPOINT_PROBE && !FIRME_ENABLE_CXL_SPDM
    val_host_doe_endpoint_t endpoint = {0};
#endif

    nsel2_platform_init();
#if FIRME_ENABLE_CXL_ENDPOINT_PROBE && !FIRME_ENABLE_CXL_SPDM
    if ((val_host_doe_prepare_endpoint(&endpoint) != VAL_SUCCESS) ||
        (val_host_doe_discover_spdm(&endpoint) != VAL_SUCCESS)) {
        LOG_ERROR(NULL, "CXL Type-2 endpoint DOE preflight failed");
        val_host_doe_release_endpoint(&endpoint);
        for (;;)
            val_wfe();
    }
#endif
    dispatcher_main();
#if FIRME_ENABLE_CXL_ENDPOINT_PROBE && !FIRME_ENABLE_CXL_SPDM
    if (val_host_doe_release_endpoint(&endpoint) != VAL_SUCCESS)
        LOG_ERROR(NULL, "CXL Type-2 endpoint DOE cleanup failed");
#endif
    for (;;)
        val_wfe();
}
