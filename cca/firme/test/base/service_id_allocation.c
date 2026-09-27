/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define SERVICE_ID_ALLOCATION_TEST_NAME "service_id_allocation"
#define SERVICE_ID_ALLOCATION_LAST_ALLOCATED \
    ((uint64_t)FIRME_SERVICE_ID_INTEGRATED_DEVICE)
#define SERVICE_ID_ALLOCATION_FIRST_UNALLOCATED ((uint64_t)FIRME_SERVICE_ID_MAX)
#define SERVICE_ID_ALLOCATION_LAST_SERVICE_ID    UINT64_C(0xff)

#define SERVICE_ID_ALLOCATION_STEP_BASE_ALLOCATED        UINT64_C(1)
#define SERVICE_ID_ALLOCATION_STEP_ALLOCATED_DISCOVERY   UINT64_C(2)
#define SERVICE_ID_ALLOCATION_STEP_UNALLOCATED_VERSION   UINT64_C(3)
#define SERVICE_ID_ALLOCATION_STEP_UNALLOCATED_FEATURES  UINT64_C(4)

/**
 *   @brief    - Initialize a service ID allocation test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void service_id_allocation_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service ID allocation test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - service_id: FIRME service ID being checked.
 *   @param    - res       : FIRME SMC result.
 *   @return   - void.
 **/
static void service_id_allocation_fail(firme_rule_res_t *out, uint64_t fail_step,
                                       uint64_t service_id, firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = service_id;
    }
}

/**
 *   @brief    - Validate architected service ID allocation boundaries.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int service_id_allocation_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t service_id = 0;
    int64_t version_status = 0;
    int64_t feature_status = 0;

    service_id_allocation_init_result(out);

    /* Step 1: The Base service is allocated at service ID 0 and must respond. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &res);
    if (val_firme_status_to_i64(res.x0) < 0) {
        LOG_ERROR(NULL, "%s: base service ID not allocated x0=%llx",
                  SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)res.x0);
        service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_BASE_ALLOCATED,
                                   FIRME_SERVICE_ID_BASE, &res);
        return VAL_ERROR;
    }

    /* Step 2: Check discovery at every allocated service ID, not just Base. */
    for (service_id = FIRME_SERVICE_ID_GRANULE_MGMT;
         service_id <= SERVICE_ID_ALLOCATION_LAST_ALLOCATED; service_id++) {
        val_firme_service_version(service_id, &res);
        version_status = val_firme_status_to_i64(res.x0);
        if ((version_status < 0) && (version_status != FIRME_NOT_SUPPORTED)) {
            LOG_ERROR(NULL, "%s: service_id=%llu unexpected version status=%lld",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (long long)version_status);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_ALLOCATED_DISCOVERY,
                                       service_id, &res);
            return VAL_ERROR;
        }

        val_firme_service_features(service_id, FIRME_BASE_FEATURE_REG0_INDEX, &res);
        feature_status = val_firme_status_to_i64(res.x0);
        if (((version_status == FIRME_NOT_SUPPORTED) &&
             (feature_status != FIRME_NOT_SUPPORTED)) ||
            ((version_status >= 0) && (feature_status != FIRME_SUCCESS))) {
            LOG_ERROR(NULL, "%s: service_id=%llu version=%lld feature status=%lld",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (long long)version_status, (long long)feature_status);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_ALLOCATED_DISCOVERY,
                                       service_id, &res);
            return VAL_ERROR;
        }
    }

    /* Step 3: Every unallocated service ID must not report a service version. */
    for (service_id = SERVICE_ID_ALLOCATION_FIRST_UNALLOCATED;
         service_id <= SERVICE_ID_ALLOCATION_LAST_SERVICE_ID; service_id++) {
        val_firme_service_version(service_id, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: service_id=%llu version expected NOT_SUPPORTED x0=%llx",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (unsigned long long)res.x0);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_UNALLOCATED_VERSION,
                                       service_id, &res);
            return VAL_ERROR;
        }

        /* Step 4: Unallocated service IDs must not report feature registers. */
        val_firme_service_features(service_id, FIRME_BASE_FEATURE_REG0_INDEX, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: service_id=%llu features expected NOT_SUPPORTED x0=%llx",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (unsigned long long)res.x0);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_UNALLOCATED_FEATURES,
                                       service_id, &res);
            return VAL_ERROR;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service ID allocation rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_id_allocation_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_id_allocation_run(out);
}

/**
 *   @brief    - Service ID allocation rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_id_allocation_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_id_allocation_run(out);
}

/**
 *   @brief    - Service ID allocation rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_id_allocation_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_id_allocation_run(out);
}
