/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_dispatch.h"
#include "val_firme_rules.h"
#include "val_firme_test.h"

/**
 * @brief Execute one strict ALP4 L1 GPT lifecycle rule algorithm.
 * @param rule_id Rule implemented by the dedicated test source.
 * @param world FIRME caller world.
 * @param base_pa Default platform test address.
 * @param out Rule result and diagnostics.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int val_firme_gpt_l1_rule_run(firme_rule_id_t rule_id,
                              firme_acs_world_t world,
                              uint64_t base_pa,
                              firme_rule_res_t *out);
