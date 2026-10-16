/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_pcie_ide.h"
#include "pal_pcie.h"
#include "val_log.h"

#include <stddef.h>

#define IDE_CONFIG_END 0x1000U
#define IDE_CAP_HEADER_SIZE 0xcU
#define IDE_LINK_SUPPORTED 0x1U
#define IDE_SELECTIVE_SUPPORTED 0x2U
#define IDE_LINK_COUNT_SHIFT 13U
#define IDE_LINK_COUNT_MASK 0x7U
#define IDE_SELECTIVE_COUNT_SHIFT 16U
#define IDE_SELECTIVE_COUNT_MASK 0xffU
#define IDE_LINK_SIZE 8U
#define IDE_BLOCK_SIZE 20U
#define IDE_ADDR_SIZE 12U
#define IDE_ADDR_COUNT_MASK 0xfU
#define IDE_CONTROL 4U
#define IDE_STATUS 8U
#define IDE_RID_LIMIT 12U
#define IDE_RID_BASE 16U
#define IDE_STREAM_SHIFT 24U
#define IDE_STREAM_COUNT 256U
#define IDE_RID_SHIFT 8U
#define IDE_RID_VALID 1U
#define IDE_STATE_MASK 0xfU
#define IDE_DEFAULT_STREAM (1U << 22)
#define IDE_STREAM_CONFIGURED UINT32_C(0xffffc001)
#define IDE_DVSEC_VENDOR_ARM 0x13b5U
#define IDE_DVSEC_RMEDA 0xff01U
#define IDE_RMEDA_SIZE 0x14U
#define IDE_RMEDA_LOCK 0x10U
#define IDE_RMEDA_LOCK_COUNT 32U
#define IDE_EXT_CAP_START 0x100U
#define IDE_EXT_CAP_NEXT_SHIFT 20U
#define IDE_EXT_CAP_ID_MASK 0xffffU

static uint32_t g_ide_stream_owned;

/**
 * @brief Read configuration data, rejecting absent devices and failed reads.
 * @param bdf PCI function.
 * @param offset Aligned configuration offset.
 * @param value Output register value.
 * @return VAL_SUCCESS on a completed, present-device read.
 */
