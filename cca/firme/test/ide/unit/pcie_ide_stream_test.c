/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "val_pcie_ide.h"
#include "pal_pcie.h"

#define TEST_ROOT 0x100U
#define TEST_ENDPOINT 0x10000U
#define TEST_IDE 0x140U
#define TEST_BLOCK (TEST_IDE + 12U)
#define TEST_LOCK 0x110U
#define TEST_CAPABILITY UINT32_C(0x00010030)

static uint32_t g_config[2][1024];
static uint32_t g_writes;
static uint32_t g_reject_offset;

/** @brief Expose platform address inventory, not preselected IDE stream IDs. */
uint32_t pal_pcie_ecam_count(void)
{
    return 1U;
}

/** @brief Return the enumerated bus window for the mock pair. */
int pal_pcie_ecam_info(uint32_t index, pal_pcie_ecam_info_t *info)
{
    if (index || !info)
        return VAL_ERROR;
    *info = (pal_pcie_ecam_info_t){.ecam_base = UINT64_C(0x40000000),
        .ecam_size = UINT64_C(0x200000), .end_bus = 1U};
    return VAL_SUCCESS;
}

/** @brief Read the two fake PCI configuration spaces used by real VAL code. */
int pal_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value)
{
    if (!value || offset >= 4096U)
        return VAL_ERROR;
    if ((bdf != TEST_ROOT && bdf != TEST_ENDPOINT) ||
        (bdf == TEST_ENDPOINT && !(g_config[0][0x18U / 4U] & 0xff00U))) {
        *value = UINT32_MAX;
        return VAL_SUCCESS;
    }
    *value = g_config[bdf == TEST_ENDPOINT][offset / 4U];
    return VAL_SUCCESS;
}

/** @brief Model write-ignore so readback, rollback and quarantine are exercised. */
int pal_pcie_cfg_write32(uint32_t bdf, uint32_t offset, uint32_t value)
{
    g_writes++;
    if (offset != g_reject_offset)
        g_config[bdf == TEST_ENDPOINT][offset / 4U] = value;
    return VAL_SUCCESS;
}

/** @brief Restore a single unused Selective block per port and an Arm RMEDA DVSEC. */
static val_pcie_ide_path_t test_path(void)
{
    val_pcie_ide_path_t path = {.root_port_bdf = TEST_ROOT,
        .endpoint_bdf = TEST_ENDPOINT, .root_port_present = VAL_TRUE,
        .endpoint_present = VAL_TRUE, .ecam_base = UINT64_C(0x40000000)};

    memset(g_config, 0, sizeof(g_config));
    g_config[0][0] = 0x13b5U;
    g_config[1][0] = 0x13b5U;
    g_config[0][4U / 4U] = 0x100000U;
    g_config[1][4U / 4U] = 0x100000U;
    g_config[0][0x34U / 4U] = 0x40U;
    g_config[1][0x34U / 4U] = 0x40U;
    g_config[0][0x40U / 4U] = 0x400010U;
    g_config[1][0x40U / 4U] = 0x10U;
    g_config[0][0x18U / 4U] = 0x10100U;
    g_config[0][0xcU / 4U] = 1U << 16U;
    g_config[0][0x100U / 4U] = (TEST_IDE << 20U) | 0x10023U;
    g_config[0][0x104U / 4U] = (0x14U << 20U) | 0x13b5U;
    g_config[0][0x108U / 4U] = 0xff01U;
    g_config[1][0x100U / 4U] = (TEST_IDE << 20U) | 0x10001U;
    g_config[0][TEST_IDE / 4U] = TEST_CAPABILITY;
    g_config[1][TEST_IDE / 4U] = TEST_CAPABILITY;
    g_config[0][(TEST_IDE + 4U) / 4U] = 2U;
    g_config[1][(TEST_IDE + 4U) / 4U] = 2U;
    g_writes = 0U;
    g_reject_offset = UINT32_MAX;
    return path;
}

