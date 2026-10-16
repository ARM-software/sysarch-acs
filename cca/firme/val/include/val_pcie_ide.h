/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_pcie.h"

#define VAL_PCIE_IDE_ROOT 0U
#define VAL_PCIE_IDE_ENDPOINT 1U
#define VAL_PCIE_IDE_PORT_COUNT 2U

typedef enum {
    VAL_PCIE_IDE_NO_ROOT = 1,
    VAL_PCIE_IDE_NO_ENDPOINT,
    VAL_PCIE_IDE_ROOT_UNAVAILABLE,
    VAL_PCIE_IDE_ENDPOINT_UNAVAILABLE,
    VAL_PCIE_IDE_CONFIG_ERROR,
    VAL_PCIE_IDE_REALM_REQUIRED,
    VAL_PCIE_IDE_NO_STREAM_ID,
    VAL_PCIE_IDE_OWNER_BUSY,
} val_pcie_ide_reason_t;

typedef struct {
    uint32_t bdf;
    uint32_t block;
    uint32_t index;
    uint32_t lock_register;
    uint32_t lock_owned;
    uint32_t changed;
    uint32_t original_control;
} val_pcie_ide_port_t;

typedef struct {
    val_pcie_ide_path_t path;
    val_pcie_ide_port_t ports[VAL_PCIE_IDE_PORT_COUNT];
    uint32_t stream_id;
    uint32_t acquired;
    uint32_t reason;
    uint32_t error_bdf;
    uint32_t error_offset;
    uint32_t original_buses;
    uint32_t buses_changed;
} val_pcie_ide_stream_t;

uint32_t val_pcie_ide_stream_acquire(const val_pcie_ide_path_t *path,
                                    uint32_t realm_caller,
                                    val_pcie_ide_stream_t *stream);
int val_pcie_ide_stream_release(val_pcie_ide_stream_t *stream);
uint32_t val_pcie_ide_stream_discover(uint32_t realm_caller,
                                     val_pcie_ide_stream_t *stream);
