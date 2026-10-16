/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t address;
    uint64_t lease;
    uint32_t rx_completed;
    uint32_t tx_completed;
    uint32_t failures;
} pal_cxl_traffic_snapshot_t;

int pal_cxl_traffic_prepare(uint32_t root_bdf, uint32_t endpoint_bdf);
int pal_cxl_traffic_prepare_at(uint64_t ecam_base, uint32_t root_bdf,
                                uint32_t endpoint_bdf);
int pal_cxl_traffic_check(uint32_t expect_blocked);
int pal_cxl_traffic_release(void);
int pal_cxl_traffic_snapshot(pal_cxl_traffic_snapshot_t *snapshot);
