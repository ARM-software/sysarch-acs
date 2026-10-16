/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_ide_link.h"
#include "val_firme_rules.h"
#include "val_pcie_ide.h"

#define VAL_FIRME_IDE_OBSERVE_KEYSET_ENCODING UINT64_C(0x101)

typedef struct {
    const val_pcie_ide_stream_t *stream;
    uint64_t keyset_id;
    uint64_t handle;
    uint64_t key[4];
} val_firme_ide_observation_t;

int val_firme_ide_observe_operation(uint64_t operation,
                                   firme_acs_world_t world,
                                   const val_firme_ide_observation_t *selection,
                                   firme_rule_res_t *out);
int val_firme_ide_check_effect(uint64_t operation,
                              firme_acs_world_t world,
                              const val_firme_ide_observation_t *selection,
                              firme_rule_res_t *overall);
