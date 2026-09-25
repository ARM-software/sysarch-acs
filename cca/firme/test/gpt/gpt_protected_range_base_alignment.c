/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_PROTECTED_RANGE_BASE_TEST_NAME "gpt_protected_range_base_alignment"

#define GPT_PROTECTED_RANGE_BASE_STEP_ADVERTISEMENT UINT64_C(1)
#define GPT_PROTECTED_RANGE_BASE_STEP_WINDOW        UINT64_C(2)
#define GPT_PROTECTED_RANGE_BASE_STEP_ALIGNED       UINT64_C(3)
#define GPT_PROTECTED_RANGE_BASE_STEP_MISALIGNED    UINT64_C(4)

/**
 * @brief Initialize the R0102 result.
 * @param out Rule result.
 * @return None.
 */
static void gpt_protected_range_base_init(firme_rule_res_t *out)
{
    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 * @brief Record an R0102 failure.
 * @param out Rule result.
 * @param step Failing algorithm step.
 * @param result Last ABI result.
 * @return None.
 */
static void gpt_protected_range_base_fail(
    firme_rule_res_t *out, uint64_t step, const firme_res_t *result)
{
    out->fail_step = step;
    out->last_x0 = result->x0;
    out->last_x1 = result->x1;
}

/**
 * @brief Validate protected-address-range base alignment for R0102.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
static int gpt_protected_range_base_run(uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t result = {0};
    uint64_t misaligned_base;

    if (!out)
        return VAL_ERROR;

    gpt_protected_range_base_init(out);

    /* Step 1: GPT_INFO is mandatory at every Granule Management instance. */
    if (val_firme_gpt_info_advertised(
            out, GPT_PROTECTED_RANGE_BASE_STEP_ADVERTISEMENT) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 2: derive range alignment from DUT-advertised L0GPTSZ and PPS. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_PROTECTED_RANGE_BASE_STEP_WINDOW) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 3: an L0GPTSZ-aligned protected-range base must be accepted. */
    val_firme_gm_gpt_info(window.range_base, window.range_top, &result);
    if (val_firme_gpt_info_success_valid(
            &window, &result, out,
            GPT_PROTECTED_RANGE_BASE_STEP_ALIGNED) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 4: a PGS-aligned but protected-range-misaligned base is invalid. */
    misaligned_base = window.range_base + window.granule_size;
    val_firme_gm_gpt_info(misaligned_base, window.range_top, &result);
    if (val_firme_status_to_i64(result.x0) !=
        (int64_t)FIRME_INVALID_PARAMETERS) {
        LOG_ERROR(NULL,
                  "%s: misaligned base accepted base=%llx x0=%llx",
                  GPT_PROTECTED_RANGE_BASE_TEST_NAME,
                  (unsigned long long)misaligned_base,
                  (unsigned long long)result.x0);
        gpt_protected_range_base_fail(
            out, GPT_PROTECTED_RANGE_BASE_STEP_MISALIGNED, &result);
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 * @brief Validate R0102 from the Normal-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_protected_range_base_alignment_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_protected_range_base_run(base_pa, out);
}

/**
 * @brief Validate R0102 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_protected_range_base_alignment_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_protected_range_base_run(base_pa, out);
}

/**
 * @brief Validate R0102 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS when an architectural result is produced.
 */
int firme_gpt_protected_range_base_alignment_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_protected_range_base_run(base_pa, out);
}
