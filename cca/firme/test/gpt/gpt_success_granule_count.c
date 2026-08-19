/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_platform.h"
#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_SUCCESS_COUNT_TEST_NAME "gpt_success_granule_count"
#define GPT_SUCCESS_COUNT_STEP_ADVERTISE UINT64_C(1)
#define GPT_SUCCESS_COUNT_STEP_RESULT    UINT64_C(2)
#define GPT_SUCCESS_COUNT_STRICT_COUNT   UINT64_C(2)

static void gpt_success_count_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

static uint32_t gpt_success_count_expect(firme_res_t *res, uint64_t expected_count,
                                         firme_rule_res_t *out, uint64_t fail_step)
{
    if (!val_firme_status_success(res->x0, res->x1, expected_count)) {
        LOG_ERROR(NULL, "%s: expected count=%llu x0=%llx x1=%llx",
                  GPT_SUCCESS_COUNT_TEST_NAME, (unsigned long long)expected_count,
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
 *   @brief    - Validate Secure GM_GPI_SET SUCCESS reports transitioned count.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_success_granule_count_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    const uint64_t first_pa = base_pa;
    const uint64_t second_pa = base_pa + (uint64_t)PLAT_FIRME_GPT_GRANULE_SIZE;
    const uint64_t single_granule = UINT64_C(1);
    const uint64_t test_granules = GPT_SUCCESS_COUNT_STRICT_COUNT;

    gpt_success_count_init(out);

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_SUCCESS_COUNT_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 1: seed two adjacent granules as NS. */
    val_firme_gm_gpi_set_complete(first_pa, single_granule, FIRME_GPI_NONSECURE, &res);
    val_firme_gm_gpi_set_complete(second_pa, single_granule, FIRME_GPI_NONSECURE, &res);

    /* Step 2: transition both granules and verify the returned progress count. */
    val_firme_gm_gpi_set_complete(first_pa, test_granules, FIRME_GPI_SECURE, &res);
    if (gpt_success_count_expect(&res, test_granules, out,
                                 GPT_SUCCESS_COUNT_STEP_RESULT) == VAL_FALSE)
        goto cleanup;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    val_firme_gm_gpi_set_complete(first_pa, single_granule, FIRME_GPI_NONSECURE, &res);
    val_firme_gm_gpi_set_complete(second_pa, single_granule, FIRME_GPI_NONSECURE, &res);
    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate Realm GM_GPI_SET SUCCESS reports transitioned count.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_success_granule_count_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    firme_res_t res = {0};
    const uint64_t first_pa = base_pa;
    const uint64_t second_pa = base_pa + (uint64_t)PLAT_FIRME_GPT_GRANULE_SIZE;
    const uint64_t single_granule = UINT64_C(1);
    const uint64_t test_granules = GPT_SUCCESS_COUNT_STRICT_COUNT;

    gpt_success_count_init(out);

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_SUCCESS_COUNT_STEP_ADVERTISE) == VAL_FALSE)
        return VAL_SUCCESS;

    /* Step 1: seed two adjacent granules as NS. */
    val_firme_gm_gpi_set_complete(first_pa, single_granule, FIRME_GPI_NONSECURE, &res);
    val_firme_gm_gpi_set_complete(second_pa, single_granule, FIRME_GPI_NONSECURE, &res);

    /* Step 2: transition both granules and verify the returned progress count. */
    val_firme_gm_gpi_set_complete(first_pa, test_granules, FIRME_GPI_REALM, &res);
    if (gpt_success_count_expect(&res, test_granules, out,
                                 GPT_SUCCESS_COUNT_STEP_RESULT) == VAL_FALSE)
        goto cleanup;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    val_firme_gm_gpi_set_complete(first_pa, single_granule, FIRME_GPI_NONSECURE, &res);
    val_firme_gm_gpi_set_complete(second_pa, single_granule, FIRME_GPI_NONSECURE, &res);
    return VAL_SUCCESS;
}

/**
 *   @brief    - Report NS coverage as skipped when no NS changing transition is available.
 *   @param    - base_pa : Unused GPT test window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS when a rule result is produced.
**/
int firme_gpt_success_granule_count_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    return VAL_SUCCESS;
}
