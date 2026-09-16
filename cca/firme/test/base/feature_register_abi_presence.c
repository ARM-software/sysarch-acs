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

#define FEATURE_REGISTER_ABI_TEST_NAME "feature_register_abi_presence"

#define FEATURE_REGISTER_ABI_STEP_BASE_REG0_STATUS UINT64_C(1)
#define FEATURE_REGISTER_ABI_STEP_BASE_REG0_BITS   UINT64_C(2)
#define FEATURE_REGISTER_ABI_STEP_SERVICE_REG0     UINT64_C(3)
#define FEATURE_REGISTER_ABI_STEP_UNALLOCATED_BITS UINT64_C(4)
#define FEATURE_REGISTER_ABI_STEP_MANDATORY_BITS   UINT64_C(5)
#define FEATURE_REGISTER_ABI_STEP_PROHIBITED_BITS  UINT64_C(6)
#define FEATURE_REGISTER_ABI_STEP_VERSION_STATUS   UINT64_C(7)

/**
 *   @brief    - Initialize a feature-register ABI-presence result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void feature_register_abi_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a feature-register ABI-presence failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void feature_register_abi_fail(firme_rule_res_t *out, uint64_t fail_step,
                                      uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate one advertised service reports ABI bits correctly.
 *   @param    - world      : Caller world represented by the current payload.
 *   @param    - service_id : FIRME service ID.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_TRUE if the service register is valid, otherwise VAL_FALSE.
 **/
static uint32_t feature_register_abi_check_service(firme_acs_world_t world,
                                                   uint32_t service_id,
                                                   firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t allocated_mask = 0ULL;
    uint64_t mandatory_mask = 0ULL;
    uint64_t supported_mask = 0ULL;
    uint64_t feature_reg0 = 0ULL;
    uint32_t advertised = VAL_FALSE;

    if (val_firme_service_get_feature_reg0(service_id, &advertised, &feature_reg0, &res) !=
        VAL_SUCCESS) {
        LOG_ERROR(NULL, "%s: service_id=%u reg0 unexpected x0=%llx",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)res.x0);
        feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_SERVICE_REG0,
                                  res.x0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    if (advertised == VAL_FALSE) {
        LOG_DEBUG(NULL, "%s: service_id=%u reg0 not advertised; skipping ABI checks",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id);
        return VAL_TRUE;
    }

    allocated_mask = val_firme_service_allocated_abi_mask(service_id);
    mandatory_mask = val_firme_service_mandatory_abi_mask(world, service_id);
    supported_mask = val_firme_service_supported_abi_mask(world, service_id);

    if ((feature_reg0 & mandatory_mask) != mandatory_mask) {
        LOG_ERROR(NULL, "%s: service_id=%u mandatory bits missing value=%llx mask=%llx",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0, (unsigned long long)mandatory_mask);
        feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_MANDATORY_BITS,
                                  feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    if ((feature_reg0 & (allocated_mask & ~supported_mask)) != 0ULL) {
        LOG_ERROR(NULL, "%s: service_id=%u prohibited bits set value=%llx mask=%llx",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0,
                  (unsigned long long)(allocated_mask & ~supported_mask));
        feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_PROHIBITED_BITS,
                                  feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    if ((feature_reg0 & ~allocated_mask) != 0ULL) {
        LOG_ERROR(NULL, "%s: service_id=%u reg0 unallocated bits value=%llx mask=%llx",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id,
                  (unsigned long long)feature_reg0, (unsigned long long)allocated_mask);
        feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_UNALLOCATED_BITS,
                                  feature_reg0, (uint64_t)service_id);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate feature register 0 ABI-bit reporting for advertised services.
 *   @param    - world : Caller world represented by the current payload.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
static int feature_register_abi_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint32_t service_id = 0U;
    uint32_t implemented = VAL_FALSE;
    int64_t status = 0;

    feature_register_abi_init_result(out);

    /* Step 1: Base feature register 0 must be implemented. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg0 expected SUCCESS x0=%llx",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned long long)res.x0);
        feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_BASE_REG0_STATUS,
                                  res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 2: Base feature register 0 must report both mandatory Base ABIs. */
    if ((res.x1 & FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) !=
        FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) {
        LOG_ERROR(NULL, "%s: base feature reg0 missing mandatory ABI bits value=%llx",
                  FEATURE_REGISTER_ABI_TEST_NAME, (unsigned long long)res.x1);
        feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_BASE_REG0_BITS,
                                  res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 3-6: Advertised services must expose only valid ABI bits. */
    for (service_id = FIRME_SERVICE_ID_BASE; service_id < FIRME_SERVICE_ID_MAX; service_id++) {
        implemented = VAL_FALSE;
        if (val_firme_service_get_version(service_id, &implemented, &res) != VAL_SUCCESS) {
            status = val_firme_status_to_i64(res.x0);
            LOG_ERROR(NULL, "%s: service_id=%u version unexpected status=%lld",
                      FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id,
                      (long long)status);
            feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_VERSION_STATUS,
                                      res.x0, (uint64_t)service_id);
            return VAL_SUCCESS;
        }

        if (implemented == VAL_FALSE)
            continue;

        if (val_firme_service_is_allowed_at_world(world, service_id) == VAL_FALSE) {
            LOG_ERROR(NULL, "%s: service_id=%u implemented at forbidden instance",
                      FEATURE_REGISTER_ABI_TEST_NAME, (unsigned int)service_id);
            feature_register_abi_fail(out, FEATURE_REGISTER_ABI_STEP_VERSION_STATUS,
                                      res.x0, (uint64_t)service_id);
            return VAL_SUCCESS;
        }

        if (feature_register_abi_check_service(world, service_id, out) == VAL_FALSE)
            return VAL_SUCCESS;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Feature register ABI-presence rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_abi_presence_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_abi_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Feature register ABI-presence rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_abi_presence_run_secure(uint64_t base_pa,
                                                        firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_abi_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Feature register ABI-presence rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_feature_register_abi_presence_run_realm(uint64_t base_pa,
                                                       firme_rule_res_t *out)
{
    (void)base_pa;
    return feature_register_abi_run(FIRME_ACS_WORLD_REALM, out);
}
