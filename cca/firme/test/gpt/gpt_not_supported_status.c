/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_gpt.h"
#include "pal_platform.h"
#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_firme_service.h"
#include "val_firme_suite.h"
#include "val_log.h"

#define GPT_NOT_SUPPORTED_TEST_NAME "gpt_not_supported_status"

#define GPT_NOT_SUPPORTED_STEP_DISCOVERY UINT64_C(1)
#define GPT_NOT_SUPPORTED_STEP_SCENARIO  UINT64_C(8)
#define GPT_NOT_SUPPORTED_STEP_RESULT    UINT64_C(16)
#define GPT_NOT_SUPPORTED_STEP_CLEANUP   UINT64_C(24)

#define GPT_NOT_SUPPORTED_ABI_COUNT 2U
#define GPT_NOT_SUPPORTED_WORLD_COUNT 3U
#define GPT_NOT_SUPPORTED_GRANULE_COUNT UINT64_C(1)
#define GPT_NOT_SUPPORTED_ABSENCE_ARGUMENT UINT64_C(0)

typedef struct {
    pal_firme_gpt_abi_t pal_abi;
    uint64_t feature_bit;
} gpt_not_supported_abi_t;

typedef struct {
    pal_firme_gpt_scenario_t scenario;
    firme_res_t response;
    uint32_t quarantined;
} gpt_not_supported_retained_t;

static gpt_not_supported_retained_t
    g_gpt_not_supported_retained[GPT_NOT_SUPPORTED_WORLD_COUNT];

/**
 * @brief Invoke a defined GPI ABI without retrying or inventing progress.
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
 * @brief Build an unavailable-entry-point probe without creating an operation.
 * @param world Caller whose disposable scratch range is used for GPI_SET.
 * @param abi Defined ABI reported absent by discovery.
 * @param base_pa Dispatcher-supplied exclusive test address.
 * @param scenario Zeroed input fields; no pending operation is asserted.
 * @return VAL_SUCCESS for a bounded frame, VAL_ERROR for invalid storage.
 * OP_CONTINUE's zero argument is an absence probe, not a fabricated live cookie.
 */
static int gpt_not_supported_absent_input(
    firme_acs_world_t world, const gpt_not_supported_abi_t *abi,
    uint64_t base_pa, pal_firme_gpt_scenario_t *scenario)
{
    uint64_t granule_size = PLAT_FIRME_GPT_GRANULE_SIZE;
    uint64_t protected_size = PLAT_FIRME_GPT_PROTECTED_PA_SIZE;

    *scenario = (pal_firme_gpt_scenario_t){0};
    if (abi->pal_abi == PAL_FIRME_GPT_ABI_OP_CONTINUE) {
        scenario->cookie = GPT_NOT_SUPPORTED_ABSENCE_ARGUMENT;
        return VAL_SUCCESS;
    }
    if (!base_pa || base_pa != val_firme_gpt_base_for_world(world) ||
        !granule_size || (granule_size & (granule_size - 1ULL)) ||
        base_pa % granule_size || base_pa >= protected_size ||
        granule_size > protected_size - base_pa)
        return VAL_ERROR;

    scenario->base_pa = base_pa;
    scenario->granule_count = GPT_NOT_SUPPORTED_GRANULE_COUNT;
    scenario->initial_gpi = FIRME_GPI_NONSECURE;
    scenario->target_gpi = FIRME_GPI_NONSECURE;
    return VAL_SUCCESS;
}

/**
 * @brief Retain unexpected state and request dispatcher quarantine, not guessed cleanup.
 * @param world Actual caller whose state is now unconfirmed.
 * @param scenario Exact issued input, including any provider-owned cookie.
 * @param result Unexpected response, retained without treating unused words as progress.
 * @param out Failed rule result promoted to a safety error.
 * @return VAL_ERROR; no further GPI operation or resource release is attempted.
 */
static int gpt_not_supported_quarantine(
    firme_acs_world_t world, const pal_firme_gpt_scenario_t *scenario,
    const firme_res_t *result, firme_rule_res_t *out)
{
    g_gpt_not_supported_retained[world].scenario = *scenario;
    g_gpt_not_supported_retained[world].response = *result;
    g_gpt_not_supported_retained[world].quarantined = VAL_TRUE;
    out->status = ACS_STATUS_ERR;
    LOG_ERROR(NULL, "%s: world=%u state unconfirmed; retaining inputs and response\n",
              GPT_NOT_SUPPORTED_TEST_NAME, (unsigned int)world);
    return VAL_ERROR;
}

