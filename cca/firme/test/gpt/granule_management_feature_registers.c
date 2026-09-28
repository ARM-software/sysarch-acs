/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define GM_FEATURE_REGISTERS_TEST_NAME "granule_management_feature_registers"

#define GM_FEATURE_REGISTERS_STEP_VERSION       UINT64_C(1)
#define GM_FEATURE_REGISTERS_STEP_REG0_STATUS   UINT64_C(2)
#define GM_FEATURE_REGISTERS_STEP_REG1_STATUS   UINT64_C(4)
#define GM_FEATURE_REGISTERS_STEP_REG1_PGS      UINT64_C(6)
#define GM_FEATURE_REGISTERS_STEP_NEXT_REG      UINT64_C(7)
#define GM_FEATURE_REGISTERS_STEP_REG1_RANGE    UINT64_C(8)
#define GM_FEATURE_REGISTERS_STEP_REG1_PPS      UINT64_C(9)

#define GM_FEATURE_REGISTERS_REG_COUNT          UINT64_C(2)

/**
 *   @brief    - Initialize a rule result for GM feature-register checks.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void gm_feature_registers_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a GM feature-register failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void gm_feature_registers_fail(firme_rule_res_t *out, uint64_t fail_step,
                                      uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              GM_FEATURE_REGISTERS_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate GM service feature-register indices, sizes, and encodings.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int gm_feature_registers_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t feature_reg1 = 0ULL;
    int64_t status = 0;

    if (!out)
        return VAL_ERROR;

    gm_feature_registers_init_result(out);

    /* Step 1: Skip only when Granule Management is absent at this instance. */
    val_firme_service_version(FIRME_SERVICE_ID_GRANULE_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }
    if (status < 0) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_VERSION,
                                  res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Feature register 0 must be implemented at index 0. */
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_REG0_STATUS,
                                  res.x0, res.x1);
        return VAL_ERROR;
    }
    /* Step 4: Feature register 1 must be implemented at index 1. */
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               FIRME_GM_FEATURE_REG1_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_REG1_STATUS,
                                  res.x0, res.x1);
        return VAL_ERROR;
    }
    feature_reg1 = res.x1;

    /* Unused output bits have no required value and are ignored by the caller. */

    /* Step 6: The PGS field must use an allocated GPCCR_EL3.PGS encoding. */
    if (val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1) == 0ULL) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_REG1_PGS,
                                  res.x0, feature_reg1);
        return VAL_ERROR;
    }

    if (val_firme_gpt_range_size_from_feature_reg1(feature_reg1) == 0ULL) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_REG1_RANGE,
                                  res.x0, feature_reg1);
        return VAL_ERROR;
    }
    if (val_firme_gpt_pps_size_from_feature_reg1(feature_reg1) == 0ULL) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_REG1_PPS,
                                  res.x0, feature_reg1);
        return VAL_ERROR;
    }

    /* Step 7: The first unallocated GM feature-register index must be absent. */
    val_firme_service_features(FIRME_SERVICE_ID_GRANULE_MGMT,
                               GM_FEATURE_REGISTERS_REG_COUNT, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
        gm_feature_registers_fail(out, GM_FEATURE_REGISTERS_STEP_NEXT_REG,
                                  res.x0, res.x1);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}

/**
 *   @brief    - GM feature-register rule body for NS caller execution.
 *   @param    - base_pa : Unused for feature-discovery tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_granule_management_feature_registers_run_ns(uint64_t base_pa,
                                                          firme_rule_res_t *out)
{
    (void)base_pa;
    return gm_feature_registers_run(out);
}

/**
 *   @brief    - GM feature-register rule body for Secure caller execution.
 *   @param    - base_pa : Unused for feature-discovery tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_granule_management_feature_registers_run_secure(uint64_t base_pa,
                                                              firme_rule_res_t *out)
{
    (void)base_pa;
    return gm_feature_registers_run(out);
}

/**
 *   @brief    - GM feature-register rule body for Realm caller execution.
 *   @param    - base_pa : Unused for feature-discovery tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_granule_management_feature_registers_run_realm(uint64_t base_pa,
                                                             firme_rule_res_t *out)
{
    (void)base_pa;
    return gm_feature_registers_run(out);
}
