/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_firme_ide_observation.h"
#include "val_log.h"
#include "val_pcie.h"
#include "val_pcie_ide.h"

typedef struct {
    val_pcie_ide_stream_t stream;
    uint64_t ecam_address;
    uint64_t keyset_id;
    uint64_t pending_handle;
    uint32_t programmed;
    uint32_t pending;
    uint32_t quarantined;
} firme_ide_test_state_t;

/** @brief Observe the completed Selective request while its exact stream remains owned. */
static inline int firme_ide_test_observe(const firme_ide_test_state_t *state,
                                         uint64_t operation, uint64_t handle,
                                         const val_firme_ide_prog_args_t *program,
                                         firme_rule_res_t *out)
{
    val_firme_ide_observation_t selection = {0};

    selection.stream = &state->stream;
    selection.keyset_id = state->keyset_id;
    selection.handle = handle;
    selection.key[0] = program->keyqw0;
    selection.key[1] = program->keyqw1;
    selection.key[2] = program->keyqw2;
    selection.key[3] = program->keyqw3;
    return val_firme_ide_check_effect(operation, FIRME_ACS_WORLD_REALM, &selection, out);
}

/**
 * @brief Discover and configure the Selective pair from the Realm caller.
 * @param state Initially empty test-owned state.
 * @param out Rule result, including unavailable-resource diagnostics.
 * @return VAL_TRUE when ready, otherwise VAL_FALSE with SKIP or FAIL recorded.
 */
static inline uint32_t firme_ide_test_prepare(firme_ide_test_state_t *state,
                                             firme_rule_res_t *out)
{
    val_firme_ide_keyset_fields_t fields = {0};
    uint32_t result = ACS_STATUS_SKIP;

    result = val_pcie_ide_stream_discover(VAL_TRUE, &state->stream);
    if (result != ACS_STATUS_PASS) {
        out->status = result;
        out->fail_step = state->stream.reason;
        if (result == ACS_STATUS_FAIL) {
            out->last_x0 = state->stream.error_bdf;
            out->last_x1 = state->stream.error_offset;
            if (state->stream.reason == VAL_PCIE_IDE_OWNER_BUSY) {
                LOG_ERROR(NULL, "Selective IDE setup failed: ACS stream already owned\n");
            } else {
                LOG_ERROR(NULL, "Selective IDE setup failed: BDF=%llx offset=%llx\n",
                          (unsigned long long)out->last_x0,
                          (unsigned long long)out->last_x1);
            }
        } else {
            out->last_x0 = state->stream.path.root_port_bdf;
            out->last_x1 = state->stream.path.endpoint_bdf;
            LOG_WARN(NULL, "Selective IDE setup unavailable: no usable ACS-owned pair\n");
        }
        return VAL_FALSE;
    }
    fields.segment = state->stream.path.segment;
    fields.root_port_id = state->stream.path.root_port_id;
    fields.stream_id = state->stream.stream_id;
    state->ecam_address = state->stream.path.ecam_base;
    state->keyset_id = val_firme_ide_keyset_id(&fields);
    return VAL_TRUE;
}

/**
 * @brief Remember accepted requests so failures cannot bypass cleanup.
 * @param state Test-owned keyset state.
 * @param res ABI response.
 * @param handle Caller handle supplied with this request.
 */
static inline void firme_ide_test_track(firme_ide_test_state_t *state,
                                       const firme_res_t *res, uint64_t handle)
{
    int64_t status = val_firme_status_to_i64(res->x0);

    if (status == FIRME_SUCCESS || status == FIRME_INCOMPLETE)
        state->programmed = VAL_TRUE;
    if (status == FIRME_INCOMPLETE) {
        state->pending = VAL_TRUE;
        state->pending_handle = handle;
    }
}

/**
 * @brief Retain ownership if a request expected to be rejected was accepted.
 * @param state Test reservation whose DUT operation state is now uncertain.
 * @param res Unexpected response to malformed or conflicting input.
 */
static inline void firme_ide_test_unexpected_accept(firme_ide_test_state_t *state,
                                                    const firme_res_t *res)
{
    int64_t status = val_firme_status_to_i64(res->x0);

    if (status == FIRME_SUCCESS || status == FIRME_INCOMPLETE)
        state->quarantined = VAL_TRUE;
}

