/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_PROTECTED_RANGE_SIZE_TEST_NAME "gpt_protected_range_size"

#define GPT_PROTECTED_RANGE_SIZE_STEP_ADVERTISEMENT UINT64_C(1)
#define GPT_PROTECTED_RANGE_SIZE_STEP_WINDOW        UINT64_C(2)
#define GPT_PROTECTED_RANGE_SIZE_STEP_EXACT         UINT64_C(3)
#define GPT_PROTECTED_RANGE_SIZE_STEP_MISALIGNED    UINT64_C(4)

/**
 * @brief Initialize the R0103 result.
 * @param out Rule result.
 * @return None.
 */
static void gpt_protected_range_size_init(firme_rule_res_t *out)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 * @brief Record an R0103 failure.
 * @param out Rule result.
 * @param step Failing algorithm step.
 * @param result Last ABI result.
 * @return None.
 */
static void gpt_protected_range_size_fail(
    firme_rule_res_t *out, uint64_t step, const firme_res_t *result)
{
    out->fail_step = step;
    out->last_x0 = result->x0;
    out->last_x1 = result->x1;
}

/**
 * @brief Validate protected-address-range size for R0103.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
static int gpt_protected_range_size_run(uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result = {0};
    uint64_t misaligned_top;

    if (!out)
        return VAL_ERROR;

    gpt_protected_range_size_init(out);

    /* Step 1: GPT_INFO is mandatory at every Granule Management instance. */
    if (val_firme_gpt_info_advertised(
            out, GPT_PROTECTED_RANGE_SIZE_STEP_ADVERTISEMENT) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 2: derive one protected range from advertised L0GPTSZ and PPS. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_PROTECTED_RANGE_SIZE_STEP_WINDOW) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 3: a query spanning exactly one protected range must succeed. */
    val_firme_gm_gpt_info(window.range_base, window.range_top, &result);
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_PROTECTED_RANGE_SIZE_STEP_EXACT) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 4: a top inside that L0GPTSZ-sized range is not a valid boundary. */
    misaligned_top = window.range_top - window.granule_size;
    val_firme_gm_gpt_info(window.range_base, misaligned_top, &result);
    if (val_firme_status_to_i64(result.x0) !=
        (int64_t)FIRME_INVALID_PARAMETERS) {
        LOG_ERROR(NULL,
                  "%s: partial protected range accepted top=%llx x0=%llx",
                  GPT_PROTECTED_RANGE_SIZE_TEST_NAME,
                  (unsigned long long)misaligned_top,
                  (unsigned long long)result.x0);
        gpt_protected_range_size_fail(
            out, GPT_PROTECTED_RANGE_SIZE_STEP_MISALIGNED, &result);
        return VAL_ERROR;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0103 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_protected_range_size_run_ns(uint64_t base_pa,
                                          firme_rule_res_t *out)
{
    return gpt_protected_range_size_run(base_pa, out);
}

/**
 * @brief Validate R0103 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_protected_range_size_run_secure(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return gpt_protected_range_size_run(base_pa, out);
}

/**
 * @brief Validate R0103 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_protected_range_size_run_realm(uint64_t base_pa,
                                             firme_rule_res_t *out)
{
    return gpt_protected_range_size_run(base_pa, out);
}
