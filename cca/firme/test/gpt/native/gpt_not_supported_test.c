/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../gpt_not_supported_status.c"

static uint64_t native_features;
static int native_discovery_status;
static uint32_t native_discovery_calls;
static uint32_t native_invocations[GPT_NOT_SUPPORTED_ABI_COUNT];
static uint32_t native_prepares;
static uint32_t native_cleanups;
static pal_firme_gpt_scenario_status_t native_prepare_status;
static pal_firme_gpt_scenario_status_t native_cleanup_status;
static firme_res_t native_responses[GPT_NOT_SUPPORTED_ABI_COUNT];
static uint64_t native_cookie;

/** @brief Decode the architected status word without inspecting unused upper bits. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)(int32_t)(uint32_t)value;
}

/** @brief Keep controlled native test output concise. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *format, ...)
{
    return VAL_SUCCESS;
}

/** @brief Model each caller's dispatcher-owned disposable test granule. */
uint64_t val_firme_gpt_base_for_world(firme_acs_world_t world)
{
    assert((uint32_t)world < GPT_NOT_SUPPORTED_WORLD_COUNT);
    return PLAT_FIRME_GPT_BASE_NS;
}

/** @brief Model ABI advertisement independently from actual invocation results. */
int val_firme_service_abi_is_advertised(uint32_t service, uint64_t mask,
                                        uint32_t *advertised, uint64_t *features,
                                        firme_res_t *result)
{
    assert(service == FIRME_SERVICE_ID_GRANULE_MGMT);
    native_discovery_calls++;
    *advertised = (native_features & mask) ? VAL_TRUE : VAL_FALSE;
    *features = native_features;
    *result = (firme_res_t){.x0 = FIRME_SUCCESS, .x1 = native_features};
    return native_discovery_status;
}

/** @brief Verify that even an absent entry point receives a bounded real GPI frame. */
void val_firme_gm_gpi_set_do(uint64_t base, uint64_t count, uint64_t target,
                             firme_res_t *result)
{
    assert(base == PLAT_FIRME_GPT_BASE_NS);
    assert(count == GPT_NOT_SUPPORTED_GRANULE_COUNT && target == FIRME_GPI_NONSECURE);
    native_invocations[PAL_FIRME_GPT_ABI_GPI_SET]++;
    *result = native_responses[PAL_FIRME_GPT_ABI_GPI_SET];
}

/** @brief Record absence probes without pretending their opaque input is live state. */
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *result)
{
    native_cookie = cookie;
    native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE]++;
    *result = native_responses[PAL_FIRME_GPT_ABI_OP_CONTINUE];
}

/** @brief Provide caller-forbidden scenarios only when explicitly enabled in a test. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_prepare(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected, uint64_t base,
    pal_firme_gpt_scenario_t *scenario)
{
    assert(world < GPT_NOT_SUPPORTED_WORLD_COUNT && expected == FIRME_NOT_SUPPORTED);
    assert(abi < GPT_NOT_SUPPORTED_ABI_COUNT);
    native_prepares++;
    *scenario = (pal_firme_gpt_scenario_t){
        .base_pa = base, .granule_count = GPT_NOT_SUPPORTED_GRANULE_COUNT,
        .initial_gpi = FIRME_GPI_NONSECURE, .target_gpi = FIRME_GPI_NONSECURE,
        .cookie = UINT64_C(0x1234),
    };
    return native_prepare_status;
}

/** @brief Record provider cleanup; it must not run after unconfirmed state changes. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_cleanup(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected,
    const pal_firme_gpt_scenario_t *scenario)
{
    assert(world < GPT_NOT_SUPPORTED_WORLD_COUNT && expected == FIRME_NOT_SUPPORTED);
    assert(abi < GPT_NOT_SUPPORTED_ABI_COUNT && scenario->cookie == UINT64_C(0x1234));
    native_cleanups++;
    return native_cleanup_status;
}

/** @brief Reset simulated firmware and quarantine only between independent tests. */
static void native_reset(void)
{
    uint32_t abi_index;

    native_features = 0ULL;
    native_discovery_status = VAL_SUCCESS;
    native_discovery_calls = native_prepares = native_cleanups = 0U;
    native_cookie = UINT64_MAX;
    native_prepare_status = PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    native_cleanup_status = PAL_FIRME_GPT_SCENARIO_READY;
    memset(native_invocations, 0, sizeof(native_invocations));
    memset(g_gpt_not_supported_retained, 0, sizeof(g_gpt_not_supported_retained));
    for (abi_index = 0U; abi_index < GPT_NOT_SUPPORTED_ABI_COUNT; abi_index++)
        native_responses[abi_index] = (firme_res_t){
            .x0 = UINT64_C(0xa5a5a5a500000000) | (uint32_t)FIRME_NOT_SUPPORTED,
            .x1 = UINT64_MAX, .x2 = UINT64_MAX, .x3 = UINT64_MAX,
        };
}