/**
 * @brief Validate a consumed response without interpreting unused outputs.
 * @param state Test-owned keyset state.
 * @param res POLL response.
 * @return VAL_SUCCESS for matching completion, otherwise VAL_ERROR.
 */
static inline int firme_ide_test_polled(firme_ide_test_state_t *state,
                                       const firme_res_t *res)
{
    int64_t status;

    if (!state || !res || state->pending != VAL_TRUE)
        return VAL_ERROR;

    status = val_firme_status_to_i64(res->x0);
    if (status == FIRME_SUCCESS && res->x1 == state->pending_handle) {
        state->pending = VAL_FALSE;
        return VAL_SUCCESS;
    }
    if (status == FIRME_SUCCESS) {
        LOG_ERROR(NULL, "IDE completion handle mismatch: expected=%llx received=%llx\n",
                  (unsigned long long)state->pending_handle,
                  (unsigned long long)res->x1);
        state->quarantined = VAL_TRUE;
    } else if (status != FIRME_BUSY && status != FIRME_INCOMPLETE) {
        state->quarantined = VAL_TRUE;
    }
    return VAL_ERROR;
}

/**
 * @brief Complete a pending request, or accept synchronous completion.
 * @param state Test-owned keyset state.
 * @param res Operation response, replaced by POLL outputs when applicable.
 * @return VAL_SUCCESS for successful completion, otherwise VAL_ERROR.
 */
static inline int firme_ide_test_complete(firme_ide_test_state_t *state,
                                         firme_res_t *res)
{
    if (!state->pending)
        return val_firme_status_to_i64(res->x0) == FIRME_SUCCESS ?
               VAL_SUCCESS : VAL_ERROR;

    val_firme_ide_keyset_poll_do_retry(state->ecam_address,
                                      val_firme_ide_poll_flags(VAL_FALSE),
                                      state->keyset_id, res);
    return firme_ide_test_polled(state, res);
}

/**
 * @brief Drain owned requests and disable the stream before sequential reuse.
 * @param state Test-owned keyset state.
 * @param out Rule result whose original failure must be preserved.
 * @return VAL_SUCCESS for PASS/SKIP with cleanup, otherwise VAL_ERROR.
 */
static inline int firme_ide_test_cleanup(firme_ide_test_state_t *state,
                                        firme_rule_res_t *out)
{
    firme_res_t res = {0};
    uint64_t handle = UINT64_C(0x494445434c45414e);

    if (state->quarantined) {
        LOG_ERROR(NULL, "IDE stream quarantined: DUT accepted a rejected-input scenario\n");
        out->status = ACS_STATUS_FAIL;
        return VAL_ERROR;
    }
    if (state->pending && firme_ide_test_complete(state, &res) != VAL_SUCCESS)
        goto fail;
    if (state->programmed) {
        val_firme_ide_keyset_stop_do_retry(state->ecam_address,
                                          val_firme_ide_flags(VAL_FIRME_IDE_TYPE_SELECTIVE),
                                          state->keyset_id, handle, &res);
        firme_ide_test_track(state, &res, handle);
        if (firme_ide_test_complete(state, &res) != VAL_SUCCESS)
            goto fail;
        state->programmed = VAL_FALSE;
    }
    if (state->stream.acquired &&
        val_pcie_ide_stream_release(&state->stream) != VAL_SUCCESS)
        goto fail;

    return out->status == ACS_STATUS_PASS || out->status == ACS_STATUS_SKIP ?
           VAL_SUCCESS : VAL_ERROR;

fail:
    LOG_ERROR(NULL, "IDE test cleanup failed: keyset=%llx x0=%llx x1=%llx",
              (unsigned long long)state->keyset_id, (unsigned long long)res.x0,
              (unsigned long long)res.x1);
    if (out->status != ACS_STATUS_FAIL) {
        out->status = ACS_STATUS_FAIL;
        out->last_x0 = res.x0;
        out->last_x1 = res.x1;
    }
    return VAL_ERROR;
}
