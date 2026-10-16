/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pal_pcie.h"
#include "val_host_doe.h"
#include "val_pcie.h"
#include "val_log.h"

#define TEST_ECAM_FIRST UINT64_C(0x40000000)
#define TEST_ECAM_SECOND UINT64_C(0x60000000)
#define TEST_ROOT_FIRST 0x100U
#define TEST_ROOT_SECOND 0x200U
#define TEST_ENDPOINT_TYPE2 0x20001U
#define TEST_DOE 0x140U

static const uint32_t g_bdfs[] = {
    TEST_ROOT_FIRST, TEST_ROOT_SECOND, 0x10000U, 0x20000U, TEST_ENDPOINT_TYPE2
};
static uint32_t g_config[2][5][1024];
static uint64_t g_last_ecam;
static uint32_t g_accesses;
static uint32_t g_read_failure;
static uint32_t g_write_failure;
static uint32_t g_restore_failure;
static uint32_t g_ignore_write;

/** @brief Expose duplicate BDFs and segment numbers in two distinct ECAM spaces. */
uint32_t pal_pcie_ecam_count(void)
{
    return 2U;
}

/** @brief Provide the two real namespaces used by this isolated hardware mock. */
int pal_pcie_ecam_info(uint32_t index, pal_pcie_ecam_info_t *info)
{
    if (!info || index >= 2U)
        return VAL_ERROR;
    *info = (pal_pcie_ecam_info_t){.ecam_base = index ? TEST_ECAM_SECOND : TEST_ECAM_FIRST,
                                 .ecam_size = UINT64_C(0x300000), .end_bus = 2U};
    return VAL_SUCCESS;
}

/** @brief Never resolve duplicate BDFs without an ECAM namespace in these tests. */
int pal_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value)
{
    (void)bdf;
    (void)offset;
    (void)value;
    assert(0 && "namespace discovery must not use BDF-only reads");
    return VAL_ERROR;
}

/** @brief Read only the named mock namespace and preserve configuration failures. */
int pal_pcie_cfg_read32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                           uint32_t *value)
{
    uint32_t namespace_index = ecam_base == TEST_ECAM_SECOND;
    uint32_t index;

    assert(value && offset <= 0xffcU && !(offset & 3U));
    if (ecam_base != TEST_ECAM_FIRST && ecam_base != TEST_ECAM_SECOND)
        return VAL_ERROR;
    g_last_ecam = ecam_base;
    g_accesses++;
    if (g_read_failure && ecam_base == TEST_ECAM_FIRST && bdf == TEST_ROOT_FIRST)
        return VAL_ERROR;
    *value = UINT32_MAX;
    for (index = 0U; index < 5U; index++) {
        if (g_bdfs[index] != bdf)
            continue;
        if (val_pcie_bdf_bus(bdf) == 2U &&
            !g_config[namespace_index][1][VAL_PCIE_CFG_BUS_NUMBERS / 4U])
            return VAL_SUCCESS;
        *value = g_config[namespace_index][index][offset / 4U];
        break;
    }

    return VAL_SUCCESS;
}

