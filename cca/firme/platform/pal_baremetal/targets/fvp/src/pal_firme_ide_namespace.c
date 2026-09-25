/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_firme_ide_namespace.h"

/**
 * @brief Report that the reference FVP lacks independent IDE path evidence.
 * @param kind Requested namespace scenario.
 * @param scenario Returned scenario, cleared when present.
 * @return UNAVAILABLE until two observable endpoint paths are provided.
 */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_prepare(
    pal_firme_ide_namespace_kind_t kind,
    pal_firme_ide_namespace_scenario_t *scenario)
{
    (void)kind;

    if (scenario)
        *scenario = (pal_firme_ide_namespace_scenario_t){0};

    return PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE;
}

/**
 * @brief Reject IDE observations unavailable on the reference FVP.
 * @param kind Requested namespace scenario.
 * @param scenario Prepared paths, if any.
 * @param first_active Expected first-path active state.
 * @param second_active Expected second-path active state.
 * @return UNAVAILABLE because protected traffic cannot be observed.
 */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_observe(
    pal_firme_ide_namespace_kind_t kind,
    const pal_firme_ide_namespace_scenario_t *scenario,
    uint32_t first_active, uint32_t second_active)
{
    (void)kind;
    (void)scenario;
    (void)first_active;
    (void)second_active;

    return PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE;
}

/**
 * @brief Report that no reference-FVP IDE namespace state was prepared.
 * @param kind Requested namespace scenario.
 * @param scenario Prepared paths, if any.
 * @return UNAVAILABLE because no reference-FVP paths can be prepared.
 */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_cleanup(
    pal_firme_ide_namespace_kind_t kind,
    const pal_firme_ide_namespace_scenario_t *scenario)
{
    (void)kind;
    (void)scenario;

    return PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE;
}
