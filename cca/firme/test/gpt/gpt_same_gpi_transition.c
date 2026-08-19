/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_SAME_GPI_TEST_NAME "gpt_same_gpi_transition"
#define GPT_SAME_GPI_STEP_ADVERTISE UINT64_C(1)
#define GPT_SAME_GPI_STEP_SEED      UINT64_C(2)
#define GPT_SAME_GPI_STEP_RESULT    UINT64_C(3)

static void gpt_same_gpi_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

static uint32_t gpt_same_gpi_expect_success(firme_res_t *res, uint64_t expected_count,
                                            firme_rule_res_t *out, uint64_t fail_step)
{
    if (!val_firme_status_success(res->x0, res->x1, expected_count)) {
        LOG_ERROR(NULL, "%s: expected same-GPI SUCCESS count=%llu x0=%llx x1=%llx",
                  GPT_SAME_GPI_TEST_NAME, (unsigned long long)expected_count,
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
 *   @brief    - Validate same-GPI transitions from one caller world.
 *   @param    - base_pa    : Base PA for the GPT test window.
 *   @param    - target_gpi : Target GPI used as both current and target state.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
static int gpt_same_gpi_run(uint64_t base_pa, uint64_t target_gpi, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    const uint64_t granule_count = UINT64_C(1);

    gpt_same_gpi_init(out);

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_SAME_GPI_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 1: place the granule at the target GPI. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, target_gpi, &res);
    if (gpt_same_gpi_expect_success(&res, granule_count, out,
                                    GPT_SAME_GPI_STEP_SEED) == VAL_FALSE)
        goto cleanup;

    /* Step 2: invoke GM_GPI_SET again with Current GPI == Target GPI. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, target_gpi, &res);
    if (gpt_same_gpi_expect_success(&res, granule_count, out,
                                    GPT_SAME_GPI_STEP_RESULT) == VAL_FALSE)
        goto cleanup;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate same-GPI transitions from Secure world.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_same_gpi_transition_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_same_gpi_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate same-GPI transitions from Realm world.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_same_gpi_transition_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_same_gpi_run(base_pa, FIRME_GPI_REALM, out);
}

/**
 *   @brief    - Validate same-GPI transitions from NS world.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_same_gpi_transition_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_same_gpi_run(base_pa, FIRME_GPI_NONSECURE, out);
}
