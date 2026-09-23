/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_mec_observation.h"

#include "val_common/val_status.h"
#include "val_firme_abi.h"

/**
 *   @brief    - Default MPE observation-adapter availability hook.
 *   @return   - VAL_FALSE until a platform adapter overrides this hook.
 **/
__attribute__((weak)) uint32_t val_firme_mec_observation_probe_available(void)
{
    return VAL_FALSE;
}

/**
 *   @brief    - Default MEC refresh observation probe hook.
 *   @param    - test_case : MEC semantic scenario to exercise.
 *   @param    - evidence  : Returned refresh and MPE observation evidence.
 *   @return   - VAL_FIRME_MEC_OBSERVATION_PROBE_UNAVAILABLE.
 **/
__attribute__((weak)) int val_firme_mec_observation_probe(
    val_firme_mec_observation_case_t test_case,
    val_firme_mec_observation_evidence_t *evidence)
{
    uint32_t index = 0U;

    if ((test_case != VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH) &&
        (test_case != VAL_FIRME_MEC_OBSERVE_CIPHERTEXT_DECORRELATION))
        return VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR;

    if (evidence != 0) {
        evidence->abi_status = (int64_t)FIRME_NOT_SUPPORTED;
        evidence->mecid = 0ULL;
        evidence->test_pa = 0ULL;
        evidence->plaintext = 0ULL;
        evidence->setup_fault_esr = 0ULL;
        evidence->cleanup_status = (int64_t)FIRME_NOT_SUPPORTED;
        evidence->stale_read_completed_mask = 0U;
        evidence->stale_read_fault_mask = 0U;
        evidence->round_trip_completed_mask = 0U;
        evidence->setup_completed = VAL_FALSE;
        evidence->cache_maintenance_completed = VAL_FALSE;
        evidence->sample_count = 0U;
        evidence->valid_fields = 0U;

        for (index = 0U;
             index < VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES; index++) {
            evidence->stale_read[index] = 0ULL;
            evidence->round_trip[index] = 0ULL;
            evidence->stale_read_fault_esr[index] = 0ULL;
            evidence->round_trip_fault_esr[index] = 0ULL;
        }
    }

    return VAL_FIRME_MEC_OBSERVATION_PROBE_UNAVAILABLE;
}
