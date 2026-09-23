/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_spec_policy.h"

/**
 *   @brief    - Default specification release-metadata probe.
 *   @param    - evidence : Returned previous and current release metadata.
 *   @return   - VAL_FIRME_SPEC_POLICY_PROBE_UNAVAILABLE.
 **/
__attribute__((weak)) int
val_firme_spec_policy_probe(val_firme_spec_policy_evidence_t *evidence)
{
    uint32_t index = 0U;

    if (evidence != 0) {
        evidence->previous.major_version = 0U;
        evidence->previous.minor_version = 0U;
        evidence->previous.service_count = 0U;
        evidence->current.major_version = 0U;
        evidence->current.minor_version = 0U;
        evidence->current.service_count = 0U;

        for (index = 0U; index < VAL_FIRME_SPEC_POLICY_MAX_SERVICES; index++) {
            evidence->previous.services[index].service_id = 0U;
            evidence->previous.services[index].major_version = 0U;
            evidence->previous.services[index].minor_version = 0U;
            evidence->current.services[index].service_id = 0U;
            evidence->current.services[index].major_version = 0U;
            evidence->current.services[index].minor_version = 0U;
        }
    }

    return VAL_FIRME_SPEC_POLICY_PROBE_UNAVAILABLE;
}
