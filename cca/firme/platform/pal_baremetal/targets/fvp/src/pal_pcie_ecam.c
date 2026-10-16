/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_pcie.h"
#include "pal_platform.h"
#include "val_common/val_status.h"
#include "val_common/val_fault.h"

#define PAL_PCIE_BDF_SEG_SHIFT 24U
#define PAL_PCIE_BDF_BUS_SHIFT 16U
#define PAL_PCIE_BDF_DEV_SHIFT 8U
#define PAL_PCIE_BDF_FUNC_SHIFT 0U
#define PAL_PCIE_BDF_FIELD_MASK 0xffU

#define PAL_PCIE_CFG_BUS_SHIFT 20U
#define PAL_PCIE_CFG_DEV_SHIFT 15U
#define PAL_PCIE_CFG_FUNC_SHIFT 12U
#define PAL_PCIE_CFG_ALIGN_MASK 0x3U

/**
 *   @brief    - Extract the PCI segment from a PAL packed BDF value.
 *   @param    - bdf : PAL packed BDF value.
 *   @return   - PCI segment number.
 **/
static uint32_t pal_pcie_bdf_segment(uint32_t bdf)
{
    return (bdf >> PAL_PCIE_BDF_SEG_SHIFT) & PAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Extract the PCI bus number from a PAL packed BDF value.
 *   @param    - bdf : PAL packed BDF value.
 *   @return   - PCI bus number.
 **/
static uint32_t pal_pcie_bdf_bus(uint32_t bdf)
{
    return (bdf >> PAL_PCIE_BDF_BUS_SHIFT) & PAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Extract the PCI device number from a PAL packed BDF value.
 *   @param    - bdf : PAL packed BDF value.
 *   @return   - PCI device number.
 **/
static uint32_t pal_pcie_bdf_device(uint32_t bdf)
{
    return (bdf >> PAL_PCIE_BDF_DEV_SHIFT) & PAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Extract the PCI function number from a PAL packed BDF value.
 *   @param    - bdf : PAL packed BDF value.
 *   @return   - PCI function number.
 **/
static uint32_t pal_pcie_bdf_function(uint32_t bdf)
{
    return (bdf >> PAL_PCIE_BDF_FUNC_SHIFT) & PAL_PCIE_BDF_FIELD_MASK;
}

/**
 *   @brief    - Translate a packed BDF and register offset to an ECAM address.
 *   @param    - ecam_base : Explicit namespace when qualified is nonzero.
 *   @param    - qualified : Require the selected ECAM rather than BDF-only lookup.
 *   @param    - bdf     : PAL packed BDF value.
 *   @param    - offset  : PCI configuration register offset.
 *   @param    - address : Returned ECAM MMIO address.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
static int pal_pcie_cfg_address(uint64_t ecam_base, uint32_t qualified,
                                 uint32_t bdf, uint32_t offset, uint64_t *address)
{
    pal_pcie_ecam_info_t info = {0};
    uint64_t cfg_offset = 0ULL;
    uint32_t segment = pal_pcie_bdf_segment(bdf);
    uint32_t bus = pal_pcie_bdf_bus(bdf);
    uint32_t device = pal_pcie_bdf_device(bdf);
    uint32_t function = pal_pcie_bdf_function(bdf);
    uint32_t index = 0U;
    uint32_t found = VAL_FALSE;
    uint64_t selected = 0ULL;

    if (!address)
        return VAL_ERROR;

    *address = 0ULL;
    if ((offset > 0xffcU) || ((offset & PAL_PCIE_CFG_ALIGN_MASK) != 0U) ||
        (device >= PLAT_FIRME_PCIE_MAX_DEV) ||
        (function >= PLAT_FIRME_PCIE_MAX_FUNC))
        return VAL_ERROR;

    for (index = 0U; index < pal_pcie_ecam_count(); index++) {
        if (pal_pcie_ecam_info(index, &info) != VAL_SUCCESS)
            return VAL_ERROR;

        if (qualified && info.ecam_base != ecam_base)
            continue;

        if ((segment != info.segment) || (bus < info.start_bus) ||
            (bus > info.end_bus))
            continue;

        cfg_offset = ((uint64_t)(bus - info.start_bus) << PAL_PCIE_CFG_BUS_SHIFT) |
                     ((uint64_t)device << PAL_PCIE_CFG_DEV_SHIFT) |
                     ((uint64_t)function << PAL_PCIE_CFG_FUNC_SHIFT) |
                     (uint64_t)offset;

        if (found || info.start_bus > info.end_bus || info.end_bus > 255U ||
            (cfg_offset + sizeof(uint32_t)) > info.ecam_size ||
            info.ecam_base > UINT64_MAX - cfg_offset - (sizeof(uint32_t) - 1U))
            return VAL_ERROR;

        selected = info.ecam_base + cfg_offset;
        found = VAL_TRUE;
    }

    if (!found)
        return VAL_ERROR;
    *address = selected;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Return the ECAM base address of the default PCIe Root port.
 *   @return   - ECAM base address used for FIRME IDE key management tests.
 **/
uint64_t pal_pcie_ecam_base(void)
{
    return (uint64_t)PLAT_FIRME_PCIE_ECAM_BASE;
}

/**
 *   @brief    - Return an ECAM address that must not identify a Root port.
 *   @return   - Invalid ECAM base address for negative parameter checks.
 **/
uint64_t pal_pcie_invalid_ecam_base(void)
{
    return (uint64_t)(PLAT_FIRME_PCIE_ECAM_BASE + PLAT_FIRME_PCIE_ECAM_SIZE);
}

/**
 *   @brief    - Return the default PCIe segment number.
 *   @return   - Segment number used in default keyset IDs.
 **/
uint32_t pal_pcie_default_segment(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_SEGMENT;
}

/**
 *   @brief    - Return the default PCIe Root port requester ID.
 *   @return   - Root port requester ID used in default keyset IDs.
 **/
uint32_t pal_pcie_default_root_port_id(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_ROOT_PORT_ID;
}

/**
 *   @brief    - Return the default PCIe Endpoint requester ID.
 *   @return   - Endpoint requester ID, or PAL_PCIE_BDF_INVALID if not fixed.
 **/
uint32_t pal_pcie_default_endpoint_id(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_ENDPOINT_ID;
}

/**
 *   @brief    - Return the default IDE stream ID.
 *   @return   - Stream ID used in default keyset IDs.
 **/
uint32_t pal_pcie_default_stream_id(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_STREAM_ID;
}

/**
 *   @brief    - Return the default IDE substream ID.
 *   @return   - Substream ID used in default keyset IDs.
 **/
uint32_t pal_pcie_default_substream_id(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_SUBSTREAM_ID;
}

/**
 *   @brief    - Return the default IDE direction.
 *   @return   - Direction bit used in default keyset IDs.
 **/
uint32_t pal_pcie_default_direction(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_DIRECTION_RX;
}

/**
 *   @brief    - Return the default IDE key set selector.
 *   @return   - Key set bit used in default keyset IDs.
 **/
uint32_t pal_pcie_default_key_set(void)
{
    return (uint32_t)PLAT_FIRME_PCIE_KEY_SET_0;
}

/**
 *   @brief    - Return the number of ECAM windows exposed by the platform.
 *   @return   - Number of ECAM windows.
 **/
uint32_t pal_pcie_ecam_count(void)
{
    return 1U;
}

/**
 *   @brief    - Return information for a platform ECAM window.
 *   @param    - index : ECAM window index.
 *   @param    - info  : Returned ECAM window information.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
int pal_pcie_ecam_info(uint32_t index, pal_pcie_ecam_info_t *info)
{
    if ((!info) || (index >= pal_pcie_ecam_count()))
        return VAL_ERROR;

    info->ecam_base = (uint64_t)PLAT_FIRME_PCIE_ECAM_BASE;
    info->ecam_size = (uint64_t)PLAT_FIRME_PCIE_ECAM_SIZE;
    info->segment = (uint32_t)PLAT_FIRME_PCIE_SEGMENT;
    info->start_bus = (uint32_t)PLAT_FIRME_PCIE_START_BUS;
    info->end_bus = (uint32_t)PLAT_FIRME_PCIE_END_BUS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Read a 32-bit PCI configuration register through ECAM.
 *   @param    - bdf    : PAL packed BDF value.
 *   @param    - offset : PCI configuration register offset.
 *   @param    - value  : Returned register value.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
int pal_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value)
{
    uint64_t address = 0ULL;

    if (!value)
        return VAL_ERROR;

    *value = 0xffffffffU;
    if (pal_pcie_cfg_address(0ULL, VAL_FALSE, bdf, offset, &address) != VAL_SUCCESS)
        return VAL_ERROR;

    return val_mmio_read32_checked(address, value);
}

/**
 *   @brief    - Write a 32-bit PCI configuration register through ECAM.
 *   @param    - bdf    : PAL packed BDF value.
 *   @param    - offset : PCI configuration register offset.
 *   @param    - value  : Register value to write.
 *   @return   - VAL_SUCCESS on success, VAL_ERROR otherwise.
 **/
int pal_pcie_cfg_write32(uint32_t bdf, uint32_t offset, uint32_t value)
{
    uint64_t address = 0ULL;

    if (pal_pcie_cfg_address(0ULL, VAL_FALSE, bdf, offset, &address) != VAL_SUCCESS)
        return VAL_ERROR;

    return val_mmio_write32_checked(address, value);
}

/** @brief Read only the requested, inventoried ECAM namespace; never fall back. */
int pal_pcie_cfg_read32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                           uint32_t *value)
{
    uint64_t address = 0ULL;

    if (!value)
        return VAL_ERROR;
    *value = UINT32_MAX;
    if (pal_pcie_cfg_address(ecam_base, VAL_TRUE, bdf, offset, &address) != VAL_SUCCESS)
        return VAL_ERROR;
    return val_mmio_read32_checked(address, value);
}

/** @brief Write only the requested, inventoried ECAM namespace; never fall back. */
int pal_pcie_cfg_write32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                            uint32_t value)
{
    uint64_t address = 0ULL;

    if (pal_pcie_cfg_address(ecam_base, VAL_TRUE, bdf, offset, &address) != VAL_SUCCESS)
        return VAL_ERROR;
    return val_mmio_write32_checked(address, value);
}
