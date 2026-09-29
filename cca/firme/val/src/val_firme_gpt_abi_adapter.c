/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt_fixture.h"

#include <stddef.h>

#include "val_common/val_status.h"

#define GPT_ADAPTER_LOCKED 0U
#define GPT_ADAPTER_ARMED 1U
#define GPT_ADAPTER_RECORDED 2U
#define GPT_ADAPTER_WORLDS 3U

static val_firme_gpt_fixture_t g_gpt_adapter_fixture[GPT_ADAPTER_WORLDS];
static uint32_t g_gpt_adapter_lock[GPT_ADAPTER_WORLDS];

/** @brief Return the session exclusively owned by this generated case. */
static val_firme_gpt_fixture_t *gpt_adapter_fixture(
    const val_firme_abi_test_context_t *context)
{
    if (!context || (uint32_t)context->world >= GPT_ADAPTER_WORLDS ||
        !context->scratch[GPT_ADAPTER_LOCKED])
        return NULL;
    return &g_gpt_adapter_fixture[context->world];
}

/** @brief Prepare real storage and, for continuation/destroy, real prior work. */
static int gpt_adapter_prepare(val_firme_abi_test_context_t *context)
{
    val_firme_gpt_fixture_t *fixture;
    pal_firme_gpt_memory_kind_t kind;
    pal_firme_gpt_scenario_status_t status;
    uint32_t unlocked = VAL_FALSE;

    if (!context || !context->abi || !context->test_case ||
        (uint32_t)context->world >= GPT_ADAPTER_WORLDS ||
        context->test_case->mutation != VAL_FIRME_MUTATION_NONE)
        return VAL_ERROR;
    if (!__atomic_compare_exchange_n(&g_gpt_adapter_lock[context->world],
            &unlocked, VAL_TRUE, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    context->scratch[GPT_ADAPTER_LOCKED] = VAL_TRUE;
    fixture = gpt_adapter_fixture(context);
    kind = (context->abi->id == VAL_FIRME_ABI_ID_GM_GPI_SET ||
            context->abi->id == VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE) ?
            PAL_FIRME_GPT_MEMORY_TRANSITION : PAL_FIRME_GPT_MEMORY_L1_NON_SELF;
    status = val_firme_gpt_fixture_begin(fixture, context->world, kind);
    if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE)
        return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    if (status != PAL_FIRME_GPT_SCENARIO_READY)
        return VAL_ERROR;
    if (context->abi->id == VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE) {
        status = val_firme_gpt_fixture_retain(fixture);
        if (status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE)
            return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
        if (status != PAL_FIRME_GPT_SCENARIO_READY)
            return VAL_ERROR;
    }
    if (context->abi->id == VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY &&
        val_firme_gpt_fixture_l1_complete(fixture, VAL_FALSE) != VAL_SUCCESS)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

/** @brief Build a single call from the exclusively owned session state. */
static int gpt_adapter_build(val_firme_abi_test_context_t *context)
{
    val_firme_gpt_fixture_t *fixture = gpt_adapter_fixture(context);

    if (!fixture || !context->abi || !fixture->memory.token || fixture->poisoned)
        return VAL_ERROR;
    context->input.registers[1] = fixture->memory.range_base;
    if (context->abi->id == VAL_FIRME_ABI_ID_GM_GPI_SET) {
        context->input.registers[2] = fixture->count;
        context->input.registers[3] = FIRME_GPI_ATTR_TARGET(fixture->target_gpi);
    } else if (context->abi->id == VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE) {
        if (!fixture->pending)
            return VAL_ERROR;
        context->input.registers[1] = fixture->cookie;
    } else if (context->abi->id == VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE) {
        context->input.registers[2] = fixture->memory.object_base;
    } else if (context->abi->id != VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY) {
        return VAL_ERROR;
    }
    context->scratch[GPT_ADAPTER_ARMED] = VAL_TRUE;
    return VAL_SUCCESS;
}

/** @brief Save the actual response before any failure can bypass cleanup. */
static int gpt_adapter_record(val_firme_abi_test_context_t *context)
{
    val_firme_gpt_fixture_t *fixture = gpt_adapter_fixture(context);
    firme_res_t result = {0};
    int record_status;

    if (!fixture || !context->abi || !context->scratch[GPT_ADAPTER_ARMED] ||
        context->call_issued != VAL_TRUE)
        return VAL_ERROR;
    if (context->scratch[GPT_ADAPTER_RECORDED])
        return fixture->poisoned ? VAL_ERROR : VAL_SUCCESS;
    context->scratch[GPT_ADAPTER_RECORDED] = VAL_TRUE;
    result.x0 = context->output.registers[0];
    result.x1 = context->output.registers[1];
    result.x2 = context->output.registers[2];
    result.x3 = context->output.registers[3];
    if (fixture->kind == PAL_FIRME_GPT_MEMORY_TRANSITION)
        record_status = val_firme_gpt_fixture_gpi_record(fixture, &result);
    else
        record_status = val_firme_gpt_fixture_l1_record(fixture,
            context->abi->id == VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY ? VAL_TRUE : VAL_FALSE,
            &result);
    if (record_status != VAL_SUCCESS)
        fixture->poisoned = VAL_TRUE;
    return record_status;
}

/** @brief Validate positive-path output without treating any architected error as PASS. */
static uint32_t gpt_adapter_validate(val_firme_abi_test_context_t *context)
{
    val_firme_gpt_fixture_t *fixture = gpt_adapter_fixture(context);

    if (!fixture || gpt_adapter_record(context) != VAL_SUCCESS)
        return VAL_FALSE;
    return (fixture->last_status == (int64_t)FIRME_SUCCESS ||
            fixture->last_status == (int64_t)FIRME_BUSY ||
            fixture->last_status == (int64_t)FIRME_INCOMPLETE) ? VAL_TRUE : VAL_FALSE;
}

/** @brief Record every issued response, then restore only known operation state. */
static int gpt_adapter_cleanup(val_firme_abi_test_context_t *context)
{
    val_firme_gpt_fixture_t *fixture = gpt_adapter_fixture(context);
    int record_status = VAL_SUCCESS;
    int cleanup_status;

    if (!context)
        return VAL_ERROR;
    if (!fixture)
        return VAL_SUCCESS;
    if (context->call_issued == VAL_TRUE &&
        !context->scratch[GPT_ADAPTER_RECORDED]) {
        if (!context->scratch[GPT_ADAPTER_ARMED]) {
            fixture->poisoned = VAL_TRUE;
            return VAL_ERROR;
        }
        record_status = gpt_adapter_record(context);
        if (record_status != VAL_SUCCESS)
            fixture->poisoned = VAL_TRUE;
    }
    cleanup_status = val_firme_gpt_fixture_cleanup(fixture);
    if (cleanup_status == VAL_SUCCESS) {
        context->scratch[GPT_ADAPTER_LOCKED] = VAL_FALSE;
        __atomic_store_n(&g_gpt_adapter_lock[context->world], VAL_FALSE, __ATOMIC_RELEASE);
    }
    return (cleanup_status == VAL_SUCCESS && record_status == VAL_SUCCESS) ?
           VAL_SUCCESS : VAL_ERROR;
}

static const val_firme_abi_test_ops_t g_gpt_adapter_ops = {
    .prepare_state = gpt_adapter_prepare,
    .build_valid_input = gpt_adapter_build,
    .validate_output = gpt_adapter_validate,
    .cleanup = gpt_adapter_cleanup,
};

/** @brief Expose supported adapters; registration and gates remain parent-owned. */
const val_firme_abi_test_ops_t *val_firme_gpt_abi_adapter_get(
    val_firme_abi_id_t abi_id)
{
    if (abi_id == VAL_FIRME_ABI_ID_GM_GPI_SET ||
        abi_id == VAL_FIRME_ABI_ID_GM_GPI_OP_CONTINUE ||
        abi_id == VAL_FIRME_ABI_ID_GM_L1_GPT_CREATE ||
        abi_id == VAL_FIRME_ABI_ID_GM_L1_GPT_DESTROY)
        return &g_gpt_adapter_ops;
    return NULL;
}
