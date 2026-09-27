/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define FEATURE_REGISTER_INDEX_TEST_NAME "feature_register_index"
#define FEATURE_REGISTER_INDEX_MAX       UINT64_C(0xff)

#define FEATURE_REGISTER_INDEX_STEP_REG0_STATUS UINT64_C(1)
#define FEATURE_REGISTER_INDEX_STEP_REG1_STATUS UINT64_C(2)
#define FEATURE_REGISTER_INDEX_STEP_MAX_STATUS  UINT64_C(3)

/**
 *   @brief    - Initialize a feature-register index result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void feature_register_index_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a feature-register index failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void feature_register_index_fail(firme_rule_res_t *out, uint64_t fail_step,
                                        uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate feature register index boundary behavior.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int feature_register_index_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};

    feature_register_index_init_result(out);

    /* Step 1: Base feature register index 0 is allocated and must be readable. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg0 expected SUCCESS x0=%llx",
                  FEATURE_REGISTER_INDEX_TEST_NAME, (unsigned long long)res.x0);
        feature_register_index_fail(out, FEATURE_REGISTER_INDEX_STEP_REG0_STATUS,
                                    res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Base feature register index 1 is allocated and must be readable. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG1_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg1 expected SUCCESS x0=%llx",
                  FEATURE_REGISTER_INDEX_TEST_NAME, (unsigned long long)res.x0);
        feature_register_index_fail(out, FEATURE_REGISTER_INDEX_STEP_REG1_STATUS,
                                    res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: The maximum 8-bit index is validly encoded but unallocated in Base. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FEATURE_REGISTER_INDEX_MAX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
        LOG_ERROR(NULL, "%s: base feature reg0xff expected NOT_SUPPORTED x0=%llx",
                  FEATURE_REGISTER_INDEX_TEST_NAME, (unsigned long long)res.x0);
        feature_register_index_fail(out, FEATURE_REGISTER_INDEX_STEP_MAX_STATUS,
                                    res.x0, res.x1);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Feature register index rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_index_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_index_run(out);
}

/**
 *   @brief    - Feature register index rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_index_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_index_run(out);
}

/**
 *   @brief    - Feature register index rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_index_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_index_run(out);
}
