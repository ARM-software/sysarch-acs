/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdint.h>
#include "val_firme_shared_buffer.h"
#include "val_common/val_status.h"

uint8_t __text_start__[1];
uint8_t __mec_test_end__[1];
static uint32_t g_mapping_ready = VAL_TRUE;
static uint32_t g_binding_ready;
static const uint8_t g_binding[32] = {0x54U, 0x45U, 0x53U, 0x54U};
static const uint8_t *g_binding_pointer = g_binding;
static uint32_t g_binding_size = sizeof(g_binding);
static int g_prepare_result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
static int g_binding_result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
static uint32_t g_clear_count;
static uint32_t g_prepare_count;

/** @brief Keep this ownership-only harness independent of crypto and live setup calls. */
int val_realm_pat_binding_prepare(uint8_t *buffer, uint64_t size, uint32_t *pending)
{
    assert(buffer == (uint8_t *)(uintptr_t)UINT64_C(0xfc000000));
    assert(size == 4096ULL && pending != 0);
    *pending = VAL_FALSE;
    g_prepare_count++;
    return g_prepare_result;
}

/** @brief Native synthetic binding storage has no private material to release. */
void val_realm_pat_binding_clear(void)
{
    g_clear_count++;
}

/** @brief No live setup status exists in the ownership-only harness. */
uint64_t val_realm_pat_binding_status(void)
{
    return 0ULL;
}

/** @brief Supply native fixture bytes only; these are not live attestation evidence. */
int val_firme_shared_buffer_get_realm_binding(const uint8_t **binding, uint32_t *size)
{
    *binding = g_binding_pointer;
    *size = g_binding_size;
    return g_binding_ready == VAL_TRUE ? g_binding_result :
                                       VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
}

/** @brief Replace hardware mapping checks when exercising lease ownership natively. */
uint32_t pal_firme_shared_buffer_realm_mapping_ready(uint64_t base, uint64_t size)
{
    assert(base == UINT64_C(0xfc000000));
    assert(size == 4096ULL);
    return g_mapping_ready;
}

/** @brief Replace the architectural barrier only in the native harness. */
void dsb_sy(void)
{
}

/** @brief Reject unsafe handoffs, wrong leases, stale ownership and pending reuse. */
int main(void)
{
    const uint64_t base = UINT64_C(0xfc000000);
    val_firme_shared_buffer_lease_t first = {0};
    val_firme_shared_buffer_lease_t second = {0};
    val_firme_shared_buffer_lease_t stale = {0};
    uint32_t prepare_count;

    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    val_firme_shared_buffer_init_realm_pat(0ULL, base);
    val_firme_shared_buffer_init_realm_pat(0x20000ULL, 0ULL);
    val_firme_shared_buffer_init_realm_pat(0x20000ULL, base + 1ULL);
    val_firme_shared_buffer_init_realm_pat(0x20000ULL, UINT64_MAX & ~UINT64_C(4095));
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    g_mapping_ready = VAL_FALSE;
    val_firme_shared_buffer_init_realm_pat(0x20000ULL, base);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    g_mapping_ready = VAL_TRUE;
    val_firme_shared_buffer_init_realm_pat(0x20000ULL, base);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(first.prerequisite_reason == VAL_FIRME_PAT_NO_WORKLOAD_BINDING);
    g_binding_ready = VAL_TRUE;
    g_prepare_result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(first.owner_token == 0ULL && first.workload_binding == 0);
    g_prepare_result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
    g_binding_result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(first.owner_token == 0ULL && first.workload_binding == 0);
    g_binding_result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
    g_binding_pointer = 0;
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(first.owner_token == 0ULL && first.workload_binding == 0 && g_clear_count == 1U);
    g_binding_pointer = g_binding;
    g_binding_size = 0U;
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(first.owner_token == 0ULL && first.workload_binding == 0 && g_clear_count == 2U);
    g_binding_size = sizeof(g_binding);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, 0) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 3ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_ERROR);
    assert(val_firme_shared_buffer_acquire_realm_pat(4097ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(first.prerequisite_reason == VAL_FIRME_PAT_BACKING_TOO_SMALL);
    assert(first.prerequisite_detail == 4097ULL && first.owner_token == 0ULL);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    assert(first.base_pa == base && first.capacity == 4096ULL);
    assert(first.base_va == (uint8_t *)(uintptr_t)base && first.challenge_size == 32U);
    assert(first.owner_token == 1ULL);
    prepare_count = g_prepare_count;
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &second) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(g_prepare_count == prepare_count);
    assert(val_firme_shared_buffer_release_realm_pat(&second, VAL_FALSE) == VAL_ERROR);
    stale = first;
    stale.capacity++;
    assert(val_firme_shared_buffer_release_realm_pat(&stale, VAL_FALSE) == VAL_ERROR);
    stale = first;
    stale.workload_binding_size++;
    assert(val_firme_shared_buffer_release_realm_pat(&stale, VAL_FALSE) == VAL_ERROR);
    stale = first;
    assert(val_firme_shared_buffer_release_realm_pat(&first, VAL_FALSE) == VAL_SUCCESS);
    assert(val_firme_shared_buffer_release_realm_pat(&first, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &second) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE);
    assert(first.owner_token != second.owner_token);
    assert(val_firme_shared_buffer_release_realm_pat(&stale, VAL_FALSE) == VAL_ERROR);
    assert(val_firme_shared_buffer_release_realm_pat(&second, VAL_TRUE) == VAL_SUCCESS);
    val_firme_shared_buffer_init_realm_pat(0x20000ULL, base);
    assert(val_firme_shared_buffer_acquire_realm_pat(4096ULL, 4096ULL, &first) ==
           VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE);
    assert(val_firme_shared_buffer_release_realm_pat(&second, VAL_FALSE) == VAL_ERROR);
    return VAL_SUCCESS;
}
