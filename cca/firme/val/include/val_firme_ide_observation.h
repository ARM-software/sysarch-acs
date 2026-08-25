/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_ide_link.h"
#include "val_firme_rules.h"

int val_firme_ide_observe_operation(uint64_t operation,
                                   firme_acs_world_t world,
                                   firme_rule_res_t *out);
int val_firme_ide_check_effect(uint64_t operation,
                              firme_acs_world_t world,
                              firme_acs_call_res_t *overall);
