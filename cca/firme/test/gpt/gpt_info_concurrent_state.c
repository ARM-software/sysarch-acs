/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_gpt.h"
#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_INFO_CONCURRENT_TEST_NAME "gpt_info_concurrent_state"

#define GPT_INFO_CONCURRENT_STEP_ADVERTISE UINT64_C(1)
#define GPT_INFO_CONCURRENT_STEP_PREPARE   UINT64_C(2)
#define GPT_INFO_CONCURRENT_STEP_RANGE     UINT64_C(3)
#define GPT_INFO_CONCURRENT_STEP_BEGIN     UINT64_C(4)
#define GPT_INFO_CONCURRENT_STEP_RESULT    UINT64_C(5)
#define GPT_INFO_CONCURRENT_STEP_STATE     UINT64_C(6)
#define GPT_INFO_CONCURRENT_STEP_CLEANUP   UINT64_C(7)

/**
 * @brief Validate R0144 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
static int gpt_info_concurrent_run(firme_acs_world_t world,
                                   uint64_t base_pa,
                                   firme_rule_res_t *out)
{
    pal_firme_gpt_concurrent_info_t evidence = {0};
    val_firme_gpt_info_window_t window = {0};
    firme_res_t result = {0};
    pal_firme_gpt_scenario_status_t scenario_status;
    uint32_t scenario_prepared = VAL_FALSE;
    uint32_t assertion_passed = VAL_FALSE;
    int execution_status = VAL_SUCCESS;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require mandatory GPT_INFO discovery. */
    if (val_firme_gpt_info_advertised(
            out, GPT_INFO_CONCURRENT_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /*
     * Step 2: prepare exactly one range-state transition on another PE.
     */
    scenario_status = pal_firme_gpt_concurrent_info_prepare(
        (uint32_t)world, base_pa, &evidence);
    if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_INFO_CONCURRENT_STEP_PREPARE,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }
    scenario_prepared = VAL_TRUE;

    /* Step 3: validate the independently known query geometry. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_INFO_CONCURRENT_STEP_RANGE) == VAL_FALSE)
        goto cleanup;

    if ((evidence.range_base != window.range_base) ||
        (evidence.range_top != window.range_top)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_INFO_CONCURRENT_STEP_RANGE,
            evidence.range_base, evidence.range_top);
        goto cleanup;
    }

    /*
     * Step 4: release the worker immediately before the production wrapper
     * invokes GPT_INFO. PAL must guarantee that the transition overlaps it.
     */
    scenario_status = pal_firme_gpt_concurrent_info_begin(
        (uint32_t)world, &evidence);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_INFO_CONCURRENT_STEP_BEGIN,
            (uint64_t)scenario_status, 0ULL);
        execution_status = VAL_ERROR;
        goto cleanup;
    }

    val_firme_gm_gpt_info(
        evidence.range_base, evidence.range_top, &result);

    scenario_status = pal_firme_gpt_concurrent_info_finish(
        (uint32_t)world, &evidence);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_INFO_CONCURRENT_STEP_RESULT,
            (uint64_t)scenario_status, result.x0);
        execution_status = VAL_ERROR;
        goto cleanup;
    }

    /*
     * Step 5: validate raw DUT outputs. The reported state may be either
     * state observed during the invocation, but not an unrelated encoding.
     */
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_INFO_CONCURRENT_STEP_RESULT) == VAL_FALSE)
        goto cleanup;

    if (evidence.transition_completed_during_call != VAL_TRUE) {
        val_firme_gpt_rule_result_fail(
            out, GPT_INFO_CONCURRENT_STEP_RESULT,
            evidence.transition_completed_during_call, result.x2);
        goto cleanup;
    }

    if ((result.x2 > FIRME_GPT_PAR_STATE_MAX) ||
        (evidence.state_before > FIRME_GPT_PAR_STATE_MAX) ||
        (evidence.state_after > FIRME_GPT_PAR_STATE_MAX) ||
        (evidence.state_before == evidence.state_after) ||
        ((result.x2 != evidence.state_before) &&
         (result.x2 != evidence.state_after))) {
        LOG_ERROR(NULL,
                  "%s: reported=%llu before=%llu after=%llu",
                  GPT_INFO_CONCURRENT_TEST_NAME,
                  (unsigned long long)result.x2,
                  (unsigned long long)evidence.state_before,
                  (unsigned long long)evidence.state_after);
        val_firme_gpt_rule_result_fail(
            out, GPT_INFO_CONCURRENT_STEP_STATE,
            result.x2,
            (evidence.state_before << 32) | evidence.state_after);
        goto cleanup;
    }

    assertion_passed = VAL_TRUE;

cleanup:
    if (scenario_prepared == VAL_TRUE) {
        scenario_status = pal_firme_gpt_concurrent_info_cleanup(
            (uint32_t)world, &evidence);
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_INFO_CONCURRENT_STEP_CLEANUP,
                (uint64_t)scenario_status, 0ULL);
            return VAL_ERROR;
        }
    }

    if (execution_status != VAL_SUCCESS)
        return VAL_ERROR;

    if (assertion_passed == VAL_TRUE)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 * @brief Validate R0144 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_info_concurrent_state_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_info_concurrent_run(
        FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0144 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_info_concurrent_state_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_info_concurrent_run(
        FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0144 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_info_concurrent_state_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_info_concurrent_run(
        FIRME_ACS_WORLD_REALM, base_pa, out);
}
