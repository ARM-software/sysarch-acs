/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_shared_buffer.h"
#include "val_realm_pat_binding.h"

#include "aarch64/sysreg.h"
#include "pal_firme_shared_buffer.h"
#include "val_common/val_status.h"

#define VAL_REALM_BOOT_VERSION_0_8 UINT64_C(0x00000008)
#define VAL_REALM_BOOT_VERSION_2_0 UINT64_C(0x00020000)
#define VAL_REALM_BOOT_SHARED_SIZE UINT64_C(4096)

typedef enum {
    VAL_REALM_PAT_UNAVAILABLE = 0,
    VAL_REALM_PAT_FREE,
    VAL_REALM_PAT_OWNED,
    VAL_REALM_PAT_QUARANTINED,
} val_realm_pat_state_t;

extern uint8_t __text_start__[];
extern uint8_t __mec_test_end__[];

static uint64_t g_realm_pat_base;
static uint64_t g_realm_pat_generation;
static uint32_t g_realm_pat_lock;
static val_realm_pat_state_t g_realm_pat_state;
static uint64_t g_realm_pat_reason = VAL_FIRME_PAT_NO_BOOT_BUFFER;
static uint64_t g_realm_pat_detail;
static const uint8_t *g_realm_pat_binding;
static uint32_t g_realm_pat_binding_size;
static const uint8_t g_realm_pat_challenge[32] = {
    0x46U, 0x49U, 0x52U, 0x4dU, 0x45U, 0x2dU, 0x41U, 0x43U,
    0x53U, 0x2dU, 0x50U, 0x41U, 0x54U, 0x2dU, 0x52U, 0x4cU,
    0x20U, 0x26U, 0x09U, 0x28U, 0x11U, 0x22U, 0x33U, 0x44U,
    0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xaaU, 0xbbU, 0xccU,
};

/**
 * @brief Retain X3's boot shared page, not arbitrary Realm payload storage.
 * @param boot_version X1 from the supported EL3/Realm cold-boot interface.
 * @param base_pa X3's identity-mapped shared-page address.
 * Call after consuming boot metadata and establishing coherent translation.
 * These boot interface versions provide one 4 KiB page; no adjacent memory
 * is leased. Initialization cannot release an owned or quarantined page.
 */
void val_firme_shared_buffer_init_realm_pat(uint64_t boot_version, uint64_t base_pa)
{
    uint64_t image_start = (uint64_t)(uintptr_t)__text_start__;
    uint64_t image_end = (uint64_t)(uintptr_t)__mec_test_end__;

    if (g_realm_pat_state != VAL_REALM_PAT_UNAVAILABLE)
        return;
    g_realm_pat_detail = boot_version;
    if (((boot_version != VAL_REALM_BOOT_VERSION_0_8) &&
         (boot_version != VAL_REALM_BOOT_VERSION_2_0)) || (base_pa == 0ULL) ||
        ((base_pa & (VAL_REALM_BOOT_SHARED_SIZE - 1ULL)) != 0ULL) ||
        (base_pa > UINT64_MAX - VAL_REALM_BOOT_SHARED_SIZE) ||
        ((base_pa < image_end) && (base_pa + VAL_REALM_BOOT_SHARED_SIZE > image_start)))
        return;
    g_realm_pat_reason = VAL_FIRME_PAT_BOOT_MAPPING_UNAVAILABLE;
    g_realm_pat_detail = base_pa;
    if (pal_firme_shared_buffer_realm_mapping_ready(base_pa,
             VAL_REALM_BOOT_SHARED_SIZE) != VAL_TRUE)
        return;
    g_realm_pat_base = base_pa;
    g_realm_pat_state = VAL_REALM_PAT_FREE;
    dsb_sy();
}

