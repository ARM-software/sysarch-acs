/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_service.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define SERVICE_VERSION_TEST_NAME "service_version"
#define SERVICE_VERSION_EXPECTED_MAJOR 1U

#define SERVICE_VERSION_STEP_BASE_VERSION     UINT64_C(1)
#define SERVICE_VERSION_STEP_BASE_MAJOR       UINT64_C(2)
#define SERVICE_VERSION_STEP_SERVICE_LIST     UINT64_C(3)
#define SERVICE_VERSION_STEP_ABSENT_SERVICE   UINT64_C(4)
#define SERVICE_VERSION_STEP_PRESENT_SERVICE  UINT64_C(5)
#define SERVICE_VERSION_STEP_INVALID_ID_RANGE UINT64_C(6)

/**
 *   @brief    - Initialize a rule result record for the service version test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void service_version_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service version test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - res       : FIRME SMC result.
 *   @return   - void
 **/
static void service_version_fail(firme_rule_res_t *out, uint64_t fail_step, firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = res ? res->x1 : 0;
    }
}

/**
 *   @brief    - Validate FIRME_SERVICE_VERSION from the current caller world.
 *   @param    - out : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
static int service_version_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = (firme_res_t){0};
    firme_res_t feature_res = {0};
    uint64_t service_list = 0ULL;
    uint32_t major = 0;
    uint32_t service_id = 0U;
    uint32_t expected_present = VAL_FALSE;
    uint32_t allowed = VAL_FALSE;
    int64_t status = 0;

    service_version_init_result(out);

    /* Step 1: Base service version is mandatory at every FIRME instance. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status < 0) {
        LOG_ERROR(NULL, "%s: base service version returned status=%lld",
                  SERVICE_VERSION_TEST_NAME, (long long)status);
        service_version_fail(out, SERVICE_VERSION_STEP_BASE_VERSION, &res);
        return VAL_SUCCESS;
    }

    /* Step 2: Base service major version must be compatible with ALP4 v1.x. */
    major = FIRME_SERVICE_VERSION_MAJOR(res.x0);
    if (major != SERVICE_VERSION_EXPECTED_MAJOR) {
        LOG_ERROR(NULL, "%s: base service major=%u expected=%u",
                  SERVICE_VERSION_TEST_NAME, (unsigned int)major,
                  (unsigned int)SERVICE_VERSION_EXPECTED_MAJOR);
        service_version_fail(out, SERVICE_VERSION_STEP_BASE_MAJOR, &res);
        return VAL_SUCCESS;
    }

    /* Step 3: Discover the current instance service list from Base feature register 1. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG1_INDEX,
                               &feature_res);
    if (val_firme_status_to_i64(feature_res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg1 status=%llx",
                  SERVICE_VERSION_TEST_NAME, (unsigned long long)feature_res.x0);
        service_version_fail(out, SERVICE_VERSION_STEP_SERVICE_LIST, &feature_res);
        return VAL_SUCCESS;
    }
    service_list = feature_res.x1 & ((uint64_t)FIRME_BASE_SERVICE_LIST_MASK <<
                                    FIRME_BASE_SERVICE_LIST_SHIFT);

    /* Step 4: Allocated-but-absent services must report exactly NOT_SUPPORTED. */
    for (service_id = FIRME_SERVICE_ID_GRANULE_MGMT; service_id < FIRME_SERVICE_ID_MAX;
         service_id++) {
        allowed = val_firme_service_is_allowed_at_world(world, service_id);
        expected_present = ((allowed == VAL_TRUE) &&
                            ((service_list & val_firme_base_service_list_bit(service_id)) !=
                             0ULL)) ? VAL_TRUE : VAL_FALSE;

        val_firme_service_version((uint64_t)service_id, &res);
        status = val_firme_status_to_i64(res.x0);
        if (expected_present == VAL_FALSE) {
            if (status != (int64_t)FIRME_NOT_SUPPORTED) {
                LOG_ERROR(NULL, "%s: absent service_id=%u expected NOT_SUPPORTED x0=%llx",
                          SERVICE_VERSION_TEST_NAME, (unsigned int)service_id,
                          (unsigned long long)res.x0);
                service_version_fail(out, SERVICE_VERSION_STEP_ABSENT_SERVICE, &res);
                return VAL_SUCCESS;
            }
            continue;
        }

        if (status <= 0) {
            LOG_ERROR(NULL, "%s: present service_id=%u expected positive version x0=%llx",
                      SERVICE_VERSION_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            service_version_fail(out, SERVICE_VERSION_STEP_PRESENT_SERVICE, &res);
            return VAL_SUCCESS;
        }
    }

    /* Step 5: All unallocated 8-bit service IDs must not report a version. */
    for (service_id = FIRME_SERVICE_ID_MAX; service_id <= FIRME_SERVICE_ID_8BIT_MAX;
         service_id++) {
        val_firme_service_version((uint64_t)service_id, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: service_id=%u expected NOT_SUPPORTED x0=%llx",
                      SERVICE_VERSION_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            service_version_fail(out, SERVICE_VERSION_STEP_INVALID_ID_RANGE, &res);
            return VAL_SUCCESS;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - SERVICE_VERSION rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_base_service_version_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_version_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - SERVICE_VERSION rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_base_service_version_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_version_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - SERVICE_VERSION rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_base_service_version_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_version_run(FIRME_ACS_WORLD_REALM, out);
}
