/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_mec_observation.h"
#include "val_firme_rules.h"

#define MEC_SEMANTIC_STEP_PROBE_UNAVAILABLE UINT64_C(1)
#define MEC_SEMANTIC_STEP_PROBE             UINT64_C(2)
#define MEC_SEMANTIC_STEP_EVIDENCE          UINT64_C(3)
#define MEC_SEMANTIC_STEP_STATUS            UINT64_C(4)
#define MEC_SEMANTIC_STEP_SETUP             UINT64_C(5)
#define MEC_SEMANTIC_STEP_CACHE_MAINTENANCE UINT64_C(6)
#define MEC_SEMANTIC_STEP_SAMPLE_COUNT      UINT64_C(7)
#define MEC_SEMANTIC_STEP_STALE_READ        UINT64_C(8)
#define MEC_SEMANTIC_STEP_ROUND_TRIP        UINT64_C(9)
#define MEC_SEMANTIC_STEP_INTEGRITY_ABORT   UINT64_C(10)
#define MEC_SEMANTIC_STEP_CLEANUP           UINT64_C(11)

/**
 * @brief Classify stale-read data errors without claiming integrity evidence.
 * @param esr Syndrome captured by the same-EL stale-data read probe.
 * @return VAL_TRUE for inconclusive data errors, VAL_FALSE for unrelated faults.
 */
uint32_t firme_mec_semantic_test_fault_is_inconclusive(uint64_t esr);

/**
 * @brief Validate actual refresh effects and successful owned-context cleanup.
 * @param test_case CREATE or DESTROY refresh observation.
 * @param out Rule result with independent memory observations.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int firme_mec_semantic_test_context_refresh(
    val_firme_mec_observation_case_t test_case, firme_rule_res_t *out);

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
                                  uint64_t last_x1);

/**
 *   @brief    - Prepare a MEC semantic rule.
 *   @param    - out : Result record.
 *   @return   - VAL_SUCCESS when setup produced a rule outcome, else VAL_ERROR.
 **/
int firme_mec_semantic_test_prepare(firme_rule_res_t *out);

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
    val_firme_mec_observation_evidence_t *evidence);
