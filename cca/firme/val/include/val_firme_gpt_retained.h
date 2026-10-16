/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_gpt.h"
#include "val_firme_abi_test.h"

/**
 * @brief Exercise a retained-operation rule using an owned, independently observed range.
 * @param rule R0114, R0115, R0129 or R0130.
 * @param world Actual local FIRME caller; no implicit world substitution.
 * @param out Verdict, with ERR preserving unresolved memory ownership.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int val_firme_gpt_retained_run(firme_rule_id_t rule, firme_acs_world_t world,
    firme_rule_res_t *out);

/**
 * @brief Keep the actual caller transport while independently observing retained work.
 * @param rule R0114, R0115, R0129 or R0130.
 * @param context Caller world and optional full-frame transport retained through cleanup.
 * @param out Verdict, with uncertain leases retained rather than reused.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int val_firme_gpt_retained_execute(firme_rule_id_t rule,
    val_firme_abi_test_context_t *context, firme_rule_res_t *out);