/** @brief Exercise unavailable ABIs in each world without invoking a stub fixture. */
static void native_absent_test(void)
{
    firme_rule_res_t result = {0};
    uint32_t world;

    for (world = 0U; world < GPT_NOT_SUPPORTED_WORLD_COUNT; world++) {
        native_reset();
        assert(gpt_not_supported_run((firme_acs_world_t)world,
            PLAT_FIRME_GPT_BASE_NS, &result) == VAL_SUCCESS);
        assert(result.status == ACS_STATUS_PASS);
        assert(native_invocations[PAL_FIRME_GPT_ABI_GPI_SET] == 1U);
        assert(native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE] == 1U);
        assert(native_cookie == GPT_NOT_SUPPORTED_ABSENCE_ARGUMENT);
        assert(!native_prepares && !native_cleanups);
    }
    native_reset();
    native_features = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    assert(firme_gpt_not_supported_status_run_realm(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_SUCCESS && result.status == ACS_STATUS_PASS);
    assert(!native_invocations[PAL_FIRME_GPT_ABI_GPI_SET]);
    assert(native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE] == 1U);
    assert(native_prepares == 1U && !native_cleanups);
}

/** @brief No applicable forbidden/absent ABI is SKIP, not an invented passing case. */
static void native_unavailable_test(void)
{
    firme_rule_res_t result = {0};

    native_reset();
    native_features = FIRME_GM_FEATURE_REG0_GPI_SET_BIT | FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
    assert(firme_gpt_not_supported_status_run_ns(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_SUCCESS && result.status == ACS_STATUS_SKIP);
    assert(native_prepares == GPT_NOT_SUPPORTED_ABI_COUNT && !native_cleanups);
    assert(!native_invocations[PAL_FIRME_GPT_ABI_GPI_SET]);
    assert(!native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE]);
    native_reset();
    native_discovery_status = VAL_ERROR;
    assert(firme_gpt_not_supported_status_run_ns(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_ERROR && result.status == ACS_STATUS_FAIL);
    assert(!native_invocations[PAL_FIRME_GPT_ABI_GPI_SET]);
    native_reset();
    assert(firme_gpt_not_supported_status_run_ns(PLAT_FIRME_GPT_BASE_NS + 1ULL,
        &result) == VAL_ERROR && result.status == ACS_STATUS_FAIL);
    assert(!native_invocations[PAL_FIRME_GPT_ABI_GPI_SET]);
}

/** @brief A different rejection is still a failed NOT_SUPPORTED expectation. */
static void native_wrong_rejection_test(void)
{
    firme_rule_res_t result = {0};

    native_reset();
    native_responses[PAL_FIRME_GPT_ABI_GPI_SET].x0 = (uint64_t)FIRME_INVALID_PARAMETERS;
    assert(firme_gpt_not_supported_status_run_ns(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_ERROR && result.status == ACS_STATUS_FAIL);
    assert(!g_gpt_not_supported_retained[FIRME_ACS_WORLD_NSEL2].quarantined);
    assert(!native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE]);
    assert(!native_cleanups);
}

/** @brief Never replay or release an unexpectedly accepted absent-ABI invocation. */
static void native_quarantine_test(void)
{
    static const int64_t unexpected[] = {FIRME_SUCCESS, FIRME_INCOMPLETE, FIRME_BUSY};
    firme_rule_res_t result = {0};
    uint32_t response_index;

    for (response_index = 0U; response_index < sizeof(unexpected) / sizeof(unexpected[0]);
         response_index++) {
        native_reset();
        native_features = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
        native_responses[PAL_FIRME_GPT_ABI_OP_CONTINUE] = (firme_res_t){
            .x0 = (uint64_t)unexpected[response_index], .x1 = 0ULL, .x2 = UINT64_C(0x5678),
        };
        assert(firme_gpt_not_supported_status_run_realm(PLAT_FIRME_GPT_BASE_NS,
            &result) == VAL_ERROR && result.status == ACS_STATUS_ERR);
        assert(g_gpt_not_supported_retained[FIRME_ACS_WORLD_REALM].quarantined);
        assert(g_gpt_not_supported_retained[FIRME_ACS_WORLD_REALM].response.x2 ==
               UINT64_C(0x5678));
        assert(native_cookie == GPT_NOT_SUPPORTED_ABSENCE_ARGUMENT && !native_cleanups);
        assert(firme_gpt_not_supported_status_run_realm(PLAT_FIRME_GPT_BASE_NS,
            &result) == VAL_ERROR && result.status == ACS_STATUS_ERR);
        assert(native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE] == 1U);
        assert(native_discovery_calls == GPT_NOT_SUPPORTED_ABI_COUNT);
    }
}

