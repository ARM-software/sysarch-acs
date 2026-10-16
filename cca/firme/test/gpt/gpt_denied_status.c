/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_DENIED_STATUS_TEST_NAME "gpt_denied_status"
#define GPT_DENIED_STATUS_STEP_ADVERTISE UINT64_C(1)
#define GPT_DENIED_STATUS_STEP_RESULT    UINT64_C(2)
#define GPT_DENIED_STATUS_STEP_SETUP     UINT64_C(3)
#define GPT_DENIED_STATUS_STEP_PRESTATE  UINT64_C(4)
#define GPT_DENIED_STATUS_STEP_POSTSTATE UINT64_C(7)
#define GPT_DENIED_STATUS_STEP_CLEANUP   UINT64_C(10)

/**
 *   @brief    - Validate GM_GPI_SET returns DENIED for a non-permitted transition.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_denied_status_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    const uint64_t granule_count = UINT64_C(1);
    uint32_t test_passed = VAL_FALSE;

    if (!out)
        return VAL_ERROR;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_DENIED_STATUS_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: Establish a successfully completed Non-secure pre-state. */
    if (val_firme_gpt_transition_terminal(base_pa, FIRME_GPI_NONSECURE,
            &res, out, GPT_DENIED_STATUS_STEP_SETUP) != VAL_SUCCESS) {
        val_firme_gpt_rule_result_fail(out, GPT_DENIED_STATUS_STEP_SETUP,
                                      res.x0, res.x1);
        out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    if (!val_firme_status_success(res.x0, res.x1, granule_count)) {
        val_firme_gpt_rule_result_fail(out, GPT_DENIED_STATUS_STEP_SETUP,
                                      res.x0, res.x1);
        goto cleanup;
    }

    /* Step 2: Confirm the source permissions independently of the ABI result. */
    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE, 0ULL,
                                       VAL_FALSE, out,
                                       GPT_DENIED_STATUS_STEP_PRESTATE))
        goto cleanup;

    /* Step 3: A prohibited NS-to-Root request must deny all progress. */
    if (val_firme_gpt_transition_terminal(base_pa, FIRME_GPI_ROOT,
            &res, out, GPT_DENIED_STATUS_STEP_RESULT) != VAL_SUCCESS) {
        val_firme_gpt_rule_result_fail(out, GPT_DENIED_STATUS_STEP_RESULT,
                                      res.x0, res.x1);
        out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    if (!val_firme_status_deniedish(res.x0) || (res.x1 != 0ULL)) {
        LOG_ERROR(NULL, "%s: expected DENIED/count=0 x0=%llx x1=%llx",
                  GPT_DENIED_STATUS_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        val_firme_gpt_rule_result_fail(out, GPT_DENIED_STATUS_STEP_RESULT,
                                      res.x0, res.x1);
        goto cleanup;
    }

    /* Step 4: Check that denial left the granule's permissions unchanged. */
    test_passed = val_firme_check_access_matrix(
        base_pa, FIRME_GPI_NONSECURE, 0ULL, VAL_FALSE, out,
        GPT_DENIED_STATUS_STEP_POSTSTATE);

cleanup:
    /* Step 5: Independently confirm NS rather than issuing redundant same-GPI cleanup. */
    if (!val_firme_gpt_confirm_ns_restored(base_pa, out, GPT_DENIED_STATUS_STEP_CLEANUP)) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    if (test_passed)
        out->status = (uint64_t)ACS_STATUS_PASS;

    return val_firme_gpt_rule_result_status(out);
}
