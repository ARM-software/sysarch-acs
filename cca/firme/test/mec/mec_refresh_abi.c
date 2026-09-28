/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mec.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define MEC_REFRESH_ABI_TEST_NAME "mec_refresh_abi"

#define MEC_REFRESH_ABI_STEP_NS_DENIED       UINT64_C(1)
#define MEC_REFRESH_ABI_STEP_SECURE_DENIED   UINT64_C(2)
#define MEC_REFRESH_ABI_STEP_UNSUPPORTED     UINT64_C(3)
#define MEC_REFRESH_ABI_STEP_WIDTH_DISCOVERY UINT64_C(4)
#define MEC_REFRESH_ABI_STEP_WIDTH_RANGE     UINT64_C(5)
#define MEC_REFRESH_ABI_STEP_CREATE          UINT64_C(6)
#define MEC_REFRESH_ABI_STEP_DESTROY         UINT64_C(7)
#define MEC_REFRESH_ABI_STEP_INVALID_MECID   UINT64_C(8)

/**
 *   @brief    - Initialize a rule result record for the MEC refresh ABI test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void mec_refresh_abi_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a MEC refresh ABI test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void mec_refresh_abi_fail(firme_rule_res_t *out, uint64_t fail_step,
                                 uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              MEC_REFRESH_ABI_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Check that MEC_REFRESH is not supported from a disallowed caller.
 *   @param    - fail_step : Failing test step to record.
 *   @param    - out       : Optional result record.
 *   @return   - 1 if the caller is rejected, 0 otherwise.
 **/
static uint32_t mec_refresh_abi_check_disallowed_caller(uint64_t fail_step,
                                                        firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t params = val_firme_mec_params(UINT64_C(0), FIRME_MEC_REFRESH_REASON_CREATE);

    val_firme_mec_refresh(params, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_NOT_SUPPORTED) {
        mec_refresh_abi_fail(out, fail_step, res.x0, res.x1);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate MEC_REFRESH rejection from NS caller.
 *   @param    - base_pa : Unused for MEC tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 **/
int firme_mec_refresh_abi_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    if (out == NULL)
        return VAL_ERROR;

    mec_refresh_abi_init_result(out);

    /* Step 1: NS FIRME instance must not support MEC_REFRESH. */
    if (mec_refresh_abi_check_disallowed_caller(MEC_REFRESH_ABI_STEP_NS_DENIED, out) ==
        VAL_FALSE)
        return VAL_ERROR;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate MEC_REFRESH rejection from Secure caller.
 *   @param    - base_pa : Unused for MEC tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS on PASS, otherwise VAL_ERROR.
 **/
int firme_mec_refresh_abi_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    if (out == NULL)
        return VAL_ERROR;

    mec_refresh_abi_init_result(out);

    /* Step 1: Secure FIRME instance must not support MEC_REFRESH. */
    if (mec_refresh_abi_check_disallowed_caller(MEC_REFRESH_ABI_STEP_SECURE_DENIED, out) ==
        VAL_FALSE)
        return VAL_ERROR;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate MEC_REFRESH behavior from Realm caller.
 *   @param    - base_pa : Unused for MEC tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS on PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_mec_refresh_abi_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    firme_res_t width_res = {0};
    uint32_t width = 0U;
    uint64_t valid_mecid = UINT64_C(1);
    uint64_t invalid_mecid = 0ULL;
    uint64_t params = 0ULL;

    (void)base_pa;
    if (out == NULL)
        return VAL_ERROR;

    mec_refresh_abi_init_result(out);

    /* Step 1: If MECID management is absent, this platform cannot run the rule. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &res);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_NOT_SUPPORTED) {
        LOG_DEBUG(NULL, "%s: MECID management service not implemented",
                  MEC_REFRESH_ABI_TEST_NAME);
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (val_firme_status_to_i64(res.x0) < 0) {
        mec_refresh_abi_fail(out, MEC_REFRESH_ABI_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Read the DUT advertised common width for ABI parameter checks. */
    if (val_firme_mec_advertised_common_mecid_width(&width, &width_res) !=
        VAL_SUCCESS) {
        mec_refresh_abi_fail(out, MEC_REFRESH_ABI_STEP_WIDTH_DISCOVERY,
                             width_res.x0, width_res.x1);
        return VAL_ERROR;
    }
    if ((width < VAL_FIRME_MECID_WIDTH_MIN) || (width > VAL_FIRME_MECID_WIDTH_MAX) ||
        (width > VAL_FIRME_MECID_U64_SHIFT_MAX)) {
        mec_refresh_abi_fail(out, MEC_REFRESH_ABI_STEP_WIDTH_RANGE, (uint64_t)width, 0);
        return VAL_ERROR;
    }

    /* Step 3: Valid Realm-creation refresh request must complete successfully. */
    params = val_firme_mec_params(valid_mecid, FIRME_MEC_REFRESH_REASON_CREATE);
    val_firme_mec_refresh_do_retry(params, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        mec_refresh_abi_fail(out, MEC_REFRESH_ABI_STEP_CREATE, res.x0, params);
        return VAL_ERROR;
    }

    /* Step 4: Valid Realm-destruction refresh request must complete successfully. */
    params = val_firme_mec_params(valid_mecid, FIRME_MEC_REFRESH_REASON_DESTROY);
    val_firme_mec_refresh_do_retry(params, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        mec_refresh_abi_fail(out, MEC_REFRESH_ABI_STEP_DESTROY, res.x0, params);
        return VAL_ERROR;
    }

    /* Step 5: MECID outside the common width must be rejected when encodable. */
    if (width < VAL_FIRME_MECID_WIDTH_MAX) {
        invalid_mecid = UINT64_C(1) << width;
        params = val_firme_mec_params(invalid_mecid, FIRME_MEC_REFRESH_REASON_CREATE);
        val_firme_mec_refresh(params, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
            mec_refresh_abi_fail(out, MEC_REFRESH_ABI_STEP_INVALID_MECID, res.x0,
                                 invalid_mecid);
            return VAL_ERROR;
        }
    }

    /* Step 6: No additional malformed MEC params encoding is defined for v1.0. */

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}
