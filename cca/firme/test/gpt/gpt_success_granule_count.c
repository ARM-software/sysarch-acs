/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_SUCCESS_COUNT_TEST_NAME "gpt_success_granule_count"
#define GPT_SUCCESS_COUNT_STEP_ADVERTISE UINT64_C(1)
#define GPT_SUCCESS_COUNT_STEP_FEATURE   UINT64_C(2)
#define GPT_SUCCESS_COUNT_STEP_SETUP     UINT64_C(10)
#define GPT_SUCCESS_COUNT_STEP_RESULT    UINT64_C(20)
#define GPT_SUCCESS_COUNT_STEP_PREFIX    UINT64_C(30)
#define GPT_SUCCESS_COUNT_STEP_BOUNDARY  UINT64_C(40)
#define GPT_SUCCESS_COUNT_STEP_CLEANUP   UINT64_C(50)
#define GPT_SUCCESS_COUNT_GRANULES       UINT64_C(2)
#define GPT_SUCCESS_COUNT_MAX_CALLS       VAL_FIRME_GPT_TRACE_MAX_PHASES

/**
 *   @brief    - Initialise an R0113 result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void gpt_success_count_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0ULL;
        out->last_x0 = 0ULL;
        out->last_x1 = 0ULL;
    }
}

/**
 *   @brief    - Record an ABI-result failure for R0113.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @param    - result    : ABI result to record.
 *   @return   - void.
 **/
