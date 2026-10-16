/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_gpt_memory.h"

#include "val_common/val_status.h"
#include "val_firme_dispatch.h"
#include "val_firme_gpt_defs.h"

#define PAL_GPT_GPI_ANY UINT64_C(0xf)

/** @brief Check a nonempty owned extent without wrapping its exclusive end. */
static uint32_t pal_firme_gpt_extent_contains(uint64_t base, uint64_t size,
                                             uint64_t address, uint64_t length)
{
    return base && size && length && base <= UINT64_MAX - size &&
           address >= base && length <= size && address - base <= size - length;
}

/** @brief Reject zero, overflow and non-power-of-two architectural geometry. */
static uint32_t pal_firme_gpt_power_of_two(uint64_t value)
{
    return value && !(value & (value - 1ULL));
}

/** @brief Choose an aligned object wholly inside the declared allocation pool. */
static pal_firme_gpt_scenario_status_t pal_firme_gpt_object_allocate(
    uint64_t base, uint64_t size, uint64_t granule_size, uint64_t object_size,
    pal_firme_gpt_memory_t *memory)
{
    uint64_t aligned;

    if (!base || !size || base > UINT64_MAX - size ||
        base > UINT64_MAX - (object_size - 1ULL))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    aligned = (base + object_size - 1ULL) & ~(object_size - 1ULL);
    if (pal_firme_gpt_granule_envelope(aligned, object_size, granule_size,
            &memory->pool_base, &memory->pool_size) != PAL_FIRME_GPT_SCENARIO_READY ||
        !pal_firme_gpt_extent_contains(base, size, memory->pool_base,
                                        memory->pool_size))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    memory->object_base = aligned;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Select only explicit platform reservations, never firmware-derived free RAM. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_memory_layout(
    uint32_t world, pal_firme_gpt_memory_kind_t kind, uint64_t granule_size,
    uint64_t range_size, uint64_t object_size, pal_firme_gpt_memory_t *memory)
{
    uint64_t range_base = PLAT_FIRME_GPT_L1_RANGE_BASE;
    uint64_t reserved_size = PLAT_FIRME_GPT_L1_RANGE_SIZE;
    uint64_t pool_base = PLAT_FIRME_GPT_L1_POOL_BASE;
    uint64_t pool_size = PLAT_FIRME_GPT_L1_POOL_SIZE;
    pal_firme_gpt_scenario_status_t status;

    if (!memory || world > FIRME_ACS_WORLD_REALM ||
        kind > PAL_FIRME_GPT_MEMORY_L1_NON_SELF ||
        !pal_firme_gpt_power_of_two(granule_size))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    *memory = (pal_firme_gpt_memory_t){0};
    if (kind == PAL_FIRME_GPT_MEMORY_TRANSITION) {
        if (!PLAT_FIRME_GPT_BASE_NS || !PLAT_FIRME_GPT_TRANSITION_SIZE)
            return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;

        if (granule_size != PLAT_FIRME_GPT_GRANULE_SIZE ||
            PLAT_FIRME_GPT_BASE_NS % granule_size ||
            PLAT_FIRME_GPT_TRANSITION_SIZE % granule_size ||
            PLAT_FIRME_GPT_BASE_NS > UINT64_MAX - PLAT_FIRME_GPT_TRANSITION_SIZE)
            return PAL_FIRME_GPT_SCENARIO_ERROR;

        memory->range_base = PLAT_FIRME_GPT_BASE_NS;
        memory->range_size = PLAT_FIRME_GPT_TRANSITION_SIZE;
        memory->initial_gpi = FIRME_GPI_NONSECURE;
        return PAL_FIRME_GPT_SCENARIO_READY;
    }

    if (world != FIRME_ACS_WORLD_REALM || !PLAT_FIRME_GPT_L1_BACKED_RESERVED)
        return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;

    if (!pal_firme_gpt_power_of_two(range_size) ||
        !pal_firme_gpt_power_of_two(object_size) || !range_base ||
        range_base % range_size || range_size % granule_size ||
        !pal_firme_gpt_extent_contains(range_base, reserved_size, range_base, range_size) ||
        (PLAT_FIRME_GPT_L1_INITIAL_GPI != FIRME_GPI_NONSECURE &&
         PLAT_FIRME_GPT_L1_INITIAL_GPI != PAL_GPT_GPI_ANY))
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    memory->range_base = range_base;
    memory->range_size = range_size;
    memory->initial_gpi = PLAT_FIRME_GPT_L1_INITIAL_GPI;
    if (kind == PAL_FIRME_GPT_MEMORY_L1_SELF) {
        if (memory->initial_gpi != FIRME_GPI_NONSECURE)
            return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;

        pool_base = range_base;
        pool_size = range_size;
    }

    status = pal_firme_gpt_object_allocate(pool_base, pool_size, granule_size,
                                           object_size, memory);
    if (status != PAL_FIRME_GPT_SCENARIO_READY)
        return status;

    if (kind == PAL_FIRME_GPT_MEMORY_L1_NON_SELF &&
        memory->pool_base < range_base + range_size &&
        memory->pool_base + memory->pool_size > range_base)
        return PAL_FIRME_GPT_SCENARIO_ERROR;

    return PAL_FIRME_GPT_SCENARIO_READY;
}

/** @brief Restrict fault-recoverable loads to PAL-owned RAM, including all eight bytes. */
uint32_t pal_firme_gpt_memory_probe_allowed(uint64_t address)
{
    if (address % sizeof(uint64_t))
        return VAL_FALSE;

    if (pal_firme_gpt_extent_contains(PLAT_FIRME_GPT_BASE_NS,
            PLAT_FIRME_GPT_TRANSITION_SIZE, address, sizeof(uint64_t)))
        return VAL_TRUE;

    if (!PLAT_FIRME_GPT_L1_BACKED_RESERVED)
        return VAL_FALSE;

    return pal_firme_gpt_extent_contains(PLAT_FIRME_GPT_L1_RANGE_BASE,
               PLAT_FIRME_GPT_L1_RANGE_SIZE, address, sizeof(uint64_t)) ||
           pal_firme_gpt_extent_contains(PLAT_FIRME_GPT_L1_POOL_BASE,
               PLAT_FIRME_GPT_L1_POOL_SIZE, address, sizeof(uint64_t));
}
