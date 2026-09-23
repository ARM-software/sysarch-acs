/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "partial_retrieval_test_common.h"

#include "val_firme_abi.h"

/**
 *   @brief    - Validate R0082 no-chunk partial-retrieval outputs.
 *   @param    - world   : FIRME caller world.
 *   @param    - base_pa : Unused by the service adapter.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
static int partial_retrieval_no_chunk_run(
    firme_acs_world_t world, uint64_t base_pa, firme_rule_res_t *out)
{
    val_firme_partial_retrieval_evidence_t evidence = {0};
    int result = VAL_ERROR;

    (void)base_pa;
    result = firme_partial_retrieval_test_prepare(world, out);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    result = firme_partial_retrieval_test_collect(
        world, VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK, out, &evidence);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    if (evidence.abi_status != (int64_t)FIRME_INCOMPLETE) {
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_STATUS,
            (uint64_t)evidence.abi_status, 0ULL);
        return VAL_SUCCESS;
    }

    if (evidence.written_size != 0ULL) {
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_WRITTEN_SIZE,
            evidence.written_size, evidence.remaining_size);
        return VAL_SUCCESS;
    }

    if (evidence.remaining_size != 0ULL) {
        firme_partial_retrieval_test_fail(
            out, PARTIAL_RETRIEVAL_STEP_REMAINING_SIZE,
            evidence.written_size, evidence.remaining_size);
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Execute R0082 from the NS FIRME instance.
 *   @param    - base_pa : Unused by the service adapter.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_partial_retrieval_no_chunk_run_ns(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return partial_retrieval_no_chunk_run(
        FIRME_ACS_WORLD_NSEL2, base_pa, out);
}

/**
 *   @brief    - Execute R0082 from the Realm FIRME instance.
 *   @param    - base_pa : Unused by the service adapter.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_base_partial_retrieval_no_chunk_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    return partial_retrieval_no_chunk_run(
        FIRME_ACS_WORLD_REALM, base_pa, out);
}
