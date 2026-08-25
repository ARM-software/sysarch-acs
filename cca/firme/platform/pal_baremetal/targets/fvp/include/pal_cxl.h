/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

int pal_cxl_traffic_prepare(uint32_t root_bdf, uint32_t endpoint_bdf);
int pal_cxl_traffic_check(uint32_t expect_blocked);
int pal_cxl_traffic_release(void);