/** @brief Serialize the shared Realm instance without waiting on pending firmware. */
static uint32_t val_realm_pat_try_lock(void)
{
    uint32_t unlocked = VAL_FALSE;

    return __atomic_compare_exchange_n(&g_realm_pat_lock, &unlocked, VAL_TRUE, 0,
                                       __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ?
               VAL_TRUE : VAL_FALSE;
}

/** @brief Lease only the firmware-provided shared page, with separate challenge data. */
int val_firme_shared_buffer_acquire_realm_pat(uint64_t capacity, uint64_t alignment,
                                             val_firme_shared_buffer_lease_t *lease)
{
    int result = VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    const uint8_t *binding = 0;
    uint32_t binding_size = 0U;
    uint32_t pending = VAL_FALSE;
    int binding_result;

    if (lease == 0)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *lease = (val_firme_shared_buffer_lease_t){0};
    if ((capacity == 0ULL) || (alignment == 0ULL) ||
        ((alignment & (alignment - 1ULL)) != 0ULL))
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    if (val_realm_pat_try_lock() != VAL_TRUE) {
        lease->prerequisite_reason = VAL_FIRME_PAT_INSTANCE_UNAVAILABLE;
        return result;
    }

    if (g_realm_pat_state == VAL_REALM_PAT_UNAVAILABLE) {
        lease->prerequisite_reason = g_realm_pat_reason;
        lease->prerequisite_detail = g_realm_pat_detail;
    } else if (capacity > VAL_REALM_BOOT_SHARED_SIZE) {
        lease->prerequisite_reason = VAL_FIRME_PAT_BACKING_TOO_SMALL;
        lease->prerequisite_detail = capacity;
    } else if (g_realm_pat_state != VAL_REALM_PAT_FREE) {
        lease->prerequisite_reason = VAL_FIRME_PAT_INSTANCE_UNAVAILABLE;
        lease->prerequisite_detail = g_realm_pat_state;
    } else {
        lease->prerequisite_reason = VAL_FIRME_PAT_BOOT_MAPPING_UNAVAILABLE;
        lease->prerequisite_detail = g_realm_pat_base;
    }
    if ((g_realm_pat_state == VAL_REALM_PAT_FREE) &&
        (capacity <= VAL_REALM_BOOT_SHARED_SIZE) &&
        ((g_realm_pat_base & (alignment - 1ULL)) == 0ULL) &&
        (pal_firme_shared_buffer_realm_mapping_ready(g_realm_pat_base,
             VAL_REALM_BOOT_SHARED_SIZE) == VAL_TRUE)) {
        binding_result = val_realm_pat_binding_prepare(
            (uint8_t *)(uintptr_t)g_realm_pat_base, VAL_REALM_BOOT_SHARED_SIZE, &pending);
        if (binding_result == VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE)
            binding_result = val_firme_shared_buffer_get_realm_binding(&binding, &binding_size);
        if (pending == VAL_TRUE) {
            g_realm_pat_state = VAL_REALM_PAT_QUARANTINED;
            binding_result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
        }

        if (binding_result != VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE) {
            lease->prerequisite_reason = VAL_FIRME_PAT_NO_WORKLOAD_BINDING;
            lease->prerequisite_detail = val_realm_pat_binding_status();
            result = binding_result == VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE ?
                     binding_result : VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
        } else if (binding == 0 || binding_size == 0U) {
            val_realm_pat_binding_clear();
            result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
        } else if (g_realm_pat_generation == UINT64_MAX) {
            val_realm_pat_binding_clear();
            g_realm_pat_state = VAL_REALM_PAT_QUARANTINED;
            result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
        } else {
            g_realm_pat_generation++;
            g_realm_pat_state = VAL_REALM_PAT_OWNED;
            g_realm_pat_binding = binding;
            g_realm_pat_binding_size = binding_size;
            *lease = (val_firme_shared_buffer_lease_t){
                .base_va = (uint8_t *)(uintptr_t)g_realm_pat_base,
                .base_pa = g_realm_pat_base, .capacity = VAL_REALM_BOOT_SHARED_SIZE,
                .challenge = g_realm_pat_challenge,
                .challenge_size = sizeof(g_realm_pat_challenge),
                .workload_binding = binding, .workload_binding_size = binding_size,
                .owner_token = g_realm_pat_generation,
            };
            result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
        }
    }
    __atomic_store_n(&g_realm_pat_lock, VAL_FALSE, __ATOMIC_RELEASE);
    return result;
}

/** @brief Release the exact lease or quarantine its page and PAT instance until reset. */
int val_firme_shared_buffer_release_realm_pat(const val_firme_shared_buffer_lease_t *lease,
                                             uint32_t pending)
{
    int result = VAL_ERROR;

    if ((lease == 0) || ((pending != VAL_FALSE) && (pending != VAL_TRUE)))
        return VAL_ERROR;
    if (val_realm_pat_try_lock() != VAL_TRUE)
        return VAL_ERROR;
    if ((g_realm_pat_state == VAL_REALM_PAT_OWNED) &&
        (lease->owner_token == g_realm_pat_generation) &&
        (lease->base_va == (uint8_t *)(uintptr_t)g_realm_pat_base) &&
        (lease->base_pa == g_realm_pat_base) &&
        (lease->capacity == VAL_REALM_BOOT_SHARED_SIZE) &&
        (lease->challenge == g_realm_pat_challenge) &&
        (lease->challenge_size == sizeof(g_realm_pat_challenge)) &&
        (lease->workload_binding == g_realm_pat_binding) &&
        (lease->workload_binding_size == g_realm_pat_binding_size)) {
        dsb_sy();
        g_realm_pat_state = pending == VAL_TRUE ?
                            VAL_REALM_PAT_QUARANTINED : VAL_REALM_PAT_FREE;
        result = VAL_SUCCESS;
        if (pending == VAL_FALSE)
            val_realm_pat_binding_clear();
    }
    __atomic_store_n(&g_realm_pat_lock, VAL_FALSE, __ATOMIC_RELEASE);
    return result;
}
