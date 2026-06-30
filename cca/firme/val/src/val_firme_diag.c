/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_diag.h"

#include "val_firme_abi.h"
#include "val_common/val_fault.h"

/**
 *   @brief    - Run a small diagnostic test inside the current world payload.
 *   @param    - test_id : Test selector.
 *   @param    - a0      : Test argument 0.
 *   @param    - a1      : Test argument 1.
 *   @param    - a2      : Test argument 2.
 *   @param    - a3      : Test argument 3.
 *   @param    - out     : Optional output structure.
 *   @return   - ACS_STATUS_PASS on success, ACS_STATUS_ERR on unknown test_id,
 *               ACS_STATUS_SKIP when skipped by convention.
**/
int val_firme_diag_run(uint64_t test_id, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                       firme_diag_out_t *out)
{
    if (out) {
        out->x0 = 0;
        out->x1 = 0;
        out->x2 = 0;
        out->x3 = 0;
    }

    if (test_id == 0) {
        firme_res_t v;
        val_firme_version(&v);
        if (out) {
            out->x0 = v.x0;
            out->x1 = v.x1;
            out->x2 = v.x2;
            out->x3 = v.x3;
        }
        return (int)ACS_STATUS_PASS;
    }

    if (test_id == 1) {
        firme_res_t f;
        /*
         * FIRME_FEATURES takes a feature register index (w1): 0..2.
         * (Not a function ID.)
         */
        val_firme_features(a0, &f);
        if (out) {
            out->x0 = f.x0;
            out->x1 = f.x1;
            out->x2 = f.x2;
            out->x3 = f.x3;
        }
        return (int)ACS_STATUS_PASS;
    }

    if (test_id == 2) {
        val_fault_info_t fi = {0};
        uint64_t v = 0;
        int rc = 0;

        (void)a1;
        (void)a2;
        (void)a3;

        rc = val_probe_read64(a0, &v, &fi);

        if (out) {
            out->x0 = (uint64_t)(rc != 0 ? 1u : 0u); /* faulted */
            out->x1 = (rc != 0) ? fi.esr : v;
            out->x2 = fi.far;
            out->x3 = (rc == 0) ? v : 0;
        }
        return (int)ACS_STATUS_PASS;
    }

    return (int)ACS_STATUS_ERR;
}
