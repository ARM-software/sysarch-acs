/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_rules.h"
#include "val_firme_smccc_probe.h"
#include "val_common/val_smccc.h"
#include "val_log.h"

#define SMCCC_VERSION_TEST_NAME "smccc_version"
#define SMCCC_VERSION_FID       UINT64_C(0x80000000)
#define SMCCC_VERSION_MAJOR_SHIFT 16U
#define SMCCC_VERSION_MINOR_MASK  UINT64_C(0xffff)
#define SMCCC_VERSION_MIN_MAJOR   UINT64_C(1)
#define SMCCC_VERSION_MIN_MINOR   UINT64_C(2)

#define SMCCC_VERSION_STEP_CALL  UINT64_C(1)
#define SMCCC_VERSION_STEP_MAJOR UINT64_C(2)
#define SMCCC_VERSION_STEP_MINOR UINT64_C(3)

/**
 *   @brief    - Initialize an SMCCC version test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void smccc_version_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record an SMCCC version test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - res       : SMCCC result.
 *   @return   - void.
 **/
static void smccc_version_fail(firme_rule_res_t *out, uint64_t fail_step, smccc_res_t *res)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Validate SMCCC >= 1.2 for FIRME ABI hosting.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
static int smccc_version_run(firme_rule_res_t *out)
{
    smccc_res_t res = {0};
    uint64_t major = 0;
    uint64_t minor = 0;

    smccc_version_init_result(out);

    /* Step 1: Query the SMCCC version implemented by EL3 firmware. */
    val_smccc_smc_call(SMCCC_VERSION_FID, 0, 0, 0, 0, 0, 0, 0, &res);
    if ((res.x0 & UINT64_C(0x80000000)) != 0ULL) {
        LOG_ERROR(NULL, "%s: SMCCC_VERSION failed x0=%llx\n",
                  SMCCC_VERSION_TEST_NAME, (unsigned long long)res.x0);
        smccc_version_fail(out, SMCCC_VERSION_STEP_CALL, &res);
        return VAL_ERROR;
    }

    major = res.x0 >> SMCCC_VERSION_MAJOR_SHIFT;
    minor = res.x0 & SMCCC_VERSION_MINOR_MASK;

    /* Step 2: Major version must be at least 1 for SMCCC 1.2 compliance. */
    if (major < SMCCC_VERSION_MIN_MAJOR) {
        LOG_ERROR(NULL, "%s: SMCCC major=%llu expected >=%llu\n",
                  SMCCC_VERSION_TEST_NAME, (unsigned long long)major,
                  (unsigned long long)SMCCC_VERSION_MIN_MAJOR);
        smccc_version_fail(out, SMCCC_VERSION_STEP_MAJOR, &res);
        return VAL_ERROR;
    }

    /* Step 3: For major version 1, minor version must be at least 2. */
    if ((major == SMCCC_VERSION_MIN_MAJOR) && (minor < SMCCC_VERSION_MIN_MINOR)) {
        LOG_ERROR(NULL, "%s: SMCCC version=%llu.%llu expected >=1.2\n",
                  SMCCC_VERSION_TEST_NAME, (unsigned long long)major,
                  (unsigned long long)minor);
        smccc_version_fail(out, SMCCC_VERSION_STEP_MINOR, &res);
        return VAL_ERROR;
    }

    /* Step 4: Snapshot preserved general and SIMD/FP registers across Base SMCs. */
    if (val_firme_smccc_check_state(0, out) != VAL_SUCCESS)
        return VAL_SUCCESS;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - SMCCC version rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
int firme_base_smccc_version_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_version_run(out);
}

/**
 *   @brief    - SMCCC version rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
int firme_base_smccc_version_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_version_run(out);
}

/**
 *   @brief    - SMCCC version rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
int firme_base_smccc_version_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_version_run(out);
}
