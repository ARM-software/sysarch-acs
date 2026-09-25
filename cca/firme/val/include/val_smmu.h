/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#define VAL_SMMU_MEC_UNAVAILABLE 1

int val_smmu_mecid_width(uint32_t index, uint32_t *width);
