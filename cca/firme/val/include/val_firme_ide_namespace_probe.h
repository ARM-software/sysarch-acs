/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t lease;
    uint64_t address;
    uint64_t pattern;
    uint32_t pe_index;
    uint32_t valid;
} val_firme_ide_namespace_probe_t;

int val_firme_ide_namespace_rx_stopped(val_firme_ide_namespace_probe_t *probe);
int val_firme_ide_namespace_rx_restored(val_firme_ide_namespace_probe_t *probe);
