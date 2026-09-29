/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cxl_test_common.h"
#include "val_log.h"
#include "val_pcie.h"

static uint32_t g_cxl_test_reserved;

/**
 * @brief Reserve the field-test CXL path within this caller payload.
 * @param state Empty caller-owned operation state.
 * @param keyset_id Valid baseline keyset on the configured CXL path.
 * @return VAL_SUCCESS, or VAL_ERROR when an earlier owner still needs recovery.
 */
int firme_ide_cxl_begin(firme_ide_cxl_state_t *state, uint64_t keyset_id)
{
    uint32_t expected = VAL_FALSE;

    if (!state || state->acquired)
        return VAL_ERROR;

    *state = (firme_ide_cxl_state_t){0};
    if (!__atomic_compare_exchange_n(&g_cxl_test_reserved, &expected, VAL_TRUE,
                                     VAL_FALSE, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        LOG_ERROR(NULL, "CXL field-test path still owned: recovery/reset required\n");
        return VAL_ERROR;
    }

    state->acquired = VAL_TRUE;
    state->ecam_address = val_pcie_ecam_base();
    state->keyset_id = keyset_id;
    return VAL_SUCCESS;
}

/**
 * @brief Consume only the response belonging to this known pending operation.
 * @param state Active field-test owner.
 * @param res Returned exact-keyset POLL response; unused outputs are ignored.
 * @return VAL_SUCCESS for matching SUCCESS, otherwise VAL_ERROR.
 */
static int firme_ide_cxl_poll(firme_ide_cxl_state_t *state, firme_res_t *res)
{
    int64_t status;

    val_firme_ide_keyset_poll_do_retry(state->ecam_address,
                                      val_firme_ide_poll_flags(VAL_FALSE),
                                      state->keyset_id, res);
    status = val_firme_status_to_i64(res->x0);
    if (status == FIRME_SUCCESS || status == FIRME_ABORTED)
        state->pending = VAL_FALSE;
    if (status == FIRME_SUCCESS) {
        if (res->x1 == state->pending_handle)
            return VAL_SUCCESS;
        state->unknown = VAL_TRUE;
    } else if (status != FIRME_BUSY && status != FIRME_INCOMPLETE &&
               status != FIRME_ABORTED) {
        state->unknown = VAL_TRUE;
    }

    return VAL_ERROR;
}

/**
 * @brief Record acceptance before attempting bounded asynchronous completion.
 * @param state Active field-test owner.
 * @param handle Handle supplied with the operation.
 * @param res Raw operation response, replaced by POLL outputs if asynchronous.
 * @return VAL_SUCCESS only when the operation completes successfully.
 */
static int firme_ide_cxl_complete(firme_ide_cxl_state_t *state, uint64_t handle,
                                  firme_res_t *res)
{
    int64_t status = val_firme_status_to_i64(res->x0);

    if (status == FIRME_SUCCESS || status == FIRME_INCOMPLETE)
        state->programmed = VAL_TRUE;
    if (status == FIRME_SUCCESS)
        return VAL_SUCCESS;
    if (status == FIRME_INCOMPLETE) {
        state->pending = VAL_TRUE;
        state->pending_handle = handle;
        return firme_ide_cxl_poll(state, res);
    }

    if (status != FIRME_NOT_SUPPORTED && status != FIRME_INVALID_PARAMETERS &&
        status != FIRME_BUSY && status != FIRME_OP_CONFLICT && status != FIRME_DENIED)
        state->unknown = VAL_TRUE;
    return VAL_ERROR;
}

/**
 * @brief Program the owned valid CXL keyset and retain incomplete operation state.
 * @param state Active field-test owner.
 * @param handle Caller handle for this programming request.
 * @param res Returned operation or completion response.
 * @return VAL_SUCCESS only after successful completion, otherwise VAL_ERROR.
 */
int firme_ide_cxl_prog(firme_ide_cxl_state_t *state, uint64_t handle, firme_res_t *res)
{
    val_firme_ide_prog_args_t args = {0};

    if (!state || !res || !state->acquired || state->pending || state->unknown)
        return VAL_ERROR;
    val_firme_ide_prog_args_init(&args, state->ecam_address,
                                VAL_FIRME_IDE_TYPE_CXL_CACHEMEM, state->keyset_id, handle);
    val_firme_ide_keyset_prog_do_retry(&args, res);
    return firme_ide_cxl_complete(state, handle, res);
}

/**
 * @brief Activate only the successfully programmed field-test keyset.
 * @param state Active field-test owner.
 * @param handle Caller handle for this activation request.
 * @param res Returned operation or completion response.
 * @return VAL_SUCCESS only after successful completion, otherwise VAL_ERROR.
 */
int firme_ide_cxl_go(firme_ide_cxl_state_t *state, uint64_t handle, firme_res_t *res)
{
    if (!state || !res || !state->acquired || !state->programmed ||
        state->pending || state->unknown)
        return VAL_ERROR;
    val_firme_ide_keyset_go_do_retry(state->ecam_address,
                                    val_firme_ide_flags(VAL_FIRME_IDE_TYPE_CXL_CACHEMEM),
                                    state->keyset_id, handle, res);
    return firme_ide_cxl_complete(state, handle, res);
}

/**
 * @brief Quarantine unexpected acceptance without guessing a malformed keyset's owner.
 * @param state Active field-test owner whose baseline must not be reused.
 * @param res Response to an input required to be rejected.
 */
void firme_ide_cxl_rejected(firme_ide_cxl_state_t *state, const firme_res_t *res)
{
    int64_t status;

    if (!state || !res || !state->acquired)
        return;
    status = val_firme_status_to_i64(res->x0);
    if (status != FIRME_INVALID_PARAMETERS && status != FIRME_NOT_SUPPORTED &&
        status != FIRME_BUSY)
        state->unknown = VAL_TRUE;
}

/**
 * @brief Drain accepted work and stop the known keyset before releasing the path.
 * @param state Active owner; unsafe cleanup retains the reservation until reset.
 * @param handle Handle for the cleanup STOP request.
 * @param res Returned cleanup diagnostic; original rule failures belong to the caller.
 * @return VAL_SUCCESS for safe cleanup, otherwise VAL_ERROR requiring recovery.
 */
int firme_ide_cxl_finish(firme_ide_cxl_state_t *state, uint64_t handle, firme_res_t *res)
{
    int completion = VAL_SUCCESS;

    if (!state || !res || !state->acquired)
        return VAL_ERROR;
    if (state->unknown)
        goto fail;
    if (state->pending) {
        completion = firme_ide_cxl_poll(state, res);
        if (state->pending || state->unknown)
            goto fail;
    }

    if (state->programmed) {
        val_firme_ide_keyset_stop_do_retry(state->ecam_address,
                                          val_firme_ide_flags(VAL_FIRME_IDE_TYPE_CXL_CACHEMEM),
                                          state->keyset_id, handle, res);
        if (firme_ide_cxl_complete(state, handle, res) != VAL_SUCCESS)
            goto fail;
        state->programmed = VAL_FALSE;
    }

    state->acquired = VAL_FALSE;
    __atomic_store_n(&g_cxl_test_reserved, VAL_FALSE, __ATOMIC_RELEASE);
    return completion;

fail:
    LOG_ERROR(NULL, "CXL cleanup failed: keyset=%llx x0=%llx; recovery/reset required\n",
              (unsigned long long)state->keyset_id, (unsigned long long)res->x0);
    return VAL_ERROR;
}
