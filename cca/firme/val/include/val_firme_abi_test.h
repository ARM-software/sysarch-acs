/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_common/val_smccc.h"
#include "val_firme_abi_catalog.h"
#include "val_firme_dispatch.h"
#include "val_firme_rules.h"

#define VAL_FIRME_ABI_SCRATCH_QWORDS 16U

typedef struct val_firme_abi_test_context val_firme_abi_test_context_t;

typedef int (*val_firme_abi_prepare_state_fn_t)(
    val_firme_abi_test_context_t *context);
typedef int (*val_firme_abi_build_input_fn_t)(
    val_firme_abi_test_context_t *context);
typedef int (*val_firme_abi_apply_mutation_fn_t)(
    val_firme_abi_test_context_t *context);
typedef uint32_t (*val_firme_abi_validate_fn_t)(
    val_firme_abi_test_context_t *context);
typedef int (*val_firme_abi_cleanup_fn_t)(
    val_firme_abi_test_context_t *context);

typedef struct {
    val_firme_abi_prepare_state_fn_t prepare_state;
    val_firme_abi_build_input_fn_t build_valid_input;
    val_firme_abi_apply_mutation_fn_t apply_mutation;
    val_firme_abi_validate_fn_t validate_output;
    val_firme_abi_validate_fn_t validate_effect;
    val_firme_abi_cleanup_fn_t cleanup;
} val_firme_abi_test_ops_t;

struct val_firme_abi_test_context {
    const val_firme_abi_descriptor_t *abi;
    const val_firme_abi_test_plan_t *plan;
    const val_firme_abi_case_t *test_case;
    smccc_call_frame_t input;
    smccc_call_frame_t output;
    firme_acs_world_t world;
    uint64_t base_pa;
    uint64_t scratch[VAL_FIRME_ABI_SCRATCH_QWORDS];
    void *adapter_context;
};

typedef struct {
    val_firme_abi_test_plan_id_t plan_id;
    firme_acs_world_t world;
    uint64_t base_pa;
    void *adapter_context;
    const val_firme_abi_test_ops_t *ops;
} val_firme_abi_test_request_t;

/**
 * @brief Execute a generated ABI test plan through service-specific adapters.
 * @param request Test-plan identifier, caller world, state and adapter hooks.
 * @param out Rule result populated with PASS, FAIL or SKIP.
 * @return VAL_SUCCESS when an architectural result is produced, otherwise
 *         VAL_ERROR for an ACS framework error.
 */
int val_firme_abi_test_execute(const val_firme_abi_test_request_t *request,
                               firme_rule_res_t *out);
