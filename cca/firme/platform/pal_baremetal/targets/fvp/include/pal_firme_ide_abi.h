/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

typedef enum {
    PAL_FIRME_IDE_ABI_UNAVAILABLE = 0,
    PAL_FIRME_IDE_ABI_READY,
    PAL_FIRME_IDE_ABI_ERROR,
} pal_firme_ide_abi_status_t;

typedef enum {
    PAL_FIRME_IDE_ABI_PROG = 0,
    PAL_FIRME_IDE_ABI_GO,
    PAL_FIRME_IDE_ABI_STOP,
    PAL_FIRME_IDE_ABI_POLL,
} pal_firme_ide_abi_operation_t;

typedef enum {
    PAL_FIRME_IDE_ABI_VALID = 0,
    PAL_FIRME_IDE_ABI_INVALID_ECAM,
    PAL_FIRME_IDE_ABI_INVALID_TYPE,
    PAL_FIRME_IDE_ABI_NO_PENDING,
    PAL_FIRME_IDE_ABI_POLL_ANY,
} pal_firme_ide_abi_condition_t;

typedef struct {
    uint64_t ecam_address;
    uint64_t invalid_ecam_address;
    uint64_t keyset_id;
    uint64_t keyqw[4];
    uint64_t handle;
    uint64_t token;
    uint32_t ide_type;
} pal_firme_ide_abi_scenario_t;

/**
 * @brief Reserve one real IDE Root Port and endpoint path for an ABI case.
 * @param world FIRME caller world.
 * @param operation IDE ABI to exercise.
 * @param condition Scenario to prepare.
 * @param scenario Returned path, key, handle and cleanup token.
 * @return READY only when the path and rollback are safe to use. UNAVAILABLE
 *         leaves no reservation; ERROR may quarantine a backend-owned fixture.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_prepare(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition,
    pal_firme_ide_abi_scenario_t *scenario);

/**
 * @brief Restore the paired fixture after the adapter drains and stops its raw keyset.
 * @param world FIRME caller world.
 * @param operation IDE ABI exercised.
 * @param condition Scenario that was prepared.
 * @param scenario Previously reserved path and cleanup token.
 * @return READY when all platform state has been restored, else ERROR.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_cleanup(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition,
    const pal_firme_ide_abi_scenario_t *scenario);

/**
 * @brief Establish a measured pre-state after the adapter's setup operations.
 * @return READY only when the exact invocation has a usable functional observer.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_ready(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition,
    const pal_firme_ide_abi_scenario_t *scenario);
/**
 * @brief Observe the issued invocation without reprogramming its Root Port keyset.
 * @return READY only for the expected traffic effect or independently proven no-effect.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_observe(
    uint32_t world, pal_firme_ide_abi_operation_t operation,
    pal_firme_ide_abi_condition_t condition,
    const pal_firme_ide_abi_scenario_t *scenario);
/**
 * @brief Stop the peer transmitter before the adapter stops its receive keyset.
 * @return READY only after peer STOP is acknowledged; failure retains ownership.
 */
pal_firme_ide_abi_status_t pal_firme_ide_abi_quiesce(
    uint32_t world, const pal_firme_ide_abi_scenario_t *scenario);
