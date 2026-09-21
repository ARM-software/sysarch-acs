/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_SUCCESS_NONZERO_TEST_NAME "gpt_success_count_nonzero"
#define GPT_SUCCESS_NONZERO_STEP_ADVERTISE UINT64_C(1)
#define GPT_SUCCESS_NONZERO_STEP_SETUP     UINT64_C(2)
#define GPT_SUCCESS_NONZERO_STEP_RESULT    UINT64_C(3)
#define GPT_SUCCESS_NONZERO_STEP_CLEANUP   UINT64_C(4)

/**
 *   @brief    - Initialise an R0120 result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void gpt_success_nonzero_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Require a completed GPI operation to report non-zero progress.
 *   @param    - result    : Composite GPI operation result.
 *   @param    - out       : Optional rule result.
 *   @param    - fail_step : Failure step identifier.
 *   @return   - VAL_TRUE when SUCCESS reports non-zero progress, otherwise VAL_FALSE.
 **/
static uint32_t gpt_success_nonzero_check(const firme_res_t *result,
                                          firme_rule_res_t *out,
                                          uint64_t fail_step)
{
    if (!result ||
        (val_firme_as_i64(result->x0) != (int64_t)FIRME_SUCCESS) ||
        (result->x1 == 0ULL)) {
        LOG_ERROR(NULL, "%s: expected SUCCESS with non-zero count x0=%llx x1=%llx",
                  GPT_SUCCESS_NONZERO_TEST_NAME,
                  (unsigned long long)(result ? result->x0 : 0ULL),
                  (unsigned long long)(result ? result->x1 : 0ULL));
        if (out) {
            out->fail_step = fail_step;
            out->last_x0 = result ? result->x0 : 0ULL;
            out->last_x1 = result ? result->x1 : 0ULL;
        }
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Complete one GPI operation and validate its SUCCESS progress.
 *   @param    - base_pa    : Base PA for the GPT test window.
 *   @param    - target_gpi : Target GPI for the operation.
 *   @param    - out        : Optional rule result.
 *   @param    - fail_step  : Failure step identifier.
 *   @return   - VAL_TRUE for a valid non-zero SUCCESS result, otherwise VAL_FALSE.
 **/
static uint32_t gpt_success_nonzero_invoke(uint64_t base_pa, uint64_t target_gpi,
                                           firme_rule_res_t *out,
                                           uint64_t fail_step)
{
    firme_res_t result = {0};
    const uint64_t granule_count = UINT64_C(1);

    if (val_firme_gm_gpi_set_complete(base_pa, granule_count, target_gpi,
                                      &result) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: GPI operation did not reach a valid terminal result",
                  GPT_SUCCESS_NONZERO_TEST_NAME);
        if (out) {
            out->fail_step = fail_step;
            out->last_x0 = result.x0;
            out->last_x1 = result.x1;
        }
        return VAL_FALSE;
    }

    return gpt_success_nonzero_check(&result, out, fail_step);
}

/**
 *   @brief    - Run the R0120 transition check for Secure or Realm callers.
 *   @param    - base_pa    : Base PA for the GPT test window.
 *   @param    - target_gpi : Caller-world target GPI.
 *   @param    - out        : Optional rule result.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
static int gpt_success_nonzero_run_changing(uint64_t base_pa,
                                            uint64_t target_gpi,
                                            firme_rule_res_t *out)
{
    uint32_t passed = VAL_TRUE;

    gpt_success_nonzero_init(out);

    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_SUCCESS_NONZERO_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 1: establish Non-secure as the current GPI. */
    if (gpt_success_nonzero_invoke(base_pa, FIRME_GPI_NONSECURE, out,
                                   GPT_SUCCESS_NONZERO_STEP_SETUP) == VAL_FALSE)
        passed = VAL_FALSE;

    /* Step 2: perform a changing transition and check its SUCCESS count. */
    if ((passed == VAL_TRUE) &&
        (gpt_success_nonzero_invoke(base_pa, target_gpi, out,
                                    GPT_SUCCESS_NONZERO_STEP_RESULT) == VAL_FALSE))
        passed = VAL_FALSE;

    /* Step 3: complete cleanup through the same state-machine helper. */
    if (gpt_success_nonzero_invoke(base_pa, FIRME_GPI_NONSECURE, out,
                                   GPT_SUCCESS_NONZERO_STEP_CLEANUP) == VAL_FALSE)
        passed = VAL_FALSE;

    if ((passed == VAL_TRUE) && out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate Secure GPI management SUCCESS reports non-zero progress.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
int firme_gpt_success_count_nonzero_run_secure(uint64_t base_pa,
                                               firme_rule_res_t *out)
{
    return gpt_success_nonzero_run_changing(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate Realm GPI management SUCCESS reports non-zero progress.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
int firme_gpt_success_count_nonzero_run_realm(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return gpt_success_nonzero_run_changing(base_pa, FIRME_GPI_REALM, out);
}

/**
 *   @brief    - Validate an NS-to-NS no-op SUCCESS reports non-zero progress.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
int firme_gpt_success_count_nonzero_run_ns(uint64_t base_pa,
                                           firme_rule_res_t *out)
{
    gpt_success_nonzero_init(out);

    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_SUCCESS_NONZERO_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 1: invoke the architecturally permitted NS-to-NS no-op. */
    if (gpt_success_nonzero_invoke(base_pa, FIRME_GPI_NONSECURE, out,
                                   GPT_SUCCESS_NONZERO_STEP_RESULT) == VAL_FALSE)
        return VAL_SUCCESS;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}
