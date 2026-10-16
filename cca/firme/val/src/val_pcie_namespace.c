/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_pcie.h"
#include "pal_pcie.h"

#define VAL_PCIE_NAMESPACE_CONFIG_END 0xffcU
#define VAL_PCIE_NAMESPACE_LEGACY_START 0x40U
#define VAL_PCIE_NAMESPACE_LEGACY_END 0xfcU
#define VAL_PCIE_NAMESPACE_EXT_START 0x100U
#define VAL_PCIE_NAMESPACE_CAP_LIST (1U << 20)
#define VAL_PCIE_NAMESPACE_CXL_VENDOR 0x1e98U
#define VAL_PCIE_NAMESPACE_CXL_DEVICE 0U
#define VAL_PCIE_NAMESPACE_CXL_TYPE2_CAPS 7U

/** @brief Read a bounded PCI register in the explicitly selected ECAM namespace. */
int val_pcie_cfg_read32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                           uint32_t *value)
{
    if (!value || offset > VAL_PCIE_NAMESPACE_CONFIG_END || (offset & 3U) ||
        val_pcie_requester_id_from_bdf(bdf) == VAL_PCIE_REQ_ID_INVALID)
        return VAL_ERROR;
    return pal_pcie_cfg_read32_at(ecam_base, bdf, offset, value);
}

/** @brief Write a bounded PCI register without resolving an ambiguous BDF. */
int val_pcie_cfg_write32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                            uint32_t value)
{
    if (offset > VAL_PCIE_NAMESPACE_CONFIG_END || (offset & 3U) ||
        val_pcie_requester_id_from_bdf(bdf) == VAL_PCIE_REQ_ID_INVALID)
        return VAL_ERROR;
    return pal_pcie_cfg_write32_at(ecam_base, bdf, offset, value);
}

/** @brief Preserve the failing offset while reading namespace-qualified configuration. */
static int val_pcie_namespace_read(uint64_t ecam_base, uint32_t bdf,
                                   uint32_t offset, uint32_t *value,
                                   uint32_t *error_offset)
{
    if (error_offset)
        *error_offset = offset;
    return val_pcie_cfg_read32_at(ecam_base, bdf, offset, value);
}

/** @brief Distinguish a missing PCIe function from malformed or unreadable capabilities. */
val_pcie_probe_status_t val_pcie_device_port_type_probe_at(uint64_t ecam_base,
                                                          uint32_t bdf,
                                                          uint32_t *port_type,
                                                          uint32_t *error_offset)
{
    uint64_t visited = 0ULL;
    uint32_t value;
    uint32_t offset;
    uint32_t index;

    if (!port_type)
        return VAL_PCIE_PROBE_ERROR;
    *port_type = 0U;
    if (val_pcie_namespace_read(ecam_base, bdf, VAL_PCIE_CFG_VENDOR_ID,
                                 &value, error_offset) != VAL_SUCCESS)
        return VAL_PCIE_PROBE_ERROR;
    if ((value & UINT16_MAX) == UINT16_MAX || (value & UINT16_MAX) == 0U)
        return VAL_PCIE_PROBE_ABSENT;
    if (val_pcie_namespace_read(ecam_base, bdf, VAL_PCIE_CFG_STATUS,
                                 &value, error_offset) != VAL_SUCCESS)
        return VAL_PCIE_PROBE_ERROR;
    if (!(value & VAL_PCIE_NAMESPACE_CAP_LIST))
        return VAL_PCIE_PROBE_ABSENT;
    if (val_pcie_namespace_read(ecam_base, bdf, VAL_PCIE_CFG_CAP_PTR,
                                 &offset, error_offset) != VAL_SUCCESS)
        return VAL_PCIE_PROBE_ERROR;
    offset &= 0xfcU;
    while (offset) {
        if (offset < VAL_PCIE_NAMESPACE_LEGACY_START ||
            offset > VAL_PCIE_NAMESPACE_LEGACY_END)
            return VAL_PCIE_PROBE_ERROR;
        index = (offset - VAL_PCIE_NAMESPACE_LEGACY_START) / sizeof(uint32_t);
        if (visited & (UINT64_C(1) << index))
            return VAL_PCIE_PROBE_ERROR;
        visited |= UINT64_C(1) << index;
        if (val_pcie_namespace_read(ecam_base, bdf, offset, &value,
                                     error_offset) != VAL_SUCCESS ||
            value == 0U || value == UINT32_MAX)
            return VAL_PCIE_PROBE_ERROR;
        if ((value & UINT8_MAX) == VAL_PCIE_CAP_ID_EXPRESS) {
            *port_type = (value >> 20U) & 0xfU;
            return VAL_PCIE_PROBE_PRESENT;
        }
        offset = (value >> 8U) & 0xfcU;
    }

    return VAL_PCIE_PROBE_ABSENT;
}

