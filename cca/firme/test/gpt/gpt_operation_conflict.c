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

#define GPT_OPERATION_CONFLICT_TEST_NAME "gpt_operation_conflict"

#define GPT_OPERATION_CONFLICT_STEP_ADVERTISE UINT64_C(1)
#define GPT_OPERATION_CONFLICT_STEP_SCENARIO  UINT64_C(2)
#define GPT_OPERATION_CONFLICT_STEP_STATUS    UINT64_C(3)
#define GPT_OPERATION_CONFLICT_STEP_COUNT     UINT64_C(4)
#define GPT_OPERATION_CONFLICT_STEP_ORIGINAL  UINT64_C(5)
#define GPT_OPERATION_CONFLICT_STEP_CLEANUP   UINT64_C(6)

/**
 * @brief Validate R0129 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
static int gpt_operation_conflict_run(firme_acs_world_t world,
                                      uint64_t base_pa,
                                      firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_t scenario = {0};
    firme_res_t conflict_result = {0};
    firme_res_t original_result = {0};
    pal_firme_gpt_scenario_status_t scenario_status;
    int64_t actual_status;
    int64_t original_status;
    uint32_t assertion_passed = VAL_FALSE;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require both mandatory GPI management ABIs. */
    if ((val_firme_gpt_gpi_set_advertised(
             out, GPT_OPERATION_CONFLICT_STEP_ADVERTISE) == VAL_FALSE) ||
        (val_firme_gpt_op_continue_advertised(
             out, GPT_OPERATION_CONFLICT_STEP_ADVERTISE) == VAL_FALSE))
        return val_firme_gpt_rule_result_status(out);

    /*
     * Step 2: retain an overlapping operation and return its cookie while
     * arranging for the next GPI_SET to encounter that operation.
     */
    scenario_status = pal_firme_gpt_scenario_prepare(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_OP_CONFLICT, base_pa, &scenario);
    if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_OPERATION_CONFLICT_STEP_SCENARIO,
            (uint64_t)scenario_status, (uint64_t)FIRME_OP_CONFLICT);
        return VAL_ERROR;
    }

    /* Step 3: the overlapping operation must return exact OP_CONFLICT. */
    val_firme_gm_gpi_set_do(
        scenario.base_pa, scenario.granule_count,
        scenario.target_gpi, &conflict_result);
    actual_status = val_firme_status_to_i64(conflict_result.x0);
    if (actual_status != (int64_t)FIRME_OP_CONFLICT) {
        LOG_ERROR(NULL,
                  "%s: status=%lld expected OP_CONFLICT",
                  GPT_OPERATION_CONFLICT_TEST_NAME,
                  (long long)actual_status);
        val_firme_gpt_rule_result_fail(
            out, GPT_OPERATION_CONFLICT_STEP_STATUS,
            conflict_result.x0, (uint64_t)FIRME_OP_CONFLICT);
        goto cleanup;
    }

    /* Step 4: the valid prefix count cannot exceed the requested range. */
    if (val_firme_gpt_operation_count_valid(
            actual_status, conflict_result.x1,
            scenario.granule_count) == VAL_FALSE) {
        val_firme_gpt_rule_result_fail(
            out, GPT_OPERATION_CONFLICT_STEP_COUNT,
            conflict_result.x1, scenario.granule_count);
        goto cleanup;
    }

    /*
     * Step 5: the conflicting request must not destroy the retained original
     * operation. Its cookie must still identify a valid continuation.
     */
    val_firme_gm_gpi_op_continue(
        scenario.cookie, &original_result);
    original_status = val_firme_status_to_i64(original_result.x0);
    if (val_firme_gpt_operation_status_valid(original_status) ==
        VAL_FALSE) {
        val_firme_gpt_rule_result_fail(
            out, GPT_OPERATION_CONFLICT_STEP_ORIGINAL,
            original_result.x0, scenario.cookie);
        goto cleanup;
    }

    assertion_passed = VAL_TRUE;

cleanup:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_OP_CONFLICT, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_OPERATION_CONFLICT_STEP_CLEANUP,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }

    if (assertion_passed == VAL_TRUE)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0129 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_operation_conflict_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_operation_conflict_run(
        FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0129 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_operation_conflict_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_operation_conflict_run(
        FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0129 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_operation_conflict_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_operation_conflict_run(
        FIRME_ACS_WORLD_REALM, base_pa, out);
}
