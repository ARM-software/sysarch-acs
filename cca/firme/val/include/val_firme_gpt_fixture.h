/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "pal_firme_gpt.h"
#include "val_firme_gpt.h"
#include "val_firme_abi_test.h"

typedef struct {
    pal_firme_gpt_memory_t memory;
    pal_firme_gpt_memory_kind_t kind;
    firme_acs_world_t world;
    uint64_t granule_size;
    uint64_t range_size;
    uint64_t object_size;
    uint64_t object_granule_base;
    uint64_t object_granule_size;
    uint64_t target_gpi;
    uint64_t count;
    uint64_t completed;
    uint64_t cookie;
    int64_t last_status;
    uint32_t pending;
    uint32_t poisoned;
    uint32_t l1_present;
    uint32_t l1_destroy;
    uint32_t restoring;
} val_firme_gpt_fixture_t;

/** @brief Acquire and independently validate a disposable platform fixture.
 * @param fixture Unused caller-owned session, retained until cleanup succeeds.
 * @param world Calling world; subsequent calls must run on the same PE/world.
 * @param kind Transition, self-describing, or non-self-describing layout.
 * @return READY, UNAVAILABLE before destructive work, or ERROR. A nonzero token
 * on error remains reserved and must be passed to cleanup, never discarded.
 */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_begin(
    val_firme_gpt_fixture_t *fixture, firme_acs_world_t world,
    pal_firme_gpt_memory_kind_t kind);

/** @brief Record one real GPI response, respecting per-call count validity.
 * @param fixture Prepared session describing the exact issued call.
 * @param result Actual GPI_SET or OP_CONTINUE response.
 * @return VAL_SUCCESS for a well-formed progress row, otherwise VAL_ERROR.
 */
int val_firme_gpt_fixture_gpi_record(val_firme_gpt_fixture_t *fixture,
                                      const firme_res_t *result);

/** @brief Issue one real GPI_SET or retained-cookie continuation call.
 * @param fixture Prepared session, updated with progress and retained state.
 * @param result Raw returned registers.
 * @return VAL_SUCCESS for a valid row, not necessarily terminal SUCCESS.
 */
int val_firme_gpt_fixture_gpi_step(val_firme_gpt_fixture_t *fixture,
                                    firme_res_t *result);

/** @brief Seek a real retained operation without inventing INCOMPLETE or BUSY.
 * @param fixture Prepared transition session.
 * @return READY if a valid INCOMPLETE cookie is retained; UNAVAILABLE if the
 * request finishes synchronously; ERROR on invalid status or bounded timeout.
 * Every result requires cleanup of any acquired reservation.
 */
pal_firme_gpt_scenario_status_t val_firme_gpt_fixture_retain(
    val_firme_gpt_fixture_t *fixture);

/** @brief Check conflict on the first unfinished granule of a retained request.
 * @param fixture Real paused GPI operation; cookie is preserved on conflict.
 * @return VAL_SUCCESS only for OP_CONFLICT with no progress. Unexpected effects
 * quarantine the lease rather than guessing which operation remains pending.
 */
int val_firme_gpt_fixture_conflict(val_firme_gpt_fixture_t *fixture);

/** @brief Record one actual L1 CREATE or DESTROY response.
 * @param fixture Prepared L1 session.
 * @param destroy VAL_TRUE for DESTROY, otherwise CREATE.
 * @param result Actual response; DESTROY object base is x2 on SUCCESS only.
 * @return VAL_SUCCESS for SUCCESS, BUSY, or INCOMPLETE with valid correlation.
 */
int val_firme_gpt_fixture_l1_record(val_firme_gpt_fixture_t *fixture,
                                     uint32_t destroy, const firme_res_t *result);

/** @brief Complete a real L1 operation with bounded BUSY/INCOMPLETE retries.
 * @param fixture Prepared session.
 * @param destroy VAL_TRUE for DESTROY, otherwise CREATE.
 * @return VAL_SUCCESS only after terminal SUCCESS and a matching object base.
 */
int val_firme_gpt_fixture_l1_complete(val_firme_gpt_fixture_t *fixture,
                                       uint32_t destroy);

/** @brief Complete retained work, restore initial state, then release storage.
 * @param fixture Session; failed/uncertain cleanup keeps its lease quarantined.
 * @return VAL_SUCCESS only after independent full-range GPI verification and,
 * for L1 fixtures, GPT_INFO confirms the original Host-managed block.
 */
int val_firme_gpt_fixture_cleanup(val_firme_gpt_fixture_t *fixture);

/** @brief Return generated GPT operations without changing registry gates.
 * @param abi_id GPI_SET, OP_CONTINUE, L1_CREATE, or L1_DESTROY identifier.
 * @return Adapter operations, or NULL for an unrelated ABI.
 */
const val_firme_abi_test_ops_t *val_firme_gpt_abi_adapter_get(
    val_firme_abi_id_t abi_id);
