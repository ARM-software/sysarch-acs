/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdio.h>

#include "../../../val/src/val_firme_gpt.c"
#include "../gpt_info_concurrent_state.c"
#include "../gpt_l1_allocation_not_found.c"
#include "../gpt_management_owner_stability.c"

#define REVIEW_BASE UINT64_C(0xfb000000)
#define REVIEW_COUNT UINT64_C(2)

static uint64_t g_review_count;
static uint32_t g_review_info_calls;
static uint32_t g_review_cleanup_calls;
static uint32_t g_review_restore_calls;
static uint32_t g_review_restore_fails;
static uint32_t g_review_owner_mode;

/** @brief Suppress expected error diagnostics in negative native cases. */
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *fmt, ...)
{
    return 0U;
}

/** @brief Decode the mocked architectural return status. */
int64_t val_firme_status_to_i64(uint64_t value)
{
    return (int64_t)value;
}

/** @brief Advertise the ABI under test without emulating discovery tests. */
int val_firme_service_abi_is_advertised(uint32_t service_id, uint64_t abi_mask,
    uint32_t *advertised, uint64_t *feature_reg0, firme_res_t *features_res)
{
    *advertised = VAL_TRUE;
    *feature_reg0 = abi_mask;
    *features_res = (firme_res_t){.x0 = FIRME_SUCCESS, .x1 = abi_mask};
    return VAL_SUCCESS;
}

/** @brief Supply the 34-bit protected range with a valid zero range base. */
void val_firme_service_features(uint64_t service_id, uint64_t reg, firme_res_t *result)
{
    *result = (firme_res_t){.x0 = FIRME_SUCCESS,
        .x1 = (FIRME_GM_L0GPTSZ_34BITS << FIRME_GM_FEATURE_REG1_L0GPTSZ_SHIFT) |
              (FIRME_GM_PPS_1TB << FIRME_GM_FEATURE_REG1_PPS_SHIFT)};
}

/** @brief Return a valid query response and count actual ABI invocations. */
void val_firme_gm_gpt_info(uint64_t base, uint64_t top, firme_res_t *result)
{
    assert(base == 0ULL);
    assert(top == (UINT64_C(1) << 34));
    g_review_info_calls++;
    *result = (firme_res_t){.x0 = FIRME_SUCCESS, .x1 = top, .x2 = 0ULL};
}

/** @brief Prepare independently known concurrent-state evidence. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_prepare(
    uint32_t world, uint64_t default_base, pal_firme_gpt_concurrent_info_t *evidence)
{
    assert(default_base == REVIEW_BASE);
    *evidence = (pal_firme_gpt_concurrent_info_t){
        .range_base = 0ULL, .range_top = UINT64_C(1) << 34,
        .state_before = 0ULL, .state_after = 1ULL};
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Model release of the concurrent worker. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_begin(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence)
{
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Confirm the transition overlapped the production ABI call. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_finish(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence)
{
    evidence->transition_completed_during_call = VAL_TRUE;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Count cleanup of the concurrent fixture. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_cleanup(
    uint32_t world, const pal_firme_gpt_concurrent_info_t *evidence)
{
    g_review_cleanup_calls++;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Prepare a two-granule allocation-failure scenario for each caller. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_prepare(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t status,
    uint64_t default_base, pal_firme_gpt_scenario_t *scenario)
{
    *scenario = (pal_firme_gpt_scenario_t){.base_pa = REVIEW_BASE,
        .granule_count = REVIEW_COUNT, .initial_gpi = FIRME_GPI_NONSECURE,
        .target_gpi = world == FIRME_ACS_WORLD_SECURE ? FIRME_GPI_SECURE :
            world == FIRME_ACS_WORLD_REALM ? FIRME_GPI_REALM : FIRME_GPI_NONSECURE};
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Verify every allocation-failure path cleans its fixture. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_cleanup(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t status,
    const pal_firme_gpt_scenario_t *scenario)
{
    g_review_cleanup_calls++;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Return the requested prefix count with NOT_FOUND. */