/** @brief Find Type-2 and DOE capabilities without treating access errors as absence. */
val_pcie_probe_status_t val_pcie_cxl_doe_probe_at(uint64_t ecam_base, uint32_t bdf,
                                                uint32_t *doe_offset,
                                                uint32_t *error_offset)
{
    uint8_t visited[256] = {0};
    uint32_t offset = VAL_PCIE_NAMESPACE_EXT_START;
    uint32_t header;
    uint32_t header1;
    uint32_t header2;
    uint32_t capabilities;
    uint32_t length;
    uint32_t found_doe = 0U;
    uint32_t found_type2 = VAL_FALSE;

    if (!doe_offset)
        return VAL_PCIE_PROBE_ERROR;
    *doe_offset = 0U;
    while (offset) {
        if (offset < VAL_PCIE_NAMESPACE_EXT_START ||
            offset > VAL_PCIE_NAMESPACE_CONFIG_END || (offset & 3U) ||
            (visited[offset / 16U] & (1U << ((offset / 4U) & 3U))))
            return VAL_PCIE_PROBE_ERROR;
        visited[offset / 16U] |= (uint8_t)(1U << ((offset / 4U) & 3U));
        if (val_pcie_namespace_read(ecam_base, bdf, offset, &header,
                                     error_offset) != VAL_SUCCESS)
            return VAL_PCIE_PROBE_ERROR;
        if (header == 0U || header == UINT32_MAX) {
            if (offset != VAL_PCIE_NAMESPACE_EXT_START)
                return VAL_PCIE_PROBE_ERROR;
            break;
        }

        if ((header & UINT16_MAX) == VAL_PCIE_EXT_CAP_ID_DOE) {
            if (offset > VAL_PCIE_NAMESPACE_CONFIG_END - 0x14U)
                return VAL_PCIE_PROBE_ERROR;
            if (!found_doe)
                found_doe = offset;
        }
        if ((header & UINT16_MAX) == VAL_PCIE_EXT_CAP_ID_DVSEC) {
            if (offset > VAL_PCIE_NAMESPACE_CONFIG_END - 8U ||
                val_pcie_namespace_read(ecam_base, bdf, offset + 4U, &header1,
                                         error_offset) != VAL_SUCCESS ||
                val_pcie_namespace_read(ecam_base, bdf, offset + 8U, &header2,
                                         error_offset) != VAL_SUCCESS)
                return VAL_PCIE_PROBE_ERROR;
            length = header1 >> 20U;
            if (length < 12U || length > VAL_PCIE_NAMESPACE_CONFIG_END + 4U - offset)
                return VAL_PCIE_PROBE_ERROR;
            if ((header1 & UINT16_MAX) == VAL_PCIE_NAMESPACE_CXL_VENDOR &&
                (header2 & UINT16_MAX) == VAL_PCIE_NAMESPACE_CXL_DEVICE) {
                capabilities = (header2 >> 16U) & VAL_PCIE_NAMESPACE_CXL_TYPE2_CAPS;
                found_type2 |= capabilities == VAL_PCIE_NAMESPACE_CXL_TYPE2_CAPS;
            }
        }
        offset = header >> 20U;
    }

    if (!found_type2 || !found_doe)
        return VAL_PCIE_PROBE_ABSENT;
    *doe_offset = found_doe;
    return VAL_PCIE_PROBE_PRESENT;
}
