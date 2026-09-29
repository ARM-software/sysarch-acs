/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_shared_buffer.h"

#include "aarch64/sysreg.h"
#include "val_common/val_mmu.h"
#include "val_common/val_status.h"
#include "val_firme_shared_buffer.h"
#include "val_log.h"

#define PAL_PAT_HOST_BASE UINT64_C(0x88000000)
#define PAL_PAT_HOST_TOP UINT64_C(0x88200000)
#define PAL_PAT_BLOCK_SIZE (UINT64_C(1) << 30)
#define PAL_PAT_BLOCK_ADDRESS UINT64_C(0x0000ffffc0000000)
#define PAL_PAT_BLOCK_ATTRIBUTES UINT64_C(0x80000000000007df)
#define PAL_PAT_BLOCK_REQUIRED UINT64_C(0x705)
#define PAL_PAT_EL2 UINT64_C(8)
#define PAL_PAT_SCTLR_MC UINT64_C(5)
#define PAL_PAT_TCR_RES1 ((UINT64_C(1) << 31) | (UINT64_C(1) << 23))
#define PAL_PAT_HCR_E2H (UINT64_C(1) << 34)

typedef enum {
    PAL_PAT_FREE = 0,
    PAL_PAT_OWNED,
    PAL_PAT_QUARANTINED,
} pal_pat_state_t;

extern uint64_t g_val_mmu_l1[];

static _Alignas(PAL_FIRME_PAT_ALIGNMENT) uint8_t g_pat_buffer[PAL_FIRME_PAT_BUFFER_SIZE];
static const uint8_t g_pat_challenge[32] = {
    0x46U, 0x49U, 0x52U, 0x4dU, 0x45U, 0x2dU, 0x41U, 0x43U,
    0x53U, 0x2dU, 0x50U, 0x41U, 0x54U, 0x2dU, 0x4eU, 0x53U,
    0x20U, 0x26U, 0x09U, 0x28U, 0x11U, 0x22U, 0x33U, 0x44U,
    0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xaaU, 0xbbU, 0xccU,
};
static uint32_t g_pat_lock;
static pal_pat_state_t g_pat_state;
static uint64_t g_pat_generation;

/**
 * @brief Check the current host boot mapping before touching a reserved PAT buffer.
 * @param base Buffer identity address, reserved by tools/linker/host.ld.
 * @param size Full backed range inside the FVP NS DRAM host image.
 * @return VAL_TRUE only for live Normal WBWA, inner-shareable, writable mappings.
 */
__attribute__((weak)) uint32_t pal_firme_shared_buffer_ns_mapping_ready(
    uint64_t base, uint64_t size)
{
    uint64_t block = 0ULL;
    uint64_t descriptor = 0ULL;
    uint64_t sctlr = 0ULL;
    uint64_t tcr = 0ULL;
    uint64_t mair = 0ULL;
    uint64_t ttbr = 0ULL;
    uint64_t hcr = 0ULL;

    if ((base < PAL_PAT_HOST_BASE) || (base >= PAL_PAT_HOST_TOP) ||
        (size == 0ULL) || (size > PAL_PAT_HOST_TOP - base) ||
        (read_currentel() != PAL_PAT_EL2))
        return VAL_FALSE;
    sctlr = read_sctlr_el2();
    tcr = val_read_tcr_el2();
    mair = pal_firme_shared_buffer_read_mair_el2();
    ttbr = pal_firme_shared_buffer_read_ttbr0_el2();
    hcr = pal_firme_shared_buffer_read_hcr_el2();
    LOG_DEBUG(NULL, "PAT NS mapping sctlr=%llx tcr=%llx mair=%llx ttbr=%llx hcr=%llx",
              (unsigned long long)sctlr, (unsigned long long)tcr,
              (unsigned long long)mair, (unsigned long long)ttbr, (unsigned long long)hcr);
    if (((sctlr & PAL_PAT_SCTLR_MC) != PAL_PAT_SCTLR_MC) ||
        ((hcr & PAL_PAT_HCR_E2H) != 0ULL) ||
        ((tcr & ~PAL_PAT_TCR_RES1) != VAL_MMU_EL2_BOOT_TCR) ||
        (mair != VAL_MMU_BOOT_MAIR) ||
        (ttbr != (uint64_t)(uintptr_t)g_val_mmu_l1))
        return VAL_FALSE;
    block = base & ~(PAL_PAT_BLOCK_SIZE - 1ULL);
    for (; block < base + size; block += PAL_PAT_BLOCK_SIZE) {
        descriptor = g_val_mmu_l1[block / PAL_PAT_BLOCK_SIZE];
        LOG_DEBUG(NULL, "PAT NS mapping block=%llx descriptor=%llx",
                  (unsigned long long)block, (unsigned long long)descriptor);
        if (((descriptor & PAL_PAT_BLOCK_ADDRESS) != block) ||
            ((descriptor & PAL_PAT_BLOCK_ATTRIBUTES) != PAL_PAT_BLOCK_REQUIRED))
            return VAL_FALSE;
    }
    return VAL_TRUE;
}

