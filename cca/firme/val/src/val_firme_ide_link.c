/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_ide_link.h"

#include "val_firme_ide.h"
#include "val_common/val_mem.h"
#include "val_common/val_mp.h"
#include "val_common/val_mmu.h"
#include "val_firme_rules.h"
#include "val_pcie.h"

#define VAL_IDE_LINK_WORDS_LOW  1U
#define VAL_IDE_LINK_WORDS_HIGH 2U
#define VAL_IDE_LINK_WORDS_ALL  3U

typedef struct {
    val_firme_ide_prog_args_t args;
    uint32_t open;
    uint32_t pe_index;
    uint32_t words;
    uint32_t programmed;
    uint32_t stopped;
    uint32_t pending;
    uint32_t quarantined;
    uint64_t pending_operation;
} val_firme_ide_link_state_t;

static val_firme_ide_link_state_t g_val_ide_link[2];
static uint64_t g_val_ide_link_handle;

/**
 * @brief Apply only an acknowledged operation to the retained link state.
 * @param state Owner of the acknowledged operation.
 */
static void val_firme_ide_link_acknowledge(val_firme_ide_link_state_t *state)
{
    if (state->pending_operation == VAL_IDE_LINK_PROG) {
        state->programmed = VAL_TRUE;
        state->stopped = VAL_FALSE;
        state->words = 0U;
        state->args.keyqw0 = 0ULL;
        state->args.keyqw1 = 0ULL;
        state->args.keyqw2 = 0ULL;
        state->args.keyqw3 = 0ULL;
    } else {
        state->stopped = (state->pending_operation == VAL_IDE_LINK_STOP);
    }
    state->pending = VAL_FALSE;
    state->pending_operation = 0ULL;
}

/**
 * @brief Drain one retained operation without reissuing PROG, GO or STOP.
 * @param state Owner of the operation and any still-needed key material.
 * @param res Raw POLL response, including the original error on failure.
 * @return VAL_SUCCESS only for a correlated completion or an idle owner.
 */
static int val_firme_ide_link_drain(val_firme_ide_link_state_t *state,
                                    firme_res_t *res)
{
    int64_t status;

    if (state->quarantined)
        return VAL_ERROR;
    if (!state->pending)
        return VAL_SUCCESS;

    val_firme_ide_keyset_poll_do_retry(state->args.ecam_address,
                                       val_firme_ide_poll_flags(VAL_FALSE),
                                       state->args.keyset_id, res);
    status = val_firme_status_to_i64(res->x0);
    if (status == FIRME_SUCCESS && res->x1 == state->args.handle) {
        val_firme_ide_link_acknowledge(state);
        return VAL_SUCCESS;
    }
    if (status != FIRME_BUSY && status != FIRME_INCOMPLETE)
        state->quarantined = VAL_TRUE;
    return VAL_ERROR;
}

/**
 * @brief Complete an accepted operation before reusing its keyset or handle.
 * @param state Owner of the operation and its key material.
 * @param res In/out FIRME response; errors and unfinished operations are retained.
 * @return VAL_SUCCESS only for correlated SUCCESS, preserving failures otherwise.
 */
static int val_firme_ide_link_complete(val_firme_ide_link_state_t *state,
                                        firme_res_t *res)
{
    int64_t status = val_firme_status_to_i64(res->x0);

    if (status == FIRME_SUCCESS) {
        val_firme_ide_link_acknowledge(state);
        return VAL_SUCCESS;
    }
    if (status == FIRME_INCOMPLETE) {
        state->pending = VAL_TRUE;
        return val_firme_ide_link_drain(state, res);
    }
    if (status != FIRME_BUSY && status != FIRME_DENIED &&
        status != FIRME_INVALID_PARAMETERS && status != FIRME_NOT_SUPPORTED)
        state->quarantined = VAL_TRUE;
    return VAL_ERROR;
}

/**
 * @brief Execute a serial link operation on its owning PE in the selected caller world.
 * @param operation ACS link command, not an EL3 ABI identifier.
 * @param keyset_id Fully encoded, test-owned CXL keyset.
 * @param data0 ECAM base or first key quadword for the selected command.
 * @param data1 Second key quadword; zero for commands without key data.
 * @param out ACS status and raw FIRME status/diagnostic words.
 */
