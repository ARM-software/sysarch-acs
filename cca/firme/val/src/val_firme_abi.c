/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_abi.h"
#include "aarch64/sysreg.h"
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

typedef struct {
    uint64_t fid;
    firme_res_t mask;
    firme_res_t value;
    uint64_t owner_mpidr;
    uint32_t enabled;
} val_firme_output_poison_t;

static volatile val_firme_output_poison_t g_val_firme_output_poison;

/**
 *   @brief    - Enable caller-side poisoning for one FIRME ABI on the current PE.
 *   @param    - fid   : FIRME function ID whose result is to be poisoned.
 *   @param    - mask  : Bits replaced in x0-x3.
 *   @param    - value : Replacement values for bits selected by mask.
 *   @return   - VAL_SUCCESS when enabled, otherwise VAL_ERROR.
 **/
int val_firme_test_output_poison_enable(uint64_t fid, const firme_res_t *mask,
                                        const firme_res_t *value)
{
    if (!mask || !value)
        return VAL_ERROR;

    g_val_firme_output_poison.fid = fid;
    g_val_firme_output_poison.mask.x0 = mask->x0;
    g_val_firme_output_poison.mask.x1 = mask->x1;
    g_val_firme_output_poison.mask.x2 = mask->x2;
    g_val_firme_output_poison.mask.x3 = mask->x3;
    g_val_firme_output_poison.value.x0 = value->x0;
    g_val_firme_output_poison.value.x1 = value->x1;
    g_val_firme_output_poison.value.x2 = value->x2;
    g_val_firme_output_poison.value.x3 = value->x3;
    g_val_firme_output_poison.owner_mpidr = read_mpidr_el1();
    g_val_firme_output_poison.enabled = VAL_TRUE;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Disable caller-side FIRME output poisoning.
 *   @return   - void.
 **/
void val_firme_test_output_poison_disable(void)
{
    g_val_firme_output_poison.enabled = VAL_FALSE;
}

/**
 *   @brief    - Replace selected bits in one result word.
 *   @param    - original : Original value returned by the DUT.
 *   @param    - mask     : Bits selected for replacement.
 *   @param    - value    : Replacement values.
 *   @return   - Result containing the selected replacement bits.
 **/
static uint64_t val_firme_output_poison_word(uint64_t original, uint64_t mask,
                                             uint64_t value)
{
    return (original & ~mask) | (value & mask);
}

/**
 *   @brief    - Store an SMC result and apply an enabled caller-side poison pattern.
 *   @param    - fid    : FIRME function ID that produced the result.
 *   @param    - result : Raw SMC result returned by the DUT.
 *   @param    - out    : FIRME result visible to the caller.
 *   @return   - void.
 **/
static void val_firme_store_result(uint64_t fid, const smccc_res_t *result,
                                   firme_res_t *out)
{
    if (!result || !out)
        return;

    out->x0 = result->x0;
    out->x1 = result->x1;
    out->x2 = result->x2;
    out->x3 = result->x3;

    if ((g_val_firme_output_poison.enabled == VAL_TRUE) &&
        (g_val_firme_output_poison.fid == fid) &&
        (g_val_firme_output_poison.owner_mpidr == read_mpidr_el1())) {
        out->x0 = val_firme_output_poison_word(
            out->x0, g_val_firme_output_poison.mask.x0,
            g_val_firme_output_poison.value.x0);
        out->x1 = val_firme_output_poison_word(
            out->x1, g_val_firme_output_poison.mask.x1,
            g_val_firme_output_poison.value.x1);
        out->x2 = val_firme_output_poison_word(
            out->x2, g_val_firme_output_poison.mask.x2,
            g_val_firme_output_poison.value.x2);
        out->x3 = val_firme_output_poison_word(
            out->x3, g_val_firme_output_poison.mask.x3,
            g_val_firme_output_poison.value.x3);
    }
}

/**
 *   @brief    - Convert a FIRME return value into a signed status.
 *   @param    - value : Raw x0 return; the ABI status is the signed 32-bit W0 value.
 *   @return   - Signed FIRME status value.
 **/
int64_t val_firme_status_to_i64(uint64_t value)
{
    /* FIRME defines return status in W0; unused X0 upper bits are ignored. */
    return (int64_t)(int32_t)(uint32_t)value;
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

    service_id &= FIRME_SERVICE_ID_INPUT_MASK;
    val_smccc_smc_call(FIRME_FID_SERVICE_VERSION, service_id, 0, 0, 0, 0, 0, 0, &res);
    val_firme_store_result(FIRME_FID_SERVICE_VERSION, &res, out);
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

    service_id &= FIRME_SERVICE_ID_INPUT_MASK;
    feature_reg_index &= FIRME_FEATURE_REG_INDEX_INPUT_MASK;
    val_smccc_smc_call(FIRME_FID_SERVICE_FEATURES, service_id, feature_reg_index,
                       0, 0, 0, 0, 0, &res);
    val_firme_store_result(FIRME_FID_SERVICE_FEATURES, &res, out);
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

    attributes &= FIRME_GM_GPI_SET_ATTRIBUTES_MASK;
    val_smccc_smc_call(FIRME_FID_GM_GPI_SET, base_address, granule_count, attributes,
                       0, 0, 0, 0, &res);
    val_firme_store_result(FIRME_FID_GM_GPI_SET, &res, out);
}

/**
 *   @brief    - Resume an incomplete GPI transition operation.
 *   @param    - cookie : Cookie returned by a previous GPI management ABI call.
 *   @param    - out    : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_gm_gpi_op_continue(uint64_t cookie, firme_res_t *out)
{
    smccc_res_t res;

    val_smccc_smc_call(FIRME_FID_GM_GPI_OP_CONTINUE, cookie, 0, 0, 0, 0, 0, 0, &res);
    val_firme_store_result(FIRME_FID_GM_GPI_OP_CONTINUE, &res, out);
}

/**
 *   @brief    - Invoke FIRME_MEC_REFRESH from the current world.
 *   @param    - mec_params : Encoded MEC params input value.
 *   @param    - out        : Returned registers (x0-x3).
 *   @return   - void (SMC status is returned in out->x0).
 **/
void val_firme_mec_refresh(uint64_t mec_params, firme_res_t *out)
{
    smccc_res_t res;

    mec_params &= FIRME_MEC_REFRESH_PARAMS_MASK;
    val_smccc_smc_call(FIRME_FID_MEC_REFRESH, mec_params, 0, 0, 0, 0, 0, 0, &res);
    val_firme_store_result(FIRME_FID_MEC_REFRESH, &res, out);
}
