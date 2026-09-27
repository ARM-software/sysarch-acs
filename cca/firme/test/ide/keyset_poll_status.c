/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "ide_test_common.h"
#include "val_pcie.h"
#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define IDE_KEYSET_POLL_TEST_NAME "ide_keyset_poll_status"

#define IDE_KEYSET_POLL_STEP_UNSUPPORTED      UINT64_C(1)
#define IDE_KEYSET_POLL_STEP_POLL_BIT         UINT64_C(2)
#define IDE_KEYSET_POLL_STEP_INVALID_ECAM     UINT64_C(3)
#define IDE_KEYSET_POLL_STEP_DENIED        UINT64_C(4)
#define IDE_KEYSET_POLL_STEP_PROGRAM_EXACT    UINT64_C(5)
#define IDE_KEYSET_POLL_STEP_EXACT_SUCCESS    UINT64_C(6)
#define IDE_KEYSET_POLL_STEP_PROGRAM_ANY      UINT64_C(7)
#define IDE_KEYSET_POLL_STEP_ANY_SUCCESS      UINT64_C(8)
#define IDE_KEYSET_POLL_HANDLE_EXACT          UINT64_C(0x494445504f4c4c01)
#define IDE_KEYSET_POLL_HANDLE_ANY            UINT64_C(0x494445504f4c4c02)

/**
 *   @brief    - Initialize a rule result for IDE KEYSET_POLL status.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_keyset_poll_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record an IDE KEYSET_POLL status failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_keyset_poll_fail(firme_rule_res_t *out, uint64_t fail_step,
                                 uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_KEYSET_POLL_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Check that the Realm instance advertises FIRME_IDE_KEYSET_POLL.
 *   @param    - out : Optional result record.
 *   @return   - VAL_TRUE when POLL can be tested, otherwise VAL_FALSE.
 **/
static uint32_t ide_keyset_poll_check_available(firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t feature_reg0 = 0ULL;
    int64_t status = 0;

    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_FALSE;
    }
    if (status < 0) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_FALSE;
    }

    val_firme_service_features(FIRME_SERVICE_ID_IDE_KEY_MGMT,
                               FIRME_IDE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_POLL_BIT, res.x0, res.x1);
        return VAL_FALSE;
    }

    feature_reg0 = res.x1;
    if ((feature_reg0 & FIRME_IDE_FEATURE_REG0_POLL_BIT) == 0ULL) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_POLL_BIT, res.x0,
                             feature_reg0);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Start a PROG operation that is expected to become pending.
 *   @param    - state     : Test-owned platform keyset.
 *   @param    - handle    : Caller handle for non-blocking tracking.
 *   @param    - step      : Failure step to report.
 *   @param    - out       : Optional result record.
 *   @return   - VAL_TRUE when PROG returns INCOMPLETE, otherwise VAL_FALSE.
 **/
static uint32_t ide_keyset_poll_start_pending(firme_ide_test_state_t *state, uint64_t handle,
                                              uint64_t step, firme_rule_res_t *out)
{
    val_firme_ide_prog_args_t prog_args = {0};
    firme_res_t res = {0};

    val_firme_ide_prog_args_init(&prog_args, state->ecam_address,
                                 VAL_FIRME_IDE_TYPE_SELECTIVE, state->keyset_id,
                                 handle);
    val_firme_ide_keyset_prog_do_retry(&prog_args, &res);
    firme_ide_test_track(state, &res, handle);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_SUCCESS) {
        LOG_WARN(NULL, "%s: synchronous PROG provides no pending POLL scenario",
                 IDE_KEYSET_POLL_TEST_NAME);
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_FALSE;
    }
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INCOMPLETE) {
        ide_keyset_poll_fail(out, step, res.x0, res.x1);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate FIRME_IDE_KEYSET_POLL status behavior from Realm.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_keyset_poll_status_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_ide_test_state_t state = firme_ide_test_state();
    firme_res_t res = {0};
    uint64_t ecam_address = state.ecam_address;
    uint64_t exact_keyset_id = state.keyset_id;
    uint64_t any_keyset_id = state.keyset_id;

    (void)base_pa;
    if (!out)
        return VAL_ERROR;
    ide_keyset_poll_init_result(out);

    /* Step 1: Skip or fail early based on IDE service and POLL availability. */
    if (ide_keyset_poll_check_available(out) == VAL_FALSE)
        return out->status == ACS_STATUS_SKIP ? VAL_SUCCESS : VAL_ERROR;

    /* Step 2: Invalid ECAM base must be rejected. */
    val_firme_ide_keyset_poll(val_pcie_invalid_ecam_base(),
                              val_firme_ide_poll_flags(VAL_FALSE),
                              exact_keyset_id, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_INVALID_ECAM,
                             res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: Polling a keyset with no pending response must return DENIED. */
    val_firme_ide_keyset_poll(ecam_address, val_firme_ide_poll_flags(VAL_FALSE),
                              exact_keyset_id, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_DENIED) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_DENIED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 4: Create one pending operation and poll its exact keyset. */
    if (ide_keyset_poll_start_pending(&state, IDE_KEYSET_POLL_HANDLE_EXACT,
                                      IDE_KEYSET_POLL_STEP_PROGRAM_EXACT,
                                      out) == VAL_FALSE)
        goto cleanup;

    val_firme_ide_keyset_poll_do_retry(ecam_address,
                                       val_firme_ide_poll_flags(VAL_FALSE),
                                       exact_keyset_id, &res);
    if ((firme_ide_test_polled(&state, &res) != VAL_SUCCESS) ||
        (res.x1 != IDE_KEYSET_POLL_HANDLE_EXACT)) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_EXACT_SUCCESS,
                             res.x0, res.x1);
        goto cleanup;
    }

    /* Step 5: Reuse the same owned keyset after consuming the exact response. */
    if (ide_keyset_poll_start_pending(&state, IDE_KEYSET_POLL_HANDLE_ANY,
                                      IDE_KEYSET_POLL_STEP_PROGRAM_ANY,
                                      out) == VAL_FALSE)
        goto cleanup;

    val_firme_ide_keyset_poll_do_retry(ecam_address,
                                       val_firme_ide_poll_flags(VAL_TRUE),
                                       0ULL, &res);
    if ((firme_ide_test_polled(&state, &res) != VAL_SUCCESS) ||
        (res.x1 != IDE_KEYSET_POLL_HANDLE_ANY) ||
        ((res.x2 & VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) != any_keyset_id)) {
        ide_keyset_poll_fail(out, IDE_KEYSET_POLL_STEP_ANY_SUCCESS,
                             res.x0, res.x1);
        goto cleanup;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    return firme_ide_test_cleanup(&state, out);
}