void val_firme_gm_gpi_set_do(uint64_t base, uint64_t count,
    uint64_t target, firme_res_t *result)
{
    if (g_review_owner_mode != 0U) {
        *result = (firme_res_t){.x0 = FIRME_SUCCESS,
            .x1 = g_review_owner_mode == 1U ? REVIEW_COUNT : count};
        if (target == FIRME_GPI_NONSECURE) {
            g_review_restore_calls++;
            result->x0 = g_review_restore_fails ? (uint64_t)FIRME_DENIED : FIRME_SUCCESS;
            result->x1 = g_review_restore_fails ? 0ULL : count;
        }

        return;
    }

    *result = (firme_res_t){.x0 = (uint64_t)FIRME_NOT_FOUND, .x1 = g_review_count};
}

/** @brief No scripted review response is permitted to create retained work. */
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *result)
{
    assert(VAL_FALSE);
}

/** @brief Model accessible prefix granules and GPFs for the remainder. */
int val_probe_read64(uint64_t address, uint64_t *value, val_fault_info_t *fault)
{
    *fault = (val_fault_info_t){0};
    if ((address - REVIEW_BASE) / PLAT_FIRME_GPT_GRANULE_SIZE >= g_review_count) {
        fault->faulted = 1ULL;
        fault->esr = (UINT64_C(0x25) << 26) | UINT64_C(0x28);
    }
    return VAL_SUCCESS;
}

/** @brief Model a changed granule with malformed count and optional cleanup failure. */
int val_firme_gm_gpi_set_complete(uint64_t base, uint64_t count,
    uint64_t target, firme_res_t *result)
{
    *result = (firme_res_t){.x0 = FIRME_SUCCESS, .x1 = REVIEW_COUNT};
    if (target == FIRME_GPI_NONSECURE) {
        g_review_restore_calls++;
        result->x0 = g_review_restore_fails ? (uint64_t)FIRME_DENIED : FIRME_SUCCESS;
        result->x1 = g_review_restore_fails ? 0ULL : count;
    }
    return VAL_SUCCESS;
}

/** @brief Exercise the real rule bodies, including false-pass and cleanup paths. */
int main(void)
{
    firme_rule_res_t result = {0};
    firme_acs_world_t worlds[] = {FIRME_ACS_WORLD_NSEL2,
        FIRME_ACS_WORLD_SECURE, FIRME_ACS_WORLD_REALM};
    size_t world_index;
    uint32_t cleanup_before;

    assert(gpt_info_concurrent_run(FIRME_ACS_WORLD_NSEL2, REVIEW_BASE, &result) ==
           VAL_SUCCESS);
    assert(result.status == ACS_STATUS_PASS && g_review_info_calls == 1U);
    assert(g_review_cleanup_calls == 1U);

    for (world_index = 0U; world_index < sizeof(worlds) / sizeof(worlds[0]);
         world_index++) {
        for (g_review_count = 0ULL; g_review_count <= REVIEW_COUNT + 1ULL;
             g_review_count++) {
            cleanup_before = g_review_cleanup_calls;
            gpt_not_found_run(worlds[world_index], REVIEW_BASE, &result);
            assert(result.status == (g_review_count < REVIEW_COUNT ?
                   ACS_STATUS_PASS : ACS_STATUS_FAIL));
            if (g_review_count >= REVIEW_COUNT)
                assert(result.fail_step == GPT_NOT_FOUND_STEP_COUNT);
            assert(g_review_cleanup_calls == cleanup_before + 1U);
        }
    }

    for (g_review_restore_fails = VAL_FALSE; g_review_restore_fails <= VAL_TRUE;
         g_review_restore_fails++) {
        g_review_owner_mode = 1U;
        g_review_restore_calls = 0U;
        gpt_management_owner_run(REVIEW_BASE, FIRME_GPI_REALM, &result);
        assert(result.status == ACS_STATUS_ERR);
        assert(result.fail_step == GPT_MANAGEMENT_OWNER_STEP_TRANSITION);
        assert(result.last_x0 == FIRME_SUCCESS && result.last_x1 == REVIEW_COUNT);
        assert(g_review_restore_calls == 0U);

        g_review_owner_mode = 2U;
        gpt_management_owner_run(REVIEW_BASE, FIRME_GPI_REALM, &result);
        assert(result.status == (g_review_restore_fails ? ACS_STATUS_FAIL : ACS_STATUS_PASS));
        assert(g_review_restore_calls == 1U);
    }

    puts("GPT review regression: PASS");
    return VAL_SUCCESS;
}
