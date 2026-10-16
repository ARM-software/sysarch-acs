/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_platform.h"
#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_START_GRANULE_TEST_NAME "gpt_start_granule"
#define GPT_START_GRANULE_STEP_ADVERTISE       UINT64_C(1)
#define GPT_START_GRANULE_STEP_SET             UINT64_C(2)
#define GPT_START_GRANULE_STEP_FIRST_ACCESS    UINT64_C(10)
#define GPT_START_GRANULE_STEP_ADJACENT_ACCESS UINT64_C(20)
#define GPT_START_GRANULE_STEP_CLEANUP          UINT64_C(30)

/** @brief Initialize this rule's result to fail until every observation succeeds. */
static void gpt_start_granule_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }
}

/** @brief Record the fault that violated the expected access policy. */
static void gpt_start_granule_record_fault(firme_rule_res_t *out, uint64_t fail_step,
                                           val_fault_info_t *fault_info)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = fault_info->esr;
        out->last_x1 = fault_info->far;
    }
}

/** @brief Require a completed read through the caller's normal mapping. */
static uint32_t gpt_start_granule_expect_access(uint64_t addr, firme_rule_res_t *out,
                                                uint64_t fail_step)
{
    val_fault_info_t fault_info = {0};
    uint64_t value = 0;

    if (val_probe_read64(addr, &value, &fault_info) != VAL_SUCCESS ||
        fault_info.faulted != 0U) {
        LOG_ERROR(NULL, "%s: expected granule access esr=%llx far=%llx",
                  GPT_START_GRANULE_TEST_NAME,
                  (unsigned long long)fault_info.esr,
                  (unsigned long long)fault_info.far);
        gpt_start_granule_record_fault(out, fail_step, &fault_info);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/** @brief Require a granule protection fault rather than an unrelated exception. */
static uint32_t gpt_start_granule_expect_gpf(uint64_t addr, firme_rule_res_t *out,
                                             uint64_t fail_step)
{
    val_fault_info_t fault_info = {0};
    uint64_t value = 0;

    val_probe_read64(addr, &value, &fault_info);
    if ((fault_info.faulted == 0U) || !val_fault_is_gpf(fault_info.esr)) {
        LOG_ERROR(NULL, "%s: expected granule GPF faulted=%u esr=%llx far=%llx",
                  GPT_START_GRANULE_TEST_NAME, (unsigned int)fault_info.faulted,
                  (unsigned long long)fault_info.esr,
                  (unsigned long long)fault_info.far);
        gpt_start_granule_record_fault(out, fail_step, &fault_info);
        return VAL_FALSE;
    }

    return VAL_TRUE;
}

/**
 *   @brief    - Complete one GPI transition and require full success.
 *   @param    - base_pa    : Base PA of the granule.
 *   @param    - target_gpi : Requested target GPI.
 *   @param    - out        : Optional result record.
 *   @param    - fail_step      : Failure step identifier.
 *   @return   - VAL_SUCCESS for a completed transition, otherwise VAL_ERROR.
**/
static int gpt_start_granule_transition(uint64_t base_pa, uint64_t target_gpi,
                                        firme_rule_res_t *out, uint64_t fail_step)
{
    firme_res_t result = {0};
    const uint64_t granule_count = UINT64_C(1);
    int completion_status = VAL_ERROR;

    completion_status = val_firme_gpt_transition_terminal(
        base_pa, target_gpi, &result, out, fail_step);
    if (completion_status != VAL_SUCCESS)
        return VAL_ERROR;

    if (!val_firme_status_success(result.x0, result.x1, granule_count)) {
        LOG_ERROR(NULL, "%s: transition failed PA=%llx GPI=%llu x0=%llx x1=%llx",
                  GPT_START_GRANULE_TEST_NAME,
                  (unsigned long long)base_pa,
                  (unsigned long long)target_gpi,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        if (out) {
            out->status = (uint64_t)ACS_STATUS_FAIL;
            out->fail_step = fail_step;
            out->last_x0 = result.x0;
            out->last_x1 = result.x1;
        }
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Validate GM_GPI_SET starts at Base from one caller world.
 *   @param    - base_pa    : Base PA for the GPT test window.
 *   @param    - target_gpi : Target GPI for the caller world.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
static int gpt_start_granule_run(uint64_t base_pa, uint64_t target_gpi, firme_rule_res_t *out)
{
    const uint64_t first_pa = base_pa;
    uint64_t second_pa;
    uint32_t first_access;
    uint64_t changed_count = 0ULL;

    if (!out)
        return VAL_ERROR;

    gpt_start_granule_init(out);

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(out, GPT_START_GRANULE_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    if ((base_pa % PLAT_FIRME_GPT_GRANULE_SIZE != 0ULL) ||
        (base_pa > UINT64_MAX - UINT64_C(2) * PLAT_FIRME_GPT_GRANULE_SIZE)) {
        val_firme_gpt_rule_result_fail(out, GPT_START_GRANULE_STEP_SET, base_pa,
                                       PLAT_FIRME_GPT_GRANULE_SIZE);
        return VAL_ERROR;
    }

    second_pa = base_pa + PLAT_FIRME_GPT_GRANULE_SIZE;
    first_access = (target_gpi != FIRME_GPI_NSP);

    /* Step 1: seed two adjacent granules as NS before the one-granule request. */
    if (gpt_start_granule_transition(first_pa, FIRME_GPI_NONSECURE, out,
                                     GPT_START_GRANULE_STEP_SET) != VAL_SUCCESS)
        goto cleanup;
    if (gpt_start_granule_transition(second_pa, FIRME_GPI_NONSECURE, out,
                                     GPT_START_GRANULE_STEP_SET) != VAL_SUCCESS)
        goto cleanup;

    /* Step 2: transition only the start granule at Base. */
    if (gpt_start_granule_transition(first_pa, target_gpi, out,
                                     GPT_START_GRANULE_STEP_SET) != VAL_SUCCESS)
        goto cleanup;
    changed_count = UINT64_C(1);
    /* Step 3: require different access policies for the first and adjacent granules. */
    if (first_access == VAL_TRUE) {
        if (!gpt_start_granule_expect_access(first_pa, out,
                                             GPT_START_GRANULE_STEP_FIRST_ACCESS) ||
            !gpt_start_granule_expect_gpf(second_pa, out,
                                          GPT_START_GRANULE_STEP_ADJACENT_ACCESS))
            goto cleanup;
    } else {
        if (!gpt_start_granule_expect_gpf(first_pa, out,
                                          GPT_START_GRANULE_STEP_FIRST_ACCESS) ||
            !gpt_start_granule_expect_access(second_pa, out,
                                             GPT_START_GRANULE_STEP_ADJACENT_ACCESS))
            goto cleanup;
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    /* Step 4: Restore the changed prefix; Host checks both granules before reuse. */
    if (!val_firme_gpt_restore_prefix_ns(first_pa, changed_count,
            PLAT_FIRME_GPT_GRANULE_SIZE, out, GPT_START_GRANULE_STEP_CLEANUP))
        return VAL_ERROR;

    return val_firme_gpt_rule_result_status(out);
}

/**
 *   @brief    - Validate that Secure GM_GPI_SET starts at the Base granule.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_start_granule_run_secure(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_start_granule_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate that Realm GM_GPI_SET starts at the Base granule.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_start_granule_run_realm(uint64_t base_pa, firme_rule_res_t *out)
{
    return gpt_start_granule_run(base_pa, FIRME_GPI_REALM, out);
}

/**
 *   @brief    - Use GDI to distinguish the transitioned granule from its NS neighbor.
 *   @param    - base_pa : Owned NS GPT test window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_start_granule_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    if (!out)
        return VAL_ERROR;

    if (!val_firme_feat_rme_gdi_present()) {
        gpt_start_granule_init(out);
        out->status = ACS_STATUS_SKIP;
        return VAL_SUCCESS;
    }

    return gpt_start_granule_run(base_pa, FIRME_GPI_NSP, out);
}
