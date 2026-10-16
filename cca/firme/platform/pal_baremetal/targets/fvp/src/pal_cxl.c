/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_cxl.h"

#include "aarch64/sysreg.h"
#include "val_common/val_fault.h"
#include "val_common/val_mem.h"
#include "val_common/val_mmu.h"
#include "val_log.h"
#include "val_pcie.h"

#define PAL_CXL_CHBCR UINT64_C(0x2b600000)
#define PAL_CXL_MEMORY UINT64_C(0x100000000)
#define PAL_CXL_WINDOW_SIZE UINT32_C(0x10000000)
#define PAL_CXL_ENDPOINT_BAR UINT32_C(0x50080000)
#define PAL_CXL_ROOT_BAR UINT32_C(0x50000000)
#define PAL_CXL_KM_BASE (PAL_CXL_ROOT_BAR + UINT32_C(0x20000))
#define PAL_CXL_SIGNATURE UINT32_C(0x46494b4d)
#define PAL_CXL_COMPONENT_BASE (PAL_CXL_ENDPOINT_BAR + UINT32_C(0x1000))
#define PAL_CXL_CFG_COMMAND 0x04U
#define PAL_CXL_CFG_BAR4 0x20U
#define PAL_CXL_CFG_MEMORY_WINDOW 0x20U
#define PAL_CXL_MEMORY_COMMAND 0x2U
#define PAL_CXL_HDM_CAP_ID 0x5U
#define PAL_CXL_DVSEC_VENDOR 0x1e98U
#define PAL_CXL_HDM_REG_COUNT 8U
#define PAL_CXL_PROBE_PATTERN UINT64_C(0x43584c50524f5445)

typedef struct {
    uint64_t ecam_base;
    uint32_t root_bdf;
    uint32_t endpoint_bdf;
    uint32_t command;
    uint32_t bar;
    uint32_t window;
    uint32_t dvsec;
    uint32_t control;
    uint32_t active;
    uint32_t releasing;
    uint32_t mapped;
    uint32_t decoders_saved;
    uint64_t lease;
    uint64_t endpoint_hdm;
    uint32_t host_registers[PAL_CXL_HDM_REG_COUNT];
    uint32_t endpoint_registers[PAL_CXL_HDM_REG_COUNT];
} pal_cxl_fixture_t;

static pal_cxl_fixture_t g_pal_cxl;
static uint64_t g_pal_cxl_probe_sequence;
static uint64_t g_pal_cxl_lease;
static const uint32_t g_pal_cxl_hdm_offsets[PAL_CXL_HDM_REG_COUNT] = {
    0x04U, 0x10U, 0x14U, 0x18U, 0x1cU, 0x20U, 0x24U, 0x28U
};

/** @brief Read a register in the explicitly selected local model fixture. */
static uint32_t pal_cxl_read32(uint64_t address)
{
    return *(volatile uint32_t *)(uintptr_t)address;
}

/** @brief Complete a fixture register write before issuing the next operation. */
static void pal_cxl_write32(uint64_t address, uint32_t value)
{
    *(volatile uint32_t *)(uintptr_t)address = value;
    dsb_sy();
}

/**
 * @brief Find the CXL Device DVSEC without assuming its ECAM offset.
 * @param bdf Discovered test endpoint.
 * @return Capability offset, or zero when malformed/absent.
 */
static uint32_t pal_cxl_device_dvsec(uint32_t bdf)
{
    uint32_t offset = 0x100U;
    uint32_t count;
    uint32_t header;
    uint32_t vendor;
    uint32_t identifier;

    for (count = 0U; count < 256U && offset >= 0x100U && offset <= 0xff0U; count++) {
        if ((offset & 3U) ||
            val_pcie_cfg_read32_at(g_pal_cxl.ecam_base, bdf, offset, &header) != VAL_SUCCESS)
            return 0U;
        if ((header & 0xffffU) == VAL_PCIE_EXT_CAP_ID_DVSEC) {
            if (val_pcie_cfg_read32_at(g_pal_cxl.ecam_base, bdf, offset + 4U,
                                       &vendor) != VAL_SUCCESS ||
                val_pcie_cfg_read32_at(g_pal_cxl.ecam_base, bdf, offset + 8U,
                                       &identifier) != VAL_SUCCESS)
                return 0U;
            if ((vendor & 0xffffU) == PAL_CXL_DVSEC_VENDOR &&
                (identifier & 0xffffU) == 0U)
                return offset;
        }
        offset = header >> 20U;
    }
    return 0U;
}

