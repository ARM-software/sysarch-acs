/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_pcie.h"

#include "pal_pcie.h"

#define VAL_PCIE_BDF_SEG_SHIFT 24U
#define VAL_PCIE_BDF_BUS_SHIFT 16U
#define VAL_PCIE_BDF_DEV_SHIFT 8U
#define VAL_PCIE_BDF_FUNC_SHIFT 0U
#define VAL_PCIE_BDF_FIELD_MASK 0xffU

#define VAL_PCIE_REQ_ID_BUS_SHIFT 8U
#define VAL_PCIE_REQ_ID_DEV_SHIFT 3U
#define VAL_PCIE_REQ_ID_BUS_MASK 0xffU
#define VAL_PCIE_REQ_ID_DEV_MASK 0x1fU
#define VAL_PCIE_REQ_ID_FUNC_MASK 0x7U

#define VAL_PCIE_VENDOR_ID_MASK 0xffffU
#define VAL_PCIE_VENDOR_ID_ABSENT 0xffffU
#define VAL_PCIE_CFG_VALUE_ABSENT 0xffffffffU
#define VAL_PCIE_STATUS_CAP_LIST 0x00100000U
#define VAL_PCIE_CAP_PTR_MASK 0xfcU
#define VAL_PCIE_CAP_ID_MASK 0xffU
#define VAL_PCIE_CAP_NEXT_SHIFT 8U
#define VAL_PCIE_CAP_NEXT_MASK 0xfcU
#define VAL_PCIE_CAP_START 0x40U
#define VAL_PCIE_CAP_END 0xfcU
#define VAL_PCIE_CAP_SEARCH_LIMIT 48U
#define VAL_PCIE_EXT_CAP_ID_MASK 0xffffU
#define VAL_PCIE_EXT_CAP_NEXT_SHIFT 20U
#define VAL_PCIE_EXT_CAP_NEXT_MASK 0xffcU
#define VAL_PCIE_EXT_CAP_START 0x100U
#define VAL_PCIE_EXT_CAP_END 0xffcU
#define VAL_PCIE_PORT_TYPE_SHIFT 20U
#define VAL_PCIE_PORT_TYPE_MASK 0xfU
#define VAL_PCIE_SCAN_MAX_DEVICE 32U
#define VAL_PCIE_SCAN_MAX_FUNCTION 8U
#define VAL_PCIE_SECONDARY_BUS_SHIFT 8U
#define VAL_PCIE_SUBORDINATE_BUS_SHIFT 16U
#define VAL_PCIE_DVSEC_VENDOR_CXL 0x1e98U
#define VAL_PCIE_DVSEC_DEVICE 0U
#define VAL_PCIE_DVSEC_HEADER1 4U
#define VAL_PCIE_DVSEC_HEADER2 8U
#define VAL_PCIE_DVSEC_MIN_LENGTH 12U
#define VAL_PCIE_DVSEC_LENGTH_SHIFT 20U
#define VAL_PCIE_CXL_CAP_SHIFT 16U
#define VAL_PCIE_CXL_CAP_CACHE 1U
#define VAL_PCIE_CXL_CAP_IO 2U
#define VAL_PCIE_CXL_CAP_MEM 4U
#define VAL_PCIE_CONFIG_SIZE 0x1000U

/**
 * @brief Find the ECAM allocation containing a discovered PCI function.
 * @param bdf Packed BDF, including the segment.
 * @param info Output ECAM allocation; never substitute a different segment.
 * @return VAL_SUCCESS when the bus is covered, otherwise VAL_ERROR.
 */
static int val_pcie_ecam_for_bdf(uint32_t bdf, pal_pcie_ecam_info_t *info)
{
    uint32_t index = 0U;
    uint32_t bus = val_pcie_bdf_bus(bdf);

    for (index = 0U; index < pal_pcie_ecam_count(); index++) {
        if ((pal_pcie_ecam_info(index, info) == VAL_SUCCESS) &&
            (info->segment == val_pcie_bdf_segment(bdf)) &&
            (bus >= info->start_bus) && (bus <= info->end_bus))
            return VAL_SUCCESS;
    }

    return VAL_ERROR;
}

/**
 * @brief Resolve the actual ECAM allocation for a discovered Root Port.
 * @param bdf Packed PCI function address, including its segment.
 * @param ecam_base Receives the matching allocation base only on success.
 * @return VAL_ERROR for malformed or unmapped functions; never use a default.
 */
