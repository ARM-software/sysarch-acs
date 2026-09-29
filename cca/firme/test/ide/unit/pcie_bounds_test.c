/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "val_pcie.h"
#include "pal_pcie.h"

static uint32_t g_config_accesses;

/** @brief Expose two disjoint native ECAM allocations. */
uint32_t pal_pcie_ecam_count(void)
{
    return 2U;
}

/** @brief Require bus and segment matching rather than default ECAM fallback. */
int pal_pcie_ecam_info(uint32_t index, pal_pcie_ecam_info_t *info)
{
    if (!info || index >= 2U)
        return VAL_ERROR;
    *info = (pal_pcie_ecam_info_t){.segment = index, .start_bus = 32U,
        .end_bus = 63U, .ecam_base = UINT64_C(0x40000000) + index * UINT64_C(0x10000000)};
    return VAL_SUCCESS;
}

/**
 * @brief Record configuration reads without accessing hardware.
 * @return VAL_SUCCESS for a non-null destination.
 */
int pal_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value)
{
    (void)bdf;
    (void)offset;
    g_config_accesses++;
    if (!value)
        return VAL_ERROR;
    *value = 0U;
    return VAL_SUCCESS;
}

/**
 * @brief Record configuration writes without accessing hardware.
 * @return VAL_SUCCESS.
 */
int pal_pcie_cfg_write32(uint32_t bdf, uint32_t offset, uint32_t value)
{
    (void)bdf;
    (void)offset;
    (void)value;
    g_config_accesses++;
    return VAL_SUCCESS;
}

/**
 * @brief Reject identifiers and offsets that would alias another PCI function.
 * @return VAL_SUCCESS after all conversion and access assertions pass.
 */
int main(void)
{
    uint32_t bdf;
    uint32_t value;
    uint64_t ecam = 0ULL;

    assert(val_pcie_ecam_base_for_bdf(val_pcie_bdf_create(1U, 32U, 1U, 0U),
                                       &ecam) == VAL_SUCCESS);
    assert(ecam == UINT64_C(0x50000000));
    assert(val_pcie_ecam_base_for_bdf(val_pcie_bdf_create(0U, 31U, 1U, 0U),
                                       &ecam) == VAL_ERROR);
    assert(ecam == UINT64_C(0x50000000));
    assert(val_pcie_ecam_base_for_bdf(val_pcie_bdf_create(2U, 32U, 1U, 0U),
                                       &ecam) == VAL_ERROR);
    assert(val_pcie_ecam_base_for_bdf(VAL_PCIE_BDF_INVALID, &ecam) == VAL_ERROR);
    assert(val_pcie_ecam_base_for_bdf(0U, NULL) == VAL_ERROR);

    bdf = val_pcie_bdf_create(255U, 255U, 31U, 7U);
    assert(bdf != VAL_PCIE_BDF_INVALID);
    assert(val_pcie_requester_id_from_bdf(bdf) == 0xffffU);
    assert(val_pcie_bdf_from_requester_id(255U, 0xffffU) == bdf);
    assert(val_pcie_bdf_create(256U, 0U, 0U, 0U) == VAL_PCIE_BDF_INVALID);
    assert(val_pcie_bdf_create(0U, 256U, 0U, 0U) == VAL_PCIE_BDF_INVALID);
    assert(val_pcie_bdf_create(0U, 0U, 32U, 0U) == VAL_PCIE_BDF_INVALID);
    assert(val_pcie_bdf_create(0U, 0U, 0U, 8U) == VAL_PCIE_BDF_INVALID);
    assert(val_pcie_bdf_from_requester_id(0U, 0x10000U) == VAL_PCIE_BDF_INVALID);
    assert(val_pcie_bdf_from_requester_id(256U, 0U) == VAL_PCIE_BDF_INVALID);
    assert(val_pcie_requester_id_from_bdf(VAL_PCIE_BDF_INVALID) == VAL_PCIE_REQ_ID_INVALID);
    assert(val_pcie_requester_id_from_bdf(0x00002000U) == VAL_PCIE_REQ_ID_INVALID);
    assert(val_pcie_requester_id_from_bdf(0x00000008U) == VAL_PCIE_REQ_ID_INVALID);

    assert(val_pcie_cfg_read32(bdf, 0xffcU, &value) == VAL_SUCCESS);
    assert(val_pcie_cfg_write32(bdf, 0xffcU, 0U) == VAL_SUCCESS);
    assert(g_config_accesses == 2U);
    assert(val_pcie_cfg_read32(bdf, 0x1000U, &value) == VAL_ERROR);
    assert(val_pcie_cfg_write32(bdf, 0x1000U, 0U) == VAL_ERROR);
    assert(val_pcie_cfg_read32(bdf, 1U, &value) == VAL_ERROR);
    assert(val_pcie_cfg_write32(bdf, 1U, 0U) == VAL_ERROR);
    assert(val_pcie_cfg_read32(0x00002000U, 0U, &value) == VAL_ERROR);
    assert(val_pcie_cfg_write32(0x00000008U, 0U, 0U) == VAL_ERROR);
    assert(val_pcie_cfg_read32(bdf, 0U, NULL) == VAL_ERROR);
    assert(g_config_accesses == 2U);
    puts("PCIe address bounds: PASS");
    return VAL_SUCCESS;
}
