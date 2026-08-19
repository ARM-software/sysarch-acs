/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_abi.h"

/* GPI encodings (Target GPI field values) */
#define FIRME_GPI_SECURE    0x8U
#define FIRME_GPI_NONSECURE 0x9U
#define FIRME_GPI_ROOT      0xAU
#define FIRME_GPI_REALM     0xBU
#define FIRME_GPI_NSO       0xDU /* FEAT_RME_GPC2 */
#define FIRME_GPI_SA        0x4U /* FEAT_RME_GDI */
#define FIRME_GPI_NSP       0x5U /* FEAT_RME_GDI */
#define FIRME_GPI_UNALLOCATED_TEST 0x6U

/* FIRME_GM_GPI_SET Attributes helpers */
#define FIRME_GPI_ATTR_TARGET_SHIFT 0U
#define FIRME_GPI_ATTR_TARGET_MASK  0xFU
#define FIRME_GPI_ATTR_TARGET(v) \
    (((uint64_t)(v) & FIRME_GPI_ATTR_TARGET_MASK) << FIRME_GPI_ATTR_TARGET_SHIFT)
