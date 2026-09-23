/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#define VAL_FIRME_SPEC_POLICY_MAX_SERVICES 6U

#define VAL_FIRME_SPEC_POLICY_PROBE_AVAILABLE   0
#define VAL_FIRME_SPEC_POLICY_PROBE_UNAVAILABLE 1
#define VAL_FIRME_SPEC_POLICY_PROBE_ERROR       (-1)

typedef struct {
    uint32_t service_id;
    uint32_t major_version;
    uint32_t minor_version;
} val_firme_spec_policy_service_t;

typedef struct {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t service_count;
    val_firme_spec_policy_service_t services[VAL_FIRME_SPEC_POLICY_MAX_SERVICES];
} val_firme_spec_policy_release_t;

typedef struct {
    val_firme_spec_policy_release_t previous;
    val_firme_spec_policy_release_t current;
} val_firme_spec_policy_evidence_t;

/**
 *   @brief    - Supply consecutive FIRME specification release metadata.
 *   @param    - evidence : Returned previous and current release metadata.
 *   @return   - VAL_FIRME_SPEC_POLICY_PROBE_*.
 **/
int val_firme_spec_policy_probe(val_firme_spec_policy_evidence_t *evidence);
