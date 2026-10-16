/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_abi_frame.h"

typedef struct {
    firme_acs_world_t world;
    uint32_t owner_pe;
    uint32_t sequence;
    uint32_t initialized;
    uint32_t quarantined;
    val_firme_abi_frame_outcome_t outcome;
} val_host_abi_frame_context_t;

/**
 * @brief Bind a callback cookie to the current physical PE without resetting tokens.
 * @param context Persistent caller-owned cookie, valid until invocation completes.
 * @return VAL_SUCCESS when the PE transport is usable, otherwise VAL_ERROR.
 */
int val_host_abi_frame_init(val_host_abi_frame_context_t *context);

/**
 * @brief Bind a cookie to an explicit Secure or Realm target on this physical PE.
 * @param context Persistent caller-owned cookie retained through recovery/cleanup.
 * @param world Target security state; Normal world and unknown values are rejected.
 * @return VAL_SUCCESS for an available world/PE slot, otherwise VAL_ERROR.
 */
int val_host_abi_frame_init_world(val_host_abi_frame_context_t *context,
                                 firme_acs_world_t world);

/**
 * @brief Invoke one target-local SMC with all x0-x17 inputs and outputs by value.
 * @param input Immutable full register frame; only discovery/GM/IDE FIDs are allowed.
 * @param output Populated only after all outputs and slot release are acknowledged.
 * @param issued VAL_TRUE once EXECUTE may have reached its target, even after error.
 * @param cookie Context initialized for the current PE and intended target world.
 * @return VAL_SUCCESS means output-valid. VAL_ERROR with issued true requires
 *         fixture retention: execution or result/release recovery is uncertain.
 *         A quarantined world/PE slot cannot be reset by replacing cookies.
 */
int val_host_abi_frame_invoke(const smccc_call_frame_t *input,
                             smccc_call_frame_t *output, uint32_t *issued,
                             void *cookie);
