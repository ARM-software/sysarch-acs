/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_pcie.h"
#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define IDE_SECURE_CALLER_TEST_NAME "ide_secure_caller_not_supported"

#define IDE_SECURE_CALLER_STEP_PROG UINT64_C(1)
#define IDE_SECURE_CALLER_STEP_GO   UINT64_C(2)
#define IDE_SECURE_CALLER_STEP_STOP UINT64_C(3)
#define IDE_SECURE_CALLER_STEP_POLL UINT64_C(4)
#define IDE_SECURE_CALLER_HANDLE    UINT64_C(0x4944455345430001)

/**
 *   @brief    - Initialize a rule result for the Secure caller IDE test.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_secure_caller_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a Secure caller IDE failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - res       : Raw FIRME result.
 *   @return   - void
 **/
static void ide_secure_caller_fail(firme_rule_res_t *out, uint64_t fail_step,
                                   const firme_res_t *res)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_SECURE_CALLER_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)(res ? res->x0 : 0ULL),
              (unsigned long long)(res ? res->x1 : 0ULL));

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = res ? res->x0 : 0ULL;
        out->last_x1 = res ? res->x1 : 0ULL;
    }
}

/**
 *   @brief    - Expect NOT_SUPPORTED from one Secure IDE ABI invocation.
 *   @param    - res       : Raw FIRME result.
 *   @param    - fail_step : Failing step to record.
 *   @param    - out       : Optional result record.
 *   @return   - VAL_TRUE on expected status, otherwise VAL_FALSE.
 **/
static uint32_t ide_secure_caller_expect_not_supported(const firme_res_t *res,
                                                       uint64_t fail_step,
                                                       firme_rule_res_t *out)
{
    if (val_firme_status_to_i64(res->x0) != (int64_t)FIRME_NOT_SUPPORTED) {
        ide_secure_caller_fail(out, fail_step, res);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate IDE key management ABIs are inaccessible from Secure.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
 **/
int firme_ide_secure_caller_not_supported_run_secure(uint64_t base_pa,
                                                     firme_rule_res_t *out)
{
    val_firme_ide_prog_args_t prog_args = {0};
    firme_res_t res = {0};
    uint64_t ecam_address = val_pcie_ecam_base();
    uint64_t keyset_id = val_firme_ide_default_keyset_id();
    uint64_t flags = val_firme_ide_flags(VAL_FIRME_IDE_TYPE_SELECTIVE);

    (void)base_pa;
    ide_secure_caller_init_result(out);

    /* Step 1: Secure caller must not be able to program IDE keys. */
    val_firme_ide_prog_args_init(&prog_args, ecam_address, VAL_FIRME_IDE_TYPE_SELECTIVE,
                                 keyset_id, IDE_SECURE_CALLER_HANDLE);
    val_firme_ide_keyset_prog(&prog_args, &res);
    if (ide_secure_caller_expect_not_supported(&res, IDE_SECURE_CALLER_STEP_PROG,
                                               out) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 2: Secure caller must not be able to start IDE key use. */
    val_firme_ide_keyset_go(ecam_address, flags, keyset_id, IDE_SECURE_CALLER_HANDLE,
                            &res);
    if (ide_secure_caller_expect_not_supported(&res, IDE_SECURE_CALLER_STEP_GO,
                                               out) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 3: Secure caller must not be able to stop IDE key use. */
    val_firme_ide_keyset_stop(ecam_address, flags, keyset_id, IDE_SECURE_CALLER_HANDLE,
                              &res);
    if (ide_secure_caller_expect_not_supported(&res, IDE_SECURE_CALLER_STEP_STOP,
                                               out) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 4: Secure caller must not be able to poll IDE operation status. */
    val_firme_ide_keyset_poll(ecam_address, val_firme_ide_poll_flags(VAL_FALSE),
                              keyset_id, &res);
    if (ide_secure_caller_expect_not_supported(&res, IDE_SECURE_CALLER_STEP_POLL,
                                               out) == VAL_FALSE)
        return VAL_SUCCESS;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}
