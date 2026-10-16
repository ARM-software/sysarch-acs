/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val/src/val_firme_abi_test.c"

static val_firme_abi_test_context_t *g_context;
static int g_prepare_status = VAL_SUCCESS;
static int g_cleanup_status = VAL_SUCCESS;
static uint32_t g_invalid_input;
static uint32_t g_cleanup_called;
static uint32_t g_observed_issued;
static uint32_t g_smc_count;
static int64_t g_response = FIRME_SUCCESS;
static int g_remote_status = VAL_SUCCESS;
static uint32_t g_remote_issued = VAL_TRUE;
static uint32_t g_remote_calls;
static uint32_t g_remote_features;

/** @brief Suppress expected diagnostics from malformed native cases. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    return 0U;
}

/** @brief Decode the fixture's explicitly sign-extended status values. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)value;
}

/** @brief Verify invocation ownership is published before entering the wrapper. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input,
                              smccc_call_frame_t *output)
{
    assert(g_context->call_issued == VAL_TRUE);
    assert(input->registers[0] == g_context->abi->fid);
    g_smc_count++;
    output->registers[0] = (uint64_t)g_response;
}

/** @brief Verify a previous case cannot leak invocation ownership into setup. */
static int native_prepare(val_firme_abi_test_context_t *context)
{
    assert(context->call_issued == VAL_FALSE);
    return g_prepare_status;
}

/** @brief Supply either a legal input or a deliberately reserved register. */
static int native_build(val_firme_abi_test_context_t *context)
{
    context->input.registers[17] = g_invalid_input;
    return VAL_SUCCESS;
}

/** @brief Observe ownership even when generic output validation failed first. */
static int native_cleanup(val_firme_abi_test_context_t *context)
{
    g_cleanup_called++;
    g_observed_issued = context->call_issued;
    return g_cleanup_status;
}

/** @brief Inject remote execution and lost responses without executing a local SMC. */
static int native_remote(const smccc_call_frame_t *input, smccc_call_frame_t *output,
                           uint32_t *issued, void *cookie)
{
    assert(cookie == &g_remote_calls);
    g_remote_calls++;
    *issued = g_remote_issued;
    output->registers[17] = UINT64_MAX;
    if (input->registers[0] == FIRME_FID_SERVICE_VERSION)
        output->registers[0] = UINT64_C(0x10000);
    else {
        assert(input->registers[0] == FIRME_FID_SERVICE_FEATURES);
        output->registers[0] = FIRME_SUCCESS;
        output->registers[1] = g_remote_features;
    }

    return g_remote_status;
}

/** @brief Reject accidental local discovery in remote-only fixture tests. */
int val_firme_service_get_version(uint32_t service, uint32_t *implemented,
                                  firme_res_t *result)
{
    assert(0 && "discovery must use the selected remote transport");
    return VAL_ERROR;
}

/** @brief Reject a local feature query for a Realm plan. */
int val_firme_service_abi_is_advertised(uint32_t service, uint64_t mask,
                                        uint32_t *advertised, uint64_t *feature,
                                        firme_res_t *result)
{
    assert(0 && "features must use the selected remote transport");
    return VAL_ERROR;
}

/** @brief Check rejected setup, returned errors, retries and cleanup failures. */
int main(void)
{
    val_firme_abi_descriptor_t descriptor = {
        .id = VAL_FIRME_ABI_ID_SERVICE_FEATURES,
        .name = "native_abi", .fid = FIRME_FID_SERVICE_FEATURES,
        .input_bit_masks = {UINT64_MAX},
        .execution_model = VAL_FIRME_ABI_EXEC_SYNCHRONOUS,
    };
    val_firme_abi_case_t test_case = {
        .name = "native_case", .expectation = VAL_FIRME_EXPECT_STATUS,
        .expected_status = FIRME_SUCCESS, .diagnostic_step = 1U,
    };
    val_firme_abi_test_plan_t plan = {.name = "native_plan"};
    val_firme_abi_test_context_t context = {
        .abi = &descriptor, .plan = &plan, .test_case = &test_case,
        .call_issued = VAL_TRUE,
    };
    val_firme_abi_test_ops_t operations = {
        .prepare_state = native_prepare, .build_valid_input = native_build,
        .cleanup = native_cleanup,
    };
    firme_rule_res_t result = {.status = ACS_STATUS_PASS};

    g_context = &context;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_SUCCESS);
    assert(g_cleanup_called == 1U && g_smc_count == 1U && g_observed_issued);

    g_response = FIRME_NOT_SUPPORTED;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && g_observed_issued && g_smc_count == 2U);

    g_response = FIRME_SUCCESS;
    g_invalid_input = VAL_TRUE;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(!g_observed_issued && g_smc_count == 2U);
    g_invalid_input = VAL_FALSE;

    g_prepare_status = VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP && !g_observed_issued && g_smc_count == 2U);

    g_prepare_status = VAL_ERROR;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && !g_observed_issued);

    g_prepare_status = VAL_SUCCESS;
    g_cleanup_status = VAL_ERROR;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR && g_observed_issued);

    g_prepare_status = VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR && !g_observed_issued);

    g_cleanup_status = VAL_SUCCESS;
    g_prepare_status = VAL_SUCCESS;
    descriptor.execution_model = VAL_FIRME_ABI_EXEC_RETRYABLE;
    g_response = FIRME_BUSY;
    g_smc_count = 0U;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(g_observed_issued && g_smc_count == VAL_FIRME_ABI_RETRY_LIMIT);
    assert(g_cleanup_called == 8U);
    descriptor.execution_model = VAL_FIRME_ABI_EXEC_SYNCHRONOUS;
    context.invoke_frame = native_remote;
    context.invoke_cookie = &g_remote_calls;
    g_smc_count = 0U;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_SUCCESS);
    assert(g_remote_calls == 1U && g_smc_count == 0U && context.output_valid);
    assert(context.output.registers[17] == UINT64_MAX);

    g_remote_status = VAL_ERROR;
    g_remote_issued = VAL_FALSE;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(!context.output_valid && !context.transport_uncertain && !g_observed_issued);
    assert(context.output.registers[17] == 0ULL);

    g_remote_issued = VAL_TRUE;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(context.transport_uncertain && g_observed_issued && !context.output_valid);
    assert(g_remote_calls == 3U);
    g_remote_status = VAL_SUCCESS;
    assert(val_firme_abi_execute_case(&context, &operations, &result) == VAL_ERROR);
    assert(g_remote_calls == 3U && g_smc_count == 0U);

    context.transport_uncertain = VAL_FALSE;
    context.world = FIRME_ACS_WORLD_REALM;
    plan.require_advertised = VAL_TRUE;
    plan.required_worlds_mask = VAL_FIRME_ABI_WORLD_MASK_REALM;
    descriptor.feature_bit = UINT64_C(1);
    g_remote_features = 1U;
    assert(val_firme_abi_check_advertisement(&context, &result) == VAL_TRUE);
    assert(g_remote_calls == 5U && g_smc_count == 0U);
    g_remote_features = 0U;
    assert(val_firme_abi_check_advertisement(&context, &result) == VAL_FALSE);
    g_remote_status = VAL_ERROR;
    assert(val_firme_abi_check_advertisement(&context, &result) == VAL_ERROR);
    puts("generated ABI invocation/cleanup native checks: PASS");
    return VAL_SUCCESS;
}
