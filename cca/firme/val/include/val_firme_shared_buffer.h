/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_dispatch.h"

#define VAL_FIRME_SHARED_BUFFER_ATTR_NORMAL          (UINT32_C(1) << 0)
#define VAL_FIRME_SHARED_BUFFER_ATTR_WRITE_BACK      (UINT32_C(1) << 1)
#define VAL_FIRME_SHARED_BUFFER_ATTR_INNER_SHAREABLE (UINT32_C(1) << 2)
#define VAL_FIRME_SHARED_BUFFER_REQUIRED_ATTRS \
    (VAL_FIRME_SHARED_BUFFER_ATTR_NORMAL | \
     VAL_FIRME_SHARED_BUFFER_ATTR_WRITE_BACK | \
     VAL_FIRME_SHARED_BUFFER_ATTR_INNER_SHAREABLE)

#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_STATUS       (UINT32_C(1) << 0)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_CALLER_ATTRS (UINT32_C(1) << 1)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_EL3_ATTRS    (UINT32_C(1) << 2)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAS          (UINT32_C(1) << 3)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_GPI          (UINT32_C(1) << 4)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE         (UINT32_C(1) << 5)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT   (UINT32_C(1) << 6)
#define VAL_FIRME_SHARED_BUFFER_EVIDENCE_SERVICE      (UINT32_C(1) << 7)

#define VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE   0
#define VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE 1
#define VAL_FIRME_SHARED_BUFFER_PROBE_ERROR       (-1)

typedef enum {
    VAL_FIRME_SHARED_BUFFER_CASE_VALID = 0,
    VAL_FIRME_SHARED_BUFFER_CASE_MINIMUM,
    VAL_FIRME_SHARED_BUFFER_CASE_MAXIMUM,
    VAL_FIRME_SHARED_BUFFER_CASE_ABOVE_MAXIMUM,
    VAL_FIRME_SHARED_BUFFER_CASE_ALIGNED,
    VAL_FIRME_SHARED_BUFFER_CASE_MISALIGNED,
} val_firme_shared_buffer_case_t;

typedef struct {
    uint64_t base_pa;
    uint64_t page_count;
    int64_t abi_status;
    uint32_t caller_attributes;
    uint32_t el3_attributes;
    firme_acs_world_t pas_world;
    uint32_t gpi;
    uint32_t service_id;
    uint32_t valid_fields;
} val_firme_shared_buffer_evidence_t;

/**
 *   @brief    - Report whether a service-specific shared-buffer probe exists.
 *   @param    - world : FIRME caller world.
 *   @return   - VAL_TRUE when a probe is available, otherwise VAL_FALSE.
 **/
uint32_t val_firme_shared_buffer_probe_available(firme_acs_world_t world);

/**
 *   @brief    - Decode the Base service shared-buffer limits.
 *   @param    - minimum_size_bytes : Returned minimum size and alignment.
 *   @param    - maximum_page_count : Returned maximum count-minus-one value.
 *   @param    - maximum_size_bytes : Returned maximum buffer size.
 *   @return   - VAL_SUCCESS when Base feature register 1 is valid, else VAL_ERROR.
 **/
int val_firme_shared_buffer_get_limits(uint64_t *minimum_size_bytes,
                                       uint64_t *maximum_page_count,
                                       uint64_t *maximum_size_bytes);

/**
 *   @brief    - Exercise a service ABI with a controlled shared-buffer case.
 *   @param    - world              : FIRME caller world.
 *   @param    - test_case          : Shared-buffer case to exercise.
 *   @param    - minimum_size_bytes : Base-advertised minimum size.
 *   @param    - maximum_page_count : Base-advertised maximum count-minus-one.
 *   @param    - evidence           : Returned DUT and mapping evidence.
 *   @return   - VAL_FIRME_SHARED_BUFFER_PROBE_*.
 **/
int val_firme_shared_buffer_probe(
    firme_acs_world_t world,
    val_firme_shared_buffer_case_t test_case,
    uint64_t minimum_size_bytes,
    uint64_t maximum_page_count,
    val_firme_shared_buffer_evidence_t *evidence);

/**
 *   @brief    - Return the shared-buffer GPI required for a FIRME world.
 *   @param    - world : FIRME caller world.
 *   @return   - FIRME_GPI_* value, or zero for an unsupported world.
 **/
uint32_t val_firme_shared_buffer_expected_gpi(firme_acs_world_t world);

/**
 *   @brief    - Check whether a valid shared-buffer operation reached data handling.
 *   @param    - status : FIRME ABI status.
 *   @return   - VAL_TRUE for SUCCESS or INCOMPLETE, otherwise VAL_FALSE.
 **/
uint32_t val_firme_shared_buffer_status_accepted(int64_t status);
