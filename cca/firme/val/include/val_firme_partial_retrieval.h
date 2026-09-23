/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_dispatch.h"

#define VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_STATUS    (UINT32_C(1) << 0)
#define VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_WRITTEN   (UINT32_C(1) << 1)
#define VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_REMAINING (UINT32_C(1) << 2)
#define VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_SERVICE   (UINT32_C(1) << 3)

#define VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE   0
#define VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE 1
#define VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR       (-1)

typedef enum {
    VAL_FIRME_PARTIAL_RETRIEVAL_LAST_CHUNK = 0,
    VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK,
    VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK,
} val_firme_partial_retrieval_case_t;

typedef struct {
    int64_t abi_status;
    uint64_t written_size;
    uint64_t remaining_size;
    uint32_t service_id;
    uint32_t valid_fields;
} val_firme_partial_retrieval_evidence_t;

/**
 *   @brief    - Report whether a partial-retrieval service adapter exists.
 *   @param    - world : FIRME caller world.
 *   @return   - VAL_TRUE when an adapter is available, otherwise VAL_FALSE.
 **/
uint32_t val_firme_partial_retrieval_probe_available(firme_acs_world_t world);

/**
 *   @brief    - Exercise a controlled partial-retrieval service scenario.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Retrieval scenario to exercise.
 *   @param    - evidence  : Returned ABI and size evidence.
 *   @return   - VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_*.
 **/
int val_firme_partial_retrieval_probe(
    firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case,
    val_firme_partial_retrieval_evidence_t *evidence);
