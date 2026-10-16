/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_platform.h"
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
        out->fail_step = 0ULL;
        out->last_x0 = 0ULL;
        out->last_x1 = 0ULL;
    }
}

/**
 *   @brief    - Record an ABI-result failure for R0120.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @param    - result    : ABI result to record.
 *   @return   - void.
 **/
static void gpt_success_nonzero_record(firme_rule_res_t *out,
                                       uint64_t fail_step,
                                       const firme_res_t *result)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = result ? result->x0 : 0ULL;
        out->last_x1 = result ? result->x1 : 0ULL;
    }
}

/**
 *   @brief    - Confirm that the caller cannot access an NS test granule.
 *   @param    - base_pa   : Test-granule address.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @return   - VAL_TRUE when a GPF is observed, otherwise VAL_FALSE.
 **/
static uint32_t gpt_success_nonzero_expect_ns(uint64_t base_pa,
                                              firme_rule_res_t *out,
                                              uint64_t fail_step)
{
    val_fault_info_t fault_info = {0};
    uint64_t value = 0ULL;

    val_probe_read64(base_pa, &value, &fault_info);
    if ((fault_info.faulted == 0ULL) ||
        !val_fault_is_gpf(fault_info.esr)) {
        LOG_ERROR(NULL,
                  "%s: expected NS granule GPF faulted=%llu esr=%llx",
                  GPT_SUCCESS_NONZERO_TEST_NAME,
                  (unsigned long long)fault_info.faulted,
                  (unsigned long long)fault_info.esr);
        if (out) {
            out->fail_step = fail_step;
            out->last_x0 = fault_info.esr;
            out->last_x1 = base_pa;
        }
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Establish NS state while validating any SUCCESS progress.
 *   @param    - base_pa   : Test-granule address.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @return   - VAL_TRUE when the granule is confirmed NS.
 **/
static uint32_t gpt_success_nonzero_prepare_ns(uint64_t base_pa,
                                               firme_rule_res_t *out,
                                               uint64_t fail_step)
{
    firme_res_t result = {0};
    int64_t status = 0;
    const uint64_t granule_count = UINT64_C(1);

    if (val_firme_gpt_transition_terminal(base_pa, FIRME_GPI_NONSECURE,
            &result, out, fail_step) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: NS setup did not complete",
                  GPT_SUCCESS_NONZERO_TEST_NAME);
        gpt_success_nonzero_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    status = val_firme_as_i64(result.x0);
    if ((status == (int64_t)FIRME_SUCCESS) && (result.x1 != granule_count)) {
        LOG_ERROR(NULL, "%s: NS setup SUCCESS reported zero progress",
                  GPT_SUCCESS_NONZERO_TEST_NAME);
        gpt_success_nonzero_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    if (status != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: NS setup returned x0=%llx x1=%llx",
                  GPT_SUCCESS_NONZERO_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_success_nonzero_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    return gpt_success_nonzero_expect_ns(base_pa, out, fail_step);
}

/**
 *   @brief    - Invoke one changing GPI_SET and require non-zero SUCCESS.
 *   @param    - base_pa    : Test-granule address.
 *   @param    - target_gpi : Caller-world target GPI.
 *   @param    - out        : Optional result record.
 *   @param    - fail_step  : Failure step identifier.
 *   @return   - VAL_TRUE for a non-zero SUCCESS result.
 **/
static uint32_t gpt_success_nonzero_invoke(uint64_t base_pa,
                                           uint64_t target_gpi,
                                           firme_rule_res_t *out,
                                           uint64_t fail_step)
{
    firme_res_t result = {0};
    if (val_firme_gpt_transition_terminal(base_pa, target_gpi, &result,
                                         out, fail_step) != VAL_SUCCESS)
        return VAL_FALSE;
    if ((val_firme_as_i64(result.x0) != (int64_t)FIRME_SUCCESS) ||
        (result.x1 == 0ULL)) {
        LOG_ERROR(NULL, "%s: expected SUCCESS with non-zero count x0=%llx x1=%llx",
                  GPT_SUCCESS_NONZERO_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_success_nonzero_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Run the R0120 GPI_SET check from Secure or Realm.
 *   @param    - base_pa    : Platform GPT test-window base.
 *   @param    - target_gpi : Caller-world target GPI.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
static int gpt_success_nonzero_run(uint64_t base_pa,
                                   uint64_t target_gpi,
                                   firme_rule_res_t *out)
{
    uint32_t main_passed = VAL_FALSE;
    uint32_t cleanup_passed = VAL_TRUE;
    uint64_t changed_count = 0ULL;

    if (!out)
        return VAL_ERROR;

    gpt_success_nonzero_init(out);

    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_SUCCESS_NONZERO_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: establish Non-secure as the current GPI. */
    if (gpt_success_nonzero_prepare_ns(
            base_pa, out, GPT_SUCCESS_NONZERO_STEP_SETUP) == VAL_FALSE)
        goto cleanup;

    /* Step 2: inspect the raw successful changing-transition response. */
    if (gpt_success_nonzero_invoke(
            base_pa, target_gpi, out,
            GPT_SUCCESS_NONZERO_STEP_RESULT) == VAL_FALSE)
        goto cleanup;

    main_passed = VAL_TRUE;
    changed_count = UINT64_C(1);

cleanup:
    /* Step 3: Restore changed storage; Host independently confirms the final NS state. */
    cleanup_passed = val_firme_gpt_restore_prefix_ns(base_pa, changed_count,
        PLAT_FIRME_GPT_GRANULE_SIZE, out, GPT_SUCCESS_NONZERO_STEP_CLEANUP);

    if ((main_passed == VAL_TRUE) && (cleanup_passed == VAL_TRUE) && out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}

/**
 *   @brief    - Validate R0120 from the Secure FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_success_count_nonzero_run_secure(uint64_t base_pa,
                                               firme_rule_res_t *out)
{
    return gpt_success_nonzero_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate R0120 from the Realm FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_success_count_nonzero_run_realm(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return gpt_success_nonzero_run(base_pa, FIRME_GPI_REALM, out);
}