int val_pcie_ecam_base_for_bdf(uint32_t bdf, uint64_t *ecam_base)
{
    pal_pcie_ecam_info_t info = {0};

    if (!ecam_base || val_pcie_requester_id_from_bdf(bdf) == VAL_PCIE_REQ_ID_INVALID ||
        val_pcie_ecam_for_bdf(bdf, &info) != VAL_SUCCESS)
        return VAL_ERROR;
    *ecam_base = info.ecam_base;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Test whether a PCIe device type is an endpoint type.
 *   @param    - port_type : PCIe Device/Port Type field.
 *   @return   - VAL_TRUE if endpoint-like, otherwise VAL_FALSE.
 **/
static uint32_t val_pcie_is_endpoint_type(uint32_t port_type)
{
    if (port_type == VAL_PCIE_PORT_TYPE_ENDPOINT)
        return VAL_TRUE;

    if (port_type == VAL_PCIE_PORT_TYPE_LEGACY_ENDPOINT)
        return VAL_TRUE;

    if (port_type == VAL_PCIE_PORT_TYPE_RC_INTEGRATED_ENDPOINT)
        return VAL_TRUE;

    return VAL_FALSE;
}

/**
 *   @brief    - Scan ECAM space for a PCIe Device/Port Type.
 *   @param    - wanted_type       : Device/Port Type to find.
 *   @param    - endpoint_fallback : Non-zero accepts any endpoint-like type.
 *   @param    - out_bdf           : Returned packed BDF.
 *   @return   - VAL_SUCCESS if a matching function is found, otherwise VAL_ERROR.
 **/
static int val_pcie_find_device_type(uint32_t wanted_type, uint32_t endpoint_fallback,
                                     uint32_t *out_bdf)
{
    pal_pcie_ecam_info_t ecam_info = {0};
    uint32_t ecam_index = 0U;
    uint32_t bus = 0U;
    uint32_t device = 0U;
    uint32_t function = 0U;
    uint32_t bdf = VAL_PCIE_BDF_INVALID;
    uint32_t port_type = 0U;

    if (!out_bdf)
        return VAL_ERROR;

    *out_bdf = VAL_PCIE_BDF_INVALID;
    for (ecam_index = 0U; ecam_index < pal_pcie_ecam_count(); ecam_index++) {
        if (pal_pcie_ecam_info(ecam_index, &ecam_info) != VAL_SUCCESS)
            continue;

        for (bus = ecam_info.start_bus; bus <= ecam_info.end_bus; bus++) {
            for (device = 0U; device < VAL_PCIE_SCAN_MAX_DEVICE; device++) {
                for (function = 0U; function < VAL_PCIE_SCAN_MAX_FUNCTION; function++) {
                    bdf = val_pcie_bdf_create(ecam_info.segment, bus, device, function);
                    if (val_pcie_function_present(bdf) != VAL_SUCCESS)
                        continue;

                    if (val_pcie_device_port_type(bdf, &port_type) != VAL_SUCCESS)
                        continue;

                    if (port_type == wanted_type) {
                        *out_bdf = bdf;
                        return VAL_SUCCESS;
                    }

                    if ((endpoint_fallback != VAL_FALSE) &&
                        (val_pcie_is_endpoint_type(port_type) == VAL_TRUE)) {
                        *out_bdf = bdf;
                        return VAL_SUCCESS;
                    }
                }
            }
        }
    }

    return VAL_ERROR;
}

/**
 *   @brief    - Create a VAL packed BDF value.
 *   @param    - segment  : PCI segment number.
 *   @param    - bus      : PCI bus number.
 *   @param    - device   : PCI device number.
 *   @param    - function : PCI function number.
 *   @return   - Packed BDF value, or VAL_PCIE_BDF_INVALID on invalid input.
 **/
uint32_t val_pcie_bdf_create(uint32_t segment, uint32_t bus, uint32_t device,
                             uint32_t function)
{
    uint32_t bdf = 0U;

    if ((segment > VAL_PCIE_BDF_FIELD_MASK) || (bus > VAL_PCIE_BDF_FIELD_MASK) ||
        (device >= VAL_PCIE_SCAN_MAX_DEVICE) || (function >= VAL_PCIE_SCAN_MAX_FUNCTION))
        return VAL_PCIE_BDF_INVALID;

    bdf |= (segment & VAL_PCIE_BDF_FIELD_MASK) << VAL_PCIE_BDF_SEG_SHIFT;
    bdf |= (bus & VAL_PCIE_BDF_FIELD_MASK) << VAL_PCIE_BDF_BUS_SHIFT;
    bdf |= (device & VAL_PCIE_BDF_FIELD_MASK) << VAL_PCIE_BDF_DEV_SHIFT;
    bdf |= (function & VAL_PCIE_BDF_FIELD_MASK) << VAL_PCIE_BDF_FUNC_SHIFT;

    return bdf;
}

/**
 *   @brief    - Extract the PCI segment from a VAL packed BDF.
 *   @param    - bdf : VAL packed BDF value.
 *   @return   - PCI segment number.
 **/
uint32_t val_pcie_bdf_segment(uint32_t bdf)
{
    return (bdf >> VAL_PCIE_BDF_SEG_SHIFT) & VAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Extract the PCI bus number from a VAL packed BDF.
 *   @param    - bdf : VAL packed BDF value.
 *   @return   - PCI bus number.
 **/
uint32_t val_pcie_bdf_bus(uint32_t bdf)
{
    return (bdf >> VAL_PCIE_BDF_BUS_SHIFT) & VAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Extract the PCI device number from a VAL packed BDF.
 *   @param    - bdf : VAL packed BDF value.
 *   @return   - PCI device number.
 **/
uint32_t val_pcie_bdf_device(uint32_t bdf)
{
    return (bdf >> VAL_PCIE_BDF_DEV_SHIFT) & VAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Extract the PCI function number from a VAL packed BDF.
 *   @param    - bdf : VAL packed BDF value.
 *   @return   - PCI function number.
 **/
uint32_t val_pcie_bdf_function(uint32_t bdf)
{
    return (bdf >> VAL_PCIE_BDF_FUNC_SHIFT) & VAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Convert a PCI requester ID to a VAL packed BDF.
 *   @param    - segment      : PCI segment number.
 *   @param    - requester_id : PCI requester ID, encoded as bus/device/function.
 *   @return   - VAL packed BDF value, or VAL_PCIE_BDF_INVALID on invalid input.
 **/
uint32_t val_pcie_bdf_from_requester_id(uint32_t segment, uint32_t requester_id)
{
    uint32_t bus = (requester_id >> VAL_PCIE_REQ_ID_BUS_SHIFT) &
                   VAL_PCIE_REQ_ID_BUS_MASK;
    uint32_t device = (requester_id >> VAL_PCIE_REQ_ID_DEV_SHIFT) &
                      VAL_PCIE_REQ_ID_DEV_MASK;
    uint32_t function = requester_id & VAL_PCIE_REQ_ID_FUNC_MASK;

    if (requester_id > UINT16_MAX)
        return VAL_PCIE_BDF_INVALID;

    return val_pcie_bdf_create(segment, bus, device, function);
}

/**
 *   @brief    - Convert a VAL packed BDF to a PCI requester ID.
 *   @param    - bdf : VAL packed BDF value.
 *   @return   - PCI requester ID, or VAL_PCIE_REQ_ID_INVALID for an invalid BDF.
 **/
uint32_t val_pcie_requester_id_from_bdf(uint32_t bdf)
{
    uint32_t requester_id = 0U;

    if ((val_pcie_bdf_device(bdf) >= VAL_PCIE_SCAN_MAX_DEVICE) ||
        (val_pcie_bdf_function(bdf) >= VAL_PCIE_SCAN_MAX_FUNCTION))
        return VAL_PCIE_REQ_ID_INVALID;

    requester_id |= val_pcie_bdf_bus(bdf) << VAL_PCIE_REQ_ID_BUS_SHIFT;
    requester_id |= (val_pcie_bdf_device(bdf) & VAL_PCIE_REQ_ID_DEV_MASK)
                    << VAL_PCIE_REQ_ID_DEV_SHIFT;
    requester_id |= val_pcie_bdf_function(bdf) & VAL_PCIE_REQ_ID_FUNC_MASK;

    return requester_id;
}

/**
 *   @brief    - Return the default PCIe ECAM base.
 *   @return   - ECAM base address used for FIRME IDE tests.
 **/
uint64_t val_pcie_ecam_base(void)
{
    return pal_pcie_ecam_base();
}

/**
 *   @brief    - Return an invalid PCIe ECAM base for negative tests.
 *   @return   - ECAM address outside the platform ECAM window.
 **/
uint64_t val_pcie_invalid_ecam_base(void)
{
    return pal_pcie_invalid_ecam_base();
}

/**
 *   @brief    - Return the default PCIe segment number.
 *   @return   - PCI segment number used in FIRME keyset IDs.
 **/
uint32_t val_pcie_default_segment(void)
{
    return pal_pcie_default_segment();
}

/**
 *   @brief    - Return the default PCIe Root port requester ID.
 *   @return   - Root port requester ID used in FIRME keyset IDs.
 **/
uint32_t val_pcie_default_root_port_id(void)
{
    return pal_pcie_default_root_port_id();
}

/**
 *   @brief    - Return the default PCIe Endpoint requester ID.
 *   @return   - Endpoint requester ID, or VAL_PCIE_REQ_ID_INVALID if not fixed.
 **/
uint32_t val_pcie_default_endpoint_id(void)
{
    return pal_pcie_default_endpoint_id();
}

/**
 *   @brief    - Return the default IDE stream ID.
 *   @return   - Stream ID used in FIRME keyset IDs.
 **/
uint32_t val_pcie_default_stream_id(void)
{
    return pal_pcie_default_stream_id();
}

/**
 *   @brief    - Return the default IDE substream ID.
 *   @return   - Substream ID used in FIRME keyset IDs.
 **/
uint32_t val_pcie_default_substream_id(void)
{
    return pal_pcie_default_substream_id();
}

/**
 *   @brief    - Return the default IDE direction.
 *   @return   - Direction bit used in FIRME keyset IDs.
 **/
uint32_t val_pcie_default_direction(void)
{
    return pal_pcie_default_direction();
}

/**
 *   @brief    - Return the default IDE key set selector.
 *   @return   - Key Set bit used in FIRME keyset IDs.
 **/
uint32_t val_pcie_default_key_set(void)
{
    return pal_pcie_default_key_set();
}

/**
 *   @brief    - Read a 32-bit PCIe configuration register.
 *   @param    - bdf    : VAL packed BDF value.
 *   @param    - offset : PCIe configuration register offset.
 *   @param    - value  : Returned register value.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
int val_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value)
{
    if (!value || (offset > VAL_PCIE_CONFIG_SIZE - sizeof(uint32_t)) ||
        ((offset & (sizeof(uint32_t) - 1U)) != 0U) ||
        (val_pcie_requester_id_from_bdf(bdf) == VAL_PCIE_REQ_ID_INVALID))
        return VAL_ERROR;

    return pal_pcie_cfg_read32(bdf, offset, value);
}

/**
 *   @brief    - Write a 32-bit PCIe configuration register.
 *   @param    - bdf    : VAL packed BDF value.
 *   @param    - offset : PCIe configuration register offset.
 *   @param    - value  : Register value to write.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
int val_pcie_cfg_write32(uint32_t bdf, uint32_t offset, uint32_t value)
{
    if ((offset > VAL_PCIE_CONFIG_SIZE - sizeof(uint32_t)) ||
        ((offset & (sizeof(uint32_t) - 1U)) != 0U) ||
        (val_pcie_requester_id_from_bdf(bdf) == VAL_PCIE_REQ_ID_INVALID))
        return VAL_ERROR;

    return pal_pcie_cfg_write32(bdf, offset, value);
}

/**
 *   @brief    - Check whether a PCIe function is present.
 *   @param    - bdf : VAL packed BDF value.
 *   @return   - VAL_SUCCESS if present, VAL_ERROR otherwise.
 **/
int val_pcie_function_present(uint32_t bdf)
{
    uint32_t reg_value = 0U;
    uint32_t vendor_id = 0U;

    if (val_pcie_cfg_read32(bdf, VAL_PCIE_CFG_VENDOR_ID, &reg_value) != VAL_SUCCESS)
        return VAL_ERROR;

    vendor_id = reg_value & VAL_PCIE_VENDOR_ID_MASK;
    if ((reg_value == VAL_PCIE_CFG_VALUE_ABSENT) ||
        (vendor_id == VAL_PCIE_VENDOR_ID_ABSENT) ||
        (vendor_id == 0U))
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Find a legacy PCI or PCIe extended capability.
 *   @param    - bdf        : VAL packed BDF value.
 *   @param    - cap_type   : VAL_PCIE_CAP_TYPE_LEGACY or EXTENDED.
 *   @param    - cap_id     : Capability ID to search for.
 *   @param    - cap_offset : Returned capability offset.
 *   @return   - VAL_SUCCESS if found, VAL_ERROR otherwise.
 **/
int val_pcie_find_capability(uint32_t bdf, uint32_t cap_type, uint32_t cap_id,
                             uint32_t *cap_offset)
{
    uint32_t reg_value = 0U;
    uint32_t offset = 0U;
    uint32_t search_count = 0U;
    uint32_t current_cap_id = 0U;
    uint32_t next_offset = 0U;

    if (!cap_offset)
        return VAL_ERROR;

    *cap_offset = 0U;
    if (val_pcie_function_present(bdf) != VAL_SUCCESS)
        return VAL_ERROR;

    if (cap_type == VAL_PCIE_CAP_TYPE_LEGACY) {
        if (val_pcie_cfg_read32(bdf, VAL_PCIE_CFG_STATUS, &reg_value) != VAL_SUCCESS)
            return VAL_ERROR;

        if ((reg_value & VAL_PCIE_STATUS_CAP_LIST) == 0U)
            return VAL_ERROR;

        if (val_pcie_cfg_read32(bdf, VAL_PCIE_CFG_CAP_PTR, &reg_value) != VAL_SUCCESS)
            return VAL_ERROR;

        offset = reg_value & VAL_PCIE_CAP_PTR_MASK;
        while ((offset >= VAL_PCIE_CAP_START) && (offset <= VAL_PCIE_CAP_END) &&
               (search_count < VAL_PCIE_CAP_SEARCH_LIMIT)) {
            if (val_pcie_cfg_read32(bdf, offset, &reg_value) != VAL_SUCCESS)
                return VAL_ERROR;

            current_cap_id = reg_value & VAL_PCIE_CAP_ID_MASK;
            next_offset = (reg_value >> VAL_PCIE_CAP_NEXT_SHIFT) &
                          VAL_PCIE_CAP_NEXT_MASK;
            if (current_cap_id == cap_id) {
                *cap_offset = offset;
                return VAL_SUCCESS;
            }

            offset = next_offset;
            search_count++;
        }

        return VAL_ERROR;
    }

    if (cap_type == VAL_PCIE_CAP_TYPE_EXTENDED) {
        offset = VAL_PCIE_EXT_CAP_START;
        while ((offset >= VAL_PCIE_EXT_CAP_START) && (offset <= VAL_PCIE_EXT_CAP_END) &&
               (search_count < VAL_PCIE_CAP_SEARCH_LIMIT)) {
            if (val_pcie_cfg_read32(bdf, offset, &reg_value) != VAL_SUCCESS)
                return VAL_ERROR;

            if ((reg_value == 0U) || (reg_value == VAL_PCIE_CFG_VALUE_ABSENT))
                return VAL_ERROR;

            current_cap_id = reg_value & VAL_PCIE_EXT_CAP_ID_MASK;
            next_offset = (reg_value >> VAL_PCIE_EXT_CAP_NEXT_SHIFT) &
                          VAL_PCIE_EXT_CAP_NEXT_MASK;
            if (current_cap_id == cap_id) {
                *cap_offset = offset;
                return VAL_SUCCESS;
            }

            if (next_offset == 0U)
                return VAL_ERROR;

            offset = next_offset;
            search_count++;
        }
    }

    return VAL_ERROR;
}

/**
 *   @brief    - Read the PCIe Device/Port Type for a function.
 *   @param    - bdf       : VAL packed BDF value.
 *   @param    - port_type : Returned Device/Port Type field.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
int val_pcie_device_port_type(uint32_t bdf, uint32_t *port_type)
{
    uint32_t pcie_cap_offset = 0U;
    uint32_t reg_value = 0U;

    if (!port_type)
        return VAL_ERROR;

    *port_type = 0U;
    if (val_pcie_find_capability(bdf, VAL_PCIE_CAP_TYPE_LEGACY,
                                 VAL_PCIE_CAP_ID_EXPRESS,
                                 &pcie_cap_offset) != VAL_SUCCESS)
        return VAL_ERROR;

    if (val_pcie_cfg_read32(bdf, pcie_cap_offset, &reg_value) != VAL_SUCCESS)
        return VAL_ERROR;

    *port_type = (reg_value >> VAL_PCIE_PORT_TYPE_SHIFT) & VAL_PCIE_PORT_TYPE_MASK;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Find the platform Root Port used by FIRME IDE tests.
 *   @param    - bdf : Returned VAL packed BDF value.
 *   @return   - VAL_SUCCESS if a Root Port is found, VAL_ERROR otherwise.
 **/
int val_pcie_find_root_port(uint32_t *bdf)
{
    uint32_t configured_bdf = VAL_PCIE_BDF_INVALID;
    uint32_t port_type = 0U;

    if (!bdf)
        return VAL_ERROR;

    *bdf = VAL_PCIE_BDF_INVALID;
    configured_bdf = val_pcie_bdf_from_requester_id(val_pcie_default_segment(),
                                                    val_pcie_default_root_port_id());
    if ((configured_bdf != VAL_PCIE_BDF_INVALID) &&
        (val_pcie_device_port_type(configured_bdf, &port_type) == VAL_SUCCESS) &&
        (port_type == VAL_PCIE_PORT_TYPE_ROOT_PORT)) {
        *bdf = configured_bdf;
        return VAL_SUCCESS;
    }

    return val_pcie_find_device_type(VAL_PCIE_PORT_TYPE_ROOT_PORT, VAL_FALSE, bdf);
}

/**
 *   @brief    - Find an endpoint-like PCIe function behind the Root Complex.
 *   @param    - bdf : Returned VAL packed BDF value.
 *   @return   - VAL_SUCCESS if an endpoint is found, VAL_ERROR otherwise.
 **/
int val_pcie_find_endpoint(uint32_t *bdf)
{
    uint32_t configured_bdf = VAL_PCIE_BDF_INVALID;
    uint32_t endpoint_id = val_pcie_default_endpoint_id();
    uint32_t port_type = 0U;

    if (!bdf)
        return VAL_ERROR;

    *bdf = VAL_PCIE_BDF_INVALID;
    configured_bdf = val_pcie_bdf_from_requester_id(val_pcie_default_segment(),
                                                    endpoint_id);
    if ((configured_bdf != VAL_PCIE_BDF_INVALID) &&
        (val_pcie_device_port_type(configured_bdf, &port_type) == VAL_SUCCESS) &&
        (val_pcie_is_endpoint_type(port_type) == VAL_TRUE)) {
        *bdf = configured_bdf;
        return VAL_SUCCESS;
    }

    return val_pcie_find_device_type(VAL_PCIE_PORT_TYPE_ENDPOINT, VAL_TRUE, bdf);
}

/**
 *   @brief    - Discover the PCIe path used by IDE key management tests.
 *   @param    - path : Returned PCIe IDE path information.
 *   @return   - VAL_SUCCESS if the Root Port is present, VAL_ERROR otherwise.
 **/
int val_pcie_discover_ide_path(val_pcie_ide_path_t *path)
{
    pal_pcie_ecam_info_t ecam_info = {0};
    uint32_t root_bdf = VAL_PCIE_BDF_INVALID;
    uint32_t endpoint_bdf = VAL_PCIE_BDF_INVALID;
    uint32_t ide_cap_offset = 0U;
    uint32_t pcie_cap_offset = 0U;

    if (!path)
        return VAL_ERROR;

    path->ecam_base = val_pcie_ecam_base();
    path->segment = val_pcie_default_segment();
    path->root_port_bdf = VAL_PCIE_BDF_INVALID;
    path->root_port_id = val_pcie_default_root_port_id();
    path->endpoint_bdf = VAL_PCIE_BDF_INVALID;
    path->endpoint_id = val_pcie_default_endpoint_id();
    path->pcie_cap_offset = 0U;
    path->ide_cap_offset = 0U;
    path->root_port_present = VAL_FALSE;
    path->endpoint_present = VAL_FALSE;
    path->ide_cap_present = VAL_FALSE;

    if (val_pcie_find_root_port(&root_bdf) != VAL_SUCCESS)
        return VAL_ERROR;

    if (val_pcie_ecam_for_bdf(root_bdf, &ecam_info) != VAL_SUCCESS)
        return VAL_ERROR;

    path->ecam_base = ecam_info.ecam_base;
    path->segment = ecam_info.segment;
    path->root_port_bdf = root_bdf;
    path->root_port_id = val_pcie_requester_id_from_bdf(root_bdf);
    path->root_port_present = VAL_TRUE;

    if (val_pcie_find_capability(root_bdf, VAL_PCIE_CAP_TYPE_LEGACY,
                                 VAL_PCIE_CAP_ID_EXPRESS,
                                 &pcie_cap_offset) == VAL_SUCCESS)
        path->pcie_cap_offset = pcie_cap_offset;

    if (val_pcie_find_capability(root_bdf, VAL_PCIE_CAP_TYPE_EXTENDED,
                                 VAL_PCIE_EXT_CAP_ID_IDE,
                                 &ide_cap_offset) == VAL_SUCCESS) {
        path->ide_cap_offset = ide_cap_offset;
        path->ide_cap_present = VAL_TRUE;
    }

    if (val_pcie_find_endpoint_below(root_bdf, &endpoint_bdf) == VAL_SUCCESS) {
        path->endpoint_bdf = endpoint_bdf;
        path->endpoint_id = val_pcie_requester_id_from_bdf(endpoint_bdf);
        path->endpoint_present = VAL_TRUE;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Find an endpoint in the selected Root Port's enumerated bus window.
 * @param root_bdf Root Port BDF. Bus enumeration must already be complete.
 * @param bdf Output endpoint BDF; integrated endpoints are not downstream ports.
 * @return VAL_SUCCESS if an endpoint is found, otherwise VAL_ERROR.
 */
int val_pcie_find_endpoint_below(uint32_t root_bdf, uint32_t *bdf)
{
    pal_pcie_ecam_info_t ecam_info = {0};
    uint32_t buses = 0U;
    uint32_t primary = 0U;
    uint32_t secondary = 0U;
    uint32_t subordinate = 0U;
    uint32_t bus = 0U;
    uint32_t device = 0U;
    uint32_t function = 0U;
    uint32_t candidate = VAL_PCIE_BDF_INVALID;
    uint32_t port_type = 0U;

    if (!bdf)
        return VAL_ERROR;

    *bdf = VAL_PCIE_BDF_INVALID;
    if ((val_pcie_ecam_for_bdf(root_bdf, &ecam_info) != VAL_SUCCESS) ||
        (val_pcie_device_port_type(root_bdf, &port_type) != VAL_SUCCESS) ||
        (port_type != VAL_PCIE_PORT_TYPE_ROOT_PORT) ||
        (val_pcie_cfg_read32(root_bdf, VAL_PCIE_CFG_BUS_NUMBERS, &buses) != VAL_SUCCESS))
        return VAL_ERROR;

    primary = buses & VAL_PCIE_BDF_FIELD_MASK;
    secondary = (buses >> VAL_PCIE_SECONDARY_BUS_SHIFT) & VAL_PCIE_BDF_FIELD_MASK;
    subordinate = (buses >> VAL_PCIE_SUBORDINATE_BUS_SHIFT) & VAL_PCIE_BDF_FIELD_MASK;
    if ((primary != val_pcie_bdf_bus(root_bdf)) || (secondary <= primary) ||
        (subordinate < secondary) || (subordinate > ecam_info.end_bus))
        return VAL_ERROR;

    candidate = val_pcie_bdf_from_requester_id(val_pcie_default_segment(),
                                              val_pcie_default_endpoint_id());
    if ((candidate != VAL_PCIE_BDF_INVALID) &&
        (val_pcie_bdf_segment(candidate) == ecam_info.segment) &&
        (val_pcie_bdf_bus(candidate) >= secondary) &&
        (val_pcie_bdf_bus(candidate) <= subordinate) &&
        (val_pcie_device_port_type(candidate, &port_type) == VAL_SUCCESS) &&
        ((port_type == VAL_PCIE_PORT_TYPE_ENDPOINT) ||
         (port_type == VAL_PCIE_PORT_TYPE_LEGACY_ENDPOINT))) {
        *bdf = candidate;
        return VAL_SUCCESS;
    }

    for (bus = secondary; bus <= subordinate; bus++) {
        for (device = 0U; device < VAL_PCIE_SCAN_MAX_DEVICE; device++) {
            for (function = 0U; function < VAL_PCIE_SCAN_MAX_FUNCTION; function++) {
                candidate = val_pcie_bdf_create(ecam_info.segment, bus, device, function);
                if (val_pcie_device_port_type(candidate, &port_type) != VAL_SUCCESS)
                    continue;

                if ((port_type == VAL_PCIE_PORT_TYPE_ENDPOINT) ||
                    (port_type == VAL_PCIE_PORT_TYPE_LEGACY_ENDPOINT)) {
                    *bdf = candidate;
                    return VAL_SUCCESS;
                }
            }
        }
    }

    return VAL_ERROR;
}

/**
 * @brief Identify a CXL endpoint by its Device DVSEC capability bits.
 * @param bdf Endpoint BDF, not a model factory name or platform type hint.
 * @param device_type Output Type 1, 2 or 3; NONE on missing/malformed capabilities.
 * @return VAL_SUCCESS for a recognized device type, otherwise VAL_ERROR.
 */
int val_pcie_cxl_device_type(uint32_t bdf, uint32_t *device_type)
{
    uint32_t offset = VAL_PCIE_EXT_CAP_START;
    uint32_t header = 0U;
    uint32_t header1 = 0U;
    uint32_t header2 = 0U;
    uint32_t length = 0U;
    uint32_t capabilities = 0U;
    uint32_t count = 0U;
    uint32_t port_type = 0U;

    if (!device_type)
        return VAL_ERROR;

    *device_type = VAL_PCIE_CXL_TYPE_NONE;
    if ((val_pcie_device_port_type(bdf, &port_type) != VAL_SUCCESS) ||
        ((port_type != VAL_PCIE_PORT_TYPE_ENDPOINT) &&
         (port_type != VAL_PCIE_PORT_TYPE_RC_INTEGRATED_ENDPOINT)))
        return VAL_ERROR;

    while ((offset >= VAL_PCIE_EXT_CAP_START) && (offset <= VAL_PCIE_EXT_CAP_END) &&
           (count++ < (VAL_PCIE_CONFIG_SIZE - VAL_PCIE_EXT_CAP_START) / sizeof(uint32_t))) {
        if ((val_pcie_cfg_read32(bdf, offset, &header) != VAL_SUCCESS) ||
            (header == 0U) || (header == VAL_PCIE_CFG_VALUE_ABSENT))
            return VAL_ERROR;

        if ((header & VAL_PCIE_EXT_CAP_ID_MASK) == VAL_PCIE_EXT_CAP_ID_DVSEC) {
            if ((offset > VAL_PCIE_CONFIG_SIZE - VAL_PCIE_DVSEC_MIN_LENGTH) ||
                (val_pcie_cfg_read32(bdf, offset + VAL_PCIE_DVSEC_HEADER1,
                                     &header1) != VAL_SUCCESS) ||
                (val_pcie_cfg_read32(bdf, offset + VAL_PCIE_DVSEC_HEADER2,
                                     &header2) != VAL_SUCCESS))
                return VAL_ERROR;

            length = header1 >> VAL_PCIE_DVSEC_LENGTH_SHIFT;
            if ((length < VAL_PCIE_DVSEC_MIN_LENGTH) ||
                (length > VAL_PCIE_CONFIG_SIZE - offset))
                return VAL_ERROR;

            if (((header1 & VAL_PCIE_EXT_CAP_ID_MASK) == VAL_PCIE_DVSEC_VENDOR_CXL) &&
                ((header2 & VAL_PCIE_EXT_CAP_ID_MASK) == VAL_PCIE_DVSEC_DEVICE)) {
                capabilities = (header2 >> VAL_PCIE_CXL_CAP_SHIFT) &
                               (VAL_PCIE_CXL_CAP_CACHE | VAL_PCIE_CXL_CAP_IO |
                                VAL_PCIE_CXL_CAP_MEM);
                switch (capabilities) {
                case VAL_PCIE_CXL_CAP_CACHE | VAL_PCIE_CXL_CAP_IO:
                    *device_type = VAL_PCIE_CXL_TYPE_1;
                    break;
                case VAL_PCIE_CXL_CAP_CACHE | VAL_PCIE_CXL_CAP_IO | VAL_PCIE_CXL_CAP_MEM:
                    *device_type = VAL_PCIE_CXL_TYPE_2;
                    break;
                case VAL_PCIE_CXL_CAP_IO | VAL_PCIE_CXL_CAP_MEM:
                    *device_type = VAL_PCIE_CXL_TYPE_3;
                    break;
                default:
                    return VAL_ERROR;
                }

                return VAL_SUCCESS;
            }
        }

        offset = (header >> VAL_PCIE_EXT_CAP_NEXT_SHIFT) & VAL_PCIE_EXT_CAP_NEXT_MASK;
    }

    return VAL_ERROR;
}
