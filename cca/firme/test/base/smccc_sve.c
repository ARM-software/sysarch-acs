/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/sysreg.h"
#include "val_firme_abi.h"
#include "val_firme_rules.h"
#include "val_firme_smccc_probe.h"
#include "val_common/val_smccc.h"
#include "val_log.h"

#define SMCCC_SVE_TEST_NAME "smccc_sve"
#define SMCCC_VERSION_FID   UINT64_C(0x80000000)
#define SMCCC_VERSION_MAJOR_SHIFT 16U
#define SMCCC_VERSION_MINOR_MASK  UINT64_C(0xffff)
#define SMCCC_SVE_PFR0_SHIFT 32U
#define SMCCC_SVE_PFR0_MASK  UINT64_C(0xf)
#define SMCCC_SVE_MIN_MINOR  UINT64_C(3)

#define SMCCC_SVE_STEP_VERSION UINT64_C(1)
#define SMCCC_SVE_STEP_MINOR   UINT64_C(2)

/**
 *   @brief    - Initialize an SMCCC SVE test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void smccc_sve_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record an SMCCC SVE test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void smccc_sve_fail(firme_rule_res_t *out, uint64_t fail_step,
                           uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate SMCCC >= 1.3 when FEAT_SVE is implemented.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
static int smccc_sve_run(firme_rule_res_t *out)
{
    smccc_res_t res = {0};
    uint64_t pfr0 = read_id_aa64pfr0_el1();
    uint64_t sve = (pfr0 >> SMCCC_SVE_PFR0_SHIFT) & SMCCC_SVE_PFR0_MASK;
    uint64_t major = 0;
    uint64_t minor = 0;

    smccc_sve_init_result(out);

    /* Step 1: If FEAT_SVE is absent, this conditional rule is not applicable. */
    if (sve == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        LOG_WARN(NULL, "SVE absent: state-preservation check not applicable\n");
        return VAL_SUCCESS;
    }

    /* Step 2: Query SMCCC version and require at least 1.3 for SVE context rules. */
    val_smccc_smc_call(SMCCC_VERSION_FID, 0, 0, 0, 0, 0, 0, 0, &res);
    if ((res.x0 & UINT64_C(0x80000000)) != 0ULL) {
        LOG_ERROR(NULL, "%s: SMCCC_VERSION failed x0=%llx\n",
                  SMCCC_SVE_TEST_NAME, (unsigned long long)res.x0);
        smccc_sve_fail(out, SMCCC_SVE_STEP_VERSION, res.x0, pfr0);
        return VAL_ERROR;
    }

    major = res.x0 >> SMCCC_VERSION_MAJOR_SHIFT;
    minor = res.x0 & SMCCC_VERSION_MINOR_MASK;
    if ((major < 1ULL) || ((major == 1ULL) && (minor < SMCCC_SVE_MIN_MINOR))) {
        LOG_ERROR(NULL, "%s: FEAT_SVE present but SMCCC version=%llu.%llu\n",
                  SMCCC_SVE_TEST_NAME, (unsigned long long)major,
                  (unsigned long long)minor);
        smccc_sve_fail(out, SMCCC_SVE_STEP_MINOR, res.x0, pfr0);
        return VAL_ERROR;
    }

    /* Step 3: Preserve live SVE state, then check legal hint-discardable state. */
    if (val_firme_smccc_check_state(VAL_SMCCC_STATE_SVE | VAL_SMCCC_STATE_FFR, out) !=
        VAL_SUCCESS)
        return VAL_SUCCESS;

    if (val_firme_smccc_check_state(VAL_SMCCC_STATE_SVE | VAL_SMCCC_STATE_FFR |
                                    VAL_SMCCC_PROBE_HINT, out) != VAL_SUCCESS)
        return VAL_SUCCESS;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - SMCCC SVE rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
int firme_base_smccc_sve_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_sve_run(out);
}

/**
 *   @brief    - SMCCC SVE rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
int firme_base_smccc_sve_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_sve_run(out);
}

/**
 *   @brief    - SMCCC SVE rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS after recording the architectural outcome.
 **/
int firme_base_smccc_sve_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_sve_run(out);
}
