/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_shared_buffer.h"

#include "aarch64/sysreg.h"
#include "val_common/val_status.h"
#include "val_common/val_smccc.h"
#include "val_firme_abi.h"
#include "val_firme_gpt_defs.h"
#include "val_firme_service.h"
#include "val_log.h"

#define VAL_FIRME_SHARED_BUFFER_MIN_4KB  UINT64_C(4096)
#define VAL_FIRME_SHARED_BUFFER_MIN_16KB UINT64_C(16384)
#define VAL_FIRME_SHARED_BUFFER_MIN_64KB UINT64_C(65536)

/**
 *   @brief    - Report worlds with an implemented shared-buffer adapter.
 *   @param    - world : FIRME caller world.
 *   @return   - VAL_TRUE for NS; service and resource availability are probed later.
 **/
__attribute__((weak)) uint32_t
val_firme_shared_buffer_probe_available(firme_acs_world_t world)
{
    return world == FIRME_ACS_WORLD_NSEL2 ? VAL_TRUE : VAL_FALSE;
}

/**
 * @brief Leave buffer allocation and exclusive PAT ownership to the platform.
 * @param capacity Required backed capacity.
 * @param alignment Required alignment.
 * @param lease Returned lease, cleared on unavailable resources.
 * @return PROBE_UNAVAILABLE, or PROBE_ERROR for a null output.
 */
__attribute__((weak)) int val_firme_shared_buffer_acquire_ns_pat(
    uint64_t capacity, uint64_t alignment, val_firme_shared_buffer_lease_t *lease)
{
    (void)capacity;
    (void)alignment;
    if (lease == 0)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *lease = (val_firme_shared_buffer_lease_t){0};
    return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
}

/**
 * @brief Do not invent EL3 mapping, PAS or GPT observations.
 * @param lease Active lease.
 * @param invocation Invocation whose mapping lifetime must be observed.
 * @param observation Returned observation, cleared when unavailable.
 * @return PROBE_UNAVAILABLE, or PROBE_ERROR for invalid arguments.
 */
__attribute__((weak)) int val_firme_shared_buffer_observe_ns_pat(
    const val_firme_shared_buffer_lease_t *lease,
    const val_firme_shared_buffer_evidence_t *invocation,
    val_firme_shared_buffer_evidence_t *observation)
{
    if ((lease == 0) || (invocation == 0) || (observation == 0))
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *observation = (val_firme_shared_buffer_evidence_t){0};
    return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
}

/**
 * @brief Fail closed if a platform supplies acquisition without release support.
 * @param lease Owned resources.
 * @param pending Whether the instance and buffer require quarantine.
 * @return VAL_ERROR.
 */
__attribute__((weak)) int val_firme_shared_buffer_release_ns_pat(
    const val_firme_shared_buffer_lease_t *lease, uint32_t pending)
{
    (void)lease;
    (void)pending;
    return VAL_ERROR;
}

/**
 *   @brief    - Decode the Base service shared-buffer limits.
 *   @param    - minimum_size_bytes : Returned minimum size and alignment.
 *   @param    - maximum_page_count : Returned maximum count-minus-one value.
 *   @param    - maximum_size_bytes : Returned maximum buffer size.
 *   @return   - VAL_SUCCESS when Base feature register 1 is valid, else VAL_ERROR.
 **/
int val_firme_shared_buffer_get_limits(uint64_t *minimum_size_bytes,
                                       uint64_t *maximum_page_count,
                                       uint64_t *maximum_size_bytes)
{
    firme_res_t result = {0};
    uint64_t minimum_encoding = 0ULL;
    uint64_t minimum_size = 0ULL;
    uint64_t maximum_count = 0ULL;

    if ((minimum_size_bytes == 0) || (maximum_page_count == 0) ||
        (maximum_size_bytes == 0))
        return VAL_ERROR;

    val_firme_service_features(FIRME_SERVICE_ID_BASE,
                               FIRME_BASE_FEATURE_REG1_INDEX, &result);
    if (val_firme_status_to_i64(result.x0) != (int64_t)FIRME_SUCCESS)
        return VAL_ERROR;

    minimum_encoding =
        (result.x1 >> FIRME_BASE_MIN_SH_BUF_SZ_SHIFT) &
        FIRME_BASE_MIN_SH_BUF_SZ_MASK;
    switch (minimum_encoding) {
    case 0U:
        minimum_size = VAL_FIRME_SHARED_BUFFER_MIN_4KB;
        break;
    case 1U:
        minimum_size = VAL_FIRME_SHARED_BUFFER_MIN_64KB;
        break;
    case 2U:
        minimum_size = VAL_FIRME_SHARED_BUFFER_MIN_16KB;
        break;
    default:
        return VAL_ERROR;
    }

    maximum_count =
        (result.x1 >> FIRME_BASE_MAX_SH_BUF_PG_CNT_SHIFT) &
        FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK;

    *minimum_size_bytes = minimum_size;
    *maximum_page_count = maximum_count;
    *maximum_size_bytes = (maximum_count + 1ULL) * minimum_size;
    return VAL_SUCCESS;
}