static void gpt_success_count_record(firme_rule_res_t *out,
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
 *   @brief    - Check whether the current caller can access one granule.
 *   @param    - address       : Granule address to probe.
 *   @param    - expect_access : VAL_TRUE for access, VAL_FALSE for a GPF.
 *   @param    - out           : Optional result record.
 *   @param    - fail_step     : Failure step identifier.
 *   @return   - VAL_TRUE when access matches the expected state.
 **/
static uint32_t gpt_success_count_expect_access(uint64_t address,
                                                uint32_t expect_access,
                                                firme_rule_res_t *out,
                                                uint64_t fail_step)
{
    val_fault_info_t fault_info = {0};
    uint64_t value = 0ULL;
    uint32_t matched = VAL_FALSE;
    int probe_result;

    probe_result = val_probe_read64(address, &value, &fault_info);
    if (expect_access == VAL_TRUE)
        matched = (uint32_t)(probe_result == VAL_SUCCESS && fault_info.faulted == 0ULL);
    else
        matched = (uint32_t)((probe_result == VAL_ERROR) && (fault_info.faulted != 0ULL) &&
                             val_fault_is_gpf(fault_info.esr));

    if (matched == VAL_FALSE) {
        LOG_ERROR(NULL,
                  "%s: access mismatch PA=%llx expected=%u faulted=%llu esr=%llx",
                  GPT_SUCCESS_COUNT_TEST_NAME,
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
 *   @brief    - Put one granule in NS state for setup or cleanup.
 *   @param    - address   : Granule address.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @return   - VAL_TRUE when the granule is confirmed inaccessible to caller.
 **/
static uint32_t gpt_success_count_prepare_ns(uint64_t address,
                                             firme_rule_res_t *out,
                                             uint64_t fail_step)
{
    firme_res_t result = {0};
    int64_t status = 0;
    const uint64_t granule_count = UINT64_C(1);

    if (val_firme_gpt_transition_terminal(address, FIRME_GPI_NONSECURE,
            &result, out, fail_step) != VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: NS setup did not complete PA=%llx",
                  GPT_SUCCESS_COUNT_TEST_NAME,
                  (unsigned long long)address);
        gpt_success_count_record(out, fail_step, &result);
        out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_FALSE;
    }

    status = val_firme_as_i64(result.x0);
    if ((status == (int64_t)FIRME_SUCCESS) &&
        (result.x1 != granule_count)) {
        LOG_ERROR(NULL, "%s: NS setup returned invalid count x0=%llx x1=%llx",
                  GPT_SUCCESS_COUNT_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_success_count_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    if (status != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: NS setup returned unexpected status x0=%llx x1=%llx",
                  GPT_SUCCESS_COUNT_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_success_count_record(out, fail_step, &result);
        return VAL_FALSE;
    }

    return gpt_success_count_expect_access(address, VAL_FALSE, out, fail_step);
}

/**
 *   @brief    - Run the R0113 successful-prefix check from one caller world.
 *   @param    - base_pa    : Platform GPT test-window base.
 *   @param    - target_gpi : Caller-world target GPI.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
static int gpt_success_count_run(uint64_t base_pa,
                                 uint64_t target_gpi,
                                 firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size = 0ULL;
    uint64_t current_pa = 0ULL;
    uint64_t remaining = GPT_SUCCESS_COUNT_GRANULES;
    uint64_t total = 0ULL;
    uint64_t progress = 0ULL;
    uint64_t index = 0ULL;
    uint64_t cookie = 0ULL;
    int64_t status = 0;
    uint32_t main_passed = VAL_FALSE;
    uint32_t cleanup_passed = VAL_TRUE;
    uint32_t prefix_passed = VAL_TRUE;
    uint32_t pending = VAL_FALSE;
    uint32_t use_continue = VAL_FALSE;
    uint32_t safe_to_restore = VAL_TRUE;
    uint32_t attempt = 0U;

    if (!out)
        return VAL_ERROR;

    gpt_success_count_init(out);

    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_SUCCESS_COUNT_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: derive PGS and validate the two-granule platform window. */
    if (val_firme_gpt_feature_reg1(&feature_reg1, out,
                                   GPT_SUCCESS_COUNT_STEP_FEATURE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) ||
        ((base_pa % granule_size) != 0ULL) ||
        (GPT_SUCCESS_COUNT_GRANULES > (UINT64_MAX - base_pa) / granule_size)) {
        LOG_ERROR(NULL, "%s: invalid test window base=%llx PGS=%llx",
                  GPT_SUCCESS_COUNT_TEST_NAME,
                  (unsigned long long)base_pa,
                  (unsigned long long)granule_size);
        if (out) {
            out->fail_step = GPT_SUCCESS_COUNT_STEP_FEATURE;
            out->last_x0 = base_pa;
            out->last_x1 = granule_size;
        }
        return val_firme_gpt_rule_result_status(out);
    }

    /* Step 2: establish both adjacent granules as Non-secure. */
    if (gpt_success_count_prepare_ns(base_pa, out,
                                     GPT_SUCCESS_COUNT_STEP_SETUP) == VAL_FALSE)
        goto cleanup;
    if (gpt_success_count_prepare_ns(base_pa + granule_size, out,
                                     GPT_SUCCESS_COUNT_STEP_SETUP + UINT64_C(1)) ==
        VAL_FALSE)
        goto cleanup;

    /* Step 3: validate each reported prefix while completing retained work. */
    current_pa = base_pa;
    for (attempt = 0U; remaining != 0ULL && attempt < GPT_SUCCESS_COUNT_MAX_CALLS;
         attempt++) {
        if (use_continue)
            val_firme_gm_gpi_op_continue(cookie, &result);
        else
            val_firme_gm_gpi_set_do(current_pa, remaining, target_gpi, &result);

        status = val_firme_as_i64(result.x0);
        if (status == (int64_t)FIRME_BUSY) {
            if (use_continue)
                cookie = result.x2;
            continue;
        }

        if (!use_continue && (status == FIRME_INVALID_PARAMETERS ||
                              status == FIRME_NOT_SUPPORTED)) {
            gpt_success_count_record(out, GPT_SUCCESS_COUNT_STEP_RESULT, &result);
            goto cleanup;
        }

        if (val_firme_gpt_operation_count_valid(status, result.x1,
                                                 remaining) != VAL_TRUE) {
            LOG_ERROR(NULL,
                      "%s: invalid progress remaining=%llu x0=%llx x1=%llx\n",
                      GPT_SUCCESS_COUNT_TEST_NAME,
                      (unsigned long long)remaining,
                      (unsigned long long)result.x0,
                      (unsigned long long)result.x1);
            gpt_success_count_record(out, GPT_SUCCESS_COUNT_STEP_RESULT,
                                     &result);
            safe_to_restore = VAL_FALSE;
            goto cleanup;
        }

        pending = status == (int64_t)FIRME_INCOMPLETE ? VAL_TRUE : VAL_FALSE;
        use_continue = pending;
        if (pending)
            cookie = result.x2;
        progress = result.x1;
        if (status != (int64_t)FIRME_SUCCESS &&
            status != (int64_t)FIRME_INCOMPLETE) {
            LOG_ERROR(NULL, "%s: transition stopped x0=%llx x1=%llx\n",
                      GPT_SUCCESS_COUNT_TEST_NAME,
                      (unsigned long long)result.x0,
                      (unsigned long long)result.x1);
            gpt_success_count_record(out, GPT_SUCCESS_COUNT_STEP_RESULT,
                                     &result);
            total += progress;
            goto cleanup;
        }

        if (prefix_passed == VAL_TRUE) {
            for (index = 0ULL; index < progress; index++) {
                if (gpt_success_count_expect_access(
                        current_pa + (index * granule_size), VAL_TRUE, out,
                        GPT_SUCCESS_COUNT_STEP_PREFIX + total + index) == VAL_FALSE) {
                    prefix_passed = VAL_FALSE;
                    break;
                }
            }

            if (prefix_passed == VAL_TRUE && progress < remaining &&
                gpt_success_count_expect_access(
                    current_pa + (progress * granule_size), VAL_FALSE, out,
                    GPT_SUCCESS_COUNT_STEP_BOUNDARY + total) == VAL_FALSE)
                prefix_passed = VAL_FALSE;
        }

        current_pa += progress * granule_size;
        remaining -= progress;
        total += progress;
    }

    if (remaining != 0ULL) {
        LOG_ERROR(NULL, "%s: operation did not complete after %u calls\n",
                  GPT_SUCCESS_COUNT_TEST_NAME, GPT_SUCCESS_COUNT_MAX_CALLS);
        gpt_success_count_record(out, GPT_SUCCESS_COUNT_STEP_RESULT, &result);
        if (pending)
            safe_to_restore = VAL_FALSE;
        goto cleanup;
    }

    if (total != GPT_SUCCESS_COUNT_GRANULES) {
        LOG_ERROR(NULL, "%s: transitioned total=%llu expected=%llu",
                  GPT_SUCCESS_COUNT_TEST_NAME,
                  (unsigned long long)total,
                  (unsigned long long)GPT_SUCCESS_COUNT_GRANULES);
        if (out) {
            out->fail_step = GPT_SUCCESS_COUNT_STEP_RESULT;
            out->last_x0 = total;
            out->last_x1 = GPT_SUCCESS_COUNT_GRANULES;
        }
        goto cleanup;
    }

    main_passed = prefix_passed;

cleanup:
    if (safe_to_restore != VAL_TRUE || out->status == (uint64_t)ACS_STATUS_ERR) {
        LOG_ERROR(NULL, "%s: retained or unknown operation prevents safe cleanup\n",
                  GPT_SUCCESS_COUNT_TEST_NAME);
        out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    /* Step 4: Restore the reported changed prefix; Host verifies the entire range. */
    cleanup_passed = val_firme_gpt_restore_prefix_ns(base_pa, total,
        granule_size, out, GPT_SUCCESS_COUNT_STEP_CLEANUP);

    if ((main_passed == VAL_TRUE) && (cleanup_passed == VAL_TRUE) && out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}

/**
 *   @brief    - Validate R0113 from the Secure FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_success_granule_count_run_secure(uint64_t base_pa,
                                               firme_rule_res_t *out)
{
    return gpt_success_count_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate R0113 from the Realm FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_success_granule_count_run_realm(uint64_t base_pa,
                                              firme_rule_res_t *out)
{
    return gpt_success_count_run(base_pa, FIRME_GPI_REALM, out);
}
