/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_rules.h"
#include "val_firme_shared_buffer.h"

#define SHARED_BUFFER_STEP_PROBE_UNAVAILABLE UINT64_C(1)
#define SHARED_BUFFER_STEP_LIMITS            UINT64_C(2)
#define SHARED_BUFFER_STEP_PROBE             UINT64_C(3)
#define SHARED_BUFFER_STEP_EVIDENCE          UINT64_C(4)
#define SHARED_BUFFER_STEP_STATUS            UINT64_C(5)
#define SHARED_BUFFER_STEP_CALLER_ATTRIBUTES UINT64_C(6)
#define SHARED_BUFFER_STEP_EL3_ATTRIBUTES    UINT64_C(7)
#define SHARED_BUFFER_STEP_PAS               UINT64_C(8)
#define SHARED_BUFFER_STEP_GPI               UINT64_C(9)
#define SHARED_BUFFER_STEP_MINIMUM_SIZE      UINT64_C(10)
#define SHARED_BUFFER_STEP_MAXIMUM_SIZE      UINT64_C(11)
#define SHARED_BUFFER_STEP_ABOVE_MAXIMUM     UINT64_C(12)
#define SHARED_BUFFER_STEP_ALIGNED_BASE      UINT64_C(13)
#define SHARED_BUFFER_STEP_MISALIGNED_BASE   UINT64_C(14)

typedef struct {
    uint64_t minimum_size_bytes;
    uint64_t maximum_page_count;
    uint64_t maximum_size_bytes;
} shared_buffer_limits_t;

/**
 *   @brief    - Record a shared-buffer rule failure.
 *   @param    - out       : Result record.
 *   @param    - fail_step : Failing test step.
 *   @param    - last_x0   : First diagnostic value.
 *   @param    - last_x1   : Second diagnostic value.
 *   @return   - void.
 **/
void firme_shared_buffer_test_fail(firme_rule_res_t *out, uint64_t fail_step,
                                   uint64_t last_x0, uint64_t last_x1);

/**
 *   @brief    - Prepare a shared-buffer rule at one FIRME instance.
 *   @param    - world  : FIRME caller world.
 *   @param    - out    : Result record.
 *   @param    - limits : Returned Base-advertised limits.
 *   @return   - VAL_SUCCESS when setup produced a rule outcome, else VAL_ERROR.
 **/
int firme_shared_buffer_test_setup(firme_acs_world_t world,
                                   firme_rule_res_t *out,
                                   shared_buffer_limits_t *limits);

/**
 *   @brief    - Collect one service-specific shared-buffer invocation.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Shared-buffer case to exercise.
 *   @param    - limits    : Base-advertised limits.
 *   @param    - out       : Result record.
 *   @param    - evidence  : Returned invocation evidence.
 *   @return   - VAL_SUCCESS when evidence was collected, else VAL_ERROR.
 **/
int firme_shared_buffer_test_collect(
    firme_acs_world_t world,
    val_firme_shared_buffer_case_t test_case,
    const shared_buffer_limits_t *limits,
    firme_rule_res_t *out,
    val_firme_shared_buffer_evidence_t *evidence);
