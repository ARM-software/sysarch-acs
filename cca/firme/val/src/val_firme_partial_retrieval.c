/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_partial_retrieval.h"

#include "val_common/val_status.h"
#include "val_firme_abi.h"

/**
 *   @brief    - Default partial-retrieval adapter availability hook.
 *   @param    - world : FIRME caller world.
 *   @return   - VAL_FALSE until a service-specific adapter overrides it.
 **/
__attribute__((weak)) uint32_t
val_firme_partial_retrieval_probe_available(firme_acs_world_t world)
{
    (void)world;
    return VAL_FALSE;
}

/**
 *   @brief    - Default partial-retrieval service probe hook.
 *   @param    - world     : FIRME caller world.
 *   @param    - test_case : Retrieval scenario to exercise.
 *   @param    - evidence  : Returned ABI and size evidence.
 *   @return   - VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE.
 **/
__attribute__((weak)) int val_firme_partial_retrieval_probe(
    firme_acs_world_t world,
    val_firme_partial_retrieval_case_t test_case,
    val_firme_partial_retrieval_evidence_t *evidence)
{
    (void)world;
    (void)test_case;

    if (evidence != 0) {
        evidence->abi_status = (int64_t)FIRME_NOT_SUPPORTED;
        evidence->written_size = 0ULL;
        evidence->remaining_size = 0ULL;
        evidence->service_id = FIRME_SERVICE_ID_MAX;
        evidence->valid_fields = 0U;
    }

    return VAL_FIRME_PARTIAL_RETRIEVAL_PROBE_UNAVAILABLE;
}
