/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_gpt.h"

#include <stddef.h>

#include "pal_firme_gpt_memory.h"
#include "val_common/val_mp.h"
#include "val_common/val_status.h"
#include "val_firme_gpt_fixture.h"
#include "val_firme_gpt_probe.h"
#include "val_firme_gpt_retained.h"
#include "val_firme_diag.h"
#include "val_host_abi_frame.h"
#include "val_host_mp.h"
#include "val_log.h"

#define VAL_HOST_GPT_GPI_ANY UINT64_C(0xf)
#define VAL_HOST_GPT_WORLD_COUNT 3U

static uint32_t g_host_gpt_owned;
static uint32_t g_host_gpt_owner_pe;
static uint64_t g_host_gpt_token;
static uint64_t g_host_gpt_granule_size;
static pal_firme_gpt_memory_t g_host_gpt_memory;
static val_host_abi_frame_context_t g_host_gpt_retained_transport[VAL_HOST_GPT_WORLD_COUNT];

/** @brief Bound an observer to the currently leased extent, without end overflow. */
static uint32_t val_host_gpt_contains(uint64_t base, uint64_t size,
                                      uint64_t address, uint64_t length)
{
    return size && length && base <= UINT64_MAX - size && address >= base &&
           length <= size && address - base <= size - length;
}