/**
 *   @brief    - Exercise the NS PAT_GET shared-buffer adapter.
 *   @param    - world              : FIRME caller world.
 *   @param    - test_case          : Shared-buffer case to exercise.
 *   @param    - minimum_size_bytes : Base-advertised minimum size.
 *   @param    - maximum_page_count : Base-advertised maximum count-minus-one.
 *   @param    - evidence           : Returned DUT and mapping evidence.
 *   @return   - VAL_FIRME_SHARED_BUFFER_PROBE_*.
 **/
__attribute__((weak)) int val_firme_shared_buffer_probe(
    firme_acs_world_t world,
    val_firme_shared_buffer_case_t test_case,
    uint64_t minimum_size_bytes,
    uint64_t maximum_page_count,
    val_firme_shared_buffer_evidence_t *evidence)
{
    return val_firme_shared_buffer_pat_run(
        world, test_case, minimum_size_bytes, maximum_page_count, evidence,
        VAL_FIRME_PARTIAL_RETRIEVAL_LAST_CHUNK, 0);
}

/**
 * @brief Check the PAT_GET-specific W0 status set from ALP4 Table 8.26.
 * @param status Decoded W0, excluding unused upper bits of X0.
 * @return VAL_TRUE for a permitted PAT_GET status, otherwise VAL_FALSE.
 */
static uint32_t shared_buffer_pat_status_valid(int64_t status)
{
    switch (status) {
    case FIRME_SUCCESS:
    case FIRME_NOT_SUPPORTED:
    case FIRME_INVALID_PARAMETERS:
    case FIRME_NO_MEMORY:
    case FIRME_BUSY:
    case FIRME_ABORTED:
    case FIRME_DENIED:
    case FIRME_INCOMPLETE:
        return VAL_TRUE;
    default:
        return VAL_FALSE;
    }
}

/**
 * @brief Invoke NS PAT_GET with all unused input registers/bits zero.
 * @param base_pa Backed buffer physical address.
 * @param write_offset Offset at which output is written within the buffer.
 * @param page_count Representable count-minus-one encoding.
 * @param challenge_size Nonzero to start, zero to continue an owned retrieval.
 * @param response Decoded defined outputs only, per ALP4 Table 8.26.
 * @return void.
 */
static void shared_buffer_pat_call(uint64_t base_pa, uint64_t write_offset,
                                    uint64_t page_count,
                                    uint32_t challenge_size,
                                    val_firme_partial_retrieval_evidence_t *response)
{
    smccc_call_frame_t input = {0};
    smccc_call_frame_t output = {0};

    input.registers[0] = FIRME_FID_ATTEST_PAT_GET;
    input.registers[1] = base_pa;
    input.registers[2] = write_offset;
    input.registers[3] = page_count;
    input.registers[4] = challenge_size;
    dsb_sy();
    val_smccc_smc_call_frame(&input, &output);
    *response = (val_firme_partial_retrieval_evidence_t){0};
    response->abi_fid = FIRME_FID_ATTEST_PAT_GET;
    response->service_id = FIRME_SERVICE_ID_ATTESTATION;
    response->abi_status = val_firme_status_to_i64(output.registers[0]);
    response->valid_fields = VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_STATUS |
                             VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_SERVICE;
    if (val_firme_shared_buffer_status_accepted(response->abi_status) == VAL_TRUE) {
        response->written_size = output.registers[1];
        response->valid_fields |= VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_WRITTEN;
    }
    if (response->abi_status == (int64_t)FIRME_INCOMPLETE) {
        response->remaining_size = output.registers[2];
        response->valid_fields |= VAL_FIRME_PARTIAL_RETRIEVAL_EVIDENCE_REMAINING;
    }
}

