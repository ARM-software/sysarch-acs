/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_partial_retrieval.h"
#include "val_firme_rules.h"

#define PARTIAL_RETRIEVAL_STEP_PROBE_UNAVAILABLE UINT64_C(1)
#define PARTIAL_RETRIEVAL_STEP_PROBE             UINT64_C(2)
#define PARTIAL_RETRIEVAL_STEP_EVIDENCE          UINT64_C(3)
#define PARTIAL_RETRIEVAL_STEP_STATUS            UINT64_C(4)
#define PARTIAL_RETRIEVAL_STEP_WRITTEN_SIZE      UINT64_C(5)
#define PARTIAL_RETRIEVAL_STEP_REMAINING_SIZE    UINT64_C(6)

/**
 *   @brief    - Record a partial-retrieval rule failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void.
 **/
void firme_partial_retrieval_test_fail(firme_rule_res_t *out,
                                       uint64_t fail_step,
                                       uint64_t last_x0,
                                       uint64_t last_x1);

/**
 *   @brief    - Prepare a partial-retrieval rule at one FIRME instance.
 *   @param    - world : FIRME caller world.
 *   @param    - out   : Result record.
 *   @return   - VAL_SUCCESS when setup produced a rule outcome, else VAL_ERROR.
 **/
int firme_partial_retrieval_test_prepare(firme_acs_world_t world,
                                         firme_rule_res_t *out);

/**
 *   @brief    - Collect evidence from one partial-retrieval invocation.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Retrieval scenario to exercise.
 *   @param    - out       : Result record.
 *   @param    - evidence  : Returned ABI and size evidence.
 *   @return   - VAL_SUCCESS when evidence was collected, else VAL_ERROR.
 **/
int firme_partial_retrieval_test_collect(
    firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case,
    firme_rule_res_t *out,
    val_firme_partial_retrieval_evidence_t *evidence);
