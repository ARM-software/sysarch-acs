/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_INVALID_PARAMS_TEST_NAME "gpt_invalid_parameters_status"
#define GPT_INVALID_PARAMS_STEP_ADVERTISE       UINT64_C(1)
#define GPT_INVALID_PARAMS_STEP_ZERO_COUNT      UINT64_C(2)
#define GPT_INVALID_PARAMS_STEP_BAD_ATTRIBUTES  UINT64_C(3)
#define GPT_INVALID_PARAMS_STEP_UNALIGNED_BASE  UINT64_C(4)
#define GPT_INVALID_PARAMS_UNALIGNED_OFFSET     UINT64_C(1)

static uint32_t gpt_invalid_params_expect(firme_res_t *res, firme_rule_res_t *out,
                                          uint64_t fail_step)
{
    if (!val_firme_status_invalid_parameters(res->x0)) {
        LOG_ERROR(NULL, "%s: expected INVALID_PARAMETERS step=%llu x0=%llx x1=%llx",
                  GPT_INVALID_PARAMS_TEST_NAME, (unsigned long long)fail_step,
                  (unsigned long long)res->x0, (unsigned long long)res->x1);
        if (out) {
            out->fail_step = fail_step;
            out->last_x0 = res->x0;
            out->last_x1 = res->x1;
        }
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Validate GM_GPI_SET INVALID_PARAMETERS status conditions.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_invalid_parameters_status_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    const uint64_t granule_count_zero = UINT64_C(0);
    const uint64_t granule_count = UINT64_C(1);
    uint64_t unaligned_base_pa = 0ULL;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_INVALID_PARAMS_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 1: zero granule count must be rejected. */
    val_firme_gm_gpi_set_do(base_pa, granule_count_zero, FIRME_GPI_NONSECURE, &res);
    if (gpt_invalid_params_expect(&res, out, GPT_INVALID_PARAMS_STEP_ZERO_COUNT) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 2: an unallocated Target GPI is an incorrectly encoded Attributes field. */
    val_firme_gm_gpi_set_do(base_pa, granule_count,
                            FIRME_GPI_UNALLOCATED_TEST, &res);
    if (gpt_invalid_params_expect(&res, out,
                                  GPT_INVALID_PARAMS_STEP_BAD_ATTRIBUTES) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 3: a base address not aligned to PGS must be rejected. */
    unaligned_base_pa = base_pa + GPT_INVALID_PARAMS_UNALIGNED_OFFSET;
    val_firme_gm_gpi_set_do(unaligned_base_pa, granule_count,
                            FIRME_GPI_NONSECURE, &res);
    if (gpt_invalid_params_expect(&res, out,
                                  GPT_INVALID_PARAMS_STEP_UNALIGNED_BASE) == VAL_FALSE)
        return VAL_SUCCESS;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}
