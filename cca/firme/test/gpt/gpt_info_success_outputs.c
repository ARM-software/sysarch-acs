/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "pal_platform.h"
#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_INFO_SUCCESS_OUTPUTS_TEST_NAME "gpt_info_success_outputs"

#define GPT_INFO_SUCCESS_OUTPUTS_STEP_ADVERTISEMENT UINT64_C(1)
#define GPT_INFO_SUCCESS_OUTPUTS_STEP_WINDOW        UINT64_C(2)
#define GPT_INFO_SUCCESS_OUTPUTS_STEP_FIELDS        UINT64_C(3)
#define GPT_INFO_SUCCESS_OUTPUTS_STEP_STATE         UINT64_C(4)

/**
 * @brief Validate FIRME_GM_GPT_INFO SUCCESS outputs for R0140.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
static int gpt_info_success_outputs_run(uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result = {0};
    uint64_t expected_state = PLAT_FIRME_GPT_INFO_PAR_STATE;

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: require the mandatory GPT_INFO discovery ABI. */
    if (val_firme_gpt_info_advertised(
            out, GPT_INFO_SUCCESS_OUTPUTS_STEP_ADVERTISEMENT) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 2: query exactly one independently described protected range. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_INFO_SUCCESS_OUTPUTS_STEP_WINDOW) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 3: validate status, non-empty prefix, bounds and top alignment. */
    val_firme_gm_gpt_info(window.range_base, window.range_top, &result);
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_INFO_SUCCESS_OUTPUTS_STEP_FIELDS) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 4: returned state must match the independently known range state. */
    if (result.x2 != expected_state) {
        LOG_ERROR(NULL,
                  "%s: state=%llu expected=%llu returned_top=%llx",
                  GPT_INFO_SUCCESS_OUTPUTS_TEST_NAME,
                  (unsigned long long)result.x2,
                  (unsigned long long)expected_state,
                  (unsigned long long)result.x1);
        out->fail_step = GPT_INFO_SUCCESS_OUTPUTS_STEP_STATE;
        out->last_x0 = result.x2;
        out->last_x1 = expected_state;
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 * @brief Validate R0140 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_info_success_outputs_run_ns(uint64_t base_pa,
                                          firme_rule_res_t *out)
{
    return gpt_info_success_outputs_run(base_pa, out);
}

/**
 * @brief Validate R0140 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_info_success_outputs_run_secure(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return gpt_info_success_outputs_run(base_pa, out);
}

/**
 * @brief Validate R0140 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_info_success_outputs_run_realm(uint64_t base_pa,
                                             firme_rule_res_t *out)
{
    return gpt_info_success_outputs_run(base_pa, out);
}
