/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_gpt.h"
#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_firme_service.h"
#include "val_log.h"

#define GPT_NOT_SUPPORTED_TEST_NAME "gpt_not_supported_status"

#define GPT_NOT_SUPPORTED_STEP_DISCOVERY UINT64_C(1)
#define GPT_NOT_SUPPORTED_STEP_SCENARIO  UINT64_C(8)
#define GPT_NOT_SUPPORTED_STEP_RESULT    UINT64_C(16)
#define GPT_NOT_SUPPORTED_STEP_CLEANUP   UINT64_C(24)

#define GPT_NOT_SUPPORTED_ABI_COUNT 2U

typedef struct {
    pal_firme_gpt_abi_t pal_abi;
    uint64_t feature_bit;
} gpt_not_supported_abi_t;

/**
 * @brief Invoke one advertised GPI management ABI with valid state.
 * @param abi ABI descriptor.
 * @param scenario Prepared valid scenario.
 * @param result Returned ABI result.
 * @return None.
 */
static void gpt_not_supported_invoke(
    const gpt_not_supported_abi_t *abi,
    const pal_firme_gpt_scenario_t *scenario,
    firme_res_t *result)
{
    if (abi->pal_abi == PAL_FIRME_GPT_ABI_GPI_SET) {
        val_firme_gm_gpi_set_do(
            scenario->base_pa, scenario->granule_count,
            scenario->target_gpi, result);
    } else {
        val_firme_gm_gpi_op_continue(scenario->cookie, result);
    }
}

/**
 * @brief Validate R0122 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
static int gpt_not_supported_run(firme_acs_world_t world,
                                 uint64_t base_pa,
                                 firme_rule_res_t *out)
{
    static const gpt_not_supported_abi_t abis[
        GPT_NOT_SUPPORTED_ABI_COUNT] = {
        {
            .pal_abi = PAL_FIRME_GPT_ABI_GPI_SET,
            .feature_bit = FIRME_GM_FEATURE_REG0_GPI_SET_BIT,
        },
        {
            .pal_abi = PAL_FIRME_GPT_ABI_OP_CONTINUE,
            .feature_bit = FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT,
        },
    };
    pal_firme_gpt_scenario_t scenario = {0};
    firme_res_t features_result = {0};
    firme_res_t result = {0};
    pal_firme_gpt_scenario_status_t scenario_status;
    uint64_t feature_reg0 = 0ULL;
    int64_t actual_status;
    uint32_t advertised;
    uint32_t abi_index;
    uint32_t observed_case = VAL_FALSE;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);

    /*
     * Step 1: correlate NOT_SUPPORTED with feature discovery. Unadvertised
     * ABIs are not invoked, avoiding a deliberately unknown FID path.
     */
    for (abi_index = 0U; abi_index < GPT_NOT_SUPPORTED_ABI_COUNT;
         abi_index++) {
        advertised = VAL_FALSE;
        if (val_firme_service_abi_is_advertised(
                FIRME_SERVICE_ID_GRANULE_MGMT,
                abis[abi_index].feature_bit, &advertised,
                &feature_reg0, &features_result) != VAL_SUCCESS) {
            val_firme_gpt_rule_result_fail(
                out, GPT_NOT_SUPPORTED_STEP_DISCOVERY + abi_index,
                features_result.x0, features_result.x1);
            return VAL_ERROR;
        }

        if (advertised == VAL_FALSE)
            continue;

        /*
         * Step 2: request a valid invocation from a caller for which this
         * advertised ABI is not available.
         */
        scenario = (pal_firme_gpt_scenario_t){0};
        result = (firme_res_t){0};
        scenario_status = pal_firme_gpt_scenario_prepare(
            (uint32_t)world, abis[abi_index].pal_abi,
            (int64_t)FIRME_NOT_SUPPORTED, base_pa, &scenario);
        if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE)
            continue;
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_NOT_SUPPORTED_STEP_SCENARIO + abi_index,
                (uint64_t)scenario_status,
                (uint64_t)FIRME_NOT_SUPPORTED);
            return VAL_ERROR;
        }

        gpt_not_supported_invoke(&abis[abi_index], &scenario, &result);
        actual_status = val_firme_status_to_i64(result.x0);

        /* Step 3: the caller-disallowed path must return NOT_SUPPORTED. */
        if (actual_status != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL,
                      "%s: ABI index=%u status=%lld expected NOT_SUPPORTED",
                      GPT_NOT_SUPPORTED_TEST_NAME,
                      (unsigned int)abi_index,
                      (long long)actual_status);
            val_firme_gpt_rule_result_fail(
                out, GPT_NOT_SUPPORTED_STEP_RESULT + abi_index,
                result.x0, (uint64_t)FIRME_NOT_SUPPORTED);
            goto cleanup_fail;
        }
        observed_case = VAL_TRUE;

        scenario_status = pal_firme_gpt_scenario_cleanup(
            (uint32_t)world, abis[abi_index].pal_abi,
            (int64_t)FIRME_NOT_SUPPORTED, &scenario);
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_NOT_SUPPORTED_STEP_CLEANUP + abi_index,
                (uint64_t)scenario_status, 0ULL);
            return VAL_ERROR;
        }
    }

    out->status = (observed_case == VAL_TRUE) ?
                  (uint64_t)ACS_STATUS_PASS :
                  (uint64_t)ACS_STATUS_SKIP;
    return VAL_SUCCESS;

cleanup_fail:
    scenario_status = pal_firme_gpt_scenario_cleanup(
        (uint32_t)world, abis[abi_index].pal_abi,
        (int64_t)FIRME_NOT_SUPPORTED, &scenario);
    if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
        val_firme_gpt_rule_result_fail(
            out, GPT_NOT_SUPPORTED_STEP_CLEANUP + abi_index,
            (uint64_t)scenario_status, 0ULL);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 * @brief Validate R0122 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_not_supported_status_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_supported_run(FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 * @brief Validate R0122 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_not_supported_status_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_supported_run(FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 * @brief Validate R0122 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when a rule result is produced.
 */
int firme_gpt_not_supported_status_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_supported_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
