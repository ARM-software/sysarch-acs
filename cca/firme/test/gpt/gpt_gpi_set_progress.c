/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_GPI_SET_PROGRESS_TEST_NAME "gpt_gpi_set_progress"
#define GPT_GPI_SET_PROGRESS_STEP_ADVERTISE UINT64_C(1)
#define GPT_GPI_SET_PROGRESS_STEP_FEATURE   UINT64_C(2)
#define GPT_GPI_SET_PROGRESS_STEP_PLATFORM  UINT64_C(3)
#define GPT_GPI_SET_PROGRESS_STEP_SETUP     UINT64_C(10)
#define GPT_GPI_SET_PROGRESS_STEP_SUCCESS   UINT64_C(20)
#define GPT_GPI_SET_PROGRESS_STEP_PREFIX    UINT64_C(30)
#define GPT_GPI_SET_PROGRESS_STEP_BOUNDARY  UINT64_C(40)
#define GPT_GPI_SET_PROGRESS_STEP_DENIED    UINT64_C(50)
#define GPT_GPI_SET_PROGRESS_STEP_CLEANUP   UINT64_C(60)
#define GPT_GPI_SET_PROGRESS_GRANULES       UINT64_C(2)

/**
 *   @brief    - Initialise an R0117 result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void gpt_gpi_set_progress_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0ULL;
        out->last_x0 = 0ULL;
        out->last_x1 = 0ULL;
    }
}

/**
 *   @brief    - Record an ABI-result failure for R0117.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @param    - result    : ABI result to record.
 *   @return   - void.
 **/