/** @brief Retain BDF-only callers only when both functions resolve unambiguously. */
int pal_cxl_traffic_prepare(uint32_t root_bdf, uint32_t endpoint_bdf)
{
    uint64_t root_ecam;
    uint64_t endpoint_ecam;

    if (val_pcie_ecam_base_for_bdf(root_bdf, &root_ecam) != VAL_SUCCESS ||
        val_pcie_ecam_base_for_bdf(endpoint_bdf, &endpoint_ecam) != VAL_SUCCESS ||
        root_ecam != endpoint_ecam)
        return VAL_ERROR;
    return pal_cxl_traffic_prepare_at(root_ecam, root_bdf, endpoint_bdf);
}

/**
 * @brief Configure a test-owned CXL.mem aperture; never borrow DRAM or firmware RAM.
 * @param ecam_base Actual ECAM namespace of both functions.
 * @param root_bdf Root Port already programmed through FIRME.
 * @param endpoint_bdf Its directly attached Type-2 endpoint.
 * @return VAL_SUCCESS only for the experimental model with unused decoders/BAR.
 */
int pal_cxl_traffic_prepare_at(uint64_t ecam_base, uint32_t root_bdf,
                                uint32_t endpoint_bdf)
{
    uint32_t header;
    uint32_t count;
    uint32_t index;
    uint32_t cap;
    uint32_t root_bar;
    uint64_t host_hdm = PAL_CXL_CHBCR + 0x1010U;

    if (g_pal_cxl.active || g_pal_cxl_lease == UINT64_MAX)
        return VAL_ERROR;
    memset(&g_pal_cxl, 0, sizeof(g_pal_cxl));
    g_pal_cxl.ecam_base = ecam_base;
    g_pal_cxl.root_bdf = root_bdf;
    g_pal_cxl.endpoint_bdf = endpoint_bdf;
    if (val_pcie_cfg_read32_at(ecam_base, root_bdf, 0x10U, &root_bar) != VAL_SUCCESS ||
        (root_bar & 0xfffffff0U) != PAL_CXL_ROOT_BAR ||
        pal_cxl_read32(PAL_CXL_KM_BASE) != PAL_CXL_SIGNATURE ||
        pal_cxl_read32(PAL_CXL_KM_BASE + 0x18U) != 1U)
        return VAL_ERROR;
    if (val_pcie_cfg_read32_at(ecam_base, endpoint_bdf, PAL_CXL_CFG_COMMAND,
                             &g_pal_cxl.command) != VAL_SUCCESS ||
        val_pcie_cfg_read32_at(ecam_base, endpoint_bdf, PAL_CXL_CFG_BAR4,
                             &g_pal_cxl.bar) != VAL_SUCCESS ||
        val_pcie_cfg_read32_at(ecam_base, root_bdf, PAL_CXL_CFG_MEMORY_WINDOW,
                             &g_pal_cxl.window) != VAL_SUCCESS)
        return VAL_ERROR;
    if ((g_pal_cxl.bar & 0xfffffff0U) != 0U || (g_pal_cxl.bar & 7U) != 0U ||
        (g_pal_cxl.command & PAL_CXL_MEMORY_COMMAND) != 0U)
        return VAL_ERROR;
    g_pal_cxl.dvsec = pal_cxl_device_dvsec(endpoint_bdf);
    if (!g_pal_cxl.dvsec || val_pcie_cfg_read32_at(ecam_base, endpoint_bdf,
        g_pal_cxl.dvsec + 0x0cU, &g_pal_cxl.control) != VAL_SUCCESS)
        return VAL_ERROR;

    g_pal_cxl.active = VAL_TRUE;
    g_pal_cxl.lease = ++g_pal_cxl_lease;
    if (val_pcie_cfg_write32_at(ecam_base, root_bdf, PAL_CXL_CFG_MEMORY_WINDOW,
                             UINT32_C(0x50005000)) != VAL_SUCCESS ||
        val_pcie_cfg_write32_at(ecam_base, endpoint_bdf, PAL_CXL_CFG_BAR4,
                             PAL_CXL_ENDPOINT_BAR) != VAL_SUCCESS ||
        val_pcie_cfg_write32_at(ecam_base, endpoint_bdf, PAL_CXL_CFG_COMMAND,
            (g_pal_cxl.command & 0xffffU) | PAL_CXL_MEMORY_COMMAND) != VAL_SUCCESS)
        return VAL_ERROR;
    header = pal_cxl_read32(PAL_CXL_COMPONENT_BASE);
    count = header >> 24U;
    if ((header & 0xffffU) != 1U || count == 0U || count > 64U)
        return VAL_ERROR;
    for (index = 1U; index <= count; index++) {
        cap = pal_cxl_read32(PAL_CXL_COMPONENT_BASE + index * 4U);
        if ((cap & 0xffffU) == PAL_CXL_HDM_CAP_ID) {
            g_pal_cxl.endpoint_hdm = PAL_CXL_COMPONENT_BASE + (cap >> 20U);
            break;
        }
    }
    if (!g_pal_cxl.endpoint_hdm)
        return VAL_ERROR;
    for (index = 0U; index < PAL_CXL_HDM_REG_COUNT; index++) {
        g_pal_cxl.host_registers[index] = pal_cxl_read32(host_hdm + g_pal_cxl_hdm_offsets[index]);
        g_pal_cxl.endpoint_registers[index] = pal_cxl_read32(
            g_pal_cxl.endpoint_hdm + g_pal_cxl_hdm_offsets[index]);
    }
    if (g_pal_cxl.host_registers[3] || g_pal_cxl.host_registers[4] ||
        g_pal_cxl.endpoint_registers[3] || g_pal_cxl.endpoint_registers[4])
        return VAL_ERROR;
    g_pal_cxl.decoders_saved = VAL_TRUE;
    if (val_mmu_map_device_block_el2(PAL_CXL_MEMORY) != VAL_SUCCESS)
        return VAL_ERROR;
    g_pal_cxl.mapped = VAL_TRUE;
    pal_cxl_write32(host_hdm + 0x10U, (uint32_t)PAL_CXL_MEMORY);
    pal_cxl_write32(host_hdm + 0x14U, (uint32_t)(PAL_CXL_MEMORY >> 32U));
    pal_cxl_write32(host_hdm + 0x18U, PAL_CXL_WINDOW_SIZE);
    pal_cxl_write32(host_hdm + 0x1cU, 0U);
    pal_cxl_write32(host_hdm + 0x24U, 0U);
    pal_cxl_write32(host_hdm + 0x28U, 0U);
    pal_cxl_write32(host_hdm + 0x20U, 0x200U);
    pal_cxl_write32(host_hdm + 0x04U, 2U);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x10U, (uint32_t)PAL_CXL_MEMORY);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x14U, (uint32_t)(PAL_CXL_MEMORY >> 32U));
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x18U, PAL_CXL_WINDOW_SIZE);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x1cU, 0U);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x24U, 0U);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x28U, 0U);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x20U, 0x200U);
    pal_cxl_write32(g_pal_cxl.endpoint_hdm + 0x04U, 2U);
    if (val_pcie_cfg_write32_at(ecam_base, endpoint_bdf, g_pal_cxl.dvsec + 0x0cU,
        (g_pal_cxl.control & 0xffffU) | 4U) != VAL_SUCCESS)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/**
 * @brief Read counters and memory coordinates only for the currently owned aperture.
 * @param snapshot Current reservation identity and completed cipher operations.
 * @return VAL_ERROR without an active mapped reservation or on an MMIO fault.
 */
