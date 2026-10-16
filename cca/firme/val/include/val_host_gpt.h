/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_gpt.h"

#include "val_firme_abi_test.h"

/** @brief Register the Host-owned GPT allocator and independent observers. */
int val_host_gpt_init(void);

/**
 * @brief Run a retained-operation rule with Host-owned memory and actual-world ABI calls.
 * @param rule R0114, R0115, R0129 or R0130.
 * @param world Explicit FIRME caller on the primary PE.
 * @param out Rule verdict and first-failure diagnostics; ERR quarantines the lease.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int val_host_gpt_retained_run(firme_rule_id_t rule, firme_acs_world_t world,
    firme_rule_res_t *out);

/**
 * @brief Run a generated GPT rule on Host, invoking its ABI in the selected world.
 * @param plan_id Generated GPI_SET, CONTINUE, L1 CREATE or DESTROY plan.
 * @param world Actual caller world, not the coordinator's security state.
 * @param base_pa Dispatcher address retained for generated-plan diagnostics.
 * @param invoke_frame Caller-specific register-frame transport, NULL only for NS.
 * @param invoke_cookie Permanent invocation state for this plan's lifetime.
 * @param out Complete rule result, including failed or uncertain cleanup.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int val_host_gpt_generated_execute(val_firme_abi_test_plan_id_t plan_id,
    firme_acs_world_t world, uint64_t base_pa,
    int (*invoke_frame)(const smccc_call_frame_t *, smccc_call_frame_t *,
                        uint32_t *, void *),
    void *invoke_cookie, firme_rule_res_t *out);
