/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mec_semantic_test_common.h"

#include "val_common/val_fault.h"
#include "val_firme_abi.h"

/**
 *   @brief    - Validate R0236 through stale-data and new-context observations.
 *   @param    - out : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
static int mec_context_refresh_run(firme_rule_res_t *out)
{
    val_firme_mec_observation_evidence_t evidence = {0};
    uint32_t required_fields = VAL_FIRME_MEC_EVIDENCE_TEST_ADDRESS |
                               VAL_FIRME_MEC_EVIDENCE_PLAINTEXT |
                               VAL_FIRME_MEC_EVIDENCE_SAMPLE_COUNT |
                               VAL_FIRME_MEC_EVIDENCE_CACHE_MAINTENANCE |
                               VAL_FIRME_MEC_EVIDENCE_STALE_READ |
                               VAL_FIRME_MEC_EVIDENCE_ROUND_TRIP;
    uint32_t expected_mask = 0U;
    uint32_t index = 0U;
    uint32_t sample_bit = 0U;
    uint64_t fault_esr = 0ULL;
    int result = VAL_ERROR;

    result = firme_mec_semantic_test_prepare(out);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    result = firme_mec_semantic_test_collect(
        VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH, out, &evidence);
    if ((result != VAL_SUCCESS) ||
        (out->status != (uint64_t)ACS_STATUS_FAIL))
        return result;

    if ((evidence.setup_completed != VAL_TRUE) ||
        (evidence.cache_maintenance_completed != VAL_TRUE)) {
        firme_mec_semantic_test_fail(out, MEC_SEMANTIC_STEP_SETUP,
                                     evidence.setup_fault_esr,
                                     evidence.setup_completed);
        return VAL_SUCCESS;
    }

    if ((evidence.valid_fields & required_fields) != required_fields) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_mec_semantic_test_fail(out, MEC_SEMANTIC_STEP_EVIDENCE,
                                     evidence.valid_fields,
                                     required_fields);
        return VAL_ERROR;
    }

    if ((evidence.sample_count == 0U) ||
        (evidence.sample_count > VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES)) {
        firme_mec_semantic_test_fail(out, MEC_SEMANTIC_STEP_SAMPLE_COUNT,
                                     evidence.sample_count,
                                     VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES);
        return VAL_SUCCESS;
    }
    expected_mask = (UINT32_C(1) << evidence.sample_count) - 1U;

    /* Step 1: Every sample must work after being rewritten under the new MEC. */
    if (evidence.round_trip_completed_mask != expected_mask) {
        firme_mec_semantic_test_fail(
            out, MEC_SEMANTIC_STEP_ROUND_TRIP,
            evidence.round_trip_completed_mask, expected_mask);
        return VAL_SUCCESS;
    }
    for (index = 0U; index < evidence.sample_count; index++) {
        if (evidence.round_trip[index] != evidence.plaintext) {
            firme_mec_semantic_test_fail(
                out, MEC_SEMANTIC_STEP_ROUND_TRIP,
                evidence.round_trip[index], evidence.plaintext);
            return VAL_SUCCESS;
        }
    }

    /*
     * Step 2: Old protected representations must not remain readable as the
     * original plaintext after the refresh. A recovered integrity abort is
     * also evidence that the old context no longer authenticates the data.
     */
    for (index = 0U; index < evidence.sample_count; index++) {
        sample_bit = UINT32_C(1) << index;
        if ((evidence.stale_read_completed_mask & sample_bit) != 0U) {
            if (evidence.stale_read[index] == evidence.plaintext) {
                firme_mec_semantic_test_fail(
                    out, MEC_SEMANTIC_STEP_STALE_READ,
                    evidence.stale_read[index], evidence.plaintext);
                return VAL_SUCCESS;
            }
            continue;
        }

        if ((evidence.stale_read_fault_mask & sample_bit) == 0U) {
            firme_mec_semantic_test_fail(
                out, MEC_SEMANTIC_STEP_STALE_READ,
                evidence.stale_read_completed_mask,
                evidence.stale_read_fault_mask);
            return VAL_SUCCESS;
        }

        fault_esr = evidence.stale_read_fault_esr[index];
        if ((val_fault_is_abort(fault_esr) == 0U) &&
            (val_fault_is_serror(fault_esr) == 0U)) {
            firme_mec_semantic_test_fail(
                out, MEC_SEMANTIC_STEP_INTEGRITY_ABORT,
                fault_esr, index);
            return VAL_SUCCESS;
        }
    }

    /* Step 3: The cleanup refresh must retire the controlled context. */
    if (evidence.cleanup_status != (int64_t)FIRME_SUCCESS) {
        firme_mec_semantic_test_fail(
            out, MEC_SEMANTIC_STEP_CLEANUP,
            (uint64_t)evidence.cleanup_status, evidence.mecid);
        return VAL_SUCCESS;
    }

    out->status = (uint64_t)ACS_STATUS_PASS;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Execute R0236 from the Realm FIRME instance.
 *   @param    - base_pa : Unused; the Realm payload owns a reserved test page.
 *   @param    - out     : Result record.
 *   @return   - VAL_SUCCESS when the rule produced an outcome, else VAL_ERROR.
 **/
int firme_mec_context_refresh_run_realm(
    uint64_t base_pa, firme_rule_res_t *out)
{
    (void)base_pa;
    return mec_context_refresh_run(out);
}
