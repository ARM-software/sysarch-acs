/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define SERVICE_VERSIONS_TEST_NAME "service_versions"
#define SERVICE_VERSIONS_EXPECTED_MAJOR 1U

#define SERVICE_VERSIONS_STEP_SERVICE_VERSION UINT64_C(1)
#define SERVICE_VERSIONS_STEP_MAJOR           UINT64_C(2)

/**
 *   @brief    - Initialize a service versions test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void service_versions_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service versions test failure.
 *   @param    - out        : Optional result record.
 *   @param    - fail_step  : Failing test step.
 *   @param    - service_id : FIRME service ID being checked.
 *   @param    - res        : FIRME SMC result.
 *   @return   - void.
 **/
static void service_versions_fail(firme_rule_res_t *out, uint64_t fail_step,
                                  uint64_t service_id, firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = service_id;
    }
}

/**
 *   @brief    - Validate version values reported by implemented FIRME services.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int service_versions_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint32_t service_id = 0U;
    uint32_t major = 0U;
    int64_t status = 0;

    service_versions_init_result(out);

    /* Step 1: Every implemented ALP4 service must report a compatible 1.x version. */
    for (service_id = FIRME_SERVICE_ID_BASE; service_id < FIRME_SERVICE_ID_MAX;
         service_id++) {
        val_firme_service_version((uint64_t)service_id, &res);
        status = val_firme_status_to_i64(res.x0);
        if (status == (int64_t)FIRME_NOT_SUPPORTED)
            continue;
        if (status <= 0) {
            LOG_ERROR(NULL, "%s: service_id=%u expected positive version, got %lld",
                      SERVICE_VERSIONS_TEST_NAME, (unsigned int)service_id,
                      (long long)status);
            service_versions_fail(out, SERVICE_VERSIONS_STEP_SERVICE_VERSION,
                                  (uint64_t)service_id, &res);
            return VAL_ERROR;
        }

        major = FIRME_SERVICE_VERSION_MAJOR(res.x0);
        if (major != SERVICE_VERSIONS_EXPECTED_MAJOR) {
            LOG_ERROR(NULL, "%s: service_id=%u major=%u expected=%u",
                      SERVICE_VERSIONS_TEST_NAME, (unsigned int)service_id,
                      (unsigned int)major, (unsigned int)SERVICE_VERSIONS_EXPECTED_MAJOR);
            service_versions_fail(out, SERVICE_VERSIONS_STEP_MAJOR,
                                  (uint64_t)service_id, &res);
            return VAL_ERROR;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service versions rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_versions_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_versions_run(out);
}

/**
 *   @brief    - Service versions rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_versions_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_versions_run(out);
}

/**
 *   @brief    - Service versions rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_versions_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_versions_run(out);
}
