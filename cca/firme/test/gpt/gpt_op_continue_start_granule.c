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

#define GPT_CONTINUE_START_TEST_NAME "gpt_op_continue_start_granule"

#define GPT_CONTINUE_START_STEP_ADVERTISE UINT64_C(1)
#define GPT_CONTINUE_START_STEP_SCENARIO  UINT64_C(2)
#define GPT_CONTINUE_START_STEP_TRACE     UINT64_C(3)
#define GPT_CONTINUE_START_STEP_SEQUENCE  UINT64_C(4)
#define GPT_CONTINUE_START_STEP_PREFIX    UINT64_C(16)
#define GPT_CONTINUE_START_STEP_CLEANUP   UINT64_C(64)

/**
 * @brief Validate R0114 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
static int gpt_op_continue_start_run(firme_acs_world_t world,
                                     uint64_t base_pa,
                                     firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_t scenario = {0};
    val_firme_gpt_trace_t trace = {0};
    const val_firme_gpt_phase_t *phase;
    pal_firme_gpt_scenario_status_t scenario_status;
    uint64_t operation_start = 0ULL;
    uint64_t expected_start = 0ULL;
    uint64_t relative_start;
    uint64_t expected_target = FIRME_GPI_NONSECURE;
    uint32_t phase_index;
    uint32_t observe_prefix = VAL_FALSE;
    uint32_t saw_continue = VAL_FALSE;
    uint32_t assertion_passed = VAL_FALSE;
    int execution_status = VAL_SUCCESS;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require both mandatory GPI management ABIs. */
    if ((val_firme_gpt_gpi_set_advertised(
             out, GPT_CONTINUE_START_STEP_ADVERTISE) == VAL_FALSE) ||
        (val_firme_gpt_op_continue_advertised(
             out, GPT_CONTINUE_START_STEP_ADVERTISE) == VAL_FALSE))
        return VAL_SUCCESS;

    /* Step 2: request a deterministic stateful GPI_SET operation. */
    scenario_status = pal_firme_gpt_scenario_prepare(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_INCOMPLETE, base_pa, &scenario);
    if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_SCENARIO,
            (uint64_t)scenario_status, (uint64_t)FIRME_INCOMPLETE);
        return VAL_ERROR;
    }

    if ((scenario.base_pa == 0ULL) ||
        (scenario.granule_count == 0ULL)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_SCENARIO,
            scenario.base_pa, scenario.granule_count);
        goto cleanup;
    }

    if (world == FIRME_ACS_WORLD_SECURE)
        expected_target = FIRME_GPI_SECURE;
    else if (world == FIRME_ACS_WORLD_REALM)
        expected_target = FIRME_GPI_REALM;

    if ((world != FIRME_ACS_WORLD_NSEL2) &&
        ((scenario.initial_gpi != FIRME_GPI_NONSECURE) ||
         (scenario.target_gpi != expected_target))) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_SCENARIO,
            scenario.initial_gpi, scenario.target_gpi);
        goto cleanup;
    }
    observe_prefix = (uint32_t)(world != FIRME_ACS_WORLD_NSEL2);

    /* Step 3: trace every GPI_SET and OP_CONTINUE phase. */
    if (val_firme_gpt_trace_transition(
            scenario.base_pa, scenario.granule_count,
            scenario.target_gpi, observe_prefix, out,
            GPT_CONTINUE_START_STEP_PREFIX, &trace) != VAL_SUCCESS) {
        execution_status = VAL_ERROR;
        goto cleanup;
    }

    if ((trace.phase_count == 0U) ||
        (trace.phases[0].abi != VAL_FIRME_GPT_PHASE_GPI_SET) ||
        (trace.phases[0].status != (int64_t)FIRME_INCOMPLETE) ||
        (trace.malformed == VAL_TRUE) ||
        (trace.exhausted == VAL_TRUE) ||
        (trace.unexpected_status == VAL_TRUE) ||
        (trace.observation_failed == VAL_TRUE)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_TRACE,
            (trace.phase_count == 0U) ? 0ULL :
                (uint64_t)trace.phases[0].status,
            ((uint64_t)trace.malformed << 32) |
                (uint64_t)trace.exhausted);
        goto cleanup;
    }

    /*
     * Step 4: each CONTINUE phase starts after the count reported by the
     * stateful GPI_SET and every preceding incomplete CONTINUE phase.
     */
    for (phase_index = 0U; phase_index < trace.phase_count;
         phase_index++) {
        phase = &trace.phases[phase_index];
        if (phase->abi == VAL_FIRME_GPT_PHASE_GPI_SET) {
            operation_start = phase->start_granule;
            expected_start = 0ULL;
        } else {
            if (phase->start_granule < operation_start) {
                val_firme_gpt_rule_result_fail(
                    out, GPT_CONTINUE_START_STEP_SEQUENCE + phase_index,
                    phase->start_granule, operation_start);
                goto cleanup;
            }

            relative_start = phase->start_granule - operation_start;
            if (relative_start != expected_start) {
                LOG_ERROR(NULL,
                          "%s: phase=%u start=%llu expected=%llu",
                          GPT_CONTINUE_START_TEST_NAME,
                          (unsigned int)phase_index,
                          (unsigned long long)relative_start,
                          (unsigned long long)expected_start);
                val_firme_gpt_rule_result_fail(
                    out, GPT_CONTINUE_START_STEP_SEQUENCE + phase_index,
                    relative_start, expected_start);
                goto cleanup;
            }
            saw_continue = VAL_TRUE;
        }

        if (phase->status == (int64_t)FIRME_INCOMPLETE)
            expected_start += phase->granule_count;
    }

    if (saw_continue == VAL_FALSE) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_SEQUENCE, trace.phase_count, 1ULL);
        goto cleanup;
    }

    if ((val_firme_gpt_trace_has_status(
             &trace, VAL_FIRME_GPT_PHASE_OP_CONTINUE,
             (int64_t)FIRME_INVALID_PARAMETERS) == VAL_TRUE) ||
        (val_firme_gpt_trace_has_status(
             &trace, VAL_FIRME_GPT_PHASE_OP_CONTINUE,
             (int64_t)FIRME_NOT_SUPPORTED) == VAL_TRUE)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_SEQUENCE,
            trace.phase_count, trace.completed_count);
        goto cleanup;
    }

    assertion_passed = VAL_TRUE;

cleanup:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_INCOMPLETE, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_START_STEP_CLEANUP,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }

    if (execution_status != VAL_SUCCESS)
        return VAL_ERROR;

    if (assertion_passed == VAL_TRUE)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 * @brief Validate R0114 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_op_continue_start_granule_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_op_continue_start_run(FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0114 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_op_continue_start_granule_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_op_continue_start_run(FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0114 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_op_continue_start_granule_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_op_continue_start_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
