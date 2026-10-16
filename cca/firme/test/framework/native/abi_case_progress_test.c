/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val/src/val_firme_abi_test.c"

static const val_firme_abi_descriptor_t g_abi = {
    .id = VAL_FIRME_ABI_ID_SERVICE_FEATURES,
    .name = "native ABI",
    .fid = FIRME_FID_SERVICE_FEATURES,
    .input_bit_masks = {UINT32_MAX},
};
static const val_firme_abi_case_t g_cases[] = {
    {.name = "optional setup", .expectation = VAL_FIRME_EXPECT_STATUS,
     .expected_status = FIRME_SUCCESS, .diagnostic_step = 1U},
    {.name = "independent check", .expectation = VAL_FIRME_EXPECT_STATUS,
     .expected_status = FIRME_SUCCESS, .diagnostic_step = 2U},
};
static const val_firme_abi_test_plan_t g_plan = {
    .name = "native case progress", .abi_id = VAL_FIRME_ABI_ID_SERVICE_FEATURES,
    .enabled = VAL_TRUE, .required_worlds_mask = VAL_FIRME_ABI_WORLD_MASK_NS,
    .cases = g_cases, .case_count = 2U,
};
static uint32_t g_calls;
static uint32_t g_cleanups;
static uint32_t g_skip_first = VAL_TRUE;
static int64_t g_return_status = FIRME_SUCCESS;
static int g_cleanup_status = VAL_SUCCESS;

/** @brief Supply a private test plan, not an architectural rule result. */
const val_firme_abi_test_plan_t *val_firme_abi_test_plan_get(
    val_firme_abi_test_plan_id_t plan_id)
{
    return &g_plan;
}

/** @brief Supply the matching private descriptor. */
const val_firme_abi_descriptor_t *val_firme_abi_catalog_get(val_firme_abi_id_t abi_id)
{
    return &g_abi;
}

/** @brief Reject unexpected discovery because this fixture disables it. */
int val_firme_service_get_version(uint32_t service, uint32_t *implemented,
                                  firme_res_t *result)
{
    return VAL_ERROR;
}

/** @brief Reject unexpected feature discovery. */
int val_firme_service_abi_is_advertised(uint32_t service, uint64_t mask,
                                        uint32_t *advertised, uint64_t *feature,
                                        firme_res_t *result)
{
    return VAL_ERROR;
}

/** @brief Suppress expected diagnostics from the native failure cases. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    return VAL_SUCCESS;
}

/** @brief Decode only architected W0 bits. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Count only actual invocations, never unavailable setup. */
void val_smccc_smc_call_frame(const smccc_call_frame_t *input,
                              smccc_call_frame_t *output)
{
    assert(input->registers[0] == g_abi.fid);
    g_calls++;
    output->registers[0] = (uint64_t)g_return_status;
}

/** @brief Make the first case unavailable independently of the second. */
static int native_prepare(val_firme_abi_test_context_t *context)
{
    if (g_skip_first && context->test_case == &g_cases[0])
        return VAL_FIRME_ABI_SCENARIO_UNAVAILABLE;
    return VAL_SUCCESS;
}

/** @brief The initial zeroed frame already contains the valid function ID. */
static int native_build(val_firme_abi_test_context_t *context)
{
    return VAL_SUCCESS;
}

/** @brief Verify both skipped and executed cases release their fixture. */
static int native_cleanup(val_firme_abi_test_context_t *context)
{
    g_cleanups++;
    return g_cleanup_status;
}

/** @brief Reject false passes and failures hidden behind an earlier skipped case. */
int main(void)
{
    const val_firme_abi_test_ops_t ops = {
        .prepare_state = native_prepare, .build_valid_input = native_build,
        .cleanup = native_cleanup,
    };
    val_firme_abi_test_request_t request = {.world = FIRME_ACS_WORLD_NSEL2, .ops = &ops};
    firme_rule_res_t result;

    assert(val_firme_abi_test_execute(&request, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_SKIP && result.fail_step == 1ULL);
    assert(g_calls == 1U && g_cleanups == 2U);
    g_return_status = FIRME_DENIED;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_FAIL && result.fail_step == 2ULL);
    assert(g_calls == 2U && g_cleanups == 4U);
    g_skip_first = VAL_FALSE;
    g_return_status = FIRME_SUCCESS;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS && result.fail_step == 0ULL);
    assert(g_calls == 4U && g_cleanups == 6U);
    g_skip_first = VAL_TRUE;
    g_cleanup_status = VAL_ERROR;
    assert(val_firme_abi_test_execute(&request, &result) == VAL_ERROR);
    assert(result.status == ACS_STATUS_ERR && result.fail_step == 1ULL);
    assert(g_calls == 4U && g_cleanups == 7U);
    puts("Generated ABI case progress: PASS");
    return VAL_SUCCESS;
}