/**
 * @brief Validate defined PAT sizes without interpreting unused error outputs.
 * @param response Decoded invocation.
 * @param capacity Available bytes from Write offset to the end of the buffer.
 * @return VAL_TRUE for valid status/size combinations, otherwise VAL_FALSE.
 */
static uint32_t shared_buffer_pat_response_valid(
    const val_firme_partial_retrieval_evidence_t *response, uint64_t capacity)
{
    if (shared_buffer_pat_status_valid(response->abi_status) != VAL_TRUE)
        return VAL_FALSE;
    if (val_firme_shared_buffer_status_accepted(response->abi_status) != VAL_TRUE)
        return VAL_TRUE;
    if (response->written_size > capacity)
        return VAL_FALSE;
    if (response->abi_status == (int64_t)FIRME_SUCCESS)
        return response->written_size != 0ULL ? VAL_TRUE : VAL_FALSE;
    if ((response->written_size != 0ULL) && (response->remaining_size == 0ULL))
        return VAL_FALSE;
    if (response->remaining_size > UINT64_MAX - response->written_size)
        return VAL_FALSE;
    return VAL_TRUE;
}

/**
 * @brief Merge only independent observations tied to the exact invocation range.
 * @param lease Active resource lease.
 * @param evidence First invocation evidence to augment.
 * @return VAL_SUCCESS for observations or their explicit absence, else VAL_ERROR.
 */
static int shared_buffer_pat_observe(const val_firme_shared_buffer_lease_t *lease,
                                      val_firme_shared_buffer_evidence_t *evidence)
{
    val_firme_shared_buffer_evidence_t observation = {0};
    uint32_t range_fields = VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE |
                            VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT;
    uint32_t observation_fields = VAL_FIRME_SHARED_BUFFER_EVIDENCE_CALLER_ATTRS |
                                  VAL_FIRME_SHARED_BUFFER_EVIDENCE_EL3_ATTRS |
                                  VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAS |
                                  VAL_FIRME_SHARED_BUFFER_EVIDENCE_GPI;
    int result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;

    result = val_firme_shared_buffer_observe_ns_pat(lease, evidence, &observation);
    if (result == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE) {
        evidence->valid_fields |= VAL_FIRME_SHARED_BUFFER_OBSERVATION_UNAVAILABLE;
        return VAL_SUCCESS;
    }
    if ((result != VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE) ||
        ((observation.valid_fields & range_fields) != range_fields) ||
        (observation.base_pa != evidence->base_pa) ||
        (observation.page_count != evidence->page_count))
        return VAL_ERROR;
    evidence->valid_fields |= observation.valid_fields & observation_fields;
    evidence->caller_attributes = observation.caller_attributes;
    evidence->el3_attributes = observation.el3_attributes;
    evidence->pas_world = observation.pas_world;
    evidence->gpi = observation.gpi;
    return VAL_SUCCESS;
}

/**
 * @brief Exercise backed PAT buffer inputs and drain or quarantine owned state.
 * @param world NS caller world.
 * @param test_case Shared-buffer input case.
 * @param minimum_size_bytes Base minimum size and alignment.
 * @param maximum_page_count Base maximum encoded page count.
 * @param evidence First-call evidence, with independent observations if available.
 * @param chunk_case Chunk pattern to retain, without forcing firmware behaviour.
 * @param chunk Optional observed chunk; remains unset if the pattern is absent.
 * @return VAL_FIRME_SHARED_BUFFER_PROBE_*.
 */
