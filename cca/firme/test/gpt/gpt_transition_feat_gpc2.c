/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_gpt.h"
#include "val_firme_rules.h"
#include "val_log.h"

#include <stddef.h>

/* Fail-step bases for cross-world enforcement matrix checks. */
#define GPT_TRANS_FEAT_GPC2_MATRIX_BASELINE_STEP_BASE UINT64_C(10)
#define GPT_TRANS_FEAT_GPC2_MATRIX_NSO_STEP_BASE      UINT64_C(20)
#define GPT_TRANS_FEAT_GPC2_MATRIX_RESTORE_STEP_BASE  UINT64_C(30)
#define GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE         UINT64_C(40)

#define GPT_TRANS_FEAT_GPC2_TEST_NAME "gpt_transition_feat_gpc2"

/**
 *   @brief    - GPT transition policy (FEAT_RME_GPC2): NS<->NSO transitions.
 *   @details  - The access matrix includes a Secure load to the same PA using
 *               a Non-secure output PAS: allowed before transition, GPF in NSO,
 *               and allowed after restoration. Native-PAS loads alone cannot
 *               distinguish NS from NSO.
 *   @param    - base_pa : Base PA for the GPT test window.
 *   @param    - out     : Optional result record.
 *   @return   - VAL_SUCCESS only for PASS or SKIP, otherwise VAL_ERROR.
**/
int firme_gpt_transition_feat_gpc2_run_ns(uint64_t base_pa, firme_rule_res_t *out)
{
    uint32_t gpc2 = 0;
    uint32_t nso_owned = VAL_FALSE;
    firme_res_t res = (firme_res_t){0};
    val_fault_info_t fault_info = (val_fault_info_t){0};
    const uint64_t test_data = FIRME_TEST_DATA_PATTERN;
    const uint64_t expected_done = 1ULL;
    const uint32_t check_value = 1U;
    const uint64_t clear_value = 0ULL;

    if (!out)
        return VAL_ERROR;

    if (out) {
        out->status = (uint64_t)ACS_STATUS_FAIL;
        out->fail_step = 0;
        out->last_x0 = 0;
        out->last_x1 = 0;
    }

    if (!val_firme_feat_rme_present() || base_pa == 0ULL) {
        if (out)
            out->status = (uint64_t)ACS_STATUS_SKIP;
        return val_firme_gpt_rule_result_status(out);
    }

    if (val_firme_gpt_gpi_set_advertised(out, UINT64_C(1000)) == VAL_FALSE)
        return val_firme_gpt_rule_result_status(out);

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
    if (val_firme_gpt_transition_terminal(base_pa, FIRME_GPI_NSO, &res, out, UINT64_C(1)) !=
        VAL_SUCCESS)
        return VAL_ERROR;

    nso_owned = val_firme_status_success(res.x0, res.x1, expected_done);
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

        /* Step 4: scrub the entire NSO granule to PoC before releasing it to NS. */
        if (val_firme_gpt_scrub_ns(base_pa, out,
                                   GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE) == VAL_FALSE)
            return VAL_ERROR;
        if (val_firme_gpt_transition_terminal(base_pa, FIRME_GPI_NONSECURE, &res, out,
                                              UINT64_C(2)) !=
            VAL_SUCCESS)
            return VAL_ERROR;

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

        nso_owned = VAL_FALSE;
        if (!val_firme_check_access_matrix(base_pa, FIRME_GPI_NONSECURE,
                                           clear_value, check_value, out,
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
        if (!val_firme_status_invalid_parameters(res.x0)) {
            LOG_ERROR(NULL, "%s: NS->NSO expected INVALID_PARAMETERS x0=%llx x1=%llx",
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
    /* Step 5: Confirm NS independently instead of issuing a redundant same-GPI request. */
    if (nso_owned == VAL_FALSE) {
        if (!val_firme_gpt_confirm_ns_restored(base_pa, out,
                                              GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE))
            return VAL_ERROR;

        return val_firme_gpt_rule_result_status(out);
    }

    if ((nso_owned == VAL_TRUE) &&
        (val_firme_gpt_scrub_ns(base_pa, out,
                               GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE) == VAL_FALSE))
        return VAL_ERROR;
    if (val_firme_gpt_transition_terminal(base_pa, FIRME_GPI_NONSECURE, &res, out,
                            GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE) != VAL_SUCCESS)
        return VAL_ERROR;

    if (!val_firme_status_success(res.x0, res.x1, expected_done)) {
        LOG_ERROR(NULL, "%s: NS cleanup failed x0=%llx x1=%llx",
                  GPT_TRANS_FEAT_GPC2_TEST_NAME,
                  (unsigned long long)res.x0, (unsigned long long)res.x1);
        if (((out->status != ACS_STATUS_FAIL) && (out->status != ACS_STATUS_ERR)) ||
            out->fail_step == 0ULL)
            val_firme_gpt_rule_result_fail(out, GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE,
                                           res.x0, res.x1);

        out->status = ACS_STATUS_ERR;
        return VAL_ERROR;
    }

    if (!val_firme_gpt_confirm_ns_restored(base_pa, out,
            GPT_TRANS_FEAT_GPC2_CLEANUP_STEP_BASE + UINT64_C(1)))
        return VAL_ERROR;

    return val_firme_gpt_rule_result_status(out);
}
