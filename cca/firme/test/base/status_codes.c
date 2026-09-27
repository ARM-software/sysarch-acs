/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_firme_smccc_probe.h"
#include "val_log.h"

#define STATUS_CODES_TEST_NAME "status_codes"
#define STATUS_CODES_STEP_FEATURE_SUCCESS     UINT64_C(2)
#define STATUS_CODES_STEP_VERSION_STATUS      UINT64_C(3)
#define STATUS_CODES_STEP_FEATURES_STATUS     UINT64_C(4)
#define STATUS_CODES_STEP_UNALLOCATED_VERSION UINT64_C(5)
#define STATUS_CODES_STEP_UNALLOCATED_FEATURE UINT64_C(6)
#define STATUS_CODES_STEP_ADVERTISED_ABI      UINT64_C(7)

/**
 *   @brief    - Initialize a FIRME status-code result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void status_codes_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a FIRME status-code failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void status_codes_fail(firme_rule_res_t *out, uint64_t fail_step,
                              uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate an observed negative FIRME return against Table 2.1.
 *   @param    - status : Signed FIRME return value.
 *   @return   - VAL_TRUE when the value is architected or non-negative.
 **/
static uint32_t status_codes_is_valid_observed_status(int64_t status)
{
    if (status >= 0)
        return VAL_TRUE;

    return (uint32_t)val_firme_status_is_architected(status);
}

/**
 *   @brief    - Validate FIRME status-code definitions and observed ABI status values.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int status_codes_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    int64_t status = 0;
    uint32_t service_id = 0U;

    status_codes_init_result(out);

    /* Step 1: A valid Base feature request must return SUCCESS. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg0 returned status=%lld",
                  STATUS_CODES_TEST_NAME, (long long)status);
        status_codes_fail(out, STATUS_CODES_STEP_FEATURE_SUCCESS, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: SERVICE_VERSION negative returns must be architected. */
    for (service_id = FIRME_SERVICE_ID_BASE; service_id <= FIRME_SERVICE_ID_8BIT_MAX;
         service_id++) {
        val_firme_service_version((uint64_t)service_id, &res);
        status = val_firme_status_to_i64(res.x0);
        if (status_codes_is_valid_observed_status(status) != VAL_TRUE) {
            LOG_ERROR(NULL, "%s: service_id=%u version unarchitected status=%lld",
                      STATUS_CODES_TEST_NAME, (unsigned int)service_id,
                      (long long)status);
            status_codes_fail(out, STATUS_CODES_STEP_VERSION_STATUS, res.x0, service_id);
            return VAL_ERROR;
        }
        if ((service_id >= FIRME_SERVICE_ID_MAX) &&
            (status != (int64_t)FIRME_NOT_SUPPORTED)) {
            LOG_ERROR(NULL, "%s: service_id=%u version expected NOT_SUPPORTED x0=%llx",
                      STATUS_CODES_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            status_codes_fail(out, STATUS_CODES_STEP_UNALLOCATED_VERSION, res.x0, service_id);
            return VAL_ERROR;
        }
    }

    /* Step 4: SERVICE_FEATURES negative returns must be architected. */
    for (service_id = FIRME_SERVICE_ID_BASE; service_id <= FIRME_SERVICE_ID_8BIT_MAX;
         service_id++) {
        val_firme_service_features((uint64_t)service_id, FIRME_BASE_FEATURE_REG0_INDEX,
                                   &res);
        status = val_firme_status_to_i64(res.x0);
        if (val_firme_status_is_architected(status) != VAL_TRUE) {
            LOG_ERROR(NULL, "%s: service_id=%u features unarchitected status=%lld",
                      STATUS_CODES_TEST_NAME, (unsigned int)service_id,
                      (long long)status);
            status_codes_fail(out, STATUS_CODES_STEP_FEATURES_STATUS, res.x0, service_id);
            return VAL_ERROR;
        }
        if ((service_id >= FIRME_SERVICE_ID_MAX) &&
            (status != (int64_t)FIRME_NOT_SUPPORTED)) {
            LOG_ERROR(NULL, "%s: service_id=%u features expected NOT_SUPPORTED x0=%llx",
                      STATUS_CODES_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            status_codes_fail(out, STATUS_CODES_STEP_UNALLOCATED_FEATURE, res.x0, service_id);
            return VAL_ERROR;
        }
    }

    /* Step 5: Every advertised ABI must return only an architected status. */
    if (val_firme_smccc_check_state(0U, out) != VAL_SUCCESS) {
        if (out && (out->fail_step == 0U))
            out->fail_step = STATUS_CODES_STEP_ADVERTISED_ABI;
        return VAL_SUCCESS;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - FIRME status-code rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_status_codes_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return status_codes_run(out);
}

/**
 *   @brief    - FIRME status-code rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_status_codes_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return status_codes_run(out);
}

/**
 *   @brief    - FIRME status-code rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_status_codes_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return status_codes_run(out);
}