static int val_pcie_ide_read(uint32_t bdf, uint32_t offset, uint32_t *value)
{
    if (val_pcie_cfg_read32(bdf, offset, value) != VAL_SUCCESS || *value == UINT32_MAX)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/**
 * @brief Write a defined configuration field and verify write acceptance.
 * @param port Port owning the register.
 * @param offset Absolute configuration offset.
 * @param value Expected complete register value.
 * @return VAL_ERROR on a rejected write or mismatching readback.
 */
static int val_pcie_ide_write(const val_pcie_ide_port_t *port,
                              uint32_t offset, uint32_t value)
{
    uint32_t readback = 0U;

    if (val_pcie_cfg_write32(port->bdf, offset, value) != VAL_SUCCESS ||
        val_pcie_cfg_read32(port->bdf, offset, &readback) != VAL_SUCCESS ||
        readback != value)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/**
 * @brief Locate RMEDA without mistaking an unrelated DVSEC for Arm controls.
 * @param bdf Root Port function.
 * @param offset Returned lock-register offset, zero when RMEDA is absent.
 * @param error_offset Failed configuration offset when inspection fails.
 * @return VAL_ERROR on a malformed capability chain or inaccessible register.
 */
static int val_pcie_ide_rmeda(uint32_t bdf, uint32_t *offset,
                              uint32_t *error_offset)
{
    uint32_t visited[32] = {0};
    uint32_t current = IDE_EXT_CAP_START;
    uint32_t header = 0U;
    uint32_t vendor = 0U;
    uint32_t identifier = 0U;
    uint32_t next = 0U;

    *offset = 0U;
    while (current != 0U) {
        *error_offset = current;
        if (visited[current / 128U] & (1U << ((current / 4U) % 32U)))
            return VAL_ERROR;
        visited[current / 128U] |= 1U << ((current / 4U) % 32U);
        if (val_pcie_ide_read(bdf, current, &header) != VAL_SUCCESS)
            return VAL_ERROR;
        if (header == 0U)
            return VAL_SUCCESS;
        next = header >> IDE_EXT_CAP_NEXT_SHIFT;
        if (next && (next < IDE_EXT_CAP_START || next >= IDE_CONFIG_END || (next & 3U)))
            return VAL_ERROR;
        if ((header & IDE_EXT_CAP_ID_MASK) == VAL_PCIE_EXT_CAP_ID_DVSEC) {
            if (current > IDE_CONFIG_END - 12U ||
                val_pcie_ide_read(bdf, current + 4U, &vendor) != VAL_SUCCESS ||
                val_pcie_ide_read(bdf, current + 8U, &identifier) != VAL_SUCCESS)
                return VAL_ERROR;
            if ((vendor & IDE_EXT_CAP_ID_MASK) == IDE_DVSEC_VENDOR_ARM &&
                (identifier & IDE_EXT_CAP_ID_MASK) == IDE_DVSEC_RMEDA) {
                if ((vendor >> 20U) < IDE_RMEDA_SIZE ||
                    current > IDE_CONFIG_END - (vendor >> 20U) ||
                    (next > current && next < current + (vendor >> 20U)))
                    return VAL_ERROR;
                *offset = current + IDE_RMEDA_LOCK;
                return VAL_SUCCESS;
            }
        }
        current = next;
    }
    return VAL_SUCCESS;
}

/**
 * @brief Bound an IDE block by the nearest higher capability, regardless of chain order.
 * @param bdf PCI function.
 * @param capability IDE capability offset.
 * @param end Output end of its configuration-space allocation.
 * @return VAL_ERROR on malformed or cyclic links.
 */
static int val_pcie_ide_boundary(uint32_t bdf, uint32_t capability, uint32_t *end)
{
    uint32_t visited[32] = {0};
    uint32_t current = IDE_EXT_CAP_START;
    uint32_t header = 0U;

    *end = IDE_CONFIG_END;
    while (current != 0U) {
        if (current < IDE_EXT_CAP_START || current >= IDE_CONFIG_END || (current & 3U) ||
            (visited[current / 128U] & (1U << ((current / 4U) % 32U))))
            return VAL_ERROR;
        visited[current / 128U] |= 1U << ((current / 4U) % 32U);
        if (val_pcie_ide_read(bdf, current, &header) != VAL_SUCCESS || !header)
            return VAL_ERROR;
        if (current > capability && current < *end)
            *end = current;
        current = header >> IDE_EXT_CAP_NEXT_SHIFT;
    }
    return VAL_SUCCESS;
}

/**
 * @brief Distinguish a missing IDE extended capability from a broken read.
 * @param bdf Present PCIe function to inspect.
 * @param capability Returned IDE capability offset.
 * @param error_offset Last inspected configuration offset on failure.
 * @return PASS when IDE is found, SKIP when absent, FAIL on malformed reads.
 */
static uint32_t val_pcie_ide_capability(uint32_t bdf, uint32_t *capability,
                                        uint32_t *error_offset)
{
    uint32_t visited[32] = {0};
    uint32_t current = IDE_EXT_CAP_START;
    uint32_t header = 0U;

    *capability = 0U;
    while (current != 0U) {
        *error_offset = current;
        if (current < IDE_EXT_CAP_START || current >= IDE_CONFIG_END ||
            (current & 3U) ||
            (visited[current / 128U] & (1U << ((current / 4U) % 32U))))
            return ACS_STATUS_FAIL;
        visited[current / 128U] |= 1U << ((current / 4U) % 32U);
        if (val_pcie_cfg_read32(bdf, current, &header) != VAL_SUCCESS ||
            header == UINT32_MAX)
            return ACS_STATUS_FAIL;
        if (header == 0U)
            return current == IDE_EXT_CAP_START ? ACS_STATUS_SKIP : ACS_STATUS_FAIL;
        if ((header & IDE_EXT_CAP_ID_MASK) == VAL_PCIE_EXT_CAP_ID_IDE) {
            *capability = current;
            return ACS_STATUS_PASS;
        }
        current = header >> IDE_EXT_CAP_NEXT_SHIFT;
    }
    return ACS_STATUS_SKIP;
}

/**
 * @brief Select an unconfigured block and collect IDs already used on a port.
 * @param port Selection destination with its BDF initialized.
 * @param used_ids Shared map covering both partner ports.
 * @param error_offset Failed configuration offset, when inspection fails.
 * @return PASS for a usable block, SKIP for unavailable resources, FAIL on errors.
 */
static uint32_t val_pcie_ide_select(val_pcie_ide_port_t *port, uint8_t *used_ids,
                                    uint32_t *error_offset)
{
    uint32_t capability = 0U;
    uint32_t header = 0U;
    uint32_t capabilities = 0U;
    uint32_t offset = 0U;
    uint32_t end = IDE_CONFIG_END;
    uint32_t count = 0U;
    uint32_t index = 0U;
    uint32_t address_count = 0U;
    uint32_t address_index = 0U;
    uint32_t control = 0U;
    uint32_t status = 0U;
    uint32_t rid_limit = 0U;
    uint32_t rid_base = 0U;
    uint32_t address = 0U;
    uint32_t locks = 0U;
    uint32_t configured = 0U;
    uint32_t link_count = 0U;

    uint32_t probe_result = val_pcie_ide_capability(port->bdf, &capability,
                                                     error_offset);

    if (probe_result != ACS_STATUS_PASS)
        return probe_result;
    if (capability > IDE_CONFIG_END - IDE_CAP_HEADER_SIZE ||
        val_pcie_ide_read(port->bdf, capability, &header) != VAL_SUCCESS ||
        val_pcie_ide_read(port->bdf, capability + 4U, &capabilities) != VAL_SUCCESS)
        return ACS_STATUS_FAIL;
    if (!(capabilities & IDE_SELECTIVE_SUPPORTED))
        return ACS_STATUS_SKIP;
    if (val_pcie_ide_boundary(port->bdf, capability, &end) != VAL_SUCCESS)
        return ACS_STATUS_FAIL;
    if (port->lock_register &&
        val_pcie_cfg_read32(port->bdf, port->lock_register, &locks) != VAL_SUCCESS)
        return ACS_STATUS_FAIL;

    count = ((capabilities >> IDE_SELECTIVE_COUNT_SHIFT) &
              IDE_SELECTIVE_COUNT_MASK) + 1U;
    offset = capability + IDE_CAP_HEADER_SIZE;
    if (capabilities & IDE_LINK_SUPPORTED) {
        link_count = ((capabilities >> IDE_LINK_COUNT_SHIFT) & IDE_LINK_COUNT_MASK) + 1U;
        for (index = 0U; index < link_count; index++) {
            if (offset > end || end - offset < IDE_LINK_SIZE ||
                val_pcie_ide_read(port->bdf, offset, &control) != VAL_SUCCESS)
                return ACS_STATUS_FAIL;
            used_ids[control >> IDE_STREAM_SHIFT] = VAL_TRUE;
            offset += IDE_LINK_SIZE;
        }
    }
    for (index = 0U; index < count; index++) {
        if (offset > end || end - offset < IDE_BLOCK_SIZE ||
            val_pcie_ide_read(port->bdf, offset, &header) != VAL_SUCCESS)
            return ACS_STATUS_FAIL;
        address_count = header & IDE_ADDR_COUNT_MASK;
        if (address_count * IDE_ADDR_SIZE > end - offset - IDE_BLOCK_SIZE ||
            val_pcie_ide_read(port->bdf, offset + IDE_CONTROL, &control) != VAL_SUCCESS ||
            val_pcie_ide_read(port->bdf, offset + IDE_STATUS, &status) != VAL_SUCCESS ||
            val_pcie_ide_read(port->bdf, offset + IDE_RID_LIMIT, &rid_limit) != VAL_SUCCESS ||
            val_pcie_ide_read(port->bdf, offset + IDE_RID_BASE, &rid_base) != VAL_SUCCESS)
            return ACS_STATUS_FAIL;
        configured = (control & IDE_STREAM_CONFIGURED) | rid_limit | rid_base |
                     (status & IDE_STATE_MASK);
        for (address_index = 0U; address_index < address_count; address_index++) {
            if (val_pcie_ide_read(port->bdf, offset + IDE_BLOCK_SIZE +
                                  address_index * IDE_ADDR_SIZE, &address) != VAL_SUCCESS)
                return ACS_STATUS_FAIL;
            configured |= address & IDE_RID_VALID;
        }
        if (configured)
            used_ids[control >> IDE_STREAM_SHIFT] = VAL_TRUE;
        if (!configured && !port->block &&
            (!port->lock_register || (index < IDE_RMEDA_LOCK_COUNT &&
              !(locks & (1U << index))))) {
            port->block = offset;
            port->index = index;
            port->original_control = control;
        }
        offset += IDE_BLOCK_SIZE + address_count * IDE_ADDR_SIZE;
    }
    return port->block ? ACS_STATUS_PASS : ACS_STATUS_SKIP;
}

/**
 * @brief Change only the selected stream's RMEDA lock, from the Realm caller.
 * @param port Root Port whose initially clear lock belongs to ACS.
 * @param lock Nonzero acquires the lock; zero releases it.
 * @return VAL_ERROR when read-modify-write/readback cannot complete.
 */
static int val_pcie_ide_lock(val_pcie_ide_port_t *port, uint32_t lock)
{
    uint32_t value = 0U;
    uint32_t mask = 0U;

    if (!port->lock_register)
        return VAL_SUCCESS;
    if (port->index >= IDE_RMEDA_LOCK_COUNT)
        return VAL_ERROR;
    mask = 1U << port->index;
    if (val_pcie_cfg_read32(port->bdf, port->lock_register, &value) != VAL_SUCCESS)
        return VAL_ERROR;
    if (lock) {
        if (value & mask)
            return VAL_ERROR;
        port->lock_owned = VAL_TRUE;
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (val_pcie_ide_write(port, port->lock_register, value) != VAL_SUCCESS)
        return VAL_ERROR;
    port->lock_owned = lock;
    return VAL_SUCCESS;
}

/**
 * @brief Release configuration only after the caller drained and stopped keys.
 * @param stream Acquired pair; failed cleanup retains ownership for retry.
 * @return VAL_ERROR on any cleanup error, otherwise VAL_SUCCESS.
 */
static int val_pcie_ide_restore(val_pcie_ide_stream_t *stream)
{
    val_pcie_ide_port_t *port = NULL;
    uint32_t index = 0U;

    if (!stream || !stream->acquired)
        return VAL_ERROR;
    for (index = 0U; index < VAL_PCIE_IDE_PORT_COUNT; index++) {
        port = &stream->ports[index];
        if (port->lock_owned && val_pcie_ide_lock(port, VAL_FALSE) != VAL_SUCCESS)
            return VAL_ERROR;
        if (port->changed) {
            if (val_pcie_ide_write(port, port->block + IDE_CONTROL,
                                   port->original_control) != VAL_SUCCESS ||
                val_pcie_ide_write(port, port->block + IDE_RID_BASE, 0U) != VAL_SUCCESS ||
                val_pcie_ide_write(port, port->block + IDE_RID_LIMIT, 0U) != VAL_SUCCESS)
                return VAL_ERROR;
            port->changed = VAL_FALSE;
        }
    }
    if (stream->buses_changed) {
        if (val_pcie_ide_write(&stream->ports[VAL_PCIE_IDE_ROOT],
                               VAL_PCIE_CFG_BUS_NUMBERS, stream->original_buses) != VAL_SUCCESS)
            return VAL_ERROR;
        stream->buses_changed = VAL_FALSE;
    }
    stream->acquired = VAL_FALSE;
    return VAL_SUCCESS;
}

/**
 * @brief Restore an owned pair and release the shared allocator only on success.
 * @param stream Acquired stream, retained when cleanup fails.
 * @return VAL_ERROR on any cleanup error, otherwise VAL_SUCCESS.
 */
int val_pcie_ide_stream_release(val_pcie_ide_stream_t *stream)
{
    if (val_pcie_ide_restore(stream) != VAL_SUCCESS)
        return VAL_ERROR;
    __atomic_store_n(&g_ide_stream_owned, VAL_FALSE, __ATOMIC_RELEASE);
    return VAL_SUCCESS;
}

/**
 * @brief Prepare one inactive Selective IDE pair on an exclusively ACS-owned path.
 * @param path Enumerated Root Port and downstream endpoint, not PAL stream IDs.
 * @param realm_caller Nonzero only when executing in the Realm payload.
 * @param stream Caller-owned configuration retained until keys are stopped.
 * @return PASS on setup, SKIP for absent resources, FAIL for access/cleanup errors.
 */
static uint32_t val_pcie_ide_configure(const val_pcie_ide_path_t *path,
                                      uint32_t realm_caller,
                                      val_pcie_ide_stream_t *stream)
{
    uint8_t used_ids[IDE_STREAM_COUNT] = {0};
    val_pcie_ide_port_t *port = NULL;
    uint32_t result = ACS_STATUS_FAIL;
    uint32_t index = 0U;
    uint32_t peer = 0U;
    uint32_t control = 0U;

    if (!path || !stream)
        return ACS_STATUS_FAIL;
    if (!path->root_port_present || !path->endpoint_present)
        return ACS_STATUS_SKIP;
    *stream = (val_pcie_ide_stream_t){0};
    stream->path = *path;
    stream->acquired = VAL_TRUE;
    stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
    stream->ports[VAL_PCIE_IDE_ROOT].bdf = path->root_port_bdf;
    stream->ports[VAL_PCIE_IDE_ENDPOINT].bdf = path->endpoint_bdf;
    if (val_pcie_ide_rmeda(path->root_port_bdf,
                          &stream->ports[VAL_PCIE_IDE_ROOT].lock_register,
                          &stream->error_offset) != VAL_SUCCESS) {
        stream->error_bdf = path->root_port_bdf;
        goto fail;
    }
    if (stream->ports[VAL_PCIE_IDE_ROOT].lock_register && !realm_caller) {
        stream->reason = VAL_PCIE_IDE_REALM_REQUIRED;
        result = ACS_STATUS_SKIP;
        goto fail;
    }
    for (index = 0U; index < VAL_PCIE_IDE_PORT_COUNT; index++) {
        stream->reason = index == VAL_PCIE_IDE_ROOT ?
                         VAL_PCIE_IDE_ROOT_UNAVAILABLE : VAL_PCIE_IDE_ENDPOINT_UNAVAILABLE;
        result = val_pcie_ide_select(&stream->ports[index], used_ids,
                                     &stream->error_offset);
        if (result != ACS_STATUS_PASS) {
            if (result == ACS_STATUS_FAIL) {
                stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
                stream->error_bdf = stream->ports[index].bdf;
            }
            goto fail;
        }
    }
    for (index = 1U; index < IDE_STREAM_COUNT; index++) {
        if (!used_ids[index]) {
            stream->stream_id = index;
            break;
        }
    }
    if (!stream->stream_id) {
        stream->reason = VAL_PCIE_IDE_NO_STREAM_ID;
        result = ACS_STATUS_SKIP;
        goto fail;
    }
    result = ACS_STATUS_FAIL;
    stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
    for (index = 0U; index < VAL_PCIE_IDE_PORT_COUNT; index++) {
        port = &stream->ports[index];
        peer = val_pcie_requester_id_from_bdf(stream->ports[1U - index].bdf);
        if (peer == VAL_PCIE_REQ_ID_INVALID)
            goto fail;
        port->changed = VAL_TRUE;
        control = port->original_control | (stream->stream_id << IDE_STREAM_SHIFT);
        if (index == VAL_PCIE_IDE_ENDPOINT)
            control |= IDE_DEFAULT_STREAM;
        if (val_pcie_ide_write(port, port->block + IDE_RID_LIMIT,
                               peer << IDE_RID_SHIFT) != VAL_SUCCESS ||
            val_pcie_ide_write(port, port->block + IDE_RID_BASE,
                               (peer << IDE_RID_SHIFT) | IDE_RID_VALID) != VAL_SUCCESS ||
            val_pcie_ide_write(port, port->block + IDE_CONTROL, control) != VAL_SUCCESS)
            goto fail;
    }
    if (val_pcie_ide_lock(&stream->ports[VAL_PCIE_IDE_ROOT], VAL_TRUE) != VAL_SUCCESS)
        goto fail;
    stream->reason = 0U;
    return ACS_STATUS_PASS;

fail:
    if (val_pcie_ide_restore(stream) != VAL_SUCCESS)
        return ACS_STATUS_FAIL;
    return result;
}

/**
 * @brief Reserve a pair on an already enumerated ACS-owned PCIe path.
 * @param path Existing bus topology.
 * @param realm_caller Nonzero in Realm execution.
 * @param stream Output context retained until all pending key operations stop.
 * @return PASS on preparation, SKIP for unavailable resources, FAIL on errors.
 */
uint32_t val_pcie_ide_stream_acquire(const val_pcie_ide_path_t *path,
                                    uint32_t realm_caller,
                                    val_pcie_ide_stream_t *stream)
{
    uint32_t expected = VAL_FALSE;
    uint32_t result = ACS_STATUS_FAIL;

    if (!path || !stream)
        return ACS_STATUS_FAIL;
    if (!__atomic_compare_exchange_n(&g_ide_stream_owned, &expected, VAL_TRUE,
                                     VAL_FALSE, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        *stream = (val_pcie_ide_stream_t){.reason = VAL_PCIE_IDE_OWNER_BUSY};
        return ACS_STATUS_FAIL;
    }
    *stream = (val_pcie_ide_stream_t){0};
    result = val_pcie_ide_configure(path, realm_caller, stream);
    if (result == ACS_STATUS_PASS && !stream->acquired) {
        stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
        result = ACS_STATUS_FAIL;
    }
    if (result != ACS_STATUS_PASS && !stream->acquired)
        __atomic_store_n(&g_ide_stream_owned, VAL_FALSE, __ATOMIC_RELEASE);
    return result;
}

/**
 * @brief Find a bus not occupied by a function or any configured bridge window.
 * @param info Platform ECAM allocation.
 * @param root Root Port to enumerate.
 * @param free_bus Returned bus above the Root Port, if available.
 * @param error_bdf Function whose configuration access failed.
 * @param error_offset Failed configuration offset.
 * @return PASS for a free bus, SKIP if none exists, FAIL on read error.
 */
static uint32_t val_pcie_ide_free_bus(const pal_pcie_ecam_info_t *info, uint32_t root,
                                      uint32_t *free_bus, uint32_t *error_bdf,
                                      uint32_t *error_offset)
{
    uint8_t used[256] = {0};
    val_pcie_probe_status_t probe;
    uint32_t bus = 0U;
    uint32_t device = 0U;
    uint32_t function = 0U;
    uint32_t bdf = 0U;
    uint32_t header = 0U;
    uint32_t buses = 0U;
    uint32_t first = 0U;
    uint32_t last = 0U;
    uint32_t index = 0U;

    *free_bus = 0U;
    for (bus = info->start_bus; bus <= info->end_bus; bus++) {
        for (device = 0U; device < 32U; device++) {
            for (function = 0U; function < 8U; function++) {
                bdf = val_pcie_bdf_create(info->segment, bus, device, function);
                probe = val_pcie_function_probe(bdf, error_offset);
                if (probe == VAL_PCIE_PROBE_ERROR) {
                    *error_bdf = bdf;
                    return ACS_STATUS_FAIL;
                }
                if (probe == VAL_PCIE_PROBE_ABSENT)
                    continue;
                used[bus] = VAL_TRUE;
                if (val_pcie_cfg_read32(bdf, VAL_PCIE_CFG_HEADER_TYPE, &header) != VAL_SUCCESS) {
                    *error_bdf = bdf;
                    *error_offset = VAL_PCIE_CFG_HEADER_TYPE;
                    return ACS_STATUS_FAIL;
                }
                if (((header >> 16U) & 0x7fU) != 1U)
                    continue;
                if (val_pcie_cfg_read32(bdf, VAL_PCIE_CFG_BUS_NUMBERS, &buses) != VAL_SUCCESS) {
                    *error_bdf = bdf;
                    *error_offset = VAL_PCIE_CFG_BUS_NUMBERS;
                    return ACS_STATUS_FAIL;
                }
                first = (buses >> 8U) & 0xffU;
                last = (buses >> 16U) & 0xffU;
                for (index = first; index <= last; index++)
                    used[index] = VAL_TRUE;
            }
        }
    }
    for (bus = val_pcie_bdf_bus(root) + 1U; bus <= info->end_bus; bus++) {
        if (!used[bus]) {
            *free_bus = bus;
            return ACS_STATUS_PASS;
        }
    }
    return ACS_STATUS_SKIP;
}

/**
 * @brief Try downstream endpoints without assuming the first one supports IDE.
 * @param root Enumerated Root Port.
 * @param info Its platform ECAM allocation.
 * @param realm_caller Nonzero in Realm execution.
 * @param stream Resulting stream reservation.
 * @return PASS for a prepared pair, SKIP when none is usable, FAIL on setup errors.
 */
static uint32_t val_pcie_ide_find_endpoint(uint32_t root, const pal_pcie_ecam_info_t *info,
                                         uint32_t realm_caller,
                                         val_pcie_ide_stream_t *stream)
{
    val_pcie_ide_path_t path = {.ecam_base = info->ecam_base, .segment = info->segment,
        .root_port_bdf = root, .root_port_id = val_pcie_requester_id_from_bdf(root),
        .root_port_present = VAL_TRUE};
    uint32_t buses = 0U;
    uint32_t first = 0U;
    uint32_t last = 0U;
    uint32_t bus = 0U;
    uint32_t device = 0U;
    uint32_t function = 0U;
    uint32_t port_type = 0U;
    uint32_t result = ACS_STATUS_SKIP;
    uint32_t free_bus_result = ACS_STATUS_SKIP;
    uint32_t original_buses = 0U;
    uint32_t buses_changed = VAL_FALSE;
    uint32_t error_offset = 0U;
    val_pcie_probe_status_t probe;
    val_pcie_ide_port_t root_port = {.bdf = root};

    stream->path = path;
    stream->reason = VAL_PCIE_IDE_NO_ENDPOINT;
    if (val_pcie_cfg_read32(root, VAL_PCIE_CFG_BUS_NUMBERS, &buses) != VAL_SUCCESS) {
        stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
        stream->error_bdf = root;
        stream->error_offset = VAL_PCIE_CFG_BUS_NUMBERS;
        return ACS_STATUS_FAIL;
    }
    first = (buses >> 8U) & 0xffU;
    last = (buses >> 16U) & 0xffU;
    original_buses = buses;
    if (!first && !last) {
        free_bus_result = val_pcie_ide_free_bus(info, root, &first,
                                                &stream->error_bdf,
                                                &stream->error_offset);
        if (free_bus_result != ACS_STATUS_PASS) {
            if (free_bus_result == ACS_STATUS_FAIL)
                stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
            return free_bus_result;
        }
        last = first;
        buses_changed = VAL_TRUE;
        buses = (buses & 0xff000000U) | val_pcie_bdf_bus(root) |
                (first << 8U) | (last << 16U);
        if (val_pcie_ide_write(&root_port, VAL_PCIE_CFG_BUS_NUMBERS, buses) != VAL_SUCCESS) {
            result = ACS_STATUS_FAIL;
            goto restore_bus;
        }
    }
    if (!first || first > last || first < info->start_bus || last > info->end_bus)
        return ACS_STATUS_SKIP;
    for (bus = first; bus <= last; bus++) {
        for (device = 0U; device < 32U; device++) {
            for (function = 0U; function < 8U; function++) {
                path.endpoint_bdf = val_pcie_bdf_create(info->segment, bus, device, function);
                probe = val_pcie_device_port_type_probe(path.endpoint_bdf, &port_type,
                                                         &error_offset);
                if (probe == VAL_PCIE_PROBE_ERROR) {
                    stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
                    stream->error_bdf = path.endpoint_bdf;
                    stream->error_offset = error_offset;
                    result = ACS_STATUS_FAIL;
                    goto restore_bus;
                }
                if (probe != VAL_PCIE_PROBE_PRESENT ||
                    (port_type != VAL_PCIE_PORT_TYPE_ENDPOINT &&
                     port_type != VAL_PCIE_PORT_TYPE_LEGACY_ENDPOINT))
                    continue;
                path.endpoint_id = val_pcie_requester_id_from_bdf(path.endpoint_bdf);
                path.endpoint_present = VAL_TRUE;
                result = val_pcie_ide_configure(&path, realm_caller, stream);
                LOG_TEST(NULL, "Selective IDE candidate ECAM=%llx RP=%x EP=%x "
                         "result=%u reason=%u stream=%u\n",
                         (unsigned long long)path.ecam_base, path.root_port_bdf,
                         path.endpoint_bdf, result, stream->reason, stream->stream_id);
                if (result != ACS_STATUS_SKIP)
                    goto restore_bus;
            }
        }
    }
restore_bus:
    if (buses_changed) {
        if (stream->acquired) {
            stream->original_buses = original_buses;
            stream->buses_changed = VAL_TRUE;
        } else if (val_pcie_ide_write(&root_port, VAL_PCIE_CFG_BUS_NUMBERS,
                                      original_buses) != VAL_SUCCESS) {
            stream->ports[VAL_PCIE_IDE_ROOT].bdf = root;
            stream->acquired = VAL_TRUE;
            stream->original_buses = original_buses;
            stream->buses_changed = VAL_TRUE;
            return ACS_STATUS_FAIL;
        }
    }
    return result;
}

/**
 * @brief Discover Selective IDE resources using only PAL ECAM inventory.
 * @param realm_caller Nonzero only in Realm, which can update RMEDA controls.
 * @param stream Caller-owned output; failed reservations remain quarantined.
 * @return PASS for one usable pair, SKIP for no resources, FAIL for setup errors.
 */
uint32_t val_pcie_ide_stream_discover(uint32_t realm_caller,
                                     val_pcie_ide_stream_t *stream)
{
    pal_pcie_ecam_info_t info = {0};
    uint32_t allocation = 0U;
    uint32_t bus = 0U;
    uint32_t device = 0U;
    uint32_t function = 0U;
    uint32_t bdf = 0U;
    uint32_t port_type = 0U;
    uint32_t result = ACS_STATUS_SKIP;
    uint32_t expected = VAL_FALSE;
    uint32_t error_offset = 0U;
    val_pcie_probe_status_t probe;

    if (!stream)
        return ACS_STATUS_FAIL;
    if (!__atomic_compare_exchange_n(&g_ide_stream_owned, &expected, VAL_TRUE,
                                     VAL_FALSE, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        *stream = (val_pcie_ide_stream_t){.reason = VAL_PCIE_IDE_OWNER_BUSY};
        return ACS_STATUS_FAIL;
    }
    *stream = (val_pcie_ide_stream_t){.reason = VAL_PCIE_IDE_NO_ROOT};
    for (allocation = 0U; allocation < pal_pcie_ecam_count(); allocation++) {
        if (pal_pcie_ecam_info(allocation, &info) != VAL_SUCCESS ||
            info.start_bus > info.end_bus || info.end_bus > 255U || info.segment > 255U) {
            result = ACS_STATUS_FAIL;
            goto done;
        }
        for (bus = info.start_bus; bus <= info.end_bus; bus++) {
            for (device = 0U; device < 32U; device++) {
                for (function = 0U; function < 8U; function++) {
                    bdf = val_pcie_bdf_create(info.segment, bus, device, function);
                    probe = val_pcie_device_port_type_probe(bdf, &port_type,
                                                             &error_offset);
                    if (probe == VAL_PCIE_PROBE_ERROR) {
                        stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
                        stream->error_bdf = bdf;
                        stream->error_offset = error_offset;
                        result = ACS_STATUS_FAIL;
                        goto done;
                    }
                    if (probe != VAL_PCIE_PROBE_PRESENT ||
                        port_type != VAL_PCIE_PORT_TYPE_ROOT_PORT)
                        continue;
                    result = val_pcie_ide_find_endpoint(bdf, &info, realm_caller, stream);
                    if (result != ACS_STATUS_SKIP)
                        goto done;
                }
            }
        }
    }
done:
    if (result == ACS_STATUS_PASS && !stream->acquired) {
        stream->reason = VAL_PCIE_IDE_CONFIG_ERROR;
        result = ACS_STATUS_FAIL;
    }
    if (result != ACS_STATUS_PASS && !stream->acquired)
        __atomic_store_n(&g_ide_stream_owned, VAL_FALSE, __ATOMIC_RELEASE);
    return result;
}