int pal_cxl_traffic_snapshot(pal_cxl_traffic_snapshot_t *snapshot)
{
    if (!snapshot)
        return VAL_ERROR;
    *snapshot = (pal_cxl_traffic_snapshot_t){0};
    if (!g_pal_cxl.active || g_pal_cxl.releasing || !g_pal_cxl.mapped ||
        !g_pal_cxl.decoders_saved ||
        !g_pal_cxl.lease)
        return VAL_ERROR;
    if (val_mmio_read32_checked(PAL_CXL_KM_BASE + 0x20U,
                                 &snapshot->rx_completed) != VAL_SUCCESS ||
        val_mmio_read32_checked(PAL_CXL_KM_BASE + 0x24U,
                                 &snapshot->tx_completed) != VAL_SUCCESS ||
        val_mmio_read32_checked(PAL_CXL_KM_BASE + 0x28U,
                                 &snapshot->failures) != VAL_SUCCESS)
        return VAL_ERROR;
    snapshot->address = PAL_CXL_MEMORY;
    snapshot->lease = g_pal_cxl.lease;
    return VAL_SUCCESS;
}

/**
 * @brief Require actual cipher activity, or a link rejection rather than a GPF/MMU fault.
 * @param expect_blocked Nonzero after deliberately mismatching/stopping a key.
 * @return VAL_SUCCESS only for the specified transaction outcome.
 */
