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

#define IDE_CXL_STREAM_TEST_NAME "ide_cxl_stream_id_zero"

#define IDE_CXL_STREAM_STEP_UNSUPPORTED UINT64_C(1)
#define IDE_CXL_STREAM_STEP_BASELINE    UINT64_C(2)
#define IDE_CXL_STREAM_STEP_STREAM_ID   UINT64_C(3)
#define IDE_CXL_STREAM_HANDLE_BASELINE  UINT64_C(0x49444543584c5300)
#define IDE_CXL_STREAM_HANDLE_FIELD     UINT64_C(0x49444543584c5301)
#define IDE_CXL_STREAM_NONZERO_ID       1U

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
 *   @brief    - Invoke one CXL.cachemem PROG request.
 *   @param    - stream_id : Stream ID field value.
 *   @param    - handle    : Caller handle.
 *   @param    - out_res   : Raw FIRME result.
 *   @return   - VAL_ERROR for a mismatched POLL handle, otherwise VAL_SUCCESS.
 **/
static int ide_cxl_stream_prog(uint32_t stream_id, uint64_t handle,
                                firme_res_t *out_res)
{
    val_firme_ide_prog_args_t prog_args = {0};
    uint64_t keyset_id = val_firme_ide_cxl_keyset_id(0U, stream_id,
                                                     VAL_FIRME_IDE_CXL_SUBSTREAM_ID);

    val_firme_ide_prog_args_init(&prog_args, val_pcie_ecam_base(),
                                 VAL_FIRME_IDE_TYPE_CXL_CACHEMEM,
                                 keyset_id, handle);
    val_firme_ide_keyset_prog_do_retry(&prog_args, out_res);
    if (val_firme_status_to_i64(out_res->x0) == (int64_t)FIRME_INCOMPLETE) {
        val_firme_ide_keyset_poll_do_retry(val_pcie_ecam_base(),
                                           val_firme_ide_poll_flags(VAL_FALSE),
                                           keyset_id, out_res);
        if ((val_firme_status_to_i64(out_res->x0) == (int64_t)FIRME_SUCCESS) &&
            (out_res->x1 != handle))
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate the CXL.cachemem Stream ID field is zero.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_cxl_stream_id_zero_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    int64_t status = 0;
    int completion;

    (void)base_pa;
    ide_cxl_stream_init_result(out);

    /* Step 1: Skip if the current Realm FIRME instance does not implement IDE KM. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (status < 0) {
        ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Require a usable CXL.cachemem path in the platform setup. */
    completion = ide_cxl_stream_prog(0U, IDE_CXL_STREAM_HANDLE_BASELINE, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if ((completion != VAL_SUCCESS) || (status != (int64_t)FIRME_SUCCESS)) {
        ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_BASELINE, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 3: Non-zero CXL.cachemem Stream ID must be rejected. */
    completion = ide_cxl_stream_prog(IDE_CXL_STREAM_NONZERO_ID,
                                      IDE_CXL_STREAM_HANDLE_FIELD, &res);
    if ((completion != VAL_SUCCESS) ||
        (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_INVALID_PARAMETERS)) {
        ide_cxl_stream_fail(out, IDE_CXL_STREAM_STEP_STREAM_ID, res.x0, res.x1);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}
