/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef struct {
    uint64_t base;
    uint64_t realm_base;
} pal_smmu_info_t;

uint32_t pal_iovirt_smmu_count(void);
int pal_iovirt_smmu_info(uint32_t index, pal_smmu_info_t *info);
