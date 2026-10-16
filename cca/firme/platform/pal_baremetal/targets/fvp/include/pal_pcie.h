/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#define PAL_PCIE_BDF_INVALID UINT32_MAX

typedef struct {
    uint64_t ecam_base;
    uint64_t ecam_size;
    uint32_t segment;
    uint32_t start_bus;
    uint32_t end_bus;
} pal_pcie_ecam_info_t;

uint64_t pal_pcie_ecam_base(void);
uint64_t pal_pcie_invalid_ecam_base(void);
uint32_t pal_pcie_default_segment(void);
uint32_t pal_pcie_default_root_port_id(void);
uint32_t pal_pcie_default_endpoint_id(void);
uint32_t pal_pcie_default_stream_id(void);
uint32_t pal_pcie_default_substream_id(void);
uint32_t pal_pcie_default_direction(void);
uint32_t pal_pcie_default_key_set(void);
uint32_t pal_pcie_ecam_count(void);
int pal_pcie_ecam_info(uint32_t index, pal_pcie_ecam_info_t *info);
int pal_pcie_cfg_read32(uint32_t bdf, uint32_t offset, uint32_t *value);
int pal_pcie_cfg_write32(uint32_t bdf, uint32_t offset, uint32_t value);
int pal_pcie_cfg_read32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                           uint32_t *value);
int pal_pcie_cfg_write32_at(uint64_t ecam_base, uint32_t bdf, uint32_t offset,
                            uint32_t value);