int pal_cxl_traffic_check(uint32_t expect_blocked)
{
    val_fault_info_t fault = {0};
    uint64_t read_back = 0ULL;
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t failures;
    uint64_t pattern;
    int read_status;

    if (!g_pal_cxl.active || g_pal_cxl.releasing || !g_pal_cxl.mapped)
        return VAL_ERROR;
    rx_count = pal_cxl_read32(PAL_CXL_KM_BASE + 0x20U);
    tx_count = pal_cxl_read32(PAL_CXL_KM_BASE + 0x24U);
    failures = pal_cxl_read32(PAL_CXL_KM_BASE + 0x28U);
    pattern = PAL_CXL_PROBE_PATTERN ^ ++g_pal_cxl_probe_sequence;
    if (!expect_blocked &&
        val_probe_write64(PAL_CXL_MEMORY, pattern, &fault) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "CXL.mem write failed ESR=%llx FAR=%llx",
                  (unsigned long long)fault.esr, (unsigned long long)fault.far);
        return VAL_ERROR;
    }
    read_status = val_probe_read64(PAL_CXL_MEMORY, &read_back, &fault);
    if (expect_blocked) {
        if (read_status == VAL_SUCCESS || val_fault_is_gpf(fault.esr) ||
            pal_cxl_read32(PAL_CXL_KM_BASE + 0x28U) <= failures)
            return VAL_ERROR;
        LOG_TEST(NULL, "CXL.mem blocked by link authentication/state check");
        return VAL_SUCCESS;
    }
    if (read_status != VAL_SUCCESS || read_back != pattern ||
        pal_cxl_read32(PAL_CXL_KM_BASE + 0x20U) <= rx_count ||
        pal_cxl_read32(PAL_CXL_KM_BASE + 0x24U) <= tx_count ||
        pal_cxl_read32(PAL_CXL_KM_BASE + 0x28U) != failures) {
        LOG_ERROR(NULL, "CXL.mem round-trip failed status=%d ESR=%llx data=%llx",
                  read_status, (unsigned long long)fault.esr,
                  (unsigned long long)read_back);
        return VAL_ERROR;
    }
    LOG_TEST(NULL, "CXL.mem round-trip and both-direction AES-GCM counters PASS");
    return VAL_SUCCESS;
}