/** @brief Only confirmed rejection permits a provider-owned fixture to be released. */
static void native_provider_test(void)
{
    firme_rule_res_t result = {0};

    native_reset();
    native_features = FIRME_GM_FEATURE_REG0_GPI_SET_BIT | FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT;
    native_prepare_status = PAL_FIRME_GPT_SCENARIO_READY;
    assert(firme_gpt_not_supported_status_run_secure(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_SUCCESS && result.status == ACS_STATUS_PASS);
    assert(native_cleanups == GPT_NOT_SUPPORTED_ABI_COUNT);
    assert(native_cookie == UINT64_C(0x1234));
    native_reset();
    native_features = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    native_prepare_status = PAL_FIRME_GPT_SCENARIO_READY;
    native_responses[PAL_FIRME_GPT_ABI_GPI_SET].x0 = (uint64_t)FIRME_INCOMPLETE;
    assert(firme_gpt_not_supported_status_run_secure(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_ERROR && result.status == ACS_STATUS_ERR);
    assert(!native_cleanups && !native_invocations[PAL_FIRME_GPT_ABI_OP_CONTINUE]);
    native_reset();
    native_features = FIRME_GM_FEATURE_REG0_GPI_SET_BIT;
    native_prepare_status = PAL_FIRME_GPT_SCENARIO_READY;
    native_cleanup_status = PAL_FIRME_GPT_SCENARIO_ERROR;
    assert(firme_gpt_not_supported_status_run_secure(PLAT_FIRME_GPT_BASE_NS,
        &result) == VAL_ERROR && result.status == ACS_STATUS_ERR);
    assert(native_cleanups == 1U);
}

/** @brief Run R0122 control-flow tests, not measurements of firmware support. */
int main(void)
{
    native_absent_test();
    native_unavailable_test();
    native_wrong_rejection_test();
    native_quarantine_test();
    native_provider_test();
    puts("GPT R0122 absent ABI/ownership regressions: PASS");
    return VAL_SUCCESS;
}
