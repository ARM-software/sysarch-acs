/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../../platform/pal_baremetal/targets/fvp/src/pal_cxl.c"

#define PAL_CXL_TEST_ECAM UINT64_C(0x40000000)
#define PAL_CXL_TEST_ROOT UINT32_C(0x800)
#define PAL_CXL_TEST_ENDPOINT UINT32_C(0x10000)
#define PAL_CXL_TEST_DVSEC UINT32_C(0x100)
#define PAL_CXL_TEST_HDM (PAL_CXL_COMPONENT_BASE + UINT64_C(0x100))
#define PAL_CXL_TEST_CFG_COUNT 4U
#define PAL_CXL_TEST_MMIO_WRITES (2U + 2U * PAL_CXL_HDM_REG_COUNT)

typedef enum {
    PAL_CXL_TEST_NO_FAULT,
    PAL_CXL_TEST_WRITE_ERROR,
    PAL_CXL_TEST_READ_ERROR,
    PAL_CXL_TEST_DROP_WRITE,
    PAL_CXL_TEST_APPLY_THEN_ERROR
} pal_cxl_test_fault_t;

typedef struct {
    uint32_t host[PAL_CXL_HDM_REG_COUNT];
    uint32_t endpoint[PAL_CXL_HDM_REG_COUNT];
    uint32_t config[PAL_CXL_TEST_CFG_COUNT];
    uint32_t mmio_writes;
    uint32_t mmio_reads;
    uint32_t config_writes;
    uint32_t config_reads;
    uint32_t unmaps;
    uint32_t fail_mmio;
    uint32_t fail_config;
    uint32_t fail_unmap;
    pal_cxl_test_fault_t fault;
} pal_cxl_cleanup_native_t;

static pal_cxl_cleanup_native_t g_native;
static pal_cxl_fixture_t g_original;

/** @brief Resolve only decoder registers still reachable through the owned BAR. */
static uint32_t *pal_cxl_test_decoder(uint64_t address)
{
    uint32_t index;

    assert(g_pal_cxl.decoders_saved);
    assert(g_native.config_writes == 0U);
    for (index = 0U; index < PAL_CXL_HDM_REG_COUNT; index++) {
        if (address == PAL_CXL_CHBCR + 0x1010U + g_pal_cxl_hdm_offsets[index])
            return &g_native.host[index];
        if (address == PAL_CXL_TEST_HDM + g_pal_cxl_hdm_offsets[index])
            return &g_native.endpoint[index];
    }
    assert(VAL_FALSE);
    return NULL;
}