/** @brief Verify decoder restoration before relinquishing its MMIO access path. */
static int pal_cxl_restore_decoder(uint64_t address, uint32_t value)
{
    uint32_t read_back;

    if (val_mmio_write32_checked(address, value) != VAL_SUCCESS ||
        val_mmio_read32_checked(address, &read_back) != VAL_SUCCESS ||
        read_back != value) {
        LOG_ERROR(NULL, "CXL decoder restore failed address=%llx expected=%x",
                  (unsigned long long)address, value);
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

/** @brief Verify configuration restoration without writing upper-half status bits. */
static int pal_cxl_restore_config(uint32_t bdf, uint32_t offset, uint32_t value,
                                  uint32_t mask)
{
    uint32_t read_back;

    if (val_pcie_cfg_write32_at(g_pal_cxl.ecam_base, bdf, offset,
                               value & mask) != VAL_SUCCESS ||
        val_pcie_cfg_read32_at(g_pal_cxl.ecam_base, bdf, offset,
                              &read_back) != VAL_SUCCESS ||
        (read_back & mask) != (value & mask)) {
        LOG_ERROR(NULL, "CXL config restore failed BDF=%x offset=%x expected=%x",
                  bdf, offset, value & mask);
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

/** @brief Retain ownership and original values until all cleanup stages succeed. */
int pal_cxl_traffic_release(void)
{
    uint32_t index;

    if (!g_pal_cxl.active)
        return VAL_SUCCESS;
    g_pal_cxl.releasing = VAL_TRUE;
    if (g_pal_cxl.decoders_saved) {
        if (pal_cxl_restore_decoder(PAL_CXL_CHBCR + 0x1014U, 0U) != VAL_SUCCESS ||
            pal_cxl_restore_decoder(g_pal_cxl.endpoint_hdm + 0x04U, 0U) != VAL_SUCCESS)
            return VAL_ERROR;
        for (index = PAL_CXL_HDM_REG_COUNT; index > 0U; index--) {
            if (pal_cxl_restore_decoder(
                    PAL_CXL_CHBCR + 0x1010U + g_pal_cxl_hdm_offsets[index - 1U],
                    g_pal_cxl.host_registers[index - 1U]) != VAL_SUCCESS ||
                pal_cxl_restore_decoder(
                    g_pal_cxl.endpoint_hdm + g_pal_cxl_hdm_offsets[index - 1U],
                    g_pal_cxl.endpoint_registers[index - 1U]) != VAL_SUCCESS)
                return VAL_ERROR;
        }
        g_pal_cxl.decoders_saved = VAL_FALSE;
    }
    if (g_pal_cxl.mapped) {
        if (val_mmu_unmap_device_block_el2(PAL_CXL_MEMORY) != VAL_SUCCESS) {
            LOG_ERROR(NULL, "CXL aperture unmap failed; ownership retained");
            return VAL_ERROR;
        }
        g_pal_cxl.mapped = VAL_FALSE;
    }
    if (pal_cxl_restore_config(g_pal_cxl.endpoint_bdf, g_pal_cxl.dvsec + 0x0cU,
                                g_pal_cxl.control, 0xffffU) != VAL_SUCCESS ||
        pal_cxl_restore_config(g_pal_cxl.endpoint_bdf, PAL_CXL_CFG_COMMAND,
                                g_pal_cxl.command, 0xffffU) != VAL_SUCCESS ||
        pal_cxl_restore_config(g_pal_cxl.endpoint_bdf, PAL_CXL_CFG_BAR4,
                                g_pal_cxl.bar, UINT32_MAX) != VAL_SUCCESS ||
        pal_cxl_restore_config(g_pal_cxl.root_bdf, PAL_CXL_CFG_MEMORY_WINDOW,
                                g_pal_cxl.window, UINT32_MAX) != VAL_SUCCESS)
        return VAL_ERROR;
    memset(&g_pal_cxl, 0, sizeof(g_pal_cxl));
    return VAL_SUCCESS;
}
