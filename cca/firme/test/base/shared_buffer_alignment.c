/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "shared_buffer_test_common.h"

#include "val_firme_abi.h"
#include "val_firme_shared_buffer.h"

/**
 *   @brief    - Validate R0077 shared-buffer base alignment.
 *   @param    - world   : FIRME caller world.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
static int shared_buffer_alignment_run(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    shared_buffer_limits_t limits = {0};
    val_firme_shared_buffer_evidence_t evidence = {0};
    uint32_t required_fields = VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE;
    int result = VAL_ERROR;

    (void)base_pa;
    result = firme_shared_buffer_test_setup(world, out, &limits);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    result = firme_shared_buffer_test_collect(world, VAL_FIRME_SHARED_BUFFER_CASE_ALIGNED,
                                   &limits, out, &evidence);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    if (((evidence.valid_fields & required_fields) != required_fields) ||
        ((evidence.base_pa % limits.minimum_size_bytes) != 0ULL) ||
        (val_firme_shared_buffer_status_accepted(evidence.abi_status) == VAL_FALSE)) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_ALIGNED_BASE,
                           evidence.base_pa,
                           (uint64_t)evidence.abi_status);
        return VAL_SUCCESS;
    }

    result = firme_shared_buffer_test_collect(
        world, VAL_FIRME_SHARED_BUFFER_CASE_MISALIGNED,
        &limits, out, &evidence);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    if (((evidence.valid_fields & required_fields) != required_fields) ||
        ((evidence.base_pa % limits.minimum_size_bytes) == 0ULL) ||
        (evidence.abi_status != (int64_t)FIRME_INVALID_PARAMETERS)) {
        firme_shared_buffer_test_fail(out, SHARED_BUFFER_STEP_MISALIGNED_BASE,
                           evidence.base_pa,
                           (uint64_t)evidence.abi_status);
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Execute R0077 from the NS FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_shared_buffer_alignment_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_alignment_run(FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 *   @brief    - Execute R0077 from the Secure FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_shared_buffer_alignment_run_secure(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_alignment_run(FIRME_ACS_WORLD_SECURE, base_pa, out);
}

/**
 *   @brief    - Execute R0077 from the Realm FIRME instance.
 *   @param    - base_pa : Unused for shared-buffer concept tests.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_shared_buffer_alignment_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return shared_buffer_alignment_run(FIRME_ACS_WORLD_REALM, base_pa, out);
}
