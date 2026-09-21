/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define SERVICE_BASE_DEPENDENCY_TEST_NAME "service_base_dependency"
#define SERVICE_BASE_DEPENDENCY_MAJOR 1U

#define SERVICE_BASE_DEPENDENCY_STEP_BASE_VERSION UINT64_C(1)
#define SERVICE_BASE_DEPENDENCY_STEP_BASE_MAJOR   UINT64_C(2)
#define SERVICE_BASE_DEPENDENCY_STEP_SERVICE_STATUS UINT64_C(4)

/**
 *   @brief    - Initialize a service base dependency test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void service_base_dependency_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service base dependency test failure.
 *   @param    - out        : Optional result record.
 *   @param    - fail_step  : Failing test step.
 *   @param    - service_id : FIRME service ID being checked.
 *   @param    - res        : FIRME SMC result.
 *   @return   - void.
 **/
static void service_base_dependency_fail(firme_rule_res_t *out, uint64_t fail_step,
                                         uint64_t service_id, firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = service_id;
    }
}

/**
 *   @brief    - Validate Base service version dependency for implemented services.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int service_base_dependency_run(firme_rule_res_t *out)
{
    firme_res_t base_res = {0};
    firme_res_t service_res = {0};
    uint32_t service_id = 0U;
    uint32_t base_major = 0U;
    int64_t status = 0;

    service_base_dependency_init_result(out);

    /* Step 1: Read the Base service version used as the dependency anchor. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &base_res);
    if (val_firme_status_to_i64(base_res.x0) < 0) {
        LOG_ERROR(NULL, "%s: base version missing x0=%llx",
                  SERVICE_BASE_DEPENDENCY_TEST_NAME, (unsigned long long)base_res.x0);
        service_base_dependency_fail(out, SERVICE_BASE_DEPENDENCY_STEP_BASE_VERSION,
                                     FIRME_SERVICE_ID_BASE, &base_res);
        return VAL_SUCCESS;
    }

    base_major = FIRME_SERVICE_VERSION_MAJOR(base_res.x0);

    /* Step 2: Base major version dependency for ALP4 services is 1. */
    if (base_major != SERVICE_BASE_DEPENDENCY_MAJOR) {
        LOG_ERROR(NULL, "%s: base major=%u expected=%u",
                  SERVICE_BASE_DEPENDENCY_TEST_NAME, (unsigned int)base_major,
                  (unsigned int)SERVICE_BASE_DEPENDENCY_MAJOR);
        service_base_dependency_fail(out, SERVICE_BASE_DEPENDENCY_STEP_BASE_MAJOR,
                                     FIRME_SERVICE_ID_BASE, &base_res);
        return VAL_SUCCESS;
    }

    /* Step 3: Any 1.x minor revision satisfies the Base 1.0 dependency. */
    /* Step 4: Implemented non-Base services must report clean version status. */
    for (service_id = FIRME_SERVICE_ID_GRANULE_MGMT; service_id < FIRME_SERVICE_ID_MAX;
         service_id++) {
        val_firme_service_version((uint64_t)service_id, &service_res);
        status = val_firme_status_to_i64(service_res.x0);
        if (status == (int64_t)FIRME_NOT_SUPPORTED)
            continue;
        if (status <= 0) {
            LOG_ERROR(NULL, "%s: service_id=%u expected positive version, got %lld",
                      SERVICE_BASE_DEPENDENCY_TEST_NAME, (unsigned int)service_id,
                      (long long)status);
            service_base_dependency_fail(out, SERVICE_BASE_DEPENDENCY_STEP_SERVICE_STATUS,
                                         (uint64_t)service_id, &service_res);
            return VAL_SUCCESS;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service base dependency rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_base_dependency_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_base_dependency_run(out);
}

/**
 *   @brief    - Service base dependency rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_base_dependency_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_base_dependency_run(out);
}

/**
 *   @brief    - Service base dependency rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_base_dependency_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_base_dependency_run(out);
}
