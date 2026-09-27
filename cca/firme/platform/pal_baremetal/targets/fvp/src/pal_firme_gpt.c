/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_gpt.h"

/** @brief Compute a checked full-granule ownership envelope. */
pal_firme_gpt_scenario_status_t pal_firme_gpt_granule_envelope(
    uint64_t base, uint64_t size, uint64_t granule_size,
    uint64_t *covered_base, uint64_t *covered_size)
{
    uint64_t top;
    uint64_t aligned_base;

    if (!covered_base || !covered_size || !size || !granule_size ||
        ((granule_size & (granule_size - 1ULL)) != 0ULL) ||
        (base > UINT64_MAX - size))
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    top = base + size;
    if (top > UINT64_MAX - (granule_size - 1ULL))
        return PAL_FIRME_GPT_SCENARIO_ERROR;
    aligned_base = base & ~(granule_size - 1ULL);
    top = (top + granule_size - 1ULL) & ~(granule_size - 1ULL);
    *covered_base = aligned_base;
    *covered_size = top - aligned_base;
    return PAL_FIRME_GPT_SCENARIO_READY;
}

/**
 * @brief Report deterministic GPT scenario support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param abi ABI that must return the requested status.
 * @param expected_status Architected status to induce.
 * @param default_base Default suite test address.
 * @param scenario Returned scenario inputs and cleanup token.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE until the DUT exposes controls.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_prepare(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected_status,
    uint64_t default_base, pal_firme_gpt_scenario_t *scenario)
{
    (void)world;
    (void)abi;
    (void)expected_status;
    (void)default_base;

    if (scenario)
        *scenario = (pal_firme_gpt_scenario_t){0};

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Restore a deterministic GPT scenario on the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param abi ABI selected for the scenario.
 * @param expected_status Architected status selected for the scenario.
 * @param scenario Scenario returned by the prepare hook.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE when no scenario was prepared.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_scenario_cleanup(
    uint32_t world, pal_firme_gpt_abi_t abi, int64_t expected_status,
    const pal_firme_gpt_scenario_t *scenario)
{
    (void)world;
    (void)abi;
    (void)expected_status;
    (void)scenario;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report concurrent GPT_INFO setup support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param default_base Default suite test address.
 * @param evidence Returned concurrent-state evidence.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE without a real state-change setup.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_prepare(
    uint32_t world, uint64_t default_base,
    pal_firme_gpt_concurrent_info_t *evidence)
{
    (void)world;
    (void)default_base;

    if (evidence)
        *evidence = (pal_firme_gpt_concurrent_info_t){0};

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report concurrent GPT_INFO start support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param evidence Prepared concurrent-state evidence.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE without a prepared transition.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_begin(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence)
{
    (void)world;
    (void)evidence;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report concurrent GPT_INFO finish support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param evidence Concurrent-state evidence to update.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE without a prepared transition.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_finish(
    uint32_t world, pal_firme_gpt_concurrent_info_t *evidence)
{
    (void)world;
    (void)evidence;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report concurrent GPT_INFO cleanup support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param evidence Concurrent-state evidence to release.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE when no setup was prepared.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_concurrent_info_cleanup(
    uint32_t world, const pal_firme_gpt_concurrent_info_t *evidence)
{
    (void)world;
    (void)evidence;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report L1 GPT lifecycle setup support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under test.
 * @param condition Architectural condition to establish.
 * @param default_base Default suite test address.
 * @param scenario Returned platform scenario.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE until the DUT supports L1 GPTs.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_scenario_prepare(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition, uint64_t default_base,
    pal_firme_gpt_l1_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;
    (void)default_base;

    if (scenario)
        *scenario = (pal_firme_gpt_l1_scenario_t){0};

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report L1 GPT lifecycle cleanup support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under test.
 * @param condition Architectural condition that was established.
 * @param scenario Prepared platform scenario.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE when no setup was prepared.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_scenario_cleanup(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;
    (void)scenario;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report L1 GPT event monitoring support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under observation.
 * @param condition Architectural condition under observation.
 * @param scenario Prepared platform scenario.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE until instrumentation exists.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_monitor_begin(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;
    (void)scenario;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report L1 GPT event evidence support for the reference FVP.
 * @param world Caller world encoded as firme_acs_world_t.
 * @param operation L1 GPT ABI under observation.
 * @param condition Architectural condition under observation.
 * @param scenario Prepared platform scenario.
 * @param evidence Returned event evidence.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE until instrumentation exists.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_monitor_end(
    uint32_t world, pal_firme_gpt_l1_operation_t operation,
    pal_firme_gpt_l1_condition_t condition,
    const pal_firme_gpt_l1_scenario_t *scenario,
    pal_firme_gpt_l1_evidence_t *evidence)
{
    (void)world;
    (void)operation;
    (void)condition;
    (void)scenario;

    if (evidence)
        *evidence = (pal_firme_gpt_l1_evidence_t){0};

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}

/**
 * @brief Report independent L1 GPT GPI observation support for the FVP.
 * @param range_base First granule physical address.
 * @param range_size Size of the complete range in bytes.
 * @param granule_size Granule size in bytes.
 * @param exclude_base Optional excluded subrange base.
 * @param exclude_size Optional excluded subrange size.
 * @param allowed_gpi_mask Bit mask of accepted GPI encodings.
 * @param checked_count Number of granules checked.
 * @return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE until instrumentation exists.
 */
pal_firme_gpt_scenario_status_t pal_firme_gpt_l1_gpi_range_check(
    uint64_t range_base, uint64_t range_size, uint64_t granule_size,
    uint64_t exclude_base, uint64_t exclude_size,
    uint32_t allowed_gpi_mask, uint64_t *checked_count)
{
    (void)range_base;
    (void)range_size;
    (void)granule_size;
    (void)exclude_base;
    (void)exclude_size;
    (void)allowed_gpi_mask;

    if (checked_count)
        *checked_count = 0ULL;

    return PAL_FIRME_GPT_SCENARIO_UNAVAILABLE;
}
