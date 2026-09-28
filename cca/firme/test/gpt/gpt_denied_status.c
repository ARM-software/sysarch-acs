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

    /* Step 1: NS caller is not permitted to transition an NS granule to Root. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_ROOT, &res);
    if (!val_firme_status_deniedish(res.x0)) {
        LOG_ERROR(NULL, "%s: expected DENIED x0=%llx x1=%llx",
                  GPT_DENIED_STATUS_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (out) {
            out->fail_step = GPT_DENIED_STATUS_STEP_RESULT;
            out->last_x0 = res.x0;
            out->last_x1 = res.x1;
        }
        return val_firme_gpt_rule_result_status(out);
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);
}
