/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_common/val_status.h"

#define VAL_PCIE_BDF_INVALID        UINT32_MAX
#define VAL_PCIE_REQ_ID_INVALID     UINT32_MAX

#define VAL_PCIE_CAP_TYPE_LEGACY    0U
#define VAL_PCIE_CAP_TYPE_EXTENDED  1U

#define VAL_PCIE_CFG_VENDOR_ID      0x000U
#define VAL_PCIE_CFG_STATUS         0x004U
#define VAL_PCIE_CFG_HEADER_TYPE    0x00cU
#define VAL_PCIE_CFG_BUS_NUMBERS    0x018U
#define VAL_PCIE_CFG_CAP_PTR        0x034U
#define VAL_PCIE_CAP_ID_EXPRESS     0x10U
#define VAL_PCIE_EXT_CAP_ID_IDE     0x0030U
#define VAL_PCIE_EXT_CAP_ID_DOE     0x002eU
#define VAL_PCIE_EXT_CAP_ID_DVSEC   0x0023U

#define VAL_PCIE_CXL_TYPE_NONE      0U
#define VAL_PCIE_CXL_TYPE_1         1U
#define VAL_PCIE_CXL_TYPE_2         2U
#define VAL_PCIE_CXL_TYPE_3         3U

#define VAL_PCIE_PORT_TYPE_ENDPOINT               0x0U
#define VAL_PCIE_PORT_TYPE_LEGACY_ENDPOINT        0x1U
#define VAL_PCIE_PORT_TYPE_ROOT_PORT              0x4U
#define VAL_PCIE_PORT_TYPE_RC_INTEGRATED_ENDPOINT 0x9U

typedef enum {
    VAL_PCIE_PROBE_ABSENT,
    VAL_PCIE_PROBE_PRESENT,
    VAL_PCIE_PROBE_ERROR,
} val_pcie_probe_status_t;

typedef struct {
    uint64_t ecam_base;
    uint32_t segment;
    uint32_t root_port_bdf;
    uint32_t root_port_id;
    uint32_t endpoint_bdf;
    uint32_t endpoint_id;
    uint32_t pcie_cap_offset;
    uint32_t ide_cap_offset;
    uint32_t root_port_present;
    uint32_t endpoint_present;
    uint32_t ide_cap_present;
} val_pcie_ide_path_t;

uint32_t val_pcie_bdf_create(uint32_t segment, uint32_t bus, uint32_t device,
                             uint32_t function);
uint32_t val_pcie_bdf_segment(uint32_t bdf);
uint32_t val_pcie_bdf_bus(uint32_t bdf);
uint32_t val_pcie_bdf_device(uint32_t bdf);
uint32_t val_pcie_bdf_function(uint32_t bdf);
uint32_t val_pcie_bdf_from_requester_id(uint32_t segment, uint32_t requester_id);
uint32_t val_pcie_requester_id_from_bdf(uint32_t bdf);
uint64_t val_pcie_ecam_base(void);
int val_pcie_ecam_base_for_bdf(uint32_t bdf, uint64_t *ecam_base);
uint64_t val_pcie_invalid_ecam_base(void);
uint32_t val_pcie_default_segment(void);
uint32_t val_pcie_default_root_port_id(void);
uint32_t val_pcie_default_endpoint_id(void);
uint32_t val_pcie_default_stream_id(void);
uint32_t val_pcie_default_substream_id(void);
uint32_t val_pcie_default_direction(void);
uint32_t val_pcie_default_key_set(void);
int val_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value);
int val_pcie_cfg_write32(uint32_t bdf, uint32_t offset, uint32_t value);
int val_pcie_cfg_read32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                           uint32_t *value);
int val_pcie_cfg_write32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                            uint32_t value);
val_pcie_probe_status_t val_pcie_device_port_type_probe_at(uint64_t ecam_base,
                                                          uint32_t bdf,
                                                          uint32_t *port_type,
                                                          uint32_t *error_offset);
val_pcie_probe_status_t val_pcie_cxl_doe_probe_at(uint64_t ecam_base, uint32_t bdf,
                                                uint32_t *doe_offset,
                                                uint32_t *error_offset);
int val_pcie_function_present(uint32_t bdf);
val_pcie_probe_status_t val_pcie_function_probe(uint32_t bdf,
                                                uint32_t *error_offset);
int val_pcie_find_capability(uint32_t bdf, uint32_t cap_type, uint32_t cap_id,
                             uint32_t *cap_offset);
int val_pcie_device_port_type(uint32_t bdf, uint32_t *port_type);
val_pcie_probe_status_t val_pcie_device_port_type_probe(uint32_t bdf,
                                                        uint32_t *port_type,
                                                        uint32_t *error_offset);
int val_pcie_find_root_port(uint32_t *bdf);
int val_pcie_find_endpoint(uint32_t *bdf);
int val_pcie_find_endpoint_below(uint32_t root_bdf, uint32_t *bdf);
int val_pcie_cxl_device_type(uint32_t bdf, uint32_t *device_type);
int val_pcie_discover_ide_path(val_pcie_ide_path_t *path);
