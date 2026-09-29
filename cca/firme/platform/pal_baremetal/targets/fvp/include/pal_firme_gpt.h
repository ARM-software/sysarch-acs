/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef enum {
    PAL_FIRME_GPT_ABI_GPI_SET = 0,
    PAL_FIRME_GPT_ABI_OP_CONTINUE = 1,
} pal_firme_gpt_abi_t;

typedef enum {
    PAL_FIRME_GPT_SCENARIO_UNAVAILABLE = 0,
    PAL_FIRME_GPT_SCENARIO_READY = 1,
    PAL_FIRME_GPT_SCENARIO_ERROR = 2,
    PAL_FIRME_GPT_SCENARIO_MISMATCH = 3,
} pal_firme_gpt_scenario_status_t;

typedef enum {
    PAL_FIRME_GPT_MEMORY_TRANSITION = 0,
    PAL_FIRME_GPT_MEMORY_L1_SELF,
    PAL_FIRME_GPT_MEMORY_L1_NON_SELF,
} pal_firme_gpt_memory_kind_t;

typedef struct {
    uint64_t range_base;
    uint64_t range_size;
    uint64_t initial_gpi;
    uint64_t object_base;
    uint64_t pool_base;
    uint64_t pool_size;
    uint64_t token;
} pal_firme_gpt_memory_t;

typedef struct {
    pal_firme_gpt_scenario_status_t (*acquire)(
        uint32_t world, pal_firme_gpt_memory_kind_t kind,
        uint64_t granule_size, uint64_t range_size, uint64_t object_size,
        pal_firme_gpt_memory_t *memory);
    pal_firme_gpt_scenario_status_t (*observe)(
        uint64_t base, uint64_t size, uint64_t granule_size,
        uint32_t allowed_gpi_mask, uint64_t *checked_count);
    pal_firme_gpt_scenario_status_t (*release)(uint64_t token);
} pal_firme_gpt_memory_ops_t;

