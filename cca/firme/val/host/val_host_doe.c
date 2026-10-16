/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#if FIRME_ENABLE_CXL_ENDPOINT_PROBE

#include "val_host_doe.h"

#include "pal_pcie.h"
#include "val_common/val_status.h"
#include "val_log.h"
#include "val_pcie.h"

#define VAL_HOST_DOE_CTRL_OFFSET             0x08U
#define VAL_HOST_DOE_STATUS_OFFSET           0x0cU
#define VAL_HOST_DOE_WRITE_OFFSET            0x10U
#define VAL_HOST_DOE_READ_OFFSET             0x14U
#define VAL_HOST_DOE_CTRL_ABORT              0x00000001U
#define VAL_HOST_DOE_CTRL_GO                 0x80000000U
#define VAL_HOST_DOE_STATUS_BUSY             0x00000001U
#define VAL_HOST_DOE_STATUS_ERROR            0x00000004U
#define VAL_HOST_DOE_STATUS_READY            0x80000000U
#define VAL_HOST_DOE_POLL_LIMIT              1000000U
#define VAL_HOST_DOE_MAX_OBJECT_BYTES        8192U
#define VAL_HOST_DOE_DISCOVERY_LIMIT         32U
#define VAL_HOST_DOE_HEADER_BYTES            8U
#define VAL_HOST_DOE_PCISIG_VENDOR           0x0001U
#define VAL_HOST_DOE_SPDM_TYPE               0x01U
#define VAL_HOST_DOE_SECURED_SPDM_TYPE       0x02U

/**
 * @brief Read one register from a discovered DOE capability.
 * @param endpoint Discovered endpoint and DOE offset.
 * @param offset Register offset within the DOE capability.
 * @param value Returned register value.
 * @return VAL_SUCCESS on a valid ECAM read, otherwise VAL_ERROR.
 */
static int val_host_doe_read_reg(const val_host_doe_endpoint_t *endpoint,
                                 uint32_t offset, uint32_t *value)
{
    if (!endpoint || !value)
        return VAL_ERROR;

    return val_pcie_cfg_read32_at(endpoint->ecam_base, endpoint->endpoint_bdf,
                               endpoint->doe_offset + offset, value);
}

/**
 * @brief Write one register in a discovered DOE capability.
 * @param endpoint Discovered endpoint and DOE offset.
 * @param offset Register offset within the DOE capability.
 * @param value Register value.
 * @return VAL_SUCCESS on a valid ECAM write, otherwise VAL_ERROR.
 */
static int val_host_doe_write_reg(const val_host_doe_endpoint_t *endpoint,
                                  uint32_t offset, uint32_t value)
{
    if (!endpoint)
        return VAL_ERROR;

    return val_pcie_cfg_write32_at(endpoint->ecam_base, endpoint->endpoint_bdf,
                                endpoint->doe_offset + offset, value);
}

/**
 * @brief Stop a failed DOE transaction before reusing the mailbox.
 * @param endpoint Discovered endpoint and DOE offset.
 */
static void val_host_doe_abort(const val_host_doe_endpoint_t *endpoint)
{
    val_host_doe_write_reg(endpoint, VAL_HOST_DOE_CTRL_OFFSET,
                           VAL_HOST_DOE_CTRL_ABORT);
}

/**
 * @brief Poll until the DOE mailbox is idle or holds a complete response.
 * @param endpoint Discovered endpoint and DOE offset.
 * @param want_ready Whether a completed response is required.
 * @return VAL_SUCCESS when the requested state is reached, otherwise VAL_ERROR.
 */
