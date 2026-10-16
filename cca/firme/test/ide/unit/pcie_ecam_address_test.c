/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "pal_pcie.h"
#include "val_pcie.h"
#include "val_common/val_fault.h"

static uint64_t g_address;
static uint32_t g_accesses;

/** @brief Inspect the real PAL translator without reading host physical memory. */
int val_mmio_read32_checked(uint64_t address, uint32_t *value)
{
    g_address = address;
    g_accesses++;
    *value = 0x1234U;
    return VAL_SUCCESS;
}

/** @brief Record only accesses which passed PAL namespace and size validation. */
int val_mmio_write32_checked(uint64_t address, uint32_t value)
{
    (void)value;
    g_address = address;
    g_accesses++;
    return VAL_SUCCESS;
}

/** @brief Check explicit ECAM selection and prevent malformed offsets from reaching MMIO. */
int main(void)
{
    pal_pcie_ecam_info_t ecam = {0};
    uint32_t bdf;
    uint32_t value;
    uint32_t accesses;

    assert(pal_pcie_ecam_info(0U, &ecam) == VAL_SUCCESS);
    bdf = val_pcie_bdf_create(ecam.segment, ecam.start_bus, 0U, 0U);
    assert(pal_pcie_cfg_read32_at(ecam.ecam_base, bdf, 0xffcU, &value) == VAL_SUCCESS);
    assert(g_address == ecam.ecam_base + 0xffcU && value == 0x1234U);
    assert(pal_pcie_cfg_write32_at(ecam.ecam_base, bdf, 0x40U, 0U) == VAL_SUCCESS);
    assert(g_address == ecam.ecam_base + 0x40U);
    accesses = g_accesses;
    assert(pal_pcie_cfg_read32_at(ecam.ecam_base + ecam.ecam_size, bdf, 0U,
                                  &value) == VAL_ERROR);
    assert(pal_pcie_cfg_write32_at(ecam.ecam_base, bdf, 0x1000U, 0U) == VAL_ERROR);
    assert(pal_pcie_cfg_read32_at(ecam.ecam_base, bdf, 2U, &value) == VAL_ERROR);
    assert(pal_pcie_cfg_read32_at(ecam.ecam_base, bdf, 0U, NULL) == VAL_ERROR);
    assert(pal_pcie_cfg_read32(bdf, UINT32_MAX - 3U, &value) == VAL_ERROR);
    assert(g_accesses == accesses);
    puts("IDE PAL ECAM namespace and address bounds: PASS");
    return VAL_SUCCESS;
}