/**
 * @brief Install platform reservation and independent GPI observation hooks.
 * @param ops Permanent hooks, installed once before dispatch on any PE. Acquire
 * must exclusively reserve backed, mapped, disposable storage, including the
 * entire target range and all object granules. Release is called only after
 * restoration; failed sessions retain their reservation until platform reset.
 * Hooks must serialize leases across worlds and must not infer GPI from the ABI
 * result being tested. NULL leaves fixtures unavailable.
 * @return Scenario readiness or error for incomplete/repeated registration.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_memory_register(
    const pal_firme_gpt_memory_ops_t *ops);

/** @brief Reserve an explicitly platform-owned fixture; no default PA fallback.
 * @param world Caller world.
 * @param kind Required memory layout.
 * @param granule_size Discovered granule size.
 * @param range_size Discovered protected range size.
 * @param object_size Architectural object byte size, not allocation size.
 * @param memory Returned exclusive reservation and nonzero cleanup token.
 * @return Scenario readiness, unavailability, or error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_memory_acquire(
    uint32_t world, pal_firme_gpt_memory_kind_t kind, uint64_t granule_size,
    uint64_t range_size, uint64_t object_size, pal_firme_gpt_memory_t *memory);

/** @brief Release a reservation only after its owning session restores state.
 * @param token Platform reservation token.
 * @return Scenario readiness or cleanup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_memory_release(uint64_t token);

/** @brief Compute the complete granule envelope of a nonempty byte range.
 * @param base Byte-range base.
 * @param size Byte-range size.
 * @param granule_size Power-of-two granule size.
 * @param covered_base Returned aligned base.
 * @param covered_size Returned aligned size.
 * @return Scenario readiness, or error on invalid/overflowing geometry.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_granule_envelope(
    uint64_t base, uint64_t size, uint64_t granule_size,
    uint64_t *covered_base, uint64_t *covered_size);

typedef struct {
    uint64_t base_pa;
    uint64_t granule_count;
    uint64_t target_gpi;
    uint64_t cookie;
    uint64_t initial_gpi;
    uint64_t completed_count;
} pal_firme_gpt_scenario_t;

typedef struct {
    uint64_t range_base;
    uint64_t range_top;
    uint64_t state_before;
    uint64_t state_after;
    uint32_t transition_completed_during_call;
} pal_firme_gpt_concurrent_info_t;

typedef enum {
    PAL_FIRME_GPT_L1_CREATE = 0,
    PAL_FIRME_GPT_L1_DESTROY = 1,
} pal_firme_gpt_l1_operation_t;

typedef enum {
    PAL_FIRME_GPT_L1_LAYOUT_SELF_DESCRIBING = 0,
    PAL_FIRME_GPT_L1_LAYOUT_NON_SELF_DESCRIBING = 1,
} pal_firme_gpt_l1_layout_t;

typedef enum {
    PAL_FIRME_GPT_L1_CONDITION_VALID_SELF = 0,
    PAL_FIRME_GPT_L1_CONDITION_VALID_NON_SELF,
    PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_SELF,
    PAL_FIRME_GPT_L1_CONDITION_INCOMPLETE_NON_SELF,
    PAL_FIRME_GPT_L1_CONDITION_PA_OUTSIDE_PPS,
    PAL_FIRME_GPT_L1_CONDITION_PA_MISALIGNED,
    PAL_FIRME_GPT_L1_CONDITION_EL3_MANAGED,
    PAL_FIRME_GPT_L1_CONDITION_OBJECT_OUTSIDE_PPS,
    PAL_FIRME_GPT_L1_CONDITION_OBJECT_MISALIGNED,
    PAL_FIRME_GPT_L1_CONDITION_OBJECT_UNSUITABLE,
    PAL_FIRME_GPT_L1_CONDITION_CONTINUE_WITHOUT_PENDING,
    PAL_FIRME_GPT_L1_CONDITION_AUXILIARY_L1_UNAVAILABLE,
    PAL_FIRME_GPT_L1_CONDITION_ALREADY_PRESENT,
    PAL_FIRME_GPT_L1_CONDITION_CONFLICT,
    PAL_FIRME_GPT_L1_CONDITION_BUSY,
    PAL_FIRME_GPT_L1_CONDITION_NO_MEMORY,
    PAL_FIRME_GPT_L1_CONDITION_OBJECT_GPI_DENIED,
    PAL_FIRME_GPT_L1_CONDITION_NO_L1_OBJECT,
    PAL_FIRME_GPT_L1_CONDITION_MIXED_RANGE_GPI,
    PAL_FIRME_GPT_L1_CONDITION_SELF_OBJECT_NOT_ROOT,
    PAL_FIRME_GPT_L1_CONDITION_SELF_NON_OBJECT_NOT_NS,
    PAL_FIRME_GPT_L1_CONDITION_SCRUB_SELF,
    PAL_FIRME_GPT_L1_CONDITION_SCRUB_NON_SELF,
} pal_firme_gpt_l1_condition_t;

typedef struct {
    uint64_t range_base;
    uint64_t range_size;
    uint64_t object_base;
    uint64_t object_size;
    uint64_t granule_size;
    uint64_t range_initial_gpi;
    uint64_t object_initial_gpi;
    uint64_t denied_granule_index;
    uint64_t denied_granule_gpi;
    uint64_t token;
    pal_firme_gpt_l1_layout_t layout;
    uint32_t fixture_owned;
} pal_firme_gpt_l1_scenario_t;

typedef struct {
    uint64_t bridge_base;
    uint64_t bridge_size;
    uint64_t bridge_gpi;
    uint64_t scrubbed_size;
    uint32_t bridge_observed;
    uint32_t bridge_owned_by_el3;
    uint32_t bridge_reclaimed;
    uint32_t scrub_value_independent;
    uint32_t scrub_completed_before_nonsecure;
} pal_firme_gpt_l1_evidence_t;

/**
 * @brief Prepare a deterministic Granule Management status scenario.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param abi ABI that must return the requested status.
 * @param expected_status Architected status to induce.
 * @param default_base Default suite test address.
 * @param scenario Returned scenario inputs and cleanup token.
 * @return Scenario availability or setup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_prepare(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected_status,
    uint64_t default_base, pal_firme_gpt_scenario_t *scenario);

/**
 * @brief Restore all state changed while preparing a GPT scenario.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param abi ABI selected for the scenario.
 * @param expected_status Architected status selected for the scenario.
 * @param scenario Scenario returned by the prepare hook.
 * @return Scenario readiness on success or setup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_cleanup(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected_status,
    const pal_firme_gpt_scenario_t *scenario);

/**
 * @brief Prepare another PE to change GPT state during a GPT_INFO call.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param default_base Default suite test address.
 * @param evidence Returned concurrent-state evidence.
 * @return Scenario availability or setup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_prepare(
    uint32_t world, uint64_t default_base,
    pal_firme_gpt_concurrent_info_t *evidence);

/**
 * @brief Release the prepared PE to overlap the next GPT_INFO invocation.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param evidence Prepared concurrent-state evidence.
 * @return Scenario readiness or execution error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_begin(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence);

/**
 * @brief Wait for the concurrent transition and collect its final state.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param evidence Updated concurrent-state evidence.
 * @return Scenario readiness or execution error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_finish(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence);

/**
 * @brief Restore state used by a concurrent GPT_INFO scenario.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param evidence Concurrent-state evidence to release.
 * @return Scenario readiness or cleanup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_cleanup(
    uint32_t world, const pal_firme_gpt_concurrent_info_t *evidence);

/**
 * @brief Prepare one deterministic L1 GPT lifecycle condition.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under test.
 * @param condition Architectural condition to establish.
 * @param default_base Default suite test address.
 * @param scenario Returned platform-owned range and object description.
 * @return Scenario readiness, unavailability, or setup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_scenario_prepare(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition, uint64_t default_base,
    pal_firme_gpt_l1_scenario_t *scenario);

/**
 * @brief Restore all state used by an L1 GPT lifecycle condition.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under test.
 * @param condition Architectural condition that was established.
 * @param scenario Prepared platform scenario.
 * @return Scenario readiness on success or cleanup error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_scenario_cleanup(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario);

/**
 * @brief Begin independent bridge or scrub event observation.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under observation.
 * @param condition Architectural condition under observation.
 * @param scenario Prepared platform scenario.
 * @return Scenario readiness or observation error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_monitor_begin(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario);

/**
 * @brief Finish independent bridge or scrub event observation.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under observation.
 * @param condition Architectural condition under observation.
 * @param scenario Prepared platform scenario.
 * @param evidence Returned independently observed evidence.
 * @return Scenario readiness or observation error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_monitor_end(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario,
    pal_firme_gpt_l1_evidence_t *evidence);

/**
 * @brief Verify every granule in a range has one of the allowed GPIs.
 * @param range_base First granule physical address.
 * @param range_size Size of the complete range in bytes.
 * @param granule_size Granule size in bytes.
 * @param exclude_base Optional excluded subrange base.
 * @param exclude_size Optional excluded subrange size.
 * @param allowed_gpi_mask Bit mask of accepted architectural GPI encodings.
 * @param checked_count Number of granules independently checked.
 * @return Scenario readiness, unavailability, or observation error.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_gpi_range_check(
    uint64_t range_base, uint64_t range_size, uint64_t granule_size,
    uint64_t exclude_base, uint64_t exclude_size,
    uint32_t allowed_gpi_mask, uint64_t *checked_count);
