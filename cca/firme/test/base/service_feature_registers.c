/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_dispatch.h"
#include "val_firme_rules.h"
#include "val_firme_service.h"
#include "val_log.h"

#define SERVICE_FEATURE_REGISTERS_TEST_NAME "service_feature_registers"

#define SERVICE_FEATURE_REGISTERS_STEP_REG0_STATUS      UINT64_C(1)
#define SERVICE_FEATURE_REGISTERS_STEP_REG0_BITS        UINT64_C(2)
#define SERVICE_FEATURE_REGISTERS_STEP_REG1_STATUS      UINT64_C(3)
#define SERVICE_FEATURE_REGISTERS_STEP_MIN_BUF_ENCODING UINT64_C(4)
#define SERVICE_FEATURE_REGISTERS_STEP_SERVICE_LIST     UINT64_C(5)

/**
 *   @brief    - Initialize a rule result record for the feature register test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void service_feature_registers_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a service feature register test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void service_feature_registers_fail(firme_rule_res_t *out, uint64_t fail_step,
                                           uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Return the Table 2.3 service-list bits permitted for a caller world.
 *   @param    - world : Caller world represented by the current payload.
 *   @return   - Permitted Base feature register 1 service-list bit mask.
 **/
static uint64_t service_feature_registers_allowed_service_list(firme_acs_world_t world)
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
 *   @brief    - Validate Base feature register 1 service-list encoding.
 *   @param    - feature_reg1 : Base service feature register 1 value.
 *   @param    - world        : Caller world represented by the current payload.
 *   @param    - out          : Optional result record.
 *   @return   - VAL_TRUE if the list is a permitted Table 2.3 subset, otherwise VAL_FALSE.
 **/
static uint32_t service_feature_registers_check_service_list(uint64_t feature_reg1,
                                                             firme_acs_world_t world,
                                                             firme_rule_res_t *out)
{
    uint64_t service_list_mask = FIRME_BASE_SERVICE_LIST_ALLOCATED_MASK <<
                                 FIRME_BASE_SERVICE_LIST_SHIFT;
    uint64_t actual_mask = feature_reg1 & service_list_mask;
    uint64_t allowed_mask = service_feature_registers_allowed_service_list(world);

    if ((actual_mask & ~allowed_mask) != 0ULL) {
        LOG_ERROR(NULL, "%s: service-list value=%llx allowed=%llx",
                  SERVICE_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)actual_mask,
                  (unsigned long long)allowed_mask);
        service_feature_registers_fail(out, SERVICE_FEATURE_REGISTERS_STEP_SERVICE_LIST,
                                       actual_mask, allowed_mask);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate Base service feature register encodings.
 *   @param    - world : Caller world represented by the current payload.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
static int service_feature_registers_run(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t min_sh_buf_sz = 0;

    service_feature_registers_init_result(out);

    /* Step 1: Base feature register 0 must be readable. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg0 status=%llx",
                  SERVICE_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)res.x0);
        service_feature_registers_fail(out, SERVICE_FEATURE_REGISTERS_STEP_REG0_STATUS,
                                       res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Base feature register 0 must report both Base service ABIs. */
    if ((res.x1 & FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) !=
        FIRME_BASE_FEATURE_REG0_MANDATORY_BITS) {
        LOG_ERROR(NULL, "%s: base feature reg0 missing mandatory bits value=%llx",
                  SERVICE_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)res.x1);
        service_feature_registers_fail(out, SERVICE_FEATURE_REGISTERS_STEP_REG0_BITS,
                                       res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: Base feature register 1 must be readable. */
    val_firme_service_features(FIRME_SERVICE_ID_BASE, FIRME_BASE_FEATURE_REG1_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        LOG_ERROR(NULL, "%s: base feature reg1 status=%llx",
                  SERVICE_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)res.x0);
        service_feature_registers_fail(out, SERVICE_FEATURE_REGISTERS_STEP_REG1_STATUS,
                                       res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 4: MIN_SH_BUF_SZ encoding must use one of the allocated values. */
    min_sh_buf_sz = (res.x1 >> FIRME_BASE_MIN_SH_BUF_SZ_SHIFT) &
                    FIRME_BASE_MIN_SH_BUF_SZ_MASK;
    if (min_sh_buf_sz > FIRME_BASE_MIN_SH_BUF_SZ_MAX_ALLOCATED) {
        LOG_ERROR(NULL, "%s: invalid MIN_SH_BUF_SZ=%llu",
                  SERVICE_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)min_sh_buf_sz);
        service_feature_registers_fail(out, SERVICE_FEATURE_REGISTERS_STEP_MIN_BUF_ENCODING,
                                       res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 5: Feature register service list must be a Table 2.3 subset for this world. */
    if (service_feature_registers_check_service_list(res.x1, world, out) == VAL_FALSE)
        return VAL_ERROR;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Feature register rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_feature_registers_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_feature_registers_run(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - Feature register rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_feature_registers_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_feature_registers_run(FIRME_ACS_WORLD_SECURE, out);
}

/**
 *   @brief    - Feature register rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS, otherwise VAL_ERROR.
 **/
int firme_base_service_feature_registers_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return service_feature_registers_run(FIRME_ACS_WORLD_REALM, out);
}
