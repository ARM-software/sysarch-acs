/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_ide_abi.h"

/**
 * @brief Report that the reference FVP has no observable IDE endpoint path.
 * @param world FIRME caller world.
 * @param operation IDE ABI to exercise.
 * @param condition Scenario to prepare.
 * @param scenario Returned scenario, cleared when present.
 * @return UNAVAILABLE until real Root Port programming and cleanup exist.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_prepare(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition,
    pal_firme_ide_abi_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;

    if (scenario)
        *scenario = (pal_firme_ide_abi_scenario_t){0};

    return PAL_FIRME_IDE_ABI_UNAVAILABLE;
}

/**
 * @brief Report that no reference-FVP IDE ABI state was reserved.
 * @param world FIRME caller world.
 * @param operation IDE ABI exercised.
 * @param condition Scenario that was prepared.
 * @param scenario Previously reserved path, if any.
 * @return UNAVAILABLE because no test path was prepared.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_cleanup(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition,
    const pal_firme_ide_abi_scenario_t *scenario)
{
    (void)world;
    (void)operation;
    (void)condition;
    (void)scenario;

    return PAL_FIRME_IDE_ABI_UNAVAILABLE;
}
