/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"

#include "val_common/val_status.h"
#include "val_log.h"

#define GPT_MANAGEMENT_OWNER_TEST_NAME "gpt_management_owner_stability"

#define GPT_MANAGEMENT_OWNER_STEP_INFO_ADVERTISEMENT UINT64_C(1)
#define GPT_MANAGEMENT_OWNER_STEP_GPI_ADVERTISEMENT  UINT64_C(2)
#define GPT_MANAGEMENT_OWNER_STEP_WINDOW             UINT64_C(3)
#define GPT_MANAGEMENT_OWNER_STEP_BEFORE             UINT64_C(4)
#define GPT_MANAGEMENT_OWNER_STEP_TRANSITION         UINT64_C(5)
#define GPT_MANAGEMENT_OWNER_STEP_DURING             UINT64_C(6)
#define GPT_MANAGEMENT_OWNER_STEP_CLEANUP            UINT64_C(7)
#define GPT_MANAGEMENT_OWNER_STEP_AFTER              UINT64_C(8)

/**
 * @brief Record an R0134 failure.
 * @param out Rule result.
 * @param step Failing algorithm step.
 * @param result Last ABI result.
 * @return None.
 */
static void gpt_management_owner_fail(firme_rule_res_t *out,
                                      uint64_t step,
                                      const firme_res_t *result)
{
    out->fail_step = step;
    out->last_x0 = result->x0;
    out->last_x1 = result->x1;
}

/**
 * @brief Query and validate GPT_INFO for the selected protected range.
 * @param window Protected-address-range query.
 * @param result Returned GPT_INFO result.
 * @param out Rule result.
 * @param step Diagnostic step.
 * @return VAL_TRUE for valid SUCCESS outputs, otherwise VAL_FALSE.
 */
static uint32_t gpt_management_owner_query(
    const val_firme_gpt_info_window_t *window, firme_res_t *result,
    firme_rule_res_t *out, uint64_t step)
{
    val_firme_gm_gpt_info(window->range_base, window->range_top, result);
    return val_firme_gpt_info_success_valid(window, result, out, step);
}

/**
 * @brief Validate protected-range owner stability for R0134.
 * @param base_pa Platform GPT test address.
 * @param target_gpi Caller-owned GPI used for the temporary transition.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
static int gpt_management_owner_run(uint64_t base_pa,
                                    uint64_t target_gpi,
                                    firme_rule_res_t *out)
{
    val_firme_gpt_info_window_t window;
    firme_res_t before = {0};
    firme_res_t during = {0};
    firme_res_t after = {0};
    firme_res_t transition = {0};
    uint64_t owner_before;
    uint32_t main_passed = VAL_FALSE;
    uint64_t changed_count = 0ULL;

    if (!out)
        return VAL_ERROR;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;

    /* Step 1: require the mandatory GPT_INFO discovery ABI. */
    if (val_firme_gpt_info_advertised(
            out, GPT_MANAGEMENT_OWNER_STEP_INFO_ADVERTISEMENT) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 2: require GPI_SET for the non-owner-changing state operation. */
    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_MANAGEMENT_OWNER_STEP_GPI_ADVERTISEMENT) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 3: derive the protected range containing the transition granule. */
    if (val_firme_gpt_info_prepare_window(
            base_pa, &window, out,
            GPT_MANAGEMENT_OWNER_STEP_WINDOW) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);
    if ((base_pa % window.granule_size) != 0ULL) {
        LOG_ERROR(NULL, "%s: test PA is not PGS aligned pa=%llx pgs=%llx",
                  GPT_MANAGEMENT_OWNER_TEST_NAME,
                  (unsigned long long)base_pa,
                  (unsigned long long)window.granule_size);
        out->fail_step = GPT_MANAGEMENT_OWNER_STEP_WINDOW;
        out->last_x0 = base_pa;
        out->last_x1 = window.granule_size;
        return val_firme_gpt_rule_result_status(out);
    }

    /* Step 4: record ownership before changing one granule's GPI. */
    if (gpt_management_owner_query(
            &window, &before, out,
            GPT_MANAGEMENT_OWNER_STEP_BEFORE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);
    owner_before = before.x2 & FIRME_GPT_PAR_STATE_OWNER_MASK;

    /* Step 5: perform a GPI transition that must not alter range ownership. */
    if (val_firme_gpt_transition_terminal(base_pa, target_gpi, &transition,
            out, GPT_MANAGEMENT_OWNER_STEP_TRANSITION) != VAL_SUCCESS)
        return VAL_ERROR;

    if (val_firme_status_success(
             transition.x0, transition.x1, UINT64_C(1)) == VAL_FALSE) {
        LOG_ERROR(NULL, "%s: transition failed x0=%llx x1=%llx",
                  GPT_MANAGEMENT_OWNER_TEST_NAME,
                  (unsigned long long)transition.x0,
                  (unsigned long long)transition.x1);
        gpt_management_owner_fail(
            out, GPT_MANAGEMENT_OWNER_STEP_TRANSITION, &transition);
        goto cleanup;
    }

    changed_count = UINT64_C(1);

    /* Step 6: ownership must remain unchanged while the new GPI is active. */
    if (gpt_management_owner_query(
            &window, &during, out,
            GPT_MANAGEMENT_OWNER_STEP_DURING) == VAL_FALSE)
        goto cleanup;
    if ((during.x2 & FIRME_GPT_PAR_STATE_OWNER_MASK) != owner_before) {
        LOG_ERROR(NULL, "%s: owner changed before=%llu during=%llu",
                  GPT_MANAGEMENT_OWNER_TEST_NAME,
                  (unsigned long long)before.x2,
                  (unsigned long long)during.x2);
        gpt_management_owner_fail(
            out, GPT_MANAGEMENT_OWNER_STEP_DURING, &during);
        goto cleanup;
    }

    main_passed = VAL_TRUE;

cleanup:
    /* Step 7: Restore only transitioned storage; Host proves exact NS before reuse. */
    if (!val_firme_gpt_restore_prefix_ns(base_pa, changed_count, window.granule_size,
                                        out, GPT_MANAGEMENT_OWNER_STEP_CLEANUP))
        return VAL_ERROR;

    if (main_passed == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 8: ownership must also remain unchanged after cleanup. */
    if (gpt_management_owner_query(
            &window, &after, out,
            GPT_MANAGEMENT_OWNER_STEP_AFTER) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);
    if ((after.x2 & FIRME_GPT_PAR_STATE_OWNER_MASK) != owner_before) {
        LOG_ERROR(NULL, "%s: owner changed before=%llu after=%llu",
                  GPT_MANAGEMENT_OWNER_TEST_NAME,
                  (unsigned long long)before.x2,
                  (unsigned long long)after.x2);
        gpt_management_owner_fail(
            out, GPT_MANAGEMENT_OWNER_STEP_AFTER, &after);
        return VAL_ERROR;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return val_firme_gpt_rule_result_status(out);
}

/**
 * @brief Validate R0134 from the Secure-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_management_owner_stability_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_management_owner_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 * @brief Validate R0134 from the Realm-world FIRME instance.
 * @param base_pa Platform GPT test address.
 * @param out Rule result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_gpt_management_owner_stability_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_management_owner_run(base_pa, FIRME_GPI_REALM, out);
}
