/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define IDE_FEATURE_REGISTER_TEST_NAME "ide_feature_register"

#define IDE_FEATURE_REGISTER_STEP_VERSION       UINT64_C(1)
#define IDE_FEATURE_REGISTER_STEP_REG0_STATUS   UINT64_C(2)
#define IDE_FEATURE_REGISTER_STEP_MANDATORY_ABI UINT64_C(4)

/**
 *   @brief    - Initialize a rule result for the IDE feature-register test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_feature_register_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record an IDE feature-register failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_feature_register_fail(firme_rule_res_t *out, uint64_t fail_step,
                                      uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_FEATURE_REGISTER_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate IDE feature register 0 from an IDE-capable instance.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int ide_feature_register_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    int64_t status = 0;
    uint64_t feature_reg0 = 0ULL;

    ide_feature_register_init_result(out);

    /* Step 1: If IDE key management is absent, the platform cannot run this rule. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (status < 0) {
        ide_feature_register_fail(out, IDE_FEATURE_REGISTER_STEP_VERSION, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Feature register 0 must be implemented for IDE key management. */
    val_firme_service_features(FIRME_SERVICE_ID_IDE_KEY_MGMT,
                               FIRME_IDE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_feature_register_fail(out, IDE_FEATURE_REGISTER_STEP_REG0_STATUS,
                                  res.x0, res.x1);
        return VAL_ERROR;
    }
    feature_reg0 = res.x1;

    /* Step 3: Check mandatory ABI bits and ignore unused output bits. */
    if ((feature_reg0 & FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS) !=
        FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS) {
        ide_feature_register_fail(out, IDE_FEATURE_REGISTER_STEP_MANDATORY_ABI,
                                  res.x0, feature_reg0);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - IDE feature-register rule body for NS caller execution.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_feature_register_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return ide_feature_register_run(out);
}

/**
 *   @brief    - IDE feature-register rule body for Realm caller execution.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_feature_register_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return ide_feature_register_run(out);
}
