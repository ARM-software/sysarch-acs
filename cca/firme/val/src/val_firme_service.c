/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_service.h"

#include "val_firme_abi.h"
#include "val_common/val_status.h"

#define VAL_FIRME_SERVICE_WORLD_NS     (UINT32_C(1) << 0)
#define VAL_FIRME_SERVICE_WORLD_SECURE (UINT32_C(1) << 1)
#define VAL_FIRME_SERVICE_WORLD_REALM  (UINT32_C(1) << 2)

typedef struct {
    uint32_t service_id;
    uint32_t allowed_worlds;
    uint64_t allocated_abi_mask;
    uint64_t mandatory_ns_mask;
    uint64_t mandatory_secure_mask;
    uint64_t mandatory_realm_mask;
    uint64_t optional_ns_mask;
    uint64_t optional_secure_mask;
    uint64_t optional_realm_mask;
} val_firme_service_desc_t;

static const val_firme_service_desc_t g_val_firme_service_desc[] = {
    {
        .service_id = FIRME_SERVICE_ID_BASE,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_SECURE |
                          VAL_FIRME_SERVICE_WORLD_REALM,
        .allocated_abi_mask = FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
        .mandatory_ns_mask = FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
        .mandatory_secure_mask = FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
        .mandatory_realm_mask = FIRME_BASE_FEATURE_REG0_MANDATORY_BITS,
    },
    {
        .service_id = FIRME_SERVICE_ID_GRANULE_MGMT,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_SECURE |
                          VAL_FIRME_SERVICE_WORLD_REALM,
        .allocated_abi_mask = FIRME_GM_FEATURE_REG0_ALLOCATED_BITS,
        .mandatory_ns_mask = FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
                             FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT,
        .mandatory_secure_mask = FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
                                 FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT,
        .mandatory_realm_mask = FIRME_GM_FEATURE_REG0_GPI_SET_BIT |
                                FIRME_GM_FEATURE_REG0_OP_CONTINUE_BIT,
        .optional_realm_mask = FIRME_GM_FEATURE_REG0_L1_CREATE_BIT |
                               FIRME_GM_FEATURE_REG0_L1_DESTROY_BIT,
    },
    {
        .service_id = FIRME_SERVICE_ID_IDE_KEY_MGMT,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_REALM,
        .allocated_abi_mask = FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS,
        .mandatory_ns_mask = FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS,
        .mandatory_realm_mask = FIRME_IDE_FEATURE_REG0_ALL_ABI_BITS,
    },
    {
        .service_id = FIRME_SERVICE_ID_MECID_MGMT,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_REALM,
        .allocated_abi_mask = FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT,
        .mandatory_realm_mask = FIRME_MECID_FEATURE_REG0_MEC_REFRESH_BIT,
    },
    {
        .service_id = FIRME_SERVICE_ID_ATTESTATION,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_REALM,
        .allocated_abi_mask = FIRME_ATTEST_FEATURE_REG0_ALL_ABI_BITS,
        .mandatory_ns_mask = FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT,
        .mandatory_realm_mask = FIRME_ATTEST_FEATURE_REG0_PAT_GET_BIT |
                                FIRME_ATTEST_FEATURE_REG0_RAK_GET_BIT,
        .optional_realm_mask = FIRME_ATTEST_FEATURE_REG0_RAT_SIGN_BIT |
                               FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_STAGE_BIT |
                               FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_FINAL_BIT |
                               FIRME_ATTEST_FEATURE_REG0_EXT_CLAIMS_CLEAR_BIT,
    },
    {
        .service_id = FIRME_SERVICE_ID_INTEGRATED_DEVICE,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_REALM,
        .allocated_abi_mask = FIRME_IDEV_FEATURE_REG0_ALL_ABI_BITS,
        .mandatory_realm_mask = FIRME_IDEV_FEATURE_REG0_ALL_ABI_BITS,
    },
};


/**
 *   @brief    - Query whether a FIRME service is implemented by the current instance.
 *   @param    - service_id   : FIRME service ID.
 *   @param    - implemented  : Set to VAL_TRUE when SERVICE_VERSION reports a version.
 *   @param    - version_res  : Optional raw SERVICE_VERSION result.
 *   @return   - VAL_SUCCESS for implemented or cleanly absent services, else VAL_ERROR.
 **/
