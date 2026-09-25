/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_INFO_STATUS_TEST_NAME "gpt_info_status"

#define GPT_INFO_STATUS_STEP_ADVERTISEMENT UINT64_C(1)
#define GPT_INFO_STATUS_STEP_WINDOW        UINT64_C(2)
#define GPT_INFO_STATUS_STEP_SUCCESS       UINT64_C(3)
#define GPT_INFO_STATUS_STEP_BASE_ALIGN    UINT64_C(4)
#define GPT_INFO_STATUS_STEP_TOP_ALIGN     UINT64_C(5)
#define GPT_INFO_STATUS_STEP_ORDER_EQUAL   UINT64_C(6)
#define GPT_INFO_STATUS_STEP_ORDER_LESS    UINT64_C(7)
#define GPT_INFO_STATUS_STEP_PPS           UINT64_C(8)

/**
 * @brief Invoke GPT_INFO and require one exact status without consuming outputs.
 * @param range_base Inclusive input range base.
 * @param range_top Exclusive input range top.
 * @param expected_status Expected architected status.
 * @param out Rule result.
 * @param step Diagnostic step.
 * @return VAL_TRUE when the status matches, otherwise VAL_FALSE.
 */
static uint32_t gpt_info_status_expect(uint64_t range_base,
                                       uint64_t range_top,
                                       int64_t expected_status,
                                       firme_rule_res_t *out,
                                       uint64_t step)
{
    firme_res_t result = {0};
    int64_t actual_status;

    val_firme_gm_gpt_info(range_base, range_top, &result);
    actual_status = val_firme_status_to_i64(result.x0);
    if (actual_status == expected_status)
        return VAL_TRUE;

    LOG_ERROR(NULL,
              "%s: step=%llu status=%lld expected=%lld base=%llx top=%llx",
              GPT_INFO_STATUS_TEST_NAME, (unsigned long long)step,
              (long long)actual_status, (long long)expected_status,
              (unsigned long long)range_base,
              (unsigned long long)range_top);
    out->fail_step = step;
    out->last_x0 = result.x0;
    out->last_x1 = (uint64_t)expected_status;
    return VAL_FALSE;
}

/**
 * @brief Validate FIRME_GM_GPT_INFO status conditions for R0139.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
static int gpt_info_status_run(uint64_t base_pa, firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result = {0};
    uint64_t misaligned_base;
    uint64_t misaligned_top;
    uint64_t outside_base;
    uint64_t outside_top;

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: require the mandatory GPT_INFO discovery ABI. */
    if (val_firme_gpt_info_advertised(
            out, GPT_INFO_STATUS_STEP_ADVERTISEMENT) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 2: derive valid boundaries from advertised L0GPTSZ and PPS. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_INFO_STATUS_STEP_WINDOW) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 3: a valid range must complete with SUCCESS. */
    val_firme_gm_gpt_info(window.range_base, window.range_top, &result);
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_INFO_STATUS_STEP_SUCCESS) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 4: a misaligned protected-range base is invalid. */
    misaligned_base = window.range_base + window.granule_size;
    if (gpt_info_status_expect(
            misaligned_base, window.range_top,
            (int64_t)FIRME_INVALID_PARAMETERS, out,
            GPT_INFO_STATUS_STEP_BASE_ALIGN) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 5: a misaligned protected-range top is invalid. */
    misaligned_top = window.range_top - window.granule_size;
    if (gpt_info_status_expect(
            window.range_base, misaligned_top,
            (int64_t)FIRME_INVALID_PARAMETERS, out,
            GPT_INFO_STATUS_STEP_TOP_ALIGN) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 6: an empty range whose top equals its base is invalid. */
    if (gpt_info_status_expect(
            window.range_base, window.range_base,
            (int64_t)FIRME_INVALID_PARAMETERS, out,
            GPT_INFO_STATUS_STEP_ORDER_EQUAL) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 7: an aligned top less than its base is also invalid. */
    if (gpt_info_status_expect(
            window.range_top, window.range_base,
            (int64_t)FIRME_INVALID_PARAMETERS, out,
            GPT_INFO_STATUS_STEP_ORDER_LESS) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 8: an aligned range starting at PPS lies outside protected space. */
    outside_base = window.protected_pa_size;
    if (window.range_size > (UINT64_MAX - outside_base))
        return VAL_ERROR;
    outside_top = outside_base + window.range_size;
    if (gpt_info_status_expect(
            outside_base, outside_top,
            (int64_t)FIRME_INVALID_PARAMETERS, out,
            GPT_INFO_STATUS_STEP_PPS) == VAL_FALSE)
        return VAL_SUCCESS;

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 * @brief Validate R0139 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_info_status_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_info_status_run(base_pa, out);
}

/**
 * @brief Validate R0139 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_info_status_run_secure(uint64_t base_pa,
                                     firme_rule_res_t *out)
{
    return gpt_info_status_run(base_pa, out);
}

/**
 * @brief Validate R0139 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_info_status_run_realm(uint64_t base_pa,
                                    firme_rule_res_t *out)
{
    return gpt_info_status_run(base_pa, out);
}