int val_firme_shared_buffer_pat_run(
    firme_acs_world_t world, val_firme_shared_buffer_case_t test_case,
    uint64_t minimum_size_bytes, uint64_t maximum_page_count,
    val_firme_shared_buffer_evidence_t *evidence,
    val_firme_partial_retrieval_case_t chunk_case,
    val_firme_partial_retrieval_evidence_t *chunk)
{
    val_firme_shared_buffer_lease_t lease = {0};
    val_firme_partial_retrieval_evidence_t response = {0};
    firme_res_t features = {0};
    uint64_t page_count = 0ULL;
    uint64_t displacement = 0ULL;
    uint64_t size_bytes = 0ULL;
    uint64_t required_capacity = 0ULL;
    uint64_t call_base = 0ULL;
    uint64_t write_offset = 0ULL;
    uint32_t advertised = VAL_FALSE;
    uint32_t pending = VAL_FALSE;
    uint32_t invocation = 0U;
    uint32_t index = 0U;
    uint32_t challenge_size = 0U;
    int result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;

    if (evidence == 0)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *evidence = (val_firme_shared_buffer_evidence_t){0};
    if (chunk != 0)
        *chunk = (val_firme_partial_retrieval_evidence_t){0};
    if (world != FIRME_ACS_WORLD_NSEL2)
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    if (((minimum_size_bytes != VAL_FIRME_SHARED_BUFFER_MIN_4KB) &&
         (minimum_size_bytes != VAL_FIRME_SHARED_BUFFER_MIN_16KB) &&
         (minimum_size_bytes != VAL_FIRME_SHARED_BUFFER_MIN_64KB)) ||
        (maximum_page_count > FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK) ||
        ((chunk != 0) && (chunk_case != VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK) &&
         (chunk_case != VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK)))
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    switch (test_case) {
    case VAL_FIRME_SHARED_BUFFER_CASE_VALID:
    case VAL_FIRME_SHARED_BUFFER_CASE_MINIMUM:
    case VAL_FIRME_SHARED_BUFFER_CASE_ALIGNED:
        break;
    case VAL_FIRME_SHARED_BUFFER_CASE_MAXIMUM:
        page_count = maximum_page_count;
        break;
    case VAL_FIRME_SHARED_BUFFER_CASE_ABOVE_MAXIMUM:
        if (maximum_page_count == FIRME_BASE_MAX_SH_BUF_PG_CNT_MASK)
            return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
        page_count = maximum_page_count + 1ULL;
        break;
    case VAL_FIRME_SHARED_BUFFER_CASE_MISALIGNED:
        displacement = 1ULL;
        break;
    default:
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    }
    if (val_firme_service_abi_is_advertised(FIRME_SERVICE_ID_ATTESTATION,
            FIRME_ATTEST_FEATURE_PAT_GET_BIT, &advertised, 0, &features) != VAL_SUCCESS)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    LOG_DEBUG(NULL, "PAT_GET advertised=%u discovery x0=%llx x1=%llx",
              advertised, (unsigned long long)features.x0, (unsigned long long)features.x1);
    if (advertised != VAL_TRUE)
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    size_bytes = (page_count + 1ULL) * minimum_size_bytes;
    required_capacity = size_bytes + displacement;
    result = val_firme_shared_buffer_acquire_ns_pat(
        required_capacity, minimum_size_bytes, &lease);
    if (result != VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE)
        return result == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE ? result :
                   VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    if ((lease.base_va == 0) || (lease.challenge == 0) || (lease.challenge_size == 0U) ||
        (lease.challenge_size > minimum_size_bytes) ||
        (lease.capacity < required_capacity) ||
        (lease.base_pa % minimum_size_bytes != 0ULL) ||
        (lease.base_pa > UINT64_MAX - required_capacity) ||
        (required_capacity > UINTPTR_MAX) ||
        ((uintptr_t)lease.base_va > UINTPTR_MAX - required_capacity) ||
        ((uintptr_t)lease.challenge > UINTPTR_MAX - lease.challenge_size) ||
        (((uintptr_t)lease.challenge < (uintptr_t)lease.base_va + required_capacity) &&
         ((uintptr_t)lease.challenge + lease.challenge_size > (uintptr_t)lease.base_va)))
        goto release;
    for (index = 0U; index < lease.challenge_size; index++)
        lease.base_va[displacement + index] = lease.challenge[index];
    call_base = lease.base_pa + displacement;
    challenge_size = lease.challenge_size;
    if ((chunk != 0) && (chunk_case == VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK))
        write_offset = size_bytes - 1ULL;
    evidence->base_pa = call_base;
    evidence->page_count = page_count;
    evidence->service_id = FIRME_SERVICE_ID_ATTESTATION;
    evidence->valid_fields = VAL_FIRME_SHARED_BUFFER_EVIDENCE_BASE |
                             VAL_FIRME_SHARED_BUFFER_EVIDENCE_PAGE_COUNT |
                             VAL_FIRME_SHARED_BUFFER_EVIDENCE_SERVICE;
    result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
    for (invocation = 0U; invocation < VAL_FIRME_PAT_MAX_INVOCATIONS; invocation++) {
        shared_buffer_pat_call(call_base, write_offset, page_count, challenge_size, &response);
        LOG_DEBUG(NULL, "PAT_GET base=%llx offset=%llx pages=%llu challenge=%u status=%lld "
                        "written=%llu remaining=%llu fields=%x",
                  (unsigned long long)call_base, (unsigned long long)write_offset,
                  (unsigned long long)page_count, challenge_size,
                  (long long)response.abi_status, (unsigned long long)response.written_size,
                  (unsigned long long)response.remaining_size, response.valid_fields);
        if ((response.abi_status == (int64_t)FIRME_SUCCESS) ||
            (response.abi_status == (int64_t)FIRME_ABORTED))
            pending = VAL_FALSE;
        else if ((response.abi_status == (int64_t)FIRME_INCOMPLETE) ||
                 (shared_buffer_pat_status_valid(response.abi_status) != VAL_TRUE))
            pending = VAL_TRUE;
        if (invocation == 0U) {
            evidence->abi_status = response.abi_status;
            evidence->valid_fields |= VAL_FIRME_SHARED_BUFFER_EVIDENCE_STATUS;
            if (shared_buffer_pat_observe(&lease, evidence) != VAL_SUCCESS)
                result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
        }
        if (shared_buffer_pat_response_valid(&response, size_bytes - write_offset) != VAL_TRUE) {
            if (result != VAL_FIRME_SHARED_BUFFER_PROBE_ERROR)
                result = VAL_FIRME_SHARED_BUFFER_PROBE_FAILED;
            if (chunk != 0)
                *chunk = response;
        } else if ((chunk != 0) && (chunk->valid_fields == 0U) &&
                   (response.abi_status == (int64_t)FIRME_INCOMPLETE)) {
            if (((chunk_case == VAL_FIRME_PARTIAL_RETRIEVAL_INTERMEDIATE_CHUNK) &&
                 (response.written_size != 0ULL) && (response.remaining_size != 0ULL)) ||
                ((chunk_case == VAL_FIRME_PARTIAL_RETRIEVAL_NO_CHUNK) &&
                 (response.written_size == 0ULL) && (response.remaining_size == 0ULL)))
                *chunk = response;
        }
        if (pending == VAL_FALSE)
            break;
        call_base = lease.base_pa;
        page_count = 0ULL;
        size_bytes = minimum_size_bytes;
        challenge_size = 0U;
        write_offset = 0ULL;
    }
release:
    if (val_firme_shared_buffer_release_ns_pat(&lease, pending) != VAL_SUCCESS)
        result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    if (pending == VAL_TRUE)
        result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    return result;
}

/**
 *   @brief    - Return the shared-buffer GPI required for a FIRME world.
 *   @param    - world : FIRME caller world.
 *   @return   - FIRME_GPI_* value, or zero for an unsupported world.
 **/
uint32_t val_firme_shared_buffer_expected_gpi(firme_acs_world_t world)
{
    switch (world) {
    case FIRME_ACS_WORLD_NSEL2:
        return FIRME_GPI_NONSECURE;
    case FIRME_ACS_WORLD_SECURE:
        return FIRME_GPI_SECURE;
    case FIRME_ACS_WORLD_REALM:
        return FIRME_GPI_REALM;
    default:
        return 0U;
    }
}

/**
 *   @brief    - Check whether a valid shared-buffer operation reached data handling.
 *   @param    - status : FIRME ABI status.
 *   @return   - VAL_TRUE for SUCCESS or INCOMPLETE, otherwise VAL_FALSE.
 **/
uint32_t val_firme_shared_buffer_status_accepted(int64_t status)
{
    if ((status == (int64_t)FIRME_SUCCESS) ||
        (status == (int64_t)FIRME_INCOMPLETE))
        return VAL_TRUE;

    return VAL_FALSE;
}
