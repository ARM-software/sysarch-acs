/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_test.h"

#include "aarch64/sysreg.h"
#include "val_common/val_status.h"

/* Feature discovery (RME/GDI discovery via ID registers). */
#define ID_AA64PFR0_EL1_RME_SHIFT 52U
#define ID_AA64PFR0_EL1_RME_MASK  0xFU

#define ID_AA64MMFR4_EL1_GDI_SHIFT 28U
#define ID_AA64MMFR4_EL1_GDI_MASK  0xFU

/**
 *   @brief    - Return the RME feature level from ID_AA64PFR0_EL1.
 *   @return   - RME feature level (0..15).
**/
uint32_t val_firme_feat_rme_level(void)
{
    uint64_t v = read_id_aa64pfr0_el1();
    return (uint32_t)((v >> ID_AA64PFR0_EL1_RME_SHIFT) & ID_AA64PFR0_EL1_RME_MASK);
}

/**
 *   @brief    - Check whether FEAT_RME is present.
 *   @return   - 1 if present, 0 otherwise.
**/
uint32_t val_firme_feat_rme_present(void)
{
    return val_firme_feat_rme_level() >= 1U;
}

/**
 *   @brief    - Check whether FEAT_RME_GPC2 is present (TF-A convention).
 *   @return   - 1 if present, 0 otherwise.
**/
uint32_t val_firme_feat_rme_gpc2_present(void)
{
    /* TF-A convention: RME level >= 2 indicates FEAT_RME_GPC2. */
    return val_firme_feat_rme_level() >= 2U;
}

/**
 *   @brief    - Check whether FEAT_RME_GDI is present.
 *   @return   - 1 if present, 0 otherwise.
**/
uint32_t val_firme_feat_rme_gdi_present(void)
{
    uint64_t v = read_id_aa64mmfr4_el1();
    return (((v >> ID_AA64MMFR4_EL1_GDI_SHIFT) & ID_AA64MMFR4_EL1_GDI_MASK) != 0U);
}

/**
 *   @brief    - Invoke FIRME_GM_GPI_SET with a target GPI attribute.
 *   @param    - base_pa       : Base physical address of the window.
 *   @param    - granule_count : Number of GPT granules in the window.
 *   @param    - target_gpi    : Target GPI (FIRME_GPI_*).
 *   @param    - out           : Optional result record.
 *   @return   - 0 on call dispatch (status in `out->x0`).
**/
void val_firme_gm_gpi_set_do(uint64_t base_pa, uint64_t granule_count, uint64_t target_gpi,
                             firme_res_t *out)
{
    val_firme_gm_gpi_set(base_pa, granule_count, FIRME_GPI_ATTR_TARGET(target_gpi), out);
}

/**
 *   @brief    - Retry FIRME_GM_GPI_SET until it no longer returns FIRME_RETRY.
 *   @param    - base_pa       : Base physical address of the window.
 *   @param    - granule_count : Number of GPT granules in the window.
 *   @param    - target_gpi    : Target GPI (FIRME_GPI_*).
 *   @param    - out           : Optional result record (last return).
 *   @return   - 0 (status in `out->x0`); retries are bounded.
**/
void val_firme_gm_gpi_set_do_retry(uint64_t base_pa, uint64_t granule_count, uint64_t target_gpi,
                                   firme_res_t *out)
{
    firme_res_t r = {0};
    for (uint32_t i = 0; i < 8U; i++) {
        val_firme_gm_gpi_set_do(base_pa, granule_count, target_gpi, &r);
        if (val_firme_status_to_i64(r.x0) != (int64_t)FIRME_RETRY) {
            if (out)
                *out = r;
            return;
        }
    }
    if (out)
        *out = r;
}