/** @brief Lease explicit PAL storage once, with one Host owner across all caller worlds. */
static pal_firme_gpt_scenario_status_t val_host_gpt_acquire(
    uint32_t world, pal_firme_gpt_memory_kind_t kind, uint64_t granule_size,
    uint64_t range_size, uint64_t object_size, pal_firme_gpt_memory_t *memory)
{
    firme_acs_call_res_t response = {0};
    pal_firme_gpt_memory_t selected = {0};
    pal_firme_gpt_scenario_status_t status;
    uint32_t target_world;
    uint32_t unlocked = VAL_FALSE;
    uint32_t pe_index = val_pe_index();

    if (!memory || pe_index >= PAL_PE_COUNT)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    *memory = (pal_firme_gpt_memory_t){0};
    status = pal_firme_gpt_memory_layout(world, kind, granule_size,
                                         range_size, object_size, &selected);
    if (status != PAL_FIRME_GPT_SCENARIO_READY)
        return status;

    if (kind != PAL_FIRME_GPT_MEMORY_TRANSITION) {
        LOG_WARN(NULL, "GPT L1 fixture: exact Root GPI needs an independent observer; "
                 "native PAS denial alone is insufficient");
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
    }

    /* Step 1: Require remote observers before acquiring mutable memory. */
    for (target_world = FIRME_ACS_WORLD_SECURE;
         target_world <= FIRME_ACS_WORLD_REALM; target_world++) {
        if (val_firme_world_call((firme_acs_world_t)target_world, FIRME_ACS_CMD_PING,
                                0ULL, 0ULL, 0ULL, 0ULL, &response) != VAL_SUCCESS)
            return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;

        if (response.x0 != ACS_STATUS_PASS)
            return PAL_FIRME_GPT_SCENARIO_ERROR;
    }

    /* Step 2: Retain ownership until the exact lease is independently restored. */
    if (!__atomic_compare_exchange_n(&g_host_gpt_owned, &unlocked, VAL_TRUE,
                                     0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    if (g_host_gpt_token == UINT64_MAX) {
        __atomic_store_n(&g_host_gpt_owned, VAL_FALSE, __ATOMIC_RELEASE);
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    }

    g_host_gpt_owner_pe = pe_index;
    g_host_gpt_granule_size = granule_size;
    g_host_gpt_token++;
    selected.token = g_host_gpt_token;
    g_host_gpt_memory = selected;
    *memory = selected;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Observe the native PAS of one caller without trusting a FIRME return status. */
static int val_host_gpt_probe(firme_acs_world_t world, uint64_t address,
                              uint32_t *accessible)
{
    firme_acs_call_res_t response = {0};

    if (world == FIRME_ACS_WORLD_NSEL2)
        val_firme_gpt_probe_command(address, &response);
    else if (val_firme_world_call(world, FIRME_ACS_CMD_GPT_PROBE,
                                  address, 0ULL, 0ULL, 0ULL, &response) != VAL_SUCCESS)
        return VAL_ERROR;

    if (response.x0 != ACS_STATUS_PASS || response.x1 > VAL_TRUE ||
        (response.x1 && !val_fault_is_gpf(response.x2)))
        return VAL_ERROR;

    *accessible = response.x1 ? VAL_FALSE : VAL_TRUE;
    return VAL_SUCCESS;
}

/** @brief Match native PAS enforcement and separately discriminate NS from NSO. */
static uint32_t val_host_gpt_pattern_matches(uint64_t gpi,
    const uint32_t *native_access, uint32_t ns_output_access)
{
    uint32_t world;
    uint32_t expected;

    if (gpi != FIRME_GPI_NONSECURE && gpi != FIRME_GPI_NSO &&
        gpi != FIRME_GPI_SECURE && gpi != FIRME_GPI_REALM &&
        gpi != VAL_HOST_GPT_GPI_ANY)
        return VAL_FALSE;

    if (gpi == FIRME_GPI_NSO && !val_firme_feat_rme_gpc2_present())
        return VAL_FALSE;

    for (world = FIRME_ACS_WORLD_NSEL2; world <= FIRME_ACS_WORLD_REALM; world++) {
        expected = gpi == VAL_HOST_GPT_GPI_ANY ? VAL_TRUE :
                   val_firme_expect_access_world(gpi, (firme_acs_world_t)world);
        if (native_access[world] != expected)
            return VAL_FALSE;
    }

    expected = (gpi == FIRME_GPI_NONSECURE || gpi == VAL_HOST_GPT_GPI_ANY) ?
               VAL_TRUE : VAL_FALSE;
    return ns_output_access == expected;
}

/** @brief Check every owned granule; never infer an unprobed suffix from progress counts. */
static pal_firme_gpt_scenario_status_t val_host_gpt_observe(
    uint64_t base, uint64_t size, uint64_t granule_size,
    uint32_t allowed_gpi_mask, uint64_t *checked_count)
{
    firme_acs_call_res_t ns_response = {0};
    uint32_t native_access[VAL_HOST_GPT_WORLD_COUNT] = {0};
    uint64_t offset;
    uint64_t gpi;
    uint32_t world;
    uint32_t ns_output_access;
    uint32_t matched;

    if (!checked_count)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    *checked_count = 0ULL;
    if (!__atomic_load_n(&g_host_gpt_owned, __ATOMIC_ACQUIRE) ||
        g_host_gpt_owner_pe != val_pe_index() || !granule_size ||
        granule_size != g_host_gpt_granule_size || base % granule_size ||
        size % granule_size || !allowed_gpi_mask ||
        (allowed_gpi_mask & ~UINT32_C(0xffff)) ||
        (!val_host_gpt_contains(g_host_gpt_memory.range_base, g_host_gpt_memory.range_size,
                                base, size) &&
         !val_host_gpt_contains(g_host_gpt_memory.pool_base, g_host_gpt_memory.pool_size,
                                base, size)))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    /* Step 1: Probe all native PASes and a Secure caller's Non-secure output PAS. */
    for (offset = 0ULL; offset < size; offset += granule_size) {
        for (world = FIRME_ACS_WORLD_NSEL2; world <= FIRME_ACS_WORLD_REALM; world++) {
            if (val_host_gpt_probe((firme_acs_world_t)world, base + offset,
                                    &native_access[world]) != VAL_SUCCESS)
                return PAL_FIRME_GPT_SCENARIO_ERROR;
        }

        if (val_firme_world_call(FIRME_ACS_WORLD_SECURE, FIRME_ACS_CMD_RUN_DIAG,
                FIRME_DIAG_READ_NS_PAS, base + offset, 0ULL, 0ULL,
                &ns_response) != VAL_SUCCESS || ns_response.x0 != ACS_STATUS_PASS ||
            ns_response.x1 > VAL_TRUE ||
            (ns_response.x1 && !val_fault_is_gpf(ns_response.x2)))
            return PAL_FIRME_GPT_SCENARIO_ERROR;

        ns_output_access = ns_response.x1 ? VAL_FALSE : VAL_TRUE;
        matched = VAL_FALSE;
        for (gpi = 0ULL; gpi <= VAL_HOST_GPT_GPI_ANY; gpi++) {
            if ((allowed_gpi_mask & (UINT32_C(1) << gpi)) &&
                val_host_gpt_pattern_matches(gpi, native_access, ns_output_access)) {
                matched = VAL_TRUE;
                break;
            }
        }

        if (!matched) {
            LOG_ERROR(NULL, "GPT fixture: PAS observation mismatch PA=%llx allowed=%x",
                      (unsigned long long)(base + offset), allowed_gpi_mask);
            return PAL_FIRME_GPT_SCENARIO_ERROR;
        }

        (*checked_count)++;
    }

    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Refuse stale owners and release only a fully observed original layout. */
static pal_firme_gpt_scenario_status_t val_host_gpt_release(uint64_t token)
{
    uint64_t checked_count = 0ULL;

    if (!token || token != g_host_gpt_token ||
        g_host_gpt_memory.initial_gpi > VAL_HOST_GPT_GPI_ANY ||
        val_host_gpt_observe(g_host_gpt_memory.range_base, g_host_gpt_memory.range_size,
            g_host_gpt_granule_size, UINT32_C(1) << g_host_gpt_memory.initial_gpi,
            &checked_count) != PAL_FIRME_GPT_SCENARIO_READY ||
        checked_count != g_host_gpt_memory.range_size / g_host_gpt_granule_size)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    if (g_host_gpt_memory.pool_size &&
        (val_host_gpt_observe(g_host_gpt_memory.pool_base, g_host_gpt_memory.pool_size,
             g_host_gpt_granule_size, UINT32_C(1) << FIRME_GPI_NONSECURE,
             &checked_count) != PAL_FIRME_GPT_SCENARIO_READY ||
         checked_count != g_host_gpt_memory.pool_size / g_host_gpt_granule_size))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    g_host_gpt_memory = (pal_firme_gpt_memory_t){0};
    __atomic_store_n(&g_host_gpt_owned, VAL_FALSE, __ATOMIC_RELEASE);
    return PAL_FIRME_GPT_SCENARIO_READY;
}

static const pal_firme_gpt_memory_ops_t g_host_gpt_ops = {
    .acquire = val_host_gpt_acquire,
    .observe = val_host_gpt_observe,
    .release = val_host_gpt_release,
};

/** @brief Install one allocator; Secure/Realm generated callers are coordinated by Host. */
int val_host_gpt_init(void)
{
    return pal_firme_gpt_memory_register(&g_host_gpt_ops) ==
           PAL_FIRME_GPT_SCENARIO_READY ? VAL_SUCCESS : VAL_ERROR;
}

/** @brief Reuse the full-frame transport while retaining observations and ownership in Host. */
int val_host_gpt_retained_run(firme_rule_id_t rule, firme_acs_world_t world,
    firme_rule_res_t *out)
{
    val_firme_abi_test_context_t context = {.world = world};
    val_host_abi_frame_context_t *transport;

    if (!out)
        return VAL_ERROR;

    *out = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    if ((uint32_t)world >= VAL_HOST_GPT_WORLD_COUNT ||
        val_pe_index() != val_host_mp_primary_index())
        return VAL_ERROR;

    if (world != FIRME_ACS_WORLD_NSEL2) {
        transport = &g_host_gpt_retained_transport[world];
        if (!transport->initialized &&
            val_host_abi_frame_init_world(transport, world) != VAL_SUCCESS)
            return VAL_ERROR;

        context.invoke_frame = val_host_abi_frame_invoke;
        context.invoke_cookie = transport;
    }
    return val_firme_gpt_retained_execute(rule, &context, out);
}

/** @brief Reuse the generated engine without executing remote setup in the Host world. */
int val_host_gpt_generated_execute(val_firme_abi_test_plan_id_t plan_id,
    firme_acs_world_t world, uint64_t base_pa,
    int (*invoke_frame)(const smccc_call_frame_t *, smccc_call_frame_t *,
                        uint32_t *, void *),
    void *invoke_cookie, firme_rule_res_t *out)
{
    const val_firme_abi_test_plan_t *plan = val_firme_abi_test_plan_get(plan_id);
    val_firme_abi_test_request_t request = {0};

    if (!out)
        return VAL_ERROR;

    *out = (firme_rule_res_t){.status = ACS_STATUS_ERR};
    if (!plan || !plan->enabled || world > FIRME_ACS_WORLD_REALM ||
        (world != FIRME_ACS_WORLD_NSEL2 && !invoke_frame))
        return VAL_ERROR;

    request.ops = val_firme_gpt_abi_adapter_get(plan->abi_id);
    if (!request.ops)
        return VAL_ERROR;

    request.plan_id = plan_id;
    request.world = world;
    request.base_pa = base_pa;
    request.invoke_frame = invoke_frame;
    request.invoke_cookie = invoke_cookie;
    return val_firme_abi_test_execute(&request, out);
}
