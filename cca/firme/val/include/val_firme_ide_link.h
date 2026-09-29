/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_dispatch.h"

#define VAL_IDE_LINK_DISCOVER 0U
#define VAL_IDE_LINK_BEGIN    1U
#define VAL_IDE_LINK_KEY_LOW  2U
#define VAL_IDE_LINK_KEY_HIGH 3U
#define VAL_IDE_LINK_PROG     4U
#define VAL_IDE_LINK_GO       5U
#define VAL_IDE_LINK_STOP     6U
#define VAL_IDE_LINK_RELEASE  7U
#define VAL_IDE_LINK_DRAIN    8U
#define VAL_IDE_LINK_QUIESCE  9U

void val_firme_ide_link_command(uint64_t operation, uint64_t keyset_id,
                                uint64_t data0, uint64_t data1,
                                firme_acs_call_res_t *out);