/** @brief Model successful or partially applied writes without cross-namespace aliasing. */
int pal_pcie_cfg_write32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                            uint32_t value)
{
    uint32_t namespace_index = ecam_base == TEST_ECAM_SECOND;
    uint32_t index;

    assert(ecam_base == TEST_ECAM_FIRST || ecam_base == TEST_ECAM_SECOND);
    g_last_ecam = ecam_base;
    g_accesses++;
    for (index = 0U; index < 5U; index++) {
        if (g_bdfs[index] != bdf)
            continue;
        if (offset == VAL_PCIE_CFG_BUS_NUMBERS && !value && g_restore_failure)
            return VAL_ERROR;
        if (offset == VAL_PCIE_CFG_BUS_NUMBERS && g_ignore_write)
            return VAL_SUCCESS;
        g_config[namespace_index][index][offset / 4U] = value;
        if (offset == VAL_PCIE_CFG_BUS_NUMBERS && value && g_write_failure)
            return VAL_ERROR;
        return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/** @brief Keep intentional negative-case diagnostics quiet. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return VAL_SUCCESS;
}

/** @brief Make the first Root Port and first downstream endpoint unsuitable. */
static void test_inventory(void)
{
    uint32_t namespace_index;
    uint32_t index;
    uint32_t *config;

    memset(g_config, 0, sizeof(g_config));
    g_read_failure = VAL_FALSE;
    g_write_failure = VAL_FALSE;
    g_restore_failure = VAL_FALSE;
    g_ignore_write = VAL_FALSE;
    for (namespace_index = 0U; namespace_index < 2U; namespace_index++) {
        for (index = 0U; index < 5U; index++) {
            config = g_config[namespace_index][index];
            config[0] = 0x1234U;
            config[VAL_PCIE_CFG_STATUS / 4U] = 1U << 20U;
            config[VAL_PCIE_CFG_CAP_PTR / 4U] = 0x40U;
            config[0x40U / 4U] = VAL_PCIE_CAP_ID_EXPRESS;
            if (index < 2U) {
                config[0x40U / 4U] |= VAL_PCIE_PORT_TYPE_ROOT_PORT << 20U;
                config[VAL_PCIE_CFG_HEADER_TYPE / 4U] = 1U << 16U;
                config[VAL_PCIE_CFG_BUS_NUMBERS / 4U] = (index + 1U) * 0x10100U;
            } else {
                config[0x100U / 4U] = (TEST_DOE << 20U) | VAL_PCIE_EXT_CAP_ID_DVSEC;
                config[0x104U / 4U] = (12U << 20U) | 0x1e98U;
                config[0x108U / 4U] = (index == 4U && namespace_index ? 7U : 3U) << 16U;
                config[TEST_DOE / 4U] = VAL_PCIE_EXT_CAP_ID_DOE;
            }
        }
    }
}

/** @brief Exercise candidate iteration, explicit selection and strict restoration. */
int main(void)
{
    val_host_doe_endpoint_t endpoint = {0};
    uint64_t ecam = 0ULL;
    uint32_t value;
    uint32_t accesses;
    uint32_t offset;
    uint32_t error_offset;
    const uint32_t request[] = {1U, 3U, 0U};

    test_inventory();
    assert(val_pcie_ecam_base_for_bdf(TEST_ROOT_SECOND, &ecam) == VAL_ERROR);
    assert(val_host_doe_prepare_endpoint(&endpoint) == VAL_SUCCESS);
    assert(endpoint.ecam_base == TEST_ECAM_SECOND);
    assert(endpoint.root_bdf == TEST_ROOT_SECOND && endpoint.endpoint_bdf == TEST_ENDPOINT_TYPE2);
    assert(endpoint.doe_offset == TEST_DOE && !endpoint.buses_changed);
    assert(val_host_doe_send(&endpoint, request, sizeof(request)) == VAL_SUCCESS);
    assert(g_last_ecam == TEST_ECAM_SECOND);
    assert(val_host_doe_release_endpoint(&endpoint) == VAL_SUCCESS);
    assert(val_host_doe_prepare_endpoint_at(TEST_ECAM_FIRST, &endpoint) == VAL_ERROR);
    assert(val_host_doe_prepare_endpoint_at(TEST_ECAM_SECOND, &endpoint) == VAL_SUCCESS);

    accesses = g_accesses;
    assert(val_pcie_cfg_read32_at(TEST_ECAM_SECOND, TEST_ROOT_SECOND, 0x1000U,
                                 &value) == VAL_ERROR);
    assert(val_pcie_cfg_write32_at(TEST_ECAM_SECOND, TEST_ROOT_SECOND, 1U, 0U) == VAL_ERROR);
    assert(g_accesses == accesses);
    g_read_failure = VAL_TRUE;
    assert(val_host_doe_prepare_endpoint(&endpoint) == VAL_ERROR);
    assert(g_last_ecam == TEST_ECAM_FIRST);

    test_inventory();
    g_config[1][4][TEST_DOE / 4U] |= 0x100U << 20U;
    assert(val_pcie_cxl_doe_probe_at(TEST_ECAM_SECOND, TEST_ENDPOINT_TYPE2,
                                     &offset, &error_offset) == VAL_PCIE_PROBE_ERROR);
    test_inventory();
    g_config[1][1][VAL_PCIE_CFG_BUS_NUMBERS / 4U] = 0U;
    assert(val_host_doe_prepare_endpoint_at(TEST_ECAM_SECOND, &endpoint) == VAL_SUCCESS);
    assert(endpoint.buses_changed && endpoint.original_buses == 0U);
    assert(g_config[1][1][VAL_PCIE_CFG_BUS_NUMBERS / 4U] == 0x20200U);
    g_restore_failure = VAL_TRUE;
    assert(val_host_doe_release_endpoint(&endpoint) == VAL_ERROR && endpoint.buses_changed);
    g_restore_failure = VAL_FALSE;
    assert(val_host_doe_release_endpoint(&endpoint) == VAL_SUCCESS && !endpoint.buses_changed);
    assert(g_config[1][1][VAL_PCIE_CFG_BUS_NUMBERS / 4U] == 0U);

    g_write_failure = VAL_TRUE;
    assert(val_host_doe_prepare_endpoint_at(TEST_ECAM_SECOND, &endpoint) == VAL_ERROR);
    assert(!endpoint.buses_changed);
    assert(g_config[1][1][VAL_PCIE_CFG_BUS_NUMBERS / 4U] == 0U);
    g_write_failure = VAL_FALSE;
    g_ignore_write = VAL_TRUE;
    assert(val_host_doe_prepare_endpoint_at(TEST_ECAM_SECOND, &endpoint) == VAL_ERROR);
    assert(!endpoint.buses_changed);
    puts("IDE ECAM namespaces, later candidates, DOE routing and rollback: PASS");
    return VAL_SUCCESS;
}
