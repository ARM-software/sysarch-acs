/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_shared_buffer.h"

#include "val_common/val_status.h"
#include "val_firme_abi.h"
#include "val_firme_gpt_defs.h"

#define VAL_FIRME_SHARED_BUFFER_MIN_4KB  UINT64_C(4096)
#define VAL_FIRME_SHARED_BUFFER_MIN_16KB UINT64_C(16384)
#define VAL_FIRME_SHARED_BUFFER_MIN_64KB UINT64_C(65536)

/**
 *   @brief    - Default shared-buffer probe availability hook.
 *   @param    - world : FIRME caller world.
 *   @return   - VAL_FALSE until a service-specific probe overrides this hook.
 **/
__attribute__((weak)) uint32_t
val_firme_shared_buffer_probe_available(firme_acs_world_t world)
{
    (void)world;
    return VAL_FALSE;
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
 *   @brief    - Default service-specific shared-buffer probe hook.
 *   @param    - world              : FIRME caller world.
 *   @param    - test_case          : Shared-buffer case to exercise.
 *   @param    - minimum_size_bytes : Base-advertised minimum size.
 *   @param    - maximum_page_count : Base-advertised maximum count-minus-one.
 *   @param    - evidence           : Returned DUT and mapping evidence.
 *   @return   - VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE.
 **/
__attribute__((weak)) int val_firme_shared_buffer_probe(
    firme_acs_world_t world,
    val_firme_shared_buffer_case_t test_case,
    uint64_t minimum_size_bytes,
    uint64_t maximum_page_count,
    val_firme_shared_buffer_evidence_t *evidence)
{
    (void)world;
    (void)test_case;
    (void)minimum_size_bytes;
    (void)maximum_page_count;

    if (evidence != 0) {
        evidence->base_pa = 0ULL;
        evidence->page_count = 0ULL;
        evidence->abi_status = (int64_t)FIRME_NOT_SUPPORTED;
        evidence->caller_attributes = 0U;
        evidence->el3_attributes = 0U;
        evidence->pas_world = FIRME_ACS_WORLD_NSEL2;
        evidence->gpi = 0U;
        evidence->service_id = FIRME_SERVICE_ID_MAX;
        evidence->valid_fields = 0U;
    }

    return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
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