/**
 * @brief Try to serialize lease metadata without waiting on a pending retrieval.
 * @return VAL_TRUE when locked, otherwise VAL_FALSE.
 */
static uint32_t pal_pat_try_lock(void)
{
    uint32_t unlocked = VAL_FALSE;

    return __atomic_compare_exchange_n(&g_pat_lock, &unlocked, VAL_TRUE, 0,
                                        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ?
               VAL_TRUE : VAL_FALSE;
}

/**
 * @brief Lease a private, backed NS BSS buffer under the checked FVP boot map.
 * @param capacity Requested capacity including negative-case padding.
 * @param alignment Required physical alignment.
 * @param lease Returned unique ownership token, buffer and separate challenge.
 * The ACS NS payload must be the sole valid PAT initiator, with all such calls
 * using this lease. Other worlds use different FIRME instances and no NS lease.
 * @return PROBE_AVAILABLE, UNAVAILABLE for unmet prerequisites, or ERROR.
 */
int val_firme_shared_buffer_acquire_ns_pat(uint64_t capacity, uint64_t alignment,
                                          val_firme_shared_buffer_lease_t *lease)
{
    uint64_t base = (uint64_t)(uintptr_t)g_pat_buffer;
    int result = VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;

    if (lease == 0)
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    *lease = (val_firme_shared_buffer_lease_t){0};
    if ((capacity == 0ULL) || (alignment == 0ULL) ||
        ((alignment & (alignment - 1ULL)) != 0ULL))
        return VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
    if ((capacity > sizeof(g_pat_buffer)) || (alignment > PAL_FIRME_PAT_ALIGNMENT) ||
        ((base & (alignment - 1ULL)) != 0ULL) ||
        (pal_firme_shared_buffer_ns_mapping_ready(base, sizeof(g_pat_buffer)) != VAL_TRUE))
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    if (pal_pat_try_lock() != VAL_TRUE)
        return VAL_FIRME_SHARED_BUFFER_PROBE_UNAVAILABLE;
    if (g_pat_state == PAL_PAT_FREE) {
        if (g_pat_generation == UINT64_MAX) {
            g_pat_state = PAL_PAT_QUARANTINED;
            result = VAL_FIRME_SHARED_BUFFER_PROBE_ERROR;
        } else {
            g_pat_generation++;
            g_pat_state = PAL_PAT_OWNED;
            *lease = (val_firme_shared_buffer_lease_t){
                .base_va = g_pat_buffer, .base_pa = base, .capacity = sizeof(g_pat_buffer),
                .challenge = g_pat_challenge, .challenge_size = sizeof(g_pat_challenge),
                .owner_token = g_pat_generation,
            };
            result = VAL_FIRME_SHARED_BUFFER_PROBE_AVAILABLE;
        }
    }
    __atomic_store_n(&g_pat_lock, VAL_FALSE, __ATOMIC_RELEASE);
    return result;
}

/**
 * @brief Release a matching live lease or retain its instance/buffer until reset.
 * @param lease Lease identity returned by the successful acquisition.
 * @param pending VAL_TRUE permanently quarantines this adapter's NS PAT instance.
 * @return VAL_SUCCESS for release/quarantine, VAL_ERROR for stale/invalid leases.
 */
int val_firme_shared_buffer_release_ns_pat(const val_firme_shared_buffer_lease_t *lease,
                                          uint32_t pending)
{
    int result = VAL_ERROR;

    if ((lease == 0) || ((pending != VAL_FALSE) && (pending != VAL_TRUE)))
        return VAL_ERROR;
    if (pal_pat_try_lock() != VAL_TRUE)
        return VAL_ERROR;
    if ((g_pat_state == PAL_PAT_OWNED) && (lease->owner_token == g_pat_generation) &&
        (lease->base_va == g_pat_buffer) &&
        (lease->base_pa == (uint64_t)(uintptr_t)g_pat_buffer) &&
        (lease->capacity == sizeof(g_pat_buffer)) &&
        (lease->challenge == g_pat_challenge) &&
        (lease->challenge_size == sizeof(g_pat_challenge))) {
        dsb_sy();
        g_pat_state = pending == VAL_TRUE ? PAL_PAT_QUARANTINED : PAL_PAT_FREE;
        result = VAL_SUCCESS;
    }
    __atomic_store_n(&g_pat_lock, VAL_FALSE, __ATOMIC_RELEASE);
    return result;
}
