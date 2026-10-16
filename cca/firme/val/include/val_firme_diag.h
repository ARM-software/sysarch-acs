/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

#include <stdint.h>

#include "val_common/val_status.h"

#define FIRME_DIAG_READ_NS_PAS UINT64_C(3)

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
} firme_diag_out_t;

/*
 * Diagnostics runner shared by all worlds.
 *
 * Args are test-specific:
 * - test 0: ignores a0..a2
 * - test 1: a0 = feature register index
 * - test 2: a0 = address to probe (64-bit load)
 *
 * Return value uses ACS-style status codes:
 *   ACS_STATUS_PASS = handled
 *   ACS_STATUS_SKIP = skipped by convention (e.g. missing platform window)
 *   ACS_STATUS_ERR  = unknown test id / internal error
 */
int val_firme_diag_run(uint64_t test_id, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                       firme_diag_out_t *out);
