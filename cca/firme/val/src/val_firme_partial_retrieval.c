/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_partial_retrieval.h"

#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_shared_buffer.h"

/**
 *   @brief    - Report worlds supported by the PAT partial-retrieval adapter.
 *   @param    - world : FIRME caller world.
 *   @return   - VAL_TRUE for NS/Realm; actual resources are checked by the probe.
 **/
__attribute__((weak)) uint32_t
val_firme_partial_retrieval_probe_available(firme_acs_world_t world)
{
    return val_firme_shared_buffer_probe_available(world);
}

/**
 *   @brief    - Observe a PAT intermediate/no-chunk completion and drain it.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Retrieval scenario to exercise.
 *   @param    - evidence  : Returned ABI and size evidence.
 *   @return   - VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_*; absent patterns are unavailable.
 **/
__attribute__((weak)) int val_firme_partial_retrieval_probe(
    firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case,
    val_firme_partial_retrieval_evidence_t *evidence)
{
    val_firme_shared_buffer_evidence_t buffer = {0};
    uint64_t minimum_size = 0ULL;
    uint64_t maximum_count = 0ULL;
    uint64_t maximum_size = 0ULL;
    int result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;

    if (evidence == 0)
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR;
    *evidence = (val_firme_partial_retrieval_evidence_t){0};
    if ((val_firme_shared_buffer_probe_available(world) != VAL_TRUE) ||
        (test_case == VAL_FIRME_PARTIAL_RETRIEVAL_LAST_CHUNK))
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE;
    if ((test_case != VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK) &&
        (test_case != VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK))
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR;
    if (val_firme_shared_buffer_get_limits(&minimum_size, &maximum_count,
                                            &maximum_size) != VAL_SUCCESS)
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR;
    result = val_firme_shared_buffer_pat_run(
        world, VAL_FIRME_SHARED_BUFFER_CASE_VALID, minimum_size, maximum_count,
        &buffer, test_case, evidence);
    switch (result) {
    case VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE:
        return evidence->valid_fields != 0U ? VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_AVAILABLE :
                   VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE;
    case VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE:
        evidence->prerequisite_reason = buffer.prerequisite_reason;
        evidence->prerequisite_detail = buffer.prerequisite_detail;
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE;
    case VAL_FIRME_SHARED_BUFFER_PROBE_FAILED:
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_FAILED;
    default:
        return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_ERROR;
    }
}
