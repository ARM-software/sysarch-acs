/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define BASE_SERVICE_PRESENCE_TEST_NAME "base_service_presence"

#define BASE_SERVICE_PRESENCE_STEP_VERSION  UINT64_C(1)
#define BASE_SERVICE_PRESENCE_STEP_REG0     UINT64_C(2)
#define BASE_SERVICE_PRESENCE_STEP_REG1     UINT64_C(3)

/**
 *   @brief    - Initialize a base service presence test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void base_service_presence_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a base service presence test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - res       : FIRME SMC result.
 *   @return   - void.
 **/
static void base_service_presence_fail(firme_rule_res_t *out, uint64_t fail_step,
                                       firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = res ? res->x1 : 0;
    }
}

/**
 *   @brief    - Validate that the Base service is implemented at this instance.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int base_service_presence_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};

    base_service_presence_init_result(out);

    /* Step 1: Base service version must be reported. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &res);
    if (val_firme_status_to_i64(res.x0) < 0) {
        LOG_ERROR(NULL, "%s: SERVICE_VERSION(Base) failed x0=%llx",
                  BASE_SERVICE_PRESENCE_TEST_NAME, (unsigned long long)res.x0);
        base_service_presence_fail(out, BASE_SERVICE_PRESENCE_STEP_VERSION, &res);
        return VAL_SUCCESS;
    }

    /* Step 2: Base feature register 0 must be reported. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: SERVICE_FEATURES(Base, reg0) failed x0=%llx",
                  BASE_SERVICE_PRESENCE_TEST_NAME, (unsigned long long)res.x0);
        base_service_presence_fail(out, BASE_SERVICE_PRESENCE_STEP_REG0, &res);
        return VAL_SUCCESS;
    }

    /* Step 3: Base feature register 1 must be reported. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG1_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: SERVICE_FEATURES(Base, reg1) failed x0=%llx",
                  BASE_SERVICE_PRESENCE_TEST_NAME, (unsigned long long)res.x0);
        base_service_presence_fail(out, BASE_SERVICE_PRESENCE_STEP_REG1, &res);
        return VAL_SUCCESS;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Base service presence rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_presence_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return base_service_presence_run(out);
}

/**
 *   @brief    - Base service presence rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_presence_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return base_service_presence_run(out);
}

/**
 *   @brief    - Base service presence rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_presence_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return base_service_presence_run(out);
}
