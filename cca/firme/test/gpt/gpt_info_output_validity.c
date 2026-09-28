/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_INFO_OUTPUT_VALIDITY_TEST_NAME "gpt_info_output_validity"

#define GPT_INFO_OUTPUT_VALIDITY_STEP_ADVERTISEMENT UINT64_C(1)
#define GPT_INFO_OUTPUT_VALIDITY_STEP_WINDOW        UINT64_C(2)
#define GPT_INFO_OUTPUT_VALIDITY_STEP_SUCCESS       UINT64_C(3)
#define GPT_INFO_OUTPUT_VALIDITY_STEP_POISON        UINT64_C(4)
#define GPT_INFO_OUTPUT_VALIDITY_STEP_FAILURE       UINT64_C(5)

#define GPT_INFO_OUTPUT_POISON_TOP   UINT64_C(0xA5A5A5A5A5A5A5A5)
#define GPT_INFO_OUTPUT_POISON_STATE UINT64_C(0x5A5A5A5A5A5A5A5A)

/**
 * @brief Validate conditional GPT_INFO output validity for R0141.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
static int gpt_info_output_validity_run(uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result = {0};
    firme_res_t poison_mask = {0};
    firme_res_t poison_value = {0};
    uint64_t misaligned_base;
    int64_t status;

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: require the mandatory GPT_INFO discovery ABI. */
    if (val_firme_gpt_info_advertised(
            out, GPT_INFO_OUTPUT_VALIDITY_STEP_ADVERTISEMENT) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 2: derive one valid protected-address-range query. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_INFO_OUTPUT_VALIDITY_STEP_WINDOW) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 3: consume and validate x1/x2 only after SUCCESS. */
    val_firme_gm_gpt_info(window.range_base, window.range_top, &result);
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_INFO_OUTPUT_VALIDITY_STEP_SUCCESS) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /*
     * Step 4: poison x1/x2 for a failing invocation. The caller must decide
     * solely from x0 and must not consume outputs that ALP4 declares invalid.
     */
    poison_mask.x1 = UINT64_MAX;
    poison_mask.x2 = UINT64_MAX;
    poison_value.x1 = GPT_INFO_OUTPUT_POISON_TOP;
    poison_value.x2 = GPT_INFO_OUTPUT_POISON_STATE;
    if (val_firme_test_output_poison_enable(
            FIRME_FID_GM_GPT_INFO, &poison_mask,
            &poison_value) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: failed to enable output poisoning",
                  GPT_INFO_OUTPUT_VALIDITY_TEST_NAME);
        out->fail_step = GPT_INFO_OUTPUT_VALIDITY_STEP_POISON;
        return VAL_ERROR;
    }

    /* Step 5: invalid input must be rejected without using poisoned x1/x2. */
    misaligned_base = window.range_base + window.granule_size;
    val_firme_gm_gpt_info(misaligned_base, window.range_top, &result);
    val_firme_test_output_poison_disable();

    status = val_firme_status_to_i64(result.x0);
    if (status != (int64_t)FIRME_INVALID_PARAMETERS) {
        LOG_ERROR(NULL,
                  "%s: invalid query status=%lld base=%llx",
                  GPT_INFO_OUTPUT_VALIDITY_TEST_NAME, (long long)status,
                  (unsigned long long)misaligned_base);
        out->fail_step = GPT_INFO_OUTPUT_VALIDITY_STEP_FAILURE;
        out->last_x0 = result.x0;
        out->last_x1 = (uint64_t)(int64_t)FIRME_INVALID_PARAMETERS;
        return val_firme_gpt_rule_result_status(out);
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0141 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_info_output_validity_run_ns(uint64_t base_pa,
                                          firme_rule_res_t *out)
{
    return gpt_info_output_validity_run(base_pa, out);
}

/**
 * @brief Validate R0141 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_info_output_validity_run_secure(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return gpt_info_output_validity_run(base_pa, out);
}

/**
 * @brief Validate R0141 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_info_output_validity_run_realm(uint64_t base_pa,
                                             firme_rule_res_t *out)
{
    return gpt_info_output_validity_run(base_pa, out);
}
