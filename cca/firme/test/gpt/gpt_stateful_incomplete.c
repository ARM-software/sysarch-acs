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

#define GPT_STATEFUL_INCOMPLETE_TEST_NAME "gpt_stateful_incomplete"

#define GPT_STATEFUL_INCOMPLETE_STEP_ADVERTISE UINT64_C(1)
#define GPT_STATEFUL_INCOMPLETE_STEP_SCENARIO  UINT64_C(2)
#define GPT_STATEFUL_INCOMPLETE_STEP_INITIAL   UINT64_C(3)
#define GPT_STATEFUL_INCOMPLETE_STEP_RESUME    UINT64_C(4)
#define GPT_STATEFUL_INCOMPLETE_STEP_TERMINAL  UINT64_C(5)
#define GPT_STATEFUL_INCOMPLETE_STEP_CLEANUP   UINT64_C(6)

/**
 * @brief Validate R0130 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
static int gpt_stateful_incomplete_run(firme_acs_world_t world,
                                       uint64_t base_pa,
                                       firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_t scenario = {0};
    val_firme_gpt_trace_t trace = {0};
    const val_firme_gpt_phase_t *last_phase;
    pal_firme_gpt_scenario_status_t scenario_status;
    int64_t terminal_status;
    uint32_t assertion_passed = VAL_FALSE;
    int execution_status = VAL_SUCCESS;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require both mandatory GPI management ABIs. */
    if ((val_firme_gpt_gpi_set_advertised(
             out, GPT_STATEFUL_INCOMPLETE_STEP_ADVERTISE) == VAL_FALSE) ||
        (val_firme_gpt_op_continue_advertised(
             out, GPT_STATEFUL_INCOMPLETE_STEP_ADVERTISE) == VAL_FALSE))
        return VAL_SUCCESS;

    /* Step 2: prepare a GPI_SET that must pause with retained state. */
    scenario_status = pal_firme_gpt_scenario_prepare(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_INCOMPLETE, base_pa, &scenario);
    if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_STATEFUL_INCOMPLETE_STEP_SCENARIO,
            (uint64_t)scenario_status, (uint64_t)FIRME_INCOMPLETE);
        return VAL_ERROR;
    }

    /* Step 3: trace the paused operation through continuation to a terminal row. */
    if (val_firme_gpt_trace_transition(
            scenario.base_pa, scenario.granule_count,
            scenario.target_gpi, VAL_FALSE, out,
            GPT_STATEFUL_INCOMPLETE_STEP_INITIAL,
            &trace) != VAL_SUCCESS) {
        execution_status = VAL_ERROR;
        goto cleanup;
    }

    if ((trace.phase_count == 0U) ||
        (trace.phases[0].abi != VAL_FIRME_GPT_PHASE_GPI_SET) ||
        (trace.phases[0].status != (int64_t)FIRME_INCOMPLETE) ||
        (trace.phases[0].granule_count >= scenario.granule_count) ||
        (trace.malformed == VAL_TRUE) ||
        (trace.unexpected_status == VAL_TRUE)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_STATEFUL_INCOMPLETE_STEP_INITIAL,
            (trace.phase_count == 0U) ? 0ULL :
                (uint64_t)trace.phases[0].status,
            (trace.phase_count == 0U) ? 0ULL :
                trace.phases[0].granule_count);
        goto cleanup;
    }

    /* Step 4: the returned cookie must resume the retained operation. */
    if (val_firme_gpt_trace_has_status(
            &trace, VAL_FIRME_GPT_PHASE_OP_CONTINUE,
            (int64_t)FIRME_INVALID_PARAMETERS) == VAL_TRUE ||
        val_firme_gpt_trace_has_status(
            &trace, VAL_FIRME_GPT_PHASE_OP_CONTINUE,
            (int64_t)FIRME_NOT_SUPPORTED) == VAL_TRUE ||
        (trace.phase_count < 2U) ||
        (trace.phases[1].abi != VAL_FIRME_GPT_PHASE_OP_CONTINUE)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_STATEFUL_INCOMPLETE_STEP_RESUME,
            trace.phase_count,
            (trace.phase_count < 2U) ? 0ULL :
                (uint64_t)trace.phases[1].status);
        goto cleanup;
    }

    /* Step 5: retries must eventually leave INCOMPLETE/BUSY state. */
    if (trace.exhausted == VAL_TRUE) {
        val_firme_gpt_rule_result_fail(
            out, GPT_STATEFUL_INCOMPLETE_STEP_TERMINAL,
            trace.phase_count, trace.completed_count);
        goto cleanup;
    }

    last_phase = &trace.phases[trace.phase_count - 1U];
    terminal_status = last_phase->status;
    if ((terminal_status != (int64_t)FIRME_SUCCESS) &&
        (terminal_status != (int64_t)FIRME_DENIED) &&
        (terminal_status != (int64_t)FIRME_OP_CONFLICT) &&
        (terminal_status != (int64_t)FIRME_NOT_FOUND)) {
        LOG_ERROR(NULL,
                  "%s: no terminal result phases=%u status=%lld",
                  GPT_STATEFUL_INCOMPLETE_TEST_NAME,
                  (unsigned int)trace.phase_count,
                  (long long)terminal_status);
        val_firme_gpt_rule_result_fail(
            out, GPT_STATEFUL_INCOMPLETE_STEP_TERMINAL,
            (uint64_t)terminal_status, trace.completed_count);
        goto cleanup;
    }

    assertion_passed = VAL_TRUE;

cleanup:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_INCOMPLETE, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_STATEFUL_INCOMPLETE_STEP_CLEANUP,
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
 * @brief Validate R0130 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_stateful_incomplete_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_stateful_incomplete_run(
        FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0130 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_stateful_incomplete_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_stateful_incomplete_run(
        FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0130 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_stateful_incomplete_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_stateful_incomplete_run(
        FIRME_ACS_WORLD_REALM, base_pa, out);
}
