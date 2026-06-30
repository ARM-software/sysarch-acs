/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define BASE_SERVICE_MANDATORY_TEST_NAME "base_service_mandatory"

#define BASE_SERVICE_MANDATORY_STEP_VERSION  UINT64_C(1)
#define BASE_SERVICE_MANDATORY_STEP_FEATURES UINT64_C(2)

/**
 *   @brief    - Initialize a rule result record for the mandatory ABI test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void base_service_mandatory_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a mandatory ABI test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - res       : FIRME SMC result.
 *   @return   - void
 **/
static void base_service_mandatory_fail(firme_rule_res_t *out, uint64_t fail_step,
                                        firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = res ? res->x1 : 0;
    }
}

/**
 *   @brief    - Validate mandatory Base service ABIs from the current caller world.
 *   @param    - out : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
static int base_service_mandatory_run(firme_rule_res_t *out)
{
    firme_res_t res = (firme_res_t){0};
    int64_t status = 0;

    base_service_mandatory_init_result(out);

    /* Step 1: FIRME_SERVICE_VERSION must be implemented at this instance. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        LOG_ERROR(NULL, "%s: SERVICE_VERSION not supported x0=%llx",
                  BASE_SERVICE_MANDATORY_TEST_NAME, (unsigned long long)res.x0);
        base_service_mandatory_fail(out, BASE_SERVICE_MANDATORY_STEP_VERSION, &res);
        return VAL_SUCCESS;
    }

    if (status <= 0) {
        LOG_ERROR(NULL, "%s: SERVICE_VERSION expected positive version, got %lld x0=%llx",
                  BASE_SERVICE_MANDATORY_TEST_NAME, (long long)status,
                  (unsigned long long)res.x0);
        base_service_mandatory_fail(out, BASE_SERVICE_MANDATORY_STEP_VERSION, &res);
        return VAL_SUCCESS;
    }

    /* Step 2: FIRME_SERVICE_FEATURES must be implemented at this instance. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: SERVICE_FEATURES not implemented x0=%llx",
                  BASE_SERVICE_MANDATORY_TEST_NAME, (unsigned long long)res.x0);
        base_service_mandatory_fail(out, BASE_SERVICE_MANDATORY_STEP_FEATURES, &res);
        return VAL_SUCCESS;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Mandatory Base service rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_base_service_mandatory_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return base_service_mandatory_run(out);
}

/**
 *   @brief    - Mandatory Base service rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_base_service_mandatory_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return base_service_mandatory_run(out);
}

/**
 *   @brief    - Mandatory Base service rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_base_service_mandatory_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return base_service_mandatory_run(out);
}
