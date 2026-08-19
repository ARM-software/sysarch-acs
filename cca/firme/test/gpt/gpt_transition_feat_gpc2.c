/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

/* Fail-step bases for cross-world enforcement matrix checks. */
#define GPT_TRANS_FEAT_GPC2_MATRIX_BASELINE_STEP_BASE UINT64_C(10)
#define GPT_TRANS_FEAT_GPC2_MATRIX_NSO_STEP_BASE      UINT64_C(20)
#define GPT_TRANS_FEAT_GPC2_MATRIX_RESTORE_STEP_BASE  UINT64_C(30)

#define GPT_TRANS_FEAT_GPC2_TEST_NAME "gpt_transition_feat_gpc2"

/**
 *   @brief    - GPT transition policy (FEAT_RME_GPC2): NS<->NSO transitions.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - 0 (status stored in `out->status` when provided).
**/
int firme_gpt_transition_feat_gpc2_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    uint32_t gpc2 = 0;
    firme_res_t res = (firme_res_t){0};
    val_fault_info_t fault_info = (val_fault_info_t){0};
    const uint64_t test_data = FIRME_TEST_DATA_PATTERN;
    const uint64_t granule_count = 1ULL;
    const uint64_t expected_done = granule_count;
    const uint32_t check_value = 1U;
    const uint64_t clear_value = 0ULL;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return 0;
    }

    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return 0;

    gpc2 = val_firme_feat_rme_gpc2_present();

    /* Step 1: seed the test granule from NS. */
    if (val_probe_write64(base_pa, test_data, &fault_info) != 0) {
        LOG_ERROR(NULL, "%s: baseline write fault esr=%llx far=%llx",
                  GPT_TRANS_FEAT_GPC2_TEST_NAME,
                  (unsigned long long)fault_info.esr, (unsigned long long)fault_info.far);
        if (out) {
            out->fail_step = 0;
            out->last_x0 = fault_info.esr;
            out->last_x1 = fault_info.far;
        }
        goto cleanup;
    }

    /* Step 2: validate baseline enforcement for Non-secure GPI. */
    if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE, test_data, check_value, out,
                                       GPT_TRANS_FEAT_GPC2_MATRIX_BASELINE_STEP_BASE)) {
        if (out) {
            LOG_ERROR(NULL, "%s: baseline matrix failed step=%llu x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GPC2_TEST_NAME,
                      (unsigned long long)out->fail_step,
                      (unsigned long long)out->last_x0,
                      (unsigned long long)out->last_x1);
        }
        goto cleanup;
    }

    /* Step 3: request NS->NSO (FEAT_RME_GPC2) and validate enforcement. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NSO, &res);
    if (gpc2) {
        if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
            LOG_ERROR(NULL, "%s: NS->NSO failed x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GPC2_TEST_NAME,
                      (unsigned long long)res.x0, (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 1;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }

        if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NSO, test_data, check_value, out,
                                           GPT_TRANS_FEAT_GPC2_MATRIX_NSO_STEP_BASE)) {
            if (out) {
                LOG_ERROR(NULL, "%s: NSO matrix failed step=%llu x0=%llx x1=%llx",
                          GPT_TRANS_FEAT_GPC2_TEST_NAME,
                          (unsigned long long)out->fail_step,
                          (unsigned long long)out->last_x0,
                          (unsigned long long)out->last_x1);
            }
            goto cleanup;
        }

        /* Step 4: restore NS and validate enforcement. */
        val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
        if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
            LOG_ERROR(NULL, "%s: NSO->NS failed x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GPC2_TEST_NAME,
                      (unsigned long long)res.x0, (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 2;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }

        if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE,
                                           test_data, check_value, out,
                                           GPT_TRANS_FEAT_GPC2_MATRIX_RESTORE_STEP_BASE)) {
            if (out) {
                LOG_ERROR(NULL,
                          "%s: NS restore matrix failed step=%llu x0=%llx x1=%llx",
                          GPT_TRANS_FEAT_GPC2_TEST_NAME,
                          (unsigned long long)out->fail_step,
                          (unsigned long long)out->last_x0,
                          (unsigned long long)out->last_x1);
            }
            goto cleanup;
        }
    } else {
        if (!val_firme_status_deniedish(res.x0) && !val_firme_status_not_supported(res.x0)) {
            LOG_ERROR(NULL, "%s: NS->NSO expected DENIED/NSUP x0=%llx x1=%llx",
                      GPT_TRANS_FEAT_GPC2_TEST_NAME,
                      (unsigned long long)res.x0, (unsigned long long)res.x1);
            if (out) {
                out->fail_step = 3;
                out->last_x0 = res.x0;
                out->last_x1 = res.x1;
            }
            goto cleanup;
        }
    }

    if (out)
        out->status = (uint64_t)ACS_STATUS_PASS;

cleanup:
    /* Best-effort cleanup: restore to NS and clear test data. */
    val_firme_gm_gpi_set_complete(base_pa, granule_count, FIRME_GPI_NONSECURE, &res);
    val_probe_write64(base_pa, clear_value, &fault_info);
    return 0;
}
