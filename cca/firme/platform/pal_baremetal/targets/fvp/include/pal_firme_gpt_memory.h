/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "pal_firme_gpt.h"
#include "pal_platform.h"

#ifndef PLAT_FIRME_GPT_TRANSITION_SIZE
#define PLAT_FIRME_GPT_TRANSITION_SIZE (UINT64_C(2) * PLAT_FIRME_GPT_GRANULE_SIZE)
#endif

#ifndef PLAT_FIRME_GPT_L1_BACKED_RESERVED
#define PLAT_FIRME_GPT_L1_BACKED_RESERVED 0U
#endif

#ifndef PLAT_FIRME_GPT_L1_RANGE_BASE
#define PLAT_FIRME_GPT_L1_RANGE_BASE UINT64_C(0)
#endif

#ifndef PLAT_FIRME_GPT_L1_RANGE_SIZE
#define PLAT_FIRME_GPT_L1_RANGE_SIZE UINT64_C(0)
#endif

#ifndef PLAT_FIRME_GPT_L1_INITIAL_GPI
#define PLAT_FIRME_GPT_L1_INITIAL_GPI UINT64_C(0xf)
#endif

#ifndef PLAT_FIRME_GPT_L1_POOL_BASE
#define PLAT_FIRME_GPT_L1_POOL_BASE UINT64_C(0)
#endif

#ifndef PLAT_FIRME_GPT_L1_POOL_SIZE
#define PLAT_FIRME_GPT_L1_POOL_SIZE UINT64_C(0)
#endif

/**
 * @brief Select storage from the platform's exclusive disposable reservations.
 * @param world Requested caller, with allocation coordinated by Host.
 * @param kind Transition, self-describing, or non-self-describing fixture.
 * @param granule_size Discovered GPC granule size.
 * @param range_size Discovered L0 protected-range size.
 * @param object_size Discovered L1 object size and alignment.
 * @param memory Selected extent, without an ownership token.
 * @return READY for a valid platform reservation, UNAVAILABLE if none is declared.
 *
 * PLAT_FIRME_GPT_L1_BACKED_RESERVED is a platform guarantee that the declared
 * range and pool are backed RAM, exclusively disposable, and do not overlap
 * firmware, payloads, DMA allocations or another allocator. It is not inferred
 * from a model DRAM-size argument. The allocator must still check GPT_INFO before
 * modifying an L0 Block. The default FVP makes no such L1 ownership guarantee.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_memory_layout(
    uint32_t world, pal_firme_gpt_memory_kind_t kind, uint64_t granule_size,
    uint64_t range_size, uint64_t object_size, pal_firme_gpt_memory_t *memory);

/** @brief Allow a bounded read only within explicitly disposable, backed storage. */
uint32_t pal_firme_gpt_memory_probe_allowed(uint64_t address);
