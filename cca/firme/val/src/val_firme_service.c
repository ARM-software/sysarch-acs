/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_service.h"

#include "val_common/val_status.h"
#include "val_firme_abi.h"

#define VAL_FIRME_SERVICE_WORLD_NS     (UINT32_C(1) << 0)
#define VAL_FIRME_SERVICE_WORLD_SECURE (UINT32_C(1) << 1)
#define VAL_FIRME_SERVICE_WORLD_REALM  (UINT32_C(1) << 2)

typedef struct {
    uint32_t service_id;
    uint32_t allowed_worlds;
} val_firme_service_access_desc_t;

static const val_firme_service_access_desc_t g_val_firme_service_access_desc[] = {
    {
        .service_id = FIRME_SERVICE_ID_BASE,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_SECURE |
                          VAL_FIRME_SERVICE_WORLD_REALM,
    },
    {
        .service_id = FIRME_SERVICE_ID_GRANULE_MGMT,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_SECURE |
                          VAL_FIRME_SERVICE_WORLD_REALM,
    },
    {
        .service_id = FIRME_SERVICE_ID_IDE_KEY_MGMT,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_REALM,
    },
    {
        .service_id = FIRME_SERVICE_ID_MECID_MGMT,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_REALM,
    },
    {
        .service_id = FIRME_SERVICE_ID_ATTESTATION,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_NS | VAL_FIRME_SERVICE_WORLD_REALM,
    },
    {
        .service_id = FIRME_SERVICE_ID_INTEGRATED_DEVICE,
        .allowed_worlds = VAL_FIRME_SERVICE_WORLD_REALM,
    },
};

/**
 *   @brief    - Check whether a FIRME service is callable from a given ACS world.
 *   @param    - world      : ACS world making the service query.
 *   @param    - service_id : FIRME service ID.
 *   @return   - VAL_TRUE if the service is allowed from the world, else VAL_FALSE.
 **/
uint32_t val_firme_service_is_allowed_at_world(firme_acs_world_t world, uint32_t service_id)
{
    uint32_t index = 0U;
    uint32_t world_mask = 0U;

    switch (world) {
    case FIRME_ACS_WORLD_NSEL2:
        world_mask = VAL_FIRME_SERVICE_WORLD_NS;
        break;
    case FIRME_ACS_WORLD_SECURE:
        world_mask = VAL_FIRME_SERVICE_WORLD_SECURE;
        break;
    case FIRME_ACS_WORLD_REALM:
        world_mask = VAL_FIRME_SERVICE_WORLD_REALM;
        break;
    default:
        return VAL_FALSE;
    }

    for (index = 0U; index < (sizeof(g_val_firme_service_access_desc) /
                              sizeof(g_val_firme_service_access_desc[0])); index++) {
        if (g_val_firme_service_access_desc[index].service_id == service_id)
            return ((g_val_firme_service_access_desc[index].allowed_worlds & world_mask) !=
                    0U) ? VAL_TRUE : VAL_FALSE;
    }

    return VAL_FALSE;
}
