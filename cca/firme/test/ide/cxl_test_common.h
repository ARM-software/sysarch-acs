/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include "val_firme_ide.h"

typedef struct {
    uint64_t ecam_address;
    uint64_t keyset_id;
    uint64_t pending_handle;
    uint32_t acquired;
    uint32_t programmed;
    uint32_t pending;
    uint32_t unknown;
} firme_ide_cxl_state_t;

int firme_ide_cxl_begin(firme_ide_cxl_state_t *state, uint64_t keyset_id);
int firme_ide_cxl_prog(firme_ide_cxl_state_t *state, uint64_t handle, firme_res_t *res);
int firme_ide_cxl_go(firme_ide_cxl_state_t *state, uint64_t handle, firme_res_t *res);
void firme_ide_cxl_rejected(firme_ide_cxl_state_t *state, const firme_res_t *res);
int firme_ide_cxl_finish(firme_ide_cxl_state_t *state, uint64_t handle, firme_res_t *res);
