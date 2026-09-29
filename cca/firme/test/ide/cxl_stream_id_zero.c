/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cxl_test_common.h"
#include "val_pcie.h"
#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_log.h"

#define IDE_CXL_STREAM_TEST_NAME "ide_cxl_stream_id_zero"

#define IDE_CXL_STREAM_STEP_UNSUPPORTED UINT64_C(1)
#define IDE_CXL_STREAM_STEP_BASELINE    UINT64_C(2)
#define IDE_CXL_STREAM_STEP_STREAM_ID   UINT64_C(3)
#define IDE_CXL_STREAM_HANDLE_BASELINE  UINT64_C(0x49444543584c5300)
#define IDE_CXL_STREAM_HANDLE_FIELD     UINT64_C(0x49444543584c5301)

#define IDE_CXL_STREAM_STEP_ACQUIRE UINT64_C(6)
#define IDE_CXL_STREAM_STEP_CLEANUP UINT64_C(4)
#define IDE_CXL_STREAM_HANDLE_STOP UINT64_C(0x49444543584c5354)

/**
 *   @brief    - Initialize a rule result for CXL Stream ID handling.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_cxl_stream_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a CXL Stream ID handling failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_cxl_stream_fail(firme_rule_res_t *out, uint64_t fail_step,
                                uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_CXL_STREAM_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Validate CXL Stream ID rejection without leaking the valid baseline.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP with cleanup, otherwise VAL_ERROR.
 **/
int firme_ide_cxl_stream_id_zero_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_rule_res_t local_result = {0};
    firme_ide_cxl_state_t state = {0};
    firme_res_t res = {0};
    firme_res_t cleanup_res = {0};
    val_firme_ide_prog_args_t args = {0};
    uint64_t keyset_id;
    uint32_t stream_id;
    int64_t status;
    int completion;

    (void)base_pa;
    if (!out)
        out = &local_result;
    ide_cxl_stream_init_result(out);

    /* Step 1: Distinguish an absent IDE service from an invalid version response. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        out->status = ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (status < 0) {
        ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Reserve the valid CXL keyset; a quarantined path must not be reused. */
    keyset_id = val_firme_ide_cxl_keyset_id(0U, 0U, VAL_FIRME_IDE_CXL_SUBSTREAM_ID);
    if (firme_ide_cxl_begin(&state, keyset_id) != VAL_SUCCESS) {
        ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_ACQUIRE, keyset_id, 0);
        return VAL_ERROR;
    }

    /* Step 3: Establish the valid baseline and retain every accepted request. */
    completion = firme_ide_cxl_prog(&state, IDE_CXL_STREAM_HANDLE_BASELINE, &res);
    status = val_firme_status_to_i64(res.x0);
    if (!state.programmed && status == (int64_t)FIRME_NOT_SUPPORTED) {
        out->status = ACS_STATUS_SKIP;
        goto cleanup;
    }

    if (completion != VAL_SUCCESS) {
        ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_BASELINE, res.x0, res.x1);
        goto cleanup;
    }

    /* Step 4: Reject every nonzero Stream ID without polling malformed input. */
    for (stream_id = 1U; stream_id <= VAL_FIRME_IDE_KEYSET_STREAM_MASK; stream_id++) {
        keyset_id = val_firme_ide_cxl_keyset_id(0U, stream_id,
                                              VAL_FIRME_IDE_CXL_SUBSTREAM_ID);
        val_firme_ide_prog_args_init(&args, state.ecam_address,
                                    VAL_FIRME_IDE_TYPE_CXL_CACHEMEM, keyset_id,
                                    IDE_CXL_STREAM_HANDLE_FIELD);
        val_firme_ide_keyset_prog_do_retry(&args, &res);
        firme_ide_cxl_rejected(&state, &res);
        if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS) {
            ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_STREAM_ID, res.x0, keyset_id);
            goto cleanup;
        }
    }

    out->status = ACS_STATUS_PASS;

cleanup:
    /* Step 5: Drain and stop accepted work, preserving the original failure. */
    completion = firme_ide_cxl_finish(&state, IDE_CXL_STREAM_HANDLE_STOP,
                                     &cleanup_res);
    if (completion != VAL_SUCCESS) {
        if (out->status != ACS_STATUS_FAIL)
            ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_CLEANUP,
                           cleanup_res.x0, cleanup_res.x1);
        out->status = ACS_STATUS_FAIL;
    }

    return out->status == ACS_STATUS_FAIL ? VAL_ERROR : VAL_SUCCESS;
}
