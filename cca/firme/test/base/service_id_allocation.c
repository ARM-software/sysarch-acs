/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_firme_service.h"
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
#define SERVICE_ID_ALLOCATION_STEP_ALLOCATED_ABIS        UINT64_C(5)
#define SERVICE_ID_ALLOCATION_STEP_IDENTITY_COVERAGE     UINT64_C(6)

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
 *   @param    - world : FIRME caller instance.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS with SKIP for incomplete evidence, otherwise VAL_ERROR.
 **/
static int service_id_allocation_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t service_id = 0;
    int64_t version_status = 0;
    int64_t feature_status = 0;
    uint64_t mandatory_mask = 0ULL;
    uint64_t forbidden_mask = 0ULL;

    if (out == NULL)
        return VAL_ERROR;

    service_id_allocation_init_result(out);

    /* Step 1: The Base service is allocated at service ID 0 and must respond. */
    val_firme_service_version(FIRME_SERVICE_ID_BASE, &res);
    if (val_firme_status_to_i64(res.x0) <= 0) {
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
        if ((version_status <= 0) && (version_status != FIRME_NOT_SUPPORTED)) {
            LOG_ERROR(NULL, "%s: service_id=%llu unexpected version status=%lld",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (long long)version_status);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_ALLOCATED_DISCOVERY,
                                       service_id, &res);
            return VAL_ERROR;
        }

        if ((version_status > 0) &&
            (val_firme_service_is_allowed_at_world(world, service_id) != VAL_TRUE)) {
            LOG_ERROR(NULL, "%s: service_id=%llu implemented at forbidden world=%u\n",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (unsigned int)world);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_ALLOCATED_DISCOVERY,
                                       service_id, &res);
            return VAL_ERROR;
        }

        val_firme_service_features(service_id, FIRME_BASE_FEATURE_REG0_INDEX, &res);
        feature_status = val_firme_status_to_i64(res.x0);
        if (((version_status == FIRME_NOT_SUPPORTED) &&
             (feature_status != FIRME_NOT_SUPPORTED)) ||
            ((version_status > 0) && (feature_status != FIRME_SUCCESS))) {
            LOG_ERROR(NULL, "%s: service_id=%llu version=%lld feature status=%lld",
                      SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                      (long long)version_status, (long long)feature_status);
            service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_ALLOCATED_DISCOVERY,
                                       service_id, &res);
            return VAL_ERROR;
        }

        /* Check the service-specific compliance mask at its assigned ID. */
        if (version_status > 0) {
            mandatory_mask = val_firme_service_mandatory_abi_mask(world, service_id);
            forbidden_mask = val_firme_service_allocated_abi_mask(service_id) &
                             ~val_firme_service_supported_abi_mask(world, service_id);
            if (((res.x1 & mandatory_mask) != mandatory_mask) ||
                ((res.x1 & forbidden_mask) != 0ULL)) {
                LOG_ERROR(NULL, "%s: service_id=%llu ABI map invalid x1=%llx\n",
                          SERVICE_ID_ALLOCATION_TEST_NAME, (unsigned long long)service_id,
                          (unsigned long long)res.x1);
                service_id_allocation_fail(out, SERVICE_ID_ALLOCATION_STEP_ALLOCATED_ABIS,
                                           service_id, &res);
                return VAL_ERROR;
            }
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

    LOG_WARN(NULL, "%s: discovery allocation checks passed; service-specific "
             "operation evidence is not available for every implemented ID\n",
             SERVICE_ID_ALLOCATION_TEST_NAME);
    out->status = (uint64_t)ACS_STATUS_SKIP;
    out->fail_step = SERVICE_ID_ALLOCATION_STEP_IDENTITY_COVERAGE;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Service ID allocation rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS with SKIP for incomplete evidence, otherwise VAL_ERROR.
 **/
int firme_base_service_id_allocation_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_id_allocation_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Service ID allocation rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS with SKIP for incomplete evidence, otherwise VAL_ERROR.
 **/
int firme_base_service_id_allocation_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_id_allocation_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Service ID allocation rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS with SKIP for incomplete evidence, otherwise VAL_ERROR.
 **/
int firme_base_service_id_allocation_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_id_allocation_run(FIRME_ACS_WORLD_REALM, out);
}
