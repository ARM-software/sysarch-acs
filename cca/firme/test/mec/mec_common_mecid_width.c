/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mec.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define MEC_COMMON_WIDTH_TEST_NAME "mec_common_mecid_width"

#define MEC_COMMON_WIDTH_STEP_RANGE        UINT64_C(1)
#define MEC_COMMON_WIDTH_STEP_DUT_WIDTH    UINT64_C(2)
#define MEC_COMMON_WIDTH_STEP_UNSUPPORTED  UINT64_C(3)
#define MEC_COMMON_WIDTH_STEP_INVALID_ID   UINT64_C(4)
#define MEC_COMMON_WIDTH_STEP_VALID_ID     UINT64_C(5)
#define MEC_COMMON_WIDTH_STEP_EXPECTED     UINT64_C(6)
#define MEC_COMMON_WIDTH_STEP_MISMATCH     UINT64_C(7)

/**
 *   @brief    - Initialize a rule result record for the common MECID width test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void mec_common_width_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a common MECID width test failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void mec_common_width_fail(firme_rule_res_t *out, uint64_t fail_step,
                                  uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              MEC_COMMON_WIDTH_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate that MEC_REFRESH enforces the common MECID width.
 *   @param    - out : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
static int mec_common_width_run(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    firme_res_t width_res = {0};
    uint32_t advertised_width = 0U;
    uint32_t expected_width = 0U;
    uint64_t valid_mecid = 0ULL;
    uint64_t valid_params = 0ULL;
    uint64_t invalid_mecid = 0ULL;
    uint64_t invalid_params = 0ULL;

    mec_common_width_init_result(out);

    /* Step 1: If MECID management is absent, this platform cannot run the rule. */
    val_firme_service_version(FIRME_SERVICE_ID_MECID_MGMT, &res);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_NOT_SUPPORTED) {
        LOG_DEBUG(NULL, "%s: MECID management service not implemented",
                  MEC_COMMON_WIDTH_TEST_NAME);
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (val_firme_status_to_i64(res.x0) < 0) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_SUCCESS;
    }

    /* Step 2: Derive the ACS expected common width from discovered requesters. */
    if (val_firme_mec_expected_common_mecid_width(&expected_width) != VAL_SUCCESS) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_EXPECTED, 0, 0);
        return VAL_SUCCESS;
    }

    /* Step 3: Read the DUT advertised common width from MEC feature register 1. */
    if (val_firme_mec_advertised_common_mecid_width(&advertised_width, &width_res) !=
        VAL_SUCCESS) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_DUT_WIDTH, width_res.x0,
                              width_res.x1);
        return VAL_SUCCESS;
    }
    if ((expected_width < VAL_FIRME_MECID_WIDTH_MIN) ||
        (expected_width > VAL_FIRME_MECID_WIDTH_MAX) ||
        (expected_width > VAL_FIRME_MECID_U64_SHIFT_MAX) ||
        (advertised_width < VAL_FIRME_MECID_WIDTH_MIN) ||
        (advertised_width > VAL_FIRME_MECID_WIDTH_MAX) ||
        (advertised_width > VAL_FIRME_MECID_U64_SHIFT_MAX)) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_RANGE,
                              (uint64_t)expected_width, (uint64_t)advertised_width);
        return VAL_SUCCESS;
    }

    /* Step 4: The DUT advertised width must match the ACS-derived common width. */
    if (advertised_width != expected_width) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_MISMATCH,
                              (uint64_t)expected_width, (uint64_t)advertised_width);
        return VAL_SUCCESS;
    }

    /* Step 5: The largest MECID within the common width must be accepted. */
    valid_mecid = (UINT64_C(1) << expected_width) - 1ULL;
    valid_params = val_firme_mec_params(valid_mecid, FIRME_MEC_REFRESH_REASON_CREATE);
    val_firme_mec_refresh_do_retry(valid_params, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_VALID_ID, res.x0, valid_mecid);
        return VAL_SUCCESS;
    }

    /* Step 6: The first MECID outside the common width must be rejected when encodable. */
    if (expected_width < VAL_FIRME_MECID_WIDTH_MAX) {
        invalid_mecid = UINT64_C(1) << expected_width;
        invalid_params = val_firme_mec_params(invalid_mecid,
                                              FIRME_MEC_REFRESH_REASON_CREATE);
        val_firme_mec_refresh(invalid_params, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
            mec_common_width_fail(out, MEC_COMMON_WIDTH_STEP_INVALID_ID, res.x0,
                                  invalid_mecid);
            return VAL_SUCCESS;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Common MECID width rule body for Realm caller execution.
 *   @param    - base_pa : Unused for MEC tests.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
 **/
int firme_mec_common_mecid_width_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return mec_common_width_run(out);
}
