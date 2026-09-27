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
#include "val_firme_ide_observation.h"
#include "val_log.h"

#define IDE_KEYSET_STOP_TEST_NAME "ide_keyset_stop_status"

#define IDE_KEYSET_STOP_STEP_UNSUPPORTED  UINT64_C(1)
#define IDE_KEYSET_STOP_STEP_ABI_BIT      UINT64_C(2)
#define IDE_KEYSET_STOP_STEP_INVALID_ECAM UINT64_C(3)
#define IDE_KEYSET_STOP_STEP_UNUSED_TYPE  UINT64_C(4)
#define IDE_KEYSET_STOP_STEP_PROGRAM      UINT64_C(5)
#define IDE_KEYSET_STOP_STEP_OPERATION    UINT64_C(6)
#define IDE_KEYSET_STOP_STEP_POLL         UINT64_C(7)
#define IDE_KEYSET_STOP_HANDLE_PROG       UINT64_C(0x49444553544f5001)
#define IDE_KEYSET_STOP_HANDLE_OP         UINT64_C(0x49444553544f5002)

/**
 *   @brief    - Initialize a rule result for IDE KEYSET_STOP status.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_keyset_stop_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a IDE KEYSET_STOP status failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_keyset_stop_fail(firme_rule_res_t *out, uint64_t fail_step,
                          uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_KEYSET_STOP_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}


/**
 *   @brief    - Check that the current Realm instance can run this IDE ABI test.
 *   @param    - abi_bit : Required IDE feature-register ABI bit.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_TRUE when the ABI can be tested, otherwise VAL_FALSE.
 **/
static uint32_t ide_keyset_stop_check_available(uint64_t abi_bit, firme_rule_res_t *out)
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
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_FALSE;
    }

    val_firme_service_features(FIRME_SERVICE_ID_IDE_KEY_MGMT,
                               FIRME_IDE_FEATURE_REG0_INDEX, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_ABI_BIT, res.x0, res.x1);
        return VAL_FALSE;
    }
    feature_reg0 = res.x1;
    if ((feature_reg0 & abi_bit) == 0ULL) {
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_ABI_BIT, res.x0, feature_reg0);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate FIRME_IDE_KEYSET_STOP status behavior from Realm.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Required result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, VAL_ERROR for a failed check.
 **/
int firme_ide_keyset_stop_status_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_ide_test_state_t state = firme_ide_test_state();
    val_firme_ide_prog_args_t prog_args = {0};
    firme_res_t res = {0};
    uint64_t ecam_address = state.ecam_address;
    uint64_t keyset_id = state.keyset_id;
    uint64_t flags = val_firme_ide_flags(VAL_FIRME_IDE_TYPE_SELECTIVE);
    int64_t status = 0;

    (void)base_pa;
    if (!out)
        return VAL_ERROR;
    ide_keyset_stop_init_result(out);

    /* Step 1: Skip or fail early based on IDE service and ABI availability. */
    if (ide_keyset_stop_check_available(FIRME_IDE_FEATURE_REG0_STOP_BIT, out) == VAL_FALSE)
        return out->status == ACS_STATUS_SKIP ? VAL_SUCCESS : VAL_ERROR;

    /* Step 2: Invalid ECAM base must be rejected. */
    val_firme_ide_keyset_stop(val_pcie_invalid_ecam_base(), flags, keyset_id,
                  IDE_KEYSET_STOP_HANDLE_OP, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_INVALID_ECAM, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: Unused IDE type encoding must be rejected. */
    val_firme_ide_keyset_stop(ecam_address, val_firme_ide_flags(VAL_FIRME_IDE_TYPE_UNUSED_TEST),
                  keyset_id, IDE_KEYSET_STOP_HANDLE_OP, &res);
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_UNUSED_TYPE, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 4: Program a key before testing successful STOP. */
    val_firme_ide_prog_args_init(&prog_args, ecam_address, VAL_FIRME_IDE_TYPE_SELECTIVE,
                                 keyset_id, IDE_KEYSET_STOP_HANDLE_PROG);
    val_firme_ide_keyset_prog_do_retry(&prog_args, &res);
    firme_ide_test_track(&state, &res, IDE_KEYSET_STOP_HANDLE_PROG);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_INCOMPLETE) {
        firme_ide_test_complete(&state, &res);
        if ((val_firme_status_to_i64(res.x0) == (int64_t)FIRME_SUCCESS) &&
            (res.x1 != IDE_KEYSET_STOP_HANDLE_PROG)) {
            ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_PROGRAM, res.x0, res.x1);
            goto cleanup;
        }
    }
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_PROGRAM, res.x0, res.x1);
        goto cleanup;
    }

    /* Step 5: Valid STOP must complete or enter non-blocking completion. */
    val_firme_ide_keyset_stop_do_retry(ecam_address, flags, keyset_id,
                                         IDE_KEYSET_STOP_HANDLE_OP, &res);
    firme_ide_test_track(&state, &res, IDE_KEYSET_STOP_HANDLE_OP);
    status = val_firme_status_to_i64(res.x0);
    if (val_firme_ide_status_can_complete(status) == VAL_FALSE) {
        ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_OPERATION, res.x0, res.x1);
        goto cleanup;
    }
    if (status == (int64_t)FIRME_INCOMPLETE) {
        firme_ide_test_complete(&state, &res);
        if ((val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) ||
            (res.x1 != IDE_KEYSET_STOP_HANDLE_OP)) {
            ide_keyset_stop_fail(out, IDE_KEYSET_STOP_STEP_POLL, res.x0, res.x1);
            goto cleanup;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    return firme_ide_test_cleanup(&state, out);
}

/**
 * @brief Require observable STOP effects after the Realm ABI status checks.
 * @param res_ns Unused Normal-world result.
 * @param res_secure Unused Secure-world result.
 * @param res_realm Realm result already combined by the dispatcher.
 * @param overall Final rule outcome including functional evidence.
 * @return VAL_ERROR on failed effects, VAL_SUCCESS for PASS or explicit SKIP.
 */
int firme_ide_keyset_stop_post(const firme_acs_call_res_t *res_ns,
                               const firme_acs_call_res_t *res_secure,
                               const firme_acs_call_res_t *res_realm,
                               firme_acs_call_res_t *overall)
{
    (void)res_ns;
    (void)res_secure;
    (void)res_realm;
    return val_firme_ide_check_effect(VAL_IDE_LINK_STOP, FIRME_ACS_WORLD_REALM,
                                      overall);
}
