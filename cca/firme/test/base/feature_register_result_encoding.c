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

#define FEATURE_REGISTER_RESULT_TEST_NAME "feature_register_result_encoding"
#define FEATURE_REGISTER_RESULT_STEP_REG0_STATUS UINT64_C(1)
#define FEATURE_REGISTER_RESULT_STEP_REG0_VALUE  UINT64_C(2)
#define FEATURE_REGISTER_RESULT_STEP_REG1_STATUS UINT64_C(3)
#define FEATURE_REGISTER_RESULT_STEP_REG1_VALUE  UINT64_C(4)

/**
 *   @brief    - Initialize a feature-register result-encoding record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void feature_register_result_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a feature-register result-encoding failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void feature_register_result_fail(firme_rule_res_t *out, uint64_t fail_step,
                                         uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Return service-list bits permitted for a caller world.
 *   @param    - world : Caller world represented by the current payload.
 *   @return   - Permitted Base feature register 1 service-list bit mask.
 **/
static uint64_t feature_register_result_allowed_service_list(firme_acs_world_t world)
{
    uint64_t service_list = 0ULL;
    uint32_t service_id = FIRME_SERVICE_ID_GRANULE_MGMT;

    for (; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        if (val_firme_service_is_allowed_at_world(world, service_id) == VAL_TRUE)
            service_list |= val_firme_base_service_list_bit(service_id);
    }

    return service_list;
}

/**
 *   @brief    - Validate 64-bit Base feature registers are returned in x1.
 *   @param    - world : Caller world represented by the current payload.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int feature_register_result_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t service_list_mask = (uint64_t)FIRME_BASE_SERVICE_LIST_MASK <<
                                 FIRME_BASE_SERVICE_LIST_SHIFT;
    uint64_t allowed_service_list = feature_register_result_allowed_service_list(world);
    uint64_t actual_service_list = 0ULL;
    uint64_t min_sh_buf_sz = 0ULL;

    feature_register_result_init_result(out);

    /* Step 1: Read Base feature register 0 using SERVICE_FEATURES. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg0 expected SUCCESS x0=%llx",
                  FEATURE_REGISTER_RESULT_TEST_NAME, (unsigned long long)res.x0);
        feature_register_result_fail(out, FEATURE_REGISTER_RESULT_STEP_REG0_STATUS,
                                     res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 2: Bits[63:0] of Base feature register 0 must be visible in x1. */
    if ((res.x1 & FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) !=
        FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) {
        LOG_ERROR(NULL, "%s: base feature reg0 x1 missing mandatory bits value=%llx",
                  FEATURE_REGISTER_RESULT_TEST_NAME, (unsigned long long)res.x1);
        feature_register_result_fail(out, FEATURE_REGISTER_RESULT_STEP_REG0_VALUE,
                                     res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 3: Read Base feature register 1 using SERVICE_FEATURES. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG1_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg1 expected SUCCESS x0=%llx",
                  FEATURE_REGISTER_RESULT_TEST_NAME, (unsigned long long)res.x0);
        feature_register_result_fail(out, FEATURE_REGISTER_RESULT_STEP_REG1_STATUS,
                                     res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 4: A non-zero permitted service-list field must be visible in x1. */
    actual_service_list = res.x1 & service_list_mask;
    if ((actual_service_list == 0ULL) ||
        ((actual_service_list & ~allowed_service_list) != 0ULL)) {
        LOG_ERROR(NULL, "%s: base feature reg1 service-list value=%llx allowed=%llx",
                  FEATURE_REGISTER_RESULT_TEST_NAME,
                  (unsigned long long)actual_service_list,
                  (unsigned long long)allowed_service_list);
        feature_register_result_fail(out, FEATURE_REGISTER_RESULT_STEP_REG1_VALUE,
                                     res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 5: MIN_SH_BUF_SZ encoding in x1 must use an allocated value. */
    min_sh_buf_sz = (res.x1 >> FIRME_BASE_MIN_SH_BUF_SZ_SHIFT) &
                    FIRME_BASE_MIN_SH_BUF_SZ_MASK;
    if (min_sh_buf_sz > FIRME_BASE_MIN_SH_BUF_SZ_MAX_ALLOCATED) {
        LOG_ERROR(NULL, "%s: base feature reg1 x1 has invalid MIN_SH_BUF_SZ=%llu",
                  FEATURE_REGISTER_RESULT_TEST_NAME, (unsigned long long)min_sh_buf_sz);
        feature_register_result_fail(out, FEATURE_REGISTER_RESULT_STEP_REG1_VALUE,
                                     res.x0, res.x1);
        return VAL_SUCCESS;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Feature register result-encoding rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_result_encoding_run_ns(uint64_t base_pa,
                                                       firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_result_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Feature register result-encoding rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_result_encoding_run_secure(uint64_t base_pa,
                                                           firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_result_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Feature register result-encoding rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_result_encoding_run_realm(uint64_t base_pa,
                                                          firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_result_run(FIRME_ACS_WORLD_REALM, out);
}
