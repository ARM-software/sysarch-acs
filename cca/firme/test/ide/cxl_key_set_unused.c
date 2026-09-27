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

#define IDE_CXL_KEY_SET_TEST_NAME "ide_cxl_key_set_unused"

#define IDE_CXL_KEY_SET_STEP_UNSUPPORTED UINT64_C(1)
#define IDE_CXL_KEY_SET_STEP_KEY_SET     UINT64_C(2)
#define IDE_CXL_KEY_SET_STEP_BASELINE    UINT64_C(3)
#define IDE_CXL_KEY_SET_STEP_GO          UINT64_C(4)
#define IDE_CXL_KEY_SET_STEP_STOP        UINT64_C(5)
#define IDE_CXL_KEY_SET_HANDLE_BASELINE  UINT64_C(0x49444543584c4b00)
#define IDE_CXL_KEY_SET_HANDLE_GO        UINT64_C(0x49444543584c4b01)
#define IDE_CXL_KEY_SET_HANDLE_STOP      UINT64_C(0x49444543584c4b02)

/**
 *   @brief    - Initialize a rule result for CXL Key Set handling.
 *   @param    - out : Optional result record.
 *   @return   - void
 **/
static void ide_cxl_key_set_init_result(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/**
 *   @brief    - Record a CXL Key Set handling failure.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failing step.
 *   @param    - last_x0   : Debug value 0.
 *   @param    - last_x1   : Debug value 1.
 *   @return   - void
 **/
static void ide_cxl_key_set_fail(firme_rule_res_t *out, uint64_t fail_step,
                                 uint64_t last_x0, uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: failed step=%llu x0=%llx x1=%llx",
              IDE_CXL_KEY_SET_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = last_x0;
        out->last_x1 = last_x1;
    }
}

/**
 *   @brief    - Invoke one CXL.cachemem PROG request.
 *   @param    - keyset_id : Encoded CXL.cachemem Keyset ID.
 *   @param    - handle    : Caller handle.
 *   @param    - out_res   : Raw FIRME result.
 *   @return   - VAL_ERROR for a mismatched POLL handle, otherwise VAL_SUCCESS.
 **/
static int ide_cxl_key_set_prog(uint64_t keyset_id, uint64_t handle,
                                 firme_res_t *out_res)
{
    val_firme_ide_prog_args_t prog_args = {0};

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
 *   @brief    - Validate the CXL.cachemem Key Set field is unused.
 *   @param    - base_pa : Unused for IDE tests.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 **/
int firme_ide_cxl_key_set_unused_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t keyset_id = 0;
    int64_t status = 0;
    int completion;

    (void)base_pa;
    ide_cxl_key_set_init_result(out);

    /* Step 1: Skip if the current Realm FIRME instance does not implement IDE KM. */
    val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if (status < 0) {
        ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_UNSUPPORTED, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 2: Keep the unused Key Set field zero in the production request. */
    keyset_id = val_firme_ide_cxl_keyset_id(0U, 0U,
                                            VAL_FIRME_IDE_CXL_SUBSTREAM_ID);
    if (((keyset_id >> VAL_FIRME_IDE_KEYSET_KEY_SET_SHIFT) &
         VAL_FIRME_IDE_KEYSET_KEY_SET_MASK) != 0ULL) {
        ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_KEY_SET, keyset_id, 0);
        return VAL_ERROR;
    }

    /* Step 3: Exercise the valid request when CXL.cachemem is available. */
    completion = ide_cxl_key_set_prog(keyset_id, IDE_CXL_KEY_SET_HANDLE_BASELINE, &res);
    status = val_firme_status_to_i64(res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }
    if ((completion != VAL_SUCCESS) || (status != (int64_t)FIRME_SUCCESS)) {
        ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_BASELINE, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 4: Start the programmed CXL.cachemem keyset with Key Set zero. */
    val_firme_ide_keyset_go_do_retry(val_pcie_ecam_base(),
                                     val_firme_ide_flags(VAL_FIRME_IDE_TYPE_CXL_CACHEMEM),
                                     keyset_id, IDE_CXL_KEY_SET_HANDLE_GO, &res);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_INCOMPLETE) {
        val_firme_ide_keyset_poll_do_retry(val_pcie_ecam_base(),
                                           val_firme_ide_poll_flags(VAL_FALSE),
                                           keyset_id, &res);
        if ((val_firme_status_to_i64(res.x0) == (int64_t)FIRME_SUCCESS) &&
            (res.x1 != IDE_CXL_KEY_SET_HANDLE_GO)) {
            ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_GO, res.x0, res.x1);
            return VAL_ERROR;
        }
    }
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_GO, res.x0, res.x1);
        return VAL_ERROR;
    }

    /* Step 5: Stop the same CXL.cachemem keyset before the next test. */
    val_firme_ide_keyset_stop_do_retry(val_pcie_ecam_base(),
                                       val_firme_ide_flags(VAL_FIRME_IDE_TYPE_CXL_CACHEMEM),
                                       keyset_id, IDE_CXL_KEY_SET_HANDLE_STOP, &res);
    if (val_firme_status_to_i64(res.x0) == (int64_t)FIRME_INCOMPLETE) {
        val_firme_ide_keyset_poll_do_retry(val_pcie_ecam_base(),
                                           val_firme_ide_poll_flags(VAL_FALSE),
                                           keyset_id, &res);
        if ((val_firme_status_to_i64(res.x0) == (int64_t)FIRME_SUCCESS) &&
            (res.x1 != IDE_CXL_KEY_SET_HANDLE_STOP)) {
            ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_STOP, res.x0, res.x1);
            return VAL_ERROR;
        }
    }
    if (val_firme_status_to_i64(res.x0) != (int64_t)FIRME_SUCCESS) {
        ide_cxl_key_set_fail(out, IDE_CXL_KEY_SET_STEP_STOP, res.x0, res.x1);
        return VAL_ERROR;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return VAL_SUCCESS;
}