/**
 * @brief Validate R0122 from one FIRME caller world.
 * @param world Caller world.
 * @param base_pa Default platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
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
    uint32_t prepared;
    uint32_t observed_case = VAL_FALSE;

    if (!out)
        return VAL_ERROR;

    val_firme_gpt_rule_result_init(out);
    if ((uint32_t)world >= GPT_NOT_SUPPORTED_WORLD_COUNT) {
        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }
    if (g_gpt_not_supported_retained[world].quarantined) {
        out->status = ACS_STATUS_ERR;
        LOG_ERROR(NULL, "%s: world=%u remains quarantined until reset\n",
                  GPT_NOT_SUPPORTED_TEST_NAME, (unsigned int)world);
        return VAL_ERROR;
    }
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

        scenario = (pal_firme_gpt_scenario_t){0};
        result = (firme_res_t){0};
        prepared = VAL_FALSE;
        if (advertised == VAL_FALSE) {
            if (gpt_not_supported_absent_input(world, &abis[abi_index],
                                               base_pa, &scenario) != VAL_SUCCESS) {
                val_firme_gpt_rule_result_fail(out,
                    GPT_NOT_SUPPORTED_STEP_SCENARIO + abi_index, base_pa,
                    PLAT_FIRME_GPT_GRANULE_SIZE);
                return VAL_ERROR;
            }
        } else {
            scenario_status = pal_firme_gpt_scenario_prepare(
                (uint32_t)world, abis[abi_index].pal_abi,
                (int64_t)FIRME_NOT_SUPPORTED, base_pa, &scenario);
            if (scenario_status == PAL_FIRME_GPT_SCENARIO_UNAVAILABLE)
                continue;
            if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
                val_firme_gpt_rule_result_fail(
                    out, GPT_NOT_SUPPORTED_STEP_SCENARIO + abi_index,
                    (uint64_t)scenario_status, (uint64_t)FIRME_NOT_SUPPORTED);
                return gpt_not_supported_quarantine(world, &scenario, &result, out);
            }
            prepared = VAL_TRUE;
        }

        gpt_not_supported_invoke(&abis[abi_index], &scenario, &result);
        actual_status = val_firme_status_to_i64(result.x0);

        if (actual_status != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL,
                      "%s: ABI index=%u advertised=%u status=%lld expected NOT_SUPPORTED\n",
                      GPT_NOT_SUPPORTED_TEST_NAME,
                      (unsigned int)abi_index,
                      (unsigned int)advertised,
                      (long long)actual_status);
            val_firme_gpt_rule_result_fail(
                out, GPT_NOT_SUPPORTED_STEP_RESULT + abi_index,
                result.x0, (uint64_t)FIRME_NOT_SUPPORTED);
            if (actual_status != (int64_t)FIRME_INVALID_PARAMETERS)
                return gpt_not_supported_quarantine(world, &scenario, &result, out);
            if (prepared == VAL_TRUE &&
                pal_firme_gpt_scenario_cleanup((uint32_t)world,
                    abis[abi_index].pal_abi, (int64_t)FIRME_NOT_SUPPORTED,
                    &scenario) != PAL_FIRME_GPT_SCENARIO_READY)
                return gpt_not_supported_quarantine(world, &scenario, &result, out);
            return VAL_ERROR;
        }
        observed_case = VAL_TRUE;

        if (prepared == VAL_FALSE)
            continue;
        scenario_status = pal_firme_gpt_scenario_cleanup(
            (uint32_t)world, abis[abi_index].pal_abi,
            (int64_t)FIRME_NOT_SUPPORTED, &scenario);
        if (scenario_status != PAL_FIRME_GPT_SCENARIO_READY) {
            val_firme_gpt_rule_result_fail(
                out, GPT_NOT_SUPPORTED_STEP_CLEANUP + abi_index,
                (uint64_t)scenario_status, 0ULL);
            return gpt_not_supported_quarantine(world, &scenario, &result, out);
        }
    }

    out->status = (observed_case == VAL_TRUE) ?
                  (uint64_t)ACS_STATUS_PASS :
                  (uint64_t)ACS_STATUS_SKIP;
    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0122 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
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
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
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
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_not_supported_status_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_not_supported_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
