/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_firme_dispatch.h"
#include "val_firme_partial_retrieval.h"

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
#define VAL_FIRME_SHARED_BUFFER_OBSERVATION_UNAVAILABLE (UINT32_C(1) << 8)

#define VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE   0
#define VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE 1
#define VAL_FIRME_SHARED_BUFFER_PROBE_ERROR       (-1)
#define VAL_FIRME_SHARED_BUFFER_PROBE_FAILED      2
#define VAL_FIRME_PAT_MAX_INVOCATIONS             64U
#define VAL_FIRME_PAT_NO_SERVICE                  1U
#define VAL_FIRME_PAT_NO_BOOT_BUFFER              2U
#define VAL_FIRME_PAT_BOOT_MAPPING_UNAVAILABLE    3U
#define VAL_FIRME_PAT_BACKING_TOO_SMALL           4U
#define VAL_FIRME_PAT_INSTANCE_UNAVAILABLE        5U
#define VAL_FIRME_PAT_NO_WORKLOAD_BINDING          6U
#define VAL_FIRME_PAT_WORKLOAD_BINDING_OFFSET     UINT64_C(0x40)

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
    uint64_t prerequisite_reason;
    uint64_t prerequisite_detail;
} val_firme_shared_buffer_evidence_t;

typedef struct {
    uint8_t *base_va;
    uint64_t base_pa;
    uint64_t capacity;
    const uint8_t *challenge;
    uint32_t challenge_size;
    const uint8_t *workload_binding;
    uint32_t workload_binding_size;
    uint64_t owner_token;
    uint64_t prerequisite_reason;
    uint64_t prerequisite_detail;
} val_firme_shared_buffer_lease_t;

/** @brief Retain the supported EL3/Realm boot handoff's shared page after cold boot. */
void val_firme_shared_buffer_init_realm_pat(uint64_t boot_version, uint64_t base_pa);

/**
 * @brief Supply the real workload binding for the externally selected attestation model.
 * @param binding Base-model RAT hash or delegated-model current public RAK hash.
 * @param size Binding size, held stable outside the shared buffer until release.
 * @return PROBE_AVAILABLE, UNAVAILABLE without a valid fixture, or ERROR.
 * Model selection is outside FIRME discovery (I0248). Arbitrary challenge
 * bytes or absent RAK advertisement cannot establish this prerequisite.
 */
int val_firme_shared_buffer_get_realm_binding(const uint8_t **binding, uint32_t *size);

/**
 * @brief Acquire exclusive ownership of an idle NS PAT instance and its buffer.
 * @param capacity Required contiguous, backed buffer capacity, including padding.
 * @param alignment Required physical alignment.
 * @param lease Returned mapping and separate, valid CCA platform challenge.
 * The platform must establish a safe coherent caller mapping and NS ownership,
 * exclude other PAT callers until release, and keep the challenge outside the
 * buffer. These prerequisites are not independent R0070/R0071 evidence.
 * @return PROBE_AVAILABLE with a lease, UNAVAILABLE without one, or ERROR.
 * No ownership may be transferred on an unsuccessful acquisition.
 */
int val_firme_shared_buffer_acquire_ns_pat(uint64_t capacity, uint64_t alignment,
                                          val_firme_shared_buffer_lease_t *lease);

/**
 * @brief Obtain independent mapping/PAS/GPT observations for the first PAT call.
 * @param lease Active buffer lease.
 * @param invocation Exact buffer range and status of that invocation.
 * @param observation Independently observed attributes, PAS and GPI, with flags.
 * Evidence must describe the entire invocation range during the SMC, including
 * any temporary EL3 mapping, not be inferred from SUCCESS or the caller world.
 * Available evidence must identify matching BASE and PAGE_COUNT fields.
 * @return PROBE_AVAILABLE, UNAVAILABLE for incomplete observations, or ERROR.
 * Partial observations may accompany UNAVAILABLE, but each supplied field must
 * be independently measured and identify the matching BASE and PAGE_COUNT.
 */
int val_firme_shared_buffer_observe_ns_pat(
    const val_firme_shared_buffer_lease_t *lease,
    const val_firme_shared_buffer_evidence_t *invocation,
    val_firme_shared_buffer_evidence_t *observation);

/**
 * @brief Release a PAT lease, or quarantine it if retrieval remains pending.
 * @param lease Owned buffer and instance lease.
 * @param pending VAL_TRUE requires durable quarantine of the buffer and instance;
 *                do not free or reassign them without confirmed completion.
 * @return VAL_SUCCESS when released/quarantined, otherwise VAL_ERROR.
 */
int val_firme_shared_buffer_release_ns_pat(
    const val_firme_shared_buffer_lease_t *lease, uint32_t pending);

/** @brief Acquire the Realm PAT instance and its firmware-established buffer. */
int val_firme_shared_buffer_acquire_realm_pat(uint64_t capacity, uint64_t alignment,
                                             val_firme_shared_buffer_lease_t *lease);

/** @brief Release or quarantine the exact owned Realm PAT lease. */
int val_firme_shared_buffer_release_realm_pat(
    const val_firme_shared_buffer_lease_t *lease, uint32_t pending);

/** @brief Obtain independent invocation-time Realm shared-buffer evidence. */
int val_firme_shared_buffer_observe_realm_pat(
    const val_firme_shared_buffer_lease_t *lease,
    const val_firme_shared_buffer_evidence_t *invocation,
    val_firme_shared_buffer_evidence_t *observation);

/**
 * @brief Run a real PAT_GET buffer case and drain its owned retrieval.
 * @param world NS or Realm caller world, with separately owned resources.
 * @param test_case Buffer case, with safe backing even for negative inputs.
 * @param minimum_size_bytes Base minimum size/alignment.
 * @param maximum_page_count Base maximum count-minus-one encoding.
 * @param evidence First invocation evidence; unavailable observations stay unset.
 * @param chunk_case Optional chunk scenario to retain while draining.
 * @param chunk Optional observed chunk, never inferred from unused registers.
 * @return PROBE_AVAILABLE, UNAVAILABLE, FAILED for malformed DUT output, or ERROR
 *         for resource/cleanup failure. Pending retrieval is never success.
 */
int val_firme_shared_buffer_pat_run(
    firme_acs_world_t world, val_firme_shared_buffer_case_t test_case,
    uint64_t minimum_size_bytes, uint64_t maximum_page_count,
    val_firme_shared_buffer_evidence_t *evidence,
    val_firme_partial_retrieval_case_t chunk_case,
    val_firme_partial_retrieval_evidence_t *chunk);

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
