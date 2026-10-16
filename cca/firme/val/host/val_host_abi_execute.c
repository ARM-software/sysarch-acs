/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_abi_frame.h"
#include "val_host_mp.h"
#include "val_host_gpt.h"
#include "val_common/val_mp.h"
#include "val_firme_abi_adapter.h"
#include "val_firme_abi_generated.h"
#include "val_firme_abi.h"

static val_host_abi_frame_context_t g_host_abi_transport[FIRME_ACS_WORLD_REALM + 1U];

/**
 * @brief Retain mutable fixtures in NS while every FIRME call executes remotely.
 * @param plan_id Generated GPT or IDE plan selected by its explicit registry entry.
 * @param world Requested architectural caller; never silently replaced by NS.
 * @param base_pa Platform-owned test address.
 * @param out Combined setup, invocation, observation and cleanup result.
 * @return VAL_SUCCESS for PASS/SKIP, otherwise VAL_ERROR.
 */
int val_firme_abi_generated_remote_execute(
    val_firme_abi_test_plan_id_t plan_id, firme_acs_world_t world,
    uint64_t base_pa, firme_rule_res_t *out)
{
    val_firme_abi_test_request_t request = {0};
    const val_firme_abi_test_plan_t *plan;
    const val_firme_abi_descriptor_t *abi;
    val_host_abi_frame_context_t *transport;

    if (!out)
        return VAL_ERROR;

    *out = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    plan = val_firme_abi_test_plan_get(plan_id);
    if (!plan || !plan->enabled ||
        (world != FIRME_ACS_WORLD_REALM && world != FIRME_ACS_WORLD_SECURE) ||
        val_pe_index() != val_host_mp_primary_index())
        return VAL_ERROR;

    abi = val_firme_abi_catalog_get(plan->abi_id);
    if (!abi || (abi->service_id != FIRME_SERVICE_ID_IDE_KEY_MGMT &&
                  abi->service_id != FIRME_SERVICE_ID_GRANULE_MGMT))
        return VAL_ERROR;

    transport = &g_host_abi_transport[world];
    if (val_host_abi_frame_init_world(transport, world) != VAL_SUCCESS)
        return VAL_ERROR;

    if (abi->service_id == FIRME_SERVICE_ID_GRANULE_MGMT)
        return val_host_gpt_generated_execute(plan_id, world, base_pa,
            val_host_abi_frame_invoke, transport, out);

    request.plan_id = plan_id;
    request.world = world;
    request.base_pa = base_pa;
    request.ops = val_firme_abi_adapter_get(plan->abi_id);
    request.invoke_frame = val_host_abi_frame_invoke;
    request.invoke_cookie = transport;
    return val_firme_abi_test_execute(&request, out);
}
