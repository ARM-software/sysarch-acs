/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../../val/src/val_firme_abi_adapter.c"

static uint32_t g_width = VAL_FIRME_MECID_WIDTH_MAX;
static uint32_t g_refresh_calls;
static uint64_t g_last_params;
static int64_t g_refresh_status = FIRME_SUCCESS;

/** @brief Reject unrelated GPT preparation retained by sanitizer instrumentation. */
uint32_t val_firme_gpt_info_prepare_window(
    uint64_t test_pa, val_firme_gpt_info_window_t *window,
    firme_rule_res_t *out, uint64_t fail_step)
{
    assert(0 && "MEC adapter must not prepare a GPT window");
    return VAL_FALSE;
}

/** @brief Reject unrelated GPT validation retained by sanitizer instrumentation. */
uint32_t val_firme_gpt_info_success_valid(
    const val_firme_gpt_info_window_t *window, const firme_res_t *result,
    firme_rule_res_t *out, uint64_t fail_step)
{
    assert(0 && "MEC adapter must not validate GPT output");
    return VAL_FALSE;
}

/** @brief Suppress expected negative-case diagnostics. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    return VAL_SUCCESS;
}

/** @brief Decode architected W0 rather than unused upper bits. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Advertise controlled widths without using the local PE as the system width. */
int val_firme_mec_advertised_common_mecid_width(uint32_t *width, firme_res_t *result)
{
    *width = g_width;
    *result = (firme_res_t){.x0 = FIRME_SUCCESS};
    return VAL_SUCCESS;
}

/** @brief Use the real input field layout and reject non-owned MECIDs. */
uint64_t val_firme_mec_params(uint64_t mecid, uint64_t reason)
{
    assert(mecid == PLAT_FIRME_MEC_TEST_MECID);
    return (mecid << FIRME_MEC_REFRESH_MECID_SHIFT) | reason;
}

/** @brief Represent the bounded retry helper's final result. */
void val_firme_mec_refresh_do_retry(uint64_t params, firme_res_t *result)
{
    assert((params >> FIRME_MEC_REFRESH_MECID_SHIFT) == PLAT_FIRME_MEC_TEST_MECID);
    g_refresh_calls++;
    g_last_params = params;
    *result = (firme_res_t){.x0 = (uint64_t)g_refresh_status};
}

/** @brief Reset a native fixture as a platform reset would clear its quarantine. */
static void native_reset(val_firme_abi_test_context_t *context)
{
    memset(context->scratch, 0, sizeof(context->scratch));
    context->call_issued = VAL_FALSE;
    g_mec_abi_owned = VAL_FALSE;
    g_width = VAL_FIRME_MECID_WIDTH_MAX;
    g_refresh_calls = 0U;
    g_refresh_status = FIRME_SUCCESS;
}

/** @brief Check reserved-ID selection, exact lifecycle and failure quarantine. */
int main(void)
{
    val_firme_abi_test_plan_t plan = {.name = "native MEC ownership"};
    val_firme_abi_case_t test_case = {.mutation = VAL_FIRME_MUTATION_NONE};
    val_firme_abi_test_context_t context = {
        .plan = &plan, .test_case = &test_case, .world = FIRME_ACS_WORLD_REALM,
    };
    val_firme_abi_test_context_t second = context;

    native_reset(&context);
    g_width = 1U;
    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_ERROR);
    assert(!g_mec_abi_owned && !g_refresh_calls);
    g_width = VAL_FIRME_MECID_WIDTH_MAX;
    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_SUCCESS);
    assert(val_firme_abi_mec_refresh_build(&context) == VAL_SUCCESS);
    assert(context.input.registers[1] == val_firme_mec_params(
        PLAT_FIRME_MEC_TEST_MECID, FIRME_MEC_REFRESH_REASON_CREATE));
    assert(val_firme_abi_mec_refresh_cleanup(&context) == VAL_SUCCESS);
    assert(!g_mec_abi_owned && !g_refresh_calls);

    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = FIRME_SUCCESS;
    assert(val_firme_abi_mec_refresh_cleanup(&context) == VAL_SUCCESS);
    assert(g_refresh_calls == 1U && g_last_params == val_firme_mec_params(
        PLAT_FIRME_MEC_TEST_MECID, FIRME_MEC_REFRESH_REASON_DESTROY));

    native_reset(&context);
    test_case.mutation = VAL_FIRME_MUTATION_DESTROY_REASON;
    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_SUCCESS);
    assert(g_refresh_calls == 1U && g_last_params == val_firme_mec_params(
        PLAT_FIRME_MEC_TEST_MECID, FIRME_MEC_REFRESH_REASON_CREATE));
    assert(val_firme_abi_mec_refresh_build(&context) == VAL_SUCCESS);
    assert(val_firme_abi_mec_refresh_mutate(&context) == VAL_SUCCESS);
    assert(context.input.registers[1] == val_firme_mec_params(
        PLAT_FIRME_MEC_TEST_MECID, FIRME_MEC_REFRESH_REASON_DESTROY));
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = FIRME_SUCCESS;
    assert(val_firme_abi_mec_refresh_cleanup(&context) == VAL_SUCCESS);
    assert(g_refresh_calls == 1U && !g_mec_abi_owned);

    native_reset(&context);
    g_refresh_status = FIRME_INVALID_PARAMETERS;
    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_ERROR);
    assert(val_firme_abi_mec_refresh_cleanup(&context) == VAL_SUCCESS);
    assert(g_refresh_calls == 1U && !g_mec_abi_owned);

    native_reset(&context);
    g_refresh_status = FIRME_INCOMPLETE;
    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_ERROR);
    assert(val_firme_abi_mec_refresh_cleanup(&context) == VAL_ERROR);
    assert(g_refresh_calls == 1U && g_mec_abi_owned);

    native_reset(&context);
    test_case.mutation = VAL_FIRME_MUTATION_NONE;
    assert(val_firme_abi_mec_refresh_prepare(&context) == VAL_SUCCESS);
    context.call_issued = VAL_TRUE;
    context.output.registers[0] = FIRME_SUCCESS;
    g_refresh_status = FIRME_BUSY;
    assert(val_firme_abi_mec_refresh_cleanup(&context) == VAL_ERROR);
    assert(g_refresh_calls == 1U && g_mec_abi_owned);
    assert(val_firme_abi_mec_refresh_prepare(&second) == VAL_ERROR);
    assert(val_firme_abi_mec_refresh_cleanup(&second) == VAL_SUCCESS);
    assert(g_refresh_calls == 1U && g_mec_abi_owned);
    puts("Generated MEC reserved ownership: PASS");
    return VAL_SUCCESS;
}
