/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_dispatch.h"
#include "val_common/val_smccc.h"
#include "val_common/val_status.h"

#define VAL_ABI_FRAME_OPERATION_MASK UINT32_C(0x3f)
#define VAL_ABI_FRAME_WORLD_SHIFT 6U
#define VAL_ABI_FRAME_WORLD_MASK UINT32_C(0x3)
#define VAL_ABI_FRAME_WORLD_COUNT 2U
#define VAL_ABI_FRAME_WORD_SHIFT 8U
#define VAL_ABI_FRAME_WORD_MASK UINT32_C(0xff)
#define VAL_ABI_FRAME_OWNER_SHIFT 16U
#define VAL_ABI_FRAME_OWNER_MAX UINT32_C(0xffff)
#define VAL_ABI_FRAME_RETRY_COUNT 3U

typedef enum {
    VAL_ABI_FRAME_BEGIN = 1U,
    VAL_ABI_FRAME_WRITE,
    VAL_ABI_FRAME_EXECUTE,
    VAL_ABI_FRAME_READ,
    VAL_ABI_FRAME_RELEASE,
} val_firme_abi_frame_operation_t;

typedef enum {
    VAL_ABI_FRAME_RELEASED = 0U,
    VAL_ABI_FRAME_UPLOADING,
    VAL_ABI_FRAME_EXECUTING,
    VAL_ABI_FRAME_COMPLETE,
} val_firme_abi_frame_phase_t;

typedef enum {
    VAL_ABI_FRAME_UNISSUED = 0U,
    VAL_ABI_FRAME_UNCERTAIN,
    VAL_ABI_FRAME_OUTPUT_VALID,
} val_firme_abi_frame_outcome_t;

/**
 * @brief Limit remote full-frame execution to discovery, granule and IDE management.
 * @param fid Exact SMC function identifier, without truncation or normalization.
 * @return VAL_TRUE for an allowlisted identifier, otherwise VAL_FALSE.
 */
int val_firme_abi_frame_fid_allowed(uint64_t fid);

/**
 * @brief Process one by-value frame command in the selected world-private PE slot.
 * @param world Actual receiving world, supplied by the Secure or Realm handler.
 * @param control Operation [5:0], world [7:6], word [15:8], owner PE [31:16].
 * @param sequence Nonzero, monotonically increasing 32-bit transaction identity.
 * @param low Lower word half for WRITE, otherwise zero.
 * @param high Upper word half for WRITE, otherwise zero.
 * @param response ACS status plus two RMMD-safe payload words; no shared pointer.
 */
void val_firme_abi_frame_command(firme_acs_world_t world, uint64_t control, uint64_t sequence,
                                uint64_t low, uint64_t high,
                                firme_acs_call_res_t *response);
