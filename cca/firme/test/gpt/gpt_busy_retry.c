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

#define GPT_BUSY_RETRY_TEST_NAME "gpt_busy_retry"

#define GPT_BUSY_RETRY_STEP_ADVERTISE UINT64_C(1)
#define GPT_BUSY_RETRY_STEP_SCENARIO  UINT64_C(8)
#define GPT_BUSY_RETRY_STEP_STATUS    UINT64_C(16)
#define GPT_BUSY_RETRY_STEP_EFFECT    UINT64_C(24)
#define GPT_BUSY_RETRY_STEP_RETRY     UINT64_C(32)
#define GPT_BUSY_RETRY_STEP_CLEANUP   UINT64_C(40)

#define GPT_BUSY_RETRY_ABI_COUNT 2U

/**
 * @brief Invoke a prepared GPI management scenario.
 * @param abi ABI selected by the scenario.
 * @param scenario Prepared scenario.
 * @param cookie OP_CONTINUE cookie.
 * @param result Returned ABI result.
 * @return None.
 */
static void gpt_busy_retry_invoke(
    pal_firme_gpt_abi_t abi,
    const pal_firme_gpt_scenario_t *scenario,
    uint64_t cookie,
    firme_res_t *result)
{
    if (abi == PAL_FIRME_GPT_ABI_GPI_SET) {
        val_firme_gm_gpi_set_do(
            scenario->base_pa, scenario->granule_count,
            scenario->target_gpi, result);
    } else {
        val_firme_gm_gpi_op_continue(cookie, result);
    }
}

/**
 * @brief Validate R0124 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
static int gpt_busy_retry_run(firme_acs_world_t world,
                              uint64_t base_pa,
                              firme_rule_res_t *out)
{
    static const pal_firme_gpt_abi_t abis[GPT_BUSY_RETRY_ABI_COUNT] = {
        PAL_FIRME_GPT_ABI_GPI_SET,
        PAL_FIRME_GPT_ABI_OP_CONTINUE,
    };
    pal_firme_gpt_scenario_t scenario = {0};
    firme_res_t result = {0};
    firme_res_t retry_result = {0};
    pal_firme_gpt_scenario_status_t scenario_status;
    uint64_t retry_cookie;
    uint64_t remaining;
    uint64_t expected_target = FIRME_GPI_NONSECURE;
    int64_t actual_status;
    int64_t retry_status;
    uint32_t abi_index;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /* Step 1: require both mandatory GPI management ABIs. */
    if ((val_firme_gpt_gpi_set_advertised(
             out, GPT_BUSY_RETRY_STEP_ADVERTISE) == VAL_FALSE) ||
        (val_firme_gpt_op_continue_advertised(
             out, GPT_BUSY_RETRY_STEP_ADVERTISE) == VAL_FALSE))
        return val_firme_gpt_rule_result_status(out);

    if (world == FIRME_ACS_WORLD_SECURE)
        expected_target = FIRME_GPI_SECURE;
    else if (world == FIRME_ACS_WORLD_REALM)
        expected_target = FIRME_GPI_REALM;

    /* Step 2: induce BUSY independently for GPI_SET and OP_CONTINUE. */
    for (abi_index = 0U; abi_index < GPT_BUSY_RETRY_ABI_COUNT;
         abi_index++) {
        scenario_status = pal_firme_gpt_scenario_prepare(
            (uint32_t)world, abis[abi_index],
            (int64_t)FIRME_BUSY, base_pa, &scenario);
        if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE) {
            out->status = (uint64_t)ACS_STATUS_SKIP;
            return val_firme_gpt_rule_result_status(out);
        }
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_BUSY_RETRY_STEP_SCENARIO + abi_index,
                (uint64_t)scenario_status, (uint64_t)FIRME_BUSY);
            return VAL_ERROR;
        }

        if ((scenario.granule_count == 0ULL) ||
            (scenario.completed_count >= scenario.granule_count) ||
            ((abis[abi_index] == PAL_FIRME_GPT_ABI_GPI_SET) &&
             (scenario.completed_count != 0ULL))) {
            val_firme_gpt_rule_result_fail(
                out, GPT_BUSY_RETRY_STEP_SCENARIO + abi_index,
                scenario.completed_count, scenario.granule_count);
            goto cleanup_fail;
        }
        remaining = scenario.granule_count - scenario.completed_count;

        gpt_busy_retry_invoke(
            abis[abi_index], &scenario, scenario.cookie, &result);
        actual_status = val_firme_status_to_i64(result.x0);
        if (actual_status != (int64_t)FIRME_BUSY) {
            LOG_ERROR(NULL,
                      "%s: ABI=%u status=%lld expected BUSY",
                      GPT_BUSY_RETRY_TEST_NAME,
                      (unsigned int)abis[abi_index],
                      (long long)actual_status);
            val_firme_gpt_rule_result_fail(
                out, GPT_BUSY_RETRY_STEP_STATUS + abi_index,
                result.x0, (uint64_t)FIRME_BUSY);
            goto cleanup_fail;
        }

        /*
         * Step 3: BUSY reports no progress. Secure and Realm callers verify
         * that the locally observable prefix is unchanged.
         */
        if ((world != FIRME_ACS_WORLD_NSEL2) &&
            ((scenario.initial_gpi != FIRME_GPI_NONSECURE) ||
             (scenario.target_gpi != expected_target))) {
            val_firme_gpt_rule_result_fail(
                out, GPT_BUSY_RETRY_STEP_SCENARIO + abi_index,
                scenario.initial_gpi, scenario.target_gpi);
            goto cleanup_fail;
        }

        if ((world != FIRME_ACS_WORLD_NSEL2) &&
            (val_firme_gpt_local_prefix_matches(
                 scenario.base_pa, scenario.granule_count,
                 scenario.completed_count, out,
                 GPT_BUSY_RETRY_STEP_EFFECT + abi_index) == VAL_FALSE))
            goto cleanup_fail;

        /*
         * Step 4: retry the operation. OP_CONTINUE must use the cookie
         * returned by BUSY; GPI_SET repeats the original request.
         */
        retry_cookie = (abis[abi_index] ==
                        PAL_FIRME_GPT_ABI_OP_CONTINUE) ?
                       result.x2 : scenario.cookie;
        gpt_busy_retry_invoke(
            abis[abi_index], &scenario, retry_cookie, &retry_result);
        retry_status = val_firme_status_to_i64(retry_result.x0);
        if (val_firme_gpt_operation_count_valid(
                retry_status, retry_result.x1, remaining) == VAL_FALSE) {
            val_firme_gpt_rule_result_fail(
                out, GPT_BUSY_RETRY_STEP_RETRY + abi_index,
                retry_result.x0, retry_cookie);
            goto cleanup_fail;
        }

        scenario_status = pal_firme_gpt_scenario_cleanup(
            (uint32_t)world, abis[abi_index],
            (int64_t)FIRME_BUSY, &scenario);
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_BUSY_RETRY_STEP_CLEANUP + abi_index,
                (uint64_t)scenario_status, 0ULL);
            return VAL_ERROR;
        }
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);

cleanup_fail:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, abis[abi_index],
        (int64_t)FIRME_BUSY, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_BUSY_RETRY_STEP_CLEANUP + abi_index,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }

    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0124 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_busy_retry_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_busy_retry_run(FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0124 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_busy_retry_run_secure(uint64_t base_pa,
                                    firme_rule_res_t *out)
{
    return gpt_busy_retry_run(FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0124 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_busy_retry_run_realm(uint64_t base_pa,
                                   firme_rule_res_t *out)
{
    return gpt_busy_retry_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
