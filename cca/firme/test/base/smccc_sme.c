/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "aarch64/sysreg.h"
#include "val_firme_rules.h"
#include "val_firme_smccc_probe.h"
#include "val_common/val_smccc.h"
#include "val_log.h"

#define SMCCC_SME_TEST_NAME "smccc_sme"
#define SMCCC_VERSION_FID   UINT64_C(0x80000000)
#define SMCCC_VERSION_MAJOR_SHIFT 16U
#define SMCCC_VERSION_MINOR_MASK  UINT64_C(0xffff)
#define SMCCC_SME_PFR1_SHIFT 24U
#define SMCCC_SME_PFR1_MASK  UINT64_C(0xf)
#define SMCCC_SME_MIN_MINOR  UINT64_C(4)

#define SMCCC_SME_STEP_VERSION UINT64_C(1)
#define SMCCC_SME_STEP_MINOR   UINT64_C(2)

/**
 *   @brief    - Initialize an SMCCC SME test result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void smccc_sme_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record an SMCCC SME test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void.
 **/
static void smccc_sme_fail(firme_rule_res_t *out, uint64_t fail_step,
                           uint64_t last_x0, uint64_t last_x1)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate SMCCC >= 1.4 when FEAT_SME is implemented.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
static int smccc_sme_run(firme_rule_res_t *out)
{
    smccc_res_t res = {0};
    uint64_t pfr1 = read_id_aa64pfr1_el1();
    uint64_t sme = (pfr1 >> SMCCC_SME_PFR1_SHIFT) & SMCCC_SME_PFR1_MASK;
    uint64_t major = 0;
    uint64_t minor = 0;

    smccc_sme_init_result(out);

    /* Step 1: If FEAT_SME is absent, this conditional rule is not applicable. */
    if (sme == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        LOG_WARN(NULL, "SME absent: state-preservation check not applicable\n");
        return VAL_SUCCESS;
    }

    /* Step 2: Query SMCCC version and require at least 1.4 for SME context rules. */
    val_smccc_smc_call(SMCCC_VERSION_FID, 0, 0, 0, 0, 0, 0, 0, &res);
    if ((res.x0 & UINT64_C(0x80000000)) != 0ULL) {
        LOG_ERROR(NULL, "%s: SMCCC_VERSION failed x0=%llx\n",
                  SMCCC_SME_TEST_NAME, (unsigned long long)res.x0);
        smccc_sme_fail(out, SMCCC_SME_STEP_VERSION, res.x0, pfr1);
        return VAL_ERROR;
    }

    major = (uint32_t)res.x0 >> SMCCC_VERSION_MAJOR_SHIFT;
    minor = res.x0 & SMCCC_VERSION_MINOR_MASK;
    if ((major < 1ULL) || ((major == 1ULL) && (minor < SMCCC_SME_MIN_MINOR))) {
        LOG_ERROR(NULL, "%s: FEAT_SME present but SMCCC version=%llu.%llu\n",
                  SMCCC_SME_TEST_NAME, (unsigned long long)major,
                  (unsigned long long)minor);
        smccc_sme_fail(out, SMCCC_SME_STEP_MINOR, res.x0, pfr1);
        return VAL_ERROR;
    }

    /* Step 3: Exercise live ZA with both non-streaming and streaming vectors. */
    if (val_firme_smccc_check_state(VAL_SMCCC_STATE_ZA, out) != VAL_SUCCESS)
        return VAL_ERROR;

    if (val_firme_smccc_check_state(VAL_SMCCC_STATE_ZA | VAL_SMCCC_PROBE_STREAMING,
                                    out) != VAL_SUCCESS)
        return VAL_ERROR;

    /* Step 4: The SVE absence hint never permits discarding live ZA or SIMD/FP. */
    if (val_firme_smccc_check_state(VAL_SMCCC_STATE_ZA | VAL_SMCCC_PROBE_HINT, out) !=
        VAL_SUCCESS)
        return VAL_ERROR;

    if (val_firme_smccc_check_state(VAL_SMCCC_STATE_ZA | VAL_SMCCC_PROBE_STREAMING |
                                    VAL_SMCCC_PROBE_HINT, out) != VAL_SUCCESS)
        return VAL_ERROR;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - SMCCC SME rule body for NS caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_smccc_sme_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_sme_run(out);
}

/**
 *   @brief    - SMCCC SME rule body for Secure caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_smccc_sme_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_sme_run(out);
}

/**
 *   @brief    - SMCCC SME rule body for Realm caller execution.
 *   @param    - base_pa : Unused for base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_base_smccc_sme_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smccc_sme_run(out);
}
