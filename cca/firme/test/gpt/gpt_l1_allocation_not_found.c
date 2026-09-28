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

#define GPT_NOT_FOUND_TEST_NAME "gpt_l1_allocation_not_found"

#define GPT_NOT_FOUND_STEP_ADVERTISE UINT64_C(1)
#define GPT_NOT_FOUND_STEP_SCENARIO  UINT64_C(2)
#define GPT_NOT_FOUND_STEP_STATUS    UINT64_C(3)
#define GPT_NOT_FOUND_STEP_COUNT     UINT64_C(4)
#define GPT_NOT_FOUND_STEP_PREFIX    UINT64_C(16)
#define GPT_NOT_FOUND_STEP_CLEANUP   UINT64_C(64)

/**
 * @brief Validate R0126 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
static int gpt_not_found_run(firme_acs_world_t world,
                             uint64_t base_pa,
                             firme_rule_res_t *out)
{
    pal_firme_gpt_scenario_t scenario = {0};
    firme_res_t result = {0};
    pal_firme_gpt_scenario_status_t scenario_status;
    uint64_t expected_target = FIRME_GPI_NONSECURE;
    int64_t actual_status;
    uint32_t assertion_passed = VAL_FALSE;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require the mandatory GPI_SET ABI. */
    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_NOT_FOUND_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /*
     * Step 2: prepare an L0-block partial transition while L1 allocation is
     * deterministically unavailable.
     */
    scenario_status = pal_firme_gpt_scenario_prepare(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_NOT_FOUND, base_pa, &scenario);
    if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_FOUND_STEP_SCENARIO,
            (uint64_t)scenario_status, (uint64_t)FIRME_NOT_FOUND);
        return VAL_ERROR;
    }

    if ((scenario.base_pa == 0ULL) ||
        (scenario.granule_count == 0ULL) ||
        (scenario.completed_count != 0ULL)) {
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_FOUND_STEP_SCENARIO,
            scenario.base_pa, scenario.granule_count);
        goto cleanup;
    }

    /* Step 3: require exact NOT_FOUND and a bounded valid prefix count. */
    val_firme_gm_gpi_set_do(
        scenario.base_pa, scenario.granule_count,
        scenario.target_gpi, &result);
    actual_status = val_firme_status_to_i64(result.x0);
    if (actual_status != (int64_t)FIRME_NOT_FOUND) {
        LOG_ERROR(NULL,
                  "%s: status=%lld expected NOT_FOUND",
                  GPT_NOT_FOUND_TEST_NAME, (long long)actual_status);
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_FOUND_STEP_STATUS,
            result.x0, (uint64_t)FIRME_NOT_FOUND);
        goto cleanup;
    }

    if (val_firme_gpt_operation_count_valid(
            actual_status, result.x1, scenario.granule_count) == VAL_FALSE) {
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_FOUND_STEP_COUNT,
            result.x1, scenario.granule_count);
        goto cleanup;
    }

    /*
     * Step 4: where GPI changes are observable, x1 must identify exactly the
     * successful prefix before the allocator failure.
     */
    if (world == FIRME_ACS_WORLD_SECURE)
        expected_target = FIRME_GPI_SECURE;
    else if (world == FIRME_ACS_WORLD_REALM)
        expected_target = FIRME_GPI_REALM;

    if ((world != FIRME_ACS_WORLD_NSEL2) &&
        ((scenario.initial_gpi != FIRME_GPI_NONSECURE) ||
         (scenario.target_gpi != expected_target))) {
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_FOUND_STEP_SCENARIO,
            scenario.initial_gpi, scenario.target_gpi);
        goto cleanup;
    }

    if ((world != FIRME_ACS_WORLD_NSEL2) &&
        (val_firme_gpt_local_prefix_matches(
             scenario.base_pa, scenario.granule_count,
             result.x1, out, GPT_NOT_FOUND_STEP_PREFIX) == VAL_FALSE))
        goto cleanup;

    assertion_passed = VAL_TRUE;

cleanup:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, PAL_FIRME_GPT_ABI_GPI_SET,
        (int64_t)FIRME_NOT_FOUND, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_FOUND_STEP_CLEANUP,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }

    if (assertion_passed == VAL_TRUE)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0126 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_l1_allocation_not_found_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_found_run(FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0126 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_l1_allocation_not_found_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_found_run(FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0126 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_l1_allocation_not_found_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_found_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