void val_firme_ide_link_command(uint64_t operation, uint64_t keyset_id,
                                uint64_t data0, uint64_t data1,
                                firme_acs_call_res_t *out)
{
    val_firme_ide_link_state_t *state;
    firme_res_t res = {0};
    uint32_t direction;
    uint32_t root_bdf;
    uint64_t ecam_base;
    int completion = VAL_SUCCESS;

    if (!out)
        return;

    memset(out, 0, sizeof(*out));
    out->x0 = ACS_STATUS_FAIL;
    out->x1 = (uint64_t)FIRME_INVALID_PARAMETERS;
    if (!val_mmu_is_controller_pe())
        return;

    if (operation == VAL_IDE_LINK_DISCOVER) {
        val_firme_service_version(FIRME_SERVICE_ID_IDE_KEY_MGMT, &res);
        out->x1 = res.x0;
        if (val_firme_status_to_i64(res.x0) >= 0)
            out->x0 = ACS_STATUS_PASS;
        else if (val_firme_status_to_i64(res.x0) == FIRME_NOT_SUPPORTED)
            out->x0 = ACS_STATUS_SKIP;
        return;
    }

    if ((keyset_id & ~VAL_FIRME_IDE_KEYSET_ID_INPUT_MASK) != 0ULL ||
        ((keyset_id >> VAL_FIRME_IDE_KEYSET_SUBSTREAM_SHIFT) &
         VAL_FIRME_IDE_KEYSET_SUBSTREAM_MASK) != VAL_FIRME_IDE_CXL_SUBSTREAM_ID ||
        ((keyset_id >> VAL_FIRME_IDE_KEYSET_STREAM_SHIFT) &
         VAL_FIRME_IDE_KEYSET_STREAM_MASK) != 0ULL)
        return;

    direction = (uint32_t)((keyset_id >> VAL_FIRME_IDE_KEYSET_DIRECTION_SHIFT) &
                          VAL_FIRME_IDE_KEYSET_DIRECTION_MASK);
    state = &g_val_ide_link[direction];
    if (operation == VAL_IDE_LINK_BEGIN) {
        root_bdf = val_pcie_bdf_from_requester_id(
            (uint32_t)((keyset_id >> VAL_FIRME_IDE_KEYSET_SEGMENT_SHIFT) &
                       VAL_FIRME_IDE_KEYSET_SEGMENT_MASK),
            (uint32_t)((keyset_id >> VAL_FIRME_IDE_KEYSET_ROOT_PORT_SHIFT) &
                       VAL_FIRME_IDE_KEYSET_ROOT_PORT_MASK));
        if (state->open || data1 != 0ULL ||
            val_pcie_ecam_base_for_bdf(root_bdf, &ecam_base) != VAL_SUCCESS ||
            data0 != ecam_base)
            return;
        memset(state, 0, sizeof(*state));
        state->args.ecam_address = data0;
        state->args.flags = val_firme_ide_flags(VAL_FIRME_IDE_TYPE_CXL_CACHEMEM);
        state->args.keyset_id = keyset_id;
        state->pe_index = val_pe_index();
        state->open = VAL_TRUE;
    } else {
        if (!state->open || state->quarantined || state->args.keyset_id != keyset_id ||
            state->pe_index != val_pe_index())
            return;
        if (operation == VAL_IDE_LINK_DRAIN || operation == VAL_IDE_LINK_QUIESCE) {
            completion = val_firme_ide_link_drain(state, &res);
            if (completion != VAL_SUCCESS || operation == VAL_IDE_LINK_DRAIN)
                goto result;
            if (!state->programmed || state->stopped)
                goto result;
            operation = VAL_IDE_LINK_STOP;
        } else if (state->pending) {
            return;
        }
        if (operation == VAL_IDE_LINK_PROG || operation == VAL_IDE_LINK_GO ||
            operation == VAL_IDE_LINK_STOP) {
            if (g_val_ide_link_handle == UINT64_MAX)
                return;
            state->args.handle = ++g_val_ide_link_handle;
            state->pending_operation = operation;
        }
        switch (operation) {
        case VAL_IDE_LINK_KEY_LOW:
        case VAL_IDE_LINK_KEY_HIGH:
            if (state->programmed && !state->stopped)
                return;
            if (operation == VAL_IDE_LINK_KEY_LOW) {
                state->args.keyqw0 = data0;
                state->args.keyqw1 = data1;
                state->words = VAL_IDE_LINK_WORDS_LOW;
            } else {
                if (state->words != VAL_IDE_LINK_WORDS_LOW)
                    return;
                state->args.keyqw2 = data0;
                state->args.keyqw3 = data1;
                state->words |= VAL_IDE_LINK_WORDS_HIGH;
            }
            break;
        case VAL_IDE_LINK_PROG:
            if (state->words != VAL_IDE_LINK_WORDS_ALL ||
                (state->programmed && !state->stopped))
                return;
            val_firme_ide_keyset_prog_do_retry(&state->args, &res);
            completion = val_firme_ide_link_complete(state, &res);
            break;
        case VAL_IDE_LINK_GO:
        case VAL_IDE_LINK_STOP:
            if (!state->programmed)
                return;
            if (operation == VAL_IDE_LINK_GO)
                val_firme_ide_keyset_go_do_retry(state->args.ecam_address,
                    state->args.flags, keyset_id, state->args.handle, &res);
            else
                val_firme_ide_keyset_stop_do_retry(state->args.ecam_address,
                    state->args.flags, keyset_id, state->args.handle, &res);
            completion = val_firme_ide_link_complete(state, &res);
            break;
        case VAL_IDE_LINK_RELEASE:
            if (state->programmed && !state->stopped)
                return;
            memset(state, 0, sizeof(*state));
            break;
        default:
            return;
        }
    }

result:
    out->x1 = res.x0;
    out->x2 = res.x1;
    if (completion == VAL_SUCCESS && val_firme_status_to_i64(res.x0) == FIRME_SUCCESS)
        out->x0 = ACS_STATUS_PASS;
}
