/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"

#define VAL_FIRME_STATUS_CODE_COUNT 12U

static const int64_t g_val_firme_status_codes[VAL_FIRME_STATUS_CODE_COUNT] = {
    FIRME_SUCCESS,
    FIRME_NOT_SUPPORTED,
    FIRME_INVALID_PARAMETERS,
    FIRME_ABORTED,
    FIRME_INCOMPLETE,
    FIRME_DENIED,
    FIRME_BUSY,
    FIRME_OP_CONFLICT,
    FIRME_ALREADY_EXISTS,
    FIRME_NOT_FOUND,
    FIRME_NO_MEMORY,
    FIRME_BAD_DATA,
};

/**
 *   @brief    - Convert a FIRME return value into a signed status.
 *   @param    - value : Raw x0 return, possibly transported as zero-extended SMC32.
 *   @return   - Signed FIRME status value.
 **/
int64_t val_firme_status_to_i64(uint64_t value)
{
    uint32_t lower = (uint32_t)value;

    if (((value >> 32) == 0ULL) && ((lower & 0x80000000U) != 0U))
        return (int64_t)(int32_t)lower;

    return (int64_t)value;
}

/**
 *   @brief    - Check whether a FIRME status value is architected.
 *   @param    - status : Signed FIRME status value.
 *   @return   - VAL_TRUE if the status is architected, otherwise VAL_FALSE.
 **/
int val_firme_status_is_architected(int64_t status)
{
    uint32_t index = 0U;

    for (index = 0U; index < VAL_FIRME_STATUS_CODE_COUNT; index++) {
        if (g_val_firme_status_codes[index] == status)
            return VAL_TRUE;
    }

    return VAL_FALSE;
}

/**
 *   @brief    - Return the Base feature-register service-list bit for a service ID.
 *   @param    - service_id : FIRME service ID.
 *   @return   - Bit mask for the service, or zero for IDs without a list bit.
 **/
uint64_t val_firme_base_service_list_bit(uint32_t service_id)
{
    uint32_t shift = 0U;

    if ((service_id <= FIRME_SERVICE_ID_BASE) || (service_id >= FIRME_SERVICE_ID_MAX))
        return 0ULL;

    shift = FIRME_BASE_SERVICE_LIST_SHIFT + (service_id - 1U);
    if (shift >= 64U)
        return 0ULL;

    return UINT64_C(1) << shift;
}

/**
 *   @brief    - Invoke FIRME_SERVICE_VERSION from the current world.
 *   @param    - service_id : FIRME service ID.
 *   @param    - out        : Returned registers (x0-x3).
 *   @return   - void (SMC status/version is returned in out->x0).
 **/
void val_firme_service_version(uint64_t service_id, firme_res_t *out)
{
    smccc_res_t res;

    val_smccc_smc_call(FIRME_FID_SERVICE_VERSION, service_id, 0, 0, 0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_SERVICE_FEATURES from the current world.
 *   @param    - service_id        : FIRME service ID.
 *   @param    - feature_reg_index : FIRME feature register index.
 *   @param    - out               : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_service_features(uint64_t service_id, uint64_t feature_reg_index,
                                firme_res_t *out)
{
    smccc_res_t res;

    val_smccc_smc_call(FIRME_FID_SERVICE_FEATURES, service_id, feature_reg_index,
                       0, 0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}

/**
 *   @brief    - Invoke FIRME_SERVICE_VERSION for the Base service.
 *   @param    - out : Returned registers (x0-x3).
 *   @return   - void (SMC status/version is returned in out->x0).
 **/
void val_firme_version(firme_res_t *out)
{
    val_firme_service_version(FIRME_SERVICE_ID_BASE, out);
}

/**
 *   @brief    - Invoke FIRME_SERVICE_FEATURES for the Base service.
 *   @param    - feature_id : FIRME Base service feature register index.
 *   @param    - out        : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_features(uint64_t feature_id, firme_res_t *out)
{
    val_firme_service_features(FIRME_SERVICE_ID_BASE, feature_id, out);
}

/**
 *   @brief    - Invoke FIRME_GM_GPI_SET from the current world.
 *   @param    - base_address  : Base PA to transition.
 *   @param    - granule_count : Number of granules (FVP currently supports 1).
 *   @param    - attributes    : Encodes the target GPI (and any future attributes).
 *   @param    - out           : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_gm_gpi_set(uint64_t base_address, uint64_t granule_count, uint64_t attributes,
                          firme_res_t *out)
{
    smccc_res_t res;

    val_smccc_smc_call(FIRME_FID_GM_GPI_SET, base_address, granule_count, attributes,
                       0, 0, 0, 0, &res);
    if (out) {
        out->x0 = res.x0;
        out->x1 = res.x1;
        out->x2 = res.x2;
        out->x3 = res.x3;
    }
}