/** @brief Test actual discovery, RMEDA locking, rollback and sequential reuse. */
int main(void)
{
    val_pcie_ide_path_t path = test_path();
    val_pcie_ide_stream_t stream = {0};
    val_pcie_ide_stream_t second = {0};
    uint32_t pass = 0U;

    assert(val_pcie_ide_stream_discover(VAL_TRUE, NULL) == ACS_STATUS_FAIL);
    assert(val_pcie_ide_stream_acquire(NULL, VAL_TRUE, &stream) == ACS_STATUS_FAIL);
    assert(val_pcie_ide_stream_release(NULL) == VAL_ERROR);
    assert(val_pcie_ide_stream_discover(VAL_TRUE, &stream) == ACS_STATUS_PASS);
    assert(stream.path.root_port_id == 8U && stream.path.endpoint_id == 0x100U);
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);

    g_config[0][0x18U / 4U] = 0U;
    assert(val_pcie_ide_stream_discover(VAL_TRUE, &stream) == ACS_STATUS_PASS);
    assert(stream.buses_changed && g_config[0][0x18U / 4U] == 0x10100U);
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);
    assert(g_config[0][0x18U / 4U] == 0U);
    path = test_path();

    g_config[0][0x18U / 4U] = 0U;
    assert(val_pcie_ide_stream_discover(VAL_TRUE, &stream) == ACS_STATUS_PASS);
    g_reject_offset = 0x18U;
    assert(val_pcie_ide_stream_release(&stream) == VAL_ERROR);
    assert(stream.acquired && stream.buses_changed);
    assert(val_pcie_ide_stream_discover(VAL_TRUE, &second) == ACS_STATUS_FAIL);
    g_reject_offset = UINT32_MAX;
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);
    assert(g_config[0][0x18U / 4U] == 0U);
    path = test_path();

    for (pass = 0U; pass < 4U; pass++) {
        assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_PASS);
        assert(stream.stream_id == 1U && stream.ports[0].index == 0U);
        assert(g_config[0][TEST_LOCK / 4U] == 1U);
        assert(g_config[0][(TEST_BLOCK + 4U) / 4U] == (1U << 24U));
        assert(g_config[0][(TEST_BLOCK + 16U) / 4U] == 0x10001U);
        assert(g_config[1][(TEST_BLOCK + 16U) / 4U] == 0x801U);
        assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &second) == ACS_STATUS_FAIL);
        assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);
        assert(g_config[0][TEST_LOCK / 4U] == 0U);
        assert(g_config[0][(TEST_BLOCK + 4U) / 4U] == 0U);
        assert(g_config[1][(TEST_BLOCK + 16U) / 4U] == 0U);
    }

    path = test_path();
    assert(val_pcie_ide_stream_acquire(&path, VAL_FALSE, &stream) == ACS_STATUS_SKIP);
    assert(g_writes == 0U);
    g_config[0][TEST_LOCK / 4U] = 1U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_SKIP);
    assert(g_writes == 0U && g_config[0][TEST_LOCK / 4U] == 1U);

    path = test_path();
    g_config[0][(TEST_BLOCK + 4U) / 4U] = 73U << 24U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_SKIP);
    assert(g_writes == 0U);

    path = test_path();
    g_config[0][(TEST_BLOCK + 4U) / 4U] = 0x300U;
    g_config[1][(TEST_BLOCK + 4U) / 4U] = 0x300U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_PASS);
    assert(g_config[0][(TEST_BLOCK + 4U) / 4U] == ((1U << 24U) | 0x300U));
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);
    assert(g_config[0][(TEST_BLOCK + 4U) / 4U] == 0x300U);

    path = test_path();
    g_config[1][(TEST_IDE + 4U) / 4U] = 0U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_SKIP);
    assert(g_writes == 0U);

    path = test_path();
    g_config[0][TEST_IDE / 4U] |= (TEST_BLOCK + 16U) << 20U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_FAIL);
    assert(g_writes == 0U);

    path = test_path();
    g_config[0][TEST_IDE / 4U] |= 0x100U << 20U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_FAIL);
    assert(g_writes == 0U);

    path = test_path();
    g_config[0][0x100U / 4U] = (0x1c0U << 20U) | 0x10023U;
    g_config[0][0x1c0U / 4U] = (TEST_IDE << 20U) | 0x10001U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_PASS);
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);

    path = test_path();
    g_config[0][(TEST_IDE + 4U) / 4U] = 3U;
    g_config[1][(TEST_IDE + 4U) / 4U] = 3U;
    g_config[0][TEST_BLOCK / 4U] = (1U << 24U) | 1U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_PASS);
    assert(stream.ports[0].block == TEST_BLOCK + 8U);
    assert(stream.stream_id == 2U);
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);

    path = test_path();
    g_config[0][(TEST_IDE + 4U) / 4U] |= 1U << 16U;
    g_config[0][TEST_BLOCK / 4U] = 2U;
    g_config[0][(TEST_BLOCK + 4U) / 4U] = 1U << 24U;
    g_config[0][TEST_LOCK / 4U] = 1U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_PASS);
    assert(stream.stream_id == 2U && stream.ports[0].index == 1U);
    assert(stream.ports[0].block == TEST_BLOCK + 44U);
    assert(g_config[0][TEST_LOCK / 4U] == 3U);
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);
    assert(g_config[0][TEST_LOCK / 4U] == 1U);
    assert(g_config[0][(TEST_BLOCK + 4U) / 4U] == 1U << 24U);

    path = test_path();
    g_reject_offset = TEST_BLOCK + 4U;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_FAIL);
    assert(!stream.acquired && g_config[0][(TEST_BLOCK + 16U) / 4U] == 0U);
    g_reject_offset = UINT32_MAX;
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &stream) == ACS_STATUS_PASS);
    g_reject_offset = TEST_LOCK;
    assert(val_pcie_ide_stream_release(&stream) == VAL_ERROR);
    assert(stream.acquired);
    assert(val_pcie_ide_stream_acquire(&path, VAL_TRUE, &second) == ACS_STATUS_FAIL);
    g_reject_offset = UINT32_MAX;
    assert(val_pcie_ide_stream_release(&stream) == VAL_SUCCESS);
    puts("Selective IDE discovery, Realm locking and cleanup: PASS");
    return VAL_SUCCESS;
}
