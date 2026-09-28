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

#define GPT_CONTINUE_PROGRESS_TEST_NAME "gpt_op_continue_progress"

#define GPT_CONTINUE_PROGRESS_STEP_ADVERTISE UINT64_C(1)
#define GPT_CONTINUE_PROGRESS_STEP_SCENARIO  UINT64_C(8)
#define GPT_CONTINUE_PROGRESS_STEP_STATUS    UINT64_C(24)
#define GPT_CONTINUE_PROGRESS_STEP_OUTPUT    UINT64_C(40)
#define GPT_CONTINUE_PROGRESS_STEP_COOKIE    UINT64_C(56)
#define GPT_CONTINUE_PROGRESS_STEP_CLEANUP   UINT64_C(72)

#define GPT_CONTINUE_PROGRESS_STATUS_COUNT 6U

/**
 * @brief Validate R0118 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
static int gpt_op_continue_progress_run(firme_acs_world_t world,
                                        uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    static const int64_t expected_statuses[
        GPT_CONTINUE_PROGRESS_STATUS_COUNT] = {
        (int64_t)FIRME_SUCCESS,
        (int64_t)FIRME_DENIED,
        (int64_t)FIRME_OP_CONFLICT,
        (int64_t)FIRME_NOT_FOUND,
        (int64_t)FIRME_INCOMPLETE,
        (int64_t)FIRME_BUSY,
    };
    pal_firme_gpt_scenario_t scenario = {0};
    firme_res_t result = {0};
    firme_res_t retry_result = {0};
    pal_firme_gpt_scenario_status_t scenario_status;
    int64_t actual_status;
    int64_t retry_status;
    int64_t expected_status;
    uint64_t remaining;
    uint32_t status_index;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require mandatory FIRME_GM_GPI_OP_CONTINUE discovery. */
    if (val_firme_gpt_op_continue_advertised(
            out, GPT_CONTINUE_PROGRESS_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /*
     * Step 2: exercise every architected Table 3.5 row with a PAL-provided
     * deterministic retained-operation state.
     */
    for (status_index = 0U;
         status_index < GPT_CONTINUE_PROGRESS_STATUS_COUNT;
         status_index++) {
        expected_status = expected_statuses[status_index];
        scenario_status = pal_firme_gpt_scenario_prepare(
            (uint32_t)world, PAL_FIRME_GPT_ABI_OP_CONTINUE,
            expected_status, base_pa, &scenario);
        if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
            out->status = (uint64_t)ACS_STATUS_SKIP;
            return val_firme_gpt_rule_result_status(out);
        }
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_CONTINUE_PROGRESS_STEP_SCENARIO + status_index,
                (uint64_t)scenario_status, (uint64_t)expected_status);
            return VAL_ERROR;
        }

        if (scenario.completed_count >= scenario.granule_count) {
            val_firme_gpt_rule_result_fail(
                out, GPT_CONTINUE_PROGRESS_STEP_SCENARIO + status_index,
                scenario.completed_count, scenario.granule_count);
            goto cleanup_fail;
        }
        remaining = scenario.granule_count - scenario.completed_count;

        val_firme_gm_gpi_op_continue(scenario.cookie, &result);
        actual_status = val_firme_status_to_i64(result.x0);
        if (actual_status != expected_status) {
            LOG_ERROR(NULL,
                      "%s: row=%u status=%lld expected=%lld",
                      GPT_CONTINUE_PROGRESS_TEST_NAME,
                      (unsigned int)status_index,
                      (long long)actual_status,
                      (long long)expected_status);
            val_firme_gpt_rule_result_fail(
                out, GPT_CONTINUE_PROGRESS_STEP_STATUS + status_index,
                result.x0, (uint64_t)expected_status);
            goto cleanup_fail;
        }

        /*
         * Step 3: consume x1 only for rows where Table 3.5 makes it valid.
         * Its value must fit the suffix represented by the retained operation.
         */
        if (val_firme_gpt_operation_count_valid(
                actual_status, result.x1, remaining) == VAL_FALSE) {
            val_firme_gpt_rule_result_fail(
                out, GPT_CONTINUE_PROGRESS_STEP_OUTPUT + status_index,
                result.x1, remaining);
            goto cleanup_fail;
        }

        /*
         * Step 4: INCOMPLETE and BUSY return a valid cookie. Reusing that
         * cookie must not be rejected as invalid or unsupported.
         */
        if ((actual_status == (int64_t)FIRME_INCOMPLETE) ||
            (actual_status == (int64_t)FIRME_BUSY)) {
            if (actual_status == (int64_t)FIRME_INCOMPLETE)
                remaining -= result.x1;
            val_firme_gm_gpi_op_continue(result.x2, &retry_result);
            retry_status = val_firme_status_to_i64(retry_result.x0);
            if (val_firme_gpt_operation_count_valid(
                    retry_status, retry_result.x1, remaining) == VAL_FALSE) {
                val_firme_gpt_rule_result_fail(
                    out, GPT_CONTINUE_PROGRESS_STEP_COOKIE + status_index,
                    retry_result.x0, result.x2);
                goto cleanup_fail;
            }
        }

        scenario_status = pal_firme_gpt_scenario_cleanup(
            (uint32_t)world, PAL_FIRME_GPT_ABI_OP_CONTINUE,
            expected_status, &scenario);
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_CONTINUE_PROGRESS_STEP_CLEANUP + status_index,
                (uint64_t)scenario_status, 0ULL);
            return VAL_ERROR;
        }
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);

cleanup_fail:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, PAL_FIRME_GPT_ABI_OP_CONTINUE,
        expected_status, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_CONTINUE_PROGRESS_STEP_CLEANUP + status_index,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }

    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0118 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_op_continue_progress_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_op_continue_progress_run(
        FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0118 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_op_continue_progress_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_op_continue_progress_run(
        FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0118 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_op_continue_progress_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_op_continue_progress_run(
        FIRME_ACS_WORLD_REALM, base_pa, out);
}