/** @brief Inject failed, silently dropped, or partially completed decoder writes. */
int val_mmio_write32_checked(uint64_t address, uint32_t value)
{
    uint32_t *reg;

    reg = pal_cxl_test_decoder(address);
    g_native.mmio_writes++;
    if (g_native.mmio_writes == g_native.fail_mmio) {
        if (g_native.fault == PAL_CXL_TEST_WRITE_ERROR)
            return VAL_ERROR;
        if (g_native.fault == PAL_CXL_TEST_DROP_WRITE)
            return VAL_SUCCESS;
    }
    *reg = value;
    if (g_native.mmio_writes == g_native.fail_mmio &&
        g_native.fault == PAL_CXL_TEST_APPLY_THEN_ERROR)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/** @brief Require actual decoder readback, including inaccessible-register errors. */
int val_mmio_read32_checked(uint64_t address, uint32_t *value)
{
    uint32_t *reg;

    reg = pal_cxl_test_decoder(address);
    g_native.mmio_reads++;
    if (g_native.mmio_reads == g_native.fail_mmio &&
        g_native.fault == PAL_CXL_TEST_READ_ERROR)
        return VAL_ERROR;
    *value = *reg;
    return VAL_SUCCESS;
}

/** @brief Match configuration accesses to the original ECAM and function identity. */
static uint32_t pal_cxl_test_config_index(uint64_t ecam, uint32_t bdf, uint32_t offset)
{
    assert(ecam == PAL_CXL_TEST_ECAM);
    if (bdf == PAL_CXL_TEST_ENDPOINT && offset == PAL_CXL_TEST_DVSEC + 0x0cU)
        return 0U;
    if (bdf == PAL_CXL_TEST_ENDPOINT && offset == PAL_CXL_CFG_COMMAND)
        return 1U;
    if (bdf == PAL_CXL_TEST_ENDPOINT && offset == PAL_CXL_CFG_BAR4)
        return 2U;
    assert(bdf == PAL_CXL_TEST_ROOT && offset == PAL_CXL_CFG_MEMORY_WINDOW);
    return 3U;
}

/** @brief Inject configuration errors while preserving write-one-to-clear status. */
int val_pcie_cfg_write32_at(uint64_t ecam, uint32_t bdf, uint32_t offset, uint32_t value)
{
    uint32_t index;

    index = pal_cxl_test_config_index(ecam, bdf, offset);
    assert(!g_pal_cxl.decoders_saved && !g_pal_cxl.mapped);
    g_native.config_writes++;
    if (index < 2U)
        assert((value & 0xffff0000U) == 0U);
    if (g_native.config_writes == g_native.fail_config) {
        if (g_native.fault == PAL_CXL_TEST_WRITE_ERROR)
            return VAL_ERROR;
        if (g_native.fault == PAL_CXL_TEST_DROP_WRITE)
            return VAL_SUCCESS;
    }
    g_native.config[index] = index < 2U ? value | 0x80000000U : value;
    if (g_native.config_writes == g_native.fail_config &&
        g_native.fault == PAL_CXL_TEST_APPLY_THEN_ERROR)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/** @brief Expose configuration readback failures rather than assuming writes worked. */
int val_pcie_cfg_read32_at(uint64_t ecam, uint32_t bdf, uint32_t offset, uint32_t *value)
{
    uint32_t index;

    index = pal_cxl_test_config_index(ecam, bdf, offset);
    g_native.config_reads++;
    if (g_native.config_reads == g_native.fail_config &&
        g_native.fault == PAL_CXL_TEST_READ_ERROR)
        return VAL_ERROR;
    *value = g_native.config[index];
    return VAL_SUCCESS;
}

/** @brief Keep failed-unmap ownership outstanding and catch duplicate successful unmaps. */
int val_mmu_unmap_device_block_el2(uint64_t address)
{
    assert(address == PAL_CXL_MEMORY && g_pal_cxl.mapped);
    assert(!g_pal_cxl.decoders_saved && g_native.config_writes == 0U);
    g_native.unmaps++;
    return g_native.fail_unmap ? VAL_ERROR : VAL_SUCCESS;
}

/** @brief Satisfy prepare linkage; a retained owner must reject before mapping. */
int val_mmu_map_device_block_el2(uint64_t address)
{
    (void)address;
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief A release-in-progress must not issue memory traffic. */
int val_probe_write64(uint64_t address, uint64_t value, val_fault_info_t *fault)
{
    (void)address;
    (void)value;
    (void)fault;
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief A release-in-progress must not read from a partially restored aperture. */
int val_probe_read64(uint64_t address, uint64_t *value, val_fault_info_t *fault)
{
    (void)address;
    (void)value;
    (void)fault;
    assert(VAL_FALSE);
    return VAL_ERROR;
}

/** @brief Suppress expected fault-injection diagnostics in native output. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    (void)verbosity;
    (void)tag;
    (void)format;
    return 0U;
}

/** @brief Seed distinct original and active values so dropped writes cannot pass. */
static void pal_cxl_test_reset(void)
{
    uint32_t index;

    g_native = (pal_cxl_cleanup_native_t){0};
    g_pal_cxl = (pal_cxl_fixture_t){
        .ecam_base = PAL_CXL_TEST_ECAM,
        .root_bdf = PAL_CXL_TEST_ROOT,
        .endpoint_bdf = PAL_CXL_TEST_ENDPOINT,
        .command = 0x100001U,
        .bar = 8U,
        .window = 0x60006000U,
        .dvsec = PAL_CXL_TEST_DVSEC,
        .control = 0x100001U,
        .active = VAL_TRUE,
        .mapped = VAL_TRUE,
        .decoders_saved = VAL_TRUE,
        .lease = 19U,
        .endpoint_hdm = PAL_CXL_TEST_HDM
    };
    for (index = 0U; index < PAL_CXL_HDM_REG_COUNT; index++) {
        g_pal_cxl.host_registers[index] = index + 1U;
        g_pal_cxl.endpoint_registers[index] = index + 1U;
        g_native.host[index] = index + 0x100U;
        g_native.endpoint[index] = index + 0x200U;
    }
    g_pal_cxl.host_registers[3] = 0U;
    g_pal_cxl.host_registers[4] = 0U;
    g_pal_cxl.host_registers[5] = 0U;
    g_pal_cxl.endpoint_registers[3] = 0U;
    g_pal_cxl.endpoint_registers[4] = 0U;
    g_pal_cxl.endpoint_registers[5] = 0U;
    g_native.config[0] = 5U;
    g_native.config[1] = 3U;
    g_native.config[2] = PAL_CXL_ENDPOINT_BAR;
    g_native.config[3] = 0x50005000U;
    g_original = g_pal_cxl;
}

/** @brief Verify failed cleanup cannot erase ownership, acquire again, or probe traffic. */
static void pal_cxl_test_retained(void)
{
    pal_cxl_traffic_snapshot_t snapshot;
    uint32_t mmio_reads;
    uint32_t config_reads;

    assert(g_pal_cxl.active && g_pal_cxl.releasing);
    assert(g_pal_cxl.ecam_base == g_original.ecam_base);
    assert(g_pal_cxl.root_bdf == g_original.root_bdf);
    assert(g_pal_cxl.endpoint_bdf == g_original.endpoint_bdf);
    assert(g_pal_cxl.lease == g_original.lease);
    assert(g_pal_cxl.command == g_original.command && g_pal_cxl.bar == g_original.bar);
    assert(g_pal_cxl.window == g_original.window && g_pal_cxl.control == g_original.control);
    assert(memcmp(g_pal_cxl.host_registers, g_original.host_registers,
                  sizeof(g_pal_cxl.host_registers)) == 0);
    assert(memcmp(g_pal_cxl.endpoint_registers, g_original.endpoint_registers,
                  sizeof(g_pal_cxl.endpoint_registers)) == 0);
    mmio_reads = g_native.mmio_reads;
    config_reads = g_native.config_reads;
    assert(pal_cxl_traffic_prepare_at(PAL_CXL_TEST_ECAM, PAL_CXL_TEST_ROOT,
                                      PAL_CXL_TEST_ENDPOINT) == VAL_ERROR);
    assert(pal_cxl_traffic_snapshot(&snapshot) == VAL_ERROR);
    assert(pal_cxl_traffic_check(VAL_FALSE) == VAL_ERROR);
    assert(pal_cxl_traffic_check(VAL_TRUE) == VAL_ERROR);
    assert(g_native.mmio_reads == mmio_reads && g_native.config_reads == config_reads);
}

/** @brief Verify successful retry restores the originals and makes release idempotent. */
static void pal_cxl_test_released(void)
{
    uint32_t config_writes;
    uint32_t mmio_writes;
    uint32_t unmaps;

    assert(!g_pal_cxl.active && !g_pal_cxl.releasing && !g_pal_cxl.mapped);
    assert(!g_pal_cxl.decoders_saved && g_pal_cxl.lease == 0ULL);
    assert(memcmp(g_native.host, g_original.host_registers, sizeof(g_native.host)) == 0);
    assert(memcmp(g_native.endpoint, g_original.endpoint_registers,
                  sizeof(g_native.endpoint)) == 0);
    assert(g_native.config[0] == ((g_original.control & 0xffffU) | 0x80000000U));
    assert(g_native.config[1] == ((g_original.command & 0xffffU) | 0x80000000U));
    assert(g_native.config[2] == g_original.bar && g_native.config[3] == g_original.window);
    config_writes = g_native.config_writes;
    mmio_writes = g_native.mmio_writes;
    unmaps = g_native.unmaps;
    assert(pal_cxl_traffic_release() == VAL_SUCCESS);
    assert(g_native.config_writes == config_writes && g_native.mmio_writes == mmio_writes);
    assert(g_native.unmaps == unmaps);
}

/** @brief Inject every decoder/configuration failure and retry only unfinished stages. */
int main(void)
{
    uint32_t ordinal;
    uint32_t mmio_writes;
    pal_cxl_test_fault_t fault;

    pal_cxl_test_reset();
    assert(pal_cxl_traffic_release() == VAL_SUCCESS);
    pal_cxl_test_released();
    for (fault = PAL_CXL_TEST_WRITE_ERROR; fault <= PAL_CXL_TEST_APPLY_THEN_ERROR; fault++) {
        for (ordinal = 1U; ordinal <= PAL_CXL_TEST_MMIO_WRITES; ordinal++) {
            pal_cxl_test_reset();
            g_native.fail_mmio = ordinal;
            g_native.fault = fault;
            assert(pal_cxl_traffic_release() == VAL_ERROR);
            pal_cxl_test_retained();
            assert(g_pal_cxl.decoders_saved && g_pal_cxl.mapped);
            assert(g_native.unmaps == 0U && g_native.config_writes == 0U);
            g_native.fail_mmio = 0U;
            assert(pal_cxl_traffic_release() == VAL_SUCCESS);
            assert(g_native.unmaps == 1U);
            pal_cxl_test_released();
        }
        for (ordinal = 1U; ordinal <= PAL_CXL_TEST_CFG_COUNT; ordinal++) {
            pal_cxl_test_reset();
            g_native.fail_config = ordinal;
            g_native.fault = fault;
            assert(pal_cxl_traffic_release() == VAL_ERROR);
            pal_cxl_test_retained();
            assert(!g_pal_cxl.decoders_saved && !g_pal_cxl.mapped);
            assert(g_native.unmaps == 1U);
            mmio_writes = g_native.mmio_writes;
            g_native.fail_config = 0U;
            assert(pal_cxl_traffic_release() == VAL_SUCCESS);
            assert(g_native.unmaps == 1U && g_native.mmio_writes == mmio_writes);
            pal_cxl_test_released();
        }
    }
    pal_cxl_test_reset();
    g_native.fail_unmap = VAL_TRUE;
    assert(pal_cxl_traffic_release() == VAL_ERROR);
    pal_cxl_test_retained();
    assert(!g_pal_cxl.decoders_saved && g_pal_cxl.mapped);
    assert(g_native.config_writes == 0U);
    mmio_writes = g_native.mmio_writes;
    g_native.fail_unmap = VAL_FALSE;
    assert(pal_cxl_traffic_release() == VAL_SUCCESS);
    assert(g_native.unmaps == 2U && g_native.mmio_writes == mmio_writes);
    pal_cxl_test_released();
    pal_cxl_test_reset();
    g_pal_cxl.mapped = VAL_FALSE;
    g_pal_cxl.decoders_saved = VAL_FALSE;
    memcpy(g_native.host, g_original.host_registers, sizeof(g_native.host));
    memcpy(g_native.endpoint, g_original.endpoint_registers, sizeof(g_native.endpoint));
    assert(pal_cxl_traffic_release() == VAL_SUCCESS);
    assert(g_native.mmio_writes == 0U && g_native.unmaps == 0U);
    pal_cxl_test_released();
    puts("CXL PAL cleanup ownership, readback and retry failure injection: PASS");
    return VAL_SUCCESS;
}
