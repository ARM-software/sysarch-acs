/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_firme_service.h"
#include "val_log.h"

#define IDE_ABI_COMPLIANCE_TEST_NAME "ide_abi_compliance"

#define IDE_ABI_COMPLIANCE_STEP_SECURE_VERSION UINT64_C(1)
#define IDE_ABI_COMPLIANCE_STEP_VERSION        UINT64_C(2)
#define IDE_ABI_COMPLIANCE_STEP_REG0           UINT64_C(3)
#define IDE_ABI_COMPLIANCE_STEP_MANDATORY_BITS UINT64_C(5)

/**
 *   @brief    - Initialize a rule result for IDE ABI compliance.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_abi_compliance_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record an IDE ABI compliance failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_abi_compliance_fail(firme_rule_res_t *out, uint64_t fail_step,
                                    uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_ABI_COMPLIANCE_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate mandatory IDE ABI bits for an IDE-capable instance.
 *   @param    - world : Current caller world.
 *   @param    - out   : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int ide_abi_compliance_run_supported(firme_acs_world_t world, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t expected_bits = val_firme_service_mandatory_abi_mask(
        world, FIRME_SERVICE_ID_IDE_KEY_MGMT);
    int64_t status = 0;

    ide_abi_compliance_init_result(out);

    /* Step 1: Skip only when this FIRME instance does not implement IDE service. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (status < 0) {
        ide_abi_compliance_fail(out, IDE_ABI_COMPLIANCE_STEP_VERSION, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Read IDE feature register 0. */
    val_firme_service_features(FIRME_SERVICE_ID_IDE_KEY_MGMT,
                               FIRME_IDE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_abi_compliance_fail(out, IDE_ABI_COMPLIANCE_STEP_REG0, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: Check mandatory ABI bits and ignore unused output bits. */
    if ((res.x1 & expected_bits) != expected_bits) {
        ide_abi_compliance_fail(out, IDE_ABI_COMPLIANCE_STEP_MANDATORY_BITS,
                                res.x0, res.x1);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate Secure instance IDE service is not supported.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_abi_compliance_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};

    (void)base_pa;
    ide_abi_compliance_init_result(out);

    /* Step 1: Secure FIRME instance must not implement IDE key management. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
        ide_abi_compliance_fail(out, IDE_ABI_COMPLIANCE_STEP_SECURE_VERSION,
                                res.x0, res.x1);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - IDE ABI compliance rule body for NS caller execution.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_abi_compliance_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return ide_abi_compliance_run_supported(FIRME_ACS_WORLD_NSEL2, out);
}

/**
 *   @brief    - IDE ABI compliance rule body for Realm caller execution.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_abi_compliance_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return ide_abi_compliance_run_supported(FIRME_ACS_WORLD_REALM, out);
}
