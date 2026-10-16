/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#define VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES 8U

#define VAL_FIRME_MEC_EVIDENCE_STATUS            (UINT32_C(1) << 0)
#define VAL_FIRME_MEC_EVIDENCE_MECID             (UINT32_C(1) << 1)
#define VAL_FIRME_MEC_EVIDENCE_TEST_ADDRESS      (UINT32_C(1) << 2)
#define VAL_FIRME_MEC_EVIDENCE_PLAINTEXT         (UINT32_C(1) << 3)
#define VAL_FIRME_MEC_EVIDENCE_SAMPLE_COUNT      (UINT32_C(1) << 4)
#define VAL_FIRME_MEC_EVIDENCE_CACHE_MAINTENANCE (UINT32_C(1) << 5)
#define VAL_FIRME_MEC_EVIDENCE_STALE_READ        (UINT32_C(1) << 6)
#define VAL_FIRME_MEC_EVIDENCE_ROUND_TRIP        (UINT32_C(1) << 7)

#define VAL_FIRME_MEC_OBSERVATION_PROBE_AVAILABLE   0
#define VAL_FIRME_MEC_OBSERVATION_PROBE_UNAVAILABLE 1
#define VAL_FIRME_MEC_OBSERVATION_PROBE_ERROR       (-1)

typedef enum {
    VAL_FIRME_MEC_OBSERVE_CONTEXT_REFRESH = 0,
    VAL_FIRME_MEC_OBSERVE_CIPHERTEXT_DECORRELATION,
    VAL_FIRME_MEC_OBSERVE_CONTEXT_DESTROY,
} val_firme_mec_observation_case_t;

typedef struct {
    int64_t abi_status;
    uint64_t mecid;
    uint64_t test_pa;
    uint64_t plaintext;
    uint64_t stale_read[VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES];
    uint64_t round_trip[VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES];
    uint64_t stale_read_fault_esr[VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES];
    uint64_t round_trip_fault_esr[VAL_FIRME_MEC_OBSERVATION_MAX_SAMPLES];
    uint64_t setup_fault_esr;
    int64_t cleanup_status;
    uint32_t stale_read_completed_mask;
    uint32_t stale_read_fault_mask;
    uint32_t round_trip_completed_mask;
    uint32_t setup_completed;
    uint32_t cache_maintenance_completed;
    uint32_t sample_count;
    uint32_t valid_fields;
} val_firme_mec_observation_evidence_t;

/* Realm linker-provided physical page reserved for indirect MEC observation. */
extern uint8_t __mec_test_pa__[];

/**
 *   @brief    - Report whether an MPE observation adapter is available.
 *   @return   - VAL_TRUE when the adapter is available, otherwise VAL_FALSE.
 **/
uint32_t val_firme_mec_observation_probe_available(void);

/**
 *   @brief    - Exercise a controlled MEC refresh observation scenario.
 *   @param    - test_case : MEC semantic scenario to exercise.
 *   @param    - evidence  : Returned refresh and MPE observation evidence.
 *   @return   - VAL_FIRME_MEC_OBSERVATION_PROBE_*.
 **/
int val_firme_mec_observation_probe(
    val_firme_mec_observation_case_t test_case,
    val_firme_mec_observation_evidence_t *evidence);