static int val_host_doe_wait(const val_host_doe_endpoint_t *endpoint,
                             uint32_t want_ready)
{
    uint32_t attempt = 0U;
    uint32_t status = 0U;

    for (attempt = 0U; attempt < VAL_HOST_DOE_POLL_LIMIT; attempt++) {
        if (val_host_doe_read_reg(endpoint, VAL_HOST_DOE_STATUS_OFFSET,
                                  &status) != VAL_SUCCESS)
            return VAL_ERROR;

        if ((status & VAL_HOST_DOE_STATUS_ERROR) != 0U)
            return VAL_ERROR;

        if (((status & VAL_HOST_DOE_STATUS_BUSY) == 0U) &&
            (((status & VAL_HOST_DOE_STATUS_READY) != 0U) == (want_ready != 0U)))
            return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Store a DOE dword without assuming destination alignment.
 * @param bytes Destination buffer.
 * @param offset Byte offset of the dword.
 * @param word Dword to store in PCIe little-endian order.
 */
static void val_host_doe_store_word(uint8_t *bytes, size_t offset, uint32_t word)
{
    uint32_t index = 0U;

    for (index = 0U; index < sizeof(word); index++)
        bytes[offset + index] = (uint8_t)(word >> (index * 8U));
}

/**
 * @brief Submit one complete DOE object through the endpoint mailbox.
 * @param endpoint Discovered endpoint and DOE offset.
 * @param message DOE object, including its eight-byte header.
 * @param message_size Object size in bytes.
 * @return VAL_SUCCESS when the GO command is accepted, otherwise VAL_ERROR.
 */
int val_host_doe_send(const val_host_doe_endpoint_t *endpoint,
                      const void *message, size_t message_size)
{
    const uint8_t *bytes = (const uint8_t *)message;
    size_t offset = 0U;
    uint32_t word = 0U;
    uint32_t byte_index = 0U;

    if (!endpoint || !message || (message_size < VAL_HOST_DOE_HEADER_BYTES) ||
        (message_size > VAL_HOST_DOE_MAX_OBJECT_BYTES) ||
        (val_host_doe_wait(endpoint, VAL_FALSE) != VAL_SUCCESS))
        return VAL_ERROR;

    for (offset = 0U; offset < message_size; offset += sizeof(word)) {
        word = 0U;
        for (byte_index = 0U; byte_index < sizeof(word); byte_index++) {
            if (offset + byte_index < message_size)
                word |= (uint32_t)bytes[offset + byte_index] << (byte_index * 8U);
        }
        if (val_host_doe_write_reg(endpoint, VAL_HOST_DOE_WRITE_OFFSET,
                                   word) != VAL_SUCCESS) {
            val_host_doe_abort(endpoint);
            return VAL_ERROR;
        }
    }

    if (val_host_doe_write_reg(endpoint, VAL_HOST_DOE_CTRL_OFFSET,
                               VAL_HOST_DOE_CTRL_GO) != VAL_SUCCESS) {
        val_host_doe_abort(endpoint);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Receive one bounded DOE response from the endpoint mailbox.
 * @param endpoint Discovered endpoint and DOE offset.
 * @param message Destination buffer.
 * @param capacity Destination capacity in bytes.
 * @param message_size Returned object size in bytes.
 * @return VAL_SUCCESS for a complete object, otherwise VAL_ERROR.
 */
int val_host_doe_receive(const val_host_doe_endpoint_t *endpoint,
                         void *message, size_t capacity, size_t *message_size)
{
    uint8_t *bytes = (uint8_t *)message;
    uint32_t header[2] = {0U, 0U};
    uint32_t word = 0U;
    uint32_t total_words = 0U;
    uint32_t index = 0U;

    if (!endpoint || !message || !message_size ||
        (capacity < VAL_HOST_DOE_HEADER_BYTES))
        return VAL_ERROR;

    *message_size = 0U;
    if (val_host_doe_wait(endpoint, VAL_TRUE) != VAL_SUCCESS) {
        val_host_doe_abort(endpoint);
        return VAL_ERROR;
    }

    for (index = 0U; index < 2U; index++) {
        if ((val_host_doe_read_reg(endpoint, VAL_HOST_DOE_READ_OFFSET,
                                   &header[index]) != VAL_SUCCESS) ||
            (val_host_doe_write_reg(endpoint, VAL_HOST_DOE_READ_OFFSET, 0U)
             != VAL_SUCCESS)) {
            val_host_doe_abort(endpoint);
            return VAL_ERROR;
        }
    }

    total_words = header[1] & 0x3ffffU;
    if ((total_words < 2U) ||
        (total_words > VAL_HOST_DOE_MAX_OBJECT_BYTES / sizeof(word)) ||
        ((size_t)total_words * sizeof(word) > capacity)) {
        val_host_doe_abort(endpoint);
        return VAL_ERROR;
    }

    val_host_doe_store_word(bytes, 0U, header[0]);
    val_host_doe_store_word(bytes, sizeof(word), header[1]);
    for (index = 2U; index < total_words; index++) {
        if ((val_host_doe_read_reg(endpoint, VAL_HOST_DOE_READ_OFFSET,
                                   &word) != VAL_SUCCESS) ||
            (val_host_doe_write_reg(endpoint, VAL_HOST_DOE_READ_OFFSET, 0U)
             != VAL_SUCCESS)) {
            val_host_doe_abort(endpoint);
            return VAL_ERROR;
        }
        val_host_doe_store_word(bytes, (size_t)index * sizeof(word), word);
    }

    if (val_host_doe_wait(endpoint, VAL_FALSE) != VAL_SUCCESS) {
        val_host_doe_abort(endpoint);
        return VAL_ERROR;
    }

    *message_size = (size_t)total_words * sizeof(word);
    return VAL_SUCCESS;
}

/** @brief Choose a free bus without aliasing another function or bridge window. */
static val_pcie_probe_status_t val_host_doe_free_bus(const pal_pcie_ecam_info_t *ecam,
                                                    uint32_t root, uint32_t *free_bus)
{
    uint8_t used[256] = {0};
    uint32_t bus;
    uint32_t device;
    uint32_t function;
    uint32_t bdf;
    uint32_t value;
    uint32_t first;
    uint32_t last;
    uint32_t index;

    for (bus = ecam->start_bus; bus <= ecam->end_bus; bus++) {
        for (device = 0U; device < 32U; device++) {
            for (function = 0U; function < 8U; function++) {
                bdf = val_pcie_bdf_create(ecam->segment, bus, device, function);
                if (val_pcie_cfg_read32_at(ecam->ecam_base, bdf, VAL_PCIE_CFG_VENDOR_ID,
                                           &value) != VAL_SUCCESS)
                    return VAL_PCIE_PROBE_ERROR;
                if ((value & UINT16_MAX) == 0U || (value & UINT16_MAX) == UINT16_MAX)
                    continue;
                used[bus] = VAL_TRUE;
                if (val_pcie_cfg_read32_at(ecam->ecam_base, bdf, VAL_PCIE_CFG_HEADER_TYPE,
                                           &value) != VAL_SUCCESS)
                    return VAL_PCIE_PROBE_ERROR;
                if (((value >> 16U) & 0x7fU) != 1U)
                    continue;
                if (val_pcie_cfg_read32_at(ecam->ecam_base, bdf, VAL_PCIE_CFG_BUS_NUMBERS,
                                           &value) != VAL_SUCCESS)
                    return VAL_PCIE_PROBE_ERROR;
                first = (value >> 8U) & UINT8_MAX;
                last = (value >> 16U) & UINT8_MAX;
                for (index = first; index <= last; index++)
                    used[index] = VAL_TRUE;
            }
        }
    }

    for (bus = val_pcie_bdf_bus(root) + 1U; bus <= ecam->end_bus; bus++) {
        if (!used[bus]) {
            *free_bus = bus;
            return VAL_PCIE_PROBE_PRESENT;
        }
    }

    return VAL_PCIE_PROBE_ABSENT;
}

/** @brief Confirm bus writes before trusting enumeration or releasing restoration ownership. */
static int val_host_doe_write_buses(const val_host_doe_endpoint_t *endpoint, uint32_t buses)
{
    uint32_t readback;

    if (val_pcie_cfg_write32_at(endpoint->ecam_base, endpoint->root_bdf,
                                VAL_PCIE_CFG_BUS_NUMBERS, buses) != VAL_SUCCESS ||
        val_pcie_cfg_read32_at(endpoint->ecam_base, endpoint->root_bdf,
                               VAL_PCIE_CFG_BUS_NUMBERS, &readback) != VAL_SUCCESS ||
        readback != buses)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/** @brief Search every endpoint below one Root Port and retain only its owned bus changes. */
static val_pcie_probe_status_t val_host_doe_prepare_root(const pal_pcie_ecam_info_t *ecam,
                                                         val_host_doe_endpoint_t *endpoint)
{
    uint32_t buses;
    uint32_t first;
    uint32_t last;
    uint32_t bus;
    uint32_t device;
    uint32_t function;
    uint32_t port_type;
    uint32_t error_offset = 0U;
    val_pcie_probe_status_t probe;

    if (val_pcie_cfg_read32_at(ecam->ecam_base, endpoint->root_bdf,
                               VAL_PCIE_CFG_BUS_NUMBERS, &buses) != VAL_SUCCESS)
        return VAL_PCIE_PROBE_ERROR;
    endpoint->original_buses = buses;
    first = (buses >> 8U) & UINT8_MAX;
    last = (buses >> 16U) & UINT8_MAX;

    if (!first && !last) {
        probe = val_host_doe_free_bus(ecam, endpoint->root_bdf, &first);
        if (probe != VAL_PCIE_PROBE_PRESENT)
            return probe;
        last = first;
        buses = (buses & UINT32_C(0xff000000)) | val_pcie_bdf_bus(endpoint->root_bdf) |
                (first << 8U) | (last << 16U);
        endpoint->buses_changed = VAL_TRUE;
        if (val_host_doe_write_buses(endpoint, buses) != VAL_SUCCESS)
            return VAL_PCIE_PROBE_ERROR;
    }

    if ((buses & UINT8_MAX) != val_pcie_bdf_bus(endpoint->root_bdf) ||
        first <= val_pcie_bdf_bus(endpoint->root_bdf) || first > last ||
        first < ecam->start_bus || last > ecam->end_bus)
        return VAL_PCIE_PROBE_ERROR;
    for (bus = first; bus <= last; bus++) {
        for (device = 0U; device < 32U; device++) {
            for (function = 0U; function < 8U; function++) {
                endpoint->endpoint_bdf = val_pcie_bdf_create(ecam->segment, bus,
                                                             device, function);
                probe = val_pcie_device_port_type_probe_at(ecam->ecam_base,
                    endpoint->endpoint_bdf, &port_type, &error_offset);
                if (probe == VAL_PCIE_PROBE_ERROR)
                    goto failure;
                if (probe == VAL_PCIE_PROBE_ABSENT ||
                    (port_type != VAL_PCIE_PORT_TYPE_ENDPOINT &&
                     port_type != VAL_PCIE_PORT_TYPE_LEGACY_ENDPOINT))
                    continue;
                probe = val_pcie_cxl_doe_probe_at(ecam->ecam_base, endpoint->endpoint_bdf,
                                                  &endpoint->doe_offset, &error_offset);
                if (probe == VAL_PCIE_PROBE_ERROR)
                    goto failure;
                if (probe == VAL_PCIE_PROBE_PRESENT)
                    return probe;
                LOG_TEST(NULL, "IDE candidate ECAM=%llx RP=%x EP=%x lacks Type-2/DOE\n",
                         (unsigned long long)ecam->ecam_base, endpoint->root_bdf,
                         endpoint->endpoint_bdf);
            }
        }
    }

    return VAL_PCIE_PROBE_ABSENT;

failure:
    LOG_ERROR(NULL, "IDE candidate ECAM=%llx RP=%x EP=%x config error offset=%x\n",
              (unsigned long long)ecam->ecam_base, endpoint->root_bdf,
              endpoint->endpoint_bdf, error_offset);
    return VAL_PCIE_PROBE_ERROR;
}

/** @brief Discover usable Type-2 paths instead of stopping at the first PCIe endpoint. */
static int val_host_doe_discover_endpoint(uint64_t ecam_base, uint32_t qualified,
                                          val_host_doe_endpoint_t *endpoint)
{
    pal_pcie_ecam_info_t ecam = {0};
    uint32_t allocation;
    uint32_t bus;
    uint32_t device;
    uint32_t function;
    uint32_t bdf;
    uint32_t port_type;
    uint32_t error_offset = 0U;
    val_pcie_probe_status_t probe;

    if (!endpoint)
        return VAL_ERROR;
    *endpoint = (val_host_doe_endpoint_t){.root_bdf = VAL_PCIE_BDF_INVALID,
                                         .endpoint_bdf = VAL_PCIE_BDF_INVALID};
    for (allocation = 0U; allocation < pal_pcie_ecam_count(); allocation++) {
        if (pal_pcie_ecam_info(allocation, &ecam) != VAL_SUCCESS ||
            ecam.start_bus > ecam.end_bus || ecam.end_bus > UINT8_MAX ||
            ecam.segment > UINT8_MAX)
            return VAL_ERROR;
        if (qualified && ecam.ecam_base != ecam_base)
            continue;
        for (bus = ecam.start_bus; bus <= ecam.end_bus; bus++) {
            for (device = 0U; device < 32U; device++) {
                for (function = 0U; function < 8U; function++) {
                    bdf = val_pcie_bdf_create(ecam.segment, bus, device, function);
                    probe = val_pcie_device_port_type_probe_at(ecam.ecam_base, bdf,
                                                               &port_type, &error_offset);
                    if (probe == VAL_PCIE_PROBE_ERROR) {
                        LOG_ERROR(NULL, "IDE discovery ECAM=%llx BDF=%x config error offset=%x\n",
                                  (unsigned long long)ecam.ecam_base, bdf, error_offset);
                        return VAL_ERROR;
                    }
                    if (probe != VAL_PCIE_PROBE_PRESENT ||
                        port_type != VAL_PCIE_PORT_TYPE_ROOT_PORT)
                        continue;
                    endpoint->ecam_base = ecam.ecam_base;
                    endpoint->root_bdf = bdf;
                    probe = val_host_doe_prepare_root(&ecam, endpoint);
                    if (probe == VAL_PCIE_PROBE_PRESENT) {
                        LOG_TEST(NULL, "CXL Type-2 ECAM=%llx RP=%x EP=%x DOE=%x\n",
                                 (unsigned long long)endpoint->ecam_base, endpoint->root_bdf,
                                 endpoint->endpoint_bdf, endpoint->doe_offset);
                        return VAL_SUCCESS;
                    }
                    if (val_host_doe_release_endpoint(endpoint) != VAL_SUCCESS ||
                        probe == VAL_PCIE_PROBE_ERROR)
                        return VAL_ERROR;
                }
            }
        }
    }

    return VAL_ERROR;
}

/** @brief Discover a Type-2 DOE endpoint across all inventoried ECAM allocations. */
int val_host_doe_prepare_endpoint(val_host_doe_endpoint_t *endpoint)
{
    return val_host_doe_discover_endpoint(0ULL, VAL_FALSE, endpoint);
}

/** @brief Select an endpoint only in the requested namespace without BDF fallback. */
int val_host_doe_prepare_endpoint_at(uint64_t ecam_base, val_host_doe_endpoint_t *endpoint)
{
    return val_host_doe_discover_endpoint(ecam_base, VAL_TRUE, endpoint);
}

/**
 * @brief Restore only the Root Port bus register changed by this probe.
 * @param endpoint Prepared endpoint with saved bus configuration.
 * @return VAL_SUCCESS after restoration; VAL_ERROR leaves failed ownership intact.
 */
int val_host_doe_release_endpoint(val_host_doe_endpoint_t *endpoint)
{
    if (!endpoint)
        return VAL_ERROR;

    if (endpoint->buses_changed != VAL_FALSE) {
        if (val_host_doe_write_buses(endpoint, endpoint->original_buses) != VAL_SUCCESS) {
            LOG_ERROR(NULL, "CXL endpoint bus restore failed for Root Port 0x%x",
                      endpoint->root_bdf);
            return VAL_ERROR;
        }
        endpoint->buses_changed = VAL_FALSE;
    }
    return VAL_SUCCESS;
}

/**
 * @brief Require both SPDM and Secured SPDM DOE protocols at the endpoint.
 * @param endpoint Prepared endpoint with a DOE capability.
 * @return VAL_SUCCESS when both protocols are advertised, otherwise VAL_ERROR.
 */
int val_host_doe_discover_spdm(const val_host_doe_endpoint_t *endpoint)
{
    uint32_t request[3] = {VAL_HOST_DOE_PCISIG_VENDOR, 3U, 0U};
    uint32_t response[3] = {0U, 0U, 0U};
    uint8_t visited[256] = {0U};
    uint32_t index = 0U;
    uint32_t next_index = 0U;
    uint32_t protocol = 0U;
    uint32_t spdm_found = VAL_FALSE;
    uint32_t secure_found = VAL_FALSE;
    size_t response_size = 0U;

    if (!endpoint)
        return VAL_ERROR;

    for (index = 0U; index < VAL_HOST_DOE_DISCOVERY_LIMIT; index++) {
        if (visited[next_index] != 0U)
            return VAL_ERROR;
        visited[next_index] = 1U;
        request[2] = next_index;
        if ((val_host_doe_send(endpoint, request, sizeof(request)) != VAL_SUCCESS) ||
            (val_host_doe_receive(endpoint, response, sizeof(response),
                                   &response_size) != VAL_SUCCESS) ||
            (response_size != sizeof(response)) ||
            (response[0] != VAL_HOST_DOE_PCISIG_VENDOR) ||
            (response[1] != 3U))
            return VAL_ERROR;

        protocol = response[2];
        if ((protocol & 0xffffU) == VAL_HOST_DOE_PCISIG_VENDOR) {
            if (((protocol >> 16U) & 0xffU) == VAL_HOST_DOE_SPDM_TYPE)
                spdm_found = VAL_TRUE;
            if (((protocol >> 16U) & 0xffU) == VAL_HOST_DOE_SECURED_SPDM_TYPE)
                secure_found = VAL_TRUE;
        }

        next_index = protocol >> 24U;
        if (next_index == 0U)
            break;
    }

    LOG_TEST(NULL, "CXL DOE SPDM=%u Secured SPDM=%u", spdm_found, secure_found);
    return (spdm_found && secure_found) ? VAL_SUCCESS : VAL_ERROR;
}

#endif
