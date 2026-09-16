/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_dispatch.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define SERVICE_INSTANCE_PRESENCE_TEST_NAME "service_instance_presence"

#define SERVICE_INSTANCE_PRESENCE_STEP_BASE_VERSION      UINT64_C(1)
#define SERVICE_INSTANCE_PRESENCE_STEP_BASE_FEATURES     UINT64_C(2)
#define SERVICE_INSTANCE_PRESENCE_STEP_FORBIDDEN_VERSION UINT64_C(3)
#define SERVICE_INSTANCE_PRESENCE_STEP_FORBIDDEN_FEATURE UINT64_C(4)

/**
 *   @brief    - Initialize a service instance presence test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void service_instance_presence_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service instance presence test failure.
 *   @param    - out        : Optional result record.
 *   @param    - fail_step  : Failing test step.
 *   @param    - service_id : FIRME service ID being checked.
 *   @param    - res        : FIRME SMC result.
 *   @return   - void.
 **/
static void service_instance_presence_fail(firme_rule_res_t *out, uint64_t fail_step,
                                           uint64_t service_id, firme_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = service_id;
    }
}

/**
 *   @brief    - Return whether a service is forbidden at the given FIRME instance.
 *   @param    - world      : Caller world representing the FIRME instance.
 *   @param    - service_id : FIRME service ID.
 *   @return   - VAL_TRUE if the service cannot be implemented at this instance.
 **/
static uint32_t service_instance_presence_is_forbidden(firme_acs_world_t world,
                                                       uint32_t service_id)
{
    if (world == FIRME_ACS_WORLD_SECURE) {
        if ((service_id == FIRME_SERVICE_ID_IDE_KEY_MGMT) ||
            (service_id == FIRME_SERVICE_ID_MECID_MGMT) ||
            (service_id == FIRME_SERVICE_ID_ATTESTATION) ||
            (service_id == FIRME_SERVICE_ID_INTEGRATED_DEVICE))
            return VAL_TRUE;
    }

    if (world == FIRME_ACS_WORLD_NSEL2) {
        if ((service_id == FIRME_SERVICE_ID_MECID_MGMT) ||
            (service_id == FIRME_SERVICE_ID_INTEGRATED_DEVICE))
            return VAL_TRUE;
    }

    return VAL_FALSE;
}

/**
 *   @brief    - Validate FIRME service presence constraints for one instance.
 *   @param    - world : Caller world representing the FIRME instance.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int service_instance_presence_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint32_t service_id = 0U;

    service_instance_presence_init_result(out);

    /* Step 1: The Base service must be present at every FIRME instance. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &res);
    if (val_firme_status_to_i64(res.x0) < 0) {
        LOG_ERROR(NULL, "%s: base service missing x0=%llx",
                  SERVICE_INSTANCE_PRESENCE_TEST_NAME, (unsigned long long)res.x0);
        service_instance_presence_fail(out, SERVICE_INSTANCE_PRESENCE_STEP_BASE_VERSION,
                                       FIRME_SERVICE_ID_BASE, &res);
        return VAL_SUCCESS;
    }

    /* Step 2: The Base service feature register 0 must be present. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base service feature reg0 missing x0=%llx",
                  SERVICE_INSTANCE_PRESENCE_TEST_NAME, (unsigned long long)res.x0);
        service_instance_presence_fail(out, SERVICE_INSTANCE_PRESENCE_STEP_BASE_FEATURES,
                                       FIRME_SERVICE_ID_BASE, &res);
        return VAL_SUCCESS;
    }

    /* Step 3/4: Services marked "No" in Table 2.3 must not be implemented. */
    for (service_id = FIRME_SERVICE_ID_GRANULE_MGMT;
         service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        if (service_instance_presence_is_forbidden(world, service_id) == VAL_FALSE)
            continue;

        val_firme_service_version((uint64_t)service_id, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: forbidden service_id=%u version x0=%llx",
                      SERVICE_INSTANCE_PRESENCE_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            service_instance_presence_fail(out,
                                           SERVICE_INSTANCE_PRESENCE_STEP_FORBIDDEN_VERSION,
                                           (uint64_t)service_id, &res);
            return VAL_SUCCESS;
        }

        val_firme_service_features((uint64_t)service_id, 0, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: forbidden service_id=%u features x0=%llx",
                      SERVICE_INSTANCE_PRESENCE_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            service_instance_presence_fail(out,
                                           SERVICE_INSTANCE_PRESENCE_STEP_FORBIDDEN_FEATURE,
                                           (uint64_t)service_id, &res);
            return VAL_SUCCESS;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service instance presence rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_instance_presence_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_instance_presence_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Service instance presence rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_instance_presence_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_instance_presence_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Service instance presence rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_service_instance_presence_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_instance_presence_run(FIRME_ACS_WORLD_REALM, out);
}
