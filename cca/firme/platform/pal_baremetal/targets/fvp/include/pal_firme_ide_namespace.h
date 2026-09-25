/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef enum {
    PAL_FIRME_IDE_NAMESPACE_BY_ECAM = 0,
    PAL_FIRME_IDE_NAMESPACE_BY_KEYSET,
} pal_firme_ide_namespace_kind_t;

typedef enum {
    PAL_FIRME_IDE_NAMESPACE_UNAVAILABLE = 0,
    PAL_FIRME_IDE_NAMESPACE_READY,
    PAL_FIRME_IDE_NAMESPACE_ERROR,
    PAL_FIRME_IDE_NAMESPACE_MISMATCH,
} pal_firme_ide_namespace_status_t;

typedef struct {
    uint64_t ecam_address;
    uint64_t keyset_id;
    uint64_t keyqw[4];
    uint64_t handle;
    uint32_t ide_type;
} pal_firme_ide_namespace_path_t;

typedef struct {
    pal_firme_ide_namespace_path_t path[2];
    uint64_t token;
} pal_firme_ide_namespace_scenario_t;

/**
 * @brief Reserve two independent IDE paths and configure their endpoints.
 * @param kind Select equal IDs in distinct ECAMs or distinct IDs in one ECAM.
 * @param scenario Returned paths, distinct keys, handles and cleanup token.
 * @return READY only when protected-traffic observation is available.
 *         A non-READY result must leave no reserved platform resources.
 */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_prepare(
    pal_firme_ide_namespace_kind_t kind,
    pal_firme_ide_namespace_scenario_t *scenario);

/**
 * @brief Observe both Root Port streams using protected endpoint traffic.
 * @param kind Namespace scenario selected during preparation.
 * @param scenario Prepared paths and endpoint configuration.
 * @param first_active Expected first-path active state.
 * @param second_active Expected second-path active state.
 * @return READY for independently observed matching states, else an error.
 */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_observe(
    pal_firme_ide_namespace_kind_t kind,
    const pal_firme_ide_namespace_scenario_t *scenario,
    uint32_t first_active, uint32_t second_active);

/**
 * @brief Restore endpoint and platform state reserved for the scenario.
 * @param kind Namespace scenario selected during preparation.
 * @param scenario Prepared paths and cleanup token.
 * @return READY when all platform resources are restored, else an error.
 */
pal_firme_ide_namespace_status_t pal_firme_ide_namespace_cleanup(
    pal_firme_ide_namespace_kind_t kind,
    const pal_firme_ide_namespace_scenario_t *scenario);
