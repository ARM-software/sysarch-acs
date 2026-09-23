/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mec_semantic_test_common.h"

#include "val_firme_abi.h"
#include "val_log.h"

#define MEC_SEMANTIC_TEST_NAME "mec_refresh_semantics"

/**
 *   @brief    - Initialize a MEC semantic-rule result.
 *   @param    - out : Result record.
 *   @return   - void.
 **/
static void mec_semantic_init_result(firme_rule_res_t *out)
{
    if (out == 0)
        return;

    out->status = (uint64_t)ACS_STATUS_FAIL;
    out->fail_step = 0ULL;
    out->last_x0 = 0ULL;
    out->last_x1 = 0ULL;
}

/**
 *   @brief    - Record a MEC semantic-rule failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void.
 **/
void firme_mec_semantic_test_fail(firme_rule_res_t *out,
                                  uint64_t fail_step,
                                  uint64_t last_x0,
                                  uint64_t last_x1)
{
    LOG_ERROR(NULL, "%s: step=%llu x0=%llx x1=%llx",
              MEC_SEMANTIC_TEST_NAME, (unsigned long long)fail_step,
              (unsigned long long)last_x0, (unsigned long long)last_x1);

    if (out == 0)
        return;

    out->fail_step = fail_step;
    out->last_x0 = last_x0;
    out->last_x1 = last_x1;
}

/**
 *   @brief    - Prepare a MEC semantic rule.
 *   @param    - out : Result record.
 *   @return   - VAL_SUCCESS when setup produced a rule outcome, else VAL_ERROR.
 **/
int firme_mec_semantic_test_prepare(firme_rule_res_t *out)
{
    if (out == 0)
        return VAL_ERROR;

    mec_semantic_init_result(out);
    if (val_firme_mec_observation_probe_available() == VAL_FALSE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = MEC_SEMANTIC_STEP_PROBE_UNAVAILABLE;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Collect evidence from one controlled MEC refresh scenario.
 *   @param    - test_case : MEC semantic scenario to exercise.
 *   @param    - out       : Result record.
 *   @param    - evidence  : Returned refresh and MPE observation evidence.
 *   @return   - VAL_SUCCESS when evidence was collected, else VAL_ERROR.
 **/
int firme_mec_semantic_test_collect(
    val_firme_mec_observation_case_t test_case,
    firme_rule_res_t *out,
    val_firme_mec_observation_evidence_t *evidence)
{
    uint32_t required_fields = VAL_FIRME_MEC_EVIDENCE_STATUS |
                               VAL_FIRME_MEC_EVIDENCE_MECID;
    int probe_result = VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;

    if ((out == 0) || (evidence == 0))
        return VAL_ERROR;

    probe_result = val_firme_mec_observation_probe(test_case, evidence);
    if (probe_result == VAL_FIRME_MEC_OBSERVATION_PROBE_UNAVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_SKIP;
        out->fail_step = MEC_SEMANTIC_STEP_PROBE_UNAVAILABLE;
        return VAL_SUCCESS;
    }

    if (probe_result != VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_mec_semantic_test_fail(out, MEC_SEMANTIC_STEP_PROBE,
                                     (uint64_t)(int64_t)probe_result,
                                     (uint64_t)test_case);
        return VAL_ERROR;
    }

    if ((evidence->valid_fields & required_fields) != required_fields) {
        out->status = (uint64_t)ACS_STATUS_ERR;
        firme_mec_semantic_test_fail(out, MEC_SEMANTIC_STEP_EVIDENCE,
                                     evidence->valid_fields,
                                     required_fields);
        return VAL_ERROR;
    }

    if (evidence->abi_status != (int64_t)FIRME_SUCCESS) {
        firme_mec_semantic_test_fail(out, MEC_SEMANTIC_STEP_STATUS,
                                     (uint64_t)evidence->abi_status,
                                     evidence->mecid);
        return VAL_SUCCESS;
    }

    return VAL_SUCCESS;
}
