/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_platform.h"
#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_GRANULE_SIZE_TEST_NAME "gpt_granule_size"
#define GPT_GRANULE_SIZE_STEP_FEATURE_REG1 UINT64_C(1)
#define GPT_GRANULE_SIZE_STEP_PGS_MISMATCH UINT64_C(2)

/**
 *   @brief    - Validate that the transition granule size matches advertised PGS.
 *   @param    - base_pa : Unused GPT test window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_granule_size_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    uint64_t feature_reg1 = 0ULL;
    uint64_t advertised_granule_size = 0ULL;
    const uint64_t platform_granule_size = (uint64_t)PLAT_FIRME_GPT_GRANULE_SIZE;

    (void)base_pa;

    if (!out)
        return VAL_ERROR;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present()) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_GRANULE_SIZE_STEP_FEATURE_REG1) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: read GM feature register 1 and decode the PGS field. */
    if (val_firme_gpt_feature_reg1(&feature_reg1, out,
                                   GPT_GRANULE_SIZE_STEP_FEATURE_REG1) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    advertised_granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if (advertised_granule_size != platform_granule_size) {
        LOG_ERROR(NULL, "%s: PGS size mismatch advertised=%llx platform=%llx reg1=%llx",
                  GPT_GRANULE_SIZE_TEST_NAME,
                  (unsigned long long)advertised_granule_size,
                  (unsigned long long)platform_granule_size,
                  (unsigned long long)feature_reg1);
        if (out) {
            out->fail_step = GPT_GRANULE_SIZE_STEP_PGS_MISMATCH;
            out->last_x0 = advertised_granule_size;
            out->last_x1 = platform_granule_size;
        }
        return val_firme_gpt_rule_result_status(out);
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);
}
