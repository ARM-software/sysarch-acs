/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_ide.h"
#include "val_firme_rules.h"
#include "val_log.h"
#include "val_pcie.h"

typedef struct {
    uint64_t ecam_address;
    uint64_t keyset_id;
    uint64_t pending_handle;
    uint32_t programmed;
    uint32_t pending;
} firme_ide_test_state_t;

/**
 * @brief Select the PAL-reserved Selective IDE keyset, not an invented stream.
 * @return Initial test-owned keyset state.
 */
static inline firme_ide_test_state_t firme_ide_test_state(void)
{
    firme_ide_test_state_t state = {0};

    state.ecam_address = val_pcie_ecam_base();
    state.keyset_id = val_firme_ide_default_keyset_id();
    return state;
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
 * @brief Validate a consumed response without interpreting unused outputs.
 * @param state Test-owned keyset state.
 * @param res POLL response.
 * @return VAL_SUCCESS for matching completion, otherwise VAL_ERROR.
 */
static inline int firme_ide_test_polled(firme_ide_test_state_t *state,
                                       const firme_res_t *res)
{
    int64_t status = val_firme_status_to_i64(res->x0);

    if (status != FIRME_BUSY && status != FIRME_INCOMPLETE)
        state->pending = VAL_FALSE;
    if (status != FIRME_SUCCESS || res->x1 != state->pending_handle)
        return VAL_ERROR;
    return VAL_SUCCESS;
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

    return out->status == ACS_STATUS_FAIL ? VAL_ERROR : VAL_SUCCESS;

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
