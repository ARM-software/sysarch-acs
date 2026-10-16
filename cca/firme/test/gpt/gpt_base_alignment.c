/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_log.h"

#define GPT_BASE_ALIGNMENT_TEST_NAME "gpt_base_alignment"
#define GPT_BASE_ALIGNMENT_STEP_ADVERTISE UINT64_C(1)
#define GPT_BASE_ALIGNMENT_STEP_FEATURE   UINT64_C(2)
#define GPT_BASE_ALIGNMENT_STEP_PLATFORM  UINT64_C(3)
#define GPT_BASE_ALIGNMENT_STEP_MISALIGN  UINT64_C(4)
#define GPT_BASE_ALIGNMENT_STEP_ALIGNED   UINT64_C(5)
#define GPT_BASE_ALIGNMENT_STEP_CLEANUP   UINT64_C(6)

/**
 *   @brief    - Initialise an R0090 result record.
 *   @param    - out : Optional result record.
 *   @return   - void.
 **/
static void gpt_base_alignment_init(firme_rule_res_t *out)
{
    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0ULL;
        out->last_x0 = 0ULL;
        out->last_x1 = 0ULL;
    }
}

/**
 *   @brief    - Record an ABI-result failure for R0090.
 *   @param    - out       : Optional result record.
 *   @param    - fail_step : Failure step identifier.
 *   @param    - result    : ABI result to record.
 *   @return   - void.
 **/
static void gpt_base_alignment_record(firme_rule_res_t *out,
                                      uint64_t fail_step,
                                      const firme_res_t *result)
{
    if (out) {
        out->fail_step = fail_step;
        out->last_x0 = result ? result->x0 : 0ULL;
        out->last_x1 = result ? result->x1 : 0ULL;
    }
}

/**
 *   @brief    - Run the R0090 alignment checks from one permitted caller.
 *   @param    - base_pa    : Platform GPT test-window base.
 *   @param    - target_gpi : Caller-world target GPI.
 *   @param    - out        : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
static int gpt_base_alignment_run(uint64_t base_pa,
                                  uint64_t target_gpi,
                                  firme_rule_res_t *out)
{
    firme_res_t result = {0};
    uint64_t feature_reg1 = 0ULL;
    uint64_t granule_size = 0ULL;
    uint64_t misaligned_pa = 0ULL;
    uint64_t changed_count = 0ULL;
    const uint64_t granule_count = UINT64_C(1);

    if (!out)
        return VAL_ERROR;

    gpt_base_alignment_init(out);

    if (!val_firme_feat_rme_present() || (base_pa == 0ULL)) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(
            out, GPT_BASE_ALIGNMENT_STEP_ADVERTISE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    /* Step 1: derive PGS from the DUT-advertised GM feature register. */
    if (val_firme_gpt_feature_reg1(&feature_reg1, out,
                                   GPT_BASE_ALIGNMENT_STEP_FEATURE) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

    granule_size = val_firme_gpt_pgs_size_from_feature_reg1(feature_reg1);
    if ((granule_size == 0ULL) || ((base_pa % granule_size) != 0ULL) ||
        (base_pa == UINT64_MAX)) {
        LOG_ERROR(NULL, "%s: invalid platform base=%llx PGS=%llx",
                  GPT_BASE_ALIGNMENT_TEST_NAME,
                  (unsigned long long)base_pa,
                  (unsigned long long)granule_size);
        if (out) {
            out->fail_step = GPT_BASE_ALIGNMENT_STEP_PLATFORM;
            out->last_x0 = base_pa;
            out->last_x1 = granule_size;
        }
        return val_firme_gpt_rule_result_status(out);
    }

    /* Step 2: a deliberately misaligned Base must be rejected. */
    misaligned_pa = base_pa + UINT64_C(1);
    val_firme_gm_gpi_set_do(misaligned_pa, granule_count, target_gpi, &result);
    if (val_firme_status_invalid_parameters(result.x0) == VAL_FALSE) {
        LOG_ERROR(NULL, "%s: misaligned Base was not rejected x0=%llx x1=%llx",
                  GPT_BASE_ALIGNMENT_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_base_alignment_record(out, GPT_BASE_ALIGNMENT_STEP_MISALIGN,
                                  &result);
        out->status = ACS_STATUS_ERR;
        goto cleanup;
    }

    /* Step 3: the same request with a PGS-aligned Base must succeed. */
    if (val_firme_gpt_transition_terminal(base_pa, target_gpi, &result,
            out, GPT_BASE_ALIGNMENT_STEP_ALIGNED) != VAL_SUCCESS)
        return VAL_ERROR;

    if (val_firme_status_success(result.x0, result.x1, granule_count) == VAL_FALSE) {
        LOG_ERROR(NULL, "%s: aligned Base failed x0=%llx x1=%llx",
                  GPT_BASE_ALIGNMENT_TEST_NAME,
                  (unsigned long long)result.x0,
                  (unsigned long long)result.x1);
        gpt_base_alignment_record(out, GPT_BASE_ALIGNMENT_STEP_ALIGNED,
                                  &result);
        goto cleanup;
    }
    changed_count = granule_count;

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    if (!val_firme_gpt_restore_prefix_ns(base_pa, changed_count, granule_size,
                                        out, GPT_BASE_ALIGNMENT_STEP_CLEANUP))
        return VAL_ERROR;

    return val_firme_gpt_rule_result_status(out);
}

/**
 *   @brief    - Validate R0090 from the Secure FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_base_alignment_run_secure(uint64_t base_pa,
                                        firme_rule_res_t *out)
{
    return gpt_base_alignment_run(base_pa, FIRME_GPI_SECURE, out);
}

/**
 *   @brief    - Validate R0090 from the Realm FIRME instance.
 *   @param    - base_pa : Platform GPT test-window base.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
 **/
int firme_gpt_base_alignment_run_realm(uint64_t base_pa,
                                       firme_rule_res_t *out)
{
    return gpt_base_alignment_run(base_pa, FIRME_GPI_REALM, out);
}
