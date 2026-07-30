/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_rules.h"
#include "val_firme_smccc_probe.h"

#define SMC64_CALLING_STEP_FID_ENCODING UINT64_C(1)
#define SMC64_CALLING_STEP_RUNTIME      UINT64_C(2)

/**
 *   @brief    - Initialize an SMC64 calling-convention result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void smc64_calling_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0U;
        out->last_x0 = 0U;
        out->last_x1 = 0U;
    }
}

/**
 *   @brief    - Validate all ALP3 FIRME ABIs use and accept SMC64 calls.
 *   @param    - out : Optional result record.
 *   @return   - VAL_SUCCESS with the architectural outcome stored in out.
 **/
static int smc64_calling_run(firme_rule_res_t *out)
{
    smc64_calling_init_result(out);

    /* Step 1: Every ALP3 FIRME ABI FID must carry the SMC64 convention bit. */
    if (val_firme_smccc_check_fid_encodings(out) != VAL_SUCCESS) {
        if (out && (out->fail_step == 0U))
            out->fail_step = SMC64_CALLING_STEP_FID_ENCODING;
        return VAL_SUCCESS;
    }

    /* Step 2: Exercise every ABI advertised at this FIRME instance as SMC64. */
    if (val_firme_smccc_check_state(0U, out) != VAL_SUCCESS) {
        if (out && (out->fail_step == 0U))
            out->fail_step = SMC64_CALLING_STEP_RUNTIME;
        return VAL_SUCCESS;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - SMC64 calling-convention rule body for NS caller execution.
 *   @param    - base_pa : Unused for Base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_smc64_calling_convention_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return smc64_calling_run(out);
}

/**
 *   @brief    - SMC64 calling-convention rule body for Secure caller execution.
 *   @param    - base_pa : Unused for Base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_smc64_calling_convention_run_secure(uint64_t base_pa,
                                                   firme_rule_res_t *out)
{
    (void)base_pa;
    return smc64_calling_run(out);
}

/**
 *   @brief    - SMC64 calling-convention rule body for Realm caller execution.
 *   @param    - base_pa : Unused for Base service tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS.
 **/
int firme_base_smc64_calling_convention_run_realm(uint64_t base_pa,
                                                  firme_rule_res_t *out)
{
    (void)base_pa;
    return smc64_calling_run(out);
}