int val_firme_service_get_version(uint32_t service_id, uint32_t *implemented,
                                  firme_res_t *version_res)
{
    firme_res_t local_res = {0};
    int64_t status = 0;

    if (implemented)
        *implemented = VAL_FALSE;

    val_firme_service_version((uint64_t)service_id, &local_res);
    if (version_res)
        *version_res = local_res;

    status = val_firme_status_to_i64(local_res.x0);
    if (status == (int64_t)FIRME_NOT_SUPPORTED)
        return VAL_SUCCESS;

    if (status <= 0)
        return VAL_ERROR;

    if (implemented)
        *implemented = VAL_TRUE;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Query whether feature register 0 is advertised for a FIRME service.
 *   @param    - service_id    : FIRME service ID.
 *   @param    - advertised    : Set to VAL_TRUE when SERVICE_FEATURES(reg0) succeeds.
 *   @param    - feature_reg0  : Optional feature register 0 value.
 *   @param    - features_res  : Optional raw SERVICE_FEATURES result.
 *   @return   - VAL_SUCCESS for advertised or cleanly absent registers, else VAL_ERROR.
 **/
int val_firme_service_get_feature_reg0(uint32_t service_id, uint32_t *advertised,
                                       uint64_t *feature_reg0, firme_res_t *features_res)
{
    firme_res_t local_res = {0};
    firme_res_t version_res = {0};
    uint32_t implemented = VAL_FALSE;
    int64_t status = 0;

    if (advertised)
        *advertised = VAL_FALSE;

    if (feature_reg0)
        *feature_reg0 = 0ULL;

    if (val_firme_service_get_version(service_id, &implemented, &version_res) != VAL_SUCCESS) {
        if (features_res)
            *features_res = version_res;
        return VAL_ERROR;
    }

    if (implemented == VAL_FALSE) {
        if (features_res)
            *features_res = version_res;
        return VAL_SUCCESS;
    }

    val_firme_service_features((uint64_t)service_id, FIRME_BASE_FEATURE_REG0_INDEX,
                               &local_res);
    if (features_res)
        *features_res = local_res;

    status = val_firme_status_to_i64(local_res.x0);
    if (status != (int64_t)FIRME_SUCCESS)
        return VAL_ERROR;

    if (advertised)
        *advertised = VAL_TRUE;

    if (feature_reg0)
        *feature_reg0 = local_res.x1;

    return VAL_SUCCESS;
}

/**
 *   @brief    - Query whether a FIRME service ABI bit is advertised by the current instance.
 *   @param    - service_id    : FIRME service ID.
 *   @param    - abi_mask      : ABI bit mask to query in feature register 0.
 *   @param    - advertised    : Set to VAL_TRUE when all bits in abi_mask are advertised.
 *   @param    - feature_reg0  : Optional feature register 0 value.
 *   @param    - features_res  : Optional raw SERVICE_FEATURES result.
 *   @return   - VAL_SUCCESS for advertised or cleanly absent ABIs, else VAL_ERROR.
 **/
int val_firme_service_abi_is_advertised(uint32_t service_id, uint64_t abi_mask,
                                        uint32_t *advertised, uint64_t *feature_reg0,
                                        firme_res_t *features_res)
{
    uint64_t local_feature_reg0 = 0ULL;
    uint32_t service_advertised = VAL_FALSE;

    if (advertised)
        *advertised = VAL_FALSE;

    if (val_firme_service_get_feature_reg0(service_id, &service_advertised,
                                           &local_feature_reg0, features_res) != VAL_SUCCESS)
        return VAL_ERROR;

    if (feature_reg0)
        *feature_reg0 = local_feature_reg0;

    if ((service_advertised == VAL_TRUE) && ((local_feature_reg0 & abi_mask) == abi_mask)) {
        if (advertised)
            *advertised = VAL_TRUE;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    - Map a dispatcher world to a service-table world mask.
 *   @param    - world : Caller world.
 *   @return   - Internal world mask, or 0 if unknown.
 **/
static uint32_t val_firme_service_world_mask(firme_acs_world_t world)
{
    if (world == FIRME_ACS_WORLD_NSEL2)
        return VAL_FIRME_SERVICE_WORLD_NS;

    if (world == FIRME_ACS_WORLD_SECURE)
        return VAL_FIRME_SERVICE_WORLD_SECURE;

    if (world == FIRME_ACS_WORLD_REALM)
        return VAL_FIRME_SERVICE_WORLD_REALM;

    return 0U;
}

/**
 *   @brief    - Find the service descriptor for a FIRME service ID.
 *   @param    - service_id : FIRME service ID.
 *   @return   - Service descriptor, or NULL if unknown.
 **/
static const val_firme_service_desc_t *val_firme_service_desc(uint32_t service_id)
{
    uint32_t index = 0U;
    uint32_t count = (uint32_t)(sizeof(g_val_firme_service_desc) /
                               sizeof(g_val_firme_service_desc[0]));

    for (index = 0U; index < count; index++) {
        if (g_val_firme_service_desc[index].service_id == service_id)
            return &g_val_firme_service_desc[index];
    }

    return 0;
}

/**
 *   @brief    - Return whether a service can be implemented at a caller world.
 *   @param    - world      : Caller world.
 *   @param    - service_id : FIRME service ID.
 *   @return   - VAL_TRUE if allowed, otherwise VAL_FALSE.
 **/
uint32_t val_firme_service_is_allowed_at_world(firme_acs_world_t world, uint32_t service_id)
{
    const val_firme_service_desc_t *desc = val_firme_service_desc(service_id);
    uint32_t world_mask = val_firme_service_world_mask(world);

    if (!desc)
        return VAL_FALSE;

    return ((desc->allowed_worlds & world_mask) != 0U) ? VAL_TRUE : VAL_FALSE;
}

/**
 *   @brief    - Return allocated ABI bits for service feature register 0.
 *   @param    - service_id : FIRME service ID.
 *   @return   - Allocated ABI bit mask, or 0 if unknown.
 **/
uint64_t val_firme_service_allocated_abi_mask(uint32_t service_id)
{
    const val_firme_service_desc_t *desc = val_firme_service_desc(service_id);

    return desc ? desc->allocated_abi_mask : 0ULL;
}

/**
 *   @brief    - Return mandatory ABI bits for a caller world.
 *   @param    - world      : Caller world.
 *   @param    - service_id : FIRME service ID.
 *   @return   - Mandatory ABI bit mask, or 0 if none.
 **/
uint64_t val_firme_service_mandatory_abi_mask(firme_acs_world_t world, uint32_t service_id)
{
    const val_firme_service_desc_t *desc = val_firme_service_desc(service_id);

    if (!desc)
        return 0ULL;

    if (world == FIRME_ACS_WORLD_NSEL2)
        return desc->mandatory_ns_mask;

    if (world == FIRME_ACS_WORLD_SECURE)
        return desc->mandatory_secure_mask;

    if (world == FIRME_ACS_WORLD_REALM)
        return desc->mandatory_realm_mask;

    return 0ULL;
}

/**
 *   @brief    - Return optional ABI bits for a caller world.
 *   @param    - world      : Caller world.
 *   @param    - service_id : FIRME service ID.
 *   @return   - Optional ABI bit mask, or 0 if none.
 **/
uint64_t val_firme_service_optional_abi_mask(firme_acs_world_t world, uint32_t service_id)
{
    const val_firme_service_desc_t *desc = val_firme_service_desc(service_id);

    if (!desc)
        return 0ULL;

    if (world == FIRME_ACS_WORLD_NSEL2)
        return desc->optional_ns_mask;

    if (world == FIRME_ACS_WORLD_SECURE)
        return desc->optional_secure_mask;

    if (world == FIRME_ACS_WORLD_REALM)
        return desc->optional_realm_mask;

    return 0ULL;
}

/**
 *   @brief    - Return all supported ABI bits for a caller world.
 *   @param    - world      : Caller world.
 *   @param    - service_id : FIRME service ID.
 *   @return   - Mandatory plus optional ABI bit mask.
 **/
uint64_t val_firme_service_supported_abi_mask(firme_acs_world_t world, uint32_t service_id)
{
    return val_firme_service_mandatory_abi_mask(world, service_id) |
           val_firme_service_optional_abi_mask(world, service_id);
}