static void gpt_gpi_set_progress_record(firme_rule_res_t *out,
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
 *   @brief    - Check caller access to one test granule.
 *   @param    - address       : Granule address to probe.
 *   @param    - expect_access : VAL_TRUE for access, VAL_FALSE for a GPF.
 *   @param    - out           : Optional result record.
 *   @param    - fail_step     : Failure step identifier.
 *   @return   - VAL_TRUE when access matches the expected state.
 **/
static uint32_t gpt_gpi_set_progress_expect_access(uint64_t address,
                                                   uint32_t expect_access,
                                                   firme_rule_res_t *out,
                                                   uint64_t fail_step)
{
    val_fault_info_t fault_info = {0};
    uint64_t value = 0ULL;
    uint32_t matched = VAL_FALSE;

    val_probe_read64(address, &value, &fault_info);
    if (expect_access == VAL_TRUE)
        matched = (uint32_t)(fault_info.faulted == 0ULL);
    else
        matched = (uint32_t)((fault_info.faulted != 0ULL) &&
                             val_fault_is_gpf(fault_info.esr));

    if (matched == VAL_FALSE) {
        LOG_ERROR(NULL,
                  "%s: access mismatch PA=%llx expected=%u faulted=%llu esr=%llx",
                  GPT_GPI_SET_PROGRESS_TEST_NAME,
                  (unsigned long long)address,
                  (unsigned int)expect_access,
                  (unsigned long long)fault_info.faulted,
                  (unsigned long long)fault_info.esr);
        if (out) {
            out->fail_step = fail_step;
            out->last_x0 = fault_info.esr;
            out->last_x1 = address;
        }
    }

    return matched;
}

/**
 *   @brief    - Establish and confirm Non-secure state for one granule.
 *   @param    - address   : Granule address.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @return   - VAL_TRUE when the granule is confirmed Non-secure.
 **/
static uint32_t gpt_gpi_set_progress_prepare_ns(uint64_t address,
                                                firme_rule_res_t *out,
                                                uint64_t fail_step)
{
    firme_res_t result = {0};
    int64_t status = 0;
    const uint64_t granule_count = UINT64_C(1);

    if (val_firme_gm_gpi_set_complete(address, granule_count,
                                      FIRME_GPI_NONSECURE,
                                      &result) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: NS setup did not complete PA=%llx",
                  GPT_GPI_SET_PROGRESS_TEST_NAME,
                  (unsigned long long)address);
        gpt_gpi_set_progress_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    status = val_firme_as_i64(result.x0);
    if ((status == (int64_t)FIRME_SUCCESS) &&
        (result.x1 != granule_count)) {
        LOG_ERROR(NULL, "%s: NS setup returned invalid count x0=%llx x1=%llx",
                  GPT_GPI_SET_PROGRESS_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_gpi_set_progress_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    /*
     * Current TF-A returns DENIED for an already-NS setup request. That known
     * same-GPI issue is tolerated only for setup after confirming the GPI by
     * an access probe; the R0117 requests below remain strict.
     */
    if ((status != (int64_t)FIRME_SUCCESS) &&
        (status != (int64_t)FIRME_DENIED)) {
        LOG_ERROR(NULL, "%s: NS setup returned x0=%llx x1=%llx",
                  GPT_GPI_SET_PROGRESS_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_gpi_set_progress_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    return gpt_gpi_set_progress_expect_access(address, VAL_FALSE, out,
                                              fail_step);
}

/**
 *   @brief    - Run the R0117 GPI_SET progress-table checks from one caller.
 *   @param    - base_pa    : Platform GPT test-window base.
 *   @param    - target_gpi : Caller-world target GPI.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
static int gpt_gpi_set_progress_run(uint64_t base_pa,
                                    uint64_t target_gpi,
                                    firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size = 0ULL;
    uint64_t current_pa = 0ULL;
    uint64_t remaining = GPT_GPI_SET_PROGRESS_GRANULES;
    uint64_t progress = 0ULL;
    uint64_t total = 0ULL;
    uint64_t index = 0ULL;
    uint32_t main_passed = VAL_FALSE;
    uint32_t cleanup_passed = VAL_TRUE;

    gpt_gpi_set_progress_init(out);

    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_GPI_SET_PROGRESS_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    if (val_firme_gpt_feature_reg1(&feature_reg1, out,
                                   GPT_GPI_SET_PROGRESS_STEP_FEATURE) ==
        VAL_FALSE)
        return VAL_SUCCESS;

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) ||
        ((base_pa % granule_size) != 0ULL) ||
        (base_pa > (UINT64_MAX - granule_size))) {
        LOG_ERROR(NULL, "%s: invalid platform base=%llx PGS=%llx",
                  GPT_GPI_SET_PROGRESS_TEST_NAME,
                  (unsigned long long)base_pa,
                  (unsigned long long)granule_size);
        if (out) {
            out->fail_step = GPT_GPI_SET_PROGRESS_STEP_PLATFORM;
            out->last_x0 = base_pa;
            out->last_x1 = granule_size;
        }
        return VAL_SUCCESS;
    }

    /* Step 1: establish two adjacent Non-secure granules. */
    if (gpt_gpi_set_progress_prepare_ns(
            base_pa, out, GPT_GPI_SET_PROGRESS_STEP_SETUP) == VAL_FALSE)
        goto cleanup;
    if (gpt_gpi_set_progress_prepare_ns(
            base_pa + granule_size, out,
            GPT_GPI_SET_PROGRESS_STEP_SETUP + UINT64_C(1)) == VAL_FALSE)
        goto cleanup;

    /*
     * Step 2: validate SUCCESS rows in Table 3.4. A count equal to the
     * request completes the operation; a smaller count is stateless progress
     * and is resumed by a new GPI_SET over the remaining range.
     */
    current_pa = base_pa;
    while (remaining != 0ULL) {
        val_firme_gm_gpi_set_do(current_pa, remaining, target_gpi, &result);
        if ((val_firme_as_i64(result.x0) != (int64_t)FIRME_SUCCESS) ||
            (result.x1 == 0ULL) || (result.x1 > remaining)) {
            LOG_ERROR(NULL,
                      "%s: invalid SUCCESS progress remaining=%llu x0=%llx x1=%llx",
                      GPT_GPI_SET_PROGRESS_TEST_NAME,
                      (unsigned long long)remaining,
                      (unsigned long long)result.x0,
                      (unsigned long long)result.x1);
            gpt_gpi_set_progress_record(
                out, GPT_GPI_SET_PROGRESS_STEP_SUCCESS + total, &result);
            goto cleanup;
        }

        progress = result.x1;
        for (index = 0ULL; index < progress; index++) {
            if (gpt_gpi_set_progress_expect_access(
                    current_pa + (index * granule_size), VAL_TRUE, out,
                    GPT_GPI_SET_PROGRESS_STEP_PREFIX + total + index) ==
                VAL_FALSE)
                goto cleanup;
        }

        if ((progress < remaining) &&
            (gpt_gpi_set_progress_expect_access(
                 current_pa + (progress * granule_size), VAL_FALSE, out,
                 GPT_GPI_SET_PROGRESS_STEP_BOUNDARY + total) == VAL_FALSE))
            goto cleanup;

        current_pa += progress * granule_size;
        remaining -= progress;
        total += progress;
    }

    /*
     * Step 3: validate the reachable DENIED row. Root is not a permitted
     * target from these callers, so no granule may be reported as progressed.
     */
    val_firme_gm_gpi_set_do(base_pa, GPT_GPI_SET_PROGRESS_GRANULES,
                            FIRME_GPI_ROOT, &result);
    if ((val_firme_as_i64(result.x0) != (int64_t)FIRME_DENIED) ||
        (result.x1 >= GPT_GPI_SET_PROGRESS_GRANULES)) {
        LOG_ERROR(NULL, "%s: invalid DENIED progress x0=%llx x1=%llx",
                  GPT_GPI_SET_PROGRESS_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_gpi_set_progress_record(out, GPT_GPI_SET_PROGRESS_STEP_DENIED,
                                    &result);
        goto cleanup;
    }

    for (index = 0ULL; index < GPT_GPI_SET_PROGRESS_GRANULES; index++) {
        if (gpt_gpi_set_progress_expect_access(
                base_pa + (index * granule_size), VAL_TRUE, out,
                GPT_GPI_SET_PROGRESS_STEP_DENIED + index) == VAL_FALSE)
            goto cleanup;
    }

    main_passed = VAL_TRUE;

cleanup:
    /* Step 4: restore both test granules to Non-secure state. */
    if (gpt_gpi_set_progress_prepare_ns(
            base_pa, out, GPT_GPI_SET_PROGRESS_STEP_CLEANUP) == VAL_FALSE)
        cleanup_passed = VAL_FALSE;
    if (gpt_gpi_set_progress_prepare_ns(
            base_pa + granule_size, out,
            GPT_GPI_SET_PROGRESS_STEP_CLEANUP + UINT64_C(1)) == VAL_FALSE)
        cleanup_passed = VAL_FALSE;

    if ((main_passed == VAL_TRUE) && (cleanup_passed == VAL_TRUE) && out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate R0117 from the Secure FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
int firme_gpt_gpi_set_progress_run_secure(uint64_t base_pa,
                                          firme_rule_res_t *out)
{
    return gpt_gpi_set_progress_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate R0117 from the Realm FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
int firme_gpt_gpi_set_progress_run_realm(uint64_t base_pa,
                                         firme_rule_res_t *out)
{
    return gpt_gpi_set_progress_run(base_pa, FIRME_GPI_REALM, out);
}
