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

#define SERVICE_FEATURES_TEST_NAME "service_features"

#define SERVICE_FEATURES_STEP_VALID_BASE_REG0 UINT64_C(1)
#define SERVICE_FEATURES_STEP_VALID_BASE_REG1 UINT64_C(2)
#define SERVICE_FEATURES_STEP_ABSENT_SERVICE  UINT64_C(3)
#define SERVICE_FEATURES_STEP_PRESENT_SERVICE UINT64_C(4)
#define SERVICE_FEATURES_STEP_INVALID_REG     UINT64_C(5)
#define SERVICE_FEATURES_STEP_INVALID_SERVICE UINT64_C(6)

/**
 *   @brief    - Initialize a rule result record for the service features test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void service_features_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service features test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void service_features_fail(firme_rule_res_t *out, uint64_t fail_step,
                                  uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate SERVICE_FEATURES ABI return semantics.
 *   @param    - world : FIRME caller instance.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
static int service_features_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t service_list = 0ULL;
    uint32_t feature_reg_index = 0U;
    uint32_t service_id = 0U;
    uint32_t expected_present = VAL_FALSE;
    uint32_t allowed = VAL_FALSE;
    int64_t expected_status = FIRME_NOT_SUPPORTED;

    if (out == NULL)
        return VAL_ERROR;

    service_features_init_result(out);

    /* Step 1: Base feature register 0 must be returned for a valid request. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg0 expected SUCCESS x0=%llx",
                  SERVICE_FEATURES_TEST_NAME, (unsigned long long)res.x0);
        service_features_fail(out, SERVICE_FEATURES_STEP_VALID_BASE_REG0, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Base feature register 1 must be returned for a valid request. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG1_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg1 expected SUCCESS x0=%llx",
                  SERVICE_FEATURES_TEST_NAME, (unsigned long long)res.x0);
        service_features_fail(out, SERVICE_FEATURES_STEP_VALID_BASE_REG1, res.x0, res.x1);
        return VAL_ERROR;
    }

    service_list = res.x1 & ((uint64_t)FIRME_BASE_SERVICE_LIST_MASK <<
                             FIRME_BASE_SERVICE_LIST_SHIFT);

    /* Step 3: Check every register index at each allocated service ID. */
    for (service_id = FIRME_SERVICE_ID_GRANULE_MGMT; service_id < FIRME_SERVICE_ID_MAX;
         service_id++) {
        allowed = val_firme_service_is_allowed_at_world(world, service_id);
        expected_present = ((allowed == VAL_TRUE) &&
                            ((service_list & val_firme_base_service_list_bit(service_id)) !=
                             0ULL)) ? VAL_TRUE : VAL_FALSE;

        for (feature_reg_index = FIRME_BASE_FEATURE_REG0_INDEX;
             feature_reg_index <= FIRME_FEATURE_REG_INDEX_8BIT_MAX; feature_reg_index++) {
            expected_status = ((expected_present == VAL_TRUE) &&
                               (feature_reg_index <= FIRME_BASE_FEATURE_REG1_INDEX)) ?
                               FIRME_SUCCESS : FIRME_NOT_SUPPORTED;
            val_firme_service_features(service_id, feature_reg_index, &res);
            if (val_firme_status_to_i64(res.x0) != expected_status) {
                LOG_ERROR(NULL, "%s: service=%u register=%u expected=%lld x0=%llx\n",
                          SERVICE_FEATURES_TEST_NAME, service_id, feature_reg_index,
                          (long long)expected_status, (unsigned long long)res.x0);
                service_features_fail(out,
                    expected_status == FIRME_SUCCESS ? SERVICE_FEATURES_STEP_PRESENT_SERVICE :
                        SERVICE_FEATURES_STEP_ABSENT_SERVICE,
                    res.x0, ((uint64_t)service_id << 32) | feature_reg_index);
                return VAL_ERROR;
            }
        }
    }

    /* Step 4: All unallocated Base feature register indices must be unsupported. */
    for (feature_reg_index = FIRME_BASE_FEATURE_REG_COUNT;
         feature_reg_index <= FIRME_FEATURE_REG_INDEX_8BIT_MAX; feature_reg_index++) {
        val_firme_service_features(FIRME_SERVICE_ID_BASE, (uint64_t)feature_reg_index, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: base feature reg%u expected NOT_SUPPORTED x0=%llx",
                      SERVICE_FEATURES_TEST_NAME, (unsigned int)feature_reg_index,
                      (unsigned long long)res.x0);
            service_features_fail(out, SERVICE_FEATURES_STEP_INVALID_REG, res.x0, res.x1);
            return VAL_ERROR;
        }
    }

    /* Step 5: All unallocated 8-bit service IDs must be unsupported. */
    for (service_id = FIRME_SERVICE_ID_MAX; service_id <= FIRME_SERVICE_ID_8BIT_MAX;
         service_id++) {
        val_firme_service_features((uint64_t)service_id, FIRME_BASE_FEATURE_REG0_INDEX, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
            LOG_ERROR(NULL, "%s: service_id=%u expected NOT_SUPPORTED x0=%llx",
                      SERVICE_FEATURES_TEST_NAME, (unsigned int)service_id,
                      (unsigned long long)res.x0);
            service_features_fail(out, SERVICE_FEATURES_STEP_INVALID_SERVICE, res.x0, res.x1);
            return VAL_ERROR;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - SERVICE_FEATURES rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_features_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_features_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - SERVICE_FEATURES rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_features_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_features_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - SERVICE_FEATURES rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_features_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_features_run(FIRME_ACS_WORLD_REALM, out);
}
